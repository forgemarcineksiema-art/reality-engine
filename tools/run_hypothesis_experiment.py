"""Prepare/freeze, run once, and independently judge hypothesis-v1.

Scientific failure is exit 0. Broken infrastructure or evidence is nonzero.
"""
from fractions import Fraction
from pathlib import Path
import argparse
import hashlib
import json
import math
import platform
import re
import shutil
import subprocess
import sys
import time

sys.dont_write_bytecode = True

from hypothesis_model import (OPS, CHAINS, RandomDomain, ReplayAgent, flat_models,
                              models, output, shuffle, syntax, table)

ROOT = Path(__file__).resolve().parents[1]
PROTOCOL = ROOT / 'docs/work-items/hypothesis-building-experiment.md'
EXAMPLE = table(((2, 8), (0, 8)))


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def verify_hashes(root, hashes):
    for name, digest in hashes.items():
        path = (root / name).resolve()
        if not path.is_relative_to(root.resolve()) or sha256(path) != digest:
            raise ValueError(f'changed or outside evidence: {name}')


def verify_analysis_code(root):
    paths = (__file__, sys.modules[ReplayAgent.__module__].__file__,
             sys.modules[RandomDomain.__module__].__file__)
    for path in paths:
        frozen = root / 'frozen-source/tools' / Path(path).name
        if sha256(path) != sha256(frozen):
            raise ValueError(f'analysis dependency differs from frozen code: {Path(path).name}')


def write_json(path, value):
    with path.open('x', encoding='utf-8', newline='\n') as out:
        json.dump(value, out, sort_keys=True, indent=2)
        out.write('\n')


def make_case(seed, excluded, program=None, divisor=0):
    random = RandomDomain(seed, 40)
    rejected = []
    if program is None and not divisor:
        for _ in range(100000):
            length = 2 + random.draw() % 2
            candidate = tuple(OPS[random.draw() % 27] for _ in range(length))
            values = table(candidate)
            reason = None
            if values in flat_models():
                reason = 'flat'
            elif any(all(v[e] == values[e] for e in range(1, 65)) for v in flat_models()):
                reason = 'flat-on-probes'
            elif not any(output(values, c, e) > 0 for c in CHAINS for e in (80, 96, 112, 128)):
                reason = 'no-positive-goal'
            elif values == EXAMPLE or values in excluded:
                reason = 'excluded-table'
            if reason:
                rejected.append({'program': candidate, 'reason': reason})
            else:
                program = candidate
                break
        else:
            raise ValueError(f'candidate budget exhausted for {seed}')
    if divisor:
        program = ()
    program = tuple(map(tuple, program))
    values = tuple(e * e // divisor for e in range(129)) if divisor else table(program)
    predictions = [{'input': e, 'chain': [0]} for e in (72, 80, 88, 96, 104, 112, 120, 128)]
    forecast_random = RandomDomain(seed, 42)
    extra = shuffle([(e, c) for e in (80, 96, 112, 128) for c in CHAINS if len(c) > 1], forecast_random)
    predictions.extend({'input': e, 'chain': list(c)} for e, c in extra[:16])
    pool = sorted({(e, output(values, c, e)) for e in (80, 96, 112, 128) for c in CHAINS
                   if output(values, c, e) > 0})
    if not pool:
        raise ValueError('no useful goal')
    goal_random = RandomDomain(seed, 43)
    goals = []
    for _ in range(20):
        energy, target = pool[goal_random.draw() % len(pool)]
        goals.append({'input': energy, 'target': target})
    zero_goals = [{'input': e, 'target': 0} for e in (80, 96, 112, 128)
                  if any(output(values, c, e) == 0 for c in CHAINS)]
    return {'seed': seed, 'program': [list(p) for p in program], 'divisor': divisor,
            'predictions': predictions, 'goals': goals, 'zero_goals': zero_goals,
            'goal_pool_size': len(pool), 'distinct_goals': len({(g['input'], g['target']) for g in goals}),
            'rejected': rejected, 'counters': {'40': random.counter, '42': forecast_random.counter,
                                             '43': goal_random.counter}}


def write_case(path, case):
    lines = [f"HCASE 1 {case['seed']} {case['divisor']} {len(case['program'])} " +
             ' '.join(str(v) for op in case['program'] for v in op)]
    for prediction in case['predictions']:
        chain = prediction['chain']
        lines.append(f"{prediction['input']} {len(chain)} " + ' '.join(map(str, chain)))
    lines.extend(f"{g['input']} {g['target']}" for g in case['goals'])
    lines.append(str(len(case['zero_goals'])))
    lines.extend(f"{g['input']} {g['target']}" for g in case['zero_goals'])
    lines.append(' '.join(str(case['counters'][tag]) for tag in ('40', '42', '43')))
    with path.open('x', encoding='utf-8', newline='\n') as out:
        out.write('\n'.join(lines) + '\n')


def sign_test(differences):
    wins, losses = sum(d > 0 for d in differences), sum(d < 0 for d in differences)
    n = wins + losses
    return Fraction(sum(math.comb(n, k) for k in range(wins, n + 1)), 2 ** n)


def summarize(rows):
    n = len(rows)
    if not n:
        raise ValueError('empty cohort')
    mean = lambda mode, key, denominator: Fraction(sum(r[mode][key] for r in rows), n * denominator)
    differences = lambda key: [r['SA'][key] - r['FA'][key] for r in rows]
    gain_d = mean('SA', 'D', 24) - mean('FA', 'D', 24)
    gain_g = mean('SA', 'G', 20) - mean('FA', 'G', 20)
    gain_a = mean('SA', 'curve', 528) - mean('SR', 'curve', 528)
    p_d, p_g = sign_test(differences('D')), sign_test(differences('G'))
    p_a = sign_test([r['SA']['curve'] - r['SR']['curve'] for r in rows])
    novel = Fraction(sum(bool(r['SA']['novel']) for r in rows), n)
    h = (novel >= Fraction(9, 10) and mean('SA', 'D', 24) >= Fraction(9, 10) and
         mean('SA', 'G', 20) >= Fraction(4, 5) and min(gain_d, gain_g) >= Fraction(1, 5) and
         max(p_d, p_g) <= Fraction(1, 60))
    return {'n': n, 'H': h, 'A': gain_a >= Fraction(1, 20) and p_a <= Fraction(1, 60),
            'D': {m: str(mean(m, 'D', 24)) for m in ('FA', 'SR', 'SA')},
            'G': {m: str(mean(m, 'G', 20)) for m in ('FA', 'SR', 'SA')},
            'curve': {m: str(mean(m, 'curve', 528)) for m in ('FA', 'SR', 'SA')},
            'gain_D': str(gain_d), 'gain_G': str(gain_g), 'gain_A': str(gain_a),
            'p_D': str(p_d), 'p_G': str(p_g), 'p_A': str(p_a), 'novel': str(novel),
            'sign_counts': {key: {'wins': sum(d > 0 for d in differences(key)),
                                  'losses': sum(d < 0 for d in differences(key)),
                                  'ties': sum(d == 0 for d in differences(key))} for key in ('D', 'G')}}


def claim_run(root):
    record = {'utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())}
    for name in ('freeze.json', 'case-manifest.json'):
        if (root / name).exists():
            record[name] = sha256(root / name)
    write_json(root / 'RUN.started.json', record)


def judge(case, raw):
    if raw['seed'] != case['seed'] or set(raw['variants']) != {'FA', 'SR', 'SA'}:
        raise ValueError('native case identity')
    truth = tuple(e * e // case['divisor'] for e in range(129)) if case['divisor'] else table(case['program'])
    if raw['law_table'] != list(truth):
        raise ValueError('native/Python interpreter mismatch')
    result = {}
    for mode in ('FA', 'SR', 'SA'):
        agent = ReplayAgent(mode, case['seed'])
        variant = raw['variants'][mode]
        if len(variant['steps']) != 24 or len(variant['goals']) != 20:
            raise ValueError('missing native trials')
        curve = spent = received = 0
        novel = False
        for index, step in enumerate(variant['steps']):
            choice = agent.choice(index)
            if step['choice'] != choice:
                raise ValueError(f'{mode} invalid intervention choice at {index}')
            energy = choice['input']
            measured = truth[energy]
            expected = {'id': 2 * index + 2, 'command': 2 * index + 2, 'tick': 2 * index + 2,
                        'actor': 1, 'apparatus': 0, 'input': energy, 'output': measured, 'chain': [0]}
            if step['measurement'] != expected:
                raise ValueError('nonphysical or wrong training observation')
            spent += energy
            received += measured
            agent.observe(energy, measured, expected['tick'])
            predictions = [agent.predict(p['chain'], p['input']) for p in case['predictions']] if index >= 2 else []
            if step['predictions'] != predictions or step['state'] != agent.state():
                raise ValueError(f'{mode} invalid prediction/state/cost at {index}')
            correct = sum(p is not None and p[0] == output(truth, task['chain'], task['input'])
                          for p, task in zip(predictions, case['predictions']))
            curve += correct
            novel |= agent.synthesized and bool(agent.current) and bool(agent.contradiction_tick)
        if variant['learning_ledger'] != {'source': 8192 - spent, 'output': received, 'loss': spent - received}:
            raise ValueError('learning energy imbalance')
        d = correct
        successes = zero_successes = eval_spent = eval_received = 0
        command = tick = 48
        if len(variant['zero_goals']) != len(case['zero_goals']):
            raise ValueError('missing zero diagnostics')
        for zero, tasks, records in ((False, case['goals'], variant['goals']),
                                     (True, case['zero_goals'], variant['zero_goals'])):
            for task, record in zip(tasks, records):
                chain = agent.choose_chain(task['input'], task['target'])
                measurement = None
                if chain is not None:
                    command += 2
                    tick += 2
                    value = output(truth, chain, task['input'])
                    measurement = {'id': command, 'command': command, 'tick': tick, 'actor': 1,
                                   'apparatus': 0, 'input': task['input'], 'output': value, 'chain': chain}
                    eval_spent += task['input']
                    eval_received += value
                    hit = value == task['target']
                    if zero:
                        zero_successes += hit
                    else:
                        successes += hit
                if record != {'input': task['input'], 'target': task['target'], 'chain': chain,
                              'measurement': measurement}:
                    raise ValueError('invalid physical goal result')
        if variant['evaluation_ledger'] != {'source': 8192 - eval_spent, 'output': eval_received,
                                            'loss': eval_spent - eval_received}:
            raise ValueError('evaluation energy imbalance')
        if variant['final_state'] != agent.state() or variant['final_tick'] != tick:
            raise ValueError('final state/cost mismatch')
        result[mode] = {'D': d, 'G': successes, 'curve': curve, 'novel': novel, 'zero_G': zero_successes,
                        'spent': spent, 'evaluation_spent': eval_spent, 'final_state': agent.state(),
                        'wall_ms': variant['wall_ms'], 'cpu_ms': variant['cpu_ms'],
                        'peak_process_bytes': variant['peak_process_bytes']}
    return result


def build_cases(mode, excluded):
    start, count = (0x48591000, 64) if mode == 'confirmatory' else (0x48590000, 16)
    cases = []
    for seed in range(start, start + count):
        case = make_case(seed, excluded)
        case['cohort'] = 'main'
        cases.append(case)
    control_random = RandomDomain(0x48592000, 40)
    controls = shuffle(list(flat_models().values()), control_random)[:16]
    for index, program in enumerate(controls):
        case = make_case(0x48592000 + index, set(), program=program)
        case['cohort'] = 'flat'
        case['control_source'] = {'seed': 0x48592000, 'counter': control_random.counter, 'index': index}
        cases.append(case)
    for index in range(16):
        case = make_case(0x48593000 + index, set(), divisor=(128, 160, 192, 256)[index // 4])
        case['cohort'] = 'outside'
        cases.append(case)
    return cases


def source_files():
    files = [ROOT / name for name in ('CMakeLists.txt', 'CMakePresets.json', '.clang-format')]
    files.append(PROTOCOL)
    for folder in ('src', 'tests', 'tools'):
        files.extend(p for p in (ROOT / folder).rglob('*')
                     if p.suffix in ('.hpp', '.cpp', '.py') and '__pycache__' not in p.parts)
    return sorted(files)


def validate_review_record(record, sources):
    if (not isinstance(record, dict) or record.get('protocol') != 'hypothesis-v1' or
            record.get('status') != 'READY' or not re.fullmatch('[0-9a-f]{40}', record.get('reviewed_head', '')) or
            record.get('verified_sources') != sources):
        raise ValueError('review must have explicit READY status and match verified source hashes')


def cache_values(text):
    return {line.split(':', 1)[0]: line.split('=', 1)[1] for line in text.splitlines()
            if not line.startswith(('#', '//')) and ':' in line and '=' in line}


def parse_native_metadata(cache, compiler, configuration):
    values = cache_values(cache)
    fields = dict(re.findall(r'set\((CMAKE_CXX_[A-Z_]+) "([^"\n]*)"\)', compiler))
    if (configuration not in values.get('CMAKE_CONFIGURATION_TYPES', '').split(';') or
            fields.get('CMAKE_CXX_COMPILER_ID') != 'MSVC' or
            fields.get('CMAKE_CXX_COMPILER_ARCHITECTURE_ID') != 'x64' or
            fields.get('CMAKE_CXX_SIZEOF_DATA_PTR') != '8' or not fields.get('CMAKE_CXX_COMPILER_VERSION')):
        raise ValueError('native provenance has unsupported configuration/compiler/architecture')
    return {'configuration': configuration, 'compiler_version': fields['CMAKE_CXX_COMPILER_VERSION'],
            'compiler_path': fields['CMAKE_CXX_COMPILER'], 'architecture': 'x64',
            'cmake_version': '.'.join(values[f'CMAKE_CACHE_{part}_VERSION'] for part in ('MAJOR', 'MINOR', 'PATCH')),
            'generator': values['CMAKE_GENERATOR'], 'address_sanitizer': values['REALITY_ENABLE_ASAN']}


def native_provenance(exe):
    build_root = exe.resolve().parent.parent
    cache_path = build_root / 'CMakeCache.txt'
    cache = cache_path.read_text(encoding='utf-8')
    values = cache_values(cache)
    version = '.'.join(values[f'CMAKE_CACHE_{part}_VERSION'] for part in ('MAJOR', 'MINOR', 'PATCH'))
    compiler_path = build_root / f'CMakeFiles/{version}/CMakeCXXCompiler.cmake'
    metadata = parse_native_metadata(cache, compiler_path.read_text(encoding='utf-8'), exe.parent.name)
    cmake = values['CMAKE_COMMAND']
    cmake_report = subprocess.check_output([cmake, '--version'], text=True).strip()
    if cmake_report.splitlines()[0] != f'cmake version {version}':
        raise ValueError('CMake binary differs from configured version')
    msbuild = Path(values['CMAKE_GENERATOR_INSTANCE']) / 'MSBuild/Current/Bin/MSBuild.exe'
    msbuild_version = subprocess.check_output([str(msbuild), '-nologo', '-version'], text=True).strip()
    if not re.fullmatch(r'\d+(\.\d+){2,3}', msbuild_version):
        raise ValueError('MSBuild version unavailable')
    return {**metadata, 'cmake_command': cmake, 'cmake_report': cmake_report,
            'msbuild_command': str(msbuild), 'msbuild_version': msbuild_version,
            'cache_sha256': sha256(cache_path), 'compiler_definition_sha256': sha256(compiler_path)}


def prepare(args):
    excluded = set()
    if args.mode == 'confirmatory':
        if not args.review or not args.development:
            raise ValueError('confirmatory requires completed review and development evidence')
        review = json.loads(args.review.read_text(encoding='utf-8'))
        validate_review_record(review, {p.relative_to(ROOT).as_posix(): sha256(p) for p in source_files()})
        analyze(args.development)
        development = json.loads((args.development / 'cases.json').read_text(encoding='utf-8'))
        excluded = {table(c['program']) for c in development if c['cohort'] == 'main'}
        if subprocess.check_output(['git', 'status', '--porcelain'], cwd=ROOT, text=True).strip():
            raise ValueError('confirmatory freeze requires clean committed source')
    build_metadata = native_provenance(args.exe)
    root = args.output.resolve()
    root.mkdir(parents=True, exist_ok=False)
    (root / 'engine').mkdir()
    shutil.copy2(args.exe, root / 'engine/reality_hypothesis.exe')
    hashes = {'engine/reality_hypothesis.exe': sha256(root / 'engine/reality_hypothesis.exe')}
    for source in source_files():
        name = 'frozen-source/' + source.relative_to(ROOT).as_posix()
        target = root / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        hashes[name] = sha256(target)
    if args.review:
        shutil.copy2(args.review, root / 'review.txt')
        hashes['review.txt'] = sha256(root / 'review.txt')
    freeze = {'protocol': 'hypothesis-v1', 'mode': args.mode, 'files': hashes,
              'utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
              'commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
              'dirty': bool(subprocess.check_output(['git', 'status', '--porcelain'], cwd=ROOT, text=True).strip()),
              'python': sys.version, 'platform': platform.platform(), 'native_build': build_metadata,
              'excluded_development_tables': sorted(map(list, excluded))}
    if args.development:
        freeze['development_manifest_sha256'] = sha256(args.development / 'result-manifest.json')
    # This immutable snapshot exists BEFORE any confirmatory law or goal is generated.
    write_json(root / 'freeze.json', freeze)
    cases = build_cases(args.mode, excluded)
    (root / 'cases').mkdir()
    case_hashes = {}
    for case in cases:
        name = f"cases/{case['seed']:08x}"
        write_json(root / (name + '.json'), case)
        write_case(root / (name + '.txt'), case)
        for suffix in ('.json', '.txt'):
            case_hashes[name + suffix] = sha256(root / (name + suffix))
    write_json(root / 'cases.json', cases)
    case_hashes['cases.json'] = sha256(root / 'cases.json')
    write_json(root / 'case-manifest.json', case_hashes)
    print(f"PREPARED {args.mode}: {len(cases)} cases after freeze; {root}")


def verify_prepared(root):
    freeze = json.loads((root / 'freeze.json').read_text(encoding='utf-8'))
    if freeze['protocol'] != 'hypothesis-v1':
        raise ValueError('wrong protocol')
    verify_hashes(root, freeze['files'])
    verify_analysis_code(root)
    case_hashes = json.loads((root / 'case-manifest.json').read_text(encoding='utf-8'))
    verify_hashes(root, case_hashes)
    cases = json.loads((root / 'cases.json').read_text(encoding='utf-8'))
    expected = build_cases(freeze['mode'], {tuple(t) for t in freeze['excluded_development_tables']})
    # Compare JSON forms: tuples in the generator become arrays on disk.
    if cases != json.loads(json.dumps(expected)):
        raise ValueError('case list differs from frozen seeds, generator or protocol')
    return freeze, cases


def run(root):
    _, cases = verify_prepared(root)
    claim_run(root)
    (root / 'raw').mkdir()
    rows = []
    for index, case in enumerate(cases):
        name = f"{case['seed']:08x}"
        directory = root / 'raw' / name
        completed = subprocess.run([str(root / 'engine/reality_hypothesis.exe'), '--case',
                                    str(root / f'cases/{name}.txt'), '--output', str(directory)],
                                   capture_output=True, text=True, timeout=180)
        (root / 'raw' / (name + '.stdout.txt')).write_text(completed.stdout, encoding='utf-8')
        (root / 'raw' / (name + '.stderr.txt')).write_text(completed.stderr, encoding='utf-8')
        if completed.returncode:
            raise RuntimeError(f'native infrastructure failure: {name}, exit {completed.returncode}')
        raw = json.loads((directory / 'raw.json').read_text(encoding='utf-8'))
        metrics = judge(case, raw)
        rows.append({'seed': case['seed'], 'cohort': case['cohort'], **metrics})
        print(f"JUDGED {index+1}/{len(cases)} {name} {case['cohort']}", flush=True)
    write_json(root / 'rows.json', rows)
    summary = {cohort: summarize([r for r in rows if r['cohort'] == cohort])
               for cohort in ('main', 'flat', 'outside')}
    write_json(root / 'summary.json', summary)
    write_json(root / 'RUN.completed.json', {'cases': len(cases), 'utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())})
    hashes = {p.relative_to(root).as_posix(): sha256(p) for p in root.rglob('*') if p.is_file()}
    write_json(root / 'result-manifest.json', hashes)
    print(json.dumps(summary['main'], sort_keys=True))


def analyze(root):
    root = root.resolve()
    manifest = json.loads((root / 'result-manifest.json').read_text(encoding='utf-8'))
    verify_hashes(root, manifest)
    expected_files = {p.relative_to(root).as_posix() for p in root.rglob('*')
                      if p.is_file() and p.name != 'result-manifest.json'}
    if expected_files != set(manifest):
        raise ValueError('evidence manifest set mismatch')
    _, cases = verify_prepared(root)
    marker = json.loads((root / 'RUN.started.json').read_text(encoding='utf-8'))
    if any(marker[name] != sha256(root / name) for name in ('freeze.json', 'case-manifest.json')):
        raise ValueError('run identity changed')
    completion = json.loads((root / 'RUN.completed.json').read_text(encoding='utf-8'))
    if completion['cases'] != len(cases):
        raise ValueError('incomplete native run')
    rows = []
    for case in cases:
        raw = json.loads((root / f"raw/{case['seed']:08x}/raw.json").read_text(encoding='utf-8'))
        rows.append({'seed': case['seed'], 'cohort': case['cohort'], **judge(case, raw)})
    summary = {cohort: summarize([r for r in rows if r['cohort'] == cohort])
               for cohort in ('main', 'flat', 'outside')}
    if rows != json.loads((root / 'rows.json').read_text(encoding='utf-8')):
        raise ValueError('stored row metrics differ from independently judged evidence')
    if summary != json.loads((root / 'summary.json').read_text(encoding='utf-8')):
        raise ValueError('stored summary differs from exact recomputation')
    print('PASS independent analysis ' + json.dumps(summary['main'], sort_keys=True))
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    subs = parser.add_subparsers(dest='command', required=True)
    prep = subs.add_parser('prepare')
    prep.add_argument('--mode', choices=('development', 'confirmatory'), required=True)
    prep.add_argument('--exe', type=Path, required=True)
    prep.add_argument('--output', type=Path, required=True)
    prep.add_argument('--review', type=Path)
    prep.add_argument('--development', type=Path)
    for name in ('run', 'analyze'):
        sub = subs.add_parser(name)
        sub.add_argument('directory', type=Path)
    args = parser.parse_args()
    try:
        if args.command == 'prepare':
            prepare(args)
        elif args.command == 'run':
            run(args.directory.resolve())
        else:
            analyze(args.directory)
    except Exception as error:
        print(f'FAIL hypothesis infrastructure: {error}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
