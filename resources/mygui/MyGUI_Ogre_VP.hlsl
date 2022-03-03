void main(
	in float3 inPosition : POSITION0,
	in float4 inColor : COLOR0,
	in float2 inTexcoord : TEXCOORD0,
	uniform float YFlipScale,
	out float4 outPosition : SV_POSITION,
	out float4 outColor : TEXCOORD0,
	out float2 outTexcoord : TEXCOORD1 )
{
	float4 vpos = float4(inPosition, 1.0f);
	vpos.y *= YFlipScale;
	outPosition = vpos;
	outColor = inColor;
	outTexcoord = inTexcoord;
}
