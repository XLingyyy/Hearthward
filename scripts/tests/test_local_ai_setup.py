import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile

spec = importlib.util.spec_from_file_location("local_ai_setup", Path(__file__).parents[1] / "local_ai/prepare_bundle.py")
setup = importlib.util.module_from_spec(spec)
spec.loader.exec_module(setup)


class BundleExtractionTests(unittest.TestCase):
    def test_preserves_runtime_and_extensionless_license(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / "runtime.zip"
            with zipfile.ZipFile(archive, "w") as z:
                z.writestr("release/llama-server.exe", b"test executable")
                z.writestr("release/libomp140.x86_64.dll", b"test runtime")
                z.writestr("release/LICENSE-LLVM-OpenMP", b"test license")
            setup.extract_runtime(archive, root / "bundle")
            self.assertEqual((root / "bundle/LICENSE-LLVM-OpenMP").read_bytes(), b"test license")
            self.assertTrue((root / "bundle/libomp140.x86_64.dll").exists())

    def test_rejects_parent_path(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / "runtime.zip"
            with zipfile.ZipFile(archive, "w") as z:
                z.writestr("../outside.dll", b"test")
            with self.assertRaises(ValueError):
                setup.extract_runtime(archive, root / "bundle")
            self.assertFalse((root / "outside.dll").exists())
