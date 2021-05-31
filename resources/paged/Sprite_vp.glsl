#include <OgreUnifiedShader.h>

uniform mat4 worldViewProj;
#ifdef FADE
uniform vec3  camPos;
uniform float fadeGap;
uniform float invisibleDist;
#endif
uniform float uScroll;
uniform float vScroll;
uniform vec4  preRotatedQuad[4];

MAIN_PARAMETERS

IN(vec4 vertex, POSITION)
IN(vec4 normal, NORMAL)
IN(vec4 colour, COLOR)
IN(vec4 uv0, TEXCOORD0)

#ifdef OGRE_HLSL
OUT(vec4 gl_TexCoord[1], TEXCOORD0)
OUT(vec4 gl_FrontColor, COLOR)
OUT(float gl_FogFragCoord, FOG)
#endif

MAIN_DECLARATION
{
    //Face the camera
	vec4 vCenter = vec4( vertex.x, vertex.y, vertex.z, 1.0 );
	vec4 vScale = vec4( normal.x, normal.y, normal.x , 1.0 );
	gl_Position = mul(worldViewProj, vCenter + (preRotatedQuad[int(normal.z)] * vScale) );

	//Color
	gl_FrontColor = colour;

    //Fade out in the distance
#ifdef FADE
	vec4 position = vertex;
	float dist = distance(camPos.xz, position.xz);
	gl_FrontColor.w = (invisibleDist - dist) / fadeGap;
#endif

    //UV scroll
	gl_TexCoord[0] = uv0;
	gl_TexCoord[0].x += uScroll;
	gl_TexCoord[0].y += vScroll;

    //Fog
	gl_FogFragCoord = gl_Position.z;
}