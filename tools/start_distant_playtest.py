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
    args = parser.parse_args()
    exe = args.exe.resolve()
    if not exe.is_file():
        parser.error("Build first: cmake --build --preset windows-release")
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    output = root / "build" / f"distant-playtest-{stamp}-{uuid.uuid4().hex[:8]}"
    output.mkdir(parents=True, exist_ok=False)
    command = [str(exe), "--distant-delivery", "--save", str(output / "world.save"), "--report", str(output / "session.log"), "--capture", str(output / "scene.png"), "--diagnostics", str(output / "diagnostics")]
    if args.smoke_test:
        command += ["--smoke-test", "--gpu-debug"]
    print(f"Evidence: {output}", flush=True)
    print("Space: run. 1: courier. F2: autonomy. F5/F9: save/load. Cyan: terminals.", flush=True)
    result = subprocess.run(command, check=False, timeout=18 if args.smoke_test else None)
    raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
