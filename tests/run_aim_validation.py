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
names = ['aim_tests', 'snapshot_tests']
if os.name == 'nt': names.append('runtime_gate_tests')
for name in names:
    executable = output/(name + ('.exe' if os.name == 'nt' else ''))
    unit = source/'tests'/(name+'.cpp')
    command = ([compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', '/O2', str(unit),
                '/Fe:'+str(executable), '/Fo:'+str(output/(name+'.obj'))] if msvc else
               [compiler, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-pthread', str(unit), '-o', str(executable)])
    for invocation in (command, [str(executable)]):
        result = run(invocation, cwd=output, env=env, capture_output=True, text=True)
        text = result.stdout + result.stderr
        print(text, end='', flush=True)
        records.append({'command': invocation, 'exit_code': result.returncode, 'output': text})
        (output/'results.json').write_text(json.dumps(records, indent=2, ensure_ascii=False), encoding='utf-8')
        if result.returncode:
            raise SystemExit(result.returncode)
