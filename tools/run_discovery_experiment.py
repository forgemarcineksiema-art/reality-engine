"""Prepare/freeze, run once, and analyze discovery-v1 using the standard library.

Hypothesis failure is a valid result (exit 0); missing/invalid evidence is an error.
The confirmatory set must only be prepared after technical verification and review.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import platform
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
PROTOCOL = ROOT / "docs/work-items/discovery-learning-experiment.md"
MASK = (1 << 64) - 1


class RandomDomain:
    def __init__(self, seed: int, tag: int):
        if not 0 <= seed < 1 << 32 or not 0 <= tag < 256:
            raise ValueError("invalid random domain")
        self.seed, self.tag, self.counter = seed, tag, 0

    def draw(self) -> int:
        if self.counter >= 1 << 24:
            raise ValueError("random domain exhausted")
        key = self.seed << 32 | self.tag << 24 | self.counter
        self.counter += 1
        z = (key + 0x9E3779B97F4A7C15) & MASK
        z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & MASK
        z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & MASK
        return z ^ (z >> 31)


def apply_rule(rule: list[int], energy: int) -> int:
    family, parameter = rule
    if family == 0:
        return energy * parameter // 16
    if family == 1:
        return max(0, energy - parameter)
    if family == 2:
        return min(energy, parameter)
    raise ValueError("unknown rule")


def output(rules: list[list[int]], chain: list[int], energy: int) -> int:
    for symbol in chain:
        energy = apply_rule(rules[symbol], energy)
    return energy


def make_case(seed: int) -> dict:
    rng = RandomDomain(seed, 1)
    rules = []
    for _ in range(3):
        family = rng.draw() % 3
        lo, count = [(3, 10), (1, 24), (8, 41)][family]
        rules.append([family, lo + rng.draw() % count])
    result = {"seed": seed, "rules": rules}
    for tag, key, count in [(2, "predictions", 24), (3, "tasks_a", 20), (4, "tasks_b", 20)]:
        rng = RandomDomain(seed, tag)
        queries = []
        for _ in range(count):
            energy = [48, 80, 112][rng.draw() % 3]
            length = 2 + rng.draw() % 2
            recipe = [rng.draw() % 3 for _ in range(length)]
            queries.append({"input": energy, "recipe": recipe, "target": output(rules, recipe, energy)})
        result[key] = queries
    return result


def case_text(case: dict) -> str:
    lines = [f"DISCOVERY_CASE 1 {case['seed']}"]
    lines.append(" ".join(str(value) for rule in case["rules"] for value in rule))
    for key in ("predictions", "tasks_a", "tasks_b"):
        lines.append(f"{key} {len(case[key])}")
        for query in case[key]:
            recipe = query["recipe"]
            lines.append(" ".join(map(str, [query["input"], query["target"], len(recipe), *recipe])))
    return "\n".join(lines) + "\n"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def verify_hashes(records: dict[str, str]) -> None:
    for name, expected in records.items():
        path = Path(name)
        if not path.is_file() or sha256(path) != expected:
            raise ValueError(f"frozen artifact changed or missing: {name}")


def write_json(path: Path, value: dict | list) -> None:
    path.write_text(json.dumps(value, ensure_ascii=False, sort_keys=True, indent=2, allow_nan=False) + "\n", encoding="utf-8")


def source_files() -> list[Path]:
    files = [ROOT / name for name in ("CMakeLists.txt", "CMakePresets.json", ".clang-format")]
    files.append(PROTOCOL)
    for folder in ("src", "tests", "tools"):
        files.extend(p for p in (ROOT / folder).rglob("*") if p.suffix in (".cpp", ".hpp", ".py") and "__pycache__" not in p.parts)
    return sorted(files)


def prepare(directory: Path, executable: Path, mode: str, review: Path | None) -> None:
    executable = executable.resolve(strict=True)
    if mode == "confirmatory" and (review is None or not review.is_file()):
        raise ValueError("confirmatory preparation requires the completed technical review file")
    directory.mkdir(parents=True, exist_ok=False)
    cases_dir = directory / "cases"
    cases_dir.mkdir()
    start, count = (0xD15C0000, 16) if mode == "development" else (0xD15C1000, 64)
    cases = [make_case(start + n) for n in range(count)]
    for case in cases:
        (cases_dir / f"{case['seed']:08x}.case").write_text(case_text(case), encoding="utf-8", newline="\n")
    write_json(directory / "manifest.json", {"protocol": "discovery-v1", "mode": mode, "cases": cases})
    frozen = directory / "frozen"
    frozen.mkdir()
    binary = frozen / "reality_discovery.exe"
    shutil.copyfile(executable, binary)
    # Keep the byte-exact protocol alongside the hash; .txt avoids a second Markdown owner.
    shutil.copyfile(PROTOCOL, frozen / "protocol.txt")
    source_hashes = {str(path.relative_to(ROOT)): sha256(path) for path in source_files()}
    artifacts = [directory / "manifest.json", frozen / "protocol.txt", binary, *sorted(cases_dir.glob("*.case"))]
    record = {
        "protocol": "discovery-v1", "mode": mode, "count": count,
        "created_unix": time.time(), "python": sys.version, "platform": platform.platform(),
        "git_head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "sources": source_hashes, "artifacts": {str(p.resolve()): sha256(p) for p in artifacts},
        "executable": str(binary.resolve()), "case_timeout_seconds": 60,
        "criteria": {"D_min": 0.90, "G_min": 0.80, "S_min": 0.80, "gain_min": 0.20, "p_max": 0.05 / 3},
        "review": {str(review.resolve()): sha256(review)} if review else {},
    }
    write_json(directory / "freeze.json", record)
    print(f"PREPARED {mode}: {count} cases; no agent runs performed", flush=True)


def sign_test(differences: list[float]) -> float:
    nonzero = [x for x in differences if x != 0]
    n = len(nonzero)
    if n == 0:
        return 1.0
    wins = sum(x > 0 for x in nonzero)
    return sum(math.comb(n, k) for k in range(wins, n + 1)) / 2**n


def validate_row(row: dict, case: dict) -> None:
    """Independent judge checks physical outcomes and recomputes every headline metric."""
    try:
        if row["seed"] != case["seed"] or row["integrity"] is not True:
            raise ValueError("wrong case/integrity")
        fields = {"D": ("predictions", "d_after"), "G": ("tasks_a", "g"),
                  "G_random": ("tasks_a", "g_random"), "random_D": ("predictions", "random_d"),
                  "random_G": ("tasks_a", "random_g"), "S_with": ("tasks_b", "s_with"),
                  "S_without": ("tasks_b", "s_without")}
        for label, (query_key, metric) in fields.items():
            trials, queries = row["trials"][label], case[query_key]
            if len(trials) != len(queries) or len({m["command"] for m in trials}) != len(trials):
                raise ValueError("missing/duplicate trials")
            hits = 0
            for m, query in zip(trials, queries):
                chain = m["chain"]
                if not 1 <= len(chain) <= 3 or any(x not in (0, 1, 2) for x in chain):
                    raise ValueError("invalid selected chain")
                if m["input"] != query["input"] or m["output"] != output(case["rules"], chain, m["input"]):
                    raise ValueError("physical result differs from independent judge")
                if m["observed_tick"] != m["started_tick"] + 2 or m["learning"] is not False:
                    raise ValueError("invalid evaluation phase/timing")
                if label in ("D", "random_D"):
                    if chain != query["recipe"]:
                        raise ValueError("prediction query changed")
                    hits += m["prediction"] == m["output"]
                else:
                    hits += m["output"] == query["target"]
            if not math.isclose(row[metric], hits / len(trials), abs_tol=1e-14):
                raise ValueError("headline score differs from raw trials")
            for actor, ledger in enumerate(row["energy"][label], start=1):
                own = [m for m in trials if m["source_actor"] == actor]
                spent = sum(m["input"] for m in own)
                received = sum(m["output"] for m in own)
                if ledger != {"source": 8192 - spent, "output": received, "loss": spent - received}:
                    raise ValueError("energy ledger differs from physical trials")
        before = row["preexperience_predictions"]
        predictions = row["trials"]["D"]
        if len(before) != 24 or not math.isclose(row["d_before"], sum(p == m["output"] for p, m in zip(before, predictions)) / 24, abs_tol=1e-14):
            raise ValueError("preexperience score has no forecasts")
        contact = row["contact"]
        if contact["with_ticks"] != contact["without_ticks"] or contact["with_ticks"] <= 1 or not contact["path"]:
            raise ValueError("paired contact has no physical path")
        previous = {0: (-4.0, 0.0, 0.0), 1: (4.0, 0.0, 0.0)}
        for move in contact["path"]:
            point = (move["x"], move["y"], move["z"])
            if move["actor"] not in previous or move["surface"] != 1 or math.dist(point, previous[move["actor"]]) > 0.150001:
                raise ValueError("physical path contains teleport/support change")
            previous[move["actor"]] = point
        if math.dist(previous[0], previous[1]) > 2.000001:
            raise ValueError("path did not reach contact")
        packet = contact["packet"]
        if len(packet) != contact["packet_records"] or len(packet) > 6:
            raise ValueError("packet limit mismatch")
        training = {m["command"]: m for m in row["trials"]["Training_A"]}
        for record in packet:
            actual = training.get(record["id"])
            if actual is None or record["source_actor"] != 1 or record["via_actor"] != 1 or record["learned_tick"] <= record["observed_tick"]:
                raise ValueError("packet has no original witness/age")
            if any(record[k] != actual[k] for k in ("input", "output", "chain", "observed_tick")):
                raise ValueError("packet invented a discovery measurement")
    except (KeyError, TypeError, IndexError, ZeroDivisionError) as error:
        raise ValueError("incomplete or malformed raw evidence") from error


def analyze_rows(rows: list[dict]) -> dict:
    if len(rows) not in (16, 64) or len({r.get("seed") for r in rows}) != len(rows):
        raise ValueError("expected complete, unique 16/64-world set")
    fields = ("d_before", "d_after", "g", "g_random", "s_with", "s_without")
    for row in rows:
        for name in fields:
            value = row.get(name)
            if not isinstance(value, (int, float)) or not math.isfinite(value) or not 0 <= value <= 1:
                raise ValueError(f"invalid/missing metric: {name}")
    result = {"worlds": len(rows), "unit": "one world, not one task"}
    for name, learned, baseline, minimum in [("D", "d_after", "d_before", 0.9), ("G", "g", "g_random", 0.8), ("S", "s_with", "s_without", 0.8)]:
        scores = [r[learned] for r in rows]
        controls = [r[baseline] for r in rows]
        differences = [a - b for a, b in zip(scores, controls)]
        mean = math.fsum(scores) / len(rows)
        control = math.fsum(controls) / len(rows)
        gain = math.fsum(differences) / len(rows)
        p = sign_test(differences)
        result[name] = {"mean": mean, "control": control, "gain": gain, "p": p,
                        "wins": sum(x > 0 for x in differences), "ties": differences.count(0),
                        "losses": sum(x < 0 for x in differences),
                        "supported": mean >= minimum and gain >= 0.2 and p <= 0.05 / 3}
    result["supported"] = all(result[name]["supported"] for name in ("D", "G", "S"))
    return result


def verify_freeze(directory: Path) -> dict:
    freeze = json.loads((directory / "freeze.json").read_text(encoding="utf-8"))
    if freeze["protocol"] != "discovery-v1":
        raise ValueError("unsupported protocol")
    verify_hashes({str(ROOT / name): digest for name, digest in freeze["sources"].items()})
    verify_hashes(freeze["artifacts"])
    verify_hashes(freeze["review"])
    return freeze


def run(directory: Path) -> None:
    freeze = verify_freeze(directory)
    # Exclusive marker forbids overwriting or silently restarting the frozen attempt.
    with (directory / "RUN_STARTED.json").open("x", encoding="utf-8") as marker:
        json.dump({"started_unix": time.time(), "freeze_sha256": sha256(directory / "freeze.json")}, marker)
    rows = []
    manifest = json.loads((directory / "manifest.json").read_text(encoding="utf-8"))
    for index, case in enumerate(manifest["cases"]):
        case_file = directory / "cases" / f"{case['seed']:08x}.case"
        out = directory / f"world-{case['seed']:08x}"
        started = time.perf_counter()
        try:
            process = subprocess.run([freeze["executable"], "--case", str(case_file), "--output", str(out)], capture_output=True, text=True, timeout=freeze["case_timeout_seconds"])
        except subprocess.TimeoutExpired as error:
            write_json(directory / "failure.json", {"seed": case["seed"], "type": "timeout", "seconds": time.perf_counter() - started})
            raise RuntimeError("case timed out; attempt remains preserved and incomplete") from error
        (directory / f"world-{case['seed']:08x}.log").write_text(process.stdout + process.stderr, encoding="utf-8")
        if process.returncode:
            write_json(directory / "failure.json", {"seed": case["seed"], "type": "runner", "exit_code": process.returncode})
            raise RuntimeError(f"case failed with exit {process.returncode}; no hypothesis verdict")
        row = json.loads((out / "result.json").read_text(encoding="utf-8"))
        if row.get("seed") != case["seed"] or row.get("integrity") is not True:
            raise ValueError("runner returned wrong case or no integrity evidence")
        validate_row(row, case)
        row["wall_seconds"] = time.perf_counter() - started
        rows.append(row)
        print(f"{index + 1}/{freeze['count']} seed={case['seed']:08x} D={row['d_after']:.3f} G={row['g']:.3f} S={row['s_with']:.3f}/{row['s_without']:.3f}", flush=True)
    verify_freeze(directory)
    if len(rows) != freeze["count"]:
        raise ValueError("incomplete frozen attempt")
    summary = analyze_rows(rows)
    summary["mode"] = freeze["mode"]
    summary["interpretation"] = "confirmatory" if freeze["mode"] == "confirmatory" else "development only, not confirmation"
    write_json(directory / "rows.json", rows)
    write_json(directory / "summary.json", summary)
    outputs = [*sorted(directory.rglob("*.save")), *sorted(directory.rglob("*.log")), *sorted(directory.rglob("result.json")), directory / "rows.json", directory / "summary.json"]
    write_json(directory / "result-hashes.json", {str(p.relative_to(directory)): sha256(p) for p in outputs})
    print(json.dumps(summary, ensure_ascii=False, indent=2), flush=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    prepare_parser = commands.add_parser("prepare")
    prepare_parser.add_argument("--output", type=Path, required=True)
    prepare_parser.add_argument("--exe", type=Path, required=True)
    prepare_parser.add_argument("--mode", choices=("development", "confirmatory"), required=True)
    prepare_parser.add_argument("--review", type=Path)
    for name in ("run", "analyze"):
        commands.add_parser(name).add_argument("directory", type=Path)
    args = parser.parse_args()
    try:
        if args.command == "prepare":
            prepare(args.output.resolve(), args.exe, args.mode, args.review)
        elif args.command == "run":
            run(args.directory.resolve())
        else:
            print(json.dumps(analyze_rows(json.loads((args.directory / "rows.json").read_text(encoding="utf-8"))), indent=2))
        return 0
    except (ValueError, OSError, RuntimeError, KeyError, subprocess.SubprocessError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
