"""Exercise the Windows admin issuer and native loader verifier together."""
import argparse
from pathlib import Path
import json
import subprocess
import sys
import tempfile
import time
import uuid
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from license_admin import load_key,issue,public_hex,sign
from cryptography.hazmat.primitives.asymmetric import ec
parser=argparse.ArgumentParser()
parser.add_argument('--key',required=True,type=Path)
args=parser.parse_args()
out=ROOT/'build/admin';out.mkdir(parents=True,exist_ok=True)
log=[]
def run(command,expected=0):
    result=subprocess.run(list(map(str,command)),capture_output=True,text=True,encoding='utf-8',errors='replace')
    log.append({'command':[str(x) for x in command if str(x)!=str(args.key)],'exit':result.returncode,'output':result.stdout+result.stderr})
    if result.returncode!=expected:raise AssertionError(log[-1])
    return result.stdout
compiler=Path('C:/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe')
run([compiler,'/nologo','/target:exe','/platform:x64','/warnaserror+',
     '/out:'+str(out/'admin-core-tests.exe'),'/r:System.Core.dll','/r:System.Security.dll','/r:System.Web.Extensions.dll',
     ROOT/'apps/admin/Core.cs',ROOT/'tests/AdminCoreTests.cs'])
checks=0
with tempfile.TemporaryDirectory(prefix='admin-validation-',dir=out) as folder:
    folder=Path(folder)
    print(run([out/'admin-core-tests.exe',args.key,ROOT/'packaging/loader-public-key.json',folder]).strip())
    if (out/'admin-ui-tests.exe').exists():
        print(run([out/'admin-ui-tests.exe',args.key,folder/'ui-owner']).strip())
    def validate(label,token,now,valid):
        global checks
        file=folder/(label+'.zb2license');file.write_text(token,encoding='utf-8')
        run([ROOT/'build/loader/loader-tests.exe','validate',file,'1'*64,now],0 if valid else 2)
        checks+=1
    now=int(time.time())
    for label,token in json.loads((folder/'fixtures.json').read_text()).items():
        validate(label,token,now,label!='over_limit')
    owner=load_key(args.key)
    delegate=ec.generate_private_key(ec.SECP256R1())
    other=ec.generate_private_key(ec.SECP256R1())
    def grant(issued,expires,max_days=30,master=owner,product='Menu-ZB2',public=None):
        data=f'ZB2-ISSUER-1\n{product}\n{uuid.uuid4().hex}\n{("Equipe de teste").encode().hex()}\n{public or public_hex(delegate)}\n{issued}\n{expires}\n{max_days}\n'.encode('ascii')
        return data.hex()+'.'+sign(master,data).hex()
    def token(days=7,issued=now,certificate=None,signer=delegate):
        return issue(signer,'1'*64,days,issued).replace('ZB2L1.','ZB2L2.',1)+'.'+(certificate or grant(now,now+365*86400))
    valid=token()
    validate('valid_chain',valid,now,True)
    validate('normalized','\ufeff  '+valid[:80]+'\r\n'+valid[80:]+'  \n',now,True)
    validate('other_authority',token(certificate=grant(now,now+365*86400,master=other)),now,False)
    validate('subject_key_substitution',token(signer=other),now,False)
    validate('wrong_product',token(certificate=grant(now,now+365*86400,product='Other')),now,False)
    validate('license_predates_grant',token(issued=now-1),now,False)
    validate('grant_not_yet_valid',token(issued=now+1,certificate=grant(now+1,now+365*86400)),now,False)
    validate('expired_grant',token(certificate=grant(now-30*86400,now-1)),now,False)
    validate('outlives_grant',token(days=5,issued=now+86400,certificate=grant(now,now+5*86400,max_days=5)),now+2*86400,False)
    parts=valid.split('.')
    parts[3]='00'+parts[3][2:]
    validate('tampered_grant','.'.join(parts),now,False)
    validate('missing_grant','.'.join(valid.split('.')[:3]),now,False)
(out/'validation.json').write_text(json.dumps({'native_cross_checks':checks,'results':log},indent=2),encoding='utf-8')
print(checks,'C#/Python -> native verifier checks passed.')
