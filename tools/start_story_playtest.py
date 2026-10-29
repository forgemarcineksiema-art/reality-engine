"""Open the shared story course with explicit preparation variants and private evidence."""
import argparse
import datetime
from pathlib import Path
import subprocess
import uuid

def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', type=Path, default=root / 'build/msvc/RelWithDebInfo/reality_traversal.exe')
    parser.add_argument('--case', choices=('normal', 'missing-person', 'missing-place', 'moved-supplies'), default='normal')
    parser.add_argument('--smoke-test', action='store_true')
    args = parser.parse_args()
    exe = args.exe.resolve()
    if not exe.is_file():
        parser.error('Build first: cmake --build --preset windows-release')
    stamp = datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
    output = root / 'build' / ('story-playtest-' + stamp + '-' + uuid.uuid4().hex[:8])
    output.mkdir(parents=True, exist_ok=False)
    command = [str(exe), '--story', '--story-case', args.case, '--save', str(output / 'world.save'), '--report', str(output / 'session.log'), '--capture', str(output / 'scene.png'), '--diagnostics', str(output / 'diagnostics')]
    if args.smoke_test:
        command += ['--smoke-test', '--gpu-debug']
    print('Evidence:', output, flush=True)
    print('T: request story. F2: autonomy. Space/N: time. F4: cargo. F5/F9: save/load.', flush=True)
    raise SystemExit(subprocess.run(command, check=False, timeout=18 if args.smoke_test else None).returncode)
if __name__ == '__main__':
    main()
