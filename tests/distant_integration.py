"""Continue distant travel, occupied return and cancelled cargo in distinct processes."""
import pathlib
import subprocess
import sys
import tempfile


def main():
    exe = pathlib.Path(sys.argv[1]).resolve()
    method = sys.argv[2]
    assert method in ("travel", "waiting", "cancelled"), "unknown delivery method"
    with tempfile.TemporaryDirectory(prefix="genesis-distant-") as directory:
        root = pathlib.Path(directory)
        checkpoint, expected, actual = [root / f"{method}-{name}.save" for name in ("checkpoint", "expected", "actual")]
        subprocess.run([str(exe), "checkpoint", method, str(checkpoint), str(expected)], check=True, timeout=8)
        before = checkpoint.read_bytes()
        subprocess.run([str(exe), "continue", method, str(checkpoint), str(actual)], check=True, timeout=8)
        assert actual.read_bytes() == expected.read_bytes(), f"{method} continuation diverged"
        assert checkpoint.read_bytes() == before, "checkpoint overwritten"
    print(f"PASS {method} continued in a fresh process")


if __name__ == "__main__":
    main()
