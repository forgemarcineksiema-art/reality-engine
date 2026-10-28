"""Authoring-only plot, Matplotlib 3.10; runner and analysis use stdlib."""
import json
from pathlib import Path
import sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

root=Path(sys.argv[1])
cases=json.loads((root/'cases.json').read_text())[:64]
curves={mode:[] for mode in ('R','A')}
for c in cases:
    def value(x):
        for k,p in c['program']:
            if k==0: x=max(0,x-p)
            elif k==1: x=min(x,p)
            elif k==2: x=x*p//16
            else: x=x if x>=p else 0
        return x
    truth=[value(x) for x in range(129)]
    raw=json.loads((root/'raw'/f"{c['seed']:08x}.json").read_text())
    for mode in curves:
        used=set()
        curve=[]
        for step in raw[mode]['steps']:
            used.add(step['trial']['input'])
            curve.append(100*sum(p is not None and p[0]==truth[x] for x,p in enumerate(step['predictions'],1) if x not in used)/(128-len(used)))
        curves[mode].append(curve)
fig,ax=plt.subplots(figsize=(9,4.8),layout='constrained')
for mode,color,label in [('R','#64748b','Losowy wybór R'),('A','#087f8c','Aktywny wybór A')]:
    values=[sum(row[i] for row in curves[mode])/len(cases) for i in range(8)]
    ax.plot(range(1,9),values,'o-',label=label,color=color,lw=2)
ax.set(title='Wybór doświadczenia a trafność przewidywań',xlabel='Liczba fizycznych pomiarów',ylabel='Trafność na jeszcze niezmierzonych wejściach (%)',xticks=range(1,9),ylim=(0,103))
ax.spines[['top','right']].set_visible(False)
ax.grid(axis='y',alpha=.2)
ax.legend(loc='lower right',frameon=False)
fig.suptitle('discrimination-v1 · 64 światy · wspólny zakres 1–128 i budżety',fontsize=10,color='#475569')
fig.savefig(root/'learning-curves.png',dpi=160)
fig.savefig(root/'learning-curves.svg')
