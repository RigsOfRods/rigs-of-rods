
// hlslf output by Cg compiler
// cgc version 3.1.0013, build date Apr 18 2012
// command line args: -profile hlslf
// source file: nicemetal_mm.cg
//vendor NVIDIA Corporation
//version 3.1.0.13
//profile hlslf
//program main_nicemetal_transp_fp_nodmg
//semantic main_nicemetal_transp_fp_nodmg.lightDiffuse
//semantic main_nicemetal_transp_fp_nodmg.lightSpecular
//semantic main_nicemetal_transp_fp_nodmg.exponent
//semantic main_nicemetal_transp_fp_nodmg.ambient
//semantic main_nicemetal_transp_fp_nodmg.Diffuse_Map : TEXUNIT0
//semantic main_nicemetal_transp_fp_nodmg.Specular_Map : TEXUNIT1
//var float4 lightDiffuse :  : lightDiffuse : 6 : 1
//var float4 lightSpecular :  : lightSpecular : 7 : 1
//var float exponent :  : exponent : 8 : 1
//var float4 ambient :  : ambient : 9 : 1
//var sampler2D Diffuse_Map : TEXUNIT0 : Diffuse_Map 0 : 10 : 1
//var sampler2D Specular_Map : TEXUNIT1 : Specular_Map 1 : 11 : 1
//var float4 pos : $vin.TEXCOORD0 : TEXCOORD0 : 0 : 1
//var float4 incol : $vin.COLOR : COLOR : 1 : 1
//var float3 normal : $vin.TEXCOORD1 : TEXCOORD1 : 2 : 1
//var float4 lightpos : $vin.TEXCOORD2 : TEXCOORD2 : 3 : 1
//var float3 eyepos : $vin.TEXCOORD3 : TEXCOORD3 : 4 : 1
//var float2 uv : $vin.TEXCOORD4 : TEXCOORD4 : 5 : 1
//var float4 oColor : $vout.COLOR : COLOR : 12 : 1

#pragma pack_matrix(row_major)


 // main procedure, the original name was main_nicemetal_transp_fp_nodmg
void main(in float4 _pos : TEXCOORD0, in float4 _incol : COLOR0, in float3 _normal : TEXCOORD1, in float4 _lightpos : TEXCOORD2, in float3 _eyepos : TEXCOORD3, in float2 _uv : TEXCOORD4, 
    uniform float4 lightDiffuse, 
    uniform float4 lightSpecular, 
    uniform float exponent, 
    uniform float4 ambient, 
    uniform sampler2D Diffuse_Map : TEXUNIT0, 
    uniform sampler2D Specular_Map : TEXUNIT1, 
    out float4 _oColor : COLOR0)
{
float4 _TMP1;
float4 _TMP0;
float _TMP6;
float _TMP4;
float _TMP5;
float _TMP3;
float _TMP2;
float3 _v0010;
float3 _v0012;
float3 _v0014;
float4 _TMP15;
float4 _tmp0016;
float4 _a0020;
float4 _b0020;


    float3 _N;
    float3 _EyeDir;
    float3 _LightDir;
    float3 _HalfAngle;
    float _NdotL;
    float _NdotH;
    float4 _textColour;
    float4 _specColour;

    _TMP2 = dot(_normal, _normal);
    _TMP3 = rsqrt(_TMP2);
    _N = _TMP3*_normal;
    _v0010 = _eyepos - _pos.xyz;
    _TMP2 = dot(_v0010, _v0010);
    _TMP3 = rsqrt(_TMP2);
    _EyeDir = _TMP3*_v0010;
    _v0012 = _lightpos.xyz - (_pos*_lightpos.w).xyz;
    _TMP2 = dot(_v0012, _v0012);
    _TMP3 = rsqrt(_TMP2);
    _LightDir = _TMP3*_v0012;
    _v0014 = _LightDir + _EyeDir;
    _TMP2 = dot(_v0014, _v0014);
    _TMP3 = rsqrt(_TMP2);
    _HalfAngle = _TMP3*_v0014;
    _NdotL = dot(_LightDir, _N);
    _NdotH = dot(_HalfAngle, _N);
    _tmp0016 = float4(_NdotL, _NdotH, exponent, exponent);
    if (_tmp0016.x >  0.00000000000000000E000f) { // if begin
        _TMP5 = max( 0.00000000000000000E000f, _tmp0016.y);
        _TMP4 = pow(_TMP5, _tmp0016.z);
    } else {
        _TMP4 =  0.00000000000000000E000f;
    } // end if
    _TMP6 = max( 0.00000000000000000E000f, _tmp0016.x);
    _TMP15 = float4( 1.00000000000000000E000f, _TMP6, _TMP4,  1.00000000000000000E000f);
    _textColour = tex2D(Diffuse_Map, _uv);
    _textColour = _textColour*( 1.00000000000000000E000f - _incol.z/ 3.00000000000000000E000f);
    _TMP0 = tex2D(Specular_Map, _uv);
    _specColour = (_TMP0 + _incol.z/ 3.00000000000000000E000f) - _incol.w/ 2.00000000000000000E000f;
    _a0020 = (lightDiffuse*_textColour)*_TMP15.y + _textColour*ambient;
    _b0020 = lightSpecular*_TMP15.z;
    _oColor = _a0020 + _specColour*(_b0020 - _a0020);
    _TMP1 = tex2D(Diffuse_Map, _uv);
    _oColor.w = _TMP1.w;
} // main end
