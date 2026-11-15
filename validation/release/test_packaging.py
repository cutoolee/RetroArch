import importlib.util
import json
from pathlib import Path
import subprocess
import tarfile
import tempfile
import types
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("gamego_release", ROOT / "tools/gamego-release.py")
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)


class PackagingTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="gamego-package-test-")
        self.root = Path(self.temp.name)
        self.old_root, self.old_defaults = release.ROOT, release.DEFAULTS
        release.ROOT = self.root
        release.DEFAULTS = self.root / "defaults"
        files = {
            "GAMEGO_VERSION": "0.1.0-beta.1\n", "version.all": '#define PACKAGE_VERSION "1.22.2"\n',
            "COPYING": "GPLv3", "THIRD_PARTY_NOTICES.md": "Attribution",
            "docs/release/install.md": "Install", "pkg/platform/project.txt": "Build project",
            ".github/workflows/build.yml": "name: build\n",
            "handheld/distribution/resources.lock": json.dumps({"version": "pinned", "expected_sha256": "abc"}),
            "defaults/icons/monitor.png": "icon", "defaults/shaders/gamego/test.glsl": "shader",
            "defaults/presets/global.glslp": "preset", "frontend/ozone/regular.ttf": "font",
            "frontend/pkg/chinese-fallback-font.ttf": "font", "frontend/COPYING": "CC-BY-4.0",
            "retroarch": "executable",
        }
        for name, content in files.items():
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content)
        subprocess.run(["git", "init", "-q", str(self.root)], check=True)
        subprocess.run(["git", "-C", str(self.root), "add", "."], check=True)
        subprocess.run(["git", "-C", str(self.root), "-c", "user.name=Test", "-c",
                        "user.email=test@example.invalid", "commit", "-qm", "fixture"], check=True)

    def tearDown(self):
        release.ROOT, release.DEFAULTS = self.old_root, self.old_defaults
        self.temp.cleanup()

    def test_source_keeps_platform_projects_and_hidden_build_files(self):
        output = self.root.parent / (self.root.name + "-source")
        try:
            release.source(types.SimpleNamespace(output=output))
            with tarfile.open(next(output.glob("*.tar.gz"))) as archive:
                names = archive.getnames()
                self.assertIn("RetroArch-0.1.0-beta.1/pkg/platform/project.txt", names)
                self.assertIn("RetroArch-0.1.0-beta.1/.github/workflows/build.yml", names)
        finally:
            import shutil
            shutil.rmtree(output, ignore_errors=True)

    def test_dirty_source_is_rejected(self):
        (self.root / "COPYING").write_text("changed")
        with self.assertRaises(SystemExit):
            release.source(types.SimpleNamespace(output=self.root / "out"))

    def test_android_version_codes_increase_across_release_stages(self):
        values = []
        for version in ("0.1.0-alpha.1", "0.1.0-beta.1", "0.1.0-beta.2", "0.1.0-rc.1", "0.1.0", "0.1.1-alpha.1"):
            (self.root / "GAMEGO_VERSION").write_text(version)
            values.append(release.version_code())
        self.assertEqual(values, sorted(set(values)))

    def test_android_version_code_rejects_ambiguous_prereleases(self):
        for version in ("0.1.0-beta.20", "0.1.0-dev", "2001.0.0"):
            (self.root / "GAMEGO_VERSION").write_text(version)
            with self.assertRaises(SystemExit):
                release.version_code()

    def test_portable_bundle_preserves_resources_and_notices(self):
        output = self.root / "out"
        release.stage(types.SimpleNamespace(output=output, binary=self.root / "retroarch",
                                           assets=self.root / "frontend", platform="linux-x64"))
        self.assertTrue((output / "assets/gamego/icons/monitor.png").is_file())
        self.assertTrue((output / "assets/COPYING").is_file())
        self.assertTrue((output / "shaders/gamego/test.glsl").is_file())
        self.assertTrue((output / "docs/install.md").is_file())
        self.assertIn('--config "$base/gamego.cfg"', (output / "RetroArch").read_text())
        build = json.loads((output / "BUILD.json").read_text())
        self.assertEqual(build["product"], "RetroArch")
        self.assertEqual(build["upstream_version"], "1.22.2")
        self.assertEqual(build["bundled_cores"], [])
        self.assertEqual(build["runtime_acceptance"], "not-verified-by-packaging")

    def test_incomplete_assets_are_rejected(self):
        (self.root / "frontend/pkg/chinese-fallback-font.ttf").unlink()
        with self.assertRaises(SystemExit):
            release.assets(self.root / "frontend", self.root / "out")

    def test_apple_archive_uses_native_extraction_layout(self):
        output = self.root / "out"
        release.apple_assets(types.SimpleNamespace(output=output, assets=self.root / "frontend", platform="ios-arm64"))
        with zipfile.ZipFile(output / "assets.zip") as archive:
            self.assertIn("assets/gamego/icons/monitor.png", archive.namelist())
            self.assertIn("shaders/gamego/test.glsl", archive.namelist())
            self.assertNotIn("gamego.cfg", archive.namelist())

    def test_checksums_cover_deliverables_without_self_reference(self):
        output = self.root / "out"
        output.mkdir()
        (output / "file.apk").write_bytes(b"example")
        release.checksums(types.SimpleNamespace(output=output))
        first = (output / "SHA256SUMS").read_text()
        release.checksums(types.SimpleNamespace(output=output))
        self.assertEqual(first, (output / "SHA256SUMS").read_text())
        self.assertEqual(len(first.splitlines()), 1)
        self.assertTrue(first.endswith("  file.apk\n"))


if __name__ == "__main__":
    unittest.main()
