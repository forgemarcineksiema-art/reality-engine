"""Continue four live practice phases in a new process and compare full saved bytes."""
import pathlib
import subprocess
import sys
import tempfile


def main():
    exe = pathlib.Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="genesis-practice-") as directory:
        root = pathlib.Path(directory)
        for stage in range(4):
            checkpoint, expected, actual = [root / f"{stage}-{s}.save" for s in ("checkpoint", "expected", "actual")]
            subprocess.run([str(exe), "checkpoint", str(stage), str(checkpoint), str(expected), "20"], check=True, timeout=8)
            original = checkpoint.read_bytes()
            subprocess.run([str(exe), "continue", str(checkpoint), str(actual), "20"], check=True, timeout=8)
            assert actual.read_bytes() == expected.read_bytes(), f"phase {stage} differs"
            assert checkpoint.read_bytes() == original, "checkpoint overwritten"
    print("PASS four fresh-process practice continuations")


if __name__ == "__main__":
    main()
