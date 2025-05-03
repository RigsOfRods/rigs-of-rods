
void main(
       in  float4   position         : POSITION,
       in  float3   normal         : NORMAL,
       in  float2 uv : TEXCOORD0,
       in  float4 cols     : COLOR,

   out      float4   oPosition         : POSITION,
   out      float2   oUv         : TEXCOORD0,
   out      float3   oViewDirection      : TEXCOORD1,
   out      float3   oNormal         : TEXCOORD2,
   out         float4 ocols     : COLOR,

   uniform   float3   camPosition,
   uniform   float4x4   world,
   uniform   float4x4   worldViewProj)
{
	oUv=uv;
	ocols=cols;
   oPosition = mul(worldViewProj, position);
   oNormal = mul((float3x3)world, normal);
   oViewDirection = mul((float3x3)world, position.xyz - camPosition);
} 
