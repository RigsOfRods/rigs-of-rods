// Project Rigs of Rods (www.rigsofrods.org)
// PixelMetal - monolithic cross-language shader to replace classic Cg shaders
// See comments in 'mm_pixelmetal.glsl'

#include <OgreUnifiedShader.h>
#include "mm_pixelmetal.glsl"

// Equivalent of 'reflect_nicemetal_vp'
// - same semantics of uniforms (all object-space)
// - same variable names
// NOTE: This should theoretically be inside the base pixelmetal shader,
//  but for maximum consistency with the existing (multipass) OGRE material setup,
//  we keep it separate.

OGRE_NATIVE_GLSL_VERSION_DIRECTIVE

OGRE_UNIFORMS(
    uniform mat4 worldViewProj;
    uniform mat4 world;
    uniform vec3 camPosition;
)

MAIN_PARAMETERS
    IN(vec4, position, POSITION)
    IN(vec2, uv, TEXCOORD0)
    IN(vec3, normal, NORMAL)
    IN(vec3, cols, COLOR)
    // `oPos` replaced by gl_Position
    OUT(vec2, oUv, TEXCOORD0)
    OUT(vec3, oViewDirection, TEXCOORD1)
    OUT(vec3, oNormal, TEXCOORD2)
    OUT(vec4, oCols, COLOR)
MAIN_DECLARATION
{
    oUv=uv;
	oCols=cols;
   gl_Position = mul(worldViewProj, position);
   oNormal = mul((mat3)world, normal);
   oViewDirection = mul((mat3)world, position.xyz - camPosition);
}
