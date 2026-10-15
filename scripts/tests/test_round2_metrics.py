import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('round2_metrics', Path(__file__).resolve().parents[1] / 'qa/round2/metrics.py')
metrics = importlib.util.module_from_spec(spec)
spec.loader.exec_module(metrics)


class Round2MetricsTests(unittest.TestCase):
    def test_missing_scenes_never_become_pass(self):
        with tempfile.TemporaryDirectory() as root:
            slots = metrics.matrix(root)['slots']
        self.assertEqual(len(slots), 12)
        self.assertTrue(all(row['status'] == 'NOT_RUN' for row in slots))

    def test_complete_frames_keep_single_long_stall(self):
        frames = [10.] * 199 + [1010.]
        result = metrics.legacy.frame_statistics(frames)
        self.assertEqual(result['numeric_frames'], 200)
        self.assertEqual(result['p99_ms'], 10.)
        self.assertAlmostEqual(result['one_percent_low_fps'], 1000 / 510)
        self.assertEqual(result['over_50ms_fraction'], .005)

    def test_no_inference_window_cannot_claim_joint_pass(self):
        with tempfile.TemporaryDirectory() as root:
            csv = Path(root) / 'frames.csv'; log = Path(root) / 'runtime.log'
            csv.write_text('FrameTime\n10\n10\n[HasHeaderRowAtEnd],1,[EventTimestamps],0\n', encoding='utf-8')
            log.write_text('', encoding='utf-8')
            result = metrics.summarize(csv, log)
        self.assertEqual(result['frame_status'], 'PASS')
        self.assertEqual(result['joint_status'], 'NOT_RUN')

    def test_bad_numeric_frame_rejected_not_dropped(self):
        with self.assertRaises(ValueError):
            metrics.legacy.parse_frame_timings('FrameTime\n10\nnan\n[HasHeaderRowAtEnd],1,[EventTimestamps],0\n')

    def test_real_historical_baseline_reproduces_failure(self):
        root = metrics.GAME / 'docs/qa/TASK-102/settled-dialogue-cpu-20261007-01'
        timings, _ = metrics.legacy.parse_frame_timings((root / 'frames.csv').read_text(encoding='utf-8-sig'))
        result = metrics.legacy.frame_statistics(timings)
        self.assertEqual(result['numeric_frames'], 11603)
        self.assertAlmostEqual(result['p99_ms'], 16.5324)
        self.assertLess(result['one_percent_low_fps'], 60)


if __name__ == '__main__':
    unittest.main()
