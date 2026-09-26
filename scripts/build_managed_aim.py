"""Build the Mono sidecar against the installed game's exact assemblies."""
from argparse import ArgumentParser
from hashlib import sha256
import os
from pathlib import Path
from subprocess import run
from urllib.request import urlopen
from zipfile import ZipFile

VERSION = '2.3.3'
PACKAGE_SHA256 = '87b63ddb92f04fcb89c30b7ebae473c948dd65e80569eb94aef08b163bc3bf63'
GAME_SHA256 = 'c41a298975d35f0dad0a05531bce6e0b6e274d0ddf265217d65ce3ac5cbc84e1'
root = Path(__file__).resolve().parents[1]
parser = ArgumentParser()
parser.add_argument('--managed', type=Path, required=True, help='ZumbiBlocks2_Data/Managed directory')
parser.add_argument('--output', type=Path, default=root/'build/Release_x64')
args = parser.parse_args()
game = args.managed/'Assembly-CSharp.dll'
if sha256(game.read_bytes()).hexdigest() != GAME_SHA256:
    raise SystemExit('Game assembly changed. Review the firing signatures before rebuilding.')
package = root/'build/packages'/('lib.harmony.'+VERSION+'.nupkg')
package.parent.mkdir(parents=True, exist_ok=True)
if not package.exists():
    url = 'https://api.nuget.org/v3-flatcontainer/lib.harmony/'+VERSION+'/'+package.name
    with urlopen(url, timeout=60) as response:
        package.write_bytes(response.read())
if sha256(package.read_bytes()).hexdigest() != PACKAGE_SHA256:
    raise SystemExit('Harmony package checksum mismatch')
args.output.mkdir(parents=True, exist_ok=True)
with ZipFile(package) as archive:
    (args.output/'0Harmony.dll').write_bytes(archive.read('lib/net472/0Harmony.dll'))
    (args.output/'Harmony.LICENSE').write_bytes(archive.read('LICENSE'))
csc = Path(os.environ['WINDIR'])/'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
refs = ['mscorlib.dll','System.dll','System.Core.dll','netstandard.dll',
        'Assembly-CSharp.dll','UnityEngine.CoreModule.dll','UnityEngine.PhysicsModule.dll']
command = [str(csc), '/nologo','/noconfig','/target:library','/optimize+','/warnaserror+','/nostdlib+',
           '/out:'+str(args.output/'Zb2.AimBridge.dll'),'/reference:'+str(args.output/'0Harmony.dll')]
command += ['/reference:'+str(args.managed/name) for name in refs]
command += [str(path) for path in sorted((root/'managed').glob('*.cs'))]
result = run(command, capture_output=True, text=True)
print(result.stdout+result.stderr)
(args.output/'managed-build.log').write_text(result.stdout+result.stderr, encoding='utf-8')
raise SystemExit(result.returncode)
