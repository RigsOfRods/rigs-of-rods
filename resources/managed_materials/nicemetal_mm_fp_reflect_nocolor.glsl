#version 120

uniform sampler2D Specular_Map;

varying vec2 oUv;
varying vec3 oViewDirection;
varying vec3 oNormal;

void main()
{
    // See nicemetal_mm_fp_reflect.glsl: keep a single sampler type so the
    // program links on Apple's OpenGL implementation.
    vec4 emissiveColor = texture2D(Specular_Map, oUv);

    vec4 outColour = emissiveColor * 0.5;
    outColour.a = 1.0;

    gl_FragColor = outColour;
}
