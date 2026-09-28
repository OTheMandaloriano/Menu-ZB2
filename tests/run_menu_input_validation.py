from pathlib import Path
import subprocess,os,shutil
r=Path(__file__).resolve().parents[1];out=r/'build/menu-input-tests';out.mkdir(parents=True,exist_ok=True)
shutil.copy2(r/'build/Release_x64/0Harmony.dll',out/'0Harmony.dll')
csc=Path(os.environ['WINDIR'])/'Microsoft.NET/Framework64/v4.0.30319/csc.exe';exe=out/'menu-input.exe'
commands=[[str(csc),'/nologo','/warnaserror+','/optimize-','/out:'+str(exe),'/reference:'+str(out/'0Harmony.dll'),str(r/'managed/MenuInputBridge.cs'),str(r/'tests/menu_input_tests.cs')],[str(exe)]]
log=[]
for cmd in commands:
 p=subprocess.run(cmd,capture_output=True,text=True);log.append(p.stdout+p.stderr);print(log[-1],flush=True)
 (out/'results.log').write_text('\n'.join(log),encoding='utf-8')
 if p.returncode:raise SystemExit(p.returncode)
