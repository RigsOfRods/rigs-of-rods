#!/usr/bin/env python3
"""Bounded compile-profile controls; never executes the game.

python3 tests/macos/test_profile.py
ROR_TEST_DEPS_PREFIX=/absolute/prefix-release python3 tests/macos/test_profile.py
The optional integration controls copy the prefix into disposable storage.
"""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

SOURCE = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('profile', SOURCE / 'scripts/macos/profile.py')
profile = importlib.util.module_from_spec(spec)
spec.loader.exec_module(profile)


class Controls(unittest.TestCase):
    def test_selected_imported_artifacts(self):
        with tempfile.TemporaryDirectory(prefix='ror target controls ') as temporary:
            for case in ('valid', 'mapped-outside', 'later-interface', 'physical-interface', 'nested-target', 'unsupported-expression',
                         'unmapped-conditional', 'mapped-conditional-physical', 'mapped-conditional-interface'):
                build = Path(temporary) / case
                result = subprocess.run(['cmake', '-S', str(SOURCE / 'tests/macos/artifacts'),
                                         '-B', str(build), '-DCASE=' + case], capture_output=True, text=True)
                success = case in ('valid', 'unmapped-conditional')
                self.assertEqual(result.returncode == 0, success, result.stdout + result.stderr)
                if success:
                    self.assertEqual(Path((build / 'selected.txt').read_text()), (build / 'prefix/inside.dylib').resolve())
                else:
                    expected = ('unsupported MAP_IMPORTED_CONFIG_RELEASE on conditional input' if case.startswith('mapped-conditional-')
                                else 'unsupported selected' if case == 'unsupported-expression'
                                else 'escapes selected dependency prefix')
                    self.assertIn(expected, ' '.join((result.stdout + result.stderr).split()))

    def test_wrapper_arguments(self):
        for args in ([], ['--jobs'], ['--unknown'],
                     ['--deps-prefix', '/missing', '--build-dir', '/unused', '--config', 'Debug', '--jobs', '3'],
                     ['--deps-prefix', '/missing', '--build-dir', '/unused', '--config', 'Release', '--jobs', '0'],
                     ['--deps-prefix', '/missing', '--build-dir', '/unused', '--config', 'Release', '--jobs', '-1'],
                     ['--deps-prefix', '/missing', '--build-dir', '/unused', '--config', 'Release', '--jobs', 'x']):
            result = subprocess.run([str(SOURCE / 'scripts/macos/build.sh'), *args], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0, result.stdout)

    def test_missing_prefix_and_incompatible_cache(self):
        with tempfile.TemporaryDirectory(prefix='ror profile controls ') as temporary:
            root = Path(temporary)
            with self.assertRaises(FileNotFoundError):
                profile.contract(root / 'missing', 'macosx', '11.0')
            build = root / 'build with spaces'
            build.mkdir()
            (build / 'CMakeCache.txt').write_text('CMAKE_HOME_DIRECTORY:INTERNAL=/different/source\n')
            from argparse import Namespace
            with self.assertRaisesRegex(ValueError, 'incompatible cache'):
                profile.cache(Namespace(source=SOURCE, build=build, prefix=str(root / 'prefix'), sdk='macosx', deployment_target='11.0'))

    def test_content_pin_and_cleanliness(self):
        with tempfile.TemporaryDirectory(prefix='ror content control ') as temporary:
            source = Path(temporary)
            def git(*args):
                return subprocess.check_output(['git', '-C', str(source), *args], text=True)
            git('init', '-q')
            git('config', 'user.email', 'fixture@example.invalid')
            git('config', 'user.name', 'Fixture')
            (source / 'tracked').write_text('fixture')
            git('add', 'tracked')
            git('commit', '-qm', 'fixture')
            with self.assertRaisesRegex(ValueError, 'gitlink'):
                profile.content(source)
            pin = git('rev-parse', 'HEAD').strip()
            git('update-index', '--add', '--cacheinfo', '160000,' + pin + ',content')
            git('commit', '-qm', 'content gitlink')
            with self.assertRaisesRegex(ValueError, 'initialize pinned content'):
                profile.content(source)
            subprocess.run(['git', 'clone', '-q', '--no-hardlinks', str(source), str(source / 'content')], check=True)
            subprocess.run(['git', '-C', str(source / 'content'), 'checkout', '-q', pin], check=True)
            self.assertEqual(profile.content(source), pin)
            (source / 'content/tracked').write_text('dirty')
            with self.assertRaisesRegex(ValueError, 'tracked changes'):
                profile.content(source)


@unittest.skipUnless(os.environ.get('ROR_TEST_DEPS_PREFIX'), 'set ROR_TEST_DEPS_PREFIX for real-prefix controls')
class PrefixControls(unittest.TestCase):
    def test_real_configure_and_rejections(self):
        prefix = Path(os.environ['ROR_TEST_DEPS_PREFIX']).resolve()
        sdk = profile.run('xcrun', '--sdk', 'macosx', '--show-sdk-path')
        with tempfile.TemporaryDirectory(prefix='ror prefix controls ') as temporary:
            root = Path(temporary)
            copied = root / 'copied prefix'
            shutil.copytree(prefix, copied, symlinks=True)
            # Some installed package configs capture their original prefix.
            # Relocate only the disposable CMake fixture; never edit the real prefix.
            for config in copied.rglob('*.cmake'):
                text = config.read_text()
                if str(prefix) in text:
                    config.write_text(text.replace(str(prefix), str(copied.resolve())))
            def configure(name, *options, success=False, diagnostic=None):
                build = root / name
                args = ['cmake', '-S', str(SOURCE), '-B', str(build), '-G', 'Ninja',
                        '-DROR_MACOS_GL3_PROFILE=ON', '-DCMAKE_BUILD_TYPE=Release',
                        '-DCMAKE_OSX_ARCHITECTURES=arm64', '-DCMAKE_OSX_SYSROOT=' + sdk,
                        '-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0', '-DROR_DEPENDENCY_DIR=' + str(copied),
                        '-DOPENAL_LIBRARY=' + str(copied / 'lib/libopenal.dylib'),
                        '-DOPENAL_INCLUDE_DIR=' + str(copied / 'include/AL'), *options]
                result = subprocess.run(args, capture_output=True, text=True)
                self.assertEqual(result.returncode == 0, success, result.stdout + result.stderr)
                if diagnostic:
                    self.assertIn(diagnostic, ' '.join((result.stdout + result.stderr).split()))
                if not success:
                    self.assertFalse((build / 'macos/Release/build.json').exists())
                return build
            build = configure('valid build with spaces', success=True)
            dependencies = subprocess.check_output(['ninja', '-C', str(build), '-t', 'query', 'build.ninja'], text=True)
            for dependency in (SOURCE / 'scripts/macos/profile.py', SOURCE / 'source/main/plugins-macos.cfg.in',
                               copied / 'share/ror-dependencies/manifest.json',
                               copied / 'share/ror-dependencies/build-receipt.json',
                               copied / 'share/ror-dependencies/build-receipt.txt'):
                self.assertIn(str(dependency.resolve()), dependencies)
            profile.finalize(build)
            data = json.loads((build / 'macos/Release/build.json').read_text())
            self.assertEqual(len(data['plugins']), 5)
            self.assertEqual(len(data['framework_sources']), 1)
            framework = data['framework_sources'][0]
            selected = copied / 'lib/Release/MyGUIEngine.framework'
            self.assertEqual(framework, profile.framework_source(selected, copied.resolve()))
            image = Path(framework['source_image'])
            framework_root = Path(framework['source_root'])
            self.assertEqual(image, (selected / selected.stem).resolve())
            self.assertEqual(Path(framework['destination_image']).relative_to(framework['destination_root']), image.relative_to(framework_root))
            self.assertEqual(Path(framework['destination_root']), Path('Contents/Frameworks') / selected.name)
            self.assertEqual(profile.framework_source(image, copied.resolve()), framework)
            escaped = copied / 'fixture/MyGUIEngine.framework'
            escaped.mkdir(parents=True)
            (escaped / escaped.stem).symlink_to(image)
            with self.assertRaisesRegex(ValueError, 'escapes its framework root'):
                profile.framework_source(escaped, copied.resolve())
            cfg = (build / 'bin/plugins.cfg').read_text()
            self.assertFalse((build / 'bin/plugins_d.cfg').exists())
            for plugin in data['plugins']:
                self.assertIn(plugin['build_cfg_entry'], cfg)
                resolved = (root / 'App.app/Contents/MacOS' / data['bundle_plugin_folder'] /
                            plugin['bundle_cfg_entry'][len('Plugin='):]).resolve()
                self.assertEqual(resolved, (root / 'App.app' / plugin['destination']).resolve())
                self.assertEqual(str(Path(plugin['source']).resolve()), plugin['source'])
            configure('valid build with spaces', '-DROR_MACOS_GL3_PROFILE=OFF', success=True)
            for name in ('build.json', 'inputs.json', 'target-path.txt'):
                self.assertFalse((build / 'macos/Release' / name).exists(), name)
            # Re-enable once to establish a fresh handoff before the failed-config control.
            configure('valid build with spaces', success=True)
            self.assertFalse((build / 'bin/plugins_d.cfg').exists())
            profile.finalize(build)
            # Rejected reconfiguration must invalidate an earlier good handoff.
            configure('valid build with spaces', '-DCMAKE_BUILD_TYPE=Debug', diagnostic='Release only')
            configure('debug', '-DCMAKE_BUILD_TYPE=Debug', diagnostic='Release only')
            configure('wrong arch', '-DCMAKE_OSX_ARCHITECTURES=x86_64', diagnostic='requires native arm64')
            configure('feature off', '-DROR_USE_CAELUM=OFF', diagnostic='Required feature ROR_USE_CAELUM is OFF')
            configure('extra requirement', '-DROR_REQUIRED_FEATURES=UNKNOWN_FEATURE', diagnostic='Unknown required feature')
            outside = root / 'outside.dylib'
            shutil.copyfile(copied / 'lib/libopenal.dylib', outside)
            configure('external openal', '-DOPENAL_LIBRARY=' + str(outside), diagnostic='escapes selected dependency prefix')
            configure('framework openal', '-DOPENAL_LIBRARY=' + sdk + '/System/Library/Frameworks/OpenAL.framework', diagnostic='requires OpenAL Soft')
            receipt = copied / 'share/ror-dependencies/build-receipt.txt'
            original = receipt.read_text()
            receipt.write_text(original.replace('Build type: Release', 'Build type: Debug'))
            configure('receipt mismatch', diagnostic='Build type')
            receipt.write_text(original)
            completed = copied / 'share/ror-dependencies/build-receipt.json'
            original = completed.read_text()
            data = json.loads(original)
            data['state'] = 'incomplete'
            completed.write_text(json.dumps(data))
            configure('incomplete prefix', diagnostic='successful build receipt')
            data['state'] = 'complete'
            data['configuration_sha256'] = '0' * 64
            completed.write_text(json.dumps(data))
            configure('mixed prefix identity', diagnostic='identity mismatch')
            completed.write_text(original)
            for name in ('Codec_STBI', 'RenderSystem_GL3Plus'):
                plugin = copied / ('lib/OGRE/' + name + '.dylib')
                target = os.readlink(plugin)
                plugin.unlink()
                configure('missing ' + name, diagnostic='macOS handoff')
                plugin.symlink_to(target)
            # A symlink that resolves outside the copied prefix must fail as well.
            plugin = copied / 'lib/OGRE/Codec_STBI.dylib'
            plugin.unlink()
            plugin.symlink_to(prefix / 'lib/OGRE/Codec_STBI.dylib')
            configure('escaped plugin', diagnostic='escapes dependency prefix')


if __name__ == '__main__':
    unittest.main()
