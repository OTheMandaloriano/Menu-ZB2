"""Build the signed offline loader; private issuer material never enters output.

Requires MSVC x64, Windows SDK and Python cryptography. Runtime DLLs remain
unchanged; the helper is rebuilt to support a single, explicitly selected PID.
"""
import argparse
import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
from license_admin import load_key, public_hex, sign

ROOT = Path(__file__).resolve().parents[1]

def environment(output):
    installer = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)'))/'Microsoft Visual Studio/Installer/vswhere.exe'
    vs = subprocess.run([str(installer), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'],
        capture_output=True, text=True, check=True).stdout.strip()
    env = {key.upper(): value for key, value in os.environ.items()}
    setup = output/'environment.cmd'
    setup.write_text('@echo off\ncall "'+str(Path(vs)/'VC/Auxiliary/Build/vcvars64.bat')+'" >nul\nset\n')
    result = subprocess.run(['cmd.exe', '/d', '/c', str(setup)], capture_output=True, text=True, check=True, env=env)
    for line in result.stdout.splitlines():
        if '=' in line and not line.startswith('='):
            key, value = line.split('=', 1)
            env[key.upper()] = value
    return env

def compress(data):
    api = ctypes.WinDLL('cabinet', use_last_error=True)
    api.CreateCompressor.argtypes = [wintypes.DWORD, ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)]
    api.Compress.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t,
                            ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
    api.CloseCompressor.argtypes = [ctypes.c_void_p]
    handle = ctypes.c_void_p()
    if not api.CreateCompressor(2, None, ctypes.byref(handle)):
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        source = ctypes.create_string_buffer(data)
        size = ctypes.c_size_t()
        api.Compress(handle, source, len(data), None, 0, ctypes.byref(size))
        if not size.value or size.value > 128*1024*1024:
            raise RuntimeError('Invalid compression size')
        output = ctypes.create_string_buffer(size.value)
        if not api.Compress(handle, source, len(data), output, len(output), ctypes.byref(size)):
            raise ctypes.WinError(ctypes.get_last_error())
        return output.raw[:size.value]
    finally:
        api.CloseCompressor(handle)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', required=True, type=Path)
    parser.add_argument('--runtime-commit', required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--key', required=True, type=Path)
    parser.add_argument('--public', required=True, type=Path)
    parser.add_argument('--tests', action='store_true')
    args = parser.parse_args()
    if not re.fullmatch('[A-Za-z0-9][A-Za-z0-9.-]{0,63}', args.version) or not re.fullmatch('[0-9a-f]{40}', args.runtime_commit):
        parser.error('Invalid version or runtime commit')
    key = load_key(args.key)
    public = json.loads(args.public.read_text(encoding='utf-8'))
    if public != {'algorithm':'ECDSA-P256-SHA256','public_xy':public_hex(key)}:
        parser.error('Private key does not match the selected public key')
    out = ROOT/'build/loader'
    out.mkdir(parents=True, exist_ok=True)
    env = environment(out)
    cl = shutil.which('cl.exe', path=env['PATH'])
    logs = []
    def run(command):
        result = subprocess.run(command, cwd=out, env=env, capture_output=True, text=True, errors='replace')
        logs.append(result.stdout+result.stderr)
        (out/'build.log').write_text('\n'.join(logs), encoding='utf-8')
        print(logs[-1], end='', flush=True)
        if result.returncode:
            raise SystemExit(result.returncode)
    flags = ['/nologo','/std:c++17','/EHsc','/O2','/MT','/MP4','/W4','/WX','/utf-8','/DUNICODE','/D_UNICODE','/DNOMINMAX']
    run([cl, *flags, str(ROOT/'injector/injector.cpp'), '/Fe:injector.exe', '/link', 'advapi32.lib','shell32.lib','ole32.lib'])
    run([cl,*flags,'/LD',str(ROOT/'apps/loader/readiness_probe.cpp'),'/Fe:ZB2.Readiness.dll'])
    names = json.loads((ROOT/'packaging/runtime-files.json').read_text())['files']
    records = []
    resources = []
    for index, name in enumerate(names):
        path = out/name if name in ('injector.exe','ZB2.Readiness.dll') else args.runtime/name
        if path.is_symlink() or not path.is_file() or path.stat().st_size > 64*1024*1024:
            parser.error('Invalid runtime file: '+name)
        data = path.read_bytes()
        records.append({'name':name, 'size':len(data), 'sha256':hashlib.sha256(data).hexdigest()})
        packed = f'runtime-{index}.mszip'
        (out/packed).write_bytes(compress(data))
        resources.append(f'{301+index} RCDATA "{packed}"')
    manifest = ('ZB2-BUNDLE-1\n'+args.version+'\n'+args.runtime_commit+'\n'+''.join(
        f'{row["name"]}:{row["size"]}:{row["sha256"]}\n' for row in records)).encode('ascii')
    (out/'public.bin').write_bytes(bytes.fromhex(public_hex(key)))
    (out/'manifest.txt').write_bytes(manifest)
    (out/'manifest.sig').write_bytes(sign(key, manifest))
    resources += ['102 RCDATA "public.bin"','103 RCDATA "manifest.txt"','104 RCDATA "manifest.sig"']
    for name in ['client','menu']:
        shutil.copyfile(ROOT/'apps/shared/brand'/(name+'.ico'),out/(name+'.ico'))
    resources += ['1 ICON "client.ico"','2 ICON "menu.ico"']
    assets = ROOT/'apps/loader/assets'
    provenance = json.loads((assets/'provenance.json').read_text())
    for index, name in enumerate(['Lexend-SemiBold.ttf','Lexend-Bold.ttf','Lexend-Black.ttf','fa-solid-900.ttf']):
        if hashlib.sha256((assets/name).read_bytes()).hexdigest() != provenance[name]['sha256']:
            parser.error('Font asset hash mismatch: '+name)
        shutil.copyfile(assets/name, out/name)
        resources.append(f'{201+index} RCDATA "{name}"')
    notices = '\n\n'.join((assets/name).read_text(encoding='utf-8') for name in
        ['Lexend-OFL.txt','FontAwesome-LICENSE.txt','IconFontCppHeaders-LICENSE.txt'])
    notices += '\n\nDear ImGui\n'+(assets/'DearImGui-LICENSE.txt').read_text(encoding='utf-8')
    (out/'attributions.bin').write_text(notices, encoding='utf-8')
    resources.append('205 RCDATA "attributions.bin"')
    regular=assets/'Lexend-Regular.ttf'
    if hashlib.sha256(regular.read_bytes()).hexdigest()!=provenance[regular.name]['sha256']:
        parser.error('Regular font hash mismatch')
    shutil.copyfile(regular,out/regular.name)
    resources.append('206 RCDATA "Lexend-Regular.ttf"')
    (out/'loader.rc').write_text('\n'.join(resources)+'\n', encoding='ascii')
    run([shutil.which('rc.exe',path=env['PATH']),'/nologo','/fo','loader.res','loader.rc'])
    core = ['license','services','controller','ui','readiness']
    sources = [str(ROOT/'apps/loader'/f'{name}.cpp') for name in core]
    sources += [str(ROOT/'apps/shared'/f'{name}.cpp') for name in ['theme','widgets','resources']]
    imgui = [str(ROOT/'imgui'/f'{name}.cpp') for name in ['imgui','imgui_draw','imgui_tables','imgui_widgets']]
    libraries = ['user32.lib','gdi32.lib','imm32.lib','bcrypt.lib','crypt32.lib','advapi32.lib','shell32.lib','ole32.lib','cabinet.lib','comdlg32.lib']
    run([cl,*flags,*sources,*imgui,*[str(ROOT/'apps/loader'/f'{name}.cpp') for name in ['main','window']],str(ROOT/'apps/shared/graphics.cpp'),
         str(ROOT/'imgui/imgui_impl_win32.cpp'),str(ROOT/'imgui/imgui_impl_dx11.cpp'),'loader.res','/Fe:ZB2Menu.exe',
         '/link','/SUBSYSTEM:WINDOWS',*libraries,'dwmapi.lib','d3d11.lib','d3dcompiler.lib'])
    if args.tests:
        run([cl,*flags,str(ROOT/'tests/auto_inject_tests.cpp'),str(ROOT/'apps/loader/readiness.cpp'),'/Fe:auto-inject-tests.exe','/link','shell32.lib','ole32.lib'])
        run([cl,*flags,*sources,*imgui,str(ROOT/'tests/loader_functional_tests.cpp'),'loader.res',
             '/Fe:loader-tests.exe','/link',*libraries])
    dist = ROOT/'dist/loader'
    dist.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(out/'ZB2Menu.exe',dist/'ZB2Menu.exe')
    metadata = {'version':args.version,'runtime_commit':args.runtime_commit,'files':records,
        'public_key_sha256':hashlib.sha256(bytes.fromhex(public_hex(key))).hexdigest(),
        'exe_sha256':hashlib.sha256((dist/'ZB2Menu.exe').read_bytes()).hexdigest(),
        'authenticode_signed':False}
    (out/'build-metadata.json').write_text(json.dumps(metadata, indent=2)+'\n',encoding='utf-8')
    print('Built:',dist/'ZB2Menu.exe')

if __name__ == '__main__':
    main()
