// Project Rigs of Rods (www.rigsofrods.org)
// MM_PixelMetal - monolithic cross-language shader to replace classic 'nicemetal_mm.cg':
// * Based on Blinn-Phong lighting model.
// * Features: fake specular metalness, damage-tex blending, reflections.
// * Original entry points are replaced by preprocessor defines (detailed in code comments).

OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
#include <OgreUnifiedShader.h>

// This shader replaces '*nicemetal*' and '*simplemetal*' fragment programs:
// - same semantics of uniforms
// - same variable names and values (all positions are object-space)
// - calculation is done in object space

// Preprocessor defines
// ====================
// * None: Equivalent of 'main_nicemetal_fp'
// * With MM_TRANSP: Equivalent of 'main_nicemetal_transp_fp'
// * With MM_NODMG: Equivalent of 'main_nicemetal_fp_nodmg'
// * With MM_TRANSP & MM_NODMG: Equivalent of 'main_nicemetal_transp_fp_nodmg'
// * With MM_NOWET & MM_NODMG: Equivalent of 'main_simplemetal_fp'
// * With MM_NOWET & MM_NODMG & MM_TRANSP: Equivalent of 'main_simplemetal_transp_fp'

OGRE_UNIFORMS(
    uniform vec4 lightDiffuse;
    uniform vec4 lightSpecular;
    uniform float exponent;
    uniform vec4 ambient;
    
    SAMPLER2D(Diffuse_Map, 0);
    SAMPLER2D(Specular_Map, 1);
    SAMPLER2D(Dmg_Diffuse_Map, 2);
)

// GLSL equivalent of builtin `lit()` function in Cg and HLSL - defined to make the new code resemble the old one more closely.
vec4 lit(float NdotL, float NdotH, float m)
{
    float diffuse = max(NdotL, 0.0);
    float specular = (NdotL > 0.0)
        ? pow(max(NdotH, 0.0), m)
        : 0.0;

    return vec4(1.0, diffuse, specular, 1.0);
}

// GLSL `lerp()` alias - defined to make the new code resemble the old one more closely.
#define lerp mix

MAIN_PARAMETERS
    IN(vec4 pos, TEXCOORD0)
    IN(vec4 incol, COLOR)
    IN(vec3 normal, TEXCOORD1)
    IN(vec4 lightpos, TEXCOORD2)
    IN(vec3 eyepos, TEXCOORD3)
    IN(vec2 uv, TEXCOORD4)
MAIN_DECLARATION
{
    // ~~ Standard Blinn-Phong lighting ~~
    vec3 N = normalize(normal);
    vec3 EyeDir = normalize(eyepos - pos.xyz);
    vec3 LightDir = normalize(lightpos.xyz -  (pos * lightpos.w).xyz);
    vec3 HalfAngle = normalize(LightDir + EyeDir);
    
    float NdotL = dot(LightDir, N);
    float NdotH = dot(HalfAngle, N);
    vec4 Lit = lit(NdotL,NdotH,exponent);
    
#ifndef MM_NODMG 
    // ~~ Flexbody damage effect: blend Diffuse and Dmg_Diffuse based on vertex color's alpha value. ~~
    vec4 textColour = texture2D(Diffuse_Map, uv)*(1-incol.a);
    textColour=textColour+texture2D(Dmg_Diffuse_Map, uv)*incol.a;
#else
    vec4 textColour = texture2D(Diffuse_Map, uv);
#endif

#ifndef MM_NOWET    
    // ~~ Flexbody wetness effect: dim diffuse and amplify specular based on vertex color's blue value. ~~
    textColour=textColour*(1-incol.b/3);
    vec4 specColour = texture2D(Specular_Map, uv)+incol.b/3-incol.a/2;
#else
    vec4 specColour = texture2D(Specular_Map, uv);
#endif
    
    // ~~ Nicemetal secret sauce: `lerp()` by specular value, so as specular intensity increases, diffuse light is removed. ~~
    gl_FragColor = lerp(lightDiffuse * textColour * Lit.y + textColour*ambient, lightSpecular * Lit.z, specColour);
#ifdef MM_TRANSP
    gl_FragColor.a=texture2D(Diffuse_Map, uv).a;
#else
    gl_FragColor.a=1;
#endif
}
