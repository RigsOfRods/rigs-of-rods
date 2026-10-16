# Native macOS arm64 Release build

Use a native Apple Silicon host, CMake 3.16 or later, Ninja, Git and an Apple
command line toolchain. The opt-in `ROR_MACOS_GL3_PROFILE` supports a
single-configuration Release build with the `ror-dependencies`
`native-macos-gl3` Release prefix (dependency version 30). Use the same SDK and
minimum macOS version as that prefix. Debug, other architectures and
multi-configuration generators need separate qualification.

The qualified prefix was built at recipe revision
`d738cd79009362e70b654e14cc7bd1d9f8c311dc`, with Apple clang 21.0.0,
CMake 4.4.4, SDK 27.0 and minimum macOS 11.0. Later recipe documentation or
metadata changes do not make it a rebuilt prefix. Dependency pins, patches,
licenses and build instructions remain in `ror-dependencies`. Keep the complete
versioned `MyGUIEngine.framework`, including its `Versions` directory and
symlinks: its install name expects the eventual app's Frameworks layout.

This public profile branch is stacked on the proposed platform/OpenAL (#3469),
resource ZIP (#3468), executable-relative plugin/window scale (#3470), and
required-feature (#3472) commits. They are not asserted to have merged upstream.
#3472 was closed unmerged; its implementation, tests and receipt are removed by
an additive simplification commit, preserving the existing public history. The
native profile requires no package-version JSON or Python packaging handoff. Revalidate
the combined source stack after changing its foundations or dependency recipe.

Initialize the content revision selected by this checkout, then configure into a
**new build directory**. When changing dependencies or feature selection, remove
that build directory's `CMakeCache.txt` before configuring again; using a new
build directory also avoids stale generated files. Do not reuse a build directory
from the previous JSON handoff workflow. Review the discovery and feature summary
at the end of configuration.

```sh
git submodule update --init --checkout content
sdk=$(xcrun --sdk macosx --show-sdk-path)
prefix=/absolute/prefix-release
cmake -S . -B /absolute/ror-build-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_SYSROOT="$sdk" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
  -DROR_MACOS_GL3_PROFILE=ON -DROR_DEPENDENCY_DIR="$prefix" \
  -DROR_CREATE_CONTENT_FOLDER=ON -DROR_BUILD_INSTALLER=Off \
  -DROR_USE_OPENAL=ON -DROR_USE_ANGELSCRIPT=ON -DROR_USE_CURL=ON \
  -DROR_USE_SOCKETW=ON -DROR_USE_DISCORD_RPC=ON \
  -DROR_USE_CAELUM=ON -DROR_USE_PAGED=ON
cmake --build /absolute/ror-build-release --parallel 3
```

The seven `ON` options are explicit inputs. A short profile check rejects any
that dependency discovery effectively disables, including a missing optional
package. Ordinary package discovery and target linking provide dependency
errors. The developer `plugins.cfg` selects installed GL3Plus, STBI, ParticleFX,
OctreeSceneManager and Caelum. The profile selects OpenAL Soft from the prefix,
sets its `CMAKE_PREFIX_PATH`, disables package registries and excludes the usual
Homebrew `/opt/homebrew` and `/usr/local` prefixes. Use a clean CMake environment;
these settings are not a recursive audit of every imported dependency path.

The optional wrapper supplies the same options, verifies the content gitlink and
tracked cleanliness, and refuses an existing CMake cache:

```sh
scripts/macos/build.sh --deps-prefix /absolute/prefix-release \
  --build-dir /absolute/ror-build-release --config Release --jobs 3
```

Paths containing spaces are supported. `SDKROOT` selects the SDK via xcrun;
`MACOSX_DEPLOYMENT_TARGET` defaults to 11.0. Additional `-DNAME=value` settings
can follow `--`. Caller compiler flags are preserved. The qualified Command Line
Tools installation needed libc++ headers explicitly; where necessary append
`-- "-DCMAKE_CXX_FLAGS=-isystem \"$sdk/usr/include/c++/v1\""`, retaining any
other flags. This workaround needs toolchain revalidation before being removed.
The wrapper checks for at least 4 GiB free space before configure and compile.

Build outputs are `bin/RoR` and its resources/content/languages. The raw executable
is not an installed app: MyGUI's app-relative framework install name needs the
bundle layout supplied by the separate CMake/CPack contribution. This build
profile supplies no bundle portability or signing guarantee. Its inherited
non-Apple install rules alone do not produce a usable native app. Compile checks
do not establish GL3 UI/RTSS/material/terrain/water/Caelum/Paged rendering,
window lifecycle, Retina/input behavior or runtime capability. The wrapper
configures and compiles only; it never launches RoR.
