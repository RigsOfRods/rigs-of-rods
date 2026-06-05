// Project Rigs of Rods (www.rigsofrods.org)
// PixelMetal - monolithic cross-language shader to replace classic Cg shaders:
// * nicemetal.cg (fake specular metalness, damage-tex blending, reflections)
// * [FUTURE] pssm.cg (PSSM shadow rendering and mapping)
// Both these shader files had multiple entry points - detailed in code comments.
// Both were based on Blinn-Phong lighting model.
// For maximum consistency, the OGRE material/program setup was left as-is,
//  only replacing the shader code.

OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
#include <OgreUnifiedShader.h>

// GLSL equivalent of `lit()` function in Cg and HLSL, to make the new code resemble the old one more closely.
vec4 lit(float NdotL, float NdotH, float m)
{
    float diffuse = max(NdotL, 0.0);
    float specular = (NdotL > 0.0)
        ? pow(max(NdotH, 0.0), m)
        : 0.0;

    return vec4(1.0, diffuse, specular, 1.0);
}
