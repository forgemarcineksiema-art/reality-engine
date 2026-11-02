"""Native M06 profile regression; old trace schema remains a separate contract."""
import argparse
import csv
import math
from pathlib import Path
import subprocess
import tempfile


def read_csv(path):
    with path.open(encoding="utf-8", newline="") as stream:
        return list(csv.DictReader(stream))


def check(exe, root):
    initial = root / "seed.save"
    baseline = subprocess.run([str(exe), "--community", "--learning", "--measure-seconds", "1",
                               "--warmup-seconds", "0", "--save", str(initial),
                               "--report", str(root / "seed.log")], timeout=10, check=False)
    assert baseline.returncode == 0
    before = initial.read_bytes()
    trace = root / "trace"
    result = subprocess.run([str(exe), "--community", "--learning", "--gpu-debug",
                             "--profile-runtime", "--initial-world", str(initial), "--measure-seconds", "3",
                             "--warmup-seconds", "1", "--measurement-trace", str(trace),
                             "--save", str(root / "world.save"),
                             "--report", str(root / "run.log")], timeout=12, check=False)
    assert result.returncode == 0, f"profile process failed: {result.returncode}"
    assert initial.read_bytes() == before == (root / "world.save").read_bytes()
    frames = read_csv(trace / "profile.csv")
    gpu = read_csv(trace / "gpu.csv")
    old = read_csv(trace / "samples.csv")
    assert len(frames) == len(old) > 0
    assert [int(f["frame"]) for f in frames] == list(range(1, len(frames) + 1))
    assert {f["phase"] for f in frames} == {"warmup", "measurement"}
    assert all(f["phase"] == s["phase"] and f["tick"] == s["tick"]
               for f, s in zip(frames, old))
    for f in frames:
        for key in ("cpu_work_ms", "present_ms", "process_cpu_ms", "navigation_ms",
                    "learning_ms", "save_ms"):
            assert math.isfinite(float(f[key])) and float(f[key]) >= 0, key
    assert sum(int(f["navigation_calls"]) for f in frames) > 0
    assert sum(int(f["learning_calls"]) for f in frames) > 0
    valid = [g for g in gpu if g["status"] == "valid"]
    assert valid, "no real GPU timing was collected"
    ids = [int(g["frame"]) for g in gpu]
    assert len(ids) == len(set(ids)) and all(1 <= i <= len(frames) for i in ids)
    assert all(math.isfinite(float(g["gpu_ms"])) and float(g["gpu_ms"]) >= 0 for g in valid)
    assert all(g["gpu_ms"] == "" for g in gpu if g["status"] != "valid")
    report = dict(line.split("=", 1) for line in (root / "run.log").read_text(
        encoding="utf-8").splitlines() if "=" in line)
    assert report["normal_shutdown"] == "1"
    assert int(report["profile_save_calls"]) >= 2
    assert int(report["profile_gpu_resolved"]) == len(gpu)
    assert len(gpu) + int(report["profile_gpu_pending"]) + int(
        report["profile_gpu_skipped"]) == len(frames)
    assert (root / "world.save.final.save").is_file()
    assert read_csv(trace / "actors.csv"), "actor history was not retained"
    assert read_csv(trace / "progress.csv"), "station progress was not retained"
    for name in ("profile.csv", "gpu.csv", "progress.csv", "actors.csv"):
        alias = root / (name + "-trace")
        failed = subprocess.run([str(exe), "--measure-seconds", "1", "--profile-runtime",
                                  "--measurement-trace", str(alias), "--report", str(alias / name),
                                  "--save", str(root / (name + "-world.save"))],
                                 timeout=4, check=False)
        assert failed.returncode == 1 and not alias.exists(), "profile output alias was accepted"
    failed = subprocess.run([str(exe), "--community", "--learning", "--measure-seconds", "1",
                              "--initial-world", str(initial), "--save", str(initial),
                              "--report", str(root / "initial-alias.log")],
                             timeout=4, check=False)
    assert failed.returncode == 1 and initial.read_bytes() == before, "initial world was overwritten"
    link = root / "hardlink.log"
    link.hardlink_to(initial)
    failed = subprocess.run([str(exe), "--community", "--learning", "--measure-seconds", "1",
                              "--initial-world", str(initial), "--save", str(root / "hardlink.save"),
                              "--report", str(link)], timeout=4, check=False)
    assert failed.returncode == 1 and initial.read_bytes() == before, "hardlink destroyed initial world"
    print("PASS CPU/GPU profile, operation costs and preserved phase trace")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.output:
        args.output.mkdir(parents=True, exist_ok=False)
        check(args.exe.resolve(), args.output.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix="genesis-profile-") as directory:
            check(args.exe.resolve(), Path(directory))
