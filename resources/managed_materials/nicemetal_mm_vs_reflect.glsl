#version 120

uniform vec3 camPosition;
uniform mat4 world;
uniform mat4 worldViewProj;

attribute vec4 vertex;
attribute vec3 normal;
attribute vec4 colour;
attribute vec2 uv0;

varying vec2 oUv;
varying vec3 oViewDirection;
varying vec3 oNormal;
varying vec4 oCols;

void main()
{
    oUv   = uv0;
    oCols = colour;

    gl_Position     = worldViewProj * vertex;
    oNormal         = mat3(world) * normal;
    oViewDirection  = mat3(world) * (vertex.xyz - camPosition);
}
