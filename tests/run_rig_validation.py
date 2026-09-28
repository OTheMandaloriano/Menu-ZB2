from pathlib import Path
import os,subprocess
r=Path(__file__).resolve().parents[1];out=r/'build/rig-tests';out.mkdir(parents=True,exist_ok=True)
csc=Path(os.environ['WINDIR'])/'Microsoft.NET/Framework64/v4.0.30319/csc.exe';exe=out/'rig.exe'
for cmd in ([str(csc),'/nologo','/warnaserror+','/out:'+str(exe),str(r/'managed/ZombieRigBridge.cs'),str(r/'tests/rig_tests.cs')],[str(exe)]):
 p=subprocess.run(cmd,capture_output=True,text=True);print(p.stdout+p.stderr)
 if p.returncode:raise SystemExit(p.returncode)
