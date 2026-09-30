"""Validate native UI -> framed pipe -> CNG service -> native license verifier."""
from pathlib import Path
import argparse
import json
import struct
import subprocess
import tempfile
import time
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--key',required=True,type=Path);args=parser.parse_args()
native=ROOT/'build/admin-native/admin-native-tests.exe'
verifier=ROOT/'build/loader/loader-tests.exe'
checks=0
def invoke(root,command,*fields,success=True):
    global checks
    result=subprocess.run([str(native),'rpc',str(root),command,*map(str,fields)],capture_output=True,text=True,encoding='utf-8',errors='replace')
    assert (result.returncode==0)==success,(command,result.stdout,result.stderr)
    checks+=1
    if not success:return None
    parts=result.stdout.split('\t');assert parts[0]=='RESULT'
    return [bytes.fromhex(item).decode('utf-8') for item in parts[1:]]
with tempfile.TemporaryDirectory(prefix='native-bridge-',dir=ROOT/'build/admin-native') as temporary:
    directory=Path(temporary);assert directory.resolve().is_relative_to((ROOT/'build/admin-native').resolve())
    owner=directory/'owner';member=directory/'member'
    initial=invoke(owner,'snapshot');assert initial[0]=='pending'
    configured=invoke(owner,'import_owner',args.key,'Proprietario teste');assert configured[0]=='owner' and configured[2]=='3650'
    issued=invoke(owner,'issue','Cliente via ImGui','1'*64,'7');assert issued[3].startswith('ZB2L1.')
    license_file=directory/'test.zb2license';license_file.write_text(issued[3],encoding='ascii')
    result=subprocess.run([str(verifier),'validate',str(license_file),'1'*64,str(int(time.time()))],capture_output=True);assert result.returncode==0
    created=invoke(member,'create_station','Integrante ImGui');assert created[0]=='operator'
    station_file=directory/'request.zb2station';station_file.write_text(created[3],encoding='utf-8')
    inspected=invoke(owner,'inspect_request',station_file);assert inspected[5]
    approved=invoke(owner,'authorize',inspected[5],'365','30');assert approved[3].startswith('ZB2I1.')
    grant_file=directory/'grant.zb2issuer';grant_file.write_text(approved[3],encoding='ascii')
    activated=invoke(member,'import_authorization',grant_file);assert activated[2]=='30'
    issued=invoke(member,'issue','Cliente delegado','1'*64,'30');assert issued[3].startswith('ZB2L2.')
    license_file.write_text(issued[3],encoding='ascii')
    result=subprocess.run([str(verifier),'validate',str(license_file),'1'*64,str(int(time.time()))],capture_output=True);assert result.returncode==0
    invoke(member,'issue','Cliente delegado','1'*64,'31',success=False)
    invoke(member,'authorize',inspected[5],'365','30',success=False)
    checked=invoke(member,'snapshot');assert checked[6]=='1'
    before=(member/'station.json').read_bytes();invoke(member,'snapshot');assert before==(member/'station.json').read_bytes()
    backend=ROOT/'build/admin-native/ZB2AdminBackend.exe'
    bad=subprocess.run([str(backend),'--data',str(member)],input=struct.pack('<i',200000),capture_output=True)
    count=struct.unpack('<i',bad.stdout[:4])[0];assert bad.stdout[4:4+count].startswith(b'ERR\t');checks+=1
print(checks,'native bridge and permission checks passed; existing station preserved.')
