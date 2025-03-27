
// hlslf output by Cg compiler
// cgc version 3.1.0013, build date Apr 18 2012
// command line args: -profile hlslf
// source file: nicemetal_mm.cg
//vendor NVIDIA Corporation
//version 3.1.0.13
//profile hlslf
//program main_simplemetal_fp
//semantic main_simplemetal_fp.lightDiffuse
//semantic main_simplemetal_fp.lightSpecular
//semantic main_simplemetal_fp.exponent
//semantic main_simplemetal_fp.ambient
//semantic main_simplemetal_fp.Diffuse_Map : TEXUNIT0
//semantic main_simplemetal_fp.Specular_Map : TEXUNIT1
//var float4 lightDiffuse :  : lightDiffuse : 5 : 1
//var float4 lightSpecular :  : lightSpecular : 6 : 1
//var float exponent :  : exponent : 7 : 1
//var float4 ambient :  : ambient : 8 : 1
//var sampler2D Diffuse_Map : TEXUNIT0 : Diffuse_Map 0 : 9 : 1
//var sampler2D Specular_Map : TEXUNIT1 : Specular_Map 1 : 10 : 1
//var float4 pos : $vin.TEXCOORD0 : TEXCOORD0 : 0 : 1
//var float3 normal : $vin.TEXCOORD1 : TEXCOORD1 : 1 : 1
//var float4 lightpos : $vin.TEXCOORD2 : TEXCOORD2 : 2 : 1
//var float3 eyepos : $vin.TEXCOORD3 : TEXCOORD3 : 3 : 1
//var float2 uv : $vin.TEXCOORD4 : TEXCOORD4 : 4 : 1
//var float4 oColor : $vout.COLOR : COLOR : 11 : 1

#pragma pack_matrix(row_major)



 // main procedure, the original name was main_simplemetal_fp
void main(in float4 _pos : TEXCOORD0, in float3 _normal : TEXCOORD1, in float4 _lightpos : TEXCOORD2, in float3 _eyepos : TEXCOORD3, in float2 _uv : TEXCOORD4, 
    uniform float4 lightDiffuse, 
    uniform float4 lightSpecular, 
    uniform float exponent, 
    uniform float4 ambient, 
    uniform sampler2D Diffuse_Map : TEXUNIT0, 
    uniform sampler2D Specular_Map : TEXUNIT1, 
    out float4 _oColor : COLOR0)
{
float _TMP4;
float _TMP2;
float _TMP3;
float _TMP1;
float _TMP0;
float3 _v0008;
float3 _v0010;
float3 _v0012;
float4 _TMP13;
float4 _tmp0014;
float4 _a0018;
float4 _b0018;

    float3 _N;
    float3 _EyeDir;
    float3 _LightDir;
    float3 _HalfAngle;
    float _NdotL;
    float _NdotH;
    float4 _textColour;
    float4 _specColour;

    _TMP0 = dot(_normal, _normal);
    _TMP1 = rsqrt(_TMP0);
    _N = _TMP1*_normal;
    _v0008 = _eyepos - _pos.xyz;
    _TMP0 = dot(_v0008, _v0008);
    _TMP1 = rsqrt(_TMP0);
    _EyeDir = _TMP1*_v0008;
    _v0010 = _lightpos.xyz - (_pos*_lightpos.w).xyz;
    _TMP0 = dot(_v0010, _v0010);
    _TMP1 = rsqrt(_TMP0);
    _LightDir = _TMP1*_v0010;
    _v0012 = _LightDir + _EyeDir;
    _TMP0 = dot(_v0012, _v0012);
    _TMP1 = rsqrt(_TMP0);
    _HalfAngle = _TMP1*_v0012;
    _NdotL = dot(_LightDir, _N);
    _NdotH = dot(_HalfAngle, _N);
    _tmp0014 = float4(_NdotL, _NdotH, exponent, exponent);
    if (_tmp0014.x >  0.00000000000000000E000f) { // if begin
        _TMP3 = max( 0.00000000000000000E000f, _tmp0014.y);
        _TMP2 = pow(_TMP3, _tmp0014.z);
    } else {
        _TMP2 =  0.00000000000000000E000f;
    } // end if
    _TMP4 = max( 0.00000000000000000E000f, _tmp0014.x);
    _TMP13 = float4( 1.00000000000000000E000f, _TMP4, _TMP2,  1.00000000000000000E000f);
    _textColour = tex2D(Diffuse_Map, _uv);
    _specColour = tex2D(Specular_Map, _uv);
    _a0018 = (lightDiffuse*_textColour)*_TMP13.y + _textColour*ambient;
    _b0018 = lightSpecular*_TMP13.z;
    _oColor = _a0018 + _specColour*(_b0018 - _a0018);
    _oColor.w =  1.00000000000000000E000f;
} // main end
