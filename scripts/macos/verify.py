#!/usr/bin/env python3
"""Independently inspect an extracted native macOS ZIP, without launching it."""
import argparse
import json
import os
from pathlib import Path, PurePosixPath
import plistlib
import posixpath
import stat
import subprocess
import sys
import tempfile
import zipfile

from macho import (check_links, closure, framework, inspect, is_macho, relative,
                   run, system, within)
from source_companion import digest


def resource_inventory(root):
    """Inventory ZIP bytes/members and check the exact reviewed runtime fonts."""
    import hashlib
    from package import FONT_HASHES
    archives, fonts = {}, {}
    root = root.resolve(strict=True)
    for path in sorted(root.rglob('*')):
        if not path.is_file():
            continue
        if path.name in FONT_HASHES:
            fonts.setdefault(path.name, []).append(digest(path))
        if path.suffix.lower() != '.zip':
            continue
        entries, folded = [], set()
        with zipfile.ZipFile(path) as archive:
            for entry in archive.infolist():
                name = str(relative(entry.filename.rstrip('/')))
                if name.casefold() in folded or entry.flag_bits & 1:
                    raise ValueError('duplicate/case collision or encrypted resource ZIP entry: ' + name)
                folded.add(name.casefold())
                if stat.S_ISLNK(entry.external_attr >> 16):
                    raise ValueError('resource ZIP cannot contain symlinks: ' + name)
                entries.append(dict(name=name, size=entry.file_size, crc32=entry.CRC))
                if PurePosixPath(name).name in FONT_HASHES:
                    h = hashlib.sha256()
                    with archive.open(entry) as stream:
                        for block in iter(lambda: stream.read(1024 * 1024), b''):
                            h.update(block)
                    fonts.setdefault(PurePosixPath(name).name, []).append(h.hexdigest())
        archives[path.relative_to(root).as_posix()] = dict(sha256=digest(path), entries=entries)
    for name, expected in FONT_HASHES.items():
        if not fonts.get(name) or any(h != expected for h in fonts[name]):
            raise ValueError('runtime font is absent or differs from reviewed notice mapping: ' + name)
    return archives


def archive_layout(archive):
    names, links, apps, folded = set(), set(), set(), {}
    with zipfile.ZipFile(archive) as z:
        for entry in z.infolist():
            name = entry.filename.rstrip('/')
            path = relative(name)
            if name in names:
                raise ValueError('duplicate ZIP entry: ' + name)
            names.add(name)
            previous = folded.setdefault(name.casefold(), name)
            if previous != name:
                raise ValueError('case-insensitive ZIP collision: ' + name)
            top = path.parts[0]
            if top.endswith('.app'):
                apps.add(top)
            elif top != '__MACOSX':
                raise ValueError('unexpected top-level ZIP entry: ' + top)
            mode = entry.external_attr >> 16
            if stat.S_ISLNK(mode):
                links.add(name)
                if entry.file_size > 4096:
                    raise ValueError('oversized ZIP symlink')
                target = z.read(entry).decode('utf-8')
                if not target or target.startswith('/') or '\\' in target or any(c in target for c in '\r\n\0'):
                    raise ValueError('unsafe ZIP symlink: ' + name)
                resolved = posixpath.normpath(posixpath.join(posixpath.dirname(name), target))
                if resolved != top and not resolved.startswith(top + '/'):
                    raise ValueError('ZIP symlink escapes app: ' + name)
            elif mode and not (stat.S_ISREG(mode) or stat.S_ISDIR(mode)):
                raise ValueError('unsupported ZIP file type: ' + name)
        for name in names:
            if any(str(parent) in links for parent in PurePosixPath(name).parents):
                raise ValueError('ZIP entry would write through a symlink: ' + name)
    if len(apps) != 1:
        raise ValueError('ZIP must contain exactly one top-level app')
    return apps.pop()


def verify_bundle(app, source_directory):
    check_links(app)
    metadata = app / 'Contents/Resources'
    inventory = json.loads((metadata / 'artifact-inventory.json').read_text())
    version = json.loads((metadata / 'package-version.json').read_text())
    plist = plistlib.loads((app / 'Contents/Info.plist').read_bytes())
    if inventory.get('schema') != 1 or inventory.get('profile') != 'native-macos-gl3' or inventory.get('architecture') != 'arm64' or inventory.get('configuration') != 'Release':
        raise ValueError('unsupported bundle inventory identity')
    if version != inventory['version'] or version.get('dirty') is not False:
        raise ValueError('version inventory mismatch or dirty source')
    for key, value in [('CFBundleShortVersionString', version['apple_short_version']), ('CFBundleVersion', version['apple_bundle_version']),
                       ('RoRFullRevision', version['full_revision']), ('RoRDisplayVersion', version['display_version']),
                       ('LSMinimumSystemVersion', inventory['toolchain']['deployment_target'])]:
        if plist.get(key) != value:
            raise ValueError('Info.plist identity mismatch: ' + key)
    from package import FEATURES, FONT_HASHES, FONT_NOTICES, PLUGINS
    if not FEATURES.issubset(inventory['required_features']) or any(inventory['effective_features'].get(k) is not True for k in FEATURES):
        raise ValueError('bundle is missing required effective features')
    if inventory.get('signing') != 'ad-hoc' or inventory.get('runtime_tested') is not False:
        raise ValueError('unexpected package validation/signing claim')
    # Check only distributable metadata, configs, load commands and RPATHs for private
    # machine paths. Binary source/debug string content is not a path contract.
    for path in (metadata / 'artifact-inventory.json', metadata / 'package-version.json', app / 'Contents/Info.plist',
                 metadata / 'Notices/dependency-provenance.json', app / 'Contents/MacOS/plugins.cfg'):
        text = path.read_text()
        if any(token in text for token in ('/Users/', '/Volumes/', '/private/', '/opt/homebrew/', '/usr/local/')):
            raise ValueError('private/host path in redistributable metadata: ' + str(path.relative_to(app)))
    actual_payload = set()
    for path in app.rglob('*'):
        name = path.relative_to(app).as_posix()
        if '_CodeSignature' not in path.parts and name != 'Contents/Resources/artifact-inventory.json' and (path.is_symlink() or (path.is_file() and not is_macho(path))):
            actual_payload.add(name)
    if actual_payload != set(inventory['payload']):
        raise ValueError('extracted non-code payload inventory differs from package')
    for name, record in inventory['payload'].items():
        path = app / str(relative(name))
        if not within(path.resolve(strict=True), app.resolve()):
            raise ValueError('payload escapes app: ' + name)
        if 'symlink' in record:
            if not path.is_symlink() or os.readlink(path) != record['symlink']:
                raise ValueError('payload symlink changed: ' + name)
        elif path.is_symlink() or digest(path) != record['sha256'] or (path.stat().st_mode & 0o777) != record['mode']:
            raise ValueError('payload hash/mode changed: ' + name)
    plugins = inventory['plugins']
    if len(plugins) != 5 or {p['name'] for p in plugins} != PLUGINS:
        raise ValueError('missing exact required plugin set')
    cfg = (app / 'Contents/MacOS/plugins.cfg').read_text().splitlines()
    expected_cfg = ['PluginFolder=../PlugIns/OGRE'] + [p['bundle_cfg_entry'] for p in plugins]
    if cfg != expected_cfg:
        raise ValueError('plugin config differs from bundled selected plugins')
    for plugin in plugins:
        target = app / str(relative(plugin['destination']))
        entry = plugin['bundle_cfg_entry']
        if not entry.startswith('Plugin='):
            raise ValueError('invalid bundle plugin entry')
        configured = app / 'Contents/PlugIns/OGRE' / entry[len('Plugin='):]
        if target.resolve(strict=True) != configured.resolve(strict=True) or plugin.get('required') is not True:
            raise ValueError('plugin config target mismatch')
        if plugin['name'] == 'Caelum':
            if not plugin['destination'].startswith('Contents/Frameworks/') or set(plugin['roles']) != {'linked', 'plugin'}:
                raise ValueError('Caelum must have one linked/plugin identity')
        elif not plugin['destination'].startswith('Contents/PlugIns/OGRE/'):
            raise ValueError('OGRE plugin outside canonical location')
    executable = app / str(relative(inventory['executable']))
    if inventory['executable'] != 'Contents/MacOS/' + plist['CFBundleExecutable'] or not os.access(executable, os.X_OK):
        raise ValueError('executable path/mode differs from Info.plist')
    seeds = [app / p['destination'] for p in plugins] + [app / str(relative(p)) for p in inventory['dynamic_modules']]
    reader = lambda path: inspect(path, inventory['toolchain']['deployment_target'], inventory['toolchain']['sdk_version'])
    images, edges = closure(executable, seeds, (app,), reader)
    actual = {p.relative_to(app).as_posix() for p in images}
    all_code = {p.relative_to(app).as_posix() for p in app.rglob('*') if is_macho(p)}
    if actual != set(inventory['images']) or all_code != actual:
        raise ValueError('extracted code inventory differs from resolved closure')
    recorded_edges = {(e['loader'], e['load']) for e in inventory['edges']}
    actual_edges = {(p.relative_to(app).as_posix(), 'system:' + n if t is None else t.relative_to(app).as_posix()) for (p, n), t in edges.items()}
    if actual_edges != recorded_edges:
        raise ValueError('extracted dependency graph differs from selected closure')
    for path, image in images.items():
        for command, name in image.loads:
            if not system(name) and not name.startswith(('@loader_path/', '@executable_path/', '@rpath/')):
                raise ValueError('external non-system load command: ' + name)
        if image.identity and not image.identity.startswith(('@rpath/', '@loader_path/')):
            raise ValueError('external dylib install identity: ' + image.identity)
        for rpath in image.rpaths:
            if not rpath.startswith(('@loader_path/', '@executable_path/')):
                raise ValueError('external/unsupported RPATH: ' + rpath)
            from macho import expand
            if not within(expand(rpath, path, executable), app.resolve()):
                raise ValueError('RPATH escapes extracted app')
        subprocess.run(['codesign', '--verify', '--strict', str(path)], check=True)
    frameworks = {framework(path) for path in images if framework(path)}
    mygui = [p for p in frameworks if p.name == 'MyGUIEngine.framework']
    if len(mygui) != 1 or not (mygui[0] / 'Versions/Current').is_symlink():
        raise ValueError('missing versioned MyGUI framework/current symlink')
    for path in frameworks:
        if not (path / 'Resources/Info.plist').is_file():
            raise ValueError('framework lacks required Info.plist')
        subprocess.run(['codesign', '--verify', '--strict', str(path)], check=True)
    for root in ('resources', 'content', 'languages'):
        path = app / 'Contents/MacOS' / root
        if not path.is_dir() or not any(path.iterdir()):
            raise ValueError('missing required runtime resources: ' + root)
    if not (app / 'Contents/MacOS/resources/RTShaderLib/GLSL').is_dir():
        raise ValueError('missing declared RTShader GLSL payload')
    if resource_inventory(app / 'Contents/MacOS/resources') != inventory['resource_archives']:
        raise ValueError('runtime resource ZIP bytes/member inventory changed')
    for name in FONT_NOTICES:
        if not (metadata / 'Notices/fonts' / name).is_file():
            raise ValueError('missing reviewed font notice: ' + name)
    for name, expected in FONT_HASHES.items():
        # Resources may be ZIPs; exact font mapping was checked against clean source
        # during assembly, and its build resource bytes are inventoried/signature sealed.
        fonts = list((app / 'Contents/MacOS/resources').rglob(name))
        for path in fonts:
            if digest(path) != expected:
                raise ValueError('font notice mapping differs from payload')
    dependency = json.loads((metadata / 'Notices/dependency-provenance.json').read_text())
    if len(dependency['components']) != 16 or dependency['recipe_revision'] != inventory['recipe_revision']:
        raise ValueError('dependency notice/source provenance identity mismatch')
    for component in dependency['components'].values():
        if not component.get('installed_notices'):
            raise ValueError('dependency lacks selected notices')
        for name in component['installed_notices']:
            if not (metadata / 'Notices/dependencies' / str(relative(name))).is_file():
                raise ValueError('missing selected dependency notice: ' + name)
    subprocess.run(['codesign', '--verify', '--strict', '--deep', str(app)], check=True)
    details = run('codesign', '-d', '--verbose=4', app)
    if 'Signature=adhoc' not in details:
        raise ValueError('bundle is not ad hoc signed')
    companion = inventory['source_companion']
    source = source_directory / str(relative(companion['filename']))
    if source.name != companion['filename'] or digest(source) != companion['sha256']:
        raise ValueError('matching source companion is missing or changed')
    verify_source(source, inventory, dependency)
    return inventory


def verify_source(path, inventory, dependency):
    import tarfile
    with tarfile.open(path, 'r:gz') as archive:
        members = archive.getmembers()
        names = {}
        for member in members:
            relative(member.name)
            if member.name in names or PurePosixPath(member.name).parts[0] != 'corresponding-source':
                raise ValueError('invalid/duplicate source archive entry')
            if not (member.isfile() or member.isdir()):
                raise ValueError('source companion outer archive must contain regular files/directories')
            names[member.name] = member
        stream = archive.extractfile(names['corresponding-source/source-inventory.json'])
        source = json.load(stream)
        if source['source_revision'] != inventory['version']['full_revision'] or source['content_revision'] != inventory['content_revision'] or source['recipe_revision'] != inventory['recipe_revision']:
            raise ValueError('source companion does not match binary source identities')
        if set(source['components']) != set(dependency['components']) or source['source_inventory_sha256'] != inventory['source_inventory_sha256'] or source['configuration_sha256'] != inventory['configuration_sha256']:
            raise ValueError('source companion inventory identity differs from binary')
        expected_trees = {'rigs-of-rods': source['source_revision'], 'base-content': source['content_revision'], 'dependency-recipe': source['recipe_revision']}
        if {name: tree['revision'] for name, tree in source['trees'].items()} != expected_trees:
            raise ValueError('source companion lacks exact committed tree identities')
        import hashlib
        for name, component in dependency['components'].items():
            row = source['components'][name]
            if any(row.get(k) != v for k, v in component.items()):
                raise ValueError('source companion dependency selection differs: ' + name)
            member = names['corresponding-source/download-cache/' + str(relative(row['cache_path']))]
            h = hashlib.sha256()
            with archive.extractfile(member) as stream:
                for block in iter(lambda: stream.read(1024 * 1024), b''):
                    h.update(block)
            if h.hexdigest() != component['sha256']:
                raise ValueError('source archive hash mismatch: ' + name)
        for name, tree in source['trees'].items():
            h = hashlib.sha256()
            with archive.extractfile(names['corresponding-source/' + name + '.tar']) as stream:
                for block in iter(lambda: stream.read(1024 * 1024), b''):
                    h.update(block)
            if h.hexdigest() != tree['sha256']:
                raise ValueError('committed source tree archive changed: ' + name)


def verify_archive(archive):
    archive = Path(archive).resolve(strict=True)
    app_name = archive_layout(archive)
    with tempfile.TemporaryDirectory(prefix='.ror-verify-', dir=archive.parent) as directory:
        subprocess.run(['ditto', '-x', '-k', str(archive), directory], check=True)
        root = Path(directory)
        apps = list(root.glob('*.app'))
        if len(apps) != 1 or apps[0].name != app_name:
            raise ValueError('extracted app layout differs from archive')
        return verify_bundle(apps[0], archive.parent)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    args = parser.parse_args()
    result = verify_archive(args.archive)
    print('Verified extracted ZIP: arm64, closed load graph, strict ad hoc signatures, resources/notices, matching source companion; runtime untested. Revision ' + result['version']['full_revision'])


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError, zipfile.BadZipFile) as error:
        sys.exit(str(error))
