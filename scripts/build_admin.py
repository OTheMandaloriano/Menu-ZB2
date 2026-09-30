"""Build the Dear ImGui Admin with its headless CNG service embedded."""
from pathlib import Path
import subprocess
import hashlib
import shutil
import sys
from build_loader import environment
ROOT=Path(__file__).resolve().parents[1]
output=ROOT/'build/admin-native';output.mkdir(parents=True,exist_ok=True)
assets=ROOT/'apps/loader/assets'
logs=[]
def run(command,env=None):
    result=subprocess.run(list(map(str,command)),cwd=output,env=env,capture_output=True,text=True,errors='replace')
    logs.append(result.stdout+result.stderr);(output/'build.log').write_text('\n'.join(logs),encoding='utf-8');print(logs[-1],flush=True)
    if result.returncode:raise SystemExit(result.returncode)
csc=Path('C:/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe')
run([csc,'/nologo','/target:exe','/platform:x64','/optimize+','/warnaserror+',
     '/out:'+str(output/'ZB2AdminBackend.exe'),'/r:System.Core.dll','/r:System.Security.dll','/r:System.Web.Extensions.dll',
     '/resource:'+str(ROOT/'packaging/loader-public-key.json')+',Admin.Public',ROOT/'apps/admin/Core.cs',ROOT/'apps/admin/Backend.cs'])
(output/'backend.sha256').write_text(hashlib.sha256((output/'ZB2AdminBackend.exe').read_bytes()).hexdigest(),encoding='ascii')
resources=['501 RCDATA "ZB2AdminBackend.exe"','502 RCDATA "backend.sha256"']
for resource,name in [(201,'Lexend-SemiBold.ttf'),(202,'Lexend-Bold.ttf'),(203,'Lexend-Black.ttf'),(204,'fa-solid-900.ttf'),(206,'Lexend-Regular.ttf')]:
    shutil.copyfile(assets/name,output/name);resources.append(f'{resource} RCDATA "{name}"')
(output/'admin.rc').write_text('\n'.join(resources)+'\n',encoding='ascii')
env=environment(output)
run([shutil.which('rc.exe',path=env['PATH']),'/nologo','/fo','admin.res','admin.rc'],env)
cl=shutil.which('cl.exe',path=env['PATH']);flags=['/nologo','/std:c++17','/EHsc','/O2','/MT','/MP4','/W4','/WX','/utf-8','/DUNICODE','/D_UNICODE','/DNOMINMAX']
core=[ROOT/'apps/admin/native'/f'{name}.cpp' for name in ['model','backend','controller','ui']]
shared=[ROOT/'apps/shared'/f'{name}.cpp' for name in ['theme','widgets','resources']]
imgui=[ROOT/'imgui'/f'{name}.cpp' for name in ['imgui','imgui_draw','imgui_tables','imgui_widgets']]
libraries=['user32.lib','gdi32.lib','imm32.lib','bcrypt.lib','advapi32.lib','shell32.lib','ole32.lib','comdlg32.lib']
run([cl,*flags,*core,*shared,*imgui,ROOT/'apps/loader/license.cpp',ROOT/'apps/shared/graphics.cpp',ROOT/'apps/admin/native/main.cpp',
     ROOT/'imgui/imgui_impl_win32.cpp',ROOT/'imgui/imgui_impl_dx11.cpp','admin.res','/Fe:ZB2Admin.exe','/link','/SUBSYSTEM:WINDOWS',*libraries,'dwmapi.lib','d3d11.lib','d3dcompiler.lib'],env)
if '--tests' in sys.argv:
    run([cl,*flags,*core,*shared,*imgui,ROOT/'apps/loader/license.cpp',ROOT/'tests/admin_native_tests.cpp','admin.res','/Fe:admin-native-tests.exe','/link',*libraries],env)
destination=ROOT/'dist/admin';destination.mkdir(parents=True,exist_ok=True);shutil.copyfile(output/'ZB2Admin.exe',destination/'ZB2Admin.exe')
print('Built native ImGui:',destination/'ZB2Admin.exe')
