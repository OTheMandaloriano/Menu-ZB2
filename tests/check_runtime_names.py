"""Check the native/managed/loader naming contract before building a release."""
from pathlib import Path
import json,xml.etree.ElementTree as ET
r=Path(__file__).resolve().parents[1];dll='Deadblock.Menu.dll'
assert (r/'Deadblock.Menu.vcxproj').is_file()
assert not (r/'kiero-dx11-base.vcxproj').exists()
project=ET.parse(r/'Deadblock.Menu.vcxproj');ns={'m':'http://schemas.microsoft.com/developer/msbuild/2003'}
assert project.find('.//m:TargetName',ns).text=='Deadblock.Menu'
assert dll in json.loads((r/'packaging/runtime-files.json').read_text())['files']
for name in ['managed/SilhouetteBridge.cs','injector/injector.cpp','injector/config.ini','apps/loader/services.cpp','apps/loader/readiness_probe.cpp']:
    assert dll in (r/name).read_text(encoding='utf-8-sig'),name
for name in ['apps/loader/services.cpp','apps/loader/readiness_probe.cpp']:
    assert 'kiero-dx11-base.dll' in (r/name).read_text(encoding='utf-8-sig'),'Legacy loaded module must remain recognized'
for name in ['src/menu/mono.cpp','src/menu/aim_runtime.inl']:
    text=(r/name).read_text(encoding='utf-8-sig');assert 'DeadblockAim' in text and 'WohaxAim' not in text
assert 'wohax' not in (r/'src/menu/gui.cpp').read_text(encoding='utf-8-sig').lower()
print('PASS: project, native target, managed import, helper, package and loaded-module compatibility agree.')
