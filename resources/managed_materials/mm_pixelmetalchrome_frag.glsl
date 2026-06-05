// Project Rigs of Rods (www.rigsofrods.org)
// PixelMetal - monolithic cross-language shader to replace classic Cg shaders
// See comments in 'mm_pixelmetal.glsl'

#include <OgreUnifiedShader.h>
#include "mm_pixelmetal.glsl"

// Equivalent of 'reflect_nicemetal*_fp'
// - same semantics of uniforms (all object-space)
// - same variable names
// NOTE: This should theoretically be inside the base pixelmetal shader,
//  but for maximum consistency with the existing (multipass) OGRE material setup,
//  we keep it separate.

OGRE_NATIVE_GLSL_VERSION_DIRECTIVE

OGRE_UNIFORMS(
   uniform SAMPLER2D Specular_Map;
   uniform SAMPLERCUBE cubeMap;
)

MAIN_PARAMETERS
       IN(vec2, uv, TEXCOORD0)
       IN(vec3, viewDirection, TEXCOORD1)
       IN(vec3, normal, TEXCOORD2)
       IN(vec4, incol, COLOR)
MAIN_DECLARATION
{
   vec3 N = normalize(normal);
   vec3 R = reflect(viewDirection, N);
   R.z = -R.z;   // <-- should not have to do this? UPDATE: likely needed because reflect should get `-viewDirection` instead of `viewDirection`.

   vec4 reflectedColor = texCUBE(cubeMap, R);
   vec4 emissiveColor = tex2D(Specular_Map, uv)+incol.b/3-incol.a/2;

   gl_FragColor = reflectedColor*emissiveColor;
   gl_FragColor.a=1;
}       
