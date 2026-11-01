"""Exercise native traversal diagnostics and a fresh-process snapshot continuation."""

import argparse
import csv
import ctypes
from ctypes import wintypes
import math
import pathlib
import subprocess
import tempfile
import time


def check_trace(exe: pathlib.Path, output: pathlib.Path) -> None:
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    trace = output / "trace"
    report = output / "trace.log"
    result = subprocess.run([
        str(exe), "--community", "--learning", "--gpu-debug",
        "--measure-seconds", "2", "--warmup-seconds", "1",
        "--measurement-trace", str(trace) + "/", "--save", str(output / "trace.save"),
        "--report", str(report),
    ], timeout=9, startupinfo=startup, check=False)
    if result.returncode != 0:
        raise AssertionError(f"measurement trace process failed: {result.returncode}")
    with (trace / "samples.csv").open(encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream)
        expected = ["elapsed_ms", "tick", "phase", "frame_ms", "simulation_ms", "backlog_ms",
                    "working_set_bytes", "private_bytes"]
        if reader.fieldnames != expected:
            raise AssertionError("measurement trace CSV schema changed")
        rows = list(reader)
    measured = [row for row in rows if row["phase"] == "measurement"]
    warmup = [row for row in rows if row["phase"] == "warmup"]
    if not warmup or not measured or rows != warmup + measured:
        raise AssertionError("trace lost warmup, measured samples, or phase order")
    previous = -1.0
    memory_samples = 0
    for row in rows:
        elapsed = float(row["elapsed_ms"])
        if not math.isfinite(elapsed) or elapsed < previous:
            raise AssertionError("trace real-time order is invalid")
        previous = elapsed
        int(row["tick"])
        for key in ("frame_ms", "simulation_ms", "backlog_ms"):
            value = float(row[key])
            if not math.isfinite(value) or value < 0:
                raise AssertionError(f"invalid trace timing: {key}")
        if bool(row["working_set_bytes"]) != bool(row["private_bytes"]):
            raise AssertionError("trace contains only half a memory observation")
        if row["working_set_bytes"]:
            if min(int(row["working_set_bytes"]), int(row["private_bytes"])) <= 0:
                raise AssertionError("native memory observation is not a real process reading")
            memory_samples += 1
    values = dict(line.split("=", 1) for line in report.read_text(encoding="utf-8").splitlines()
                  if "=" in line)
    if memory_samples < 2 or int(values["frame_samples"]) != len(measured):
        raise AssertionError("trace lost memory history or changed measured summary membership")
    if values.get("measurement_trace_complete") != "1" or values.get("normal_shutdown") != "1":
        raise AssertionError("trace did not report successful capture and shutdown")
    for key in ("measurement_trace_observer_ms", "measurement_trace_export_ms"):
        value = float(values[key])
        if not math.isfinite(value) or value < 0:
            raise AssertionError(f"invalid observer cost: {key}")
    times = sorted(float(row["frame_ms"]) for row in measured)
    expected_summary = {
        "frame_min_ms": times[0], "frame_max_ms": times[-1],
        "frame_mean_ms": sum(times) / len(times),
        **{f"frame_p{p}_ms": times[math.ceil(p / 100 * len(times)) - 1]
           for p in (50, 95, 99)},
    }
    for key, expected_value in expected_summary.items():
        # Existing reports round to four decimal places; CSV retains six.
        if abs(float(values[key]) - expected_value) > 0.0001:
            raise AssertionError(f"legacy summary differs from measured trace samples: {key}")
    print("PASS measurement trace phases, memory history and unchanged summary")


def check_trace_paths(exe: pathlib.Path, output: pathlib.Path) -> None:
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    report = output / "preserved.log"
    report.write_bytes(b"preserve report")
    existing = output / "existing"
    existing.mkdir()
    sentinel = existing / "samples.csv"
    sentinel.write_bytes(b"preserve evidence")
    trace = output / "new-trace"
    base = ["--measure-seconds", "1", "--warmup-seconds", "0",
            "--save", str(output / "world.save"), "--report", str(report)]
    cases = [
        ["--measurement-trace", str(trace)],
        ["--smoke-test", "--measurement-trace", str(trace)],
        [*base, "--measurement-trace", str(existing)],
        [*base, "--measurement-trace", str(sentinel)],
        [*base, "--measurement-trace", str(output / "missing" / "trace")],
        [*base, "--measurement-trace", str(trace), "--save", str(trace / "SAMPLES.CSV")],
        [*base, "--measurement-trace", str(trace), "--capture", str(trace / "samples.csv")],
        [*base, "--measurement-trace", str(trace), "--diagnostics", str(trace / "samples.csv")],
        [*base, "--measurement-trace", str(trace), "--report", str(trace / ".." / "new-trace" / "samples.csv")],
        [*base, "--measurement-trace", str(trace), "--report", str(trace / "samples.csv.")],
        [*base, "--measurement-trace", str(trace), "--save", str(trace)],
        [*base, "--measurement-trace", str(trace), "--save", "\\\\?\\" + str(trace / "samples.csv")],
    ]
    for args in cases:
        result = subprocess.run([str(exe), *args], timeout=3, startupinfo=startup, check=False)
        if result.returncode != 1:
            raise AssertionError(f"invalid trace paths succeeded: {args}")
        if report.read_bytes() != b"preserve report" or sentinel.read_bytes() != b"preserve evidence":
            raise AssertionError("trace validation overwrote existing report or evidence")
        if trace.exists():
            raise AssertionError("invalid trace invocation left an output directory")
    print("PASS trace path refusals and preserved evidence")


def check_trace_interruption(exe: pathlib.Path, output: pathlib.Path, collision: bool) -> None:
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    report, trace, save = output / "run.log", output / "trace", output / "world.save"
    duration = "1" if collision else "10"
    with subprocess.Popen([
        str(exe), "--community", "--learning", "--gpu-debug", "--measure-seconds", duration,
        "--warmup-seconds", "1", "--measurement-trace", str(trace),
        "--save", str(save), "--report", str(report),
    ], startupinfo=startup) as process:
        try:
            deadline = time.monotonic() + 6
            while True:
                if process.poll() is not None:
                    raise AssertionError("trace process exited before test intervention")
                if report.exists() and "measurement_initial_checksum=" in report.read_text(encoding="utf-8"):
                    break
                if time.monotonic() >= deadline:
                    raise AssertionError("trace process did not initialize within deadline")
                time.sleep(.02)
            initial = save.read_bytes()
            if collision:
                (trace / "samples.csv").write_bytes(b"preserve competing writer")
            else:
                # Send WM_CLOSE to this process's own main window after completed frames.
                user32 = ctypes.WinDLL("user32", use_last_error=True)
                callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
                user32.EnumWindows.argtypes = [callback_type, wintypes.LPARAM]
                user32.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
                user32.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
                user32.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
                windows = []

                @callback_type
                def find_window(window, _):
                    pid = wintypes.DWORD()
                    user32.GetWindowThreadProcessId(window, ctypes.byref(pid))
                    name = ctypes.create_unicode_buffer(128)
                    user32.GetClassNameW(window, name, len(name))
                    if pid.value == process.pid and name.value == "GenesisTraversalMain":
                        windows.append(window)
                    return True

                time.sleep(.3)
                if not user32.EnumWindows(find_window, 0) or len(windows) != 1:
                    raise AssertionError("could not identify this test process's main window")
                if not user32.PostMessageW(windows[0], 0x0010, 0, 0):
                    raise AssertionError("could not request an orderly early window close")
            result = process.wait(timeout=6)
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=3)
    text = report.read_text(encoding="utf-8")
    values = dict(line.split("=", 1) for line in text.splitlines() if "=" in line)
    if result != 1 or values.get("measurement_trace_complete") != "0" or "normal_shutdown=1" in text:
        raise AssertionError("incomplete or failed export was reported as success")
    if save.read_bytes() != initial or not save.with_suffix(".save.final.save").is_file():
        raise AssertionError("failed/interrupted trace damaged the saved world")
    csv_path = trace / "samples.csv"
    if collision:
        if (csv_path.read_bytes() != b"preserve competing writer" or
                values.get("measurement_trace_exported") != "0" or "measurement_trace_error=" not in text):
            raise AssertionError("failed export overwrote evidence or hid its error")
    else:
        with csv_path.open(encoding="utf-8", newline="") as stream:
            rows = list(csv.DictReader(stream))
        if (values.get("measurement_trace_exported") != "1" or not rows or
                len(rows) != int(values["measurement_trace_samples"]) or
                values.get("measurement_finished") != "0"):
            raise AssertionError("early close lost buffered frames or claimed a complete run")
    print("PASS trace export collision" if collision else "PASS trace orderly interruption")


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
    parser.add_argument("--trace-only", action="store_true")
    parser.add_argument("--trace-case", choices=("paths", "interrupted", "collision"))
    args = parser.parse_args()

    def execute(output: pathlib.Path) -> None:
        if args.trace_case == "paths":
            check_trace_paths(args.exe.resolve(), output)
        elif args.trace_case in ("interrupted", "collision"):
            check_trace_interruption(args.exe.resolve(), output, args.trace_case == "collision")
        elif args.trace_only:
            check_trace(args.exe.resolve(), output)
        else:
            check(args.exe.resolve(), output, args.delivery_only)

    if args.output:
        args.output.mkdir(parents=True, exist_ok=False)
        execute(args.output.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix="genesis-diagnostics-") as directory:
            execute(pathlib.Path(directory))


if __name__ == "__main__":
    main()
