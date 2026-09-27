from pathlib import Path
import subprocess,os
r=Path(__file__).resolve().parents[1];out=r/'build/loot-tests';out.mkdir(parents=True,exist_ok=True)
csc=Path(os.environ['WINDIR'])/'Microsoft.NET/Framework64/v4.0.30319/csc.exe';exe=out/'loot.exe'
commands=[[str(csc),'/nologo','/warnaserror+','/out:'+str(exe),str(r/'managed/ItemEligibility.cs'),str(r/'managed/LootMagnetBridge.cs'),str(r/'tests/loot_magnet_tests.cs')],[str(exe)]]
log=[]
for cmd in commands:
 p=subprocess.run(cmd,capture_output=True,text=True);log.append(p.stdout+p.stderr);print(log[-1],flush=True)
 (out/'results.log').write_text('\n'.join(log),encoding='utf-8')
 if p.returncode:raise SystemExit(p.returncode)
