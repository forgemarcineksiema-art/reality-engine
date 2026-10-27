"""Protocol checks use development seeds only; a negative hypothesis is a valid result."""
import json
from pathlib import Path
import sys
import tempfile
import subprocess
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import run_discovery_experiment as experiment


class DiscoveryExperimentTests(unittest.TestCase):
    def test_counter_domains_do_not_reuse_inputs(self):
        a = experiment.RandomDomain(0xD15C0000, 1)
        b = experiment.RandomDomain(0xD15C0003, 2)
        self.assertNotEqual([a.draw() for _ in range(8)], [b.draw() for _ in range(8)])
        again = experiment.RandomDomain(0xD15C0000, 1)
        self.assertEqual(again.draw(), experiment.RandomDomain(0xD15C0000, 1).draw())
        again.counter = 1 << 24
        with self.assertRaises(ValueError):
            again.draw()

    def test_generator_and_independent_hand_calculations(self):
        self.assertEqual(experiment.apply_rule([0, 3], 17), 3)
        self.assertEqual(experiment.apply_rule([1, 24], 16), 0)
        rules = [[1, 4], [2, 20], [0, 8]]
        self.assertEqual(experiment.output(rules, [1, 0], 64), 16)
        self.assertEqual(experiment.output(rules, [0, 1], 64), 20)
        self.assertEqual(experiment.output(rules, [2, 2], 80), 20)
        case = experiment.make_case(0xD15C0000)
        self.assertEqual([len(case[k]) for k in ("predictions", "tasks_a", "tasks_b")], [24, 20, 20])
        for key in ("predictions", "tasks_a", "tasks_b"):
            for query in case[key]:
                self.assertIn(query["input"], [48, 80, 112])
                self.assertIn(len(query["recipe"]), [2, 3])
                self.assertEqual(query["target"], experiment.output(case["rules"], query["recipe"], query["input"]))
        self.assertEqual(case, experiment.make_case(0xD15C0000))

    def test_sign_test_keeps_ties_out_of_n_not_out_of_mean(self):
        self.assertEqual(experiment.sign_test([0, 0]), 1)
        self.assertEqual(experiment.sign_test([1, 1, 1, 0]), 0.125)
        self.assertEqual(experiment.sign_test([-1, -1]), 1)
        self.assertEqual(experiment.sign_test([1, -1]), 0.75)

    def test_negative_result_is_preserved(self):
        rows = [{"seed": n, "d_before": 0.9, "d_after": 0.9, "g": 0.8,
                 "g_random": 0.8, "s_with": 0.5, "s_without": 0.8} for n in range(64)]
        result = experiment.analyze_rows(rows)
        self.assertFalse(result["supported"])
        self.assertFalse(result["S"]["supported"])
        self.assertAlmostEqual(result["S"]["gain"], -0.3)
        self.assertEqual(result["S"]["p"], 1)
        with self.assertRaises(ValueError):
            experiment.analyze_rows(rows + rows[:1])

    def test_freeze_rejects_changed_artifact(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            target = root / "binary.exe"
            target.write_bytes(b"initial")
            record = {str(target): experiment.sha256(target)}
            experiment.verify_hashes(record)
            target.write_bytes(b"changed")
            with self.assertRaises(ValueError):
                experiment.verify_hashes(record)

    def test_missing_or_incomplete_rows_do_not_become_success(self):
        with self.assertRaises(ValueError):
            experiment.analyze_rows([])
        with self.assertRaises(ValueError):
            experiment.analyze_rows([{"seed": 1}])
        with self.assertRaises(ValueError):
            experiment.analyze_rows([{"seed": 1, "d_before": float("nan")}])

    def test_runner_evidence_requires_raw_trials(self):
        case = experiment.make_case(0xD15C0000)
        with self.assertRaises(ValueError):
            experiment.validate_row({"seed": case["seed"], "integrity": True}, case)

    def test_exact_threshold_and_one_attainable_step_below(self):
        rows = [{"seed": n, "d_before": 0.5, "d_after": 1.0, "g": 1.0,
                 "g_random": 0.8, "s_with": 1.0, "s_without": 0.8} for n in range(64)]
        result = experiment.analyze_rows(rows)
        self.assertTrue(result["G"]["supported"])
        self.assertTrue(result["S"]["supported"])
        rows[0]["s_with"] = 19 / 20
        self.assertFalse(experiment.analyze_rows(rows)["S"]["supported"])

    def test_analyze_command_rejects_unfrozen_headers(self):
        rows = [{"seed": n, "integrity": False, "d_before": 0, "d_after": 1,
                 "g": 1, "g_random": 0, "s_with": 1, "s_without": 0} for n in range(64)]
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "rows.json").write_text(json.dumps(rows), encoding="utf-8")
            result = subprocess.run([sys.executable, str(Path(experiment.__file__)), "analyze", str(root)], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
