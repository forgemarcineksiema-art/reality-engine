"""Native runner/continuation evidence; never asserts that scientific hypotheses pass."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import run_discovery_experiment as experiment


def execute(exe, *args, timeout=5, success=True):
    result = subprocess.run([str(exe), *map(str, args)], capture_output=True, text=True, timeout=timeout)
    if (result.returncode == 0) != success:
        raise AssertionError(f"unexpected exit {result.returncode}: {result.stdout} {result.stderr}")
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--mode", choices=("case", "continuation"), required=True)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="genesis-discovery-ą-") as directory:
        root = Path(directory)
        if args.mode == "continuation":
            execute(args.exe, "--fixture", "--output", root / "original")
            checkpoint = root / "original/checkpoint.save"
            digest = hashlib.sha256(checkpoint.read_bytes()).digest()
            execute(args.exe, "--continue", checkpoint, "--output", root / "resumed")
            assert (root / "resumed/final.save").read_bytes() == (root / "original/expected.save").read_bytes()
            assert hashlib.sha256(checkpoint.read_bytes()).digest() == digest
            corrupt = root / "corrupt.save"
            corrupt.write_bytes(checkpoint.read_bytes() + b"invalid")
            execute(args.exe, "--continue", corrupt, "--output", root / "rejected", success=False)
        else:
            case = experiment.make_case(0xD15C0000)
            case_file = root / "development.case"
            case_file.write_text(experiment.case_text(case), encoding="utf-8", newline="\n")
            execute(args.exe, "--case", case_file, "--output", root / "result", timeout=18)
            result = json.loads((root / "result/result.json").read_text(encoding="utf-8"))
            assert result["seed"] == case["seed"] and result["integrity"] is True
            for key in ("d_before", "d_after", "g", "g_random", "s_with", "s_without"):
                assert 0 <= result[key] <= 1
            assert len(result["trials"]["D"]) == 24
            for name in ("G", "G_random", "S_with", "S_without"):
                assert len(result["trials"][name]) == 20
            assert result["contact"]["with_ticks"] > 1
            assert result["contact"]["with_ticks"] == result["contact"]["without_ticks"]
            assert result["contact"]["packet_records"] <= 6
            assert result["teacher"]["probes"] <= 24
            assert result["student_without"]["probes"] <= 2
            assert len(result["preexperience_predictions"]) == 24
            assert result["contact"]["path"]
            assert len(result["contact"]["packet"]) == result["contact"]["packet_records"]
            experiment.validate_row(result, case)
            result["g"] = 0.123456
            try:
                experiment.validate_row(result, case)
            except ValueError:
                pass
            else:
                raise AssertionError("changed summary accepted without corresponding raw trials")
    print(f"PASS discovery native {args.mode}")


if __name__ == "__main__":
    main()
