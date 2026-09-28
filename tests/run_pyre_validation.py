from pathlib import Path
import os, subprocess
root=Path(__file__).resolve().parents[1]
out=root/'build/pyre-tests';out.mkdir(parents=True,exist_ok=True)
csc=Path(os.environ['WINDIR'])/'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
exe=out/'pyres.exe'
log=[]
for command in ([str(csc),'/nologo','/warnaserror+','/out:'+str(exe),str(root/'managed/PyreNavigation.cs'),str(root/'tests/pyre_navigation_tests.cs')],[str(exe)]):
    result=subprocess.run(command,capture_output=True,text=True)
    log.append(result.stdout+result.stderr);print(log[-1])
    (out/'results.log').write_text('\n'.join(log),encoding='utf-8')
    if result.returncode:raise SystemExit(result.returncode)
