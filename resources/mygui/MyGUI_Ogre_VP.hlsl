void main(
	in float4 inPosition : POSITION0,
	in float4 inColor : COLOR0,
	in float2 inTexcoord : TEXCOORD0,
	uniform float4x4 worldViewProj,
	out float4 outPosition : SV_POSITION,
	out float4 outColor : TEXCOORD0,
	out float2 outTexcoord : TEXCOORD1 )
{
	outPosition = mul(worldViewProj, inPosition);
	outColor = inColor;
	outTexcoord = inTexcoord;
}
