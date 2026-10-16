#!/usr/bin/env python3
"""Assemble a native macOS Release app from a successful schema-1 build handoff."""
import argparse
import json
import os
from pathlib import Path
import plistlib
import re
import shutil
import subprocess
import sys
import tempfile

from macho import (check_links, closure, destinations, framework, inspect, is_macho,
                   relative, run, version as numeric_version, within)
from source_companion import clean_tree, create as source_archive, digest, source_inputs

OWNER = 'rigs-of-rods-macos-package-v1'
FEATURES = {'ROR_USE_' + name for name in ('OPENAL', 'ANGELSCRIPT', 'CURL', 'SOCKETW', 'DISCORD_RPC', 'CAELUM', 'PAGED')}
PLUGINS = {'RenderSystem_GL3Plus', 'Codec_STBI', 'Plugin_ParticleFX', 'Plugin_OctreeSceneManager', 'Caelum'}
FONT_HASHES = {'Cousine-Regular.ttf': '0d5d5eeb6a342432bd63a3c0d16e8470160e019933ee5af3e159d06d665dacce',
               'DroidSans.ttf': '4e2371bc0e4cf6983342e150412f140da79d674c9be0b56458401f581072ecd3',
               'Roboto-Medium.ttf': '8559132c89ad51d8a2ba5b171887a44a7ba93776e205f553573de228e64b45f8'}
FONT_NOTICES = ('FONT-NOTICES.md', 'LICENSE-APACHE2.txt', 'LICENSE-OFL.txt')
ROR_NOTICES = ('COPYING', 'source/main/COPYING', 'source/main/gui/imgui/LICENSE-ImGui.txt',
               'source/main/gui/imgui/LICENSE-OGRE.txt', 'external/header_only/half/LICENSE.txt',
               'external/header_only/SimpleGlob.h', 'external/header_only/SimpleOpt.h')


def write_json(path, data):
    path.write_text(json.dumps(data, sort_keys=True, indent=2) + '\n')


def selected_path(value, root):
    path = Path(value).resolve(strict=True)
    if not within(path, root):
        raise ValueError(f'input escapes expected root {root}: {value}')
    return path


def validate_inputs(build, prefix, config):
    if config != 'Release':
        raise ValueError('only the compiled native Release profile is supported')
    handoff = json.loads((build / 'macos/Release/build.json').read_text())
    for key, value in dict(schema=1, profile='native-macos-gl3', configuration='Release', architecture='arm64').items():
        if handoff.get(key) != value:
            raise ValueError(f'build handoff requires {key}={value!r}')
    if Path(handoff['dependency_prefix']).resolve(strict=True) != prefix:
        raise ValueError('selected dependency prefix differs from build handoff')
    if not FEATURES.issubset(handoff['required_features']) or any(handoff['effective_features'].get(k) is not True for k in FEATURES):
        raise ValueError('build must require and effectively enable all seven native profile features')
    metadata = prefix / 'share/ror-dependencies'
    for key, filename in [('dependency_manifest', 'manifest.json'), ('dependency_receipt', 'build-receipt.txt'),
                          ('dependency_build_receipt', 'build-receipt.json')]:
        path = selected_path(handoff[key], prefix)
        if path != (metadata / filename).resolve(strict=True) or digest(path) != handoff[key + '_sha256']:
            raise ValueError('dependency metadata hash/path changed: ' + key)
    manifest = json.loads((metadata / 'manifest.json').read_text())
    receipt = json.loads((metadata / 'build-receipt.json').read_text())
    for key, expected in dict(schema_version=1, state='complete', profile='native-macos-gl3', dependencies_version=30, recipe_tree_dirty=False).items():
        if manifest.get(key) != expected or receipt.get(key) != expected:
            raise ValueError('dependency metadata requires ' + key)
    for key in ('source_inventory_sha256', 'configuration_sha256', 'recipe_revision'):
        if not handoff.get(key) or manifest.get(key) != handoff[key] or receipt.get(key) != handoff[key]:
            raise ValueError('dependency identity mismatch: ' + key)
    if not manifest.get('completed_at') or manifest['completed_at'] != receipt.get('completed_at'):
        raise ValueError('dependency completion identity mismatch')
    text_receipt = dict(line.split(': ', 1) for line in (metadata / 'build-receipt.txt').read_text().splitlines())
    for label, key in [('CMake', 'cmake_version'), ('Compiler', 'compiler'), ('SDK', 'sdk_version'),
                       ('Architecture', 'architecture'), ('Deployment target', 'deployment_target'), ('Build type', 'build_type')]:
        if text_receipt.get(label) != receipt['toolchain'].get(key):
            raise ValueError('dependency receipt text/JSON mismatch: ' + label)
    for key, value in dict(architecture='arm64', build_type='Release', sdk_version=handoff['sdk_version'],
                           deployment_target=handoff['deployment_target']).items():
        if receipt['toolchain'].get(key) != value:
            raise ValueError('dependency toolchain differs from selected build: ' + key)
    # This is a build output, not metadata inferred from the calendar or Git tags.
    version_path = (build / 'source/version_info/package-version.json').resolve(strict=True)
    if (build / 'macos/Release' / handoff['package_version']).resolve(strict=True) != version_path:
        raise ValueError('unexpected package version metadata location')
    package_version = json.loads(version_path.read_text())
    if package_version.get('schema') != 1 or package_version.get('dirty') is not False:
        raise ValueError('package requires schema-1 version metadata from clean committed source')
    for key in ('apple_short_version', 'apple_bundle_version'):
        value = package_version.get(key, '')
        if not re.fullmatch(r'\d{1,4}\.\d{1,2}\.\d{1,2}', value) or numeric_version(value)[0] == 0:
            raise ValueError('invalid Apple numeric version: ' + key)
    source = clean_tree(handoff['source_directory'], package_version['full_revision'])
    executable = selected_path(handoff['executable'].strip(), build)
    if not executable.is_file() or not os.access(executable, os.X_OK) or executable.name in ('.', '..'):
        raise ValueError('handoff executable is missing or not executable')
    if len(handoff['plugins']) != 5 or {p['name'] for p in handoff['plugins']} != PLUGINS:
        raise ValueError('native profile requires exactly four OGRE plugins and Caelum')
    selected = {}
    for plugin in [*handoff['plugins'], *handoff['dynamic_modules']]:
        path = selected_path(plugin['source'], prefix)
        destination = str(relative(plugin['destination']))
        if plugin.get('required') is not True:
            raise ValueError('all declared dynamic modules must be required')
        if plugin['name'] == 'Caelum':
            expected = 'Contents/Frameworks/' + path.name
            if set(plugin['roles']) != {'linked', 'plugin'}:
                raise ValueError('Caelum requires linked and plugin roles on one image')
        else:
            expected = 'Contents/PlugIns/OGRE/' + path.name
        if destination != expected or path in selected:
            raise ValueError('unexpected/duplicate dynamic module destination: ' + destination)
        selected[path] = destination
    if handoff['bundle_plugin_folder'] != '../PlugIns/OGRE':
        raise ValueError('unexpected bundle plugin folder')
    for key in ('resources', 'content', 'languages'):
        root = Path(handoff['resource_roots'][key])
        if root.name != key or root.parent.resolve(strict=True) != (build / 'bin').resolve(strict=True) or not root.is_dir() or not any(root.iterdir()):
            raise ValueError('missing exact build resource root: ' + key)
        check_links(root)
    rtss = selected_path(handoff['rtshader_glsl_root'], prefix)
    if not rtss.is_dir() or not any(rtss.iterdir()):
        raise ValueError('missing promised RTShader GLSL payload')
    return handoff, package_version, source, executable, manifest, selected, rtss


def copy_payload(source, destination):
    check_links(source)
    shutil.copytree(source, destination, symlinks=True)


def collect_notices(app, source, prefix, manifest):
    target = app / 'Contents/Resources/Notices'
    target.mkdir(parents=True)
    def copy(path, name):
        if not path.is_file():
            raise ValueError('missing selected notice: ' + str(path))
        output = target / str(relative(name))
        output.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, output)
    for name, component in sorted(manifest['components'].items()):
        if not component.get('installed_notices'):
            raise ValueError('component lacks notices: ' + name)
        for notice in component['installed_notices']:
            path = selected_path(prefix / 'share/ror-dependencies' / str(relative(notice)), prefix)
            copy(path, 'dependencies/' + notice)
    for name in ROR_NOTICES:
        copy(source / name, 'rigs-of-rods/' + name)
    # Retain all UTF8 and AngelScript addon source notice headers, not a guessed MIT text.
    for folder in ('external/header_only/utf8', 'external/angelscript_addons'):
        directory = source / folder
        if directory.is_dir():
            for path in directory.rglob('*'):
                if path.is_file() and path.suffix in ('.h', '.hpp', '.cpp', '.txt', '.md'):
                    copy(path, 'rigs-of-rods/' + path.relative_to(source).as_posix())
    copy(source / 'content/LICENSE', 'base-content/LICENSE')
    for filename in FONT_NOTICES:
        copy(source / 'resources/fonts' / filename, 'fonts/' + filename)
    for filename, expected in FONT_HASHES.items():
        if digest(source / 'resources/fonts' / filename) != expected:
            raise ValueError('font bytes differ from reviewed notice mapping: ' + filename)
    write_json(target / 'dependency-provenance.json', manifest)
    return target


def rewrite(app, images, edges, mapping):
    for source, destination in mapping.items():
        path = app / destination
        image = images[source]
        args = ['install_name_tool']
        if image.identity:
            args += ['-id', '@rpath/' + str(Path(destination).relative_to('Contents/Frameworks'))] if destination.startswith('Contents/Frameworks/') else ['-id', '@loader_path/' + path.name]
        for command, name in image.loads:
            target = edges[(source, name)]
            if target is not None:
                rewritten = '@loader_path/' + os.path.relpath(app / mapping[target], path.parent)
                args += ['-change', name, rewritten]
        for rpath in dict.fromkeys(image.rpaths):
            args += ['-delete_rpath', rpath]
        args += ['-add_rpath', '@loader_path/' + os.path.relpath(app / 'Contents/Frameworks', path.parent), str(path)]
        subprocess.run(args, check=True)


def sign(app, mapping):
    for destination in sorted(mapping.values(), key=lambda x: (-len(Path(x).parts), x)):
        path = app / destination
        # codesign recognizes a main image's containing bundle. Its enclosing
        # framework/app is signed explicitly after the nested libraries.
        if framework(path) or destination.startswith('Contents/MacOS/'):
            continue
        subprocess.run(['codesign', '--force', '--sign', '-', '--timestamp=none', str(path)], check=True)
    roots = {framework(app / destination) for destination in mapping.values()}
    for root in sorted((p for p in roots if p), key=lambda p: -len(p.parts)):
        subprocess.run(['codesign', '--force', '--sign', '-', '--timestamp=none', str(root)], check=True)


def payload_inventory(app):
    result = {}
    for path in sorted(app.rglob('*')):
        name = path.relative_to(app).as_posix()
        if '_CodeSignature' in path.parts or name == 'Contents/Resources/artifact-inventory.json':
            continue
        if path.is_symlink():
            result[name] = dict(symlink=os.readlink(path))
        elif path.is_file() and not is_macho(path):
            result[name] = dict(sha256=digest(path), mode=path.stat().st_mode & 0o777)
    return result


def make_icon(source, resources, temporary):
    icon = source / 'resources/icons/ror.png'
    if not icon.is_file():
        raise ValueError('missing committed project icon resources/icons/ror.png')
    iconset = temporary / 'ror.iconset'
    iconset.mkdir()
    for size in (16, 32, 128, 256, 512):
        for scale in (1, 2):
            name = f'icon_{size}x{size}' + ('@2x' if scale == 2 else '') + '.png'
            subprocess.run(['sips', '-z', str(size * scale), str(size * scale), str(icon), '--out', str(iconset / name)], check=True, stdout=subprocess.DEVNULL)
    subprocess.run(['iconutil', '--convert', 'icns', '--output', str(resources / 'ror.icns'), str(iconset)], check=True)


def protect_output(output, roots, replace):
    if output.is_symlink():
        raise ValueError('output cannot be a symlink')
    output = output.resolve()
    for root in roots:
        root = Path(root).resolve(strict=True)
        if within(output, root) or within(root, output):
            raise ValueError('output overlaps protected source/build/prefix/cache/recipe: ' + str(root))
    if output.exists():
        marker = output / '.ror-package-owner.json'
        if not replace or not marker.is_file() or json.loads(marker.read_text()) != dict(owner=OWNER, output=str(output)):
            raise ValueError('output exists; --replace only accepts this tool\'s prior output at this exact path')
    return output


def package(args):
    build, prefix = args.build_dir.resolve(strict=True), args.deps_prefix.resolve(strict=True)
    handoff, version, source, executable, manifest, selected, rtss = validate_inputs(build, prefix, args.config)
    inputs = source_inputs(args.source_cache, args.recipe_source, source, version, handoff, manifest)
    protected = [build, prefix, source, *[Path(p).resolve() for p in (args.source_cache, args.recipe_source) if p]]
    for left, right in ((build, prefix), (build, source), (prefix, source)):
        if within(left, right) or within(right, left):
            raise ValueError('source, build, and dependency prefix must be disjoint')
    output = protect_output(args.output, protected, args.replace)
    if not output.parent.is_dir():
        raise ValueError('output parent must already exist')
    if shutil.disk_usage(output.parent).free < 4 * 1024 ** 3:
        raise ValueError('at least 4 GiB free space is required for staging and extracted-ZIP verification')
    reader = lambda path: inspect(path, handoff['deployment_target'], handoff['sdk_version'])
    aliases = {}
    for row in handoff.get('framework_sources', []):
        root = selected_path(row['source_root'], prefix)
        image = selected_path(row['source_image'], prefix)
        destination = 'Contents/Frameworks/' + root.name + '/' + image.relative_to(root).as_posix()
        if framework(image) != root or row['destination_root'] != 'Contents/Frameworks/' + root.name or row['destination_image'] != destination:
            raise ValueError('invalid declared framework source/destination identity')
        # MyGUI's imported framework uses its future app location even in the raw
        # build. This exact declared identity is the sole non-filesystem mapping.
        alias = '@executable_path/../Frameworks/' + str(Path(destination).relative_to('Contents/Frameworks'))
        if alias in aliases and aliases[alias] != image:
            raise ValueError('conflicting declared framework identities')
        aliases[alias] = image
    images, edges = closure(executable, list(selected), (build, prefix), reader, aliases)
    mapping = destinations(images, executable, selected)
    for row in handoff.get('framework_sources', []):
        image, root = Path(row['source_image']).resolve(strict=True), Path(row['source_root']).resolve(strict=True)
        expected_root = 'Contents/Frameworks/' + root.name
        if framework(image) != root or image not in mapping or row['destination_root'] != expected_root or row['destination_image'] != mapping[image]:
            raise ValueError('declared framework differs from resolved dependency closure')
    mygui = [p for p in images if framework(p) and framework(p).name == 'MyGUIEngine.framework']
    if len(mygui) != 1 or not handoff.get('framework_sources'):
        raise ValueError('native profile requires the declared versioned MyGUIEngine framework')
    with tempfile.TemporaryDirectory(prefix='.ror-package-', dir=output.parent) as temporary:
        temporary = Path(temporary)
        publish = temporary / 'publish'
        publish.mkdir()
        app = publish / 'Rigs of Rods.app'
        (app / 'Contents/MacOS').mkdir(parents=True)
        (app / 'Contents/Resources').mkdir()
        runtime = app / 'Contents/Resources/runtime'
        runtime.mkdir()
        copied_frameworks = set()
        for path, destination in mapping.items():
            root = framework(path)
            if root:
                if root not in copied_frameworks:
                    copy_payload(root, app / 'Contents/Frameworks' / root.name)
                    copied_frameworks.add(root)
            else:
                target = app / destination
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(path, target)
        for name, root in handoff['resource_roots'].items():
            copy_payload(Path(root), runtime / name)
            (app / 'Contents/MacOS' / name).symlink_to('../Resources/runtime/' + name)
        rtss_destination = runtime / 'resources/RTShaderLib/GLSL'
        if rtss_destination.exists():
            raise ValueError('build resources already contain RTShaderLib/GLSL; require one declared payload')
        rtss_destination.parent.mkdir(parents=True, exist_ok=True)
        copy_payload(rtss, rtss_destination)
        cfg = ['PluginFolder=' + handoff['bundle_plugin_folder']]
        for row in handoff['plugins']:
            entry = row['bundle_cfg_entry']
            if not entry.startswith('Plugin=') or any(c in entry for c in '\r\n\0'):
                raise ValueError('invalid plugin config entry')
            resolved = (app / 'Contents/MacOS' / handoff['bundle_plugin_folder'] / entry[len('Plugin='):]).resolve()
            if resolved != (app / row['destination']).resolve() or not within(resolved, app.resolve()):
                raise ValueError('plugin config does not resolve to recorded destination')
            cfg.append(entry)
        (runtime / 'plugins.cfg').write_text('\n'.join(cfg) + '\n')
        (app / 'Contents/MacOS/plugins.cfg').symlink_to('../Resources/runtime/plugins.cfg')
        notices = collect_notices(app, source, prefix, manifest)
        make_icon(source, app / 'Contents/Resources', temporary)
        plist = dict(CFBundleName='Rigs of Rods', CFBundleDisplayName='Rigs of Rods', CFBundleIdentifier='org.rigsofrods.RigsOfRods',
                     CFBundlePackageType='APPL', CFBundleExecutable=executable.name, CFBundleIconFile='ror.icns',
                     CFBundleShortVersionString=version['apple_short_version'], CFBundleVersion=version['apple_bundle_version'],
                     LSMinimumSystemVersion=handoff['deployment_target'], NSHighResolutionCapable=True,
                     RoRDisplayVersion=version['display_version'], RoRFullRevision=version['full_revision'])
        (app / 'Contents/Info.plist').write_bytes(plistlib.dumps(plist))
        write_json(app / 'Contents/Resources/package-version.json', version)
        rewrite(app, images, edges, mapping)
        # Copied framework payload may not hide unselected executable images.
        copied = {p.resolve() for p in app.rglob('*') if is_macho(p)}
        if copied != {(app / d).resolve() for d in mapping.values()}:
            raise ValueError('framework/resource payload contains code outside the selected closure')
        for path in app.rglob('*'):
            if not path.is_symlink():
                subprocess.run(['xattr', '-c', str(path)], check=True, stdout=subprocess.DEVNULL)
        sign(app, mapping)
        source_zip, source_inventory = source_archive(publish, inputs, handoff, version, manifest, notices)
        from verify import resource_inventory
        inventory = dict(schema=1, profile=handoff['profile'], configuration='Release', architecture='arm64',
                         signing='ad-hoc', runtime_tested=False, version=version, content_revision=handoff['content_revision'],
                         recipe_revision=manifest['recipe_revision'], toolchain=dict(sdk_version=handoff['sdk_version'], deployment_target=handoff['deployment_target']),
                         dependency_manifest_sha256=handoff['dependency_manifest_sha256'],
                         source_inventory_sha256=manifest['source_inventory_sha256'], configuration_sha256=manifest['configuration_sha256'],
                         effective_features=handoff['effective_features'], required_features=handoff['required_features'],
                         executable=mapping[executable], plugins=[{k: p[k] for k in ('name', 'destination', 'roles', 'required', 'bundle_cfg_entry')} for p in handoff['plugins']],
                         dynamic_modules=[mapping[p] for p in selected if p not in {Path(r['source']).resolve() for r in handoff['plugins']}],
                         images=sorted(mapping.values()),
                         edges=[dict(loader=mapping[p], load=('system:' + n if t is None else mapping[t])) for (p, n), t in edges.items()],
                         source_companion=dict(filename=source_zip.name, sha256=digest(source_zip)), payload=payload_inventory(app),
                         resource_archives=resource_inventory(runtime / 'resources'))
        write_json(app / 'Contents/Resources/artifact-inventory.json', inventory)
        subprocess.run(['codesign', '--force', '--sign', '-', '--timestamp=none', str(app)], check=True)
        subprocess.run(['codesign', '--verify', '--strict', '--deep', str(app)], check=True)
        archive = publish / 'Rigs-of-Rods-macOS-arm64.zip'
        subprocess.run(['ditto', '-c', '-k', '--sequesterRsrc', '--keepParent', str(app), str(archive)], check=True)
        # The independent verifier inspects a fresh extraction of the final ZIP.
        from verify import verify_archive
        verify_archive(archive)
        write_json(publish / 'artifact-inventory.json', inventory)
        (publish / 'SHA256SUMS').write_text(''.join(digest(p) + '  ' + p.name + '\n' for p in (archive, source_zip)))
        write_json(publish / '.ror-package-owner.json', dict(owner=OWNER, output=str(output)))
        backup = temporary / 'previous'
        # Recheck after staging: a concurrent writer may have created/replaced it.
        protect_output(output, protected, args.replace)
        if output.exists():
            output.rename(backup)
        try:
            publish.rename(output)
        except BaseException:
            if backup.exists():
                backup.rename(output)
            raise
    print('Created and independently verified: ' + str(output / archive.name))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('build-dir', 'deps-prefix', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--config', default='Release')
    parser.add_argument('--source-cache', type=Path)
    parser.add_argument('--recipe-source', type=Path)
    parser.add_argument('--replace', action='store_true')
    package(parser.parse_args())


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        sys.exit(str(error))
