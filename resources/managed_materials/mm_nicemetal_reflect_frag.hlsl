
// hlslf output by Cg compiler
// cgc version 3.1.0013, build date Apr 18 2012
// command line args: -profile hlslf
// source file: nicemetal_mm.cg
//vendor NVIDIA Corporation
//version 3.1.0.13
//profile hlslf
//program reflect_nicemetal_fp
//semantic reflect_nicemetal_fp.Specular_Map : TEXUNIT0
//semantic reflect_nicemetal_fp.cubeMap : TEXUNIT1
//var sampler2D Specular_Map : TEXUNIT0 : Specular_Map 0 : 5 : 1
//var samplerCUBE cubeMap : TEXUNIT1 : cubeMap 1 : 6 : 1
//var float2 uv : $vin.TEXCOORD0 : TEXCOORD0 : 0 : 1
//var float3 viewDirection : $vin.TEXCOORD1 : TEXCOORD1 : 1 : 1
//var float3 normal : $vin.TEXCOORD2 : TEXCOORD2 : 2 : 1
//var float4 incol : $vin.COLOR : COLOR : 3 : 1
//var float4 oColor : $vout.COLOR : COLOR : 4 : 1

#pragma pack_matrix(row_major)



 // main procedure, the original name was reflect_nicemetal_fp
void main(in float2 _uv : TEXCOORD0, in float3 _viewDirection : TEXCOORD1, in float3 _normal : TEXCOORD2, in float4 _incol : COLOR0, out float4 _oColor : COLOR0, 
    uniform sampler2D Specular_Map : TEXUNIT0, 
    uniform samplerCUBE cubeMap : TEXUNIT1)
{
float4 _TMP0;
float _TMP3;
float _TMP2;
float _TMP1;

    float3 _N;
    float3 _R;
    float4 _reflectedColor;
    float4 _emissiveColor;

    _TMP1 = dot(_normal, _normal);
    _TMP2 = rsqrt(_TMP1);
    _N = _TMP2*_normal;
    _TMP3 = dot(_N, _viewDirection);
    _R = _viewDirection - ( 2.00000000000000000E000f*_N)*_TMP3;
    _R.z = -_R.z;
    _reflectedColor = texCUBE(cubeMap, _R);
    _TMP0 = tex2D(Specular_Map, _uv);
    _emissiveColor = (_TMP0 + _incol.z/ 3.00000000000000000E000f) - _incol.w/ 2.00000000000000000E000f;
    _oColor = _reflectedColor*_emissiveColor;
    _oColor.w =  1.00000000000000000E000f;
} // main end
