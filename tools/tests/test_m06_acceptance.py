import csv
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("m06", Path(__file__).parents[1] / "run_m06_acceptance.py")
m06 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m06)


class AcceptanceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "trace").mkdir()
        self.protocol = {"measure_seconds": 1, "warmup_seconds": 0,
                         "actor_ids": [1], "station_ids": [0],
                         "gates": {"frame_p95_ms": 20, "frame_p99_ms": 34,
                                   "frame_max_ms": 250, "backlog_max_ms": 1000,
                                   "memory_peak_bytes": 512 * 1024**2,
                                   "memory_growth_bytes": 64 * 1024**2,
                                   "tick_fraction": .95, "activity_window_seconds": 300,
                                   "gpu_valid_fraction": .95, "extreme_need_seconds": .5}}
        self.write("samples.csv", [dict(elapsed_ms=500, tick=10, phase="measurement",
                   frame_ms=16.67, simulation_ms=1, backlog_ms=0,
                   working_set_bytes=100, private_bytes=100),
                   dict(elapsed_ms=1000, tick=20, phase="measurement", frame_ms=16.67,
                        simulation_ms=1, backlog_ms=0, working_set_bytes=101, private_bytes=101)])
        self.write("profile.csv", [dict(frame=i, navigation_calls=1, learning_calls=1,
                   process_cpu_ms=1, cpu_work_ms=1, present_ms=15.67,
                   navigation_ms=.1, learning_ms=.1, save_ms=0, save_calls=0,
                   elapsed_ms=i*500, phase="measurement", tick=i*10) for i in (1, 2)])
        self.write("gpu.csv", [dict(frame=i, status="valid", gpu_ms=.5) for i in (1, 2)])
        self.write("progress.csv", [dict(elapsed_ms=1, tick=0, station=0, completed=0, stock=100, wear=0),
                   dict(elapsed_ms=1000, tick=20, station=0, completed=1, stock=99, wear=0)])
        self.write("actors.csv", [dict(elapsed_ms=1, tick=0, actor=1, autonomous=1, hunger=100, energy=1000),
                                  dict(elapsed_ms=1000, tick=20, actor=1, autonomous=1, hunger=10, energy=900)])
        report = dict(normal_shutdown=1, measurement_finished=1, simulation_errors=0,
                      measurement_interventions=0, measurement_minimizations=0, measurement_resizes=0,
                      occluded_frames=0, frame_samples=2, measured_ticks=20, measured_wall_ms=1000,
                      profile_gpu_pending=0, profile_gpu_skipped=0, profile_gpu_resolved=2,
                      profile_save_calls=2, viewport="846x781", adapter="NVIDIA GeForce MX330")
        (self.root / "run.log").write_text("\n".join(f"{k}={v}" for k,v in report.items())+
            "\npractice_actor=4 probes=1 records=1 method=1 reads=100 primitives=200 stage=1\n")
        (self.root / "trace/status.txt").write_text("measurement_trace_complete=1\nmeasurement_trace_exported=1\n")
        (self.root / "world.save.final.save").write_text("test fixture only")
        (self.root / "world.save").write_bytes(b"initial fixture")
        self.protocol["initial_world_sha256"] = m06.digest(self.root / "world.save")
        (self.root / "process.json").write_text(json.dumps(dict(exit_code=0, initial_world_unchanged=True)))
        (self.root / "protocol.json").write_text(json.dumps(self.protocol))

    def write(self, name, rows):
        with (self.root / "trace" / name).open("w", newline="") as stream:
            writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
            writer.writeheader()
            writer.writerows(rows)

    def test_accepts_complete_active_trial(self):
        self.assertTrue(m06.evaluate(self.root, self.protocol)["passed"])

    def test_rejects_hidden_stagnation(self):
        path = self.root / "trace/progress.csv"
        path.write_text(path.read_text().replace("1000,20,0,1,99", "1000,20,0,0,100"))
        self.assertFalse(m06.evaluate(self.root, self.protocol)["passed"])

    def test_rejects_incomplete_capture(self):
        (self.root / "trace/status.txt").write_text("measurement_trace_complete=0\n")
        self.assertFalse(m06.evaluate(self.root, self.protocol)["passed"])

    def test_preserves_bad_frame(self):
        path = self.root / "trace/samples.csv"
        path.write_text(path.read_text().replace("16.67", "300", 1))
        self.assertFalse(m06.evaluate(self.root, self.protocol)["passed"])

    def test_rejects_missing_gpu(self):
        path = self.root / "trace/gpu.csv"
        path.write_text("frame,status,gpu_ms\n")
        self.assertFalse(m06.evaluate(self.root, self.protocol)["passed"])

    def test_rejects_nonfinite_timing(self):
        path = self.root / "trace/samples.csv"
        path.write_text(path.read_text().replace("16.67", "nan", 1))
        self.assertFalse(m06.evaluate(self.root, self.protocol)["passed"])

    def test_rejects_failed_process_in_reanalysis(self):
        (self.root / "process.json").write_text(json.dumps(dict(exit_code=1, initial_world_unchanged=True)))
        self.assertFalse(m06.evaluate(self.root, self.protocol)["passed"])

    def test_rejects_prolonged_extreme_need(self):
        self.write("actors.csv", [dict(elapsed_ms=1, tick=0, actor=1, autonomous=1, hunger=1000, energy=1000),
                                  dict(elapsed_ms=1000, tick=20, actor=1, autonomous=1, hunger=1000, energy=1000)])
        self.assertFalse(m06.evaluate(self.root, self.protocol)["passed"])

    def test_rejects_missing_process(self):
        (self.root / "process.json").unlink()
        self.assertFalse(m06.evaluate(self.root, self.protocol)["passed"])

    def test_rejects_changed_initial_world(self):
        (self.root / "world.save").write_bytes(b"wrong world")
        self.assertFalse(m06.evaluate(self.root, self.protocol)["passed"])

    def test_rejects_truncated_actor_history(self):
        self.write("actors.csv", [dict(elapsed_ms=1, tick=0, actor=1, autonomous=1, hunger=1000, energy=0)])
        self.assertFalse(m06.evaluate(self.root, self.protocol)["passed"])

    def test_rejects_duplicate_station_progress(self):
        rows = [dict(elapsed_ms=1, tick=0, station=0, completed=1, stock=99, wear=0),
                dict(elapsed_ms=1000, tick=20, station=0, completed=1, stock=99, wear=0)]
        self.write("progress.csv", rows + [rows[-1]])
        self.assertFalse(m06.evaluate(self.root, self.protocol)["passed"])

    def test_rejects_changed_protocol(self):
        changed = json.loads(json.dumps(self.protocol))
        changed["gates"]["frame_p95_ms"] = 10000
        self.assertFalse(m06.evaluate(self.root, changed)["passed"])
