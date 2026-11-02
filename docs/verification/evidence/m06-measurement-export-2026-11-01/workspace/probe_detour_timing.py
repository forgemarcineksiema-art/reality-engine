"""Diagnostic only: no acceptance deadline or assertion is changed."""
import argparse
import ctypes
from ctypes import wintypes
import json
import os
import pathlib
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument("name")
parser.add_argument("--prefix-native", action="store_true")
args = parser.parse_args()
root = pathlib.Path.cwd()
output = root / "build/m06-measurement-export-workspace" / args.name
output.mkdir(exist_ok=False)
environment = os.environ.copy()
environment.update(ASAN_OPTIONS="halt_on_error=1", ASAN_VCASAN_DEBUGGING="0")
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
kernel32.GetProcessTimes.argtypes = [wintypes.HANDLE, *[ctypes.POINTER(wintypes.FILETIME)] * 4]
kernel32.GetSystemTimes.argtypes = [ctypes.POINTER(wintypes.FILETIME)] * 3

def ticks(value):
    return (value.dwHighDateTime << 32) | value.dwLowDateTime

def system_times():
    values = [wintypes.FILETIME() for _ in range(3)]
    if not kernel32.GetSystemTimes(*(ctypes.byref(value) for value in values)):
        raise ctypes.WinError(ctypes.get_last_error())
    return [ticks(value) for value in values]

if args.prefix_native:
    with (output / "prefix.log").open("wb") as log:
        subprocess.run(["ctest", "--preset", "windows-asan", "-R", "^two_methods[.]native_repair$", "-V"],
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=25)
exe = root / "build/asan/RelWithDebInfo/reality_two_methods_tests.exe"
command = [str(exe), "checkpoint", "detour", str(output / "checkpoint.save"), str(output / "expected.save")]
before = system_times()
started = time.perf_counter()
with subprocess.Popen(command, env=environment, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as process:
    try:
        stdout, stderr = process.communicate(timeout=30)
    except subprocess.TimeoutExpired:
        process.kill()
        stdout, stderr = process.communicate()
    elapsed = time.perf_counter() - started
    values = [wintypes.FILETIME() for _ in range(4)]
    if not kernel32.GetProcessTimes(wintypes.HANDLE(int(process._handle)),
                                    *(ctypes.byref(value) for value in values)):
        raise ctypes.WinError(ctypes.get_last_error())
    processor_seconds = (ticks(values[2]) + ticks(values[3])) / 10000000
    returncode = process.returncode
after = system_times()
idle, kernel, user = [last - first for first, last in zip(before, after)]
(output / "checkpoint.stdout.log").write_bytes(stdout)
(output / "checkpoint.stderr.log").write_bytes(stderr)
result = {"diagnostic_only": True, "acceptance_deadlines_unchanged": True,
          "diagnostic_outer_limit_seconds": 30, "returncode": returncode,
          "wall_seconds": elapsed, "process_cpu_seconds": processor_seconds,
          "system_busy_percent": 100 * (kernel + user - idle) / (kernel + user),
          "within_existing_checkpoint_budget": elapsed <= 8,
          "prefix_native": args.prefix_native}
(output / "timing.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps(result))
print(stdout.decode("utf-8"))
