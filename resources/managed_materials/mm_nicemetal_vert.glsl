
// glslv output by Cg compiler
// cgc version 3.1.0013, build date Apr 18 2012
// command line args: -profile glslv
// source file: nicemetal_mm.cg
//vendor NVIDIA Corporation
//version 3.1.0.13
//profile glslv
//program main_nicemetal_vp
//semantic main_nicemetal_vp.lightPosition
//semantic main_nicemetal_vp.eyePosition
//semantic main_nicemetal_vp.worldviewproj
//var float4 lightPosition :  : _lightPosition1 : 4 : 1
//var float3 eyePosition :  : _eyePosition1 : 5 : 1
//var float4x4 worldviewproj :  : _worldviewproj1[0], 4 : 6 : 1
//var float2 uv : $vin.TEXCOORD0 : ATTR8 : 0 : 1
//var float4 position : $vin.POSITION : ATTR0 : 1 : 1
//var float3 normal : $vin.NORMAL : ATTR2 : 2 : 1
//var float4 cols : $vin.COLOR : ATTR3 : 3 : 1
//var float4 oClipPos : $vout.POSITION : HPOS : 7 : 1
//var float4 oCols : $vout.COLOR : COL0 : 8 : 1
//var float4 oPos : $vout.TEXCOORD0 : TEX0 : 9 : 1
//var float3 oNorm : $vout.TEXCOORD1 : TEX1 : 10 : 1
//var float4 oLightPos : $vout.TEXCOORD2 : TEX2 : 11 : 1
//var float3 oEyePos : $vout.TEXCOORD3 : TEX3 : 12 : 1
//var float2 oUv : $vout.TEXCOORD4 : TEX4 : 13 : 1

#version 110

vec4 _oCols1;
vec4 _oClipPos1;
vec2 _oUv1;
vec3 _oNorm1;
vec3 _oEyePos1;
vec4 _oPos1;
vec4 _oLightPos1;
uniform vec4 _lightPosition1;
uniform vec3 _eyePosition1;
uniform vec4 _worldviewproj1[4];
vec4 _r0004;

 // main procedure, the original name was main_nicemetal_vp
void main()
{


    _r0004.x = dot(_worldviewproj1[0], gl_Vertex);
    _r0004.y = dot(_worldviewproj1[1], gl_Vertex);
    _r0004.z = dot(_worldviewproj1[2], gl_Vertex);
    _r0004.w = dot(_worldviewproj1[3], gl_Vertex);
    _oClipPos1 = _r0004;
    _oPos1 = gl_Vertex;
    _oNorm1 = gl_Normal;
    _oLightPos1 = _lightPosition1;
    _oEyePos1 = _eyePosition1;
    _oCols1 = gl_Color;
    _oUv1 = gl_MultiTexCoord0.xy;
    gl_TexCoord[2] = _lightPosition1;
    gl_TexCoord[0] = gl_Vertex;
    gl_TexCoord[3].xyz = _eyePosition1;
    gl_TexCoord[1].xyz = gl_Normal;
    gl_TexCoord[4].xy = gl_MultiTexCoord0.xy;
    gl_Position = _r0004;
    gl_FrontColor = gl_Color;
} // main end
