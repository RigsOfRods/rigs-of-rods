#version 130

precision highp int;
precision highp float;

uniform sampler2D sampleTexture;

in vec4 outUV0;
in vec4 outColor;

out vec4 fragColor;

// Texturing fragment program for GLSL
void main()
{
	fragColor = outColor * texture(sampleTexture, outUV0.xy);
}
