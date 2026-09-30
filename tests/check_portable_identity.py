"""Reject personal build-time paths in product code, fixtures and current guides."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
directories=['apps','src/menu','managed','injector','packaging','scripts','tests']
suffixes={'.cpp','.h','.inl','.cs','.py','.ini','.json'}
bad=[];count=0
pattern=re.compile(r'(?i)\b[A-Z]:[/\\]Users[/\\](?!Public\b|Default\b)[^\s"\'<>]+')
personal='we'+'fagundes'
for directory in directories:
    for path in (root/directory).rglob('*'):
        if not path.is_file() or path.suffix not in suffixes:continue
        count+=1;data=path.read_text(encoding='utf-8-sig')
        if pattern.search(data) or personal in data.lower():bad.append(str(path.relative_to(root)))
assert not bad,'Personal identity or fixed profile path: '+', '.join(bad)
for name in ['apps/admin/native/backend.cpp','apps/loader/services.cpp','src/menu/log.cpp']:
    text=(root/name).read_text(encoding='utf-8-sig')
    assert 'SHGetKnownFolderPath' in text and 'FOLDERID_Documents' in text,name+': preserve Windows Documents resolution'
print(f'PASS: {count} product/fixture files checked; user profile paths are resolved by Windows.')
