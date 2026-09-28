"""Enforce the cross-thread ownership boundary that prevents skipped overlay frames."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
files=['mono.cpp','aim_runtime.inl','aim_bridge.inl','world_esp.inl','distance_runtime.inl']
used=set()
for name in files:
    text=(root/name).read_text(encoding='utf-8-sig')
    assert 'Config::' not in text, name+': game code must use a captured config'
    assert 'RuntimeGate::' not in text, name+': game code must never lock the UI'
    used.update(re.findall(r's_options\.(\w+)',text))
header=(root/'runtime_settings.h').read_text()
derived={'iPoiFilter':'-1','bLimitFov':'!Config::b360Mode'}
for name in used-{'viewportWidth','viewportHeight','chamsVisible','chamsHidden'}:
    assert 'values.'+name+'='+derived.get(name,'Config::'+name)+';' in header, 'Unpublished setting: '+name
main=(root/'main.cpp').read_text(encoding='utf-8-sig')
assert main.index('GUI::Render();')<main.index('Mono::SetViewport(')<main.index('Mono::Tick();')
print('PASS: every game setting is captured; game never holds UI gate; publication precedes bootstrap')
