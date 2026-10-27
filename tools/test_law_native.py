"""Real D3D11 law controls, inspector and version 7 continuation in another process."""
import argparse
import pathlib
import subprocess
import tempfile


def check(exe: pathlib.Path, output: pathlib.Path) -> None:
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    save = output / "law.save"

    def run(name: str, resume: bool = False) -> str:
        command = [str(exe), "--law", "--smoke-test", "--gpu-debug", "--report",
                   str(output / f"{name}.log"), "--save", str(save),
                   "--capture", str(output / f"{name}.png")]
        if resume:
            command.append("--resume-smoke")
        result = subprocess.run(command, startupinfo=startup, timeout=8, check=False)
        if result.returncode:
            raise AssertionError(f"native law {name} failed: {result.returncode}")
        text = (output / f"{name}.log").read_text(encoding="utf-8")
        for marker in ("TRAVERSAL SMOKE PASS", "law_inspector=pass", "simulation_errors=0", "gpu_debug=1"):
            if marker not in text:
                raise AssertionError(f"native law {name} missing {marker}")
        return text

    first = run("law")
    for marker in ("law_pickup=pass", "law_exchange=pass", "law_refusal=pass"):
        if marker not in first:
            raise AssertionError(f"missing actual law behavior: {marker}")
    before = save.read_bytes()
    if not before.startswith(b"GENESIS_TRAVERSAL 7\n") or b"TESTIMONY " not in before:
        raise AssertionError("native law did not save v7 testimony")
    if "law_resume=pass" not in run("resume", True) or save.read_bytes() != before:
        raise AssertionError("fresh law process failed or overwrote evidence")
    print("PASS native law F4/F7, refusal, inspector, physical return and v7 continuation")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path)
    args = parser.parse_args()
    if args.output:
        args.output.mkdir(parents=True, exist_ok=False)
        check(args.exe.resolve(), args.output.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix="genesis-law-") as directory:
            check(args.exe.resolve(), pathlib.Path(directory))


if __name__ == "__main__":
    main()
