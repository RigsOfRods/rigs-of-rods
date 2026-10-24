#version 150
// Derived from MyGUI_Ogre_FP.glsl in this directory (Rigs of Rods, GPL-3.0).
// MyGUI 3.4 OgrePlatform requests this exact filename.


uniform sampler2D sampler;

in vec4 outUV0;
in vec4 outColor;

out vec4 fragColor;

// Texturing fragment program for GLSL
void main()
{
	fragColor = outColor * texture(sampler, outUV0.xy);
}
