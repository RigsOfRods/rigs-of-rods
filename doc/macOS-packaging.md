# Native macOS packages with CPack

The macOS install rules create `RoR.app`; the project's existing CPack ZIP
generator archives that installed bundle. The initial supported configuration is
a single-configuration Release build for arm64 using the native GL3 dependency
profile. Use a clean build directory when changing dependencies or features.

This integration needs the macOS platform and executable-relative plugin-path
changes, the resource archive fixes, and the font notices. Initialize the content
submodule. The selected dependency installation must provide the four GL3/STBI/
ParticleFX/Octree plugins, the complete versioned MyGUIEngine framework, and
`share/ror-dependencies/licenses`. Enable the game's optional features as required
by your build; packaging does not disable rendering effects or change that choice.

After configuring and building the game and resource targets normally:

```sh
cmake --build "$build" --parallel 3
cmake --install "$build" --prefix "$stage"
cpack --config "$build/CPackConfig.cmake" -B "$packages"
```

Use a new disposable staging directory. CPack also installs into its own staging
directory. Both paths copy dependencies with CMake's `BundleUtilities`, repair
their install names, then ad hoc sign the installed copies. Neither operation
modifies the dependency prefix or build executable. Ad hoc signing is suitable
for local qualification; it is not Developer ID signing or notarization.

Resources, content, languages and the installed plugin configuration live in
`Contents/Resources/runtime`. Four relative links from `Contents/MacOS` preserve
the game's executable-relative resource paths. Ogre plugins live in
`Contents/PlugIns/OGRE`; Caelum has a single copy in `Contents/Frameworks`.
MyGUI's complete framework is installed before dependency fixup because its
original install name already refers to its future location in the app.

The bundle version uses the existing project version (`2026.01` becomes the
numeric `2026.1.0`). The executable continues to use `GenerateVersionInfo.cmake`;
development builds may display a Git version. Select the existing release version
settings when producing a release. There is no separate package version manifest.

`tools/macos/ror.icns` is derived from the committed `resources/icons/ror.png`
using macOS `sips` at the standard iconset sizes and `iconutil -c icns`. It retains
the source asset's project licensing; packaging requires no image generator.

## Distribution inputs

The app includes project, base-content, embedded third-party, font and dependency
notices. These notices do not replace corresponding source. Before distributing
a binary, prepare dedicated clean checkouts at the exact game/content and
dependency-recipe revisions used, plus a bounded directory of the original
dependency source archives used by that recipe. Include any local patches and
the build instructions/options needed to reproduce the binaries. Keep the
original archive names and the recipe's existing source pins/hash checks.

CPack can archive these ordinary source roots. Set its standard source options
when configuring the clean game checkout, in addition to the normal build flags:

```sh
cmake -S "$game" -B "$source_build" \
  -DCPACK_SOURCE_GENERATOR=TGZ \
  -DCPACK_SOURCE_PACKAGE_FILE_NAME=rigs-of-rods-corresponding-source \
  '-DCPACK_SOURCE_IGNORE_FILES=/[.]git(/|$)' \
  -DCPACK_SOURCE_INSTALLED_DIRECTORIES="$game;rigs-of-rods;$recipe;ror-dependencies;$archives;dependency-archives"
cpack --config "$source_build/CPackSourceConfig.cmake" -B "$packages"
```

Keep build outputs outside those input roots and initialize the content submodule
inside the clean game tree. The ignore expression excludes both `.git` directories
and regular `.git` files used by worktrees/submodules, whose contents can expose
paths outside the exported tree. CPack snapshots files; it does not verify clean Git
identity, retrieve missing dependency sources, or establish source completeness.
Record the exact revisions, options, toolchain and archive inputs in human-readable
rebuild instructions next to the artifacts. A game-only CPack source archive is
insufficient for a binary containing separately built dependencies. Do not publish
a binary until its corresponding source inputs have been assembled and reviewed.

The exported source has no Git metadata. Rebuild the game with the ordinary CMake
configure command in [macOS-build.md](macOS-build.md), setting its source to the
extracted `rigs-of-rods` directory and using a new build directory and rebuilt
dependency prefix. Its extracted `content` directory is already populated. Do
not use `scripts/macos/build.sh` for this export: that checkout wrapper requires
Git's `HEAD:content` and submodule checks. Extract outside any parent Git repository
so the version generator cannot detect an unrelated revision. Supply the recorded
toolchain, deployment target, feature settings and caller compiler flags, including
any CXX/OBJCXX SDK libc++ include flags and `ROR_RTSHADER_GLSL_DIR`, as recorded in
the accompanying source archive's rebuild notes.

The dependency recipe used for the qualified prefix records `git rev-parse HEAD`
when publishing its build receipt. Before rebuilding an exported recipe, initialize
and commit that extracted tree as a new local repository, using your own configured
Git author identity:

```sh
git -C "$recipe" init
git -C "$recipe" add .
git -C "$recipe" commit -m "Import exported dependency sources for local rebuild"
```

Keep dependency build outputs outside the recipe tree. Follow the accompanying
rebuild notes to preseed the new build's `downloads` directory from the exported
original archives, preserving their component directories and filenames. The new
receipt must identify this new local commit; do not substitute the original recipe
revision.
Record the original source revisions separately in the accompanying rebuild notes.
The game's existing version generator falls back to `dev-without-git` for a
source export; version/build timestamps and the new dependency receipt identity
can differ from the original build. These instructions do not promise a bit-for-bit
identical rebuild or preserve the original development Git display version.

## Qualification

Extract the ZIP into a fresh path containing spaces. Confirm the framework's
`Versions/Current`, binary and Resources links, the four resource links, all
resource archives/base content/languages/font/RTShader payloads, and the notices.
Check the executable and every embedded Mach-O with `file`/`otool -L` for the
expected architecture and bundled or system dependencies. BundleUtilities also
searches standard locations, so independently check for escaped Homebrew,
`/usr/local`, build or dependency-prefix paths. Run:

```sh
codesign --verify --deep --strict --verbose=2 "$extracted/RoR.app"
```

Static install/package checks do not establish renderer, input, gameplay,
performance, deployment-target compatibility or notarization behavior. Runtime
qualification is a separate step.
