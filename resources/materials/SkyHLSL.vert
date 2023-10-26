float4 mainVP(
    float4 pos : POSITION,
    uniform float4x4 worldViewProj,
    out float4 projectionCoord : TEXCOORD0
) : POSITION
{
    projectionCoord = mul(worldViewProj, pos);
    return mul(worldViewProj, pos);
}