#version 120

uniform mat4 worldviewproj;
uniform vec4 lightPosition;
uniform vec3 eyePosition;

attribute vec4 vertex;
attribute vec3 normal;
attribute vec4 colour;
attribute vec2 uv0;

varying vec4 oPos;
varying vec4 oCols;
varying vec3 oNorm;
varying vec4 oLightPos;
varying vec3 oEyePos;
varying vec2 oUv;

void main()
{
    gl_Position = worldviewproj * vertex;

    oPos      = vertex;
    oNorm     = normal;
    oLightPos = lightPosition;
    oEyePos   = eyePosition;
    oCols     = colour;
    oUv       = uv0;
}
