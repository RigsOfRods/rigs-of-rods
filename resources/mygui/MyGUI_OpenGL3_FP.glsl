#version 130
in vec4 Color;
in vec2 TexCoord;
uniform sampler2D Texture;
void main(void)
{
	gl_FragColor = texture2D(Texture, TexCoord) * Color;
}