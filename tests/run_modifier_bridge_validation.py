from pathlib import Path
import os, subprocess, shutil
root=Path(__file__).resolve().parents[1]
out=root/'build/modifier-tests'
out.mkdir(parents=True,exist_ok=True)
shutil.copy2(root/'build/Release_x64/0Harmony.dll',out/'0Harmony.dll')
csc=Path(os.environ['WINDIR'])/'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
exe=out/'modifier_tests.exe'
commands=[[str(csc),'/nologo','/warnaserror+','/out:'+str(exe),'/reference:'+str(out/'0Harmony.dll'),
    str(root/'managed/OriginalValues.cs'),str(root/'managed/ModifierBridge.cs'),str(root/'tests/modifier_bridge_tests.cs')],[str(exe)],
    [str(csc),'/nologo','/warnaserror+','/out:'+str(out/'world_tests.exe'),str(root/'managed/WorldEspBridge.cs'),str(root/'managed/ItemEligibility.cs'),str(root/'tests/world_esp_tests.cs'),str(root/'tests/visual_model_stubs.cs')],
    [str(out/'world_tests.exe')]]
log=[]
for command in commands:
    r=subprocess.run(command,cwd=out,capture_output=True,text=True)
    log.append(r.stdout+r.stderr)
    (out/'results.log').write_text('\n'.join(log),encoding='utf-8')
    print(log[-1],flush=True)
    if r.returncode: raise SystemExit(r.returncode)
