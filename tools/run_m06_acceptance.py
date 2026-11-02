"""Run a predeclared native M06 protocol and retain every sample and failure."""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import time


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read_csv(path):
    with path.open(encoding="utf-8", newline="") as stream:
        return list(csv.DictReader(stream))


def fields(path):
    return dict(line.split("=", 1) for line in path.read_text(encoding="utf-8").splitlines()
                if "=" in line)


def number(value):
    result = float(value)
    if not math.isfinite(result) or result < 0:
        raise ValueError("nonfinite or negative measurement")
    return result


def percentile(values, fraction):
    return sorted(values)[max(0, math.ceil(len(values) * fraction) - 1)]


def evaluate(root, protocol):
    failures, metrics = [], {}

    def require(condition, message):
        if not condition:
            failures.append(message)

    try:
        root = Path(root)
        report, status = fields(root / "run.log"), fields(root / "trace/status.txt")
        process = json.loads((root / "process.json").read_text(encoding="utf-8"))
        require(process["exit_code"] == 0 and process["initial_world_unchanged"], "process/input integrity")
        retained_protocol = json.loads((root / "protocol.json").read_text(encoding="utf-8"))
        require(retained_protocol == protocol, "frozen protocol changed")
        require(digest(root / "world.save") == protocol["initial_world_sha256"], "initial-world identity")
        for key in ("normal_shutdown", "measurement_finished"):
            require(report.get(key) == "1", key)
        for key in ("measurement_trace_complete", "measurement_trace_exported"):
            require(status.get(key) == "1", key)
        for key in ("simulation_errors", "measurement_interventions", "measurement_minimizations",
                    "measurement_resizes", "occluded_frames"):
            require(report.get(key) == "0", key)
        require((root / "world.save.final.save").stat().st_size > 0, "empty final world")
        require(int(report["profile_save_calls"]) >= 2, "missing measured save costs")
        rows = read_csv(root / "trace/samples.csv")
        profiles = read_csv(root / "trace/profile.csv")
        gpu = read_csv(root / "trace/gpu.csv")
        progress = read_csv(root / "trace/progress.csv")
        measured = [r for r in rows if r["phase"] == "measurement"]
        require(bool(measured), "missing measured phase")
        warmup = [r for r in rows if r["phase"] == "warmup"]
        require(rows == warmup + measured, "invalid phase order")
        if protocol["warmup_seconds"]:
            require(bool(warmup), "missing warmup")
        require(len(profiles) == len(rows), "profile frame count")
        require(len(measured) == int(report["frame_samples"]), "summary membership")
        previous = -1
        for i, (row, profile) in enumerate(zip(rows, profiles), 1):
            elapsed = number(row["elapsed_ms"])
            require(elapsed >= previous, "time reversal")
            previous = elapsed
            require(int(profile["frame"]) == i and profile["tick"] == row["tick"] and
                    profile["phase"] == row["phase"], "profile frame mismatch")
            for key in ("frame_ms", "simulation_ms", "backlog_ms"):
                number(row[key])
            for key in ("cpu_work_ms", "present_ms", "process_cpu_ms", "navigation_ms", "learning_ms", "save_ms"):
                number(profile[key])
        gates = protocol["gates"]
        total_ms = (protocol["warmup_seconds"] + protocol["measure_seconds"]) * 1000

        def history(records, identity_key, expected_ids):
            batches, last_time, last_tick = {}, -1.0, -1
            previous_values = {}
            for row in records:
                elapsed, tick, identity = number(row["elapsed_ms"]), int(row["tick"]), int(row[identity_key])
                require(elapsed >= last_time and tick >= last_tick >= -1 and tick >= 0,
                        identity_key + " history moved backwards")
                last_time, last_tick = elapsed, tick
                batch = batches.setdefault(elapsed, {})
                require(identity not in batch, identity_key + " duplicated row")
                batch[identity] = tick
                if identity_key == "station":
                    completed = int(row["completed"])
                    require(completed >= previous_values.get(identity, 0), "completion counter moved backwards")
                    previous_values[identity] = completed
                else:
                    require(row["autonomous"] in ("0", "1") and
                            0 <= int(row["hunger"]) <= 1000 and 0 <= int(row["energy"]) <= 1000,
                            "invalid actor need")
            times = list(batches)
            require(len(times) >= 2, identity_key + " history truncated")
            if times:
                require(times[0] <= 1500 and times[-1] >= total_ms - 1500 and
                        times[-1] <= total_ms + 1000, identity_key + " phase coverage")
                require(all(0 < b-a <= 1500 for a, b in zip(times, times[1:])),
                        identity_key + " sampling gap")
            for batch in batches.values():
                require(set(batch) == set(expected_ids) and len(set(batch.values())) == 1,
                        identity_key + " identity/tick coverage")
            return [(t, next(iter(batch.values()))) for t, batch in batches.items()]

        station_times = history(progress, "station", protocol["station_ids"])
        actors = read_csv(root / "trace/actors.csv")
        actor_times = history(actors, "actor", protocol["actor_ids"])
        require(actor_times == station_times, "station/actor sample mismatch")
        times = [number(r["frame_ms"]) for r in measured]
        for key, value in (("frame_p95_ms", percentile(times, .95)),
                           ("frame_p99_ms", percentile(times, .99)),
                           ("frame_max_ms", max(times)),
                           ("backlog_max_ms", max(number(r["backlog_ms"]) for r in measured))):
            metrics[key] = value
            require(value <= gates[key], key)
        metrics["frame_p50_ms"] = percentile(times, .5)
        require(int(report["measured_ticks"]) >=
                protocol["measure_seconds"] * 20 * gates["tick_fraction"], "insufficient world ticks")
        require(number(report["measured_wall_ms"]) >= protocol["measure_seconds"] * 990,
                "short measurement")
        memory = [r for r in measured if r["working_set_bytes"]]
        require(len(memory) >= 2, "missing memory history")
        for key in ("working_set_bytes", "private_bytes"):
            values = [int(r[key]) for r in memory]
            peak, growth = max(values), values[-1] - values[0]
            metrics[key + "_peak"] = peak
            metrics[key + "_growth"] = growth
            require(peak <= gates["memory_peak_bytes"], key + " peak")
            require(growth <= gates["memory_growth_bytes"], key + " growth")
        ids = [int(g["frame"]) for g in gpu]
        require(len(ids) == len(set(ids)) and all(1 <= i <= len(rows) for i in ids), "GPU frame IDs")
        valid = [g for g in gpu if g["status"] == "valid"]
        for g in valid:
            number(g["gpu_ms"])
        require(all(g["status"] == "valid" or (g["status"] == "disjoint" and not g["gpu_ms"])
                    for g in gpu), "invalid GPU status")
        require(len(gpu) == int(report["profile_gpu_resolved"]), "GPU resolved count")
        require(len(gpu) + int(report["profile_gpu_pending"]) + int(report["profile_gpu_skipped"])
                == len(rows), "GPU accounting")
        require(len(valid) >= len(rows) * gates["gpu_valid_fraction"], "insufficient GPU timings")
        measured_ids = {i for i, row in enumerate(rows, 1) if row["phase"] == "measurement"}
        gpu_times = [float(g["gpu_ms"]) for g in valid if int(g["frame"]) in measured_ids]
        if gpu_times:
            metrics["gpu_p95_ms"] = percentile(gpu_times, .95)
        for key in ("cpu_work_ms", "present_ms", "process_cpu_ms", "navigation_ms", "learning_ms"):
            values = [float(p[key]) for p in profiles if p["phase"] == "measurement"]
            metrics[key + "_p95"] = percentile(values, .95)
            metrics[key + "_total"] = sum(values)
        # Each time point sums all actual station completions, not intentions.
        totals = {}
        for row in progress:
            elapsed = number(row["elapsed_ms"])
            totals[elapsed] = totals.get(elapsed, 0) + int(row["completed"])
        ordered = sorted(totals.items())
        require(bool(ordered), "missing station history")
        if ordered:
            start = protocol["warmup_seconds"] * 1000
            end = start + protocol["measure_seconds"] * 1000
            window = gates["activity_window_seconds"] * 1000
            previous_total = max((v for t, v in ordered if t <= start), default=ordered[0][1])
            cursor = start
            while cursor < end:
                boundary = min(cursor + window, end)
                current = max((v for t, v in ordered if t <= boundary), default=previous_total)
                require(current > previous_total, f"no completed work in window ending {boundary}")
                previous_total, cursor = current, boundary
            metrics["completed_first"] = ordered[0][1]
            metrics["completed_last"] = ordered[-1][1]
        extremes, longest = {}, 0.0
        require(bool(actors), "missing actor history")
        for actor in actors:
            elapsed, identity = number(actor["elapsed_ms"]), int(actor["actor"])
            bad = actor["autonomous"] == "1" and (int(actor["hunger"]) >= 1000 or int(actor["energy"]) == 0)
            if bad:
                extremes.setdefault(identity, elapsed)
                longest = max(longest, elapsed - extremes[identity])
            else:
                extremes.pop(identity, None)
        metrics["longest_extreme_need_ms"] = longest
        require(longest <= gates["extreme_need_seconds"] * 1000, "prolonged extreme need")
        practices = re.findall(r"practice_actor=\d+ probes=(\d+) records=\d+ method=(\d+)",
                               (root / "run.log").read_text(encoding="utf-8"))
        require(any(int(probes) > 0 and method == "1" for probes, method in practices),
                "no observed probes and learned method")
        if "viewport" in protocol:
            require(report["viewport"] == protocol["viewport"], "viewport changed")
        if "selected_adapter" in protocol:
            require(protocol["selected_adapter"] in report["selected_adapter"], "adapter changed")
    except (OSError, ValueError, KeyError, IndexError, TypeError) as error:
        failures.append(f"invalid/incomplete evidence: {error}")
    return {"passed": not failures, "scope": "measurement gates; final-world restoration and continuation are separate",
            "failures": failures, "metrics": metrics}


def run(protocol):
    exe, initial, root = (Path(protocol[k]) for k in ("executable", "initial_world", "output"))
    for path, key in ((exe, "executable_sha256"), (initial, "initial_world_sha256")):
        if digest(path) != protocol[key]:
            raise ValueError(f"frozen input changed: {key}")
    root.mkdir(parents=True, exist_ok=False)
    (root / "protocol.json").write_text(json.dumps(protocol, indent=2), encoding="utf-8")
    command = [str(exe), "--community", "--learning", "--initial-world", str(initial),
               "--profile-runtime", "--measurement-trace", str(root / "trace"),
               "--warmup-seconds", str(protocol["warmup_seconds"]),
               "--measure-seconds", str(protocol["measure_seconds"]),
               "--adapter", str(protocol.get("adapter_index", 0)),
               "--save", str(root / "world.save"), "--report", str(root / "run.log")]
    started = time.time()
    try:
        result = subprocess.run(command, timeout=protocol["warmup_seconds"] + protocol["measure_seconds"] + 90,
                                check=False, capture_output=True)
    except subprocess.TimeoutExpired as error:
        result = subprocess.CompletedProcess(command, -1, error.stdout or b"", error.stderr or b"")
    (root / "stdout.bin").write_bytes(result.stdout)
    (root / "stderr.bin").write_bytes(result.stderr)
    (root / "process.json").write_text(json.dumps(dict(command=command, exit_code=result.returncode,
        wall_seconds=time.time()-started, initial_world_unchanged=digest(initial) == protocol["initial_world_sha256"]),
        indent=2), encoding="utf-8")
    outcome = evaluate(root, protocol)
    if not (root / "world.save").is_file() or digest(root / "world.save") != protocol["initial_world_sha256"]:
        outcome["passed"] = False
        outcome["failures"].append("initial world was not preserved in application's initial save")
    if digest(initial) != protocol["initial_world_sha256"]:
        outcome["passed"] = False
        outcome["failures"].append("frozen input was modified")
    if result.returncode:
        outcome["passed"] = False
        outcome["failures"].append(f"process exit {result.returncode}")
    (root / "analysis.json").write_text(json.dumps(outcome, indent=2), encoding="utf-8")
    print(json.dumps(outcome, indent=2))
    return outcome["passed"]


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--protocol", type=Path, required=True)
    parser.add_argument("--analyze-dir", type=Path)
    args = parser.parse_args()
    protocol = json.loads(args.protocol.read_text(encoding="utf-8"))
    if args.analyze_dir:
        outcome = evaluate(args.analyze_dir, protocol)
        print(json.dumps(outcome, indent=2))
        raise SystemExit(0 if outcome["passed"] else 1)
    raise SystemExit(0 if run(protocol) else 1)
