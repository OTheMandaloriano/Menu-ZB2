import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from zipfile import ZipFile

spec=importlib.util.spec_from_file_location('package_runtime',Path(__file__).resolve().parents[1]/'scripts/package_runtime.py')
package=importlib.util.module_from_spec(spec);spec.loader.exec_module(package)

class PackageTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
        self.root=Path(self.tmp.name);self.source=self.root/'source';self.source.mkdir()
        for name in package.file_names():(self.source/name).write_bytes(('data:'+name).encode())
        (self.source/'private.pdb').write_bytes(b'not for distribution')
        (self.source/'credentials.env').write_bytes(b'not for distribution')
        self.output=self.root/'bundle.zip'
    def build(self):return package.build(self.source,self.output,'0.1-preview','a'*40)
    def test_allowlist(self):
        self.build()
        with ZipFile(self.output) as z:
            self.assertNotIn('private.pdb',z.namelist());self.assertNotIn('credentials.env',z.namelist())
            self.assertEqual(set(z.namelist()),set(package.file_names())|{'manifest.json'})
    def test_deterministic(self):
        self.build();first=self.output.read_bytes();self.build();self.assertEqual(first,self.output.read_bytes())
    def test_missing_dependency(self):
        (self.source/'0Harmony.dll').unlink()
        with self.assertRaises(ValueError):self.build()
        self.assertFalse(self.output.exists())
    def test_mutation(self):
        self.build()
        with ZipFile(self.output) as z:entries={n:z.read(n) for n in z.namelist()}
        entries['injector.exe']=b'changed'
        with ZipFile(self.output,'w') as z:
            for n,b in entries.items():z.writestr(n,b)
        with self.assertRaises(ValueError):package.verify(self.output)
    def test_extra_path(self):
        self.build()
        with ZipFile(self.output,'a') as z:z.writestr('../escape.dll',b'bad')
        with self.assertRaises(ValueError):package.verify(self.output)
    def test_invalid_version(self):
        with self.assertRaises(ValueError):package.build(self.source,self.output,'../bad','a'*40)
    def test_output_not_input(self):
        with self.assertRaises(ValueError):package.build(self.source,self.source/'bundle.zip','preview','a'*40)
    def test_explicit_unsigned(self):
        manifest=self.build();self.assertEqual(manifest['signature_status'],'unsigned-development')

if __name__=='__main__':unittest.main()
