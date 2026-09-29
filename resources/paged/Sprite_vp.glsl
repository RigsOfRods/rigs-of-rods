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

OUT(vec4 oUV, TEXCOORD0)
OUT(vec4 oColour, COLOR)
OUT(float oFogCoord, FOG)

MAIN_DECLARATION
{
    //Face the camera
	vec4 vCenter = vec4( vertex.x, vertex.y, vertex.z, 1.0 );
	vec4 vScale = vec4( normal.x, normal.y, normal.x , 1.0 );
	gl_Position = mul(worldViewProj, vCenter + (preRotatedQuad[int(normal.z)] * vScale) );

	//Color
	oColour = colour;

    //Fade out in the distance
#ifdef FADE
	vec4 position = vertex;
	float dist = distance(camPos.xz, position.xz);
	oColour.w = (invisibleDist - dist) / fadeGap;
#endif

    //UV scroll
	oUV = uv0;
	oUV.x += uScroll;
	oUV.y += vScroll;

    //Fog
	oFogCoord = gl_Position.z;
}