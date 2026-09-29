from pathlib import Path
import os,subprocess,shutil,ctypes
ctypes.windll.kernel32.SetErrorMode(3)
r=Path(__file__).resolve().parents[1];out=r/'build/utility-tests';out.mkdir(parents=True,exist_ok=True)
shutil.copy2(r/'build/Release_x64/0Harmony.dll',out/'0Harmony.dll')
csc=Path(os.environ['WINDIR'])/'Microsoft.NET/Framework64/v4.0.30319/csc.exe';exe=out/'utilities.exe'
sources=['managed/OriginalValues.cs','managed/UtilityBridge.cs','managed/UtilityDestinations.cs','managed/UtilitySpawnQueue.cs','tests/utility_tests.cs']
log=[]
for cmd in ([str(csc),'/nologo','/warnaserror+','/optimize-','/reference:'+str(out/'0Harmony.dll'),'/out:'+str(exe)]+[str(r/p) for p in sources],[str(exe)]):
 p=subprocess.run(cmd,cwd=out,capture_output=True,text=True,timeout=45);log.append(p.stdout+p.stderr);print(log[-1]);(out/'results.log').write_text('\n'.join(log),encoding='utf-8')
 if p.returncode:raise SystemExit(p.returncode)
