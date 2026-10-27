"""Protocol boundaries, independent interpreter and evidence integrity."""
from fractions import Fraction
from pathlib import Path
import sys
import tempfile
import unittest
import shutil
import subprocess

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import run_hypothesis_experiment as experiment


class HypothesisTests(unittest.TestCase):
    def test_grammar_and_outside(self):
        self.assertEqual(len(experiment.syntax()), 20440)
        table = experiment.table([[2, 8], [0, 8]])
        self.assertEqual(table[96], 40)
        self.assertEqual(table, experiment.table([[0, 16], [2, 8]]))
        self.assertNotIn(table, experiment.flat_models())
        models = experiment.models()
        for divisor in (128, 160, 192, 256):
            square = tuple(e * e // divisor for e in range(129))
            self.assertNotIn(square, models)
            self.assertNotIn(square, experiment.flat_models())

    def test_development_case_has_heldout_inputs_and_goals(self):
        case = experiment.make_case(0x48590000, set())
        self.assertEqual(len(case['predictions']), 24)
        self.assertEqual(len({(x['input'], tuple(x['chain'])) for x in case['predictions']}), 24)
        self.assertTrue(all(p['input'] > 64 and 0 in p['chain'] for p in case['predictions']))
        self.assertEqual(len(case['goals']), 20)
        self.assertTrue(all(g['target'] > 0 for g in case['goals']))
        self.assertGreater(case['counters']['40'], 0)
        self.assertGreater(case['counters']['42'], 0)
        self.assertGreater(case['counters']['43'], 0)
        self.assertEqual(case, experiment.make_case(0x48590000, set()))

    def test_run_marker_is_exclusive(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            experiment.claim_run(path)
            with self.assertRaises(FileExistsError):
                experiment.claim_run(path)

    def test_exact_gain_boundary_and_separate_active_hypothesis(self):
        rows = []
        for _ in range(64):
            rows.append({'FA': {'D': 0, 'G': 16, 'curve': 0, 'novel': False},
                         'SR': {'D': 24, 'G': 20, 'curve': 528, 'novel': True},
                         'SA': {'D': 24, 'G': 20, 'curve': 528, 'novel': True}})
        result = experiment.summarize(rows)
        self.assertTrue(result['H'])
        self.assertFalse(result['A'])
        self.assertEqual(result['gain_G'], str(Fraction(1, 5)))
        rows[0]['SA']['G'] = 19
        self.assertFalse(experiment.summarize(rows)['H'])

    def test_digest_detects_changed_evidence(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            evidence = root / 'raw.json'
            evidence.write_text('original', encoding='utf-8')
            digest = experiment.sha256(evidence)
            evidence.write_text('changed', encoding='utf-8')
            with self.assertRaises(ValueError):
                experiment.verify_hashes(root, {'raw.json': digest})

    def test_analysis_checks_all_imported_frozen_code(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            target = root / 'frozen-source/tools'
            target.mkdir(parents=True)
            for name in ('run_hypothesis_experiment.py', 'hypothesis_model.py', 'run_discovery_experiment.py'):
                shutil.copy2(Path(experiment.__file__).parent / name, target / name)
            experiment.verify_analysis_code(root)
            (target / 'hypothesis_model.py').write_text('changed dependency', encoding='utf-8')
            with self.assertRaises(ValueError):
                experiment.verify_analysis_code(root)

    def test_archive_tool_does_not_create_unmanifested_cache(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ('run_hypothesis_experiment.py', 'hypothesis_model.py', 'run_discovery_experiment.py'):
                shutil.copy2(Path(experiment.__file__).parent / name, root / name)
            subprocess.run([sys.executable, str(root / 'run_hypothesis_experiment.py'), '--help'],
                           check=True, capture_output=True)
            self.assertEqual(list(root.rglob('*.pyc')), [])

    def test_review_gate_requires_explicit_ready_and_matching_sources(self):
        sources = {'src/example.cpp': 'a' * 64}
        record = {'protocol': 'hypothesis-v1', 'status': 'READY', 'reviewed_head': 'b' * 40,
                  'verified_sources': sources}
        experiment.validate_review_record(record, sources)
        for status in ('NOT READY', 'WITH FIXES', 'READY after fixes', 'READY\nWITH FIXES'):
            with self.assertRaises(ValueError):
                experiment.validate_review_record({**record, 'status': status}, sources)
        with self.assertRaises(ValueError):
            experiment.validate_review_record({**record, 'note': 'READY', 'status': 'WITH FIXES'}, sources)
        with self.assertRaises(ValueError):
            experiment.validate_review_record(record, {'src/example.cpp': 'c' * 64})

    def test_native_provenance_binds_configuration_and_tool_versions(self):
        cache = ('CMAKE_CACHE_MAJOR_VERSION:INTERNAL=4\nCMAKE_CACHE_MINOR_VERSION:INTERNAL=4\n'
                 'CMAKE_CACHE_PATCH_VERSION:INTERNAL=4\nCMAKE_CONFIGURATION_TYPES:UNINITIALIZED=Debug;RelWithDebInfo\n'
                 'REALITY_ENABLE_ASAN:BOOL=OFF\nCMAKE_GENERATOR:INTERNAL=Visual Studio 17 2022\n')
        compiler = ('set(CMAKE_CXX_COMPILER "compiler.exe")\nset(CMAKE_CXX_COMPILER_ID "MSVC")\n'
                    'set(CMAKE_CXX_COMPILER_VERSION "19.44.35226.0")\n'
                    'set(CMAKE_CXX_COMPILER_ARCHITECTURE_ID "x64")\nset(CMAKE_CXX_SIZEOF_DATA_PTR "8")\n')
        metadata = experiment.parse_native_metadata(cache, compiler, 'RelWithDebInfo')
        self.assertEqual(metadata['compiler_version'], '19.44.35226.0')
        self.assertEqual(metadata['cmake_version'], '4.4.4')
        self.assertEqual(metadata['configuration'], 'RelWithDebInfo')
        with self.assertRaises(ValueError):
            experiment.parse_native_metadata(cache, compiler, 'Release')
        with self.assertRaises(ValueError):
            experiment.parse_native_metadata(cache, compiler.replace('"x64"', '"x86"'), 'RelWithDebInfo')


if __name__ == '__main__':
    unittest.main()
