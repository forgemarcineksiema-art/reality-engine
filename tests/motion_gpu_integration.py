"""Real D3D11 motion course, native input and continuation in a fresh process."""

from pathlib import Path
import hashlib
import re
import shutil
import struct
import subprocess
import sys
import tempfile


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def main():
    with tempfile.TemporaryDirectory(prefix="genesis-motion-") as directory:
        root = Path(directory)
        save = root / "podróż.save"
        checksums = []
        for resume in (False, True):
            log = root / ("resume.log" if resume else "motion.log")
            image = root / ("resume.png" if resume else "motion.png")
            command = [sys.argv[1], "--motion", "--smoke-test", "--gpu-debug",
                       "--report", str(log), "--save", str(save), "--capture", str(image)]
            if resume:
                command.append("--resume-smoke")
            result = subprocess.run(command, capture_output=True, text=True, timeout=15)
            output = log.read_text(encoding="utf-8") if log.exists() else "NO REPORT"
            print(output)
            require(result.returncode == 0, f"motion process failed: {result.returncode}: {result.stderr}")
            for marker in ("MOTION SMOKE PASS", "hardware=1 gpu_debug=1", "normal_shutdown=1",
                           "minimized=1", "restored=1", "arrived"):
                require(marker in output, f"missing evidence: {marker}")
            require("gpu_diagnostic=" not in output and "ERROR:" not in output, "GPU diagnostics failed")
            checksum = re.search(r"continuation_checksum=(\d+)", output)
            require(checksum is not None, "continuation checksum absent")
            checksums.append(checksum.group(1))
            data = image.read_bytes()
            require(data[:8] == b"\x89PNG\r\n\x1a\n", "capture is not PNG")
            width, height = struct.unpack(">II", data[16:24])
            require(width >= 500 and height >= 500, "invalid capture size")
            if not resume:
                require("interpolation_sample previous_x=" in output, "native render interpolation sample absent")
                require(image.with_name(image.name + ".interpolated.png").exists(), "interpolated hardware frame absent")
        require(checksums[0] == checksums[1], "fresh-process motion/work continuation diverged")
        require(save.read_bytes().startswith(b"GENESIS_MOTION 1\n"), "wrong save format")
        # Immutable fixture produced by the pre-polish executable, not by this test's codec.
        fixture = Path(sys.argv[2])
        require(hashlib.sha256(fixture.read_bytes()).hexdigest() ==
                "564495c65638d8a9099c8da7ea926e398b5bafd63989d560543c5b7de5486e3a",
                "legacy in-flight fixture changed")
        legacy = root / "legacy.save"
        shutil.copyfile(fixture, legacy)
        legacy_log, legacy_image = root / "legacy.log", root / "legacy.png"
        result = subprocess.run(
            [sys.argv[1], "--motion", "--smoke-test", "--resume-smoke", "--gpu-debug",
             "--report", str(legacy_log), "--save", str(legacy), "--capture", str(legacy_image)],
            capture_output=True, text=True, timeout=15,
        )
        output = legacy_log.read_text(encoding="utf-8") if legacy_log.exists() else "NO REPORT"
        print(output)
        require(result.returncode == 0, f"legacy process failed: {result.returncode}: {result.stderr}")
        for marker in ("MOTION SMOKE PASS", "hardware=1 gpu_debug=1", "normal_shutdown=1",
                       "actor=1 x=7.00 z=0.00 arrived", "minimized=1 tick=370"):
            require(marker in output, f"legacy continuation missing: {marker}")
        require("gpu_diagnostic=" not in output and "ERROR:" not in output, "legacy GPU diagnostics failed")
        snapshot = legacy.read_text(encoding="utf-8")
        require("CLOCK 50000000 370\n" in snapshot, "legacy clock was reset")
        require("RESOURCES 1\n3 1 3 0 0 1\n" in snapshot, "legacy work/material continuation changed")
        print("PASS hardware motion, native keys/picking/focus, resize, fresh-process travel and work")
        print("PASS pre-polish version-1 in-flight snapshot, identities, clock and completed work")


if __name__ == "__main__":
    main()
