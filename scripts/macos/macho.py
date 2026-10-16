"""Bounded Mach-O inspection and dyld dependency resolution; never executes code."""
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
import re
import subprocess


def run(*args):
    return subprocess.check_output([str(a) for a in args], text=True, stderr=subprocess.STDOUT).strip()


def within(path, root):
    return path == root or root in path.parents


def relative(value):
    path = PurePosixPath(value)
    if not value or not path.parts or path.as_posix() != value or path.is_absolute() or any(p in ('..', '') for p in path.parts) or '\\' in value or any(c in value for c in '\r\n\0'):
        raise ValueError('unsafe relative path: ' + repr(value))
    return path


def system(name):
    # Do not require a filesystem entry: modern macOS uses the dyld shared cache.
    return name.startswith(('/System/Library/', '/usr/lib/')) and '..' not in PurePosixPath(name).parts and PurePosixPath(name).as_posix() == name


@dataclass(frozen=True)
class Image:
    loads: tuple
    rpaths: tuple
    identity: str = ''
    minimum: str = ''
    sdk: str = ''


def parse_load_commands(text):
    loads, rpaths, identity, minimum, sdk = [], [], '', '', ''
    for block in re.split(r'\nLoad command \d+\n', '\n' + text)[1:]:
        command = re.search(r'^\s*cmd (\S+)', block, re.M)
        if not command:
            continue
        command = command[1]
        if command in ('LC_LOAD_DYLIB', 'LC_LOAD_WEAK_DYLIB', 'LC_REEXPORT_DYLIB', 'LC_LOAD_UPWARD_DYLIB', 'LC_ID_DYLIB'):
            match = re.search(r'^\s*name (.*?) \(offset \d+\)', block, re.M)
            if not match:
                raise ValueError('unparseable ' + command)
            if command == 'LC_ID_DYLIB':
                identity = match[1]
            else:
                loads.append((command, match[1]))
        elif command == 'LC_RPATH':
            match = re.search(r'^\s*path (.*?) \(offset \d+\)', block, re.M)
            if not match:
                raise ValueError('unparseable LC_RPATH')
            rpaths.append(match[1])
        elif command in ('LC_BUILD_VERSION', 'LC_VERSION_MIN_MACOSX'):
            if command == 'LC_BUILD_VERSION' and not re.search(r'^\s*platform (?:1|macos)\s*$', block, re.M):
                raise ValueError('image is not built for macOS')
            match = re.search(r'^\s*(?:minos|version) (\S+)', block, re.M)
            sdk_match = re.search(r'^\s*sdk (\S+)', block, re.M)
            minimum = match[1] if match else ''
            sdk = sdk_match[1] if sdk_match else ''
    return Image(tuple(loads), tuple(rpaths), identity, minimum, sdk)


def version(value):
    if not re.fullmatch(r'\d+(?:\.\d+){0,2}', value):
        raise ValueError('invalid numeric version: ' + repr(value))
    parts = tuple(map(int, value.split('.')))
    return parts + (0,) * (3 - len(parts))


def inspect(path, deployment=None, sdk=None):
    if run('lipo', '-archs', path) != 'arm64':
        raise ValueError('image must contain exactly arm64: ' + str(path))
    image = parse_load_commands(run('otool', '-l', path))
    if not image.minimum or not image.sdk:
        raise ValueError('missing macOS minimum/SDK load command: ' + str(path))
    if deployment and version(image.minimum) > version(deployment):
        raise ValueError('image requires newer macOS than selected deployment: ' + str(path))
    if sdk and version(image.sdk) != version(sdk):
        raise ValueError('image SDK differs from selected SDK: ' + str(path))
    return image


def is_macho(path):
    if path.is_symlink() or not path.is_file():
        return False
    with path.open('rb') as stream:
        return stream.read(4) in (b'\xcf\xfa\xed\xfe', b'\xce\xfa\xed\xfe', b'\xfe\xed\xfa\xcf', b'\xca\xfe\xba\xbe', b'\xbe\xba\xfe\xca', b'\xca\xfe\xba\xbf')


def expand(name, loader, executable):
    for token, base in (('@loader_path', loader.parent), ('@executable_path', executable.parent)):
        if name == token or name.startswith(token + '/'):
            return (base / name[len(token):].lstrip('/')).resolve()
    if name.startswith('/'):
        return Path(name).resolve()
    raise ValueError('unsupported relative dyld path: ' + name)


def search_paths(image, loader, executable, inherited):
    paths = []
    for name in image.rpaths:
        if name.startswith('@rpath/'):
            paths.extend((p / name[len('@rpath/'):]).resolve() for p in inherited)
        else:
            paths.append(expand(name, loader, executable))
    return tuple(dict.fromkeys([*paths, *inherited]))


def resolve(name, loader, executable, paths, aliases=None):
    if system(name):
        return None
    if name.startswith('@rpath/'):
        suffix = name[len('@rpath/'):]
        relative(suffix)
        candidates = [(p / suffix).resolve() for p in paths if (p / suffix).is_file()]
    else:
        candidate = expand(name, loader, executable)
        candidates = [candidate] if candidate.is_file() else []
    candidates = list(dict.fromkeys(candidates))
    alias = (aliases or {}).get(name)
    if alias is not None:
        if candidates and candidates != [alias]:
            raise ValueError(f'declared framework source conflicts with resolved load: {loader}: {name}')
        candidates = [alias]
    if len(candidates) > 1:
        raise ValueError(f'conflicting resolutions for {loader}: {name}: {candidates}')
    if not candidates:
        # Missing weak imports are deliberately fatal too. No platform-dependent omission.
        raise ValueError(f'missing dependency of {loader}: {name}; rpaths={paths}')
    return candidates[0]


def closure(executable, seeds, roots, reader=inspect, aliases=None):
    executable = executable.resolve(strict=True)
    roots = tuple(Path(p).resolve(strict=True) for p in roots)
    images, edges, seen = {}, {}, set()
    initial = search_paths(reader(executable), executable, executable, ())
    pending = [(p.resolve(strict=True), initial) for p in [executable, *seeds]]
    while pending:
        path, inherited = pending.pop(0)
        if not any(within(path, root) for root in roots):
            raise ValueError('non-system dependency escapes approved roots: ' + str(path))
        image = images.setdefault(path, reader(path))
        paths = search_paths(image, path, executable, inherited)
        key = (path, paths)
        if key in seen:
            continue
        seen.add(key)
        for command, name in image.loads:
            target = resolve(name, path, executable, paths, aliases)
            edge = (path, name)
            if edge in edges and edges[edge] != target:
                raise ValueError(f'dependency resolves differently in inherited contexts: {path}: {name}')
            edges[edge] = target
            if target is not None:
                pending.append((target, paths))
    return images, edges


def framework(path):
    for parent in path.parents:
        if parent.suffix == '.framework':
            return parent
    return None


def destinations(images, executable, selected):
    result, used, frameworks = {}, {}, {}
    for source in images:
        root = framework(source)
        if source == executable:
            target = 'Contents/MacOS/' + source.name
        elif source in selected:
            target = selected[source]
        elif root:
            prior = frameworks.setdefault(root.name.casefold(), root)
            if prior != root:
                raise ValueError(f'framework collision: {prior} and {root}')
            target = str(PurePosixPath('Contents/Frameworks') / root.name / source.relative_to(root).as_posix())
        else:
            target = 'Contents/Frameworks/' + source.name
        relative(target)
        prior = used.setdefault(target.casefold(), source)
        if prior != source:
            raise ValueError(f'bundle destination collision {target}: {prior} and {source}')
        result[source] = target
    return result


def check_links(root):
    root = root.resolve(strict=True)
    names = {}
    for path in root.rglob('*'):
        name = path.relative_to(root).as_posix()
        prior = names.setdefault(name.casefold(), name)
        if prior != name:
            raise ValueError(f'case-insensitive payload collision: {prior} and {name}')
        if path.is_symlink():
            import os
            target = os.readlink(path)
            if Path(target).is_absolute() or not within(path.resolve(strict=True), root):
                raise ValueError('escaping or absolute payload symlink: ' + str(path))
        elif not path.is_file() and not path.is_dir():
            raise ValueError('unsupported payload file type: ' + str(path))
