"""Independent integer interpreter and replay judge for hypothesis-v1.

No native bindings. This is experimenter code, never an agent information channel.
"""
from functools import lru_cache
from itertools import product
from run_discovery_experiment import RandomDomain

C = (4, 8, 12, 16, 24, 32, 48, 64)
OPS = tuple((kind, value) for kind in range(4)
            for value in ((4, 8, 12) if kind == 2 else C))
CHAINS = tuple(c for n in range(1, 4) for c in product(range(3), repeat=n) if 0 in c)


@lru_cache(None)
def _table(program):
    def apply(value):
        for kind, parameter in program:
            if kind == 0:
                value = max(0, value - parameter)
            elif kind == 1:
                value = min(value, parameter)
            elif kind == 2:
                value = value * parameter // 16
            elif kind == 3:
                value = value if value >= parameter else 0
            else:
                raise ValueError('unknown operator')
        return value
    return tuple(apply(e) for e in range(129))


def table(program):
    return _table(tuple(map(tuple, program)))


@lru_cache(None)
def syntax():
    return tuple(p for n in range(4) for p in product(OPS, repeat=n))


@lru_cache(None)
def flat_syntax():
    return ((),) + tuple((op,) for op in OPS) + tuple(((2, q),) for q in range(3, 13)) + tuple(
        ((0, d),) for d in range(1, 25)) + tuple(((1, c),) for c in range(8, 49))


def canonical(programs):
    result = {}
    for p in sorted(programs, key=lambda p: (len(p), p)):
        result.setdefault(table(p), p)
    return result


@lru_cache(None)
def flat_models():
    return canonical(flat_syntax())


@lru_cache(None)
def models():
    return canonical(syntax() + flat_syntax())


def output(values, chain, energy):
    for symbol in chain:
        energy = values[energy] if symbol == 0 else energy // 2 if symbol == 1 else min(energy, 48)
    return energy


def shuffle(values, random):
    values = list(values)
    for n in range(len(values), 1, -1):
        j = random.draw() % n
        values[n - 1], values[j] = values[j], values[n - 1]
    return values


class ReplayAgent:
    """Recomputes native declarations from measurements, including operation costs."""
    def __init__(self, mode, seed):
        self.mode = mode
        self.primitives = self.reads = self.generated = 0
        self.exhausted = self.synthesized = False
        self.contradiction_tick = 0
        self.evidence = []
        self.current = []
        self.build(flat_syntax())
        self.permutation = shuffle(range(1, 65), RandomDomain(seed, 41))

    def charge(self, count, primitive=False):
        field, limit = ('primitives', 10000000) if primitive else ('reads', 300000000)
        if self.exhausted:
            return False
        value = getattr(self, field)
        if value + count > limit:
            self.exhausted = True
            return False
        setattr(self, field, value + count)
        return True

    def build(self, programs, filtering=False):
        self.current = []
        seen = set()
        for p in sorted(programs, key=lambda p: (len(p), p)):
            if not self.charge(len(p) * 129, True):
                return
            values = table(p)
            if values in seen:
                continue
            seen.add(values)
            keep = True
            if filtering:
                for energy, measured in self.evidence:
                    if not self.charge(1):
                        return
                    if values[energy] != measured:
                        keep = False
                        break
            if keep:
                self.current.append(values)

    def observe(self, energy, measured, tick):
        self.evidence.append((energy, measured))
        if self.exhausted:
            return
        remaining = []
        for values in self.current:
            if not self.charge(1):
                return
            if values[energy] == measured:
                remaining.append(values)
        self.current = remaining
        if not remaining and not self.contradiction_tick:
            self.contradiction_tick = tick
            if self.mode != 'FA':
                self.synthesized = True
                self.generated = 20440
                self.build(syntax() + flat_syntax(), True)

    def choice(self, attempt):
        answer = {'input': 0, 'active': False, 'worst': [0] * 65, 'partition': [0] * 129}
        if attempt < 3:
            answer['input'] = (16, 32, 64)[attempt]
            return answer
        used = {e for e, _ in self.evidence}
        fallback = next(e for e in self.permutation if e not in used)
        answer['input'] = fallback
        if self.exhausted or len(self.current) < 2 or self.mode == 'SR':
            return answer
        best = len(self.current)
        for e in range(1, 65):
            if e in used:
                continue
            groups = [0] * 129
            for values in self.current:
                if not self.charge(1):
                    return {'input': fallback, 'active': False, 'worst': [0] * 65, 'partition': [0] * 129}
                groups[values[e]] += 1
            worst = max(groups)
            answer['worst'][e] = worst
            if worst < best:
                best = worst
                answer.update(input=e, partition=groups, active=True)
        return answer

    def evaluate(self, values, chain, energy):
        for symbol in chain:
            if symbol == 0:
                if not self.charge(1):
                    return None
                energy = values[energy]
            else:
                energy = energy // 2 if symbol == 1 else min(energy, 48)
        return energy

    def predict(self, chain, energy):
        if self.exhausted or not self.current:
            return None
        values = [self.evaluate(v, chain, energy) for v in self.current]
        if None in values:
            return None
        values.sort()
        return [values[(len(values) - 1) // 2], values[0], values[-1], len(values)]

    def choose_chain(self, energy, target):
        if self.exhausted or not self.current:
            return None
        candidates = []
        for chain in CHAINS:
            values = [self.evaluate(v, chain, energy) for v in self.current]
            if None in values:
                return None
            candidates.append((-values.count(target), max(values) - min(values), len(chain), chain))
        return list(min(candidates)[3])

    def state(self):
        return {'models': len(self.current), 'synthesized': self.synthesized,
                'contradiction_tick': self.contradiction_tick, 'exhausted': self.exhausted,
                'primitives': self.primitives, 'reads': self.reads, 'generated': self.generated}
