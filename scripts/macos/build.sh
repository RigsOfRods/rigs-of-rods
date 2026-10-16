#!/bin/bash
# Configure and compile only. This script never runs RoR.
set -euo pipefail
fail() { printf '%s\n' "$*" >&2; exit 1; }
usage='Usage: scripts/macos/build.sh --deps-prefix PATH --build-dir PATH --config Release --jobs N [-- CMake-options...]'
prefix= build= config= jobs=
while (($#)); do
    case "$1" in
        --deps-prefix|--build-dir|--config|--jobs)
            (($# >= 2)) || fail "$usage"
            case "$1" in
                --deps-prefix) prefix=$2 ;;
                --build-dir) build=$2 ;;
                --config) config=$2 ;;
                --jobs) jobs=$2 ;;
            esac
            shift 2 ;;
        --) shift; break ;;
        *) fail "$usage" ;;
    esac
done
[[ -n "$prefix" && -n "$build" && -n "$config" && -n "$jobs" ]] || fail "$usage"
[[ "$config" == Release ]] || fail 'Only Release is supported; use a separately qualified prefix for future configurations.'
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || fail '--jobs must be a positive integer.'
for option in "$@"; do
    [[ "$option" == -D*=* ]] || fail 'After --, pass only -DNAME=value CMake settings.'
done
for tool in cmake ninja python3 git xcrun; do command -v "$tool" >/dev/null || fail "Required tool missing: $tool"; done
source_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)
[[ -d "$prefix" ]] || fail 'Dependency prefix does not exist; build the native ror-dependencies profile first.'
prefix=$(python3 -c 'from pathlib import Path; import sys; print(Path(sys.argv[1]).resolve(strict=True))' "$prefix")
build=$(python3 -c 'from pathlib import Path; import sys; print(Path(sys.argv[1]).resolve())' "$build")
sdk=$(xcrun --sdk "${SDKROOT:-macosx}" --show-sdk-path)
deployment=${MACOSX_DEPLOYMENT_TARGET:-11.0}
helper="$source_root/scripts/macos/profile.py"
python3 "$helper" preflight --prefix "$prefix" --sdk "$sdk" --deployment-target "$deployment"
python3 "$helper" content --source "$source_root"
python3 "$helper" cache --source "$source_root" --build "$build" --prefix "$prefix" --sdk "$sdk" --deployment-target "$deployment"
mkdir -p "$build"
free_kib=$(df -Pk "$build" | awk 'END {print $4}')
((free_kib >= 4194304)) || fail 'Build requires at least 4 GiB free disk space.'
configure=(cmake -S "$source_root" -B "$build" "$@" -G Ninja
    "-DCMAKE_BUILD_TYPE=$config" -DCMAKE_OSX_ARCHITECTURES=arm64
    "-DCMAKE_OSX_SYSROOT=$sdk" "-DCMAKE_OSX_DEPLOYMENT_TARGET=$deployment"
    "-DROR_DEPENDENCY_DIR=$prefix" -DROR_MACOS_GL3_PROFILE=ON
    -DCMAKE_FIND_FRAMEWORK=LAST '-DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew;/usr/local'
    "-DOPENAL_LIBRARY=$prefix/lib/libopenal.dylib" "-DOPENAL_INCLUDE_DIR=$prefix/include/AL"
    -DROR_BUILD_INSTALLER=Off -DROR_CREATE_CONTENT_FOLDER=ON
    -DROR_USE_OPENAL=ON -DROR_USE_ANGELSCRIPT=ON -DROR_USE_CURL=ON
    -DROR_USE_SOCKETW=ON -DROR_USE_DISCORD_RPC=ON -DROR_USE_CAELUM=ON -DROR_USE_PAGED=ON)
printf '%q ' "${configure[@]}"; printf '\n'
"${configure[@]}"
python3 "$helper" finalize --build "$build"
free_kib=$(df -Pk "$build" | awk 'END {print $4}')
((free_kib >= 4194304)) || fail 'Build requires at least 4 GiB free disk space.'
cmake --build "$build" --config "$config" --parallel "$jobs"
