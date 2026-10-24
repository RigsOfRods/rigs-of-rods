# Desktop GLSL UI bootstrap

A renderer without fixed-function support that provides GLSL and the `glsl150`
profile uses explicit UI programs and OGRE's standard RTShaderSystem resolver.
This supplies the UI bootstrap, not a conversion of all game materials to GL3.
Fixed-function and other shader language paths keep their existing setup.

Pass `-DROR_RTSHADER_GLSL_DIR="/path/to/matching/OGRE/Media/RTShaderLib/GLSL"`
when configuring. Use the function library installed with the exact OGRE binary
being linked, not an unrelated source checkout or sample media directory.
`OGRE_MEDIA_DIR/RTShaderLib/GLSL` is the default if present. An explicit nonempty
invalid path fails configuration; the macOS GL3 profile requires the library.
Legacy configurations may omit it; selecting GL3 then diagnoses the missing
runtime files before initializing RTSS.

`cmake --build <build> --target ror_rtshader_resources` refreshes the complete
library in `bin/resources/rtshader-core` without linking the game. The target
also runs with the game build. It includes the OGRE MIT notice, identical to the
notice distributed by OGRE 1.11.6. The pinned 1.11.6 payload contains 25 GLSL
files. Keep this payload matched to the binary when upgrading OGRE.

The new `MyGUI_VP.glsl` and `MyGUI_FP.glsl` derive from this repository's
`MyGUI_Ogre_VP.glsl` and `MyGUI_Ogre_FP.glsl` under GPL-3.0. They retain MyGUI
3.4's clip-space position, YFlipScale, UV, colour, and texture-unit-zero ABI,
using the actual filenames requested by that backend and desktop GLSL 150.
The original resources remain available to their existing consumers.

Only the main viewport uses the RTSS scheme. Fully programmable UI techniques
stay in Default: the stock resolver declines generation for them, and OGRE
1.11.6's getBestTechnique() falls back to their supported Default techniques.
ImGui uploads its projection and binds both program parameter sets before its
manual draws; wallpaper uses OGRE's automatic WORLDVIEWPROJ during overlay draws.
Neither changes the existing vertex layout or coordinate and input behavior.

RTSS ownership begins only after `ShaderGenerator::initialize()` returns
successfully. The pinned OGRE 1.11 implementation has unsafe teardown for some
partially initialized states. If initialization throws, its original exception
ends startup and the partial singleton is abandoned for the process lifetime;
it is never captured or destroyed by this owner. There is no fallback or
in-process retry. A false initialization result also aborts startup. Release
main reports Ogre/runtime exceptions and returns failure; other uncaught
exceptions terminate the process. Cleanup of all exception categories is not
guaranteed. A future in-process recovery path requires a dependency repair.

The scoped RTSS owner must outlive any native callback owner, and must be
cleaned up while Root, MaterialManager, its scene manager and viewport are still
alive. Future global application teardown must preserve that order. Shader
program names are reused only within their resource group and survive for the
current renderer lifetime, independently of font texture rebuilds.

Compile and offline shader checks cannot establish visible output, blending,
focus, RTT orientation, DPI, scene rendering or runtime cleanup correctness.
These require separately authorized runtime testing.
