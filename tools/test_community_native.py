"""Integrated eight-person M04 scene, real Win32 controls and fresh-process continuation."""
import argparse
import pathlib
import subprocess
import tempfile


def check(exe, output):
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    save = output / "community.save"

    def run(name, resume=False):
        command = [str(exe), "--community", "--smoke-test", "--gpu-debug", "--report", str(output / f"{name}.log"), "--save", str(save), "--capture", str(output / f"{name}.png")]
        if resume:
            command.append("--resume-smoke")
        result = subprocess.run(command, startupinfo=startup, timeout=18, check=False)
        if result.returncode:
            raise AssertionError(f"native community {name} failed: {result.returncode}")
        text = (output / f"{name}.log").read_text(encoding="utf-8")
        for marker in ("TRAVERSAL SMOKE PASS", "community_panel=pass", "hardware=1", "gpu_debug=1", "simulation_errors=0"):
            if marker not in text:
                raise AssertionError(f"native community {name} missing {marker}")
        return text

    first = run("community")
    for marker in ("community_foreign_save=pass", "community_share=pass", "community_passage=pass", "community_law=pass", "community_work=pass"):
        if marker not in first:
            raise AssertionError(f"missing integrated behavior {marker}")
    expected = pathlib.Path(str(save) + ".expected.save").read_bytes()
    before = save.read_bytes()
    for record in (b"GENESIS_TRAVERSAL 7\n", b"PASSAGE_BELIEF ", b"INCIDENT ", b"TESTIMONY ", b"ACTORS 8\n"):
        if record not in before:
            raise AssertionError(f"missing persisted integrated state {record!r}")
    if "community_resume=pass" not in run("resume", True) or save.read_bytes() != before or pathlib.Path(str(save) + ".expected.save").read_bytes() != expected:
        raise AssertionError("community continuation failed or overwrote original evidence")
    print("PASS integrated community: physical controls, eight actors, law, work, food and v7 continuation")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path)
    args = parser.parse_args()
    if args.output:
        args.output.mkdir(parents=True, exist_ok=False)
        check(args.exe.resolve(), args.output.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix="genesis-community-") as directory:
            check(args.exe.resolve(), pathlib.Path(directory))


if __name__ == "__main__":
    main()
