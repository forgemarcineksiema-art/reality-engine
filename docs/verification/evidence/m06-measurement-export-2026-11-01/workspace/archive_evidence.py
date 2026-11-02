"""Freeze this task's evidence; run from repository root after final checks."""
import argparse
import hashlib
import json
import pathlib
import shutil
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--copy-only", action="store_true")
args = parser.parse_args()
root = pathlib.Path.cwd().resolve()
work = root / "build/m06-measurement-export-workspace"
destination = root / "docs/verification/evidence/m06-measurement-export-2026-11-01"
assert (root / "engineering-rules.md").is_file()
assert work.resolve().is_relative_to(root / "build")
assert destination.resolve().is_relative_to(root / "docs/verification/evidence")
if args.copy_only:
    assert not destination.exists(), "refusing to replace existing evidence"
    shutil.copytree(work, destination / "workspace")
    for original in work.rglob("*"):
        if original.is_file():
            copied = destination / "workspace" / original.relative_to(work)
            assert original.read_bytes() == copied.read_bytes(), str(original)
    print("PASS exact-byte evidence copy")
else:
    assert (destination / "workspace/progress.md").is_file()
    assert not (destination / "manifest.json").exists()
    counts = {"debug": 431, "release": 431, "asan": 433}
    for profile, count in counts.items():
        log = (destination / f"workspace/ctest-{profile}.log").read_text(encoding="utf-8")
        assert f"100% tests passed out of {count}" in log
        assert "***Failed" not in log and "***Timeout" not in log
    assert "PASS: documentation" in (destination / "final-docs.log").read_text(encoding="utf-8")
    python_log = (destination / "final-python.log").read_text(encoding="utf-8")
    assert "Ran 58 tests" in python_log and "\nOK" in python_log
    assert "fresh_detour" in (destination / "workspace/ctest-asan-timeout.log").read_text(encoding="utf-8")
    source_paths = ["CMakeLists.txt", "CMakePresets.json", ".clang-format",
                    "src/core/runtime_metrics.hpp", "src/core/runtime_metrics.cpp",
                    "src/platform/traversal_main.cpp", "tests/traversal_diagnostics_tests.cpp",
                    "tools/test_traversal_diagnostics.py", "src/research/contrast_agent.cpp",
                    "tests/contrast_tests.cpp", "tests/two_methods_tests.cpp",
                    "tests/two_methods_integration.py"]
    subprocess.run(["git", "diff", "--quiet", "HEAD", "--", *source_paths], check=True)

    def entry(path):
        data = path.read_bytes()
        return {"path": path.relative_to(root).as_posix(), "size_bytes": len(data),
                "sha256": hashlib.sha256(data).hexdigest()}

    code_head = subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()
    manifest = {
        "schema_version": 1, "started_client_date": "2026-11-01", "client_date": "2026-11-02", "timezone": "Europe/Warsaw",
        "code_head": code_head, "reviewed_head": "f2493a32abc86dda27a7798ebdce98e6c04e6031",
        "base": "3bf2d60f3c9183d1b7d74eefabf3f07fd3e42ae6",
        "scope": "Bounded measurement export and semantically identical mind initialization; no long benchmark or human acceptance.",
        "final_ctest_passed": counts, "final_python_tests_passed": 58,
        "retained_timing_failure": {
            "test": "two_methods.fresh_detour", "full_run_passed": 431, "total": 432,
            "exit_code": 8, "checkpoint_limit_seconds": 8, "cause": "unproven",
            "log": "workspace/ctest-asan-timeout.log",
            "strict_isolated_retry": "PASS, checkpoint expected_saved=5.52182 seconds",
            "isolated_log": "workspace/asan-detour-isolated.log",
            "second_run": "workspace/ctest-asan-second-timeout-aborted.log",
            "third_run": "workspace/ctest-asan-third-timeout.log",
            "optimized_initial_subset_failure": "workspace/contrast-init-optimized-tests.log",
            "note": "Diagnostic 30-second probes are not acceptance. Final 8/20-second gates unchanged. Timing variability remains open."
        },
        "commands": [
            {"command": f"cmake --build --preset windows-{profile}", "exit_code": 0}
            for profile in counts
        ] + [
            {"command": f"ctest --preset windows-{profile}" + (" --stop-on-failure" if profile == "asan" else ""), "exit_code": 0, "passed": count}
            for profile, count in counts.items()
        ] + [{"command": "python tools/check_docs.py", "exit_code": 0},
             {"command": "python -m unittest discover -s tools/tests -p test_*.py", "exit_code": 0},
             {"command": "cmake --build --preset windows-debug --target format-check", "exit_code": 0}],
        "source_working_bytes": [entry(root / path) for path in source_paths],
        "binaries": [entry(root / path) for path in (
            "build/msvc/Debug/reality_traversal.exe",
            "build/msvc/RelWithDebInfo/reality_traversal.exe",
            "build/asan/RelWithDebInfo/reality_traversal.exe",
            "build/asan/RelWithDebInfo/reality_two_methods_tests.exe")],
        "archived_files": [entry(path) for path in sorted(destination.rglob("*")) if path.is_file()],
    }
    (destination / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
                                                encoding="utf-8")
    for record in manifest["archived_files"] + manifest["source_working_bytes"] + manifest["binaries"]:
        assert entry(root / record["path"]) == record
    print(f"PASS manifest and {len(manifest['archived_files'])} archived file hashes; code {code_head}")
