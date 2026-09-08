import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile


SCRIPT = Path(__file__).parents[1] / "prepare-prebuilt-release.py"
SPEC = importlib.util.spec_from_file_location("prepare_prebuilt_release", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


class PreparePrebuiltReleaseTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.android = self.root / "Android"
        (self.android / "app/src/main/assets").mkdir(parents=True)
        (self.android / "app/src/main/assets/gamecontrollerdb.txt").write_text("current")

    def tearDown(self):
        self.temporary.cleanup()

    def apk(self, entries, name="soh-v9.2.3-p7.apk"):
        path = self.root / name
        with zipfile.ZipFile(path, "w") as archive:
            for entry, contents in entries:
                archive.writestr(entry, contents)
        return path

    def required(self):
        return [
            ("assets/soh.o2r", b"assets"),
            ("lib/arm64-v8a/libSDL2.so", b"sdl"),
            ("lib/arm64-v8a/libsoh.so", b"soh"),
        ]

    def test_extracts_assets_and_only_arm64_libraries(self):
        entries = self.required() + [
            ("assets/networking/", b""),
            ("assets/networking/cacert.pem", b"cert"),
            ("lib/x86_64/libsoh.so", b"wrong abi"),
            ("classes.dex", b"ignored"),
        ]
        MODULE.prepare(self.apk(entries), "9.2.3-p7", self.android)

        self.assertEqual(
            (self.android / "app/src/main/jniLibs/arm64-v8a/libsoh.so").read_bytes(), b"soh"
        )
        self.assertFalse((self.android / "app/src/main/jniLibs/x86_64/libsoh.so").exists())
        self.assertEqual(
            (self.android / "app/src/main/assets/gamecontrollerdb.txt").read_text(), "current"
        )
        self.assertTrue((self.android / "app/src/main/assets/networking").is_dir())

    def test_rejects_traversal_before_writing(self):
        entries = self.required() + [("assets/../../escaped", b"bad")]
        with self.assertRaisesRegex(MODULE.InvalidApk, "unsafe ZIP entry"):
            MODULE.prepare(self.apk(entries), "9.2.3-p7", self.android)
        self.assertFalse((self.android / "app/src/main/assets/soh.o2r").exists())

    def test_rejects_empty_normalized_path(self):
        entries = self.required() + [(".", b"bad")]
        with self.assertRaisesRegex(MODULE.InvalidApk, "unsafe ZIP entry"):
            MODULE.prepare(self.apk(entries), "9.2.3-p7", self.android)

    def test_rejects_duplicate_entries(self):
        entries = self.required() + [("assets/soh.o2r", b"duplicate")]
        with self.assertWarns(UserWarning):
            apk = self.apk(entries)
        with self.assertRaisesRegex(MODULE.InvalidApk, "duplicate ZIP entry"):
            MODULE.prepare(apk, "9.2.3-p7", self.android)

    def test_rejects_missing_required_file(self):
        entries = [entry for entry in self.required() if entry[0] != "lib/arm64-v8a/libsoh.so"]
        with self.assertRaisesRegex(MODULE.InvalidApk, "libsoh.so"):
            MODULE.prepare(self.apk(entries), "9.2.3-p7", self.android)


if __name__ == "__main__":
    unittest.main()
