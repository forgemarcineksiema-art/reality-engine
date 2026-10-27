"""Post-run descriptive diagnostics; never used to select or tune a trial."""
import argparse
import json
import statistics
from pathlib import Path


def output(values, chain, energy):
    for symbol in chain:
        energy = values[energy] if symbol == 0 else energy // 2 if symbol == 1 else min(energy,48)
    return energy


def describe(root):
    rows = json.loads((root / 'rows.json').read_text())
    cases = [c for c in json.loads((root / 'cases.json').read_text()) if c['cohort'] == 'main']
    by_seed = {r['seed']:r for r in rows}
    answer = {'kind':'descriptive post-run diagnostics; primary H/A unchanged', 'worlds':len(cases),
              'variants':{}, 'primary':json.loads((root/'summary.json').read_text())}
    tables = set()
    for mode in ('FA','SR','SA'):
        counts = dict(direct=0,composed=0,zero=0,nonzero=0,zero_total=0,issued=0,covered=0,
                      unanimous=0,correct_unanimous=0)
        widths=[]
        for case in cases:
            raw=json.loads((root/f"raw/{case['seed']:08x}/raw.json").read_text())
            tables.add(tuple(raw['law_table']))
            correct=0
            for index,(p,task) in enumerate(zip(raw['variants'][mode]['steps'][-1]['predictions'],case['predictions'])):
                truth=output(raw['law_table'],task['chain'],task['input'])
                hit=p is not None and p[0]==truth
                correct+=hit
                counts['direct' if index<8 else 'composed']+=hit
                counts['zero' if truth==0 else 'nonzero']+=hit
                counts['zero_total']+=truth==0
                if p is not None:
                    counts['issued']+=1;counts['covered']+=p[1]<=truth<=p[2];widths.append(p[2]-p[1])
                    if p[1]==p[2]:counts['unanimous']+=1;counts['correct_unanimous']+=hit
            assert correct==by_seed[case['seed']][mode]['D']
        values=[by_seed[c['seed']][mode] for c in cases]
        costs={key:{'mean':statistics.mean(v[key] for v in values),'min':min(v[key] for v in values),
                    'max':max(v[key] for v in values)} for key in ('spent','evaluation_spent','wall_ms','cpu_ms')}
        costs.update({key:{'mean':statistics.mean(v['final_state'][key] for v in values),
                           'max':max(v['final_state'][key] for v in values)} for key in ('primitives','reads')})
        answer['variants'][mode]={'D':sum(v['D'] for v in values),'G':sum(v['G'] for v in values),
            'novel':sum(v['novel'] for v in values),'counts':counts,'range_width_mean':statistics.mean(widths),
            'range_width_max':max(widths),'costs':costs,
            'exhausted':sum(v['final_state']['exhausted'] for v in values)}
    differences=[by_seed[c['seed']]['SA']['curve']-by_seed[c['seed']]['SR']['curve'] for c in cases]
    answer['A_sign_counts']={'wins':sum(v>0 for v in differences),'ties':sum(v==0 for v in differences),
                             'losses':sum(v<0 for v in differences)}
    answer['distinct_main_laws']=len(tables)
    answer['max_cumulative_native_process_peak_bytes']=max(r[m]['peak_process_bytes'] for r in rows for m in ('FA','SR','SA'))
    answer['SA_no_synthesis']=[{'seed':r['seed'],'D':r['SA']['D'],'G':r['SA']['G']}
                               for r in rows if r['cohort']=='main' and not r['SA']['novel']]
    answer['controls']={cohort:{'count':sum(r['cohort']==cohort for r in rows),
        'SA_empty_final':sum(r['SA']['final_state']['models']==0 for r in rows if r['cohort']==cohort)}
        for cohort in ('flat','outside')}
    return answer


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    with args.output.open('x',encoding='utf-8',newline='\n') as out:
        json.dump(describe(args.input),out,indent=2,sort_keys=True);out.write('\n')
    print('PASS descriptive diagnostics checked against frozen judged rows')


if __name__=='__main__':main()
