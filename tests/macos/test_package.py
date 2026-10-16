"""Synthetic, nonexecuted Mach-O fixtures. No RoR/window/GPU runtime tests."""
import argparse
import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import plistlib
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import tarfile
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/macos'))
import macho
import package
import source_companion
import verify


def command(*args):
    return subprocess.check_output([str(a) for a in args], text=True, stderr=subprocess.STDOUT).strip()


def fixture_commit(path):
    command('git', '-C', path, 'init', '-q')
    command('git', '-C', path, 'add', '.')
    command('git', '-C', path, '-c', 'user.name=Fixture', '-c', 'user.email=fixture@example.invalid', 'commit', '-qm', 'Synthetic source fixture')
    return command('git', '-C', path, 'rev-parse', 'HEAD')


def fixture_tools(root):
    tools = root / 'committed-packaging-tools'
    tools.mkdir()
    for name in source_companion.TOOL_FILES:
        target = tools / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / name, target)
    fixture_commit(tools)
    return tools


class SourceCompanion(unittest.TestCase):
    def test_four_committed_sources_and_untracked_tools_rejection(self):
        with tempfile.TemporaryDirectory(prefix='ror-source-only-fixture-') as directory:
            root = Path(directory).resolve()
            source = root / 'game-source'
            content = source / 'content'
            content.mkdir(parents=True)
            (content / 'LICENSE').write_text('synthetic content notice')
            content_revision = fixture_commit(content)
            (source / 'COPYING').write_text('synthetic game source notice')
            source_revision = fixture_commit(source)
            recipe = root / 'recipe'
            (recipe / 'cmake').mkdir(parents=True)
            cache = root / 'cache'
            cache.mkdir()
            components = {}
            for index in range(16):
                name = 'synthetic-%02d' % index
                path = cache / (name + '.tar.gz')
                path.write_text('synthetic source bytes ' + name)
                components[name] = dict(sha256=source_companion.digest(path), patches=[], installed_notices=['licenses/' + name])
            recipe_inventory = recipe / 'cmake/macos-sources.json'
            package.write_json(recipe_inventory, dict(components=components))
            recipe_revision = fixture_commit(recipe)
            tools = fixture_tools(root)
            tool_revision = command('git', '-C', tools, 'rev-parse', 'HEAD')
            handoff = dict(content_revision=content_revision)
            version = dict(full_revision=source_revision)
            manifest = dict(components=components, recipe_revision=recipe_revision, configuration_sha256='b' * 64,
                            source_inventory_sha256=source_companion.digest(recipe_inventory))
            inputs = source_companion.source_inputs(cache, recipe, source, version, handoff, manifest, tools)
            notices = root / 'notices'
            notices.mkdir()
            (notices / 'NOTICE').write_text('synthetic fixture only')
            destination = root / 'output'
            destination.mkdir()
            path, inventory = source_companion.create(destination, inputs, handoff, version, manifest, notices)
            self.assertEqual(inventory['packaging_revision'], tool_revision)
            self.assertEqual(set(inventory['trees']), {'rigs-of-rods', 'base-content', 'dependency-recipe', 'packaging-tools'})
            binary_inventory = dict(version=version, content_revision=content_revision, recipe_revision=recipe_revision,
                                    packaging_revision=tool_revision, source_inventory_sha256=manifest['source_inventory_sha256'], configuration_sha256='b' * 64)
            verify.verify_source(path, binary_inventory, manifest)
            with tarfile.open(path, 'r:gz') as outer:
                with tarfile.open(fileobj=outer.extractfile('corresponding-source/packaging-tools.tar'), mode='r:') as archive:
                    selected = {m.name for m in archive.getmembers() if m.isfile()}
                    self.assertEqual(selected, {'packaging-tools/' + name for name in source_companion.TOOL_FILES})
                    for name in source_companion.TOOL_FILES:
                        self.assertEqual(archive.extractfile('packaging-tools/' + name).read(), (tools / name).read_bytes())
            (tools / 'untracked-private-fixture.txt').write_text('must not be archived')
            with self.assertRaisesRegex(ValueError, 'clean, including untracked'):
                source_companion.source_inputs(cache, recipe, source, version, handoff, manifest, tools)


class ParserAndSafety(unittest.TestCase):
    def test_load_commands_exclude_identity_and_keep_weak_reexport_upward(self):
        text = 'fixture:\n' + '\n'.join('Load command %d\n cmd %s\n name %s (offset 24)\n' % (i, c, n) for i, (c, n) in enumerate([
            ('LC_ID_DYLIB', '@rpath/self.dylib'), ('LC_LOAD_DYLIB', '/usr/lib/libSystem.B.dylib'),
            ('LC_LOAD_WEAK_DYLIB', '@rpath/weak.dylib'), ('LC_REEXPORT_DYLIB', '@rpath/reexport.dylib'),
            ('LC_LOAD_UPWARD_DYLIB', '@rpath/upward.dylib')]))
        parsed = macho.parse_load_commands(text)
        self.assertEqual(parsed.identity, '@rpath/self.dylib')
        self.assertEqual(len(parsed.loads), 4)
        self.assertNotIn('LC_ID_DYLIB', [c for c, n in parsed.loads])

    def test_archive_traversal_escaping_links_duplicates_and_alias_writes(self):
        for entries in ([('../escape', b'x', 0o100644)], [('a.app/link', b'../../escape', 0o120777)],
                        [('a.app/link', b'inside', 0o120777), ('a.app/link/write', b'x', 0o100644)],
                        [('a.app/A', b'x', 0o100644), ('a.app/a', b'x', 0o100644)]):
            with self.subTest(entries=entries), tempfile.TemporaryDirectory() as folder:
                path = Path(folder) / 'bad.zip'
                with zipfile.ZipFile(path, 'w') as z:
                    for name, data, mode in entries:
                        entry = zipfile.ZipInfo(name)
                        entry.external_attr = mode << 16
                        z.writestr(entry, data)
                with self.assertRaises(ValueError):
                    verify.archive_layout(path)

    def test_path_and_output_protection(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            source = root / 'source'
            source.mkdir()
            for output in (source / 'out', root):
                with self.assertRaises(ValueError):
                    package.protect_output(output, [source], False)
            output = root / 'out'
            output.mkdir()
            (output / 'keep').write_text('user file')
            with self.assertRaises(ValueError):
                package.protect_output(output, [source], True)
            self.assertEqual((output / 'keep').read_text(), 'user file')
            package.write_json(output / '.ror-package-owner.json', dict(owner=package.OWNER, output=str(output.resolve())))
            self.assertEqual(package.protect_output(output, [source], True), output.resolve())

    def test_case_and_basename_collisions(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            exe = root / 'game'
            a, b = root / 'a/libSame.dylib', root / 'b/LIBSAME.dylib'
            for p in (exe, a, b):
                p.parent.mkdir(exist_ok=True)
                p.touch()
            with self.assertRaisesRegex(ValueError, 'collision'):
                macho.destinations([exe, a, b], exe, {})
            (root / 'escape').symlink_to('/tmp')
            with self.assertRaises(ValueError):
                macho.check_links(root)

    def test_context_conflict_missing_and_system_cache(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            exe, first, second = root / 'game', root / 'a/lib.dylib', root / 'b/lib.dylib'
            for p in (exe, first, second):
                p.parent.mkdir(exist_ok=True)
                p.touch()
            with self.assertRaisesRegex(ValueError, 'conflicting resolutions'):
                macho.resolve('@rpath/lib.dylib', exe, exe, (first.parent, second.parent))
            with self.assertRaisesRegex(ValueError, 'missing dependency'):
                macho.resolve('@rpath/missing.dylib', exe, exe, ())
            self.assertIsNone(macho.resolve('/System/Library/Frameworks/AbsentCache.framework/AbsentCache', exe, exe, ()))
            self.assertFalse(macho.system('/usr/lib/../../opt/evil.dylib'))

    def test_wrong_architecture_is_rejected(self):
        with patch.object(macho, 'run', return_value='x86_64'):
            with self.assertRaisesRegex(ValueError, 'exactly arm64'):
                macho.inspect(Path('synthetic-architecture-metadata'))


@unittest.skipUnless(sys.platform == 'darwin' and shutil.which('clang'), 'requires existing macOS command-line tools')
class NativeFixtures(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # All compilers are invoked serially and no generated image is executed.
        parent = os.environ.get('ROR_PACKAGE_FIXTURE_PARENT')
        cls.temporary = None if parent else tempfile.TemporaryDirectory(prefix='ror-package-fixtures-')
        cls.root = Path(tempfile.mkdtemp(prefix='ror-package-fixtures-', dir=parent) if parent else cls.temporary.name).resolve()
        cls.prefix = cls.root / 'prefix with spaces'
        cls.lib = cls.prefix / 'lib'
        cls.lib.mkdir(parents=True)
        cls.build = cls.root / 'build with spaces'
        (cls.build / 'bin').mkdir(parents=True)
        cls.sdk = command('xcrun', '--show-sdk-version')
        cls.code = cls.root / 'tiny.c'
        cls.code.write_text('int fixture(void) { return 1; }\n')
        cls.compile(cls.lib / 'libB.dylib', '-dynamiclib', '-Wl,-install_name,@rpath/libB.dylib')
        cls.compile(cls.lib / 'libA.dylib', '-dynamiclib', '-Wl,-install_name,@rpath/libA.dylib', str(cls.lib / 'libB.dylib'))
        cls.framework = cls.lib / 'MyGUIEngine.framework'
        real = cls.framework / 'Versions/3.4.0'
        (real / 'Resources').mkdir(parents=True)
        (real / 'Resources/Info.plist').write_bytes(plistlib.dumps(dict(CFBundleIdentifier='org.fixture.MyGUI', CFBundleExecutable='MyGUIEngine', CFBundlePackageType='FMWK', CFBundleVersion='3.4.0')))
        (real / 'Resources/payload.txt').write_text('complete required framework payload')
        (cls.framework / 'Versions/Current').symlink_to('3.4.0')
        (cls.framework / 'MyGUIEngine').symlink_to('Versions/Current/MyGUIEngine')
        (cls.framework / 'Resources').symlink_to('Versions/Current/Resources')
        cls.compile(real / 'MyGUIEngine', '-dynamiclib', '-Wl,-install_name,@rpath/MyGUIEngine.framework/Versions/3.4.0/MyGUIEngine')
        cls.selected = {}
        for name in sorted(package.PLUGINS):
            path = cls.lib / (name + '.dylib')
            cls.compile(path, '-dynamiclib', '-Wl,-install_name,@rpath/' + path.name, str(cls.lib / 'libB.dylib'))
            cls.selected[path] = ('Contents/Frameworks/' if name == 'Caelum' else 'Contents/PlugIns/OGRE/') + path.name
        cls.exe = cls.build / 'bin/RoR-fixture'
        main = cls.root / 'main.c'
        main.write_text('int main(void) { return 0; }\n')
        cls.compile(cls.exe, str(cls.lib / 'libA.dylib'), str(real / 'MyGUIEngine'), str(cls.lib / 'Caelum.dylib'), '-Wl,-rpath,' + str(cls.lib), code=main)

    @classmethod
    def compile(cls, output, *args, code=None):
        command('clang', '-arch', 'arm64', '-mmacosx-version-min=11.0', '-Wl,-headerpad_max_install_names', *(args), code or cls.code, '-o', output)

    @classmethod
    def tearDownClass(cls):
        if cls.temporary:
            cls.temporary.cleanup()

    def test_actual_closure_transitive_unlinked_plugin_and_framework(self):
        images, edges = macho.closure(self.exe, self.selected, (self.build, self.prefix))
        self.assertIn(self.lib / 'libB.dylib', images)
        self.assertIn(self.lib / 'Codec_STBI.dylib', images)
        image = (self.framework / 'Versions/3.4.0/MyGUIEngine').resolve()
        self.assertIn(image, images)
        mapping = macho.destinations(images, self.exe, self.selected)
        self.assertEqual(mapping[image], 'Contents/Frameworks/MyGUIEngine.framework/Versions/3.4.0/MyGUIEngine')
        self.assertEqual(edges[(self.lib / 'libA.dylib', '@rpath/libB.dylib')], self.lib / 'libB.dylib')
        self.assertNotIn((self.lib / 'libB.dylib', '@rpath/libB.dylib'), edges)

    def test_actual_missing_dependency_and_loader_executable_paths(self):
        executable = self.root / 'context-main'
        shutil.copyfile(self.exe, executable)
        command('install_name_tool', '-change', '@rpath/libA.dylib', '@executable_path/prefix with spaces/lib/libA.dylib', executable)
        command('install_name_tool', '-change', '@rpath/Caelum.dylib', '@loader_path/prefix with spaces/lib/Caelum.dylib', executable)
        images, edges = macho.closure(executable, [], (self.root,))
        self.assertIn(self.lib / 'libA.dylib', images)
        command('install_name_tool', '-change', '@executable_path/prefix with spaces/lib/libA.dylib', '@loader_path/missing.dylib', executable)
        with self.assertRaisesRegex(ValueError, 'missing dependency'):
            macho.closure(executable, [], (self.root,))

    def make_contract(self):
        case = self.root / 'synthetic-publication-inputs'
        case.mkdir()
        source = case / 'source'
        source.mkdir()
        for name in package.ROR_NOTICES:
            path = source / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('SYNTHETIC FIXTURE NOTICE; not dependency licensing evidence.\n')
        fonts = source / 'resources/fonts'
        fonts.mkdir(parents=True)
        for filename in package.FONT_NOTICES:
            (fonts / filename).write_text('SYNTHETIC FIXTURE NOTICE\n')
        for filename in package.FONT_HASHES:
            shutil.copyfile(ROOT / 'resources/fonts' / filename, fonts / filename)
        (source / 'resources/icons').mkdir()
        shutil.copyfile(ROOT / 'resources/icons/ror.png', source / 'resources/icons/ror.png')
        content = source / 'content'
        content.mkdir()
        (content / 'LICENSE').write_text('SYNTHETIC CONTENT LICENSE\n')
        def commit(path):
            command('git', '-C', path, 'init', '-q')
            command('git', '-C', path, 'add', '.')
            command('git', '-C', path, '-c', 'user.name=Fixture', '-c', 'user.email=fixture@example.invalid', 'commit', '-qm', 'Synthetic package fixture')
            return command('git', '-C', path, 'rev-parse', 'HEAD')
        content_revision = commit(content)
        source_revision = commit(source)
        recipe = case / 'recipe'
        recipe.mkdir()
        (recipe / 'README.md').write_text('Synthetic recipe fixture; no actual dependencies built.\n')
        cache = case / 'cache'
        cache.mkdir()
        metadata = self.prefix / 'share/ror-dependencies'
        metadata.mkdir(parents=True)
        components = {}
        for i in range(16):
            name = 'synthetic-%02d' % i
            path = cache / (name + '.tar.gz')
            path.write_bytes(('synthetic archive bytes %d' % i).encode())
            notice = 'licenses/' + name + '/LICENSE'
            (metadata / notice).parent.mkdir(parents=True)
            (metadata / notice).write_text('SYNTHETIC NOTICE\n')
            components[name] = dict(sha256=source_companion.digest(path), patches=[], installed_notices=[notice],
                                    version='fixture', revision='0' * 40, repository='https://example.invalid/fixture', url='https://example.invalid/fixture.tar.gz')
        (recipe / 'cmake').mkdir()
        package.write_json(recipe / 'cmake/macos-sources.json', dict(components=components))
        recipe_revision = commit(recipe)
        inventory_hash = source_companion.digest(recipe / 'cmake/macos-sources.json')
        manifest = dict(schema_version=1, state='complete', profile='native-macos-gl3', dependencies_version=30,
                        recipe_tree_dirty=False, recipe_revision=recipe_revision, completed_at='synthetic-fixture',
                        source_inventory_sha256=inventory_hash, configuration_sha256='b' * 64, components=components)
        toolchain = dict(architecture='arm64', build_type='Release', sdk_version=self.sdk, deployment_target='11.0', compiler='fixture', cmake_version='fixture')
        receipt = dict(manifest, toolchain=toolchain)
        receipt.pop('components')
        package.write_json(metadata / 'manifest.json', manifest)
        package.write_json(metadata / 'build-receipt.json', receipt)
        (metadata / 'build-receipt.txt').write_text('CMake: fixture\nCompiler: fixture\nSDK: ' + self.sdk + '\nArchitecture: arm64\nDeployment target: 11.0\nBuild type: Release\n')
        version = dict(schema=1, full_revision=source_revision, dirty=False, display_version='synthetic-fixture', apple_short_version='2026.1.0', apple_bundle_version='2026.1.0')
        version_dir = self.build / 'source/version_info'
        version_dir.mkdir(parents=True)
        package.write_json(version_dir / 'package-version.json', version)
        roots = {}
        for name in ('resources', 'content', 'languages'):
            root = self.build / 'bin' / name
            root.mkdir()
            (root / 'fixture.txt').write_text('synthetic ' + name)
            roots[name] = str(root)
        shutil.copytree(fonts, self.build / 'bin/resources/fonts')
        with zipfile.ZipFile(self.build / 'bin/resources/fixture-resources.zip', 'w') as z:
            z.writestr('fixture/shader.glsl', '// fixture payload\n')
        rtss = self.prefix / 'share/OGRE/Media/RTShaderLib/GLSL'
        rtss.mkdir(parents=True)
        (rtss / 'fixture.glsl').write_text('// synthetic shader source; no runtime test\n')
        plugins = []
        for path, destination in self.selected.items():
            name = path.stem
            plugins.append(dict(name=name, source=str(path), destination=destination, required=True,
                                roles=['linked', 'plugin'] if name == 'Caelum' else ['plugin'],
                                bundle_cfg_entry='Plugin=' + ('../../Frameworks/' if name == 'Caelum' else '') + path.name))
        handoff = dict(schema=1, profile='native-macos-gl3', configuration='Release', architecture='arm64',
                       dependency_prefix=str(self.prefix), source_directory=str(source), content_revision=content_revision,
                       recipe_revision=recipe_revision, effective_features={k: True for k in package.FEATURES}, required_features=sorted(package.FEATURES),
                       sdk_version=self.sdk, deployment_target='11.0', executable=str(self.exe), resource_roots=roots,
                       rtshader_glsl_root=str(rtss), plugins=plugins, dynamic_modules=[], bundle_plugin_folder='../PlugIns/OGRE',
                       package_version='../../source/version_info/package-version.json',
                       source_inventory_sha256=inventory_hash, configuration_sha256='b' * 64,
                       framework_sources=[dict(name='MyGUIEngine', source_root=str(self.framework), source_image=str(self.framework / 'Versions/3.4.0/MyGUIEngine'),
                                              destination_root='Contents/Frameworks/MyGUIEngine.framework', destination_image='Contents/Frameworks/MyGUIEngine.framework/Versions/3.4.0/MyGUIEngine')])
        for key, filename in [('dependency_manifest', 'manifest.json'), ('dependency_receipt', 'build-receipt.txt'), ('dependency_build_receipt', 'build-receipt.json')]:
            handoff[key] = str(metadata / filename)
            handoff[key + '_sha256'] = source_companion.digest(metadata / filename)
        directory = self.build / 'macos/Release'
        directory.mkdir(parents=True)
        package.write_json(directory / 'build.json', handoff)
        return case, cache, recipe, fixture_tools(case)

    def test_full_synthetic_package_final_zip_source_and_altered_signature(self):
        case, cache, recipe, tools = self.make_contract()
        # Actual MyGUI imports name the future framework location before an app
        # exists. Only the exact handoff-declared framework identity can map it.
        command('install_name_tool', '-change', '@rpath/MyGUIEngine.framework/Versions/3.4.0/MyGUIEngine',
                '@executable_path/../Frameworks/MyGUIEngine.framework/Versions/3.4.0/MyGUIEngine', self.exe)
        output = case / 'output'
        args = argparse.Namespace(build_dir=self.build, deps_prefix=self.prefix, output=output, config='Release', source_cache=cache, recipe_source=recipe, packaging_source=tools, replace=False)
        package.package(args)
        archive = output / 'Rigs-of-Rods-macOS-arm64.zip'
        result = verify.verify_archive(archive)
        self.assertFalse(result['runtime_tested'])
        framework = output / 'Rigs of Rods.app/Contents/Frameworks/MyGUIEngine.framework'
        self.assertTrue((framework / 'Versions/Current').is_symlink())
        self.assertEqual((framework / 'Versions/3.4.0/Resources/payload.txt').read_text(), 'complete required framework payload')
        with self.assertRaises(ValueError):
            package.package(args)
        extracted = case / 'altered'
        command('ditto', '-x', '-k', archive, extracted)
        app = extracted / 'Rigs of Rods.app'
        library = app / 'Contents/Frameworks/libB.dylib'
        offset = int(re.search(r'sectname __text\n.*?\n\s*offset (\d+)', command('otool', '-l', library), re.S)[1])
        with library.open('r+b') as stream:
            stream.seek(offset)
            value = stream.read(1)[0]
            stream.seek(offset)
            stream.write(bytes([value ^ 0xff]))
        with self.assertRaises(subprocess.CalledProcessError):
            command('codesign', '--verify', '--strict', library)
        altered = output / 'altered.zip'
        command('ditto', '-c', '-k', '--keepParent', app, altered)
        with self.assertRaises(subprocess.CalledProcessError):
            verify.verify_archive(altered)


if __name__ == '__main__':
    unittest.main()
