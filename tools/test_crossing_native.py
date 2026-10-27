"""Native bridge interaction, material repair and a fresh v6 process continuation."""
import argparse
import pathlib
import subprocess
import tempfile

def check(exe: pathlib.Path, output: pathlib.Path) -> None:
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    save = output / "crossing.save"
    def run(name: str, resume: bool = False) -> str:
        report = output / f"{name}.log"
        command = [str(exe), "--crossing", "--smoke-test", "--gpu-debug", "--report", str(report),
                   "--save", str(save), "--capture", str(output / f"{name}.png")]
        if resume:
            command += ["--resume-smoke"]
        result = subprocess.run(command, startupinfo=startup, timeout=8, check=False)
        if result.returncode:
            raise AssertionError(f"native crossing {name} failed: {result.returncode}")
        text = report.read_text(encoding="utf-8")
        for value in ("TRAVERSAL SMOKE PASS", "passage_inspector=pass", "simulation_errors=0", "gpu_debug=1"):
            if value not in text:
                raise AssertionError(f"native crossing {name} missing {value}")
        return text
    first = run("crossing")
    for value in ("passage_shortcut=pass", "maintenance_shortcut=pass", "crossing_delivery=pass"):
        if value not in first:
            raise AssertionError(f"missing actual crossing result: {value}")
    before = save.read_bytes()
    if not before.startswith(b"GENESIS_TRAVERSAL 6\n") or b"PASSAGE_BELIEF " not in before:
        raise AssertionError("crossing did not save v6 passage knowledge")
    if "passage_resume=pass" not in run("resume", True) or save.read_bytes() != before:
        raise AssertionError("fresh crossing process failed or overwrote evidence")
    print("PASS native crossing controls, material repair, delivery, inspector and v6 continuation")

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path)
    args = parser.parse_args()
    if args.output:
        args.output.mkdir(parents=True, exist_ok=False)
        check(args.exe.resolve(), args.output.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix="genesis-crossing-") as directory:
            check(args.exe.resolve(), pathlib.Path(directory))
if __name__ == "__main__":
    main()
