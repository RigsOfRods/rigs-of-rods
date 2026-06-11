// Project Rigs of Rods (www.rigsofrods.org)
// MM_PixelMetal - monolithic cross-language shader to replace classic 'nicemetal_mm.cg':
// * Based on Blinn-Phong lighting model.
// * Features: fake specular metalness, damage-tex blending, reflections.
// * Original entry points are replaced by preprocessor defines (detailed in code comments).

OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
#include <OgreUnifiedShader.h>

// Eequivalent of entry point 'main_nicemetal_vp'
// - same semantics of uniforms (all object-space)
// - same variable names

OGRE_UNIFORMS(
    uniform mat4 worldviewproj;
    uniform vec4 lightPosition;
    uniform vec3 eyePosition;
)

MAIN_PARAMETERS
    IN(vec2 uv, TEXCOORD0)
    IN(vec4 position, POSITION)
    IN(vec3 normal, NORMAL)
    IN(vec4 cols, COLOR)
    // `oClipPos` substituted by gl_Position
    OUT(vec4 oColor, COLOR)
    OUT(vec4 oPos, TEXCOORD0)
    OUT(vec3 oNorm, TEXCOORD1)
    OUT(vec4 oLightPos, TEXCOORD2)
    OUT(vec3 oEyePos, TEXCOORD3)
    OUT(vec2 oUv, TEXCOORD4)
MAIN_DECLARATION
{
    gl_Position = mul(worldviewproj, position);
    
    oPos = position;
    oNorm = normal;
    oLightPos = lightPosition;
    oEyePos = eyePosition;
    oColor = cols;
    oUv = uv;
}
