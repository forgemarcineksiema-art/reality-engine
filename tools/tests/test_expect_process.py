"""The process guard must reject unrelated failures and incomplete runs."""

from __future__ import annotations

import subprocess
import sys
import unittest
from pathlib import Path


GUARD = Path(__file__).resolve().parents[1] / "expect_process.py"


class ExpectedProcessTests(unittest.TestCase):
    def invoke(self, expected: str, code: str, timeout: float = 5.0) -> subprocess.CompletedProcess:
        return subprocess.run(
            [
                sys.executable, str(GUARD), "--expected", expected,
                "--contains", "SENTINEL", "--timeout", str(timeout),
                "--", sys.executable, "-c", code,
            ],
            capture_output=True, text=True, check=False, timeout=10,
        )

    def test_accepts_expected_failure_with_marker(self) -> None:
        result = self.invoke("failure", "print('SENTINEL'); raise SystemExit(7)")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("exit=7", result.stdout)

    def test_rejects_zero_exit_with_failure_marker(self) -> None:
        result = self.invoke("failure", "print('SENTINEL')")
        self.assertEqual(result.returncode, 1)

    def test_rejects_unrelated_failure(self) -> None:
        result = self.invoke("failure", "print('different error'); raise SystemExit(7)")
        self.assertEqual(result.returncode, 1)

    def test_rejects_timeout_even_with_marker(self) -> None:
        result = self.invoke(
            "failure", "import time; print('SENTINEL', flush=True); time.sleep(2)", 0.2
        )
        self.assertEqual(result.returncode, 1)
        self.assertIn("could not complete", result.stderr)

    def test_rejects_missing_executable(self) -> None:
        result = subprocess.run(
            [sys.executable, str(GUARD), "--expected", "failure", "--contains", "SENTINEL",
             "--", str(GUARD.parent / "missing-program.exe")],
            capture_output=True, text=True, check=False, timeout=10,
        )
        self.assertEqual(result.returncode, 1)
        self.assertIn("could not complete", result.stderr)

    def test_accepts_success_only_with_marker(self) -> None:
        passed = self.invoke("success", "print('SENTINEL')")
        missing_marker = self.invoke("success", "print('other output')")
        failed = self.invoke("success", "print('SENTINEL'); raise SystemExit(7)")
        self.assertEqual(passed.returncode, 0, passed.stdout + passed.stderr)
        self.assertEqual(missing_marker.returncode, 1)
        self.assertEqual(failed.returncode, 1)


if __name__ == "__main__":
    unittest.main()
