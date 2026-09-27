"""Build a single executable with an embedded verified runtime package."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,zipfile
r=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--runtime',type=Path,required=True);p.add_argument('--version',default='0.2.0');p.add_argument('--output',type=Path,default=r/'build/launcher');a=p.parse_args()
if not __import__('re').fullmatch(r'\d+\.\d+\.\d+',a.version):raise SystemExit('Use numeric major.minor.patch')
names=['injector.exe','config.ini','kiero-dx11-base.dll','Zb2.AimBridge.dll','0Harmony.dll','Harmony.LICENSE']
a.output.mkdir(parents=True,exist_ok=True)
files={n:(a.runtime/n).read_bytes() for n in names}
if b'dll=kiero-dx11-base.dll' not in files['config.ini']:raise SystemExit('Injector config must be portable')
manifest={'schema':1,'version':a.version,'game_sha256':'c41a298975d35f0dad0a05531bce6e0b6e274d0ddf265217d65ce3ac5cbc84e1','files':{n:hashlib.sha256(b).hexdigest() for n,b in files.items()}}
package=a.output/'ZB2Menu-runtime.zip'
with zipfile.ZipFile(package,'w',zipfile.ZIP_DEFLATED) as z:
 for n,b in files.items():z.writestr(n,b)
 z.writestr('manifest.json',json.dumps(manifest,indent=2))
csc=Path(os.environ['WINDIR'])/'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
command=[str(csc),'/nologo','/target:winexe','/platform:x64','/optimize+','/warnaserror+','/out:'+str(a.output/'ZB2Menu.exe'),'/resource:'+str(package)+',runtime.zip']
command+=['/reference:'+name for name in ['System.Windows.Forms.dll','System.Drawing.dll','System.Net.Http.dll','System.Web.Extensions.dll','System.IO.Compression.dll','System.IO.Compression.FileSystem.dll','System.Security.dll']]
command+=[str(f) for f in sorted((r/'launcher').glob('*.cs'))]
result=subprocess.run(command,capture_output=True,text=True);print(result.stdout+result.stderr)
(a.output/'build.log').write_text(result.stdout+result.stderr,encoding='utf-8')
if result.returncode:raise SystemExit(result.returncode)
(a.output/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.name+'\n' for f in [package,a.output/'ZB2Menu.exe']),encoding='ascii')
print('Built launcher and package',a.version)
