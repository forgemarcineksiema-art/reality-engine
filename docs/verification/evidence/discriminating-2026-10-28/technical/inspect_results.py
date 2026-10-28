"""Posthoc diagnostics only; thresholds and original raw outcomes unchanged."""
import json
from pathlib import Path
import sys
r=Path(sys.argv[1])
cases=json.loads((r/'cases.json').read_text())
a=json.loads((r/'analysis.json').read_text())
prefix={}
examples=[]
for name,lo,hi in [('main',0,64),('outside',64,80)]:
    prefix[name]={m:{k:[sum(row[m]['diagnostics'][i][k] for row in a['rows'][lo:hi]) for i in range(8)]
                    for k in ['false_unanimous','unanimous','covered','models']}
                  for m in ['R','A']}
for case,row in zip(cases[:64],a['rows'][:64]):
    if row['A']['social']==[0,1]:
        raw=json.loads((r/'raw'/('%08x.json'%case['seed'])).read_text())['A']
        examples.append(dict(seed=hex(case['seed']),program=case['program'],goal=case['goals'][0],
                             teacher=raw['teacher'],B_no=raw['social'][0],B_message=raw['social'][1]))
(r/'prefix-diagnostics.json').write_text(json.dumps(prefix,indent=2)+'\n')
(r/'teaching-examples.json').write_text(json.dumps(examples,indent=2)+'\n')
print('prefixFalse',{n:{m:prefix[n][m]['false_unanimous'] for m in ['R','A']} for n in prefix})
for example in examples:
    print({k:example[k] for k in ['seed','program','goal','teacher']}|
          {'B_no':example['B_no']['work'],'B_message':example['B_message']['work']})
