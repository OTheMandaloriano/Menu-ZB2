import os
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
out = root/'build/managed-tests'
out.mkdir(parents=True, exist_ok=True)
dependency = root/'build/Release_x64/0Harmony.dll'
if not dependency.exists():
    raise SystemExit('Run scripts/build_managed_aim.py first.')
shutil.copy2(dependency,out/'0Harmony.dll')
csc = Path(os.environ['WINDIR'])/'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
executable = out/'managed_aim_tests.exe'
compile = [str(csc), '/nologo','/target:exe','/optimize-','/warnaserror+', '/define:AIM_TESTS',
           '/reference:'+str(out/'0Harmony.dll'), '/out:'+str(executable),
           str(root/'managed/AimBridge.cs'),str(root/'tests/managed_aim_tests.cs')]
records = []
for command in [compile,[str(executable)]]:
    result = subprocess.run(command,cwd=out,capture_output=True,text=True)
    text = result.stdout+result.stderr
    records.append(text)
    print(text,flush=True)
    (out/'results.log').write_text('\n'.join(records),encoding='utf-8')
    if result.returncode: raise SystemExit(result.returncode)
