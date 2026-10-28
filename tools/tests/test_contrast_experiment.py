import copy
import json
from pathlib import Path
import sys
import unittest
import tempfile
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
        j.judge_legacy(case,raw)
        for change in ('measurement','choice','prediction','work','energy'):
            modified=copy.deepcopy(raw)
            if change=='measurement': modified['A']['steps'][0]['trial']['measurement']['output']+=1
            elif change=='choice': modified['A']['steps'][0]['trial']['input']+=1
            elif change=='prediction': modified['A']['steps'][0]['predictions'][0][0]+=1
            elif change=='work': modified['A']['teacher']['parts']+=1
            else: modified['A']['energy']+=1
            with self.subTest(change=change),self.assertRaises(ValueError):
                j.judge_legacy(case,modified)

    def test_strict_reader_requires_contact_and_metrics(self):
        root=Path(__file__).resolve().parents[2]/'tests/data/contrast'
        case=json.loads((root/'development-case.json').read_text())
        raw=json.loads((root/'development-raw.json').read_text())
        with self.assertRaises(ValueError):
            j.judge(case,raw)

    def test_source_freeze_includes_untracked_native_files(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary)
            (root/'src').mkdir()
            (root/'src/new.cpp').write_text('new source')
            self.assertIn(root/'src/new.cpp',runner.source_files(root))

    def test_manifest_verification_rejects_escape_and_extra_files(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary)
            (root/'data.txt').write_text('data')
            manifest={'data.txt':runner.digest(root/'data.txt')}
            self.assertTrue(callable(getattr(runner,'verify_files',None)), 'missing strict manifest verification')
            runner.verify_files(root,manifest,exact=True)
            (root/'extra.txt').write_text('extra')
            with self.assertRaises(ValueError): runner.verify_files(root,manifest,exact=True)
            with self.assertRaises(ValueError): runner.verify_files(root,{'../outside':'0'*64})

    def test_review_record_exact_status_and_hash_binding(self):
        self.assertTrue(callable(getattr(runner,'validate_review',None)), 'missing readiness gate')
        sources={'src/new.cpp':'a'*64}
        good=dict(protocol='discrimination-v1',status='READY',reviewed_head='a'*40,verified_sources=sources)
        runner.validate_review(good,sources)
        for bad in (dict(good,status='NOT READY'),dict(good,verified_sources={})): 
            with self.assertRaises(ValueError): runner.validate_review(bad,sources)

    def test_run_marker_cannot_be_reused(self):
        self.assertTrue(callable(getattr(runner,'claim_run',None)), 'missing bound run marker')
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary)
            (root/'manifest.json').write_text('{}')
            (root/'freeze.json').write_text('{}')
            runner.claim_run(root)
            with self.assertRaises(FileExistsError): runner.claim_run(root)


if __name__=='__main__': unittest.main()
