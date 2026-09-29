from pathlib import Path
import os,subprocess,shutil
import ctypes
ctypes.windll.kernel32.SetErrorMode(3)
r=Path(__file__).resolve().parents[1];out=r/'build/player-features-tests';out.mkdir(parents=True,exist_ok=True)
shutil.copy2(r/'build/Release_x64/0Harmony.dll',out/'0Harmony.dll')
csc=Path(os.environ['WINDIR'])/'Microsoft.NET/Framework64/v4.0.30319/csc.exe';exe=out/'features.exe'
commands=[[str(csc),'/nologo','/warnaserror+','/optimize-','/out:'+str(exe),'/reference:'+str(out/'0Harmony.dll'),str(r/'managed/SlotsBridge.cs'),str(r/'managed/NoClipBridge.cs'),str(r/'tests/player_features_tests.cs')],[str(exe)],
 [str(csc),'/nologo','/warnaserror+','/optimize-','/main:MagnetTests','/out:'+str(out/'magnet.exe'),'/reference:'+str(out/'0Harmony.dll'),str(r/'managed/SlotsBridge.cs'),str(r/'managed/NoClipBridge.cs'),str(r/'managed/MagnetBridge.cs'),str(r/'managed/MagnetLoadLease.cs'),str(r/'managed/MagnetFreeze.cs'),str(r/'tests/player_features_tests.cs'),str(r/'tests/magnet_tests.cs')],[str(out/'magnet.exe')]]
log=[]
for cmd in commands:
 p=subprocess.run(cmd,cwd=out,capture_output=True,text=True);log.append(p.stdout+p.stderr);print(log[-1],flush=True)
 (out/'results.log').write_text('\n'.join(log),encoding='utf-8')
 if p.returncode:raise SystemExit(p.returncode)
