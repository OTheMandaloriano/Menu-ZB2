"""Build a native UI preview with an embedded, non-executed development bundle."""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
from package_runtime import verify

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--bundle', required=True, type=Path)
args = parser.parse_args()
verify(args.bundle)
out = root/'build/loader-preview'
out.mkdir(parents=True, exist_ok=True)
shutil.copyfile(args.bundle, out/'runtime.zip')
(out/'bundle.rc').write_text('101 RCDATA "runtime.zip"\n', encoding='ascii')
installer = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)'))/'Microsoft Visual Studio/Installer/vswhere.exe'
vs = subprocess.run([str(installer), '-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'],capture_output=True,text=True,check=True).stdout.strip()
env = {k.upper(): v for k,v in os.environ.items()}
setup = out/'environment.cmd'
setup.write_text('@echo off\ncall "'+str(Path(vs)/'VC/Auxiliary/Build/vcvars64.bat')+'" >nul\nset\n')
result = subprocess.run(['cmd.exe','/d','/c',str(setup)],capture_output=True,text=True,check=True,env=env)
for line in result.stdout.splitlines():
    if '=' in line and not line.startswith('='):
        key,value=line.split('=',1);env[key.upper()]=value
sources = ['apps/loader/main.cpp','apps/loader/window.cpp','apps/loader/graphics.cpp','apps/loader/preview_ui.cpp','apps/loader/preview_theme.cpp'] + ['imgui/'+n+'.cpp' for n in ['imgui','imgui_draw','imgui_widgets','imgui_tables','imgui_impl_win32','imgui_impl_dx11']]
commands = [[shutil.which('rc.exe',path=env['PATH']),'/nologo','/fo','bundle.res','bundle.rc'],
 [shutil.which('cl.exe',path=env['PATH']),'/nologo','/std:c++17','/EHsc','/O2','/MT','/W3','/utf-8','/DUNICODE','/D_UNICODE','/DNOMINMAX',*[str(root/p) for p in sources],'bundle.res','/Fe:ZB2Menu-Preview.exe','/link','/SUBSYSTEM:WINDOWS','user32.lib','gdi32.lib','dwmapi.lib','imm32.lib','d3d11.lib','d3dcompiler.lib']]
logs=[]
for command in commands:
    result=subprocess.run(command,cwd=out,env=env,capture_output=True,text=True)
    logs.append(result.stdout+result.stderr);print(logs[-1],flush=True)
    (out/'build.log').write_text('\n'.join(logs),encoding='utf-8')
    if result.returncode:raise SystemExit(result.returncode)
print(out/'ZB2Menu-Preview.exe')
