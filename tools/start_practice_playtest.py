"""Open the shared prototype with discovery and separate evidence paths."""
import argparse
import datetime
import pathlib
import subprocess
import uuid


def main():
    root = pathlib.Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=pathlib.Path, default=root / "build/msvc/RelWithDebInfo/reality_traversal.exe")
    parser.add_argument("--smoke-test", action="store_true")
    parser.add_argument("--two-methods", action="store_true")
    args = parser.parse_args()
    exe = args.exe.resolve()
    if not exe.is_file():
        parser.error("Build first: cmake --build --preset windows-release")
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    output = root / "build" / f"practice-playtest-{stamp}-{uuid.uuid4().hex[:8]}"
    output.mkdir(parents=True, exist_ok=False)
    command = [str(exe), "--community", "--learning", "--save", str(output / "world.save"), "--report", str(output / "session.log"), "--capture", str(output / "scene.png"), "--diagnostics", str(output / "diagnostics")]
    if args.two_methods:
        command += ["--two-methods"]
    if args.smoke_test:
        command += ["--smoke-test", "--gpu-debug"]
    print(f"Evidence: {output}", flush=True)
    print("Space: run. 4/6: workers. F2: take control. I/O/P: probe. R: work. F7: share. F5/F9: save/load.", flush=True)
    if args.two_methods:
        print("Detour: 2, F2. Repair: 4, F2; after the bridge opens: 2, F2. F2 toggles autonomy.", flush=True)
    result = subprocess.run(command, check=False, timeout=18 if args.smoke_test else None)
    raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
