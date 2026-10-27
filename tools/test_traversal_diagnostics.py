"""Exercise native traversal diagnostics and a fresh-process snapshot continuation."""

import argparse
import pathlib
import subprocess
import tempfile


def check(exe: pathlib.Path, output: pathlib.Path, delivery_only: bool = False) -> None:
    report = output / "native.log"
    save = output / "ordinary.save"
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0

    def run(arguments: list[str]) -> None:
        result = subprocess.run(
            [str(exe), "--smoke-test", "--gpu-debug", *arguments],
            timeout=15,
            startupinfo=startup,
            check=False,
        )
        if result.returncode != 0:
            raise AssertionError(f"native process failed: {result.returncode}; {arguments}")

    if delivery_only:
        delivery_report = output / "delivery.log"
        delivery_diagnostics = output / "delivery-diagnostics"
        run(["--delivery", "--report", str(delivery_report),
             "--save", str(output / "delivery.save"), "--diagnostics", str(delivery_diagnostics)])
        delivery_text = delivery_report.read_text(encoding="utf-8")
        if ("TRAVERSAL SMOKE PASS" not in delivery_text or "cargo_lot=" not in delivery_text or
                "delivery_phase=" not in delivery_text or "simulation_errors=0" not in delivery_text):
            raise AssertionError("native delivery scene did not expose its actual courier state")
        delivery_snapshots = list(delivery_diagnostics.glob("*/world.save"))
        if len(delivery_snapshots) != 1 or not delivery_snapshots[0].read_bytes().startswith(
                b"GENESIS_TRAVERSAL 6\n"):
            raise AssertionError("native delivery diagnostic did not save the observed version 6 world")
        if b"\nBELIEF " not in delivery_snapshots[0].read_bytes():
            raise AssertionError("native delivery diagnostic lost station observations")
        if b"\nPASSAGE_BELIEF " not in delivery_snapshots[0].read_bytes():
            raise AssertionError("native delivery diagnostic lost local passage observations")
        delivery_before = delivery_snapshots[0].read_bytes()
        delivery_resume = output / "delivery-resume.log"
        run(["--resume-smoke", "--report", str(delivery_resume),
             "--save", str(delivery_snapshots[0]), "--diagnostics", str(output / "delivery-resume")])
        if ("TRAVERSAL SMOKE PASS" not in delivery_resume.read_text(encoding="utf-8") or
                delivery_snapshots[0].read_bytes() != delivery_before):
            raise AssertionError("fresh native process did not preserve and resume the delivery world")
        return

    run(["--report", str(report), "--save", str(save), "--capture", str(output / "native.png")])
    text = report.read_text(encoding="utf-8")
    if "navigation_observations=8" not in text:
        raise AssertionError("native app does not report observed navigation progress")
    values = dict(line.split("=", 1) for line in text.splitlines() if "=" in line)
    if values.get("inspector_records") != "pass":
        raise AssertionError("native inspector did not preserve separate actor records")
    if values.get("inspector_scroll") != "pass":
        raise AssertionError("native inspector reset its scroll during live updates")
    if values.get("clearance_shortcut") != "pass":
        raise AssertionError("native clearance shortcut failed its no-op check")
    if values.get("cargo_shortcut") != "pass":
        raise AssertionError("native cargo shortcut failed its remote interaction check")
    for key in ("knowledge_shortcut", "knowledge_exchange", "knowledge_inspector"):
        if values.get(key) != "pass":
            raise AssertionError(f"native knowledge interaction failed: {key}")
    if int(values.get("frame_samples", "0")) < 2:
        raise AssertionError("native app does not record actual frame timings")
    if not (0 < float(values["frame_p50_ms"]) <= float(values["frame_p95_ms"]) <=
            float(values["frame_p99_ms"]) <= float(values["frame_max_ms"])):
        raise AssertionError("native frame timing distribution is invalid")
    snapshots = list((output / "traversal-diagnostics").glob("*/world.save"))
    if len(snapshots) != 1 or not (snapshots[0].parent / "diagnostic.txt").is_file():
        raise AssertionError("F6 did not create one complete diagnostic snapshot")
    before = snapshots[0].read_bytes()
    resume = output / "resume.log"
    run(["--resume-smoke", "--report", str(resume), "--save", str(snapshots[0]),
         "--capture", str(output / "resume.png")])
    if "TRAVERSAL SMOKE PASS" not in resume.read_text(encoding="utf-8"):
        raise AssertionError("fresh process did not resume diagnostic world")
    if snapshots[0].read_bytes() != before:
        raise AssertionError("diagnostic resume overwrote the evidence")
    for bad in (["--smoke-test", "--measure-seconds", "0"],
                ["--measure-seconds", "-1"], ["--measure-seconds"],
                ["--warmup-seconds", "301"], ["--warmup-seconds", "1"]):
        invalid = subprocess.run([str(exe), *bad], timeout=3, startupinfo=startup, check=False)
        if invalid.returncode != 1:
            raise AssertionError(f"invalid unattended arguments did not return failure: {bad}")
    measured = output / "measured.log"
    result = subprocess.run([str(exe), "--measure-seconds", "1", "--warmup-seconds", "0",
                             "--save", str(output / "measurement.save"), "--report", str(measured)],
                            timeout=6, startupinfo=startup, check=False)
    measured_text = measured.read_text(encoding="utf-8") if measured.exists() else ""
    measured_values = dict(line.split("=", 1) for line in measured_text.splitlines() if "=" in line)
    if result.returncode != 0 or measured_values.get("measurement_finished") != "1" or int(
            measured_values.get("measured_ticks", "0")) == 0:
        raise AssertionError("bounded native measurement failed to run simulation and finish")
    print("PASS native progress, frame metrics, F6 snapshot and fresh-process continuation")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--delivery-only", action="store_true")
    args = parser.parse_args()
    if args.output:
        args.output.mkdir(parents=True, exist_ok=False)
        check(args.exe.resolve(), args.output.resolve(), args.delivery_only)
    else:
        with tempfile.TemporaryDirectory(prefix="genesis-diagnostics-") as directory:
            check(args.exe.resolve(), pathlib.Path(directory), args.delivery_only)


if __name__ == "__main__":
    main()
