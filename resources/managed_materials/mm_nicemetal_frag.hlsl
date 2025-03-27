
// hlslf output by Cg compiler
// cgc version 3.1.0013, build date Apr 18 2012
// command line args: -profile hlslf
// source file: nicemetal_mm.cg
//vendor NVIDIA Corporation
//version 3.1.0.13
//profile hlslf
//program main_nicemetal_fp
//semantic main_nicemetal_fp.lightDiffuse
//semantic main_nicemetal_fp.lightSpecular
//semantic main_nicemetal_fp.exponent
//semantic main_nicemetal_fp.ambient
//semantic main_nicemetal_fp.Diffuse_Map : TEXUNIT0
//semantic main_nicemetal_fp.Specular_Map : TEXUNIT1
//semantic main_nicemetal_fp.DmgDiffuse_Map : TEXUNIT2
//var float4 lightDiffuse :  : lightDiffuse : 6 : 1
//var float4 lightSpecular :  : lightSpecular : 7 : 1
//var float exponent :  : exponent : 8 : 1
//var float4 ambient :  : ambient : 9 : 1
//var sampler2D Diffuse_Map : TEXUNIT0 : Diffuse_Map 0 : 10 : 1
//var sampler2D Specular_Map : TEXUNIT1 : Specular_Map 1 : 11 : 1
//var sampler2D DmgDiffuse_Map : TEXUNIT2 : DmgDiffuse_Map 2 : 12 : 1
//var float4 pos : $vin.TEXCOORD0 : TEXCOORD0 : 0 : 1
//var float4 incol : $vin.COLOR : COLOR : 1 : 1
//var float3 normal : $vin.TEXCOORD1 : TEXCOORD1 : 2 : 1
//var float4 lightpos : $vin.TEXCOORD2 : TEXCOORD2 : 3 : 1
//var float3 eyepos : $vin.TEXCOORD3 : TEXCOORD3 : 4 : 1
//var float2 uv : $vin.TEXCOORD4 : TEXCOORD4 : 5 : 1
//var float4 oColor : $vout.COLOR : COLOR : 13 : 1

#pragma pack_matrix(row_major)



 // main procedure, the original name was main_nicemetal_fp
void main(in float4 _pos : TEXCOORD0, in float4 _incol : COLOR0, in float3 _normal : TEXCOORD1, in float4 _lightpos : TEXCOORD2, in float3 _eyepos : TEXCOORD3, in float2 _uv : TEXCOORD4, 
    uniform float4 lightDiffuse,
    uniform float4 lightSpecular, 
    uniform float exponent, 
    uniform float4 ambient, 
    uniform sampler2D Diffuse_Map : TEXUNIT0, 
    uniform sampler2D Specular_Map : TEXUNIT1, 
    uniform sampler2D DmgDiffuse_Map : TEXUNIT2,
    out float4 _oColor : COLOR0)
{
float4 _TMP2;
float4 _TMP1;
float4 _TMP0;
float _TMP7;
float _TMP5;
float _TMP6;
float _TMP4;
float _TMP3;
float3 _v0011;
float3 _v0013;
float3 _v0015;
float4 _TMP16;
float4 _tmp0017;
float4 _a0021;
float4 _b0021;

    float3 _N;
    float3 _EyeDir;
    float3 _LightDir;
    float3 _HalfAngle;
    float _NdotL;
    float _NdotH;
    float4 _textColour;
    float4 _specColour;

    _TMP3 = dot(_normal, _normal);
    _TMP4 = rsqrt(_TMP3);
    _N = _TMP4*_normal;
    _v0011 = _eyepos - _pos.xyz;
    _TMP3 = dot(_v0011, _v0011);
    _TMP4 = rsqrt(_TMP3);
    _EyeDir = _TMP4*_v0011;
    _v0013 = _lightpos.xyz - (_pos*_lightpos.w).xyz;
    _TMP3 = dot(_v0013, _v0013);
    _TMP4 = rsqrt(_TMP3);
    _LightDir = _TMP4*_v0013;
    _v0015 = _LightDir + _EyeDir;
    _TMP3 = dot(_v0015, _v0015);
    _TMP4 = rsqrt(_TMP3);
    _HalfAngle = _TMP4*_v0015;
    _NdotL = dot(_LightDir, _N);
    _NdotH = dot(_HalfAngle, _N);
    _tmp0017 = float4(_NdotL, _NdotH, exponent, exponent);
    if (_tmp0017.x >  0.00000000000000000E000f) { // if begin
        _TMP6 = max( 0.00000000000000000E000f, _tmp0017.y);
        _TMP5 = pow(_TMP6, _tmp0017.z);
    } else {
        _TMP5 =  0.00000000000000000E000f;
    } // end if
    _TMP7 = max( 0.00000000000000000E000f, _tmp0017.x);
    _TMP16 = float4( 1.00000000000000000E000f, _TMP7, _TMP5,  1.00000000000000000E000f);
    _TMP0 = tex2D(Diffuse_Map, _uv);
    _textColour = _TMP0*( 1.00000000000000000E000f - _incol.w);
    _TMP1 = tex2D(DmgDiffuse_Map, _uv);
    _textColour = _textColour + _TMP1*_incol.w;
    _textColour = _textColour*( 1.00000000000000000E000f - _incol.z/ 3.00000000000000000E000f);
    _TMP2 = tex2D(Specular_Map, _uv);
    _specColour = (_TMP2 + _incol.z/ 3.00000000000000000E000f) - _incol.w/ 2.00000000000000000E000f;
    _a0021 = (lightDiffuse*_textColour)*_TMP16.y + _textColour*ambient;
    _b0021 = lightSpecular*_TMP16.z;
    _oColor = _a0021 + _specColour*(_b0021 - _a0021);
    _oColor.w =  1.00000000000000000E000f;
} // main end
