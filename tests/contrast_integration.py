import argparse
from pathlib import Path
import subprocess
import tempfile

parser=argparse.ArgumentParser()
parser.add_argument('exe',type=Path)
args=parser.parse_args()
with tempfile.TemporaryDirectory() as temporary:
    root=Path(temporary)/'fixture'
    subprocess.run([args.exe,'--fixture',root],check=True,timeout=15)
    for name,ticks in [('configure',2),('pulse',1),('work',80),('contact',1)]:
        out=root/(name+'.actual')
        subprocess.run([args.exe,'--resume',root/(name+'.save'),str(ticks),out],check=True,timeout=15)
        if out.read_bytes()!=(root/(name+'.expected')).read_bytes():
            raise AssertionError('fresh-process continuation differs: '+name)
print('PASS four fresh-process continuations')
