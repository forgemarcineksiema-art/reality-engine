"""Offline report authoring with the existing Matplotlib installation.

Not a dependency of the engine, runner or tests. Inputs must already pass the
frozen experiment's independent analyze command. No thresholds are computed here.
"""
import argparse
import csv
import json
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


def chain_output(values, chain, energy):
    for symbol in chain:
        energy = values[energy] if symbol == 0 else energy // 2 if symbol == 1 else min(energy, 48)
    return energy


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    cases = [c for c in json.loads((args.input / 'cases.json').read_text()) if c['cohort'] == 'main']
    rows = {r['seed']: r for r in json.loads((args.input / 'rows.json').read_text())}
    grid = [112] + list(range(128, 1537, 32))
    curves = {m: [] for m in ('FA', 'SR', 'SA')}
    prefixes = {m: [] for m in curves}
    for case in cases:
        raw = json.loads((args.input / f"raw/{case['seed']:08x}/raw.json").read_text())
        for mode in curves:
            measured = []
            spent = 0
            counts = []
            for index, step in enumerate(raw['variants'][mode]['steps']):
                spent += step['measurement']['input']
                if index < 2:
                    continue
                correct = sum(p is not None and p[0] == chain_output(raw['law_table'], t['chain'], t['input'])
                              for p, t in zip(step['predictions'], case['predictions']))
                measured.append((spent, correct))
                counts.append(correct)
            assert counts[-1] == rows[case['seed']][mode]['D']
            prefixes[mode].append(counts)
            curves[mode].append([next(n for e, n in reversed(measured) if e <= energy) for energy in grid])
    means = {mode: [100 * sum(world[i] for world in worlds) / (24 * len(cases))
                    for i in range(len(grid))] for mode, worlds in curves.items()}
    args.output.mkdir(parents=True, exist_ok=True)
    with (args.output / 'learning-curves.csv').open('w', encoding='utf-8', newline='') as out:
        writer = csv.writer(out)
        writer.writerow(['energy_eu', 'FA_percent', 'SR_percent', 'SA_percent'])
        for index, energy in enumerate(grid):
            writer.writerow([energy] + [means[m][index] for m in ('FA', 'SR', 'SA')])
    with (args.output / 'prediction-prefixes.csv').open('w', encoding='utf-8', newline='') as out:
        writer = csv.writer(out)
        writer.writerow(['completed_probes', 'FA_percent', 'SR_percent', 'SA_percent'])
        for i in range(22):
            writer.writerow([i+3] + [100*sum(w[i] for w in prefixes[m])/(24*len(cases)) for m in ('FA','SR','SA')])
    plt.rcParams.update({'font.size': 11, 'svg.hashsalt': 'hypothesis-v1'})
    fig, ax = plt.subplots(figsize=(9.5, 6.2))
    styles = [('FA', '#6b7280', 'Katalog — FA'), ('SR', '#d97706', 'Synteza, losowe próby — SR'),
              ('SA', '#2563eb', 'Synteza, aktywne próby — SA')]
    for mode, color, label in styles:
        ax.plot(grid, means[mode], color=color, linewidth=2.2, label=label)
    ax.axhline(90, color='#9ca3af', linestyle='--', linewidth=1, label='Próg D na końcu próby: 90%')
    ax.set(title='Trafność prognoz a faktycznie zużyta energia', xlabel='Energia uczenia [eu]',
           ylabel='Dokładne prognozy [%]', xlim=(112,1536), ylim=(0,102))
    ax.set_xticks([112,256,512,768,1024,1280,1536])
    ax.grid(alpha=.2)
    ax.spines[['top','right']].set_visible(False)
    ax.legend(loc='upper center', bbox_to_anchor=(.5,-.18), ncol=2, frameon=False, fontsize=9)
    fig.text(.5,.025, f"Średnia: {len(cases)} światów. Ostatnia dostępna prognoza; po 24 próbach model zamrożony.",
             ha='center', fontsize=8)
    fig.subplots_adjust(bottom=.27,top=.90,left=.10,right=.97)
    fig.savefig(args.output / 'learning-curves.png', dpi=160)
    fig.savefig(args.output / 'learning-curves.svg', metadata={'Date':'2026-10-27'})
    plt.close(fig)
    print(f'PASS report figure, Matplotlib {matplotlib.__version__}; {len(cases)} worlds')


if __name__ == '__main__':
    main()
