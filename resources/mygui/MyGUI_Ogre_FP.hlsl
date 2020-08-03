void main(
	uniform Texture2D<float4> sampleTexture : register(t0),
	uniform SamplerState sampleSampler : register(s0),
	in float4 inPosition : SV_POSITION,
	in float4 inColor : TEXCOORD0,
	in float2 inTexcoord : TEXCOORD1,
	out float4 Out : SV_TARGET )
{
	Out = sampleTexture.SampleLevel(sampleSampler, inTexcoord, 0).rgba * inColor;
}
