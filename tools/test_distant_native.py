"""Real Win32/D3D11 practice controls, shared scene and save/load behavior."""
import argparse
import pathlib
import subprocess
import tempfile


def check(exe, root):
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    report = root / "practice.log"
    command = [str(exe), "--distant-delivery"]
    subprocess.run(command + ["--smoke-test", "--gpu-debug", "--report", str(report), "--save", str(root / "practice.save"), "--capture", str(root / "practice.png")], startupinfo=startup, check=True, timeout=18)
    text = report.read_text(encoding="utf-8")
    markers = ("distant_checkpoint=pass", "distant_foreign_save=pass", "distant_delivery=pass")
    for marker in markers + ("TRAVERSAL SMOKE PASS", "hardware=1", "gpu_debug=1", "simulation_errors=0"):
        assert marker in text, f"missing {marker}"
    print("PASS native distant: physical cargo, ownership, GPU and foreign saves")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path)
    args = parser.parse_args()
    if args.output:
        args.output.mkdir(parents=True, exist_ok=False)
        check(args.exe.resolve(), args.output.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix="genesis-practice-native-") as directory:
            check(args.exe.resolve(), pathlib.Path(directory))


if __name__ == "__main__":
    main()
