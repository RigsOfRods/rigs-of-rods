void main(
	uniform sampler2D sampleTexture : register(s0),
	in float4 inPosition : POSITION,
	in float4 inColor : TEXCOORD0,
	in float2 inTexcoord : TEXCOORD1,
	out float4 Out : COLOR )
{
	Out = tex2D(sampleTexture, inTexcoord) * inColor;
}
