"""Finite observational equivalence audit; no agent or confirmatory outcomes."""
import sys
sys.dont_write_bytecode = True
import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import hypothesis_model as h


def first_separator(a, b, inputs):
    return next((x for x in inputs if a[x] != b[x]), None)


def decision_tree(tables, inputs):
    """Exhaust every possible outcome of the deterministic minimax policy.

    This proves a bound for this policy on this finite set, not an optimal tree.
    """
    stack = [(tuple(tables), tuple(inputs), 0)]
    leaves = unresolved = maximum = reads = 0
    depths = Counter()
    while stack:
        current, available, depth = stack.pop()
        maximum = max(maximum, depth)
        if len(current) == 1:
            leaves += 1
            depths[depth] += 1
            continue
        best, best_groups = len(current), None
        for x in available:
            groups = defaultdict(list)
            for table in current:
                groups[table[x]].append(table)
            reads += len(current)
            worst = max(map(len, groups.values()))
            if worst < best:
                best, best_groups, chosen = worst, groups, x
        if best_groups is None:
            unresolved += len(current)
            continue
        rest = tuple(x for x in available if x != chosen)
        stack.extend((tuple(group), rest, depth + 1) for group in best_groups.values())
    return dict(max_depth=maximum, leaves=leaves, unresolved=unresolved,
                depths=dict(sorted(depths.items())), model_reads=reads)


def run():
    models = h.models()
    groups = defaultdict(list)
    for table, program in models.items():
        groups[table[1:65]].append((table, program))
    ambiguous = [group for group in groups.values() if len(group) > 1]
    # All primitive laws map zero to zero. Therefore distinct full tables always
    # differ at a permitted positive input; equal full tables share all chains.
    assert all(table[0] == 0 for table in models)
    f = h.table(((0, 48), (2, 12)))
    g = h.table(((0, 48), (1, 16), (2, 12)))
    return dict(protocol='apparatus-audit-v1', syntax=len(h.syntax()),
                semantic_models=len(models), restricted_classes=len(groups),
                ambiguous_classes=len(ambiguous),
                ambiguous_models=sum(map(len, ambiguous)),
                maximum_ambiguity=max(map(len, groups.values())),
                ambiguous_pairs=sum(len(g) * (len(g) - 1) // 2 for g in ambiguous),
                catalogue_sha256=hashlib.sha256(json.dumps(
                    sorted(models), separators=(',', ':')).encode()).hexdigest(),
                full_tree=decision_tree(models, range(1, 129)),
                witness=dict(f=models[f], g=models[g],
                             old_separator=first_separator(f, g, range(1, 65)),
                             new_separator=first_separator(f, g, range(1, 129)),
                             at_96=[f[96], g[96]]))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = run()
    with args.output.open('x', encoding='utf-8') as out:
        json.dump(result, out, indent=2)
        out.write('\n')
    print(json.dumps(result))
