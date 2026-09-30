"""Pinned, static FreeType build. Network is needed only on the first build."""
from pathlib import Path
import hashlib,shutil,subprocess,tarfile,urllib.request
ROOT=Path(__file__).resolve().parents[1]
SHA='44bd69d1f0750603410cfd4f26f1c5523c5a3a087a2dfa8639b757dc7c6677be'
URL='https://codeload.github.com/freetype/freetype/tar.gz/refs/tags/VER-2-14-1'
def prepare(env):
    base=ROOT/'build/freetype';base.mkdir(parents=True,exist_ok=True)
    archive=ROOT/'build/dependency-downloads/freetype.tar.gz'
    if not archive.exists():
        archive.parent.mkdir(parents=True,exist_ok=True)
        data=urllib.request.urlopen(URL,timeout=60).read()
        if hashlib.sha256(data).hexdigest()!=SHA:raise RuntimeError('FreeType archive hash mismatch')
        archive.write_bytes(data)
    if hashlib.sha256(archive.read_bytes()).hexdigest()!=SHA:raise RuntimeError('FreeType archive hash mismatch')
    source=base/'freetype-VER-2-14-1'
    if not source.exists():
        with tarfile.open(archive) as bundle:bundle.extractall(base,filter='data')
    binary=base/'static';library=binary/'freetype.lib'
    if not library.exists():
        cmake=shutil.which('cmake',path=env['PATH'])
        if not cmake:
            vs=Path(env['VSINSTALLDIR'])
            cmake=str(vs/'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe')
        command=[cmake,'-S',str(source),'-B',str(binary),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded','-DBUILD_SHARED_LIBS=OFF']
        command += ['-DFT_DISABLE_'+name+'=TRUE' for name in ['ZLIB','BZIP2','PNG','HARFBUZZ','BROTLI']]
        with (base/'build.log').open('w',encoding='utf-8') as log:
            for step in [command,[cmake,'--build',str(binary),'--parallel','4']]:
                result=subprocess.run(step,env=env,stdout=log,stderr=subprocess.STDOUT)
                if result.returncode:raise RuntimeError('FreeType build failed; see '+str(base/'build.log'))
    return [f'/I{source / "include"}',f'/I{ROOT / "imgui"}',f'/I{ROOT / "imgui/misc/freetype"}'],str(library),source/'docs/FTL.TXT'
