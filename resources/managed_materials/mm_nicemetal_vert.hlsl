

 // vertex shader main entry
void main(
       in    float2 uv : TEXCOORD0,
       in     float4 position : POSITION, 
       in     float3 normal   : NORMAL,
       in     float4 cols     : COLOR0,
            uniform float4 lightPosition,
            uniform float3 eyePosition,
            uniform float4x4 worldviewproj,

            out float4 oClipPos : POSITION,
            out float4 oCols : COLOR0,
            out float4 oPos : TEXCOORD0,
            out float3 oNorm    : TEXCOORD1,
            out float4 oLightPos    : TEXCOORD2,            
            out float3 oEyePos  : TEXCOORD3,
            out float2 oUv     : TEXCOORD4
 ) 
{ 
    oClipPos = mul(worldviewproj, position);
    
    oPos = position;
    oNorm     = normal; 
    oLightPos = lightPosition;
    oEyePos = eyePosition;
    oCols=cols;
    oUv = uv;
} 
