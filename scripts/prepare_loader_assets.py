"""Fetch licensed upstream font assets; retain provenance and exact hashes."""
from pathlib import Path
import hashlib
import json
import urllib.request
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont

ROOT = Path(__file__).resolve().parents[1] / 'apps/loader/assets'
SOURCES = {
    'DearImGui-LICENSE.txt': 'https://raw.githubusercontent.com/ocornut/imgui/v1.89.9/LICENSE.txt',
    'Lexend-variable.ttf': 'https://raw.githubusercontent.com/google/fonts/main/ofl/lexend/Lexend%5Bwght%5D.ttf',
    'Lexend-OFL.txt': 'https://raw.githubusercontent.com/google/fonts/main/ofl/lexend/OFL.txt',
    'fa-solid-900.ttf': 'https://raw.githubusercontent.com/FortAwesome/Font-Awesome/6.7.2/webfonts/fa-solid-900.ttf',
    'FontAwesome-LICENSE.txt': 'https://raw.githubusercontent.com/FortAwesome/Font-Awesome/6.7.2/LICENSE.txt',
    'IconsFontAwesome6.h': 'https://raw.githubusercontent.com/juliettef/IconFontCppHeaders/main/IconsFontAwesome6.h',
    'IconFontCppHeaders-LICENSE.txt': 'https://raw.githubusercontent.com/juliettef/IconFontCppHeaders/main/licence.txt',
}

def main():
    ROOT.mkdir(parents=True, exist_ok=True)
    records = {}
    for name, url in SOURCES.items():
        path = ROOT / name
        if not path.exists():
            data = urllib.request.urlopen(url, timeout=30).read()
            path.write_bytes(data)
        records[name] = {'source': url, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
    for weight, name in [(400, 'Regular'), (600, 'SemiBold'), (700, 'Bold'), (900, 'Black')]:
        path = ROOT / f'Lexend-{name}.ttf'
        if not path.exists():
            font = TTFont(ROOT / 'Lexend-variable.ttf')
            instantiateVariableFont(font, {'wght': weight}, inplace=True).save(path)
        records[path.name] = {'source': 'Lexend-variable.ttf', 'weight': weight,
                              'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
    (ROOT / 'provenance.json').write_text(json.dumps(records, indent=2)+'\n', encoding='utf-8')
    print('Font assets and provenance saved:', ROOT)

if __name__ == '__main__':
    main()
