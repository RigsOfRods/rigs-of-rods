attribute vec4 vertex;

uniform mat4 worldViewProj;

varying vec4 projectionCoord;

void main(void)
{
    gl_Position = worldViewProj * vertex;
    projectionCoord = worldViewProj * vertex;
}