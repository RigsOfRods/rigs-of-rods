# Native macOS arm64 Release build

This opt-in profile configures and compiles RoR using a prebuilt, clean
`ror-dependencies` native-macos-gl3 prefix (dependency version 30). It supports
native Apple Silicon, a single-configuration generator, and Release only. Ninja is the qualified wrapper workflow.
Debug, RelWithDebInfo, MinSizeRel, cross compilation and multi-configuration
generators are rejected until their own dependencies and application are built
and qualified.

The source stack requires the platform/OpenAL foundation (#3469), resource ZIP
fixes (#3468), executable-relative plugin paths (#3470), and effective required
feature gate (#3472). These are separate contributions; this profile does not
assert that they have merged into upstream master. The reviewed dependency
recipe is `d738cd79009362e70b654e14cc7bd1d9f8c311dc`; its source inventory,
patches and build instructions belong to ror-dependencies. RoR does not duplicate
dependency downloads or per-library pins. Revalidate the combined stack after
rebasing or changing the recipe.

Install CMake (3.16 or later), Ninja, Python 3.7 or later, Git and an Apple command line
toolchain. Build the dependency recipe first and retain its installed
`version.cmake`, `share/ror-dependencies/manifest.json` and
`share/ror-dependencies/build-receipt.txt` and successful schema 1
`share/ror-dependencies/build-receipt.json`. The wrapper validates the prefix's
profile, clean recipe identity, architecture, configuration, SDK version and
deployment target, and matches the configured graph/configuration identities
between manifest and completion receipt. The checked baseline uses macOS SDK 27.0 and minimum OS 11.0.
Initialize the exact base content gitlink selected by this source commit:

```sh
git submodule update --init --checkout content
scripts/macos/build.sh --deps-prefix /absolute/prefix-release \
  --build-dir /absolute/ror-build-release --config Release --jobs 3
```

All four options are required. Paths containing spaces are supported. Use a new
build directory for an incompatible cache; the wrapper never cleans or repurposes
one. It checks content HEAD and tracked cleanliness against `git ls-tree HEAD
content` and builds no optional content packs. At least 4 GiB free disk space is
required before configure and compile.

`SDKROOT` selects the SDK through xcrun; `MACOSX_DEPLOYMENT_TARGET` defaults to
11.0. Both must match the dependency receipt. Caller compiler flags are preserved.
Additional CMake settings can follow `--` as `-DNAME=value` arguments, for example
`-- '-DROR_CUSTOM_VERSION=local build'`. The wrapper supplies the profile's
configuration, architecture, feature switches and concrete OpenAL Soft selectors.
Direct CMake configuration may leave feature switches OFF, in which case the
existing required-feature gate fails with an actionable error. Any caller
`ROR_REQUIRED_FEATURES` entries remain required.

The qualification build used Apple clang 21.0.0, CMake 4.4.4, SDK 27.0 and
deployment 11.0. That Command Line Tools installation needed an explicit libc++
header workaround. Supply it through caller flags when needed, retaining your
other flags, for example:

```sh
sdk=$(xcrun --sdk macosx --show-sdk-path)
scripts/macos/build.sh --deps-prefix /absolute/prefix-release \
  --build-dir /absolute/ror-build-release --config Release --jobs 3 \
  -- "-DCMAKE_CXX_FLAGS=-isystem \"$sdk/usr/include/c++/v1\""
```

This is a toolchain qualification, not an unconditional SDK include path in RoR.
A clean build with ordinary toolchain discovery remains necessary before removing
that qualification. Pins alone do not promise byte-identical output; the version
generator's timestamp behavior also applies.

Build outputs are `bin/RoR`, resources/content/languages in `bin`, the effective
`ror-features.cmake` receipt, and `macos/Release/build.json` (schema 1). CMake
generates the exact target path; the serializer JSON-escapes it and all other
values. Both the wrapper and the ordinary CMake ALL build finalize the handoff.
The manifest records the actual prefix receipt/recipe, content gitlink, required
RTShaderLib GLSL source root, effective features, five plugin identities and
their build/bundle cfg entries. OGRE plugin versions come from canonical installed
files; Caelum has linked and plugin roles with one Frameworks destination.
`framework_sources` records the selected Release MyGUI framework's canonical root
and image, and its intended bundle root/image destinations. It retains the exact
`Versions/<version>/<image>` suffix from that selection. The packager must use
this identity to resolve the executable-relative MyGUI load name; the raw build
does not have the eventual Frameworks layout.

`build.json` references `source/version_info/package-version.json` from the
version metadata contribution [#3473](https://github.com/RigsOfRods/rigs-of-rods/pull/3473). That contribution must be integrated and
built before packaging; an absent version file does not imply package readiness.
Selected imported Release paths use CMake’s configuration mapping; all selected
absolute interface paths and target references are checked. The existing MyGUI
Debug/Release expressions and LINK_ONLY target references are supported. Unknown
expressions, bare library names and alternate link mechanisms fail explicitly.
The profile handoff is invalidated on every reconfigure, including profile OFF;
serializer/template/dependency metadata changes trigger reconfiguration.
Local manifest paths are build inputs and must not be redistributed unchanged.
Future packaging must follow linked Mach-O dependencies and render the recorded
bundle-relative plugin cfg. It must also include font notices (#3471), dependency
notices and corresponding sources as applicable.

This slice supports configure/build only; `cmake --install` fails before copying
files. It does not assemble/sign a bundle, install into Applications or user data,
download optional content, or launch RoR. Compile and static artifact checks do
not establish GL3 UI/RTSS rendering, terrain/water/Caelum/Paged material parity,
window lifecycle, Retina/input behavior or runtime capability. The RTShaderLib
resource root is a packaging input; registering it and validating shaders belong
to the GL3 UI contribution.
