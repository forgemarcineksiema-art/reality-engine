"""Build inspectable diagnostics/cost tables from an already independently judged run."""
import sys
sys.dont_write_bytecode = True
import csv
from fractions import Fraction
import json
from pathlib import Path
from statistics import mean
from run_contrast_experiment import read, verify, verify_files


def distribution(values):
    return dict(min=min(values), mean=mean(values), max=max(values))


def report(root):
    cases = verify(root)
    verify_files(root/'raw', read(root/'raw-manifest.json'), exact=True)
    analysis = read(root/'analysis.json')
    n = analysis['main']['n']
    output = {}
    for name, begin, end in (('main',0,n),('outside',n,len(cases))):
        cohort = {}
        for mode in ('R','A'):
            rows = [read(root/'raw'/f"{c['seed']:08x}.json")[mode] for c in cases[begin:end]]
            judged = [r[mode] for r in analysis['rows'][begin:end]]
            diag = [r['diagnostics'][-1] for r in judged]
            costs = {key:distribution([r[key] for r in rows]) for key in
                     ('training_energy','energy','primitives','reads','training_wall_ms','training_cpu_ms',
                      'wall_ms','cpu_ms','process_lifetime_peak_bytes')}
            for message in (0,1):
                costs['B_message' if message else 'B_no_message'] = {
                    key:distribution([r['social'][message][key] for r in rows]) for key in
                    ('energy','primitives','reads','wall_ms','cpu_ms','contact_ticks','contact_wall_ms','contact_cpu_ms')}
                costs['B_message' if message else 'B_no_message']['own_probe_energy'] = distribution([
                    sum(t['input'] for t in r['social'][message]['trials']) for r in rows])
            cohort[mode] = dict(costs=costs,
                final_models=distribution([d['models'] for d in diag]),
                final_all_correct=sum(d['all_correct'] for d in diag),
                final_covered=sum(d['covered'] for d in diag),
                final_unanimous=sum(d['unanimous'] for d in diag),
                final_false_unanimous=sum(d['false_unanimous'] for d in diag),
                final_width_sum=sum(d['width'] for d in diag),
                empty_final=sum(d['models']==0 for d in diag),
                first_singleton=[r['first_singleton'] for r in judged],
                teacher_work=sum(r['teacher'] for r in judged),
                B_work=[sum(r['social'][m] for r in judged) for m in (0,1)],
                exhausted=sum(r['exhausted'] for r in rows),
                curve=[mean(float(Fraction(d['all_correct'],128)) for d in
                       [r['diagnostics'][i] for r in judged]) for i in range(8)])
        output[name]=cohort
    output['distinct_tasks']=distribution([c['distinct_goals'] for c in cases[:n]])
    output['pool_size']=distribution([c['pool_size'] for c in cases[:n]])
    output['distinct_main_laws']=analysis['distinct_main_laws']
    output['rejected_candidates']=sum(len(c['rejections']) for c in cases[:n])
    (root/'diagnostics.json').write_text(json.dumps(output,indent=2)+'\n')
    with (root/'costs.csv').open('w',newline='') as out:
        writer=csv.writer(out)
        writer.writerow(('seed','mode','training_energy','teacher_energy_total','reads','primitives','wall_ms','cpu_ms','lifetime_peak_bytes','contact_ticks','B_no_message_energy','B_message_energy'))
        for c in cases:
            raw=read(root/'raw'/f"{c['seed']:08x}.json")
            for mode in ('R','A'):
                r=raw[mode]
                writer.writerow((f"{c['seed']:08x}",mode,r['training_energy'],r['energy'],r['reads'],r['primitives'],
                    r['wall_ms'],r['cpu_ms'],r['process_lifetime_peak_bytes'],r['social'][0]['contact_ticks'],
                    r['social'][0]['energy'],r['social'][1]['energy']))
    return output


if __name__=='__main__':
    result=report(Path(sys.argv[1]))
    print(json.dumps(result,indent=2))
