# macOS port build notes

Everything here is a source-level fix for the Apple Silicon port; there are
**no binary patches** involved in producing the `.app` bundle. This document
explains what to run and what is committed.

## Building from scratch

```bash
bash tools/macos/build.sh [build-dir]
```

The script does the following in order:

1. Ensures the Conan profile carries the exact conf this port was built
   with (macOS SDK path, legacy libc++ compatibility flags for the old
   OIS/OGRE/PagedGeometry recipes, and the Ogre CMake options:
   frameworks off, CG plugin off, legacy GL renderer on, GL3+ off —
   the app loads `RenderSystem_GL`, per `deployment/plugins.cfg`). The
   block is rewritten deterministically on every run.
2. Registers the legacy Rigs-of-Rods dependency remote (for OIS & co.).
3. Builds and installs AngelScript from the pinned upstream commit
   (`762ab03 — Update to 2.38.0`) into `.deps/angelscript-install`,
   arm64-only with the deployment target at macOS 12.0. The build dir is
   always reconfigured from scratch so no stale SDK flags survive.
4. Seeds the local Conan cache with the patched recipes in
   `tools/macos/conan-recipes/`. Those recipes carry the exact
   source-level fixes that the historical binary-only port relied on:
   - `zlib/1.3`                   — `TARGET_OS_MAC` OS-code guard corrected.
   - `libpng/1.6.40`              — `<fp.h>` → `<math.h>` include fix.
   - `freeimage/3.18.0@…/stable`  — recipe source traded for MonoGame's
     maintained mirror (SourceForge is flaky).
   - `ogre3d/1.11.6.1@…/stable`   — no more `-framework AGL` link; Apple
     NEON software-skinning disabled so the player character mesh is not
     corrupted (`getCpuFeatures` returns 0 on arm64-apple); `macosx`
     sysroot placeholder removed; x86 legacy `OGRE_CPU` fallback
     restricted to Intel; optional x86-only Cg Toolkit requirement gone.
   Because Conan resolves these recipes from the local cache before any
   remote, a fresh machine gets exactly the same dependency sources.
5. Configures and builds RoR with the exact release flags the current
   `build-local` build used.
6. Runs `tools/macos/bundle.sh` to produce the final, ad-hoc signed
   `RigsOfRods.app` in the build dir.

## GitHub Actions

`.github/workflows/macos-arm64.yml` runs `tools/macos/build.sh` as its
single build step on a `macos-14` runner and uploads
`build-local/RigsOfRods.app` as the artifact — identical flow to a local
from-scratch build. Trigger it with `workflow_dispatch`, or push to
`main`/`master`.

## Files of interest

| Path                          | Purpose                                     |
|-------------------------------|---------------------------------------------|
| `tools/macos/build.sh`        | Reproducible end-to-end build script.       |
| `tools/macos/bundle.sh`       | Stages and ad-hoc signs the `.app` bundle.  |
| `tools/macos/conan-recipes/*` | Vendored, patched dependency recipes (+ their `patches/` trees). |
| `.github/workflows/macos-arm64.yml` | CI build (macos-14/Actions runner).   |

## Reproducing locally

The only external requirements are `cmake`, `ninja`, `xcodebuild`/Xcode
and `conan` 2.x (plus `dylibbundler` for bundling). Everything else —
dependency sources, patches, flags — is self-contained in the
repository.

## Notes

All macOS-specific behaviour lives under `source/main` (rendering, config
defaults, plugin loading) and `resources/` (SkyX, managed materials,
fonts, overlays). The remaining code touches only what is required to
keep shadows and Caelum/PSSM permanently unavailable in this port.
