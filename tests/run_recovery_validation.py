"""Exercise recovery using a disposable authority, never the production key."""
from pathlib import Path
import argparse,json,subprocess,tempfile,hashlib,struct
ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--keep-cross-user-fixture',type=Path);args=parser.parse_args()
    out=ROOT/'build/recovery-tests';out.mkdir(parents=True,exist_ok=True)
    exe=out/'recovery-tests.exe';compiler=Path('C:/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe')
    subprocess.run([str(compiler),'/nologo','/target:exe','/platform:x64','/warnaserror+','/out:'+str(exe),'/r:System.Core.dll','/r:System.Security.dll','/r:System.Web.Extensions.dll',*[str(ROOT/p) for p in ['apps/admin/Core.cs','apps/admin/Recovery.cs','tests/RecoveryTests.cs']]],check=True)
    with tempfile.TemporaryDirectory(prefix='recovery-',dir=out) as temp:
        root=Path(temp);assert root.resolve().is_relative_to(out.resolve())
        for command in ['create','test']:subprocess.run([str(exe),command,str(root)],check=True)
        # Verify the envelope independently from its C# implementation.
        from cryptography.hazmat.primitives.ciphers import Cipher,algorithms,modes
        from cryptography.hazmat.primitives.padding import PKCS7
        from cryptography.hazmat.primitives import hashes,hmac
        from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
        data=(root/'portable.dbrecovery').read_bytes();assert data[:8]==b'DBREC001'
        rounds=struct.unpack_from('<I',data,8)[0];assert rounds==600000
        key=PBKDF2HMAC(algorithm=hashes.SHA256(),length=64,salt=data[12:28],iterations=rounds).derive(b'Test-only recovery phrase 2026!')
        mac=hmac.HMAC(key[32:],hashes.SHA256());mac.update(data[:-32]);mac.verify(data[-32:])
        decoder=Cipher(algorithms.AES(key[:32]),modes.CBC(data[28:44])).decryptor();padded=decoder.update(data[48:-32])+decoder.finalize();unpad=PKCS7(128).unpadder();plain=unpad.update(padded)+unpad.finalize();assert b'Menu-ZB2' in plain and b'RecoveryFixture' in plain
        # Simulate losing the original installation, only inside this temporary fixture.
        original=root/'original';allowed=[original/'station.json',root/'fixture.dpapi']
        for file in allowed:
            assert file.resolve().is_relative_to(root.resolve());file.unlink()
        original.rmdir()
        subprocess.run([str(exe),'recover-after-loss',str(root)],check=True)
        print('Independent envelope verification and loss-of-source simulation passed.')
    if args.keep_cross_user_fixture:
        folder=args.keep_cross_user_fixture.resolve();assert folder.is_relative_to(out.resolve()) and not folder.exists()
        subprocess.run([str(exe),'create',str(folder)],check=True)
        print('Disposable cross-user fixture prepared. Run recovery-tests.exe cross-user under a different Windows account.')
if __name__=='__main__':main()
