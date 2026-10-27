"""Continue real hypothesis snapshots in fresh native processes."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', required=True)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='hypothesis-') as directory:
        root = Path(directory)
        subprocess.run([args.exe, '--fixture', '--output', str(root / 'fixture')], check=True)
        snapshots = sorted((root / 'fixture').glob('*.state'))
        assert len(snapshots) == 8, 'missing phase/controller fixtures'
        for n, snapshot in enumerate(snapshots):
            target = root / f'continue-{n}'
            subprocess.run([args.exe, '--continue', str(snapshot), '--output', str(target)], check=True)
            assert (target / 'final.state').read_bytes() == snapshot.with_suffix('.expected').read_bytes()
    print('PASS eight fresh-process continuations')


if __name__ == '__main__':
    main()
