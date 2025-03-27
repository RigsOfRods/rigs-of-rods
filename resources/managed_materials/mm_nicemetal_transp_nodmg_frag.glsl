
// glslf output by Cg compiler
// cgc version 3.1.0013, build date Apr 18 2012
// command line args: -profile glslf
// source file: nicemetal_mm.cg
//vendor NVIDIA Corporation
//version 3.1.0.13
//profile glslf
//program main_nicemetal_transp_fp_nodmg
//semantic main_nicemetal_transp_fp_nodmg.lightDiffuse
//semantic main_nicemetal_transp_fp_nodmg.lightSpecular
//semantic main_nicemetal_transp_fp_nodmg.exponent
//semantic main_nicemetal_transp_fp_nodmg.ambient
//semantic main_nicemetal_transp_fp_nodmg.Diffuse_Map : TEXUNIT0
//semantic main_nicemetal_transp_fp_nodmg.Specular_Map : TEXUNIT1
//var float4 lightDiffuse :  : _lightDiffuse1 : 6 : 1
//var float4 lightSpecular :  : _lightSpecular1 : 7 : 1
//var float exponent :  : _exponent1 : 8 : 1
//var float4 ambient :  : _ambient1 : 9 : 1
//var sampler2D Diffuse_Map : TEXUNIT0 : _Diffuse_Map1 0 : 10 : 1
//var sampler2D Specular_Map : TEXUNIT1 : _Specular_Map1 1 : 11 : 1
//var float4 pos : $vin.TEXCOORD0 : TEX0 : 0 : 1
//var float4 incol : $vin.COLOR : COL0 : 1 : 1
//var float3 normal : $vin.TEXCOORD1 : TEX1 : 2 : 1
//var float4 lightpos : $vin.TEXCOORD2 : TEX2 : 3 : 1
//var float3 eyepos : $vin.TEXCOORD3 : TEX3 : 4 : 1
//var float2 uv : $vin.TEXCOORD4 : TEX4 : 5 : 1
//var float4 oColor : $vout.COLOR : COL : 12 : 1

#version 110

vec4 _oColor1;
vec4 _TMP1;
vec4 _TMP0;
float _TMP6;
float _TMP4;
float _TMP5;
float _TMP3;
float _TMP2;
uniform vec4 _lightDiffuse1;
uniform vec4 _lightSpecular1;
uniform float _exponent1;
uniform vec4 _ambient1;
uniform sampler2D _Diffuse_Map1;
uniform sampler2D _Specular_Map1;
vec3 _v0020;
vec3 _v0026;
vec3 _v0032;
vec4 _TMP41;
vec4 _tmp0042;
vec4 _a0056;
vec4 _b0056;

 // main procedure, the original name was main_nicemetal_transp_fp_nodmg
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

    _TMP2 = dot(gl_TexCoord[1].xyz, gl_TexCoord[1].xyz);
    _TMP3 = inversesqrt(_TMP2);
    _N = _TMP3*gl_TexCoord[1].xyz;
    _v0020 = gl_TexCoord[3].xyz - gl_TexCoord[0].xyz;
    _TMP2 = dot(_v0020, _v0020);
    _TMP3 = inversesqrt(_TMP2);
    _EyeDir = _TMP3*_v0020;
    _v0026 = gl_TexCoord[2].xyz - (gl_TexCoord[0]*gl_TexCoord[2].w).xyz;
    _TMP2 = dot(_v0026, _v0026);
    _TMP3 = inversesqrt(_TMP2);
    _LightDir = _TMP3*_v0026;
    _v0032 = _LightDir + _EyeDir;
    _TMP2 = dot(_v0032, _v0032);
    _TMP3 = inversesqrt(_TMP2);
    _HalfAngle = _TMP3*_v0032;
    _NdotL = dot(_LightDir, _N);
    _NdotH = dot(_HalfAngle, _N);
    _tmp0042 = vec4(_NdotL, _NdotH, _exponent1, _exponent1);
    if (_tmp0042.x > 0.00000000E+000) { // if begin
        _TMP5 = max(0.00000000E+000, _tmp0042.y);
        _TMP4 = pow(_TMP5, _tmp0042.z);
    } else {
        _TMP4 = 0.00000000E+000;
    } // end if
    _TMP6 = max(0.00000000E+000, _tmp0042.x);
    _TMP41 = vec4(1.00000000E+000, _TMP6, _TMP4, 1.00000000E+000);
    _textColour = texture2D(_Diffuse_Map1, gl_TexCoord[4].xy);
    _textColour = _textColour*(1.00000000E+000 - gl_Color.z/3.00000000E+000);
    _TMP0 = texture2D(_Specular_Map1, gl_TexCoord[4].xy);
    _specColour = (_TMP0 + gl_Color.z/3.00000000E+000) - gl_Color.w/2.00000000E+000;
    _a0056 = (_lightDiffuse1*_textColour)*_TMP41.y + _textColour*_ambient1;
    _b0056 = _lightSpecular1*_TMP41.z;
    _oColor1 = _a0056 + _specColour*(_b0056 - _a0056);
    _TMP1 = texture2D(_Diffuse_Map1, gl_TexCoord[4].xy);
    _oColor1.w = _TMP1.w;
    gl_FragColor = _oColor1;
} // main end
