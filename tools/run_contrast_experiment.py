"""Freeze, run once, and independently judge discrimination-v1 (stdlib)."""
import sys
sys.dont_write_bytecode = True
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import contrast_judge as j
import hypothesis_model as h
from run_hypothesis_experiment import native_provenance
import audit_apparatus
ROOT = Path(__file__).resolve().parents[1]
PROTOCOL = 'discrimination-v1'


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8'))


def write(path, value):
    with path.open('x', encoding='utf-8') as out:
        json.dump(value, out, indent=2)
        out.write('\n')


def source_files(root=ROOT):
    fixed = ('CMakeLists.txt', 'CMakePresets.json', '.clang-format', 'README.md',
             'engineering-rules.md', 'AGENTS.md', 'docs/work-items/discriminating-experiment.md',
             'docs/work-items/discriminating-implementation.md')
    result = [root/p for p in fixed if (root/p).is_file()]
    for folder in ('src', 'tests', 'tools'):
        result.extend(p for p in (root/folder).rglob('*') if p.is_file() and
                      '__pycache__' not in p.parts and p.suffix in ('.cpp', '.hpp', '.py', '.json', '.save'))
    return sorted(result)


def hashes(root, paths):
    return {p.relative_to(root).as_posix(): digest(p) for p in paths}


def verify_files(root, manifest, exact=False):
    for name, sha in manifest.items():
        path = (root/name).resolve()
        j.check(path.is_relative_to(root.resolve()) and path.is_file() and digest(path) == sha,
                'changed or outside package: '+name)
    if exact:
        j.check(set(manifest) == {p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()},
                'unmanifested package files')


def validate_review(record, sources):
    j.check(isinstance(record, dict) and record.get('protocol') == PROTOCOL and record.get('status') == 'READY'
            and re.fullmatch('[0-9a-f]{40}', record.get('reviewed_head', ''))
            and record.get('verified_sources') == sources, 'review must be READY and bound to all sources')


def validate_native_identity(identity, sources, provenance):
    major,minor,patch,*_ = map(int,provenance['compiler_version'].split('.'))
    j.check(identity['sources']==sources and identity['configuration']==provenance['configuration']
            and identity['pointer_bits']==64 and identity['asan']==int(provenance['address_sanitizer']=='ON')
            and identity['msvc_full_ver']==(major*100+minor)*100000+patch,
            'binary identity differs from sources, compiler or build configuration')


def generate(seed, excluded=(), divisor=0):
    rng = h.RandomDomain(seed, 60)
    rejected = []
    while True:
        program = tuple(h.OPS[rng.draw()%27] for _ in range(2+rng.draw()%2)) if not divisor else ()
        values = tuple(x*x//divisor for x in range(129)) if divisor else h.table(program)
        reason = None
        if not divisor:
            if values in h.flat_models(): reason = 'flat'
            elif values in excluded: reason = 'development-table'
            elif values == h.table(((2, 8), (0, 8))): reason = 'didactic-example'
        pool = sorted({(x, h.output(values, c, x)) for x in range(1, 129)
                       for c in h.CHAINS if h.output(values, c, x)>0}) if reason is None else []
        if reason is None and pool: break
        rejected.append(dict(program=program, reason=reason or 'no-positive-goal'))
        if len(rejected) >= 100000: raise ValueError('world rejection limit')
    tasks = h.RandomDomain(seed, 62)
    goals = [pool[tasks.draw()%len(pool)] for _ in range(20)]
    return dict(seed=seed, program=program, divisor=divisor, rejections=rejected,
                counters={'60':rng.counter, '62':tasks.counter}, goals=goals,
                pool_size=len(pool), distinct_goals=len(set(goals)))


def case_text(case):
    return (f"{case['seed']} {case['divisor']} {len(case['program'])}\n" +
            ''.join(f'{k} {v}\n' for k,v in case['program']) +
            ''.join(f'{x} {y}\n' for x,y in case['goals']))


def verify(directory):
    verify_files(directory, read(directory/'manifest.json'))
    freeze = read(directory/'freeze.json')
    j.check(freeze['protocol'] == PROTOCOL and freeze['mode'] in ('development','confirmatory'), 'invalid protocol or mode')
    count, base = (8,0x44560000) if freeze['mode']=='development' else (64,0x44561000)
    j.check(freeze['seeds'] == list(range(base,base+count))+list(range(0x44562000,0x44562010)), 'predeclared seeds changed')
    verify_files(directory/'frozen-source', freeze['sources'], exact=True)
    for module in (__file__,j.__file__,h.__file__,sys.modules['run_discovery_experiment'].__file__,
                   sys.modules['run_hypothesis_experiment'].__file__,audit_apparatus.__file__):
        j.check(digest(module)==digest(directory/'frozen-source/tools'/Path(module).name),
                'analysis dependency differs from frozen code: '+Path(module).name)
    cases = read(directory/'cases.json')
    j.check([c['seed'] for c in cases]==freeze['seeds'], 'case set changed')
    excluded = {tuple(t) for t in freeze['excluded_tables']}
    count = 8 if freeze['mode']=='development' else 64
    for i,c in enumerate(cases):
        divisor = 0 if i<count else (128,160,192,256)[(i-count)//4]
        expected = json.loads(json.dumps(generate(c['seed'],excluded,divisor)))
        j.check(c==expected, 'case generator replay differs')
        j.check((directory/f"{c['seed']:08x}.case").read_text()==case_text(c), 'native case differs')
    return cases


def prepare(args):
    sources = hashes(ROOT,source_files())
    excluded = set()
    development = None
    if args.mode=='confirmatory':
        j.check(args.review is not None and args.development is not None,'review and development required')
        validate_review(read(args.review),sources)
        j.check(not subprocess.check_output(['git','status','--porcelain'],cwd=ROOT,text=True).strip(),
                'confirmation requires clean committed sources')
        development = args.development.resolve()
        analyze(development,emit=False)
        old = read(development/'freeze.json')
        j.check(old['mode']=='development' and old['sources']==sources and old['exe_sha']==digest(args.exe),
                'development must verify this exact source and binary')
        excluded = {j.truth(c) for c in read(development/'cases.json')[:8]}
    provenance = native_provenance(args.exe)
    identity = json.loads(subprocess.check_output([str(args.exe.resolve()),'--metadata'],text=True))
    validate_native_identity(identity,sources,provenance)
    j.check(provenance['configuration']=='RelWithDebInfo' and provenance['address_sanitizer']=='OFF',
            'experiment needs optimized non-ASan binary')
    count,base = (8,0x44560000) if args.mode=='development' else (64,0x44561000)
    seeds = list(range(base,base+count))+list(range(0x44562000,0x44562010))
    output = args.directory
    output.mkdir()
    for source in source_files():
        target = output/'frozen-source'/source.relative_to(ROOT)
        target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(source,target)
    shutil.copyfile(args.exe,output/'reality_contrast.exe')
    write(output/'native-identity.json',identity)
    build_root=args.exe.resolve().parent.parent
    shutil.copyfile(build_root/'CMakeCache.txt',output/'CMakeCache.txt')
    version=provenance['cmake_version']
    shutil.copyfile(build_root/f'CMakeFiles/{version}/CMakeCXXCompiler.cmake',output/'CMakeCXXCompiler.cmake')
    write(output/'apparatus-audit.json',audit_apparatus.run())
    if args.review: shutil.copyfile(args.review,output/'review-record.json')
    # Freeze precedes generation of all new truths and task plans.
    write(output/'freeze.json',dict(protocol=PROTOCOL,mode=args.mode,utc=datetime.now(timezone.utc).isoformat(),
          seeds=seeds,commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
          sources=sources,exe_sha=digest(args.exe),native=provenance,excluded_tables=sorted(excluded),
          development_manifest=digest(development/'manifest.json') if development else None))
    cases = [generate(seed,excluded) for seed in seeds[:count]]
    cases += [generate(seed,divisor=(128,160,192,256)[i//4]) for i,seed in enumerate(seeds[count:])]
    write(output/'cases.json',cases)
    for c in cases: (output/f"{c['seed']:08x}.case").write_text(case_text(c),encoding='utf-8')
    write(output/'manifest.json',hashes(output,[p for p in output.rglob('*') if p.is_file()]))


def claim_run(directory):
    write(directory/'started.json',dict(utc=datetime.now(timezone.utc).isoformat(),
          manifest_sha=digest(directory/'manifest.json'),freeze_sha=digest(directory/'freeze.json')))


def run(directory):
    cases = verify(directory)
    j.check(json.loads(subprocess.check_output([str((directory/'reality_contrast.exe').resolve()),'--metadata'],text=True))
            == read(directory/'native-identity.json'),'copied binary identity changed')
    claim_run(directory)
    raw = directory/'raw'
    raw.mkdir()
    for i,c in enumerate(cases):
        stem = f"{c['seed']:08x}"
        completed = subprocess.run([str((directory/'reality_contrast.exe').resolve()),
                     str((directory/(stem+'.case')).resolve()),str((raw/(stem+'.json')).resolve())],
                     capture_output=True,text=True,timeout=60)
        (raw/(stem+'.log')).write_text(completed.stdout+completed.stderr,encoding='utf-8')
        if completed.returncode: raise ValueError(f'native failure {stem}: {completed.stderr}')
        if (i+1)%8==0: print(f'completed {i+1}/{len(cases)}',flush=True)
    write(directory/'raw-manifest.json',hashes(raw,[p for p in raw.iterdir() if p.is_file()]))


def analyze(directory,emit=True):
    cases = verify(directory)
    started = read(directory/'started.json')
    j.check(started['manifest_sha']==digest(directory/'manifest.json') and
            started['freeze_sha']==digest(directory/'freeze.json'),'run marker differs from freeze')
    manifest = read(directory/'raw-manifest.json')
    verify_files(directory/'raw',manifest,exact=True)
    j.check(set(manifest)=={f"{c['seed']:08x}.{ext}" for c in cases for ext in ('json','log')},'raw case set differs')
    rows = []
    for i,c in enumerate(cases):
        rows.append(j.judge(c,read(directory/'raw'/f"{c['seed']:08x}.json")))
        if emit and (i+1)%8==0: print(f'judged {i+1}/{len(cases)}',file=sys.stderr,flush=True)
    count = 8 if read(directory/'freeze.json')['mode']=='development' else 64
    result = dict(protocol=PROTOCOL,main=j.summarize(rows[:count]),outside=j.summarize(rows[count:]),
                  distinct_main_laws=len({j.truth(c) for c in cases[:count]}),rows=rows)
    if (directory/'analysis.json').exists(): j.check(read(directory/'analysis.json')==result,'saved analysis differs')
    else: write(directory/'analysis.json',result)
    if emit: print(json.dumps({k:v for k,v in result.items() if k!='rows'},indent=2))
    return result


if __name__=='__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('command',choices=('prepare','run','analyze'))
    parser.add_argument('directory',type=Path)
    parser.add_argument('--mode',choices=('development','confirmatory'),default='development')
    parser.add_argument('--exe',type=Path)
    parser.add_argument('--review',type=Path)
    parser.add_argument('--development',type=Path)
    args = parser.parse_args()
    if args.command=='prepare': prepare(args)
    elif args.command=='run': run(args.directory)
    else: analyze(args.directory)
