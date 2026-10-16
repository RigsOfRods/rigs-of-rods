"""Exercise resource ZIP generation through the production CMake/Ninja build graph.

Run with: python3 tests/cmake/test_resource_archives.py

Inventory changes are discovered by explicit reconfiguration. Addition/removal
use normal file dates, with a separate backdated addition to check that inventory
changes invalidate the archive independently of source-file modification times.
"""
import io
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import zipfile


MACROS = Path(__file__).resolve().parents[2] / "cmake/Macros.cmake"


@unittest.skipUnless(shutil.which("cmake") and shutil.which("ninja"),
                     "Resource archive tests require CMake and Ninja")
class ResourceArchiveTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="ror-resource-archive-")
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.source = self.root / "source"
        self.build_dir = self.root / "build"
        self.pack = self.source / "resources/materials"
        self.pack.mkdir(parents=True)
        (self.source / "CMakeLists.txt").write_text(
            'cmake_minimum_required(VERSION 3.16)\n'
            'project(ResourceArchiveFixture LANGUAGES NONE)\n'
            f'include("{MACROS.as_posix()}")\n'
            'recursive_zip_folder("${CMAKE_SOURCE_DIR}/resources" '
            '"${CMAKE_BINARY_DIR}/archives")\n'
        )
        self.archive = self.build_dir / "archives/materials.zip"

    def run_command(self, *args):
        result = subprocess.run(args, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result.stdout

    def configure(self):
        self.run_command("cmake", "-S", str(self.source), "-B", str(self.build_dir),
                         "-G", "Ninja")

    def build(self):
        return self.run_command("cmake", "--build", str(self.build_dir))

    def write_resource(self, name, content=b"original resource\n"):
        path = self.pack / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)
        return path

    def names(self):
        with zipfile.ZipFile(self.archive) as archive:
            return archive.namelist()

    def test_build_path_with_spaces_passes_archive_filelist_correctly(self):
        self.build_dir = self.root / "build with spaces"
        self.archive = self.build_dir / "archives/materials.zip"
        self.write_resource("shaders/native/base.glsl")
        self.configure()
        self.build()
        self.assertIn("shaders/native/base.glsl", self.names())

    def test_backdated_nested_addition_is_archived_after_reconfigure(self):
        self.write_resource("shaders/native/base.glsl")
        self.configure()
        self.build()
        self.assertIn("shaders/native/base.glsl", self.names())
        added = self.write_resource("shaders/native/added.glsl", b"added shader\n")
        # Model an added file carrying an older timestamp (e.g. from an archive).
        # Its mtime alone must not be needed to invalidate the existing ZIP.
        old_time = self.archive.stat().st_mtime - 60
        os.utime(added, (old_time, old_time))
        self.configure()
        self.build()
        self.assertIn("shaders/native/added.glsl", self.names())
        self.assertIn("shaders/native/base.glsl", self.names())
        with zipfile.ZipFile(self.archive) as archive:
            self.assertEqual(archive.read("shaders/native/added.glsl"), b"added shader\n")

    def test_normaldated_nested_addition_is_archived_after_reconfigure(self):
        self.write_resource("shaders/native/base.glsl")
        self.configure()
        self.build()
        self.assertIn("shaders/native/base.glsl", self.names())
        self.write_resource("shaders/native/added.glsl", b"added shader\n")
        self.configure()
        self.build()
        self.assertIn("shaders/native/added.glsl", self.names())
        self.assertIn("shaders/native/base.glsl", self.names())
        with zipfile.ZipFile(self.archive) as archive:
            self.assertEqual(archive.read("shaders/native/added.glsl"), b"added shader\n")

    def test_normaldated_removed_nested_file_is_excluded_after_reconfigure(self):
        self.write_resource("shaders/native/base.glsl")
        removed = self.write_resource("shaders/native/removed.glsl")
        self.configure()
        self.build()
        self.assertIn("shaders/native/removed.glsl", self.names())
        removed.unlink()
        self.configure()
        self.build()
        self.assertNotIn("shaders/native/removed.glsl", self.names())
        self.assertIn("shaders/native/base.glsl", self.names())

    def test_nested_entries_are_unique_and_keep_relative_paths(self):
        self.write_resource("shaders/native/base.glsl")
        self.write_resource("root.material")
        (self.pack / "shaders/empty/deep").mkdir(parents=True)
        self.configure()
        self.build()
        names = self.names()
        self.assertEqual(len(names), len(set(names)), names)
        self.assertEqual(set(names), {
            "root.material", "shaders/", "shaders/native/",
            "shaders/native/base.glsl", "shaders/empty/", "shaders/empty/deep/",
        })

    def test_unchanged_build_and_reconfigure_do_not_regenerate_archives(self):
        self.write_resource("shaders/native/base.glsl")
        self.configure()
        self.build()
        original = self.archive.read_bytes()
        mtime = self.archive.stat().st_mtime_ns
        self.build()
        self.assertEqual(self.archive.stat().st_mtime_ns, mtime)
        self.configure()
        self.build()
        self.assertEqual(self.archive.stat().st_mtime_ns, mtime)
        self.assertEqual(self.archive.read_bytes(), original)

    def test_empty_directory_inventory_changes_regenerate_only_affected_archive(self):
        self.write_resource("shaders/native/base.glsl")
        other = self.source / "resources/textures"
        other.mkdir()
        (other / "texture.dds").write_bytes(b"DDS fixture")
        self.configure()
        self.build()
        other_archive = self.build_dir / "archives/textures.zip"
        other_mtime = other_archive.stat().st_mtime_ns
        empty = self.pack / "shaders/native/empty"
        empty.mkdir()
        self.configure()
        self.build()
        self.assertIn("shaders/native/empty/", self.names())
        empty.rmdir()
        self.configure()
        self.build()
        self.assertNotIn("shaders/native/empty/", self.names())
        self.assertEqual(other_archive.stat().st_mtime_ns, other_mtime)

    def test_source_updates_and_inner_zip_bytes_are_preserved(self):
        nested = io.BytesIO()
        with zipfile.ZipFile(nested, "w") as archive:
            archive.writestr("vehicle.truck", b"original vehicle\n")
        self.write_resource("mods/original.zip", nested.getvalue())
        shader = self.write_resource("shaders/native/base.glsl")
        self.configure()
        self.build()
        shader.write_bytes(b"updated shader\n")
        self.build()
        with zipfile.ZipFile(self.archive) as archive:
            self.assertEqual(archive.read("shaders/native/base.glsl"), b"updated shader\n")
            self.assertEqual(archive.read("mods/original.zip"), nested.getvalue())


if __name__ == "__main__":
    unittest.main()
