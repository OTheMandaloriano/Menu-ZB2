"""Build a deterministic runtime bundle. Hashes provide integrity, not a signature."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo

ROOT = Path(__file__).resolve().parents[1]
MAX_FILE = 64 * 1024 * 1024


def file_names():
    names = json.loads((ROOT / 'packaging/runtime-files.json').read_text())['files']
    if len(names) != len(set(names)) or any(Path(n).name != n or '/' in n or '\\' in n for n in names):
        raise ValueError('Invalid packaging allowlist')
    return names


def build(source, output, version, commit):
    if not re.fullmatch(r'[0-9A-Za-z][0-9A-Za-z._-]{0,63}', version):
        raise ValueError('Invalid version')
    if not re.fullmatch(r'[a-fA-F0-9]{40}', commit):
        raise ValueError('Expected full source commit')
    source, output = Path(source).resolve(), Path(output).resolve()
    if output.is_relative_to(source):
        raise ValueError('Package output must be outside the build input')
    files = {}
    for name in file_names():
        path = source / name
        if path.is_symlink() or path.resolve().parent != source or not path.is_file():
            raise ValueError('Missing or redirected runtime file: ' + name)
        if path.stat().st_size > MAX_FILE:
            raise ValueError('Oversized runtime file: ' + name)
        files[name] = path.read_bytes()
    manifest = {
        'schema': 1, 'product': 'Menu-ZB2', 'version': version,
        'source_commit': commit.lower(), 'signature_status': 'unsigned-development',
        'files': [{'name': n, 'size': len(b), 'sha256': hashlib.sha256(b).hexdigest()}
                  for n, b in sorted(files.items())]
    }
    files['manifest.json'] = json.dumps(manifest, ensure_ascii=False, sort_keys=True, indent=2).encode('utf-8')
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix(output.suffix + '.partial')
    try:
        with ZipFile(temporary, 'w') as archive:
            for name, data in sorted(files.items()):
                info = ZipInfo(name, (2020, 1, 1, 0, 0, 0))
                info.compress_type = ZIP_DEFLATED
                info.external_attr = 0o100644 << 16
                archive.writestr(info, data)
        verify(temporary)
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)
    return manifest


def verify(path):
    with ZipFile(path) as archive:
        names = archive.namelist()
        if len(names) != len(set(names)) or set(names) != set(file_names()) | {'manifest.json'}:
            raise ValueError('Bundle entries differ from allowlist')
        if any(i.file_size > MAX_FILE for i in archive.infolist()):
            raise ValueError('Oversized entry')
        manifest = json.loads(archive.read('manifest.json'))
        records = manifest.get('files', [])
        if len(records) != len(file_names()) or {r['name'] for r in records} != set(file_names()):
            raise ValueError('Invalid manifest entries')
        if manifest.get('schema') != 1 or manifest.get('product') != 'Menu-ZB2':
            raise ValueError('Unsupported manifest')
        for record in records:
            data = archive.read(record['name'])
            if len(data) != record['size'] or hashlib.sha256(data).hexdigest() != record['sha256']:
                raise ValueError('Integrity check failed: ' + record['name'])
        return manifest


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--commit', required=True)
    args = parser.parse_args()
    build(args.source, args.output, args.version, args.commit)
    print('Development bundle verified:', args.output)
