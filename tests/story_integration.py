"""Resume the exact same story world in a fresh process, including refusals."""
from pathlib import Path
import subprocess
import sys
import tempfile
exe = Path(sys.argv[1]).resolve()
variant = sys.argv[2]
assert variant in ('speaker', 'site', 'supply', 'travel', 'returned', 'complete', 'failed')
with tempfile.TemporaryDirectory(prefix='genesis-story-') as directory:
    root = Path(directory)
    checkpoint, expected, actual = [root / (name + '.save') for name in ('checkpoint', 'expected', 'actual')]
    subprocess.run([str(exe), 'checkpoint', variant, str(checkpoint), str(expected)], check=True, timeout=8)
    before = checkpoint.read_bytes()
    subprocess.run([str(exe), 'continue', variant, str(checkpoint), str(actual)], check=True, timeout=8)
    assert actual.read_bytes() == expected.read_bytes(), 'story continuation diverged'
    assert checkpoint.read_bytes() == before, 'checkpoint overwritten'
print('PASS fresh story', variant)
