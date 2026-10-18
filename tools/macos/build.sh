#!/bin/bash
# Build the Apple Silicon macOS port from scratch (reproducible).
#
# Steps:
#   1. Ensure cmake/ninja/conan are present.
#   2. Build AngelScript from a pinned upstream commit into .deps/.
#   3. Seed the local Conan cache with the patched dependency recipes
#      (tools/macos/conan-recipes/*) so Conan applies the exact Mac fixes
#      it applied to the dependencies that ship in this bundle.
#   4. Configure + build RoR with the exact release flags used by the port.
#   5. Bundle into an .app (reuse tools/macos/bundle.sh).
#
# Run from anywhere:
#   bash tools/macos/build.sh [build-dir]
#
# Default build dir: ./build-local

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${1:-$REPO_ROOT/build-local}"

# Exact AngelScript commit used to build the shipped binaries.
SCRIPT_URL="https://github.com/AnotherFoxGuy/angelscript.git"
SCRIPT_REF="762ab03"        # "Update to 2.38.0"
SCRIPT_DIR="$REPO_ROOT/.deps/angelscript"
SCRIPT_INSTALL="$REPO_ROOT/.deps/angelscript-install"

# Conan recipes that need our macOS patches applied.  "ref | recipe-dir"
# (pipe-delimited plain array: bash 3.2 on macOS has no associative arrays)
CONAN_RECIPES=(
    "zlib/1.3|$REPO_ROOT/tools/macos/conan-recipes/zlib/1.3"
    "libpng/1.6.40|$REPO_ROOT/tools/macos/conan-recipes/libpng/1.6.40"
    "freeimage/3.18.0@anotherfoxguy/stable|$REPO_ROOT/tools/macos/conan-recipes/freeimage/3.18.0"
    "ogre3d/1.11.6.1@anotherfoxguy/stable|$REPO_ROOT/tools/macos/conan-recipes/ogre3d/1.11.6.1"
)

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "error: this port only builds on macOS." >&2
    exit 1
fi

if [[ "$(uname -m)" != "arm64" ]]; then
    echo "error: this port only builds on Apple Silicon (arm64)." >&2
    exit 1
fi

CD="$(command -v cmake || true)"
NJ="$(command -v ninja || true)"
CN="$(command -v conan || true)"

if [[ -z "$CD" ]]; then
    echo "error: cmake not found, brew install cmake" >&2
    exit 1
fi
if [[ -z "$NJ" ]]; then
    echo "error: ninja not found, brew install ninja" >&2
    exit 1
fi
if [[ -z "$CN" ]]; then
    echo "error: conan not found, please install conan 2.x (pip install conan)" >&2
    exit 1
fi

# AngelScript still declares CMake 3.5 as its minimum; satisfy CMake 4+.
export CMAKE_POLICY_VERSION_MINIMUM=3.5

# Ensure the Conan profile carries the exact conf this port was built with
# (SDK path, legacy libc++ compatibility flags, Ogre CMake options).
# The block is rewritten every run so a stale or divergent profile can
# never change the produced app.
CONAN_HOME_DIR="$(${CONAN:-conan} config home)"
DEFAULT_PROFILE="$CONAN_HOME_DIR/profiles/default"
if [[ ! -f "$DEFAULT_PROFILE" ]]; then
    ${CONAN:-conan} profile detect --force
fi
SDK_PATH="$(xcrun --sdk macosx --show-sdk-path)"
PROFILE_TMP="$DEFAULT_PROFILE.ror-tmp"
grep -v -E '^(tools\.apple:sdk_path|tools\.build:cxxflags|tools\.cmake\.cmaketoolchain:extra_variables|tools\.cmake:configure_args)=|^\[conf\]$' \
    "$DEFAULT_PROFILE" > "$PROFILE_TMP"
# Legacy OIS/OGRE/PagedGeometry use C++17-removed libc++ APIs:
# unary_function/binary_function, binders and auto_ptr. Restore them
# for both C++ and Objective-C++ dependency builds.
# OGRE 1.11's framework path emits Xcode-only $(...) placeholders
# which modern Ninja rejects; Rigs of Rods packages dylibs instead.
# Pass the policy and options on CMake's command line as cache values:
# its legacy cmake_dependent_option() otherwise clears normal vars.
# The app loads RenderSystem_GL (see deployment/plugins.cfg), so the
# legacy GL renderer is ON and GL3+ stays OFF — matching the golden app.
printf '\n[conf]\ntools.apple:sdk_path=%s\ntools.build:cxxflags=["-D_LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION -D_LIBCPP_ENABLE_CXX17_REMOVED_BINDERS -D_LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR"]\ntools.cmake.cmaketoolchain:extra_variables={"CMAKE_OBJCXX_FLAGS_INIT":"-D_LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION -D_LIBCPP_ENABLE_CXX17_REMOVED_BINDERS -D_LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR"}\ntools.cmake:configure_args=["-DCMAKE_POLICY_DEFAULT_CMP0077=NEW","-DOGRE_BUILD_LIBS_AS_FRAMEWORKS=OFF","-DOGRE_BUILD_PLUGIN_CG=OFF","-DOGRE_BUILD_RENDERSYSTEM_GL=ON","-DOGRE_BUILD_RENDERSYSTEM_GL3PLUS=OFF"]\n' \
    "$SDK_PATH" >> "$PROFILE_TMP"
mv "$PROFILE_TMP" "$DEFAULT_PROFILE"
# OIS and friends only exist on the legacy Rigs-of-Rods dependency remote.
${CONAN:-conan} remote add rigs-of-rods-deps \
    "https://nexus.anotherfoxguy.com/repository/rigs-of-rods/" \
    --force

mkdir -p "$(dirname "$SCRIPT_DIR")"
if [[ ! -d "$SCRIPT_DIR/.git" ]]; then
    git clone "$SCRIPT_URL" "$SCRIPT_DIR"
fi
if [[ "$(git -C "$SCRIPT_DIR" rev-parse HEAD)" != "$SCRIPT_REF" ]]; then
    # A shallow clone cannot resolve old SHAs until unshallowed; plain
    # fetch on a full clone is also fine.
    git -C "$SCRIPT_DIR" fetch --unshallow origin 'refs/heads/*:refs/remotes/origin/*' 2>/dev/null \
        || git -C "$SCRIPT_DIR" fetch origin
    git -C "$SCRIPT_DIR" checkout --force "$SCRIPT_REF"
fi
# Always reconfigure AngelScript from scratch: a stale CMakeCache would
# silently keep flags from a different deployment target or SDK.
rm -rf "$SCRIPT_DIR/build"
cmake -S "$SCRIPT_DIR" -B "$SCRIPT_DIR/build" \
    -G Ninja \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
    -DCMAKE_CXX_STANDARD=11 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$SCRIPT_INSTALL"
cmake --build "$SCRIPT_DIR/build" --parallel "$(sysctl -n hw.ncpu)"
cmake --install "$SCRIPT_DIR/build"

# Seed the local Conan cache with the patched dependency recipes.
# Ref format: "name/version" or "name/version@user/channel".  The recipes
# only declare `name`, so export must be given the version explicitly.
for entry in "${CONAN_RECIPES[@]}"; do
    ref="${entry%%|*}"
    dir="${entry#*|}"
    name="${ref%%/*}"
    ver="${ref#*/}"
    ver="${ver%%@*}"
    extra="${ref#*@}"
    args=(export "$dir" --name "$name" --version "$ver")
    if [[ "$extra" != "$ref" ]]; then
        args+=(--user "${extra%%/*}" --channel "${extra#*/}")
    fi
    echo "exporting conan recipe: $ref  (from $dir)"
    ${CONAN:-conan} "${args[@]}"
done

# Configure and build RoR with the exact release flags used by the port.
cmake -S "$REPO_ROOT" -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="$REPO_ROOT/cmake/conan_provider.cmake" \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$SCRIPT_INSTALL" \
    -DROR_USE_CAELUM=OFF
cmake --build "$BUILD_DIR" --parallel "$(sysctl -n hw.ncpu)"

# Bundle into .app (reuses the existing packaging script).
"$REPO_ROOT/tools/macos/bundle.sh" "$BUILD_DIR"

echo
echo "Build complete: $BUILD_DIR/RigsOfRods.app"
