import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('engine_gate', Path(__file__).resolve().parents[1] / 'qa/run_engine_gate.py')
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)


class EngineReportTests(unittest.TestCase):
    def check(self, data):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'index.json'
            path.write_text(json.dumps(data), encoding='utf-8-sig')
            return gate.check_report(path)

    def test_empty_report_cannot_pass(self):
        with self.assertRaises(RuntimeError):
            self.check({'tests': [], 'succeeded': 0, 'failed': 0})

    def test_incomplete_or_failed_test_cannot_pass(self):
        for state in ('Fail', 'NotRun', 'InProcess'):
            with self.subTest(state=state), self.assertRaises(RuntimeError):
                self.check({'tests': [{'state': 'Success'}, {'state': state}]})

    def test_warnings_remain_visible(self):
        result = self.check({'tests': [{'state': 'Success'}], 'succeededWithWarnings': 1})
        self.assertEqual(result, {'passed': 1, 'warnings': 1})


if __name__ == '__main__':
    unittest.main()
