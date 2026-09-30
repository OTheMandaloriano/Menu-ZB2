"""Check repository paths after moving the native menu to src/menu."""
from pathlib import Path
import re,xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[1]
assert not [p for p in root.iterdir() if p.suffix in {'.cpp','.h','.inl'}], 'Native sources belong in src/menu'
ns={'m':'http://schemas.microsoft.com/developer/msbuild/2003'}
project=ET.parse(root/'kiero-dx11-base.vcxproj')
references=[]
for tag in ('ClCompile','ClInclude'):
    for item in project.findall('.//m:'+tag,ns):
        name=item.get('Include')
        if name:
            path=root/name.replace('\\','/');assert path.is_file(),'Missing project source: '+name;references.append(path.resolve())
for source in (root/'src/menu').glob('*'):
    if source.suffix in {'.cpp','.h','.inl'}:assert source.resolve() in references,'Unlisted source: '+source.name
for folder in ['tests','src/menu']:
    for source in (root/folder).glob('*'):
        if source.suffix not in {'.cpp','.h','.inl'}:continue
        for name in re.findall(r'^\s*#include\s+"([^"]+)"',source.read_text(encoding='utf-8-sig'),re.M):
            assert (source.parent/name).exists() or (root/name).exists(),'Missing include: '+str(source.relative_to(root))+' -> '+name
print(f'PASS: {len(references)} project references and local includes resolve; root contains no native source files.')
