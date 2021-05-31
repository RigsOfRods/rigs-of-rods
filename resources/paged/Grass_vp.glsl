#include <OgreUnifiedShader.h>

#ifdef LIGHTING
uniform vec4 objSpaceLight;
uniform vec4 lightDiffuse;
uniform vec4 lightAmbient;
#endif

#ifdef ANIMATE
uniform float time;
uniform float frequency;
uniform vec4 direction;
#endif

#ifdef FADETECH_GROW
uniform float grassHeight;
#endif

uniform mat4 worldViewProj;
uniform vec3 camPos;
uniform float fadeRange;

MAIN_PARAMETERS

IN(vec4 vertex, POSITION)
#ifdef GRASSTECH_SPRITE
IN(vec4 normal, NORMAL)
#endif
IN(vec4 colour, COLOR)
IN(vec4 uv0, TEXCOORD0)

#ifdef OGRE_HLSL
OUT(vec4 gl_TexCoord[1], TEXCOORD0)
OUT(vec4 gl_FrontColor, COLOR)
OUT(float gl_FogFragCoord, FOG)
#endif

MAIN_DECLARATION
{
    vec4 color = colour;
    vec4 position = vertex;
    float dist = distance(camPos.xz, position.xz);

#ifdef LIGHTING
    vec3 light = normalize(objSpaceLight.xyz - (vertex.xyz * objSpaceLight.w));
    float diffuseFactor = max( dot( vec3(0.0,1.0,0.0), light), 0.0);
    color = (lightAmbient + diffuseFactor * lightDiffuse) * colour;
#endif

#ifdef FADETECH_ALPHA
    //Fade out in the distance
    color.w = 2.0 - (2.0 * dist / fadeRange);
#else
    color.w = 1.0;
#endif

#ifdef GRASSTECH_SPRITE
    //Face the camera
    vec3 dirVec = position.xyz - camPos.xyz;
    vec3 p = normalize(cross(vec3(0.0,1.0,0.0), dirVec));
    position += vec4(p.x * normal.x, normal.y, p.z * normal.x, 0.0);
#endif

#ifdef ANIMATE
    if (uv0.y == 0.0)
    {
        //Wave grass in breeze
        position += direction * sin(time + vertex.x * frequency);
    }
#endif

#if defined(BLEND) && defined(ANIMATE)
    else
    {
#elif defined(BLEND)
    if (uv0.y != 0.0)
    {
#endif
#ifdef BLEND
        //Blend the base of nearby grass into the terrain
        color.w = clamp(color.w, 0.0, 1.0) * 4.0 * ((dist / fadeRange) - 0.1);
    }
#endif

#ifdef FADETECH_GROW
    position.y -= grassHeight * clamp((2.0 * dist / fadeRange) - 1.0, 0.0, 1.0);
#endif

    gl_Position = mul(worldViewProj, position);
    gl_FrontColor = color;
    gl_TexCoord[0] = uv0;
    gl_FogFragCoord = gl_Position.z;
}