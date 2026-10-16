# Native macOS ARM64 Release packages

Build first with the native macOS profile and its reviewed source stack. The
packager consumes `macos/Release/build.json` (schema 1) and generated
`source/version_info/package-version.json` (schema 1). It requires the successful
complete dependency receipts, all seven effective features, five selected
plugins, the declared versioned MyGUI framework, and clean committed RoR,
content and dependency recipe sources matching those inputs. It supports
Release and arm64 only. Python 3.7+ and the existing macOS command-line tools
(`otool`, `lipo`, `install_name_tool`, `codesign`, `ditto`, `sips`, `iconutil`)
are required. Nothing is downloaded by these tools.

```sh
python3 scripts/macos/package.py \
  --build-dir /absolute/ror-build-release \
  --deps-prefix /absolute/prefix-release \
  --output /absolute/new-release-directory --config Release \
  --source-cache /absolute/dependency-build/downloads \
  --recipe-source /absolute/clean-matching-recipe-checkout

python3 scripts/macos/verify.py \
  /absolute/new-release-directory/Rigs-of-Rods-macOS-arm64.zip
```

The output parent must exist and have at least 4 GiB free. Source, prefix and
build directories must be disjoint; output must overlap none of them, the
source cache or the recipe checkout. Existing output is rejected by default.
`--replace` accepts only an output carrying this tool's ownership marker for
that exact canonical path. Staging is private under the selected output parent;
publication renames the verified directory into place. A failed replacement
restores the prior output. Cleanup is restricted to that owned staging tree.
The dependency prefix is read only. The tools neither install an app nor create
user data, and never launch or load the generated Mach-O code.

Output includes `Rigs of Rods.app`, a symlink/mode preserving binary ZIP,
`Rigs-of-Rods-source.tar.gz`, `artifact-inventory.json` and `SHA256SUMS`.
The artifact inventory is sanitized: it records revisions, selected features,
toolchain versions, code destinations, closure, resource hashes/ZIP members,
notice provenance and companion source hash, rather than raw local handoff
paths. The output ownership marker is local bookkeeping and is not in either
release archive. Apple version fields use numeric generated metadata;
the full RoR display version and revision are recorded separately.

The Mach-O closure starts with the executable and exactly the declared plugins
and runtime modules. `otool -l` parsing excludes `LC_ID_DYLIB`, follows transitive
loads, and resolves complete `@rpath` suffixes with inherited search context.
Loader and executable relative loads are supported. The handoff's exact MyGUI
framework identity resolves its future app-relative load before the app exists.
Conflicting resolutions and case/basename/framework collisions fail with source
diagnostics. All non-system images must originate in the selected build/prefix.
Only canonical `/System/Library/` and `/usr/lib/` loads remain external; a dyld
cache image need not exist as a standalone file. Missing weak imports fail,
just like mandatory/reexport/upward imports; there is no optional host fallback.

Frameworks retain their actual `Versions/<version>` image, complete payload and
contained relative links. Four OGRE plugins live in `Contents/PlugIns/OGRE`;
Caelum has a single `Contents/Frameworks` image for both linked and plugin roles.
The plugin config is rendered from the handoff and each entry must resolve to
its recorded destination. Non-system load names are rewritten to loader-relative
bundle paths and existing RPATHs are replaced with contained relative ones.
Every image must be exactly arm64, use the selected SDK, and require no newer
macOS than the selected minimum. Header space for rewritten names must have
been reserved when linking; the tool does not rebuild inputs to work around it.

Runtime payloads physically reside in `Contents/Resources/runtime`, with
contained relative `Contents/MacOS/{resources,content,languages,plugins.cfg}`
symlinks preserving RoR's process-directory lookup. Tiny actual signing fixtures
showed that plain data beneath `Contents/MacOS` is treated as nested code by
codesign and rejected. The resource layout preserves exact build output bytes,
adds only the declared RTShader GLSL source payload, and uses the committed
project icon. It copies selected dependency notices (including static and
header-only components), RoR/content/embedded notices and reviewed font notices.
The three runtime font hashes must match their notice mapping, including when
they are inside resource ZIPs. Resource ZIP bytes and member inventories are
verified again after extraction.

Nested libraries, then frameworks, then the app are explicitly ad hoc signed.
`--deep` is used only for verification, never as the signing algorithm. This is
not Developer ID signing or notarization. The separate verifier preflights ZIP
paths and symlinks before extracting with ditto, then checks all code, closure,
RPATHs, strict signatures, complete payload inventory, resources, notices,
versions and matching source companion from that fresh extraction.

The companion contains exact `git archive` source/content/recipe tree TARs, all
16 SHA256-verified original dependency archives from the selected existing EP
cache, notices, source inventory and rebuild instructions. It validates the
recipe inventory and every patch against the completed prefix, includes the
whole recipe with build scripts and patches, and excludes Git internals and
untracked/ignored fixtures. The cache must be the recipe's bounded `downloads/`
directory so its relative component layout can be restored. No source URL is
treated as a substitute for source bytes. Missing matching inputs fail before
publication. Recreating a Git repository from a tree snapshot gives a new
revision; the wrapper's original version identity requires the recorded original
Git revisions. Pins do not promise byte-identical output. Retaining Caelum's
mixed notices and zziplib's dual-license texts is an engineering inventory, not
a legal-clearance claim.

Focused tests run serial tiny C compiles without executing any result:

```sh
python3 -m unittest discover -s tests/macos -v
```

They exercise the production parser/resolver, transitive and unlinked plugins,
inherited/loader/executable paths, versioned frameworks, collisions, missing
dependencies, synthetic wrong-architecture metadata, unsafe ZIP paths/links,
strict signing, altered code, extracted final ZIP verification and a matching
synthetic source companion. An optional `ROR_PACKAGE_FIXTURE_PARENT` retains
fixtures beneath an existing private directory. This is fixture evidence; a
real release still needs a fresh successful combined build and its own package
and extracted-ZIP verification. No compile/package check establishes shader,
gameplay, graphics, window, Retina or input behavior.
