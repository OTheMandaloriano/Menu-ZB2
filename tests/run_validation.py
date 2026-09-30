from argparse import ArgumentParser
from concurrent.futures import ThreadPoolExecutor
from hashlib import sha256
from json import dumps
from os import environ
from pathlib import Path
from shutil import which
from subprocess import run
from sys import exit
from time import perf_counter
from xml.etree.ElementTree import parse

parser = ArgumentParser()
parser.add_argument("--sanitize", action="store_true")
parser.add_argument("--output", type=Path)
args = parser.parse_args()
source = Path(__file__).resolve().parents[1]
output = (args.output or source / "build" / "validation").resolve()
output.mkdir(parents=True, exist_ok=True)
build = output / "objects"
build.mkdir(exist_ok=True)
compiler = which("g++")
if not compiler:
    exit("g++ with C++17 support is required for these portable tests.")
flags = ["-I" + str(source),"-std=c++17", "-O0", "-g", "-I" + str(source / "tests" / "platform")]
if args.sanitize:
    flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
units = [source / "imgui" / (name + ".cpp") for name in ("imgui", "imgui_draw", "imgui_tables", "imgui_widgets")]
units += [source / "src/menu/esp_layout.cpp", source / "tests" / "layout_tests.cpp", source / "tests" / "gui_harness.cpp"]
records = []


def compile_unit(unit):
    target = build / (unit.stem + ".o")
    command = [compiler, *flags, "-c", str(unit), "-o", str(target)]
    completed = run(command, capture_output=True, text=True)
    return target, command, completed


started = perf_counter()
with ThreadPoolExecutor(max_workers=2) as pool:
    for target, command, completed in pool.map(compile_unit, units):
        records.append({"command": command, "exit_code": completed.returncode, "output": completed.stdout + completed.stderr})
        if completed.returncode:
            print(completed.stderr)
            (output / "results.json").write_text(dumps(records, indent=2), encoding="utf-8")
            exit(completed.returncode)
shared = [build / (unit.stem + ".o") for unit in units[:5]]
process_env = dict(environ)
if args.sanitize:
    process_env["ASAN_OPTIONS"] = "detect_leaks=0"
for name in ("layout_tests", "gui_harness"):
    executable = build / name
    command = [compiler, *flags, *map(str, shared), str(build / (name + ".o")), "-o", str(executable)]
    linked = run(command, capture_output=True, text=True)
    records.append({"command": command, "exit_code": linked.returncode, "output": linked.stdout + linked.stderr})
    if linked.returncode:
        print(linked.stderr)
        exit(linked.returncode)
    command = [str(executable)] + ([str(output / "renders")] if name == "gui_harness" else [])
    tested = run(command, capture_output=True, text=True, env=process_env)
    records.append({"command": command, "exit_code": tested.returncode, "output": tested.stdout + tested.stderr})
    print(tested.stdout, end="", flush=True)
    if tested.returncode:
        print(tested.stderr)
        (output / "results.json").write_text(dumps(records, indent=2), encoding="utf-8")
        exit(tested.returncode)
project = parse(source / "Deadblock.Menu.vcxproj")
namespace = {"m": "http://schemas.microsoft.com/developer/msbuild/2003"}
for kind in ("ClCompile", "ClInclude"):
    for item in project.findall(".//m:" + kind, namespace):
        if "Include" in item.attrib:
            assert (source / item.attrib["Include"].replace("\\", "/")).is_file()
mono = (source / "src/menu/mono.cpp").read_text(encoding="utf-8-sig")
assert "EspEntry tmp[128] = {};" in mono and "EspEntry tmpEn = {};" in mono
try:
    from PIL import Image
    for path in (output / "renders").glob("*.ppm"):
        with Image.open(path) as image:
            image.load()
            image.save(path.with_suffix(".png"))
        with Image.open(path.with_suffix(".png")) as image:
            image.load()
except ModuleNotFoundError:
    print("Pillow unavailable: screenshots are available as lossless PPM files.")
files = [path for path in source.rglob("*") if path.is_file() and "build" not in path.relative_to(source).parts]
report = {"status": "PASS", "sanitizers": "AddressSanitizer + UndefinedBehaviorSanitizer" if args.sanitize else "none", "leak_sanitizer": "disabled under traced executor" if args.sanitize else "not run", "native_windows_build": "NOT RUN", "game_runtime": "NOT RUN", "duration_seconds": round(perf_counter() - started, 2), "commands": records, "source_sha256": {str(path.relative_to(source)): sha256(path.read_bytes()).hexdigest() for path in sorted(files)}}
(output / "results.json").write_text(dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
print("PASS: project references and initialized snapshot fields; report:", output / "results.json")
