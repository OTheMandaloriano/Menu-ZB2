"""Guard current source and documentation against developer-specific profile paths."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
directories=['apps','src/menu','managed','injector','packaging','scripts','tests','docs','.github','memory']
suffixes={'.cpp','.h','.inl','.cs','.py','.ini','.json','.md','.yml','.yaml','.bat','.ps1','.txt'}
paths={root/n for n in ['README.md','AGENTS.md','CHANGELOG.md','NOTICE.md']}
for directory in directories:
    paths.update(p for p in (root/directory).rglob('*') if p.is_file() and p.suffix in suffixes and '__pycache__' not in p.parts)
profile=re.compile(r'(?i)\b[A-Z]:[/\\]Users[/\\](?!Public\b|Default\b)[^\s"\'<>]+')
personal=re.compile('we'+'fagundes|we'+'fagundos',re.I)
def issues(text):return bool(profile.search(text) or personal.search(text))
# Check the checker without placing a real personal path in this file.
assert issues('C:'+'/'+'Users'+'/'+'ExampleOperator'+'/Documents')
assert issues('C:'+chr(92)+'Users'+chr(92)+'ExampleOperator')
assert not issues('<Documentos do usuário>/ZB2Menu')
bad=[str(p.relative_to(root)) for p in sorted(paths) if issues(p.read_text(encoding='utf-8-sig'))]
assert not bad,'Personal identity or fixed profile path: '+', '.join(bad)
for name in ['apps/admin/native/backend.cpp','apps/loader/services.cpp','src/menu/log.cpp']:
    text=(root/name).read_text(encoding='utf-8-sig')
    assert 'SHGetKnownFolderPath' in text and 'FOLDERID_Documents' in text,name+': preserve Windows Documents resolution'
print(f'PASS: {len(paths)} source, fixture and documentation files; Windows resolves user data folders.')
