"""Prepare immutable experiment packages, execute once, independently judge."""
import sys
sys.dont_write_bytecode = True
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
from datetime import datetime, timezone
import contrast_judge as j
import hypothesis_model as h

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, value):
    with path.open('x',encoding='utf-8') as out:
        json.dump(value,out,indent=2)
        out.write('\n')


def generate(seed, excluded=(), divisor=0):
    rng = h.RandomDomain(seed,60)
    rejected = 0
    while True:
        program = tuple(h.OPS[rng.draw()%27] for _ in range(2+rng.draw()%2)) if not divisor else ()
        values = tuple(x*x//divisor for x in range(129)) if divisor else h.table(program)
        pool = sorted({(x,h.output(values,c,x)) for x in range(1,129) for c in h.CHAINS if h.output(values,c,x)>0})
        if divisor or (values not in h.flat_models() and values not in excluded and
                       values != h.table(((2,8),(0,8))) and pool):
            break
        rejected += 1
        if rejected >= 100000:
            raise ValueError('world rejection limit')
    tasks = h.RandomDomain(seed,62)
    return dict(seed=seed,program=program,divisor=divisor,rejections=rejected,
                counters=[rng.counter,tasks.counter],goals=[pool[tasks.draw()%len(pool)] for _ in range(20)])


def verify(directory):
    manifest = json.loads((directory/'manifest.json').read_text())
    for name, sha in manifest.items():
        j.check(digest(directory/name)==sha, 'package changed: '+name)
    return json.loads((directory/'cases.json').read_text())


def prepare(args):
    output = args.directory
    output.mkdir()
    if args.mode == 'confirmatory':
        raise ValueError('confirmation disabled until final code review and full verification')
    frozen=output/'frozen-source'
    for path in subprocess.check_output(['git','ls-files'],cwd=ROOT,text=True).splitlines():
        source=ROOT/path
        if source.suffix in ('.py','.cpp','.hpp','.md','.json','.txt') and not path.startswith('docs/verification/evidence/'):
            target=frozen/path
            target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(source,target)
    # Include newly implemented files during development, before their commit.
    for path in ('tools/contrast_judge.py','tools/run_contrast_experiment.py'):
        shutil.copyfile(ROOT/path,frozen/path)
    shutil.copyfile(args.exe,output/'reality_contrast.exe')
    cases=[generate(0x44560000+i) for i in range(8)]
    cases += [generate(0x44562000+i,divisor=(128,160,192,256)[i//4]) for i in range(16)]
    write(output/'cases.json',cases)
    for case in cases:
        text = f"{case['seed']} {case['divisor']} {len(case['program'])}\n"
        text += ''.join(f'{k} {v}\n' for k,v in case['program'])
        text += ''.join(f'{x} {y}\n' for x,y in case['goals'])
        (output/f"{case['seed']:08x}.case").write_text(text,encoding='utf-8')
    write(output/'freeze.json',dict(protocol='discrimination-v1',mode=args.mode,
          utc=datetime.now(timezone.utc).isoformat(),commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
          exe_sha=digest(args.exe)))
    write(output/'manifest.json',{p.relative_to(output).as_posix():digest(p) for p in output.rglob('*') if p.is_file()})


def run(directory):
    cases=verify(directory)
    write(directory/'started.json',dict(utc=datetime.now(timezone.utc).isoformat()))
    for case in cases:
        stem=f"{case['seed']:08x}"
        completed=subprocess.run([str((directory/'reality_contrast.exe').resolve()),str(directory/(stem+'.case')),str(directory/(stem+'.json'))],capture_output=True,text=True,timeout=60)
        (directory/(stem+'.log')).write_text(completed.stdout+completed.stderr,encoding='utf-8')
        if completed.returncode:
            raise ValueError(f'native failure {stem}: {completed.stderr}')
    write(directory/'raw-manifest.json',{p.name:digest(p) for p in directory.glob('4456*.json') if p.name!='cases.json'})


def analyze(directory):
    cases=verify(directory)
    raw_manifest=json.loads((directory/'raw-manifest.json').read_text())
    for name,sha in raw_manifest.items():
        j.check(digest(directory/name)==sha,'raw data changed')
    results=[j.judge(case,json.loads((directory/f"{case['seed']:08x}.json").read_text())) for case in cases]
    n=8 if cases[0]['seed']==0x44560000 else 64
    summary=j.summarize(results[:n])
    print(json.dumps(summary,indent=2))
    return summary


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('command',choices=('prepare','run','analyze'))
    parser.add_argument('directory',type=Path)
    parser.add_argument('--mode',choices=('development','confirmatory'),default='development')
    parser.add_argument('--exe',type=Path)
    a=parser.parse_args()
    if a.command=='prepare': prepare(a)
    elif a.command=='run': run(a.directory)
    else: analyze(a.directory)
