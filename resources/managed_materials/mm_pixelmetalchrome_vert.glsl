// Project Rigs of Rods (www.rigsofrods.org)
// MM_PixelMetal - monolithic cross-language shader to replace classic 'nicemetal_mm.cg':
// * Based on Blinn-Phong lighting model.
// * Features: fake specular metalness, damage-tex blending, reflections.
// * Original entry points are replaced by preprocessor defines (detailed in code comments).

OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
#include <OgreUnifiedShader.h>

// Equivalent of 'reflect_nicemetal_vp'
// - same semantics of uniforms (all object-space)
// - same variable names
// NOTE: This should theoretically be inside the base pixelmetal shader,
//  but for maximum consistency with the existing (multipass) OGRE material setup,
//  we keep it separate.

OGRE_UNIFORMS(
    uniform mat4 worldViewProj;
    uniform mat3 world;
    uniform vec3 camPosition;
)

MAIN_PARAMETERS
    IN(vec4 position, POSITION)
    IN(vec2 uv, TEXCOORD0)
    IN(vec3 normal, NORMAL)
    IN(vec4 cols, COLOR)
    // `oPos` substituted by gl_Position
    OUT(vec2 oUv, TEXCOORD0)
    OUT(vec3 oViewDirection, TEXCOORD1)
    OUT(vec3 oNormal, TEXCOORD2)
    OUT(vec4 oCols, COLOR)
MAIN_DECLARATION
{
    oUv=uv;
	oCols=cols;
    gl_Position = mul(worldViewProj, position);
    oNormal = mul(world, normal);
    oViewDirection = mul(world, position.xyz - camPosition);
}
