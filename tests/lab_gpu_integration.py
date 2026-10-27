"""A real hardware/device/window integration test. No renderer or domain mocks."""

from pathlib import Path
import struct
import subprocess
import sys
import tempfile


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def main():
    with tempfile.TemporaryDirectory(prefix="genesis-gpu-") as directory:
        root = Path(directory)
        log, save, image = root / "gpu.log", root / "świat.save", root / "scena.png"
        invalid_log = root / "invalid-options.log"
        invalid = subprocess.run(
            [sys.argv[1], "--smoke-test", "--report", str(invalid_log), "--adapter", "invalid"],
            capture_output=True, text=True, timeout=3,
        )
        require(invalid.returncode != 0, "invalid adapter argument was accepted")
        require(invalid_log.exists() and "ERROR:" in invalid_log.read_text(encoding="utf-8"),
                "invalid noninteractive invocation did not produce its error report")
        standard = "--standard" in sys.argv[2:]
        diagnostics = [] if standard else ["--gpu-debug"]
        result = subprocess.run(
            [sys.argv[1], "--smoke-test", *diagnostics, "--report", str(log),
             "--save", str(save), "--capture", str(image)],
            capture_output=True, text=True, timeout=15,
        )
        output = log.read_text(encoding="utf-8") if log.exists() else "NO REPORT"
        print(output)
        require(result.returncode == 0, f"native GPU test exited {result.returncode}: {result.stderr}")
        for marker in ("SMOKE PASS", f"hardware=1 gpu_debug={0 if standard else 1}", "scene_selection=1", "minimized=1", "restored=1", "normal_shutdown=1"):
            require(marker in output, f"missing evidence: {marker}")
        require("gpu_diagnostic=" not in output and "ERROR:" not in output, "GPU diagnostics failed")
        require(output.count("resize=") >= 2, "swap chain did not resize")
        snapshot = save.read_text(encoding="utf-8")
        require("CLOCK 50000000 100\n" in snapshot, "final save has wrong clock")
        require("RESOURCES 1\n3 1 3 0 0 1\n" in snapshot, "final save has wrong material balance")
        data = image.read_bytes()
        require(data[:8] == b"\x89PNG\r\n\x1a\n", "no actual PNG capture")
        width, height = struct.unpack(">II", data[16:24])
        require(width >= 500 and height >= 500, "invalid rendered target size")
        require(image.with_name(image.name + ".before.png").exists(), "initial GPU capture absent")
        print(f"PASS hardware GPU, native UI messages, readback {width}x{height}, persistence and shutdown")


if __name__ == "__main__":
    main()
