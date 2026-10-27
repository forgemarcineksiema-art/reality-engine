"""Regression cases for the documentation validator, not the game engine."""

from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))
from check_docs import validate


class DocumentationValidationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(prefix="genesis-docs-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "docs/archive").mkdir(parents=True)
        self.write("README.md", "# Project\n\n[Rules](engineering-rules.md)\n")
        self.write("AGENTS.md", "# Repository Guidelines\n")
        self.write("engineering-rules.md", "# Rules\n\n### INV-01 — Identity\n")
        self.archive = "# Original\nUntouched source.\n"
        self.write("docs/archive/original.md", self.archive)
        self.archive_manifest = {
            "version": 1,
            "files": [
                {
                    "path": "docs/archive/original.md",
                    "sha256": hashlib.sha256(self.archive.encode()).hexdigest(),
                }
            ],
        }
        self.manifest = {
            "version": 1,
            "documents": [
                {"path": path, "role": "normative", "topic": path}
                for path in ("README.md", "AGENTS.md", "engineering-rules.md")
            ],
            "identifiers": [
                {
                    "prefix": "INV",
                    "definition_file": "engineering-rules.md",
                    "pattern": "INV-[0-9]{2}",
                }
            ],
            "archive_manifest": "docs/archive/manifest.json",
        }
        self.save_manifests()

    def write(self, relative: str, text: str) -> None:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8", newline="\n")

    def save_manifests(self) -> None:
        self.write("docs/documentation-map.json", json.dumps(self.manifest))
        self.write("docs/archive/manifest.json", json.dumps(self.archive_manifest))

    def test_valid_workspace(self) -> None:
        self.assertEqual(validate(self.root), [])

    def test_rejects_missing_local_link(self) -> None:
        self.write("README.md", "# Project\n[Missing](missing.md)\n")
        self.assertTrue(any("missing local link" in x for x in validate(self.root)))

    def test_rejects_missing_anchor(self) -> None:
        self.write("README.md", "# Project\n[Bad](engineering-rules.md#missing)\n")
        self.assertTrue(any("missing anchor" in x for x in validate(self.root)))

    def test_rejects_unvalidated_reference_links(self) -> None:
        self.write("README.md", "# Project\n[Missing][nope]\n\n[nope]: missing.md\n")
        self.assertTrue(any("unsupported reference link" in x for x in validate(self.root)))

    def test_rejects_unvalidated_html_links(self) -> None:
        self.write("README.md", '# Project\n<a href="missing.md">Missing</a>\n')
        self.assertTrue(any("unsupported HTML link" in x for x in validate(self.root)))

    def test_accepts_polish_anchor_and_ignores_fenced_example(self) -> None:
        self.write(
            "README.md",
            "# Źródła i reguły\n[Here](#źródła-i-reguły)\n"
            "```markdown\n[Example](absent.md) INV-99\n```\n",
        )
        self.assertEqual(validate(self.root), [])

    def test_rejects_unknown_identifier(self) -> None:
        self.write("README.md", "# Project\nRequires INV-99.\n")
        self.assertTrue(any("undefined identifier: INV-99" in x for x in validate(self.root)))

    def test_external_filename_is_not_a_contract_reference(self) -> None:
        self.write("README.md", "# Project\n[Paper](https://example.org/INV-99x.pdf)\n")
        self.assertEqual(validate(self.root), [])

    def test_link_label_still_checks_contract_reference(self) -> None:
        self.write("README.md", "# Project\n[INV-99](https://example.org/paper.pdf)\n")
        self.assertTrue(any("undefined identifier: INV-99" in x for x in validate(self.root)))

    def test_rejects_malformed_identifier(self) -> None:
        self.write("README.md", "# Project\nRequires INV-99x.\n")
        self.assertTrue(any("undefined identifier: INV-99x" in x for x in validate(self.root)))

    def test_rejects_misplaced_identifier_definition(self) -> None:
        self.write("README.md", "# Project\n### INV-01 — Shadow contract\n")
        self.assertTrue(any("identifier defined outside owner" in x for x in validate(self.root)))

    def test_rejects_empty_identifier_groups(self) -> None:
        self.manifest["identifiers"] = []
        self.save_manifests()
        self.assertTrue(any("identifiers must not be empty" in x for x in validate(self.root)))

    def test_rejects_duplicate_identifier_definition(self) -> None:
        self.write("engineering-rules.md", "# Rules\n### INV-01 — A\n### INV-01 — B\n")
        self.assertTrue(any("duplicate identifier" in x for x in validate(self.root)))

    def test_rejects_unregistered_document(self) -> None:
        self.write("untracked.md", "# Surprise\n")
        self.assertTrue(any("unregistered Markdown" in x for x in validate(self.root)))

    def test_rejects_duplicate_topic_owner(self) -> None:
        self.manifest["documents"][1]["topic"] = "README.md"
        self.save_manifests()
        self.assertTrue(any("duplicate topic owner" in x for x in validate(self.root)))

    def test_rejects_noncanonical_document_alias(self) -> None:
        self.manifest["documents"].append(
            {"path": "./README.md", "role": "normative", "topic": "alias"}
        )
        self.save_manifests()
        self.assertTrue(any("non-canonical repository path" in x for x in validate(self.root)))

    def test_rejects_unknown_document_role(self) -> None:
        self.manifest["documents"][0]["role"] = "made-up-state"
        self.save_manifests()
        self.assertTrue(any("unknown document role" in x for x in validate(self.root)))

    def test_rejects_link_escaping_workspace(self) -> None:
        self.write("README.md", "# Project\n[Outside](../outside.md)\n")
        self.assertTrue(any("link escapes repository" in x for x in validate(self.root)))

    def test_rejects_registry_path_escaping_workspace(self) -> None:
        self.manifest["documents"][0]["path"] = "../outside.md"
        self.save_manifests()
        self.assertTrue(any("path escapes repository" in x for x in validate(self.root)))

    def test_detects_archive_tampering(self) -> None:
        self.write("docs/archive/original.md", "# Changed\n")
        self.assertTrue(any("archive checksum mismatch" in x for x in validate(self.root)))

    def test_archive_checksum_preserves_raw_line_endings(self) -> None:
        raw = b"# Original\r\nUntouched source.\r\n"
        (self.root / "docs/archive/original.md").write_bytes(raw)
        self.archive_manifest["files"][0]["sha256"] = hashlib.sha256(raw).hexdigest()
        self.save_manifests()
        self.assertEqual(validate(self.root), [])
        self.write("docs/archive/original.md", self.archive)
        self.assertTrue(any("archive checksum mismatch" in x for x in validate(self.root)))

    def test_cli_returns_failure_and_does_not_claim_engine_tests(self) -> None:
        self.write("README.md", "# Project\n[Missing](missing.md)\n")
        completed = subprocess.run(
            [sys.executable, str(TOOLS / "check_docs.py"), "--root", str(self.root)],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(completed.returncode, 1)
        self.assertIn("missing local link", completed.stdout)
        self.assertNotIn("PASS:", completed.stdout)

    def test_cli_success_discloses_scope(self) -> None:
        completed = subprocess.run(
            [sys.executable, str(TOOLS / "check_docs.py"), "--root", str(self.root)],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)
        self.assertIn("engine behavior were not tested", completed.stdout)


if __name__ == "__main__":
    unittest.main()
