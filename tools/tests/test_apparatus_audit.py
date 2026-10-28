import importlib.util
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
spec = importlib.util.find_spec('audit_apparatus')
audit = __import__('audit_apparatus') if spec else None


class ApparatusAuditTests(unittest.TestCase):
    def test_indistinguishable_until_intervention(self):
        self.assertIsNotNone(audit, 'missing apparatus distinguishability audit')
        # Two causal laws agree at every formerly permitted measurement.
        f = tuple(3 * max(0, x - 48) // 4 for x in range(129))
        g = tuple(3 * min(max(0, x - 48), 16) // 4 for x in range(129))
        self.assertEqual(audit.first_separator(f, g, range(1, 65)), None)
        self.assertEqual(audit.first_separator(f, g, range(1, 129)), 66)
        self.assertEqual((f[96], g[96]), (36, 12))

    def test_adaptive_tree_proves_bound_for_every_model(self):
        self.assertIsNotNone(audit, 'missing apparatus distinguishability audit')
        tables = ((0, 0, 0), (0, 0, 1), (0, 1, 0), (0, 1, 1))
        tree = audit.decision_tree(tables, (1, 2))
        self.assertEqual(tree['max_depth'], 2)
        self.assertEqual(tree['leaves'], 4)
        self.assertEqual(tree['unresolved'], 0)
        limited = audit.decision_tree(tables, (1,))
        self.assertEqual(limited['unresolved'], 4)


if __name__ == '__main__':
    unittest.main()
