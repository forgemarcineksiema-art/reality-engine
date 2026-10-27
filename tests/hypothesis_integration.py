"""Continue real hypothesis snapshots in fresh native processes."""
import argparse
from pathlib import Path
import subprocess
import tempfile
import sys
import json


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', required=True)
    parser.add_argument('--mode', choices=('case', 'continuation'), default='continuation')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='hypothesis-') as directory:
        root = Path(directory)
        if args.mode == 'case':
            sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
            import run_hypothesis_experiment as experiment
            case = experiment.make_case(0x48590000, set())
            experiment.write_case(root / 'case.txt', case)
            subprocess.run([args.exe, '--case', str(root / 'case.txt'), '--output', str(root / 'raw')], check=True)
            raw = json.loads((root / 'raw/raw.json').read_text(encoding='utf-8'))
            result = experiment.judge(case, raw)
            assert set(result) == {'FA', 'SR', 'SA'}
            raw['variants']['SA']['steps'][0]['measurement']['output'] += 1
            try:
                experiment.judge(case, raw)
            except ValueError:
                print('PASS native declarations, interpreter, budgets and tamper rejection')
                return
            raise AssertionError('changed measurement accepted')
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
