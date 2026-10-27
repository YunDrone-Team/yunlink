"""Check release wheel version validation without building native binaries."""

import importlib.util
from pathlib import Path
import tempfile
import unittest
from zipfile import ZipFile


SPEC = importlib.util.spec_from_file_location(
    "verify_python_wheels",
    Path(__file__).resolve().parents[1] / "tools/bindings/verify_python_wheels.py",
)
verifier = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(verifier)


class WheelVersions(unittest.TestCase):
    def test_matrix_and_version_mismatches(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for platform, suffix in verifier.PLATFORM_SUFFIXES.items():
                artifact = root / f"wheels-{platform}"
                artifact.mkdir()
                tag = (
                    f"macosx_11_0{suffix}" if platform.startswith("macos-")
                    else f"manylinux_2_28{suffix}" if platform.startswith("linux-")
                    else suffix
                )
                for python in verifier.PYTHON_TAGS:
                    wheel = artifact / f"yunlink-2.0.3-{python}-{python}-{tag}.whl"
                    with ZipFile(wheel, "w") as archive:
                        archive.writestr(
                            "yunlink-2.0.3.dist-info/METADATA", "Name: yunlink\nVersion: 2.0.3\n"
                        )
            self.assertEqual(verifier.validate(root, "2.0.3"), [])
            self.assertTrue(verifier.validate(root, "2.0.2"))
            wheel = next(root.rglob("*.whl"))
            with ZipFile(wheel, "w") as archive:
                archive.writestr(
                    "yunlink-2.0.3.dist-info/METADATA", "Name: yunlink\nVersion: 2.0.1\n"
                )
            self.assertTrue(any("metadata" in error for error in verifier.validate(root, "2.0.3")))


if __name__ == "__main__":
    unittest.main()
