void main(in float3 inPosition : POSITION0, in float4 inColor : COLOR0, in float2 inTexcoord : TEXCOORD0, out float4 outPosition : SV_POSITION, out float4 outColor : TEXCOORD0, out float2 outTexcoord : TEXCOORD1)
{
	outPosition = float4(inPosition, 1.0f);
	outColor = inColor;
	outTexcoord = inTexcoord;
}
