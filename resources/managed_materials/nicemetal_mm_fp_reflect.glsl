#version 120

uniform sampler2D Specular_Map;

varying vec2 oUv;
varying vec3 oViewDirection;
varying vec3 oNormal;
varying vec4 oCols;

void main()
{
    // Environment (cube) reflection is unavailable here: Apple's OpenGL
    // rejects programs that mix sampler2D and samplerCube units at link time.
    // Keep the specular sheen pass working with a single sampler type.
    vec4 emissiveColor = texture2D(Specular_Map, oUv) + oCols.b / 3.0 - oCols.a / 2.0;

    vec4 outColour = emissiveColor * 0.5;
    outColour.a = 1.0;

    gl_FragColor = outColour;
}
