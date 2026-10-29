"""Real Win32/D3D11 story request, consequences and save/foreign-world checks."""
import argparse
from pathlib import Path
import subprocess
import tempfile

def check(exe, case, root):
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    subprocess.run([str(exe), '--story', '--story-case', case, '--smoke-test', '--gpu-debug', '--report', str(root / 'story.log'), '--save', str(root / 'story.save'), '--capture', str(root / 'story.png')], startupinfo=startup, check=True, timeout=18)
    text = (root / 'story.log').read_text(encoding='utf-8')
    for marker in ('story_checkpoint=pass', 'story_foreign_save=pass', 'story_panel=pass', 'story_case=' + case, 'TRAVERSAL SMOKE PASS', 'hardware=1', 'gpu_debug=1', 'simulation_errors=0'):
        assert marker in text, 'missing ' + marker
    print('PASS native story', case)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--case', choices=('normal', 'missing-person', 'missing-place', 'moved-supplies'), required=True)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if args.output:
        args.output.mkdir(parents=True, exist_ok=False)
        check(args.exe.resolve(), args.case, args.output.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='genesis-story-native-') as d:
            check(args.exe.resolve(), args.case, Path(d))
if __name__ == '__main__':
    main()
