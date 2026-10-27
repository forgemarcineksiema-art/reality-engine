"""Check both process outcome and diagnostic marker, never failure alone."""

from __future__ import annotations

import argparse
import subprocess
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--expected", choices=("success", "failure"), required=True)
    parser.add_argument("--contains", required=True)
    parser.add_argument("--timeout", type=float, default=10.0)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command or not args.contains or args.timeout <= 0:
        parser.error("a command, nonempty marker and positive timeout are required")
    try:
        completed = subprocess.run(
            command,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=args.timeout,
            check=False,
        )
    except (OSError, subprocess.TimeoutExpired) as error:
        print(f"FAIL: process could not complete: {error}", file=sys.stderr)
        return 1

    output = completed.stdout + completed.stderr
    print(output, end="" if output.endswith("\n") else "\n")
    correct_status = (
        completed.returncode == 0
        if args.expected == "success"
        else completed.returncode != 0
    )
    if not correct_status or args.contains not in output:
        print(
            f"FAIL: expected {args.expected} with marker {args.contains!r}; "
            f"exit={completed.returncode}",
            file=sys.stderr,
        )
        return 1
    print(f"VERIFIED: expected {args.expected}; exit={completed.returncode}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
