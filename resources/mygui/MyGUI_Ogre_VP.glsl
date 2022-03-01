#version 130

precision highp int;
precision highp float;

in vec4 position;
in vec4 uv0;
in vec4 colour;
uniform float YFlipScale;
uniform mat4 worldViewProj;

out vec4 outUV0;
out vec4 outColor;

// Texturing vertex program for GLSL ES
void main()
{
	vec4 vpos = position;
	vpos.y *= YFlipScale;
	gl_Position = vpos;
	outUV0 = uv0;
	outColor = colour;
}
