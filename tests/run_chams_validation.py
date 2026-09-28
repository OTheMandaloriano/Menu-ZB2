from pathlib import Path
import os,subprocess
r=Path(__file__).resolve().parents[1];out=r/'build/chams-tests';out.mkdir(parents=True,exist_ok=True)
csc=Path(os.environ['WINDIR'])/'Microsoft.NET/Framework64/v4.0.30319/csc.exe';exe=out/'chams.exe'
commands=[[str(csc),'/nologo','/warnaserror+','/out:'+str(exe),str(r/'managed/ChamsBridge.cs'),str(r/'managed/EffectRange.cs'),str(r/'managed/SilhouetteBridge.cs'),str(r/'tests/chams_tests.cs')],[str(exe)]]
log=[]
for cmd in commands:
 p=subprocess.run(cmd,capture_output=True,text=True);log.append(p.stdout+p.stderr);print(log[-1],flush=True)
 (out/'results.log').write_text('\n'.join(log),encoding='utf-8')
 if p.returncode:raise SystemExit(p.returncode)
