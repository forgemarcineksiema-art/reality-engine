import copy
import json
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import contrast_judge as j
import run_contrast_experiment as runner


class ContrastTests(unittest.TestCase):
    def test_generation_is_reproducible(self):
        a=runner.generate(0x44560000)
        self.assertEqual(a,runner.generate(0x44560000))
        self.assertEqual(len(a['goals']),20)
        self.assertNotIn(j.truth(a),j.h.flat_models())

    def test_false_scientific_claim_rejected(self):
        rows=[dict(A=dict(D='1',curve='1',G=20,singleton=True,teacher=1,social=[1,1]),
                   R=dict(curve='1')) for _ in range(64)]
        summary=j.summarize(rows)
        self.assertTrue(summary['I'])
        self.assertFalse(summary['A'])
        self.assertFalse(summary['T'])

    def test_development_raw_replayed_and_tampering_rejected(self):
        root=Path(__file__).resolve().parents[2]/'tests/data/contrast'
        case=json.loads((root/'development-case.json').read_text())
        raw=json.loads((root/'development-raw.json').read_text())
        j.judge(case,raw)
        for change in ('measurement','choice','prediction','work','energy'):
            modified=copy.deepcopy(raw)
            if change=='measurement': modified['A']['steps'][0]['trial']['measurement']['output']+=1
            elif change=='choice': modified['A']['steps'][0]['trial']['input']+=1
            elif change=='prediction': modified['A']['steps'][0]['predictions'][0][0]+=1
            elif change=='work': modified['A']['teacher']['parts']+=1
            else: modified['A']['energy']+=1
            with self.subTest(change=change),self.assertRaises(ValueError):
                j.judge(case,modified)


if __name__=='__main__': unittest.main()
