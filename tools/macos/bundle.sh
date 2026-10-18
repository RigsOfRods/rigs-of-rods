#!/bin/bash
# tools/macos/bundle.sh — Rigs of Rods macOS App Bundle Assembler
# Usage: bundle.sh [build_dir]
#
# Assembles a relocatable, ad-hoc code-signed .app bundle with all
# transitive dylib dependencies resolved by dylibbundler.
# Designed for both CI runners (GitHub Actions macos-14) and local dev use.
set -euo pipefail

# ──────────────────────────────────────────────────────────────────
# Path resolution
# ──────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${1:-$REPO_ROOT/build}"

APP_NAME="RigsOfRods"
APP_DIR="$BUILD_DIR/$APP_NAME.app"
EXECUTABLE="RoR"

echo "=============================================="
echo " Rigs of Rods — macOS App Bundle Builder"
echo "=============================================="
echo "Repository : $REPO_ROOT"
echo "Build dir  : $BUILD_DIR"
echo "Output     : $APP_DIR"
echo ""

# ──────────────────────────────────────────────────────────────────
# 1. Validate prerequisites
# ──────────────────────────────────────────────────────────────────
if [[ ! -f "$BUILD_DIR/bin/$EXECUTABLE" ]]; then
    echo "FATAL: Main executable not found at $BUILD_DIR/bin/$EXECUTABLE"
    exit 1
fi

if ! command -v dylibbundler &>/dev/null; then
    echo "FATAL: dylibbundler is not installed."
    echo "       Install with: brew install dylibbundler"
    exit 1
fi

# ──────────────────────────────────────────────────────────────────
# 2. Create bundle skeleton
# ──────────────────────────────────────────────────────────────────
rm -rf "$APP_DIR"
mkdir -p "$APP_DIR/Contents/MacOS"
mkdir -p "$APP_DIR/Contents/Resources"
mkdir -p "$APP_DIR/Contents/Frameworks"
mkdir -p "$APP_DIR/Contents/PlugIns"

# PkgInfo stamp (standard for macOS application bundles)
printf 'APPL????' > "$APP_DIR/Contents/PkgInfo"

echo "[1/7] Bundle skeleton created"

# ──────────────────────────────────────────────────────────────────
# 3. Install executable and Info.plist
# ──────────────────────────────────────────────────────────────────
cp "$BUILD_DIR/bin/$EXECUTABLE" "$APP_DIR/Contents/MacOS/"
chmod 755 "$APP_DIR/Contents/MacOS/$EXECUTABLE"

if [[ -f "$REPO_ROOT/deployment/Info.plist.in" ]]; then
    # Extract version from the top-level CMakeLists.txt project() call
    PROJECT_VERSION="$(sed -n 's/.*project(.*VERSION[[:space:]]*\([0-9][0-9.]*\).*/\1/p' \
        "$REPO_ROOT/CMakeLists.txt" 2>/dev/null | head -1)"
    PROJECT_VERSION="${PROJECT_VERSION:-0.0.0}"
    sed "s/\${PROJECT_VERSION}/$PROJECT_VERSION/g" \
        "$REPO_ROOT/deployment/Info.plist.in" > "$APP_DIR/Contents/Info.plist"
    echo "  Info.plist installed (version: $PROJECT_VERSION)"
    if [[ -f "$REPO_ROOT/deployment/RoR.icns" ]]; then
        cp "$REPO_ROOT/deployment/RoR.icns" "$APP_DIR/Contents/Resources/RoR.icns"
        echo "  Application icon installed"
    else
        echo "  WARNING: deployment/RoR.icns is missing"
    fi
elif [[ -f "$BUILD_DIR/Info.plist" ]]; then
    cp "$BUILD_DIR/Info.plist" "$APP_DIR/Contents/"
    echo "  Info.plist copied from build dir"
else
    echo "  WARNING: No Info.plist template found"
fi

echo "[2/7] Executable and Info.plist installed"

# ──────────────────────────────────────────────────────────────────
# 4. Copy plugins.cfg and game data
# ──────────────────────────────────────────────────────────────────
if [[ -f "$REPO_ROOT/deployment/plugins.cfg" ]]; then
    cp "$REPO_ROOT/deployment/plugins.cfg" "$APP_DIR/Contents/Resources/"
    echo "  plugins.cfg installed from deployment/"
elif [[ -f "$BUILD_DIR/bin/plugins.cfg" ]]; then
    cp "$BUILD_DIR/bin/plugins.cfg" "$APP_DIR/Contents/Resources/"
    echo "  plugins.cfg installed from build bin/"
fi

for data_dir in resources content languages; do
    if [[ -d "$BUILD_DIR/bin/$data_dir" ]]; then
        cp -R "$BUILD_DIR/bin/$data_dir" "$APP_DIR/Contents/Resources/"
        echo "  Copied: $data_dir/"
    else
        echo "  Skipped: $data_dir/ (not found in build output)"
    fi
done

# RoR's resource groups resolve archives and managed materials from
# sys_resources_dir (Contents/Resources) directly, while the content manager
# also expects the same archives under Contents/Resources/resources. Keep both
# layouts but hard-link the duplicate files so the bundle doesn't double in
# size. If hard links aren't available, fall back to a normal copy.
if [[ -d "$APP_DIR/Contents/Resources/resources" ]]; then
    while IFS= read -r -d '' item; do
        base="$(basename "$item")"
        if [[ ! -e "$APP_DIR/Contents/Resources/$base" ]]; then
            cp -Rl "$item" "$APP_DIR/Contents/Resources/" 2>/dev/null || \
                cp -R "$item" "$APP_DIR/Contents/Resources/"
        fi
    done < <(find "$APP_DIR/Contents/Resources/resources" -mindepth 1 -maxdepth 1 -print0)
    echo "  Flattened resource archives into Contents/Resources/"
fi

echo "[3/7] Game data installed"

# ──────────────────────────────────────────────────────────────────
# 5. Discover and copy OGRE plugins (bounded search)
# ──────────────────────────────────────────────────────────────────
PLUGIN_COUNT=0
while IFS= read -r -d '' plugin; do
    plugin_name="$(basename "$plugin")"
    case "$plugin_name" in
        RenderSystem_*|Plugin_*|Codec_*|Caelum*|libOgre*)
            cp "$plugin" "$APP_DIR/Contents/PlugIns/"
            echo "  Plugin: $plugin_name"
            PLUGIN_COUNT=$((PLUGIN_COUNT + 1))
            ;;
    esac
done < <(find "$BUILD_DIR/bin" -maxdepth 2 -name '*.dylib' -type f -print0 2>/dev/null)

# OGRE's plugins.cfg names plugins without their SONAME version, while Conan
# stages versioned files such as RenderSystem_GL3Plus.1.11.6.dylib. Create the
# unversioned loader names expected by OGRE (and keep versioned files for linking).
while IFS= read -r -d '' plugin; do
    plugin_file="$(basename "$plugin")"
    plugin_stem="${plugin_file%.dylib}"
    plugin_alias_stem="$plugin_stem"
    while [[ "$plugin_alias_stem" =~ ^(.+)\.[0-9]+$ ]]; do
        plugin_alias_stem="${BASH_REMATCH[1]}"
    done
    if [[ "$plugin_alias_stem" != "$plugin_stem" ]]; then
        plugin_alias="$APP_DIR/Contents/PlugIns/${plugin_alias_stem}.dylib"
        ln -sfn "$plugin_file" "$plugin_alias"
    fi
done < <(find "$APP_DIR/Contents/PlugIns" -maxdepth 1 -name '*.dylib' -type f -print0 2>/dev/null)

echo "[4/7] $PLUGIN_COUNT OGRE plugin(s) staged into PlugIns/"

# ──────────────────────────────────────────────────────────────────
# 6. dylibbundler — resolve and relink transitive dylib dependencies
#
#    -od  : overwrite destination directory if it exists
#    -b   : bundle dependencies into the destination
#    -of  : overwrite files at destination
#    -i   : ignore/exclude paths from relinking (Apple system libraries)
# ──────────────────────────────────────────────────────────────────
# MyGUI is built as a framework by its Conan recipe. The executable already
# references it via @executable_path/../Frameworks, but it is not staged in
# bin/ by the upstream build. Copy any framework dependency from the build
# tree or Conan cache before dylibbundler validates the executable.
while IFS= read -r dep; do
    case "$dep" in
        @executable_path/../Frameworks/*)
            rel="${dep#@executable_path/../Frameworks/}"
            [[ "$rel" == *.framework/* ]] || continue
            fw_name="${rel%%.framework/*}.framework"
            if [[ -n "$fw_name" && ! -d "$APP_DIR/Contents/Frameworks/$fw_name" ]]; then
                fw_src="$(find "$BUILD_DIR" "${CONAN_HOME:-$HOME/.conan2}/p" \
                    -maxdepth 8 -type d -name "$fw_name" -print -quit 2>/dev/null)"
                if [[ -n "$fw_src" ]]; then
                    cp -R "$fw_src" "$APP_DIR/Contents/Frameworks/"
                    echo "  Framework: $fw_name"
                else
                    echo "  WARNING: framework not found: $fw_name"
                fi
            fi
            ;;
    esac
done < <(otool -L "$APP_DIR/Contents/MacOS/$EXECUTABLE" | awk 'NR > 1 { print $1 }')

DYLIBBUNDLER_ARGS=(
    -od -b -of
    -s "$BUILD_DIR/bin"
    -x "$APP_DIR/Contents/MacOS/$EXECUTABLE"
    -d "$APP_DIR/Contents/Frameworks"
    -p "@executable_path/../Frameworks/"
    -i /Library/Frameworks
    -i /System/Library
    -i /usr/lib
)

# Add each staged plugin as an additional executable to resolve
while IFS= read -r -d '' p; do
    DYLIBBUNDLER_ARGS+=(-x "$p")
done < <(find "$APP_DIR/Contents/PlugIns" -maxdepth 1 -name '*.dylib' -type f -print0 2>/dev/null)

echo "  Running dylibbundler with ${#DYLIBBUNDLER_ARGS[@]} flags..."
dylibbundler "${DYLIBBUNDLER_ARGS[@]}" || {
    echo "FATAL: dylibbundler failed — inspect library search paths above"
    exit 1
}

# dylibbundler's -od option recreates the Frameworks directory, which removes
# frameworks copied before it ran. Copy framework dependencies again after
# dependency resolution and before code signing.
while IFS= read -r dep; do
    case "$dep" in
        @executable_path/../Frameworks/*)
            rel="${dep#@executable_path/../Frameworks/}"
            [[ "$rel" == *.framework/* ]] || continue
            fw_name="${rel%%.framework/*}.framework"
            if [[ -n "$fw_name" && ! -d "$APP_DIR/Contents/Frameworks/$fw_name" ]]; then
                fw_src="$(find "$BUILD_DIR" "${CONAN_HOME:-$HOME/.conan2}/p" \
                    -maxdepth 8 -type d -name "$fw_name" -print -quit 2>/dev/null)"
                if [[ -n "$fw_src" ]]; then
                    cp -R "$fw_src" "$APP_DIR/Contents/Frameworks/"
                    echo "  Framework: $fw_name"
                else
                    echo "  WARNING: framework not found: $fw_name"
                fi
            fi
            ;;
    esac
done < <(otool -L "$APP_DIR/Contents/MacOS/$EXECUTABLE" | awk 'NR > 1 { print $1 }')

# Ensure plugins can locate frameworks via both @executable_path and @loader_path
while IFS= read -r -d '' plugin; do
    install_name_tool -add_rpath "@executable_path/../Frameworks" "$plugin" 2>/dev/null || true
    install_name_tool -add_rpath "@loader_path/../Frameworks"    "$plugin" 2>/dev/null || true
done < <(find "$APP_DIR/Contents/PlugIns" -maxdepth 1 -name '*.dylib' -type f -print0 2>/dev/null)

echo "[5/7] dylibbundler dependency resolution complete"

# dylibbundler 1.0.5 appends a new LC_RPATH entry for every dependency it
# rewrites. dyld on current macOS aborts if the same rpath appears twice, so
# collapse every duplicate to a single entry in all Mach-O files.
dedupe_rpaths() {
    local bin="$1"
    local rp
    while IFS= read -r rp; do
        [[ -n "$rp" ]] || continue
        while otool -l "$bin" | awk '/LC_RPATH/ { getline; getline; print $2 }' | grep -Fx -- "$rp" >/dev/null; do
            install_name_tool -delete_rpath "$rp" "$bin" 2>/dev/null || break
        done
        install_name_tool -add_rpath "$rp" "$bin" 2>/dev/null || true
    done < <(otool -l "$bin" | awk '/LC_RPATH/ { getline; getline; print $2 }' | sort -u)
}

echo "  Deduplicating LC_RPATH entries..."
while IFS= read -r -d '' bin; do
    [[ -L "$bin" ]] && continue
    case "$(file -b "$bin" 2>/dev/null)" in
        *Mach-O*) ;;
        *) continue ;;
    esac
    dedupe_rpaths "$bin"
done < <(find "$APP_DIR/Contents/MacOS" "$APP_DIR/Contents/Frameworks" "$APP_DIR/Contents/PlugIns" -type f -print0 2>/dev/null)

# ──────────────────────────────────────────────────────────────────
# 7. Ad-hoc code signing — strict inside-out hierarchy
#
#    macOS validates signatures from the outermost bundle inward.
#    Signing must proceed from the innermost binaries outward so that
#    each layer's seal covers already-signed children.
#
#    Layer 1 (deepest) : Contents/Frameworks/*.dylib, *.framework
#    Layer 2           : Contents/PlugIns/*.dylib
#    Layer 3           : Contents/MacOS/<executable>
#    Layer 4 (outer)   : The .app bundle itself
# ──────────────────────────────────────────────────────────────────
SIGN_COUNT=0

echo "  Signing layer 1: Frameworks..."
while IFS= read -r -d '' fw; do
    codesign --force --sign - --timestamp=none "$fw"
    SIGN_COUNT=$((SIGN_COUNT + 1))
done < <(find "$APP_DIR/Contents/Frameworks" -maxdepth 1 \( -name '*.dylib' -o -name '*.framework' \) -print0 2>/dev/null)

echo "  Signing layer 2: PlugIns..."
while IFS= read -r -d '' pl; do
    codesign --force --sign - --timestamp=none "$pl"
    SIGN_COUNT=$((SIGN_COUNT + 1))
done < <(find "$APP_DIR/Contents/PlugIns" -maxdepth 1 -name '*.dylib' -print0 2>/dev/null)

echo "  Signing layer 3: Executable..."
codesign --force --sign - --timestamp=none "$APP_DIR/Contents/MacOS/$EXECUTABLE"
SIGN_COUNT=$((SIGN_COUNT + 1))

echo "  Signing layer 4: App bundle..."
codesign --force --sign - --timestamp=none "$APP_DIR"
SIGN_COUNT=$((SIGN_COUNT + 1))

echo "[6/7] Code signing complete ($SIGN_COUNT objects signed)"

# ──────────────────────────────────────────────────────────────────
# 8. Final verification
# ──────────────────────────────────────────────────────────────────
codesign --verify --deep --strict "$APP_DIR"
echo "[7/7] Bundle signature verification PASSED"

echo ""
echo "=============================================="
echo " BUILD SUCCEEDED"
echo " Bundle : $APP_DIR"
echo " Size   : $(du -sh "$APP_DIR" | cut -f1)"
echo "=============================================="
