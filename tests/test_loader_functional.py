"""Cross-language verification of native license validation and signed installation."""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest
import ctypes
from ctypes import wintypes
import shutil
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'scripts'))
from license_admin import issue, load_key, sign
from cryptography.hazmat.primitives.asymmetric import ec

class LoaderTests(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory(prefix='zb2-loader-tests-')
        self.root = Path(self.folder.name)
        self.now = int(time.time())
        self.device = '1'*64
        self.key = load_key(KEY)

    def tearDown(self):
        self.folder.cleanup()

    def call(self, *args):
        return subprocess.run([str(EXE), *map(str,args)],capture_output=True,text=True,encoding='utf-8',errors='replace')

    def validate(self, token, device=None, now=None):
        path = self.root/'license.txt'
        path.write_text(token,encoding='ascii')
        return self.call('validate',path,device or self.device,self.now if now is None else now)

    def test_valid_signed_license(self):
        result = self.validate(issue(self.key,self.device,1,self.now))
        self.assertEqual(result.returncode,0,result.stderr)

    def test_wrong_device(self):
        self.assertEqual(self.validate(issue(self.key,self.device,1,self.now),device='2'*64).returncode,2)

    def test_expiry_boundary(self):
        token=issue(self.key,self.device,1,self.now)
        self.assertEqual(self.validate(token,now=self.now+86400-1).returncode,0)
        self.assertEqual(self.validate(token,now=self.now+86400).returncode,2)

    def test_not_yet_valid(self):
        self.assertEqual(self.validate(issue(self.key,self.device,1,self.now+1)).returncode,2)

    def test_tampered_payload(self):
        token=issue(self.key,self.device,1,self.now)
        prefix,payload,signature=token.split('.')
        payload=bytes.fromhex(payload).replace(b'Menu-ZB2',b'Menu-ZB3').hex()
        self.assertEqual(self.validate('.'.join([prefix,payload,signature])).returncode,2)

    def test_signature_from_other_issuer(self):
        other=ec.generate_private_key(ec.SECP256R1())
        self.assertEqual(self.validate(issue(other,self.device,1,self.now)).returncode,2)

    def test_malformed_but_signed_claims(self):
        valid=f'ZB2-LICENSE-1\nMenu-ZB2\n{"a"*32}\n{self.device}\n{self.now}\n{self.now+86400}\n'
        variants=[valid+'extra\n',valid.rstrip('\n'),valid.replace('Menu-ZB2','Other'),
                  valid.replace(str(self.now)+'\n','0'+str(self.now)+'\n'),valid.replace(self.device,'g'*64)]
        for payload in variants:
            data=payload.encode('ascii')
            token='ZB2L1.'+data.hex()+'.'+sign(self.key,data).hex()
            with self.subTest(payload=payload):
                self.assertEqual(self.validate(token).returncode,2)

    def test_truncated_and_oversized_inputs(self):
        for token in ['', 'ZB2L1.', 'ZB2L1.xx.yy', 'ZB2L1.'+'f'*2048, 'X'*8192]:
            with self.subTest(size=len(token)):
                self.assertEqual(self.validate(token).returncode,2)

    def test_install_verify_and_reject_corruption(self):
        first=self.call('install',self.root)
        self.assertEqual(first.returncode,0,first.stderr)
        path=Path(first.stdout)
        self.assertEqual(len(list(path.iterdir())),7)
        self.assertEqual(self.call('install',self.root).returncode,0)
        with (path/'config.ini').open('ab') as stream:
            stream.write(b'\ntampered')
        self.assertNotEqual(self.call('install',self.root).returncode,0)

    def test_dpapi_roundtrip_corruption_and_clock_rollback(self):
        result=self.call('state',self.root)
        self.assertEqual(result.returncode,0,result.stderr)

    def test_unicode_and_spaces_in_install_path(self):
        destination=self.root/'Pasta de teste 日本 ação'
        result=self.call('install',destination)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertTrue(Path(result.stdout).is_dir())
        self.assertTrue((Path(result.stdout)/'kiero-dx11-base.dll').is_file())

    def test_device_identifier_is_stable(self):
        first=self.call('device')
        self.assertEqual(first.returncode,0,first.stderr)
        self.assertRegex(first.stdout,'^[0-9a-f]{64}$')
        self.assertEqual(first.stdout,self.call('device').stdout)

    def test_signed_package_rejects_modified_resources(self):
        api=ctypes.WinDLL('kernel32',use_last_error=True)
        api.BeginUpdateResourceW.argtypes=[wintypes.LPCWSTR,wintypes.BOOL]
        api.BeginUpdateResourceW.restype=ctypes.c_void_p
        api.UpdateResourceW.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_void_p,wintypes.WORD,ctypes.c_void_p,wintypes.DWORD]
        api.EndUpdateResourceW.argtypes=[ctypes.c_void_p,wintypes.BOOL]
        for resource in [103,104,301]:
            with self.subTest(resource=resource):
                altered=self.root/f'altered-{resource}.exe'
                shutil.copyfile(EXE,altered)
                handle=api.BeginUpdateResourceW(str(altered),False)
                self.assertTrue(handle)
                try:
                    data=ctypes.create_string_buffer(b'corrupted')
                    self.assertTrue(api.UpdateResourceW(handle,10,resource,1033,data,9))
                except BaseException:
                    api.EndUpdateResourceW(handle,True)
                    raise
                self.assertTrue(api.EndUpdateResourceW(handle,False))
                result=subprocess.run([str(altered),'install',str(self.root/f'install-{resource}')],capture_output=True)
                self.assertEqual(result.returncode,1,result.stderr)
                self.assertFalse((self.root/f'install-{resource}').exists())

    def test_reparse_destination_rejected(self):
        import _winapi
        target=self.root/'target'
        target.mkdir()
        link=self.root/'redirected'
        _winapi.CreateJunction(str(target),str(link))
        try:
            self.assertNotEqual(self.call('install',link).returncode,0)
            self.assertEqual(list(target.iterdir()),[])
        finally:
            link.rmdir()

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--exe',required=True,type=Path)
    parser.add_argument('--key',required=True,type=Path)
    args,remaining=parser.parse_known_args()
    EXE,KEY=args.exe.resolve(),args.key.resolve()
    unittest.main(argv=[sys.argv[0],*remaining])
