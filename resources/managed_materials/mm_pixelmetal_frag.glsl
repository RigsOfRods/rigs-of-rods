// Project Rigs of Rods (www.rigsofrods.org)
// PixelMetal - monolithic cross-language shader to replace classic Cg shaders
// See comments in 'mm_pixelmetal.glsl'

#include <OgreUnifiedShader.h>
#include "mm_pixelmetal.glsl"

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

OGRE_NATIVE_GLSL_VERSION_DIRECTIVE

OGRE_UNIFORMS(
    uniform vec4 lightDiffuse; // color
    uniform vec4 lightSpecular; // color
    uniform float exponent;
    uniform vec4 ambient; // ambient light color
    
    uniform SAMPLER2D Diffuse_Map;
    uniform SAMPLER2D Specular_Map;
    uniform SAMPLER2D Dmg_Diffuse_Map;
)

MAIN_ARGUMENTS
    IN(vec4, pos, TEXCOORD0)
    IN(vec4, incol, COLOR)
    IN(vec3, normal, TEXCOORD1)
    IN(vec4, lightpos, TEXCOORD2)
    IN(vec3, eyepos, TEXCOORD3)
    IN(vec2, uv, TEXCOORD4)
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
    vec4 textColour = tex2D(Diffuse_Map, uv)*(1-incol.a);
    textColour=textColour+tex2D(Dmg_Diffuse_Map, uv)*incol.a;
#else
    vec4 textColour = tex2D(Diffuse_Map, uv);
#endif

#ifndef MM_NOWET    
    // ~~ Flexbody wetness effect: dim diffuse and amplify specular based on vertex color's blue value. ~~
    textColour=textColour*(1-incol.b/3);
    vec4 specColour = tex2D(Specular_Map, uv)+incol.b/3-incol.a/2;
#else
    vec4 specColour = tex2D(Specular_Map, uv);
#endif
    
    // ~~ Nicemetal secret sauce: `lerp()` by specular value, so as specular intensity increases, diffuse light is removed. ~~
    gl_FragColor = lerp(lightDiffuse * textColour * Lit.y + textColour*ambient, lightSpecular * Lit.z, specColour);
#ifdef MM_TRANSP
    gl_FragColor.a=tex2D(Diffuse_Map, uv).a;
#else
    gl_FragColor.a=1;
#endif
}
