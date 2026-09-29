"""Run deterministic aim and concurrent snapshot regressions without the game."""
from argparse import ArgumentParser
import json
import os
from pathlib import Path
from shutil import which
from subprocess import run
from tempfile import TemporaryDirectory

parser = ArgumentParser()
parser.add_argument('--output', type=Path)
parser.add_argument('--only', help='Run one named test after a focused change')
args = parser.parse_args()
source = Path(__file__).resolve().parents[1]
output = (args.output or source/'build'/'aim-validation').resolve()
output.mkdir(parents=True, exist_ok=True)
env = {key.upper(): value for key, value in os.environ.items()} if os.name == 'nt' else dict(os.environ)
compiler = which('g++')
msvc = False
if not compiler and os.name == 'nt':
    installer = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)'))/'Microsoft Visual Studio/Installer/vswhere.exe'
    found = run([str(installer), '-latest', '-products', '*', '-requires',
                 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'],
                env=env, capture_output=True, text=True, check=True).stdout.strip()
    if found:
        with TemporaryDirectory(prefix='zb2-msvc-') as temporary:
            script = Path(temporary)/'environment.cmd'
            script.write_text('@echo off\ncall "'+str(Path(found)/'VC/Auxiliary/Build/vcvars64.bat')+'" >nul\nset\n')
            initialized = run(['cmd.exe', '/d', '/c', str(script)], env=env, capture_output=True, text=True, check=True)
            for line in initialized.stdout.splitlines():
                if '=' in line and not line.startswith('='):
                    key, value = line.split('=', 1)
                    env[key.upper()] = value
        compiler = which('cl.exe', path=env['PATH'])
        msvc = True
if not compiler:
    raise SystemExit('Install g++ or the Visual Studio C++ build tools.')
records = []
names = ['aim_tests', 'snapshot_tests', 'modifier_tests', 'esp_options_tests', 'monotonic_time_tests','hotkey_toggle_tests']
if os.name == 'nt': names += ['runtime_gate_tests','settings_channel_tests','input_search_tests','esp_render_tests','silhouette_gpu_tests','loader_preview_tests']
if args.only: names = [name for name in names if name == args.only]
if not names: raise SystemExit('Unknown test name')
for name in names:
    executable = output/(name + ('.exe' if os.name == 'nt' else ''))
    unit = source/'tests'/(name+'.cpp')
    command = ([compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', '/O2', str(unit),
                '/Fe:'+str(executable), '/Fo:'+str(output/(name+'.obj'))] if msvc else
               [compiler, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-pthread', str(unit), '-o', str(executable)])
    if name == 'loader_preview_tests':
        command += ['/utf-8',str(source/'apps/loader/preview_ui.cpp'),str(source/'apps/loader/preview_theme.cpp')]
    if name in ('esp_render_tests','loader_preview_tests'):
        if name == 'esp_render_tests': command += [str(source/'esp_layout.cpp')]
        command += [str(source/'imgui'/(name+'.cpp')) for name in ['imgui','imgui_draw','imgui_tables','imgui_widgets']]
        command += ['user32.lib','/link','/IGNORE:4099']
        command = [item for item in command if not item.startswith('/Fo:')]
    for invocation in (command, [str(executable)]):
        result = run(invocation, cwd=output, env=env, capture_output=True, text=True)
        text = result.stdout + result.stderr
        print(text, end='', flush=True)
        records.append({'command': invocation, 'exit_code': result.returncode, 'output': text})
        (output/'results.json').write_text(json.dumps(records, indent=2, ensure_ascii=False), encoding='utf-8')
        if result.returncode:
            raise SystemExit(result.returncode)
