from pathlib import Path
import os,subprocess,tempfile
r=Path(__file__).resolve().parents[1];out=r/'build/launcher-tests';out.mkdir(parents=True,exist_ok=True)
csc=Path(os.environ['WINDIR'])/'Microsoft.NET/Framework64/v4.0.30319/csc.exe';exe=out/'package_tests.exe'
cmd=[str(csc),'/nologo','/warnaserror+','/out:'+str(exe),'/reference:System.Web.Extensions.dll','/reference:System.IO.Compression.dll',str(r/'launcher/PackageStore.cs'),str(r/'tests/launcher_package_tests.cs')]
log=[]
with tempfile.TemporaryDirectory(prefix='package-',dir=out) as directory:
 for command in [cmd,[str(exe),directory]]:
  p=subprocess.run(command,cwd=out,capture_output=True,text=True);log.append(p.stdout+p.stderr);print(log[-1],flush=True)
  (out/'results.log').write_text('\n'.join(log),encoding='utf-8')
  if p.returncode:raise SystemExit(p.returncode)
