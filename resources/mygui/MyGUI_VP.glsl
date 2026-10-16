#version 150
// Derived from MyGUI_Ogre_VP.glsl in this directory (Rigs of Rods, GPL-3.0).
// MyGUI 3.4 OgrePlatform requests this exact filename.


in vec4 position;
in vec4 uv0;
in vec4 colour;
uniform float YFlipScale;

out vec4 outUV0;
out vec4 outColor;

// Texturing vertex program for desktop GLSL
void main()
{
	vec4 vpos = position;
	vpos.y *= YFlipScale;
	gl_Position = vpos;
	outUV0 = uv0;
	outColor = colour;
}
