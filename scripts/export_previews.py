"""Generate public UI previews from actual renderers, never from personal profiles."""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def source_manifest():
    paths=[]
    for directory in ['src/menu','apps/admin','apps/loader','apps/shared','imgui']:
        paths += [p for p in (ROOT/directory).rglob('*') if p.is_file() and p.suffix in {'.cpp','.h','.inl','.cs','.ttf','.ico'}]
    paths += [ROOT/'tests'/n for n in ['menu_preview.cpp','software_renderer.h','admin_native_tests.cpp','loader_functional_tests.cpp']]
    return {p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes().replace(b'\r\n',b'\n') if p.suffix in {'.cpp','.h','.inl','.cs'} else p.read_bytes()).hexdigest() for p in sorted(set(paths))}
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--admin-test',type=Path,default=ROOT/'build/admin-native/admin-native-tests.exe')
    parser.add_argument('--loader-test',type=Path,default=ROOT/'build/loader/loader-tests.exe')
    parser.add_argument('--output',type=Path,default=ROOT/'docs/previews')
    parser.add_argument('--check',action='store_true',help='Check freshness without rendering or compiling')
    args=parser.parse_args();output=args.output.resolve();manifest=output/'manifest.json'
    if args.check:
        data=json.loads(manifest.read_text(encoding='utf-8'))
        if data['sources']!=source_manifest():raise SystemExit('Previews stale: rebuild UI test binaries and rerun export_previews.py.')
        for n,h in data['images'].items():
            if digest(output/n)!=h:raise SystemExit('Preview image changed: '+n)
        print('Preview sources and images match.');return
    from PIL import Image
    from build_loader import environment
    for executable in [args.admin_test,args.loader_test]:
        if not executable.is_file():raise SystemExit('Build --tests first: '+str(executable))
    base=ROOT/'build/previews';base.mkdir(parents=True,exist_ok=True);output.mkdir(parents=True,exist_ok=True)
    # Temporary outputs are restricted to this owned build directory and removed on exit.
    with tempfile.TemporaryDirectory(prefix='render-',dir=base) as temporary:
        work=Path(temporary);env=environment(work);compiler=shutil.which('cl.exe',path=env['PATH'])
        units=[ROOT/'tests/menu_preview.cpp',ROOT/'src/menu/esp_layout.cpp']+[ROOT/'imgui'/(n+'.cpp') for n in ['imgui','imgui_draw','imgui_tables','imgui_widgets']]
        command=[compiler,'/nologo','/std:c++17','/EHsc','/O2','/MT','/W4','/WX','/utf-8','/DNOMINMAX','/I'+str(ROOT),*map(str,units),'/Fe:menu-preview.exe','/link','user32.lib']
        result=subprocess.run(command,cwd=work,env=env,capture_output=True,text=True,errors='replace')
        (base/'build.log').write_text(result.stdout+result.stderr,encoding='utf-8')
        if result.returncode:raise SystemExit('Menu preview build failed. See build/previews/build.log.')
        for command in [[str(work/'menu-preview.exe'),str(work)],[str(args.admin_test.resolve())],[str(args.loader_test.resolve())]]:
            result=subprocess.run(command,cwd=work,capture_output=True,text=True,errors='replace')
            if result.returncode:raise SystemExit(result.stdout+result.stderr)
        images={'menu.png':'menu.ppm','admin-clientes.png':'admin-native-0-100.ppm','admin-equipe.png':'admin-native-1-100.ppm','admin-envio.png':'admin-native-3-100.ppm','admin-configuracoes.png':'admin-native-6-100.ppm','loader-ativacao.png':'loader-1-100.ppm','loader-carregado.png':'loader-5-100.ppm'}
        for name,render in images.items():
            with Image.open(work/render) as image:image.save(output/name,optimize=True)
        data={'schema':1,'kind':'Production UI render with synthetic fixtures; not a live gameplay capture','personal_data':False,'sources':source_manifest(),'images':{n:digest(output/n) for n in images},'renderer_binaries':{'admin':digest(args.admin_test),'loader':digest(args.loader_test)}}
        manifest.write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')
    print('7 previews generated. Synthetic data only; temporary renders removed.')
if __name__=='__main__':main()
