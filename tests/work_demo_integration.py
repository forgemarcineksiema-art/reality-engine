"""Exercise the public CLI across process lifetimes, including a corrupt load."""

from pathlib import Path
import subprocess
import sys
import tempfile


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def run(executable, mode, path, success=True):
    result = subprocess.run(
        [executable, mode, str(path)], capture_output=True, text=True, timeout=10
    )
    if (result.returncode == 0) != success:
        raise AssertionError(f"{mode}: exit={result.returncode}\n{result.stdout}\n{result.stderr}")
    return result.stdout if success else result.stderr


def main():
    with tempfile.TemporaryDirectory(prefix="genesis-m01-") as folder:
        path = Path(folder) / "świat.save"
        initial = run(sys.argv[1], "--create", path)
        require("tick=2 " in initial, "initial tick differs")
        require("quantity=2 reserved=2 available=0" in initial, "initial balance differs")
        require("state=running consumed=1 due=5" in initial, "running phase differs")
        require("result=insufficient_resource" in initial, "competitor not refused")
        require(run(sys.argv[1], "--inspect", path) == initial, "inspection changed state")
        resumed = run(sys.argv[1], "--resume", path)
        require("tick=5 " in resumed, "resumed tick differs")
        require("quantity=0 reserved=0 available=0" in resumed, "completion balance differs")
        require("state=completed consumed=3 due=5" in resumed, "completion phase differs")
        require(run(sys.argv[1], "--inspect", path) == resumed, "loaded state changed")
        repeated = run(sys.argv[1], "--resume", path)
        require("tick=8 " in repeated, "repeated advance tick differs")
        require("quantity=0 reserved=0 available=0" in repeated, "repeated completion consumed again")
        # Completed work may not emit events or consume again in a new process.
        require(
            [s for s in repeated.splitlines() if s.startswith("event=")] == [
                s for s in resumed.splitlines() if s.startswith("event=")
            ],
            "completed work emitted events again",
        )
        damaged = path.read_bytes()[:40]
        path.write_bytes(damaged)
        require("ERROR:" in run(sys.argv[1], "--resume", path, success=False), "missing load error")
        require(path.read_bytes() == damaged, "failed load overwrote file")
        require("ERROR:" in run(sys.argv[1], "--unknown", path, success=False), "invalid mode accepted")
    print("PASS fresh-process save, continuation, inspection, repeated completion and corrupt load")


if __name__ == "__main__":
    main()
