#!/usr/bin/env python3
"""Headless fixtures for the production generator and its actual generate_version target.

Run with Python 3, CMake >= 3.16, Git and a C++ compiler. No game dependencies are needed.
Use --work-dir to retain all throwaway fixture sources/builds in an explicit directory.
"""
import argparse
import ast
import datetime
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
EPOCH = 946684800
WORK = None
CMAKE = shutil.which("cmake")
GIT = shutil.which("git")
CXX = shutil.which("c++")


def run(args, cwd=None, success=True):
    env = dict(os.environ, SOURCE_DATE_EPOCH=str(EPOCH))
    result = subprocess.run([str(a) for a in args], cwd=cwd, env=env,
                            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if success and result.returncode:
        raise AssertionError(f"Command failed: {args!r}\n{result.stdout}")
    return result


def decode_rc_literal(literal):
    """Validate/decode the narrow RC subset emitted here, not C/C++ string grammar.

    Microsoft STRINGTABLE documents doubled quotes and octal escapes. LLVM's
    ResourceFileWriter::processString implements doubled quotes, escaped backslashes,
    and up to three octal digits for narrow strings. This is a grammar assertion,
    not evidence of compilation by rc.exe/llvm-rc.
    https://learn.microsoft.com/en-us/windows/win32/menurc/stringtable-resource
    https://github.com/llvm/llvm-project/blob/main/llvm/tools/llvm-rc/ResourceFileWriter.cpp
    """
    if not re.fullmatch(r'"(?:""|\\[0-7]{3}|\\\\|[^"\\\x00-\x1f])*"', literal):
        raise ValueError("Invalid literal in the emitted narrow RC grammar")

    def decoded(match):
        token = match[0]
        if token == '""':
            return '"'
        if token == '\\\\':
            return '\\'
        return chr(int(token[1:], 8))

    return re.sub(r'""|\\[0-7]{3}|\\\\', decoded, literal[1:-1])


class VersionInfoTests(unittest.TestCase):
    def setUp(self):
        self.directory = Path(tempfile.mkdtemp(prefix=self._testMethodName + " ", dir=WORK))
        self.source = self.directory / "source with spaces"
        self.source.mkdir()
        (self.source / "tracked.txt").write_text("clean\n")
        run([GIT, "init", "-q", self.source])
        run([GIT, "add", "tracked.txt"], self.source)
        run([GIT, "-c", "user.name=Version Fixture", "-c", "user.email=fixture@example.invalid",
             "commit", "-qm", "fixture"], self.source)
        self.revision = run([GIT, "rev-parse", "HEAD"], self.source).stdout.strip()
        self.short_revision = run([GIT, "rev-parse", "--short", "HEAD"], self.source).stdout.strip()
        self.output = self.directory / "generated with spaces"

    def generate(self, dev=False, custom=None, project="2026.01", git=GIT, source=None,
                 success=True):
        args = [CMAKE, f"-DBUILD_DEV_VERSION={'ON' if dev else 'OFF'}",
                f"-DBUILD_CUSTOM_VERSION={'TRUE' if custom is not None else 'FALSE'}",
                f"-DPROJECT_VERSION={project}", f"-DGIT_EXECUTABLE={git or ''}",
                f"-DVERSION_SOURCE_DIR={source or self.source}",
                f"-DVERSION_INFO_INPUT_DIR={ROOT / 'source/version_info'}",
                f"-DVERSION_INFO_OUTPUT_DIR={self.output}"]
        if custom is not None:
            args.append(f"-DCUSTOM_VERSION={custom}")
        return run(args + ["-P", ROOT / "cmake/GenerateVersionInfo.cmake"], success=success)

    def metadata(self, display, short, custom=None, dev=False, dirty=False,
                 project="2026.01", numeric="2026.1.0", revision=True):
        data = json.loads((self.output / "package-version.json").read_text())
        self.assertEqual(data, {
            "schema": 1, "project_version": project, "display_version": display,
            "short_version": short, "full_revision": self.revision if revision else None,
            "dirty": dirty, "custom_version": custom, "build_dev_version": dev,
            "build_date": datetime.datetime.fromtimestamp(EPOCH, datetime.timezone.utc).strftime("%Y-%m-%d"),
            "build_time": "00:00", "apple_short_version": numeric, "apple_bundle_version": numeric,
        })
        cpp = (self.output / "version_info.cpp").read_text()
        for name, expected in [("ROR_VERSION_STRING", display), ("ROR_VERSION_STRING_SHORT", short),
                               ("ROR_BUILD_DATE", data["build_date"]), ("ROR_BUILD_TIME", data["build_time"])]:
            match = re.search(name + r'\s*=\s*("(?:\\.|[^"\\])*");', cpp)
            self.assertIsNotNone(match, name)
            self.assertEqual(ast.literal_eval(match[1]), expected)
        header = (self.output / "version_def.h").read_text()
        literals = re.findall(r'^#define ROR_RESOURCE_VERSION_STRING (.*)$', header, re.MULTILINE)
        self.assertEqual(len(literals), 2)
        self.assertEqual(decode_rc_literal(literals[0]), display)
        self.assertEqual(ast.literal_eval(literals[1]), display)
        return data

    def test_release_project_version_and_full_revision(self):
        self.generate()
        self.metadata("2026.01", "2026.01")
        self.generate(project="0007.008.009.0010")
        self.metadata("0007.008.009.0010", "0007.008", project="0007.008.009.0010", numeric="7.8.9")
        self.assertIn("#define ROR_RESOURCE_VERSION 7,8", (self.output / "version_def.h").read_text())

    def test_dev_clean_dirty_and_no_git(self):
        self.generate(dev=True)
        self.metadata("2000.01-dev-" + self.short_revision, "2000.01", dev=True)
        (self.source / "tracked.txt").write_text("dirty\n")
        self.generate(dev=True)
        self.metadata("2000.01-dev-" + self.short_revision + "-dirty", "2000.01", dev=True, dirty=True)
        self.generate(dev=True, git=None)
        self.metadata("2000.01-dev-without-git", "2000.01", dev=True, dirty=None, revision=False)
        no_repository = self.directory / "no repository"
        no_repository.mkdir()
        self.generate(dev=True, source=no_repository)
        self.metadata("2000.01-dev-without-git", "2000.01", dev=True, dirty=None, revision=False)

    def test_custom_escaping_precedence_and_cpp_syntax(self):
        custom = '2040.08-quote"-slash\\-space ;-[$<CONFIG>]-' + ''.join(map(chr, range(1, 32))) + '9'
        self.generate(dev=True, custom=custom)
        self.metadata(custom, "2040.08", custom=custom)
        check = self.output / "check.cpp"
        check.write_text('#include "version_def.h"\nconst char* resource = ROR_RESOURCE_VERSION_STRING;\n')
        run([CXX, "-std=c++11", "-fsyntax-only", "-I", ROOT / "source/version_info",
             self.output / "version_info.cpp", check])

    def test_cpp11_trigraph_custom_version(self):
        # Every C++11 trigraph, ending in the quote-escaping regression from review.
        custom = "2040.08-??=??'??(??)??!??<??>??--??/"
        self.generate(custom=custom)
        self.metadata(custom, "2040.08", custom=custom)
        cpp = self.output / "version_info.cpp"
        self.assertNotIn("??", cpp.read_text())
        run([CXX, "-std=c++11", "-fsyntax-only", "-I", ROOT / "source/version_info", cpp])

    def test_rc_literal_grammar_and_quote_regression(self):
        # Fixed primary-source examples validate the grammar checker independently.
        self.assertEqual(decode_rc_literal('"a""b"'), 'a"b')
        self.assertEqual(decode_rc_literal(r'"Line one\012Line two"'), "Line one\nLine two")
        with self.assertRaises(ValueError):
            decode_rc_literal(r'"a\"b"')  # C++ escaping is not RC doubled quoting.
        custom = '2040.08-quote"-slash\\-line\n-tab\t-control\x019'
        self.generate(custom=custom)
        self.metadata(custom, "2040.08", custom=custom)
        header = (self.output / "version_def.h").read_text()
        self.assertIn('#ifdef RC_INVOKED\n#define ROR_RESOURCE_VERSION_STRING '
                      r'"2040.08-quote""-slash\\-line\012-tab\011-control\0019"', header)

    def test_invalid_custom_diagnostic(self):
        for custom in ["", "not a version", "2040"]:
            result = self.generate(custom=custom, success=False)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Version must start with two decimal components", result.stdout)

    def test_legacy_single_template_interface(self):
        output = self.directory / "legacy.cpp"
        run([CMAKE, "-DBUILD_CUSTOM_VERSION=TRUE", "-DCUSTOM_VERSION=2040.09-custom",
             f"-DVERSION_FILE_INPUT={ROOT / 'source/version_info/RoRVersion.cpp.in'}",
             f"-DVERSION_FILE_OUTPUT={output}", "-P", ROOT / "cmake/GenerateVersionInfo.cmake"])
        self.assertIn('"2040.09-custom"', output.read_text())

    def prepare_target(self):
        # Copy the production caller/script/templates unchanged, supplying only a tiny root project.
        shutil.copytree(ROOT / "source/version_info", self.source / "source/version_info")
        (self.source / "cmake").mkdir()
        shutil.copyfile(ROOT / "cmake/GenerateVersionInfo.cmake", self.source / "cmake/GenerateVersionInfo.cmake")
        (self.source / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.16)
project(VersionFixture VERSION 2026.01 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(DEFINED FIXTURE_CUSTOM_FILE)
    file(READ "${FIXTURE_CUSTOM_FILE}" ROR_CUSTOM_VERSION)
endif()
add_subdirectory(source/version_info)
''')
        run([GIT, "add", "."], self.source)
        run([GIT, "-c", "user.name=Version Fixture", "-c", "user.email=fixture@example.invalid",
             "commit", "-qm", "production target fixture"], self.source)
        self.revision = run([GIT, "rev-parse", "HEAD"], self.source).stdout.strip()
        self.short_revision = run([GIT, "rev-parse", "--short", "HEAD"], self.source).stdout.strip()
        self.build = self.directory / "build with spaces"
        self.output = self.build / "source/version_info"

    def configure_target(self, extra=()):
        run([CMAKE, "-S", self.source, "-B", self.build, "-DROR_BUILD_INSTALLER=Off",
             "-DROR_BUILD_DEV_VERSION=ON",
             *(["-G", "Ninja"] if shutil.which("ninja") else []), *extra])

    def build_target(self, target="generate_version"):
        return run([CMAKE, "--build", self.build, "--target", target, "--parallel", "1"])

    def test_actual_target_custom_arguments_and_installer_precedence(self):
        self.prepare_target()
        custom = '2040.08-quote"-slash\\-space ;semicolon;$<CONFIG>-??/'
        self.configure_target([f"-DROR_CUSTOM_VERSION={custom}"])
        self.build_target("version_info")
        self.metadata(custom, "2040.08", custom=custom)
        # Exercise all control characters through the actual production target too.
        custom += ''.join(map(chr, range(1, 32))) + '9'
        custom_file = self.directory / "custom input.txt"
        custom_file.write_text(custom)
        self.configure_target([f"-DFIXTURE_CUSTOM_FILE={custom_file}"])
        self.build_target("version_info")
        self.metadata(custom, "2040.08", custom=custom)
        self.configure_target(["-DROR_BUILD_INSTALLER=Bundled"])
        self.build_target()
        self.metadata("2026.01", "2026.01", custom="2026.01")
        self.configure_target(["-DROR_BUILD_DEV_VERSION=OFF", "-UFIXTURE_CUSTOM_FILE", "-UROR_CUSTOM_VERSION"])
        self.build_target()
        self.metadata("2026.01", "2026.01")

    def test_actual_target_single_snapshot_and_copy_if_different(self):
        self.prepare_target()
        extra = []
        log = self.directory / "git calls.txt"
        if os.name != "nt":
            wrapper = self.directory / "git wrapper"
            wrapper.write_text('#!/bin/sh\nprintf "%s\\n" "$*" >> ' + shlex.quote(str(log)) +
                               '\nexec ' + shlex.quote(GIT) + ' "$@"\n')
            wrapper.chmod(0o755)
            extra = [f"-DGIT_EXECUTABLE={wrapper}"]
        self.configure_target(extra)
        if log.exists():
            log.write_text("")
        self.build_target("version_info")
        self.metadata("2000.01-dev-" + self.short_revision, "2000.01", dev=True)
        if log.exists():
            self.assertEqual(log.read_text().splitlines(), ["rev-parse --verify HEAD",
                             "rev-parse --short " + self.revision, "diff-index --quiet " + self.revision + " --"])
        outputs = [self.output / n for n in ("version_info.cpp", "version_def.h", "package-version.json")]
        for output in outputs:
            os.utime(output, (EPOCH, EPOCH))
        before = {p: p.stat().st_mtime_ns for p in outputs}
        self.build_target()
        self.assertEqual(before, {p: p.stat().st_mtime_ns for p in outputs})
        # Missing JSON is a byproduct recreated by the real target/library dependency.
        (self.output / "package-version.json").unlink()
        self.build_target("version_info")
        self.metadata("2000.01-dev-" + self.short_revision, "2000.01", dev=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work-dir", type=Path)
    options, remaining = parser.parse_known_args()
    if options.work_dir:
        options.work_dir.mkdir(parents=True, exist_ok=True)
        WORK = str(options.work_dir.resolve())
        unittest.main(argv=[__file__, *remaining])
    else:
        with tempfile.TemporaryDirectory(prefix="ror-version-tests-") as temporary:
            WORK = temporary
            result = unittest.main(argv=[__file__, *remaining], exit=False).result
        sys.exit(0 if result.wasSuccessful() else 1)
