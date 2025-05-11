
// glslf output by Cg compiler
// cgc version 3.1.0013, build date Apr 18 2012
// command line args: -profile glslf
// source file: nicemetal_mm.cg
//vendor NVIDIA Corporation
//version 3.1.0.13
//profile glslf
//program main_simplemetal_transp_fp
//semantic main_simplemetal_transp_fp.lightDiffuse
//semantic main_simplemetal_transp_fp.lightSpecular
//semantic main_simplemetal_transp_fp.exponent
//semantic main_simplemetal_transp_fp.ambient
//semantic main_simplemetal_transp_fp.Diffuse_Map : TEXUNIT0
//semantic main_simplemetal_transp_fp.Specular_Map : TEXUNIT1
//var float4 lightDiffuse :  : _lightDiffuse1 : 5 : 1
//var float4 lightSpecular :  : _lightSpecular1 : 6 : 1
//var float exponent :  : _exponent1 : 7 : 1
//var float4 ambient :  : _ambient1 : 8 : 1
//var sampler2D Diffuse_Map : TEXUNIT0 : _Diffuse_Map1 0 : 9 : 1
//var sampler2D Specular_Map : TEXUNIT1 : _Specular_Map1 1 : 10 : 1
//var float4 pos : $vin.TEXCOORD0 : TEX0 : 0 : 1
//var float3 normal : $vin.TEXCOORD1 : TEX1 : 1 : 1
//var float4 lightpos : $vin.TEXCOORD2 : TEX2 : 2 : 1
//var float3 eyepos : $vin.TEXCOORD3 : TEX3 : 3 : 1
//var float2 uv : $vin.TEXCOORD4 : TEX4 : 4 : 1
//var float4 oColor : $vout.COLOR : COL : 11 : 1

#version 110

vec4 _oColor1;
vec4 _TMP0;
float _TMP5;
float _TMP3;
float _TMP4;
float _TMP2;
float _TMP1;
uniform vec4 _lightDiffuse1;
uniform vec4 _lightSpecular1;
uniform float _exponent1;
uniform vec4 _ambient1;
uniform sampler2D _Diffuse_Map1;
uniform sampler2D _Specular_Map1;
vec3 _v0019;
vec3 _v0025;
vec3 _v0031;
vec4 _TMP40;
vec4 _tmp0041;
vec4 _a0055;
vec4 _b0055;

 // main procedure, the original name was main_simplemetal_transp_fp
void main()
{

    vec3 _N;
    vec3 _EyeDir;
    vec3 _LightDir;
    vec3 _HalfAngle;
    float _NdotL;
    float _NdotH;
    vec4 _textColour;
    vec4 _specColour;

    _TMP1 = dot(gl_TexCoord[1].xyz, gl_TexCoord[1].xyz);
    _TMP2 = inversesqrt(_TMP1);
    _N = _TMP2*gl_TexCoord[1].xyz;
    _v0019 = gl_TexCoord[3].xyz - gl_TexCoord[0].xyz;
    _TMP1 = dot(_v0019, _v0019);
    _TMP2 = inversesqrt(_TMP1);
    _EyeDir = _TMP2*_v0019;
    _v0025 = gl_TexCoord[2].xyz - (gl_TexCoord[0]*gl_TexCoord[2].w).xyz;
    _TMP1 = dot(_v0025, _v0025);
    _TMP2 = inversesqrt(_TMP1);
    _LightDir = _TMP2*_v0025;
    _v0031 = _LightDir + _EyeDir;
    _TMP1 = dot(_v0031, _v0031);
    _TMP2 = inversesqrt(_TMP1);
    _HalfAngle = _TMP2*_v0031;
    _NdotL = dot(_LightDir, _N);
    _NdotH = dot(_HalfAngle, _N);
    _tmp0041 = vec4(_NdotL, _NdotH, _exponent1, _exponent1);
    if (_tmp0041.x > 0.00000000E+000) { // if begin
        _TMP4 = max(0.00000000E+000, _tmp0041.y);
        _TMP3 = pow(_TMP4, _tmp0041.z);
    } else {
        _TMP3 = 0.00000000E+000;
    } // end if
    _TMP5 = max(0.00000000E+000, _tmp0041.x);
    _TMP40 = vec4(1.00000000E+000, _TMP5, _TMP3, 1.00000000E+000);
    _textColour = texture2D(_Diffuse_Map1, gl_TexCoord[4].xy);
    _specColour = texture2D(_Specular_Map1, gl_TexCoord[4].xy);
    _a0055 = (_lightDiffuse1*_textColour)*_TMP40.y + _textColour*_ambient1;
    _b0055 = _lightSpecular1*_TMP40.z;
    _oColor1 = _a0055 + _specColour*(_b0055 - _a0055);
    _TMP0 = texture2D(_Diffuse_Map1, gl_TexCoord[4].xy);
    _oColor1.w = _TMP0.w;
    gl_FragColor = _oColor1;
} // main end
