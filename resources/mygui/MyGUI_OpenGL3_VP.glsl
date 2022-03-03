#version 130
out vec4 Color;
out vec2 TexCoord;
in vec3 VertexPosition;
in vec4 VertexColor;
in vec2 VertexTexCoord;
uniform float YScale;
void main()
{
	TexCoord = VertexTexCoord;
	Color = VertexColor;
	vec4 vpos = vec4(VertexPosition,1.0);
	vpos.y *= YScale;
	gl_Position = vpos;
}