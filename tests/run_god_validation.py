from pathlib import Path
import os,subprocess,shutil
r=Path(__file__).resolve().parents[1];out=r/'build/god-tests';out.mkdir(parents=True,exist_ok=True)
shutil.copy2(r/'build/Release_x64/0Harmony.dll',out/'0Harmony.dll')
csc=Path(os.environ['WINDIR'])/'Microsoft.NET/Framework64/v4.0.30319/csc.exe';exe=out/'god.exe'
log=[]
for command in ([str(csc),'/nologo','/warnaserror+','/optimize-','/reference:'+str(out/'0Harmony.dll'),'/out:'+str(exe),str(r/'managed/GodModeBridge.cs'),str(r/'tests/god_mode_tests.cs')],[str(exe)]):
 p=subprocess.run(command,cwd=out,capture_output=True,text=True);log.append(p.stdout+p.stderr);print(log[-1]);(out/'results.log').write_text('\n'.join(log),encoding='utf-8')
 if p.returncode:raise SystemExit(p.returncode)
