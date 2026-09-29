"""Build the graphical team issuer using the Windows .NET Framework compiler."""
from pathlib import Path
import subprocess
import hashlib
import json
import shutil
import sys
ROOT=Path(__file__).resolve().parents[1]
output=ROOT/'build/admin'
output.mkdir(parents=True,exist_ok=True)
compiler=Path('C:/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe')
assets=ROOT/'apps/loader/assets'
command=[str(compiler),'/nologo','/target:winexe','/platform:x64','/optimize+','/warnaserror+',
         '/out:'+str(output/'ZB2Admin.exe'),'/r:System.Core.dll','/r:System.Security.dll',
         '/r:System.Web.Extensions.dll','/r:System.Windows.Forms.dll','/r:System.Drawing.dll',
         '/resource:'+str(ROOT/'packaging/loader-public-key.json')+',Admin.Public',
         '/resource:'+str(assets/'Lexend-Regular.ttf')+',Admin.Font',
         '/resource:'+str(assets/'Lexend-OFL.txt')+',Admin.OFL',
         str(ROOT/'apps/admin/Core.cs'),str(ROOT/'apps/admin/Program.cs')]
result=subprocess.run(command,capture_output=True,text=True,errors='replace')
(output/'build.log').write_text(result.stdout+result.stderr,encoding='utf-8')
print(result.stdout+result.stderr)
if result.returncode:raise SystemExit(result.returncode)
distribution=ROOT/'dist/admin'
distribution.mkdir(parents=True,exist_ok=True)
shutil.copyfile(output/'ZB2Admin.exe',distribution/'ZB2Admin.exe')
print('Built:',distribution/'ZB2Admin.exe')
print('SHA256:',hashlib.sha256((distribution/'ZB2Admin.exe').read_bytes()).hexdigest())
if '--tests' in sys.argv:
    test_command=[command[0],'/target:exe','/define:ADMIN_TESTS','/main:Zb2Admin.AdminUiTests','/out:'+str(output/'admin-ui-tests.exe')]
    test_command += [arg for arg in command[1:] if not arg.startswith(('/target:','/out:'))]
    test_command += [str(ROOT/'tests/AdminUiTests.cs')]
    subprocess.run(test_command,check=True)
