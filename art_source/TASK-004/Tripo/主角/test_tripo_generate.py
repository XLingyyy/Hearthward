import tempfile
import unittest
from pathlib import Path

import tripo_generate


class TripoGenerateTests(unittest.TestCase):
    def parse(self, *arguments: str):
        return tripo_generate.build_parser().parse_args(list(arguments))

    def test_text_dry_run_summary(self):
        args = self.parse("--dry-run", "text", "--prompt", "game-ready hero")
        summary = tripo_generate.build_request_summary(args)
        self.assertEqual(summary["command"], "text")
        self.assertEqual(summary["prompt"], "game-ready hero")
        self.assertEqual(summary["model_version"], tripo_generate.DEFAULT_MODEL)
        self.assertEqual(summary["face_limit"], 80_000)
        self.assertTrue(summary["texture"])
        self.assertTrue(summary["pbr"])

    def test_prompt_file_is_utf8_and_trimmed(self):
        with tempfile.TemporaryDirectory() as directory:
            prompt_path = Path(directory) / "prompt.txt"
            prompt_path.write_text("  主角模型  \n", encoding="utf-8")
            args = self.parse("text", "--prompt-file", str(prompt_path))
            self.assertEqual(tripo_generate.build_request_summary(args)["prompt"], "主角模型")

    def test_multiview_order_is_fixed(self):
        with tempfile.TemporaryDirectory() as directory:
            paths = []
            for name in ("front.png", "left.png", "back.png", "right.png"):
                path = Path(directory) / name
                path.touch()
                paths.append(path)
            args = self.parse(
                "multiview",
                "--front",
                str(paths[0]),
                "--left",
                str(paths[1]),
                "--back",
                str(paths[2]),
                "--right",
                str(paths[3]),
            )
            summary = tripo_generate.build_request_summary(args)
            self.assertEqual(summary["image_order"], ["front", "left", "back", "right"])
            self.assertEqual(summary["images"], [str(path.resolve()) for path in paths])

    def test_prompt_limit(self):
        args = self.parse("text", "--prompt", "x" * 1025)
        with self.assertRaisesRegex(ValueError, "1024"):
            tripo_generate.build_request_summary(args)

    def test_p1_rejects_default_face_limit(self):
        args = self.parse(
            "text", "--prompt", "hero", "--model", "P1-20260311"
        )
        with self.assertRaisesRegex(ValueError, "20000"):
            tripo_generate.build_request_summary(args)

    def test_p1_omits_unsupported_texture_quality(self):
        args = self.parse(
            "text",
            "--prompt",
            "hero",
            "--model",
            "P1-20260311",
            "--face-limit",
            "10000",
        )
        summary = tripo_generate.build_request_summary(args)
        self.assertNotIn("texture_quality", summary)

    def test_smart_low_poly_enforces_its_face_limit(self):
        args = self.parse("text", "--prompt", "hero", "--smart-low-poly")
        with self.assertRaisesRegex(ValueError, "20000"):
            tripo_generate.build_request_summary(args)

    def test_task_record_can_be_loaded_for_resume(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            original = {"task_id": "task-1", "request": {"command": "image"}}
            tripo_generate._write_record(folder, original)
            self.assertEqual(tripo_generate._load_record(folder), original)


if __name__ == "__main__":
    unittest.main()
