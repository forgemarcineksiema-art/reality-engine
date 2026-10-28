"""Real Win32/D3D11 practice controls, shared scene and save/load behavior."""
import argparse
import pathlib
import subprocess
import tempfile


def check(exe, root, two_methods=False, method="detour"):
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    report = root / "practice.log"
    command = [str(exe), "--two-methods"] if two_methods else [str(exe), "--community", "--learning"]
    if two_methods and method == "repair":
        command += ["--repair-smoke"]
    subprocess.run(command + ["--smoke-test", "--gpu-debug", "--report", str(report), "--save", str(root / "practice.save"), "--capture", str(root / "practice.png")], startupinfo=startup, check=True, timeout=18)
    text = report.read_text(encoding="utf-8")
    markers = (f"two_methods_{method}=pass", "two_methods_foreign_save=pass", "two_methods_panel=pass") if two_methods else ("practice_controls=pass", "practice_foreign_save=pass", "practice_panel=pass")
    for marker in markers + ("TRAVERSAL SMOKE PASS", "hardware=1", "gpu_debug=1", "simulation_errors=0"):
        assert marker in text, f"missing {marker}"
    print("PASS native practice: physical controls, learning, GPU and foreign saves")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--two-methods", action="store_true")
    parser.add_argument("--method", choices=("repair", "detour"), default="detour")
    args = parser.parse_args()
    if args.output:
        args.output.mkdir(parents=True, exist_ok=False)
        check(args.exe.resolve(), args.output.resolve(), args.two_methods, args.method)
    else:
        with tempfile.TemporaryDirectory(prefix="genesis-practice-native-") as directory:
            check(args.exe.resolve(), pathlib.Path(directory), args.two_methods, args.method)


if __name__ == "__main__":
    main()
