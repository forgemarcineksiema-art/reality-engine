"""Independent integer replay for discrimination-v1. Never used by native agents."""
import sys
sys.dont_write_bytecode = True
from collections import Counter
from fractions import Fraction
from math import comb
import hypothesis_model as h


class Mind(h.ReplayAgent):
    def __init__(self, active, seed):
        self.active = active
        self.primitives = self.reads = self.generated = 0
        self.exhausted = False
        self.evidence = []
        self.current = []
        self.build(h.syntax() + h.flat_syntax())
        self.generated = len(h.syntax())
        self.permutation = h.shuffle(range(1, 129), h.RandomDomain(seed, 61))

    def observe(self, x, y):
        self.evidence.append((x, y))
        remaining = []
        for values in self.current:
            if not self.charge(1):
                return
            if values[x] == y:
                remaining.append(values)
        self.current = remaining

    def choice(self):
        used = {x for x, _ in self.evidence}
        fallback = next(x for x in self.permutation if x not in used)
        c = dict(input=fallback, active=0, before=len(self.current), worst=0, partition=[0]*129)
        if not self.active or self.exhausted or len(self.current) < 2:
            return c
        c['worst'] = len(self.current)
        for x in range(1, 129):
            if x in used:
                continue
            group = [0]*129
            for values in self.current:
                if not self.charge(1):
                    return dict(input=fallback, active=0, before=len(self.current), worst=0, partition=[0]*129)
                group[values[x]] += 1
            worst = max(group)
            if worst < c['worst']:
                c.update(input=x, active=1, worst=worst, partition=group)
        return c

    def choose_chain(self, x, y):
        if not self.current or self.exhausted:
            return None
        candidates = []
        for chain in h.CHAINS:
            values = [self.evaluate(v, chain, x) for v in self.current]
            if None in values:
                return None
            hits = values.count(y)
            if hits:
                candidates.append((-hits, max(values)-min(values), len(chain), chain))
        return list(min(candidates)[3]) if candidates else None


def check(value, message):
    if not value:
        raise ValueError(message)


def truth(case):
    return (tuple(x*x//case['divisor'] for x in range(129)) if case['divisor']
            else h.table(case['program']))


def trial(mind, raw, values, actor, tick, command):
    c = mind.choice()
    check(all(raw[k] == v for k, v in c.items()), 'intervention declaration changed')
    x = c['input']
    expected = dict(id=command, source=actor, tick=tick+2, input=x, output=values[x])
    check(raw['measurement'] == expected, 'physical measurement changed')
    mind.observe(x, values[x])
    check(raw['models'] == len(mind.current), 'remaining competitors changed')
    return tick+2, command+2


def goal(mind, raw, values, pair, actor, tick, command, work=False):
    x, y = pair
    chain = mind.choose_chain(x, y)
    check(raw['chain'] == chain, 'goal declaration changed')
    good = False
    if chain is not None:
        result = h.output(values, chain, x)
        check(raw['measurement'] == dict(id=command, source=actor, tick=tick+2, input=x, output=result), 'goal measurement changed')
        good = result == y
        tick += 82 if work else 2
        command += 2
    else:
        check(raw['measurement'] is None, 'undeclared impulse')
    check(raw['success'] == good, 'invented success')
    check(raw['parts'] == 4-int(good and work) and raw['completed'] == int(good and work), 'material/work balance changed')
    return tick, command, int(good)


def judge(case, raw):
    values = truth(case)
    result = {}
    for mode in ('R', 'A'):
        r = raw[mode]
        a = Mind(mode == 'A', case['seed'])
        tick, command = 0, 2
        curve = []
        for step in r['steps']:
            tick, command = trial(a, step['trial'], values, 1, tick, command)
            ps = [a.predict((0,), x) for x in range(1,129)]
            check(step['predictions'] == ps, 'prediction declaration changed')
            used = {x for x, _ in a.evidence}
            curve.append(Fraction(sum(p is not None and p[0] == values[x] for x,p in enumerate(ps,1) if x not in used), 128-len(used)))
        check(len(curve) == 8, 'training budget changed')
        hits = 0
        for pair, g in zip(case['goals'], r['goals'], strict=True):
            tick, command, good = goal(a,g,values,pair,1,tick,command)
            hits += good
        tick, command, teacher = goal(a,r['teacher'],values,case['goals'][0],1,tick,command,True)
        check((r['reads'],r['primitives'],r['exhausted']) == (a.reads,a.primitives,a.exhausted), 'teacher computation costs changed')
        expected_energy = sum(step['trial']['input'] for step in r['steps'])
        expected_energy += sum(g['measurement']['input'] for g in r['goals'] if g['measurement'])
        if r['teacher']['measurement']:
            expected_energy += r['teacher']['measurement']['input']
        check(r['energy'] == expected_energy, 'teacher energy changed')
        social = []
        for message, braw in enumerate(r['social']):
            b = Mind(True,case['seed'])
            btick, bcommand = tick+braw['contact_ticks'],command
            check(0 < braw['contact_ticks'] < 1000, 'invalid physical contact cost')
            if 'contact_tick' in braw:
                check(tick < braw['contact_tick'] <= btick, 'invalid contact instant')
            check(len(braw['received']) == (8 if message else 0), 'packet length')
            for i,e in enumerate(braw['received']):
                original = r['steps'][i]['trial']['measurement']
                check(e['measurement'] == original and e['via'] == 1 and original['tick'] < e['learned'] <= btick, 'teaching provenance changed')
                if 'contact_tick' in braw:
                    check(e['learned'] == braw['contact_tick'], 'wrong teaching instant')
                b.observe(original['input'], original['output'])
            for t in braw['trials']:
                btick,bcommand = trial(b,t,values,2,btick,bcommand)
            check(len(braw['trials']) == 2, 'recipient budget')
            btick,bcommand,good = goal(b,braw['work'],values,case['goals'][0],2,btick,bcommand,True)
            check((braw['reads'],braw['primitives']) == (b.reads,b.primitives), 'recipient costs changed')
            expected_b_energy = sum(t['input'] for t in braw['trials'])
            if braw['work']['measurement']:
                expected_b_energy += braw['work']['measurement']['input']
            check(braw['energy'] == expected_b_energy, 'recipient energy changed')
            social.append(good)
        check(len(social)==2 and r['social'][0]['contact_ticks']==r['social'][1]['contact_ticks'], 'unequal contact')
        result[mode] = dict(D=str(curve[-1]), curve=str(sum(curve)/8), G=hits, singleton=len(a.current)==1, teacher=teacher, social=social)
    return result


def sign(differences):
    wins = sum(x>0 for x in differences)
    losses = sum(x<0 for x in differences)
    n = wins+losses
    return dict(wins=wins,losses=losses,ties=len(differences)-n,
                p=str(Fraction(sum(comb(n,k) for k in range(wins,n+1)),2**n)))


def summarize(rows):
    n = len(rows)
    d = sum(Fraction(r['A']['D']) for r in rows)/n
    g = Fraction(sum(r['A']['G'] for r in rows),20*n)
    identified = sum(r['A']['singleton'] for r in rows)
    delta = [Fraction(r['A']['curve'])-Fraction(r['R']['curve']) for r in rows]
    test = sign(delta)
    teacher = sum(r['A']['teacher'] for r in rows)
    social = [sum(r['A']['social'][m] for r in rows) for m in (0,1)]
    ttest = sign([r['A']['social'][1]-r['A']['social'][0] for r in rows])
    return dict(n=n,D=str(d),G=str(g),identified=identified,delta=str(sum(delta)/n),A_sign=test,
                teacher=teacher,social=social,T_sign=ttest,
                I=identified*10>=9*n and d>=Fraction(95,100) and g>=Fraction(9,10),
                A=sum(delta)/n>=Fraction(5,100) and Fraction(test['p'])<=Fraction(25,1000),
                T=teacher*10>=9*n and social[1]*10>=9*n and (social[1]-social[0])*5>=n and Fraction(ttest['p'])<=Fraction(25,1000))
