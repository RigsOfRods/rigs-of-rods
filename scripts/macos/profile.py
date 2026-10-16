#!/usr/bin/env python3
"""Local compile handoff, not a bundle assembler or runtime verifier."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

if sys.version_info < (3, 7):
    sys.exit("The native macOS build profile requires Python 3.7 or later")


def run(*args):
    return subprocess.check_output(args, text=True).strip()


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inside(path, prefix):
    path = Path(path).resolve(strict=True)
    if path != prefix and prefix not in path.parents:
        raise ValueError(f"selected artifact escapes dependency prefix: {path}")
    return path


def contract(prefix, sdk, deployment):
    prefix = Path(prefix).resolve(strict=True)
    if not (prefix / 'version.cmake').is_file():
        raise ValueError('missing version.cmake; build the native ror-dependencies profile first')
    metadata = prefix / 'share/ror-dependencies'
    completed = json.loads(inside(metadata / 'build-receipt.json', prefix).read_text())
    if completed.get('schema_version') != 1 or completed.get('state') != 'complete':
        raise ValueError('prefix requires a schema 1 successful build receipt; wait for dependency installation to complete')
    manifest = json.loads(inside(metadata / 'manifest.json', prefix).read_text())
    receipt = dict(line.split(': ', 1) for line in
                   inside(metadata / 'build-receipt.txt', prefix).read_text().splitlines())
    if (manifest.get('profile') != 'native-macos-gl3' or
            manifest.get('dependencies_version') != 30 or
            manifest.get('recipe_tree_dirty') is not False or
            not re.fullmatch(r'[0-9a-f]{40}', manifest.get('recipe_revision', ''))):
        raise ValueError('prefix needs a clean native-macos-gl3 recipe manifest, dependency version 30')
    for key, expected in [('schema_version', 1), ('state', 'complete'), ('profile', 'native-macos-gl3'),
                          ('dependencies_version', 30), ('recipe_tree_dirty', False)]:
        if manifest.get(key) != expected or completed.get(key) != expected:
            raise ValueError(f'prefix manifest and successful build receipt require {key}={expected!r}')
    for key in ('source_inventory_sha256', 'configuration_sha256', 'recipe_revision', 'completed_at'):
        if not completed.get(key) or manifest.get(key) != completed[key]:
            raise ValueError(f'prefix manifest/build receipt identity mismatch: {key}')
    for key in ('source_inventory_sha256', 'configuration_sha256'):
        if not isinstance(completed[key], str) or not re.fullmatch(r'[0-9a-f]{64}', completed[key]):
            raise ValueError(f'prefix requires a configured {key} identity')
    for label, key in [('CMake', 'cmake_version'), ('Compiler', 'compiler'), ('SDK', 'sdk_version'),
                       ('Architecture', 'architecture'), ('Deployment target', 'deployment_target'),
                       ('Build type', 'build_type')]:
        if receipt.get(label) != completed.get('toolchain', {}).get(key):
            raise ValueError(f'prefix build receipt text/JSON mismatch for {label}')
    for key, expected in [('Architecture', 'arm64'), ('Build type', 'Release'),
                          ('Deployment target', deployment)]:
        if not expected or receipt.get(key) != expected:
            raise ValueError(f"prefix {key}={receipt.get(key)!r}, selected {expected!r}; select matching build inputs")
    if not sdk:
        raise ValueError('set CMAKE_OSX_SYSROOT to the selected macOS SDK')
    selected_sdk = run('xcrun', '--sdk', sdk, '--show-sdk-version')
    if selected_sdk != receipt.get('SDK'):
        raise ValueError(f"SDK {selected_sdk} does not match prefix SDK {receipt.get('SDK')}")
    return prefix, manifest, receipt


def content(source):
    row = run('git', '-C', str(source), 'ls-tree', 'HEAD', 'content').split()
    if len(row) != 4 or row[:2] != ['160000', 'commit']:
        raise ValueError('source HEAD must select a content gitlink')
    path = source / 'content'
    if not (path / '.git').exists():
        raise ValueError('initialize pinned content: git submodule update --init --checkout content')
    if run('git', '-C', str(path), 'rev-parse', 'HEAD') != row[2]:
        raise ValueError('content HEAD differs from source gitlink; run git submodule update --init --checkout content')
    if run('git', '-C', str(path), 'status', '--porcelain', '--untracked-files=no'):
        raise ValueError('content has tracked changes; use a clean checkout of the selected gitlink')
    return row[2]


def dylib(path, prefix):
    path = inside(path, prefix)
    if any(char in str(path) for char in '\r\n'):
        raise ValueError('plugin cfg paths cannot contain newline characters')
    if path.suffix != '.dylib':
        raise ValueError(f'plugin must be a shared dylib: {path}')
    description = run('file', '-b', str(path))
    if 'Mach-O' not in description or 'dynamically linked shared library' not in description:
        raise ValueError(f'plugin must be a Mach-O shared library: {path}')
    if run('lipo', '-archs', str(path)) != 'arm64':
        raise ValueError(f'plugin must contain only arm64: {path}')
    return path


def handoff(a):
    prefix, manifest, receipt = contract(a.prefix, a.sdk, a.deployment_target)
    source, build = a.source.resolve(), a.build.resolve()
    folder = inside(a.plugin_folder, prefix)
    if any(char in str(folder) for char in '\r\n'):
        raise ValueError('PluginFolder cannot contain newline characters')
    plugins, destinations, sources = [], set(), set()
    for name, path in [*a.plugin, ('Caelum', a.caelum)]:
        path = dylib(path, prefix)
        caelum = name == 'Caelum'
        destination = ('Contents/Frameworks/' if caelum else 'Contents/PlugIns/OGRE/') + path.name
        if destination.casefold() in destinations or path in sources:
            raise ValueError(f'plugin source/destination collision: {path} -> {destination}')
        destinations.add(destination.casefold())
        sources.add(path)
        plugins.append(dict(name=name, source=str(path), roles=['linked', 'plugin'] if caelum else ['plugin'],
                            destination=destination, required=True,
                            build_cfg_entry='Plugin=' + str(path.relative_to(folder)) if path.parent == folder
                            else 'Plugin=' + os.path.relpath(path, folder),
                            bundle_cfg_entry='Plugin=' + ('../../Frameworks/' if caelum else '') + path.name))
    roots = {key: str(build / 'bin' / key) for key in ('resources', 'content', 'languages')}
    rtss = inside(prefix / 'share/OGRE/Media/RTShaderLib/GLSL', prefix)
    required, effective = read_features(build)
    data = dict(schema=1, profile='native-macos-gl3', configuration='Release', architecture='arm64',
                sdk=a.sdk, sdk_version=receipt['SDK'], deployment_target=a.deployment_target,
                dependency_prefix=str(prefix), dependency_manifest=str(prefix / 'share/ror-dependencies/manifest.json'),
                dependency_receipt=str(prefix / 'share/ror-dependencies/build-receipt.txt'),
                dependency_build_receipt=str(prefix / 'share/ror-dependencies/build-receipt.json'),
                dependency_manifest_sha256=digest(prefix / 'share/ror-dependencies/manifest.json'),
                dependency_receipt_sha256=digest(prefix / 'share/ror-dependencies/build-receipt.txt'),
                dependency_build_receipt_sha256=digest(prefix / 'share/ror-dependencies/build-receipt.json'),
                source_inventory_sha256=manifest['source_inventory_sha256'],
                configuration_sha256=manifest['configuration_sha256'],
                recipe_revision=manifest['recipe_revision'], effective_features=effective,
                required_features=required, source_directory=str(source), content_revision=content(source), resource_roots=roots,
                rtshader_glsl_root=str(rtss), plugins=plugins, build_plugin_folder=str(folder),
                bundle_plugin_folder='../PlugIns/OGRE', dynamic_modules=[],
                package_version='../../source/version_info/package-version.json',
                package_version_requirement='Requires the version metadata contribution before packaging; this profile is build-only.')
    directory = build / 'macos/Release'
    directory.mkdir(parents=True, exist_ok=True)
    (directory / 'inputs.json').write_text(json.dumps(data, indent=2) + '\n')
    cfg = (source / 'source/main/plugins-macos.cfg.in').read_text().replace(
        '@PLUGINS_FOLDER@', str(folder)).replace('@PLUGIN_LINES@', '\n'.join(p['build_cfg_entry'] for p in plugins))
    (build / 'bin').mkdir(parents=True, exist_ok=True)
    (build / 'bin/plugins.cfg').write_text(cfg)


def read_features(build):
    text = (build / 'ror-features.cmake').read_text()
    required_match = re.search(r'set\(ROR_REQUIRED_FEATURES "([^"]*)"\)', text)
    if not required_match:
        raise ValueError('missing effective required-feature receipt')
    required = required_match[1].split(';')
    effective = {name: value == 'ON' for name, value in re.findall(r'set\((ROR_USE_\w+) (ON|OFF)\)', text)}
    promised = {'ROR_USE_' + name for name in ('OPENAL', 'ANGELSCRIPT', 'CURL', 'SOCKETW', 'DISCORD_RPC', 'CAELUM', 'PAGED')}
    if not promised.issubset(required) or any(not effective.get(name) for name in promised):
        raise ValueError('effective feature receipt must require and enable all seven profile features')
    return required, effective


def finalize(build):
    directory = build.resolve() / 'macos/Release'
    output = directory / 'build.json'
    try:
        data = json.loads((directory / 'inputs.json').read_text())
        if data['schema'] != 1 or data['configuration'] != 'Release':
            raise ValueError('unsupported handoff schema/configuration')
        required, effective = read_features(build)
        if required != data['required_features'] or effective != data['effective_features']:
            raise ValueError('effective feature receipt changed; reconfigure before building')
        for key in ('dependency_manifest', 'dependency_receipt', 'dependency_build_receipt'):
            if digest(data[key]) != data[key + '_sha256']:
                raise ValueError('dependency metadata changed; reconfigure against the completed prefix')
        if content(Path(data['source_directory'])) != data['content_revision']:
            raise ValueError('selected content changed; reconfigure before building')
        data['executable'] = (directory / 'target-path.txt').read_text()
        if not Path(data['executable']).is_absolute():
            raise ValueError('CMake must generate an absolute executable target path')
        serialized = json.dumps(data, indent=2) + '\n'
        if not output.exists() or output.read_text() != serialized:
            output.write_text(serialized)
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError):
        if output.exists():
            output.unlink()
        raise


def cache(a):
    source, build, prefix = a.source.resolve(), a.build.resolve(), Path(a.prefix).resolve()
    if build == source or source in build.parents or build == prefix or prefix in build.parents:
        raise ValueError('select an out-of-source build directory outside the dependency prefix')
    path = build / 'CMakeCache.txt'
    if not path.exists():
        if build.exists() and any(build.iterdir()):
            raise ValueError('build directory is nonempty without a CMake cache; choose a fresh directory')
        return
    entries = dict(re.findall(r'^([^/#][^:\n]*):[^=\n]*=(.*)$', path.read_text(), re.M))
    expected = dict(CMAKE_HOME_DIRECTORY=str(source), ROR_DEPENDENCY_DIR=str(prefix),
                    CMAKE_BUILD_TYPE='Release', CMAKE_OSX_ARCHITECTURES='arm64', CMAKE_GENERATOR='Ninja',
                    CMAKE_OSX_SYSROOT=a.sdk, CMAKE_OSX_DEPLOYMENT_TARGET=a.deployment_target,
                    ROR_MACOS_GL3_PROFILE='ON')
    for key, value in expected.items():
        if entries.get(key) != value:
            raise ValueError(f'incompatible cache {key}={entries.get(key)!r}, expected {value!r}; choose a fresh build directory')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    for command in ('preflight', 'handoff', 'cache'):
        p = sub.add_parser(command)
        for name in ('prefix', 'sdk', 'deployment-target'):
            p.add_argument('--' + name, required=True)
        if command in ('handoff', 'cache'):
            for name in ('source', 'build'):
                p.add_argument('--' + name, required=True, type=Path)
        if command == 'handoff':
            p.add_argument('--plugin', nargs=2, action='append', required=True)
            p.add_argument('--plugin-folder', required=True)
            p.add_argument('--caelum', required=True)
    p = sub.add_parser('content')
    p.add_argument('--source', type=Path, required=True)
    p = sub.add_parser('finalize')
    p.add_argument('--build', type=Path, required=True)
    a = parser.parse_args()
    if a.command == 'preflight':
        contract(a.prefix, a.sdk, a.deployment_target)
    elif a.command == 'content':
        content(a.source)
    elif a.command == 'handoff':
        handoff(a)
    elif a.command == 'cache':
        cache(a)
    else:
        finalize(a.build)


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        sys.exit(str(error))
