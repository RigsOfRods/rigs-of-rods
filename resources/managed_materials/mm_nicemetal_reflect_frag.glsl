
// glslf output by Cg compiler
// cgc version 3.1.0013, build date Apr 18 2012
// command line args: -profile glslf
// source file: nicemetal_mm.cg
//vendor NVIDIA Corporation
//version 3.1.0.13
//profile glslf
//program reflect_nicemetal_fp
//semantic reflect_nicemetal_fp.Specular_Map : TEXUNIT0
//semantic reflect_nicemetal_fp.cubeMap : TEXUNIT1
//var sampler2D Specular_Map : TEXUNIT0 : _Specular_Map1 0 : 5 : 1
//var samplerCUBE cubeMap : TEXUNIT1 : _cubeMap1 1 : 6 : 1
//var float2 uv : $vin.TEXCOORD0 : TEX0 : 0 : 1
//var float3 viewDirection : $vin.TEXCOORD1 : TEX1 : 1 : 1
//var float3 normal : $vin.TEXCOORD2 : TEX2 : 2 : 1
//var float4 incol : $vin.COLOR : COL0 : 3 : 1
//var float4 oColor : $vout.COLOR : COL : 4 : 1

#version 110

vec4 _oColor1;
vec4 _TMP0;
float _TMP3;
float _TMP2;
float _TMP1;
uniform sampler2D _Specular_Map1;
uniform samplerCube _cubeMap1;

 // main procedure, the original name was reflect_nicemetal_fp
void main()
{

    vec3 _N;
    vec3 _R;
    vec4 _reflectedColor;
    vec4 _emissiveColor;

    _TMP1 = dot(gl_TexCoord[2].xyz, gl_TexCoord[2].xyz);
    _TMP2 = inversesqrt(_TMP1);
    _N = _TMP2*gl_TexCoord[2].xyz;
    _TMP3 = dot(_N, gl_TexCoord[1].xyz);
    _R = gl_TexCoord[1].xyz - (2.00000000E+000*_N)*_TMP3;
    _R.z = -_R.z;
    _reflectedColor = textureCube(_cubeMap1, _R);
    _TMP0 = texture2D(_Specular_Map1, gl_TexCoord[0].xy);
    _emissiveColor = (_TMP0 + gl_Color.z/3.00000000E+000) - gl_Color.w/2.00000000E+000;
    _oColor1 = _reflectedColor*_emissiveColor;
    _oColor1.w = 1.00000000E+000;
    gl_FragColor = _oColor1;
} // main end
