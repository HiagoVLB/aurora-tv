"""Version edits and package replacement tested without a TV, SDK or network."""

import io
import json
from pathlib import Path
import runpy
import shutil
import subprocess
import sys
import tarfile
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
BUILD = runpy.run_path(str(ROOT / "scripts/webos/build_ipk.py"))
VERSION = runpy.run_path(str(ROOT / "scripts/dev/aurora_version.py"))


def tar_bytes(entries):
    output = io.BytesIO()
    with tarfile.open(fileobj=output, mode="w:gz") as archive:
        for name, data in entries.items():
            member = tarfile.TarInfo(name)
            member.size = len(data)
            archive.addfile(member, io.BytesIO(data))
    return output.getvalue()


def fixture_ipk(path, version="1.3.1", app_version=None, machine=40, tag_version=None, reset_marker=b"aurora-log-reset-v1\n"):
    app = {"id": BUILD["APP_ID"], "version": app_version or version, "main": "bin/aurora"}
    elf = bytearray(52)
    elf[:6] = b"\x7fELF\x01\x01"
    elf[18:20] = machine.to_bytes(2, "little")
    elf.extend(f"aurora-v{tag_version or version}\0".encode())
    prefix = f'./usr/palm/applications/{BUILD["APP_ID"]}/'
    data_entries = {prefix + "appinfo.json": json.dumps(app).encode(), prefix + "bin/aurora": bytes(elf)}
    if reset_marker is not None:
        data_entries[prefix + "aurora-logs-reset.pending"] = reset_marker
    members = {
        "debian-binary": b"2.0\n",
        "control.tar.gz": tar_bytes({"./control":
            f'Package: {BUILD["APP_ID"]}\nVersion: {version}\nArchitecture: arm\n'.encode()}),
        "data.tar.gz": tar_bytes(data_entries),
    }
    with path.open("wb") as output:
        output.write(b"!<arch>\n")
        for name, data in members.items():
            header = f"{name + '/':<16}{0:<12}{0:<6}{0:<6}{'100644':<8}{len(data):<10}`\n"
            output.write(header.encode())
            output.write(data)
            if len(data) % 2:
                output.write(b"\n")


class ReleaseHelpers(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="aurora-release-test-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def version_fixture(self):
        for name in ("CMakeLists.txt", "deploy/webos/appinfo.json", "src/app/stream/session_worker.c"):
            target = self.root / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, target)

    def cli_version(self, *args):
        return subprocess.run([sys.executable, str(ROOT / "scripts/dev/aurora_version.py"),
                               "--root", str(self.root), *args], capture_output=True, text=True)

    def test_preview_and_check_do_not_edit_sources(self):
        self.version_fixture()
        original = (self.root / "CMakeLists.txt").read_bytes()
        self.assertEqual(self.cli_version().returncode, 0)
        self.assertEqual(self.cli_version("--check").returncode, 0)
        self.assertEqual((self.root / "CMakeLists.txt").read_bytes(), original)

    def test_apply_changes_only_central_version_and_remains_consistent(self):
        self.version_fixture()
        before = {str(p.relative_to(self.root)): p.read_bytes() for p in self.root.rglob("*") if p.is_file()}
        current = VERSION["check_contract"](self.root)
        self.assertEqual(self.cli_version("--part", "patch", "--apply").returncode, 0)
        self.assertEqual(VERSION["check_contract"](self.root), VERSION["next_version"](current))
        self.assertEqual([name for name, data in before.items() if (self.root / name).read_bytes() != data],
                         ["CMakeLists.txt"])

    def test_minor_major_and_monotonic_explicit_versions(self):
        self.assertEqual(VERSION["next_version"]("1.3.9", "minor"), "1.4.0")
        self.assertEqual(VERSION["next_version"]("1.3.9", "major"), "2.0.0")
        self.assertEqual(VERSION["next_version"]("1.3.9", explicit="1.9.0"), "1.9.0")
        for invalid in ("1.3.9", "1.2.0", "v2.0.0", "2.0.0-beta", "02.0.0"):
            with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                VERSION["next_version"]("1.3.9", explicit=invalid)

    def test_invalid_contract_does_not_apply_version(self):
        self.version_fixture()
        original = (self.root / "CMakeLists.txt").read_bytes()
        manifest = self.root / "deploy/webos/appinfo.json"
        manifest.write_text(manifest.read_text().replace("@PROJECT_VERSION@", "0.0.1"))
        self.assertNotEqual(self.cli_version("--apply").returncode, 0)
        self.assertEqual((self.root / "CMakeLists.txt").read_bytes(), original)

    def test_package_control_appinfo_elf_and_tag_agree(self):
        package = self.root / "fixture.ipk"
        fixture_ipk(package)
        metadata = BUILD["verify_ipk"](package, "1.3.1")
        self.assertTrue(metadata["arm_elf_verified"])
        self.assertTrue(metadata["installation_log_reset_verified"])
        self.assertEqual(metadata["architecture"], "arm")

    def test_reject_wrong_package_version_metadata_architecture_and_tag(self):
        package = self.root / "fixture.ipk"
        for kwargs in ({"version": "1.3.0"}, {"app_version": "1.3.0"},
                       {"machine": 62}, {"tag_version": "1.3.0"}):
            with self.subTest(kwargs=kwargs):
                fixture_ipk(package, **kwargs)
                with self.assertRaises(ValueError):
                    BUILD["verify_ipk"](package, "1.3.1")

    def test_failed_validation_preserves_last_installer(self):
        previous = self.root / "aurora.ipk"
        previous.write_bytes(b"previous verified installer")
        package = self.root / "fixture.ipk"
        fixture_ipk(package, machine=62)
        with self.assertRaises(ValueError):
            BUILD["publish"](package, self.root, "1.3.1")
        self.assertEqual(previous.read_bytes(), b"previous verified installer")

    def test_missing_or_invalid_reset_marker_preserves_last_installer(self):
        previous = self.root / "aurora.ipk"
        previous.write_bytes(b"previous verified installer")
        package = self.root / "fixture.ipk"
        for marker in (None, b"bad-marker\n"):
            with self.subTest(marker=marker):
                fixture_ipk(package, reset_marker=marker)
                with self.assertRaises(ValueError):
                    BUILD["publish"](package, self.root, "1.3.1")
                self.assertEqual(previous.read_bytes(), b"previous verified installer")

    def test_success_replaces_same_filename_and_truncated_archive_is_rejected(self):
        package = self.root / "fixture.ipk"
        fixture_ipk(package)
        target, _ = BUILD["publish"](package, self.root, "1.3.1")
        self.assertEqual(target, self.root / "aurora.ipk")
        self.assertEqual(target.read_bytes(), package.read_bytes())
        fixture_ipk(package, version="1.3.2")
        BUILD["publish"](package, self.root, "1.3.2")
        self.assertEqual(target.read_bytes(), package.read_bytes())
        self.assertEqual(list(self.root.glob(".aurora-ipk-*")), [])
        package.write_bytes(package.read_bytes()[:-17])
        with self.assertRaises(ValueError):
            BUILD["verify_ipk"](package, "1.3.2")


if __name__ == "__main__":
    unittest.main()
