from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[1] / "qa/validate_catalog_ids.py"
SPEC = importlib.util.spec_from_file_location("hearthward_catalog_ids", TOOL)
assert SPEC and SPEC.loader
catalog = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(catalog)


def fixture():
    root = {}
    for path in catalog.TABLE_PATHS:
        parent = root
        for key in path[:-1]:
            parent = parent.setdefault(key, {})
        parent[path[-1]] = [{"id": "row_01"}, {"id": "row_02"}]
    return root


class CatalogIdValidationTests(unittest.TestCase):
    def test_valid_catalog(self):
        report = catalog.validate_catalog(fixture())
        self.assertEqual(report["status"], "PASS")
        self.assertEqual(len(report["table_rows"]), 12)

    def test_same_id_in_different_tables_allowed(self):
        self.assertEqual(catalog.validate_catalog(fixture())["errors"], [])

    def test_duplicate_in_same_table_rejected(self):
        root = fixture(); root["items"].append({"id": "row_01"})
        self.assertIn("duplicate id", " ".join(catalog.validate_catalog(root)["errors"]))

    def test_missing_id_rejected(self):
        root = fixture(); root["items"].append({"name": "No ID"})
        self.assertEqual(catalog.validate_catalog(root)["status"], "FAIL")

    def test_non_string_ids_rejected(self):
        for value in (None, 1, True, [], {}):
            with self.subTest(value=value):
                root = fixture(); root["items"][0]["id"] = value
                self.assertEqual(catalog.validate_catalog(root)["status"], "FAIL")

    def test_blank_id_rejected(self):
        for value in ("", " ", "\t"):
            root = fixture(); root["items"][0]["id"] = value
            self.assertEqual(catalog.validate_catalog(root)["status"], "FAIL")

    def test_unicode_id_allowed(self):
        root = fixture(); root["items"][0]["id"] = "河口木材"
        self.assertEqual(catalog.validate_catalog(root)["status"], "PASS")

    def test_non_object_row_rejected(self):
        root = fixture(); root["items"].append(7)
        self.assertEqual(catalog.validate_catalog(root)["status"], "FAIL")

    def test_missing_table_rejected(self):
        root = fixture(); del root["skills"]
        self.assertEqual(catalog.validate_catalog(root)["status"], "FAIL")

    def test_non_array_table_rejected(self):
        root = fixture(); root["items"] = {}
        self.assertEqual(catalog.validate_catalog(root)["status"], "FAIL")

    def test_null_table_rejected(self):
        root = fixture(); root["items"] = None
        self.assertEqual(catalog.validate_catalog(root)["status"], "FAIL")

    def test_wrong_nested_parent_rejected(self):
        root = fixture(); root["campaign"] = []
        self.assertEqual(catalog.validate_catalog(root)["status"], "FAIL")

    def test_non_object_root_rejected(self):
        for value in (None, [], "text", 1):
            self.assertEqual(catalog.validate_catalog(value)["status"], "FAIL")

    def test_empty_table_is_structurally_valid(self):
        root = fixture(); root["items"] = []
        self.assertEqual(catalog.validate_catalog(root)["status"], "PASS")

    def test_validation_does_not_modify_data(self):
        root = fixture(); before = copy.deepcopy(root)
        catalog.validate_catalog(root)
        self.assertEqual(root, before)

    def load_text(self, text):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "catalog.json"
            path.write_text(text, encoding="utf-8")
            return catalog.load_catalog(path)

    def test_duplicate_json_key_rejected(self):
        with self.assertRaises(ValueError):
            self.load_text('{"items": [], "items": []}')

    def test_nonfinite_values_rejected(self):
        for token in ("NaN", "Infinity", "-Infinity", "1e999"):
            with self.subTest(token=token), self.assertRaises(ValueError):
                self.load_text('{"value": ' + token + '}')

    def test_utf8_bom_accepted(self):
        self.assertEqual(self.load_text('\ufeff{"value": "中文"}')["value"], "中文")

    def test_cli_success_and_failure_codes(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "catalog.json"
            path.write_text(json.dumps(fixture()), encoding="utf-8")
            result = subprocess.run([sys.executable, str(TOOL), "--catalog", str(path), "--json"],
                                    text=True, capture_output=True, encoding="utf-8", timeout=20)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(json.loads(result.stdout)["status"], "PASS")
            path.write_text("[]", encoding="utf-8")
            result = subprocess.run([sys.executable, str(TOOL), "--catalog", str(path), "--json"],
                                    text=True, capture_output=True, encoding="utf-8", timeout=20)
            self.assertEqual(result.returncode, 1)
            self.assertEqual(json.loads(result.stdout)["status"], "FAIL")

    def test_missing_file_fails_without_traceback(self):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run([sys.executable, str(TOOL), "--catalog", str(Path(directory) / "missing.json"), "--json"],
                                    text=True, capture_output=True, encoding="utf-8", timeout=20)
            self.assertEqual(result.returncode, 1)
            self.assertEqual(json.loads(result.stdout)["status"], "FAIL")
            self.assertNotIn("Traceback", result.stderr)


if __name__ == "__main__":
    unittest.main()
