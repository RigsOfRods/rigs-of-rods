"""Collect committed matching source and verified cached archives, without downloads."""
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tarfile

from macho import check_links, relative, run, within

TOOL_FILES = ('scripts/macos/package.py', 'scripts/macos/verify.py',
              'scripts/macos/macho.py', 'scripts/macos/source_companion.py',
              'doc/macOS-packaging.md', 'tests/macos/test_package.py')


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def clean_tree(path, expected):
    path = Path(path).resolve(strict=True)
    actual = run('git', '-C', path, 'rev-parse', 'HEAD')
    if not re.fullmatch(r'[0-9a-f]{40}', expected) or actual != expected:
        raise ValueError(f'source revision mismatch: {path}: {actual}, expected {expected}')
    if run('git', '-C', path, 'status', '--porcelain', '--untracked-files=all', '--ignore-submodules=none'):
        raise ValueError('source tree must be clean, including untracked files: ' + str(path))
    return path


def source_inputs(cache, recipe, source, version, handoff, manifest, packaging_source=None):
    if cache is None or recipe is None:
        raise ValueError('publication requires --source-cache (existing verified EP download cache) and --recipe-source (clean matching recipe checkout); no downloads are performed')
    source = clean_tree(source, version['full_revision'])
    content = clean_tree(source / 'content', handoff['content_revision'])
    gitlink = run('git', '-C', source, 'ls-tree', 'HEAD', 'content').split()
    if gitlink[:3] != ['160000', 'commit', handoff['content_revision']]:
        raise ValueError('source/content identity differs from committed gitlink')
    recipe = clean_tree(recipe, manifest['recipe_revision'])
    tools = Path(packaging_source or Path(__file__).resolve().parents[2]).resolve(strict=True)
    tool_revision = run('git', '-C', tools, 'rev-parse', 'HEAD')
    tools = clean_tree(tools, tool_revision)
    for filename in TOOL_FILES:
        path = tools / filename
        # Archive the committed revision, never untracked working tree files.
        committed = subprocess.check_output(['git', '-C', str(tools), 'show', tool_revision + ':' + filename])
        if not path.is_file() or path.is_symlink() or committed != path.read_bytes():
            raise ValueError('packaging source must contain the committed tool file: ' + filename)
        live = Path(__file__).resolve().parents[2] / filename
        if path.read_bytes() != live.read_bytes():
            raise ValueError('selected packaging source differs from the running packaging tools: ' + filename)
    inventory = recipe / 'cmake/macos-sources.json'
    if digest(inventory) != manifest['source_inventory_sha256']:
        raise ValueError('selected recipe source inventory differs from completed dependency prefix')
    original = json.loads(inventory.read_text())
    if set(original['components']) != set(manifest['components']):
        raise ValueError('selected recipe component inventory differs from prefix')
    for name, component in original['components'].items():
        if any(manifest['components'][name].get(key) != value for key, value in component.items()):
            raise ValueError('selected recipe component metadata differs from prefix: ' + name)
    components = manifest['components']
    if len(components) != 16:
        raise ValueError('native profile requires exactly 16 selected dependency source archives')
    cache = Path(cache).resolve(strict=True)
    needed = {c['sha256'] for c in components.values()}
    if len(needed) != 16 or any(not re.fullmatch(r'[0-9a-f]{64}', h) for h in needed):
        raise ValueError('invalid source archive SHA256 inventory')
    found = {}
    # The caller explicitly selects the bounded EP download-cache directory.
    # Match bytes, not potentially ambiguous URL basenames or mutable tags.
    for path in sorted(cache.rglob('*')):
        if path.is_symlink():
            raise ValueError('download cache cannot contain symlinks: ' + str(path))
        if path.is_file():
            h = digest(path)
            if h in needed:
                found.setdefault(h, path)
    missing = [name for name, c in components.items() if c['sha256'] not in found]
    if missing:
        raise ValueError('source cache is missing hash-matching original archives: ' + ', '.join(missing))
    for name, component in components.items():
        for patch in component['patches']:
            path = recipe / str(relative(patch['path']))
            if not within(path.resolve(strict=True), recipe) or digest(path) != patch['sha256']:
                raise ValueError('recipe patch identity mismatch: ' + patch['path'])
        if not component.get('installed_notices'):
            raise ValueError('component lacks selected notice mapping: ' + name)
    return source, content, recipe, cache, found, tools, tool_revision


def create(destination, inputs, handoff, version, manifest, notices):
    source, content, recipe, cache, found, tools, tool_revision = inputs
    work = destination / 'corresponding-source'
    work.mkdir()
    trees = [('rigs-of-rods', source, version['full_revision']),
             ('base-content', content, handoff['content_revision']),
             ('dependency-recipe', recipe, manifest['recipe_revision'])]
    for name, path, revision in trees:
        # Nested git archives retain original modes and contained source links,
        # exclude .git/untracked/ignored build fixtures, and need no tar extraction.
        subprocess.run(['git', '-C', str(path), 'archive', '--format=tar',
                        '--prefix=' + name + '/', '-o', str(work / (name + '.tar')), revision], check=True)
    subprocess.run(['git', '-C', str(tools), 'archive', '--format=tar',
                    '--prefix=packaging-tools/', '-o', str(work / 'packaging-tools.tar'),
                    tool_revision, *TOOL_FILES], check=True)
    trees.append(('packaging-tools', tools, tool_revision))
    records = {}
    for name, component in sorted(manifest['components'].items()):
        original = found[component['sha256']]
        cache_path = original.relative_to(cache).as_posix()
        relative(cache_path)
        target = work / 'download-cache' / cache_path
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(original, target)
        if digest(target) != component['sha256']:
            raise ValueError('source archive changed during copy: ' + name)
        records[name] = dict(component, cache_path=cache_path)
    shutil.copytree(notices, work / 'notices', symlinks=True)
    inventory = dict(schema=1, source_revision=version['full_revision'], content_revision=handoff['content_revision'],
                     recipe_revision=manifest['recipe_revision'], source_inventory_sha256=manifest['source_inventory_sha256'],
                     packaging_revision=tool_revision,
                     packaging_files={name: digest(tools / name) for name in TOOL_FILES},
                     configuration_sha256=manifest['configuration_sha256'], components=records,
                     trees={name: {'revision': revision, 'sha256': digest(work / (name + '.tar'))}
                            for name, path, revision in trees})
    (work / 'source-inventory.json').write_text(json.dumps(inventory, indent=2, sort_keys=True) + '\n')
    (work / 'REBUILD.md').write_text('''# Corresponding source and rebuild inputs

The four TAR files contain exact committed source inputs (no Git internals).
Extract rigs-of-rods.tar, base-content.tar, dependency-recipe.tar and
packaging-tools.tar. The last archive contains the selected committed packaging
scripts, documentation and focused tests, recorded by packaging_revision and
tree hashes; it need not be part of the compiled game source revision.
Run the packager/verifier from packaging-tools/scripts/macos/. Place
the contents of base-content inside rigs-of-rods/content. These snapshots are
not Git checkouts. For the version-identifying build wrapper use checkouts at
the recorded original revisions, with these identical trees. Initializing a
new Git repository from a snapshot assigns a different revision and does not
recreate original version metadata. Source snapshots can also be configured
with ordinary CMake using BUILDING.md and the options below; compiling source
does not require byte-identical Git metadata.

Seed the original ExternalProject recipe download cache with download-cache/,
preserving its relative layout: copy its contents into the recipe build's
downloads/ directory before running the macos-downloads target. All 16 original
archives were SHA256 checked.
Use dependency-recipe's README and native macOS profile, with its included
patches; do not substitute current tags. Build Release, arm64, using the SDK
and deployment target recorded in the adjacent binary artifact inventory.
Then use scripts/macos/build.sh from rigs-of-rods and
packaging-tools/doc/macOS-packaging.md. Run the committed source tool checkout
with --packaging-source pointing to that same clean recorded tool revision.
The snapshot can be used to inspect/modify the tools, but repackaging requires
a clean committed tool checkout for a new source provenance identity.
The recipe command is cmake -S dependency-recipe -B deps-build -G Ninja with
-DROR_MACOS_GL3=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64,
the selected CMAKE_OSX_SYSROOT and CMAKE_OSX_DEPLOYMENT_TARGET, and an explicit
DEPENDENCIES_OUTPUT_DIR. Build macos-downloads, macos-patches and then the
default target with --parallel 1 (inner dependency builds use up to three jobs).
For RoR preserve ROR_USE_OPENAL, ANGELSCRIPT, CURL, SOCKETW, DISCORD_RPC, CAELUM
and PAGED all ON, ROR_CREATE_CONTENT_FOLDER=ON, CMAKE_FIND_FRAMEWORK=LAST,
ROR_DEPENDENCY_DIR and explicit OPENAL_LIBRARY/OPENAL_INCLUDE_DIR from that
prefix. The archived scripts/macos/build.sh records the remaining profile
configure arguments and required-feature gate. The snapshot's original content
gitlink is present in source-inventory.json; no optional content downloads are
needed. Keep any selected compiler flags, including SDK header workarounds.
The inventory identifies the selected recipe, source, content, options and
patch bytes, including static and header-only components. Pins support source
reproducibility; these tools do not promise byte-identical builds.

Shared libraries can be rebuilt and replaced in Contents/Frameworks. After
replacement, adjust load commands as needed and sign nested code, frameworks,
then the app with codesign --force --sign -, and run
packaging-tools/scripts/macos/verify.py on
a fresh ditto ZIP. Ad hoc signing is not Developer ID signing/notarization.
Notices retain mixed Caelum and zziplib texts; this inventory is not a legal
clearance statement. No application execution is part of the package checks.
''')
    check_links(work)
    output = destination / 'Rigs-of-Rods-source.tar.gz'
    with tarfile.open(output, 'w:gz') as archive:
        archive.add(work, arcname=work.name, recursive=True)
    shutil.rmtree(work)
    return output, inventory
