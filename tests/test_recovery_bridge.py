"""Recovery commands through the production service pipe; secrets never in argv."""
from pathlib import Path
import argparse,json,struct,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--key',required=True,type=Path);args=parser.parse_args()
service=ROOT/'build/admin-native/ZB2AdminBackend.exe'
def rpc(root,command,*fields,valid=True):
    packet=(command+''.join('\t'+str(x).encode('utf-8').hex() for x in fields)).encode('ascii')
    p=subprocess.run([str(service),'--data',str(root)],input=struct.pack('<i',len(packet))+packet,capture_output=True)
    assert p.returncode==0 and len(p.stdout)>=4,'service failed'
    size=struct.unpack('<i',p.stdout[:4])[0];assert size==len(p.stdout)-4
    rows={}
    for line in p.stdout[4:].decode('utf-8').splitlines():
        row=line.split('\t');rows.setdefault(row[0],[]).append([bytes.fromhex(f).decode('utf-8') for f in row[1:]])
    assert ('ERR' not in rows)==valid,'unexpected command result: '+command
    return rows
with tempfile.TemporaryDirectory(prefix='recovery-bridge-',dir=ROOT/'build/admin-native') as temp:
    base=Path(temp);owner=base/'owner';target=base/'new-profile';backup=base/'owner.dbrecovery';password='Synthetic recovery bridge password!'
    rpc(owner,'import_owner',args.key,'OwnerFixture')
    before=(owner/'station.json').read_bytes()
    result=rpc(owner,'export_recovery',backup,password);assert backup.exists() and (owner/'station.json').read_bytes()==before
    assert password not in str(result) and 'X' not in result,'no secret echoed in protocol'
    rpc(target,'restore_recovery',backup,'Incorrect recovery bridge password',valid=False)
    assert not (target/'station.json').exists()
    restored=rpc(target,'restore_recovery',backup,password);assert restored['S'][0][0]=='owner' and restored['S'][0][3]=='3650'
    rpc(target,'issue','Recovery client','1'*64,'30');assert len(list((target/'licenses').glob('*.json')))==1
    after=(target/'station.json').read_bytes();rpc(target,'restore_recovery',backup,password,valid=False);assert (target/'station.json').read_bytes()==after
print('Recovery pipe checks passed: export, wrong password, restore, issue and no overwrite. No real profile touched.')
