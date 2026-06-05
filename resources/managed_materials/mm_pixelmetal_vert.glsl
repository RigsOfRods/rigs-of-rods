// Project Rigs of Rods (www.rigsofrods.org)
// PixelMetal - monolithic cross-language shader to replace classic Cg shaders
// See comments in 'mm_pixelmetal.glsl'

#include <OgreUnifiedShader.h>
#include "mm_pixelmetal.glsl"

// Eequivalent of 'main_nicemetal_vp'
// - same semantics of uniforms (all object-space)
// - same variable names

OGRE_NATIVE_GLSL_VERSION_DIRECTIVE

OGRE_UNIFORMS(
    uniform mat4 worldviewproj;
    uniform vec4 lightPosition;
    uniform vec3 eyePosition;
)

MAIN_PARAMETERS
    IN(vec2, uv, TEXCOORD0)
    IN(vec4, position, POSITION)
    IN(vec3, normal, NORMAL)
    IN(vec3, cols, COLOR)
    // `oClipPos` replaced by gl_Position
    OUT(vec4, oColor, COLOR)
    OUT(vec4, oPos, TEXCOORD0)
    OUT(vec3, oNorm, TEXCOORD1)
    OUT(vec4, oLightPos, TEXCOORD2)
    OUT(vec3, oEyePos, TEXCOORD3)
    OUT(vec2, oUv, TEXCOORD4)
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
