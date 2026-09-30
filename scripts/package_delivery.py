"""Create the two initial ZIPs directly; never include an operator profile."""
from pathlib import Path
import hashlib,json,zipfile
ROOT=Path(__file__).resolve().parents[1]
out=ROOT/'dist/entrega';out.mkdir(parents=True,exist_ok=True)
bundles={'01-CLIENTE-INICIAL.zip':('loader/ZB2Menu.exe','ZB2Menu.exe'),
         '01-EQUIPE-INICIAL.zip':('admin/ZB2Admin.exe','ZB2Admin.exe')}
records={}
for name,(source,entry) in bundles.items():
    executable=ROOT/'dist'/source;archive=out/name
    temporary=archive.with_suffix('.zip.tmp')
    try:
        with zipfile.ZipFile(temporary,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:z.write(executable,entry)
        with zipfile.ZipFile(temporary) as z:
            assert z.namelist()==[entry] and z.testzip() is None
            assert hashlib.sha256(z.read(entry)).digest()==hashlib.sha256(executable.read_bytes()).digest()
        temporary.replace(archive)
    finally:temporary.unlink(missing_ok=True)
    records[name]={'files':[entry],'sha256':hashlib.sha256(archive.read_bytes()).hexdigest()}
    print(name+': enviar primeiro; não contém licença, autorização nem dados pessoais.')
(ROOT/'build/delivery-manifest.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
