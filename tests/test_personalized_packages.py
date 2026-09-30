"""Validate customer/team ready ZIPs without launching either product or game."""
from pathlib import Path
import argparse
import subprocess
import tempfile
import struct
import json
import hashlib
import time
import zipfile
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--key',required=True,type=Path);args=parser.parse_args()
exe=ROOT/'build/admin-native/admin-native-tests.exe'
service=ROOT/'build/admin-native/ZB2AdminBackend.exe'
loader=ROOT/'build/loader/loader-tests.exe'
checks=0
def rpc(root,command,*fields,valid=True):
    global checks
    r=subprocess.run([str(exe),'rpc',str(root),command,*map(str,fields)],capture_output=True,text=True,encoding='utf-8',errors='replace')
    assert (r.returncode==0)==valid,(command,r.stdout,r.stderr);checks+=1
    return [bytes.fromhex(x).decode('utf-8') for x in r.stdout.split('\t')[1:]] if valid else None
def automatic(root,authorization):
    global checks
    data=b'snapshot'
    r=subprocess.run([str(service),'--data',str(root),'--authorization',str(authorization)],input=struct.pack('<i',len(data))+data,capture_output=True)
    size=struct.unpack('<i',r.stdout[:4])[0]
    output=r.stdout[4:4+size].decode('utf-8');checks+=1;return output
with tempfile.TemporaryDirectory(prefix='packages-',dir=ROOT/'build/admin-native') as temp:
    root=Path(temp);owner=root/'owner';member=root/'member'
    rpc(owner,'import_owner',args.key,'WeFagundes')
    archive=root/'Cliente ação.zip'
    result=rpc(owner,'issue_package','Cliente pronto','1'*64,'7',archive)
    assert archive.exists() and result[3].startswith('ZB2L1.')
    with zipfile.ZipFile(archive) as z:
        assert set(z.namelist())=={'ZB2Menu.exe','licenca.zb2license'} and z.testzip() is None
        assert hashlib.sha256(z.read('ZB2Menu.exe')).digest()==hashlib.sha256((ROOT/'dist/loader/ZB2Menu.exe').read_bytes()).digest()
        customer=root/'cliente';customer.mkdir();z.extractall(customer)
    for device,code in [('1'*64,0),('2'*64,2)]:
        r=subprocess.run([str(loader),'adjacent',str(customer/'ZB2Menu.exe'),device,str(int(time.time()))],capture_output=True)
        assert r.returncode==code,r.stderr;checks+=1
    local=root/'client-state'
    imported=subprocess.run([str(loader),'import-package',str(local),str(customer/'ZB2Menu.exe'),'1'*64,str(int(time.time()))],capture_output=True)
    assert imported.returncode==0 and (local/'license.dat').exists();checks+=1
    saved=(local/'license.dat').read_bytes()
    (customer/'licenca.zb2license').write_text('invalid',encoding='ascii')
    valid_local=subprocess.run([str(loader),'import-package',str(local),str(customer/'ZB2Menu.exe'),'1'*64,str(int(time.time()))],capture_output=True)
    assert valid_local.returncode==0 and (local/'license.dat').read_bytes()==saved;checks+=1
    records=list((owner/'licenses').glob('*.json'));assert len(records)==1
    record=json.loads(records[0].read_text(encoding='utf-8'))
    rpc(owner,'client_package',record['Id'],archive);assert len(list((owner/'licenses').glob('*.json')))==1
    rpc(owner,'issue_package','Cliente','1'*64,'7',root/'missing'/'client.zip',valid=False)
    assert len(list((owner/'licenses').glob('*.json')))==1
    rpc(owner,'issue_package','Renovacao','1'*64,'30',archive)
    with zipfile.ZipFile(archive) as z:z.extractall(customer)
    renewed=subprocess.run([str(loader),'import-package',str(local),str(customer/'ZB2Menu.exe'),'1'*64,str(int(time.time()))],capture_output=True)
    assert renewed.returncode==0 and int(renewed.stdout)>int(imported.stdout);checks+=1
    request=rpc(member,'create_station','Integrante teste')[3]
    teamzip=root/'Equipe.zip'
    approved=rpc(owner,'authorize_package',request,'365','30',teamzip)
    with zipfile.ZipFile(teamzip) as z:
        assert set(z.namelist())=={'ZB2Admin.exe','autorizacao.zb2issuer'} and z.testzip() is None
        team=root/'equipe';team.mkdir();z.extractall(team)
    before=json.loads((member/'station.json').read_text(encoding='utf-8'));assert not before['Certificate']
    output=automatic(member,team/'autorizacao.zb2issuer');assert output.startswith('OK\t')
    after=json.loads((member/'station.json').read_text(encoding='utf-8'));assert after['Certificate']==approved[3]
    assert after['ProtectedKey']==before['ProtectedKey']
    unchanged=(member/'station.json').read_bytes();automatic(member,team/'autorizacao.zb2issuer');assert unchanged==(member/'station.json').read_bytes()
    stranger=root/'stranger';rpc(stranger,'create_station','Outro PC');automatic(stranger,team/'autorizacao.zb2issuer')
    assert not json.loads((stranger/'station.json').read_text(encoding='utf-8'))['Certificate']
    rpc(member,'team_package',json.loads(request)['Id'],root/'forbidden.zip',valid=False)
    assert not (root/'forbidden.zip').exists()
print(checks,'personalized ZIP, automatic import, wrong-PC and history checks passed.')
