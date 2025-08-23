
// glslv output by Cg compiler
// cgc version 3.1.0013, build date Apr 18 2012
// command line args: -profile glslv
// source file: nicemetal_mm.cg
//vendor NVIDIA Corporation
//version 3.1.0.13
//profile glslv
//program reflect_nicemetal_vp
//semantic reflect_nicemetal_vp.camPosition
//semantic reflect_nicemetal_vp.world
//semantic reflect_nicemetal_vp.worldViewProj
//var float3 camPosition :  : _camPosition1 : 9 : 1
//var float4x4 world :  : _world1[0], 4 : 10 : 1
//var float4x4 worldViewProj :  : _worldViewProj1[0], 4 : 11 : 1
//var float4 position : $vin.POSITION : ATTR0 : 0 : 1
//var float3 normal : $vin.NORMAL : ATTR2 : 1 : 1
//var float2 uv : $vin.TEXCOORD0 : ATTR8 : 2 : 1
//var float4 cols : $vin.COLOR : ATTR3 : 3 : 1
//var float4 oPosition : $vout.POSITION : HPOS : 4 : 1
//var float2 oUv : $vout.TEXCOORD0 : TEX0 : 5 : 1
//var float3 oViewDirection : $vout.TEXCOORD1 : TEX1 : 6 : 1
//var float3 oNormal : $vout.TEXCOORD2 : TEX2 : 7 : 1
//var float4 ocols : $vout.COLOR : COL0 : 8 : 1

#version 110

vec4 _ocols1;
vec4 _oPosition1;
vec3 _oViewDirection1;
vec2 _oUv1;
vec3 _oNormal1;
uniform vec3 _camPosition1;
uniform vec4 _world1[4];
uniform vec4 _worldViewProj1[4];
vec4 _r0006;
vec3 _r0016;
vec3 _r0024;
vec3 _v0024;
vec3 _TMP31;
vec3 _TMP32;
vec3 _TMP33;
vec3 _TMP34;
vec3 _TMP35;
vec3 _TMP36;

 // main procedure, the original name was reflect_nicemetal_vp
void main()
{


    _oUv1 = gl_MultiTexCoord0.xy;
    _ocols1 = gl_Color;
    _r0006.x = dot(_worldViewProj1[0], gl_Vertex);
    _r0006.y = dot(_worldViewProj1[1], gl_Vertex);
    _r0006.z = dot(_worldViewProj1[2], gl_Vertex);
    _r0006.w = dot(_worldViewProj1[3], gl_Vertex);
    _oPosition1 = _r0006;
    _TMP31.x = _world1[0].x;
    _TMP31.y = _world1[0].y;
    _TMP31.z = _world1[0].z;
    _r0016.x = dot(_TMP31, gl_Normal);
    _TMP32.x = _world1[1].x;
    _TMP32.y = _world1[1].y;
    _TMP32.z = _world1[1].z;
    _r0016.y = dot(_TMP32, gl_Normal);
    _TMP33.x = _world1[2].x;
    _TMP33.y = _world1[2].y;
    _TMP33.z = _world1[2].z;
    _r0016.z = dot(_TMP33, gl_Normal);
    _oNormal1 = _r0016;
    _v0024 = gl_Vertex.xyz - _camPosition1;
    _TMP34.x = _world1[0].x;
    _TMP34.y = _world1[0].y;
    _TMP34.z = _world1[0].z;
    _r0024.x = dot(_TMP34, _v0024);
    _TMP35.x = _world1[1].x;
    _TMP35.y = _world1[1].y;
    _TMP35.z = _world1[1].z;
    _r0024.y = dot(_TMP35, _v0024);
    _TMP36.x = _world1[2].x;
    _TMP36.y = _world1[2].y;
    _TMP36.z = _world1[2].z;
    _r0024.z = dot(_TMP36, _v0024);
    _oViewDirection1 = _r0024;
    gl_TexCoord[2].xyz = _r0016;
    gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;
    gl_TexCoord[1].xyz = _r0024;
    gl_Position = _r0006;
    gl_FrontColor = gl_Color;
} // main end
