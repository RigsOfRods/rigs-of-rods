/*
--------------------------------------------------------------------------------
This source file is part of SkyX.
Visit http://www.paradise-studios.net/products/skyx/

Copyright (C) 2009-2012 Xavier Vergu�n Gonz�lez <xavyiy@gmail.com>

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
--------------------------------------------------------------------------------
*/

// ------------------------- SkyX clouds -----------------------------

void main_vp(
    // IN
	float4 iPosition	        : POSITION,
	float3 iNPosition           : TEXCOORD0,
	// OUT
	out float4 oPosition		: POSITION,
	out float3 oPosition_       : TEXCOORD0,
	// UNIFORM
	uniform float4x4 uWorldViewProj)
{
    // Clip space position
	oPosition   = mul(uWorldViewProj, iPosition);

    oPosition_  = iNPosition;
}

void main_fp(
    // IN
    float3 iPosition       : TEXCOORD0,
	// OUT 
	out float4 oColor		: COLOR,
	// UNIFORM
	uniform float     uExposure,
	// Sun information
	uniform float3    uSunColor,
	// Main cloud layer parameters
	uniform float     uHeight,
	uniform float     uTime,
	uniform float     uScale,
	uniform float2    uWindDirection,
	// Advanced cloud layer parameters
	uniform float     uCloudLayerHeightVolume, // 0.25
	uniform float     uCloudLayerVolumetricDisplacement, // 0.01
	uniform float3    uAmbientLuminosity, // 0.55 0.55 0.55
	uniform float     uDetailAttenuation, // 0.45
	uniform float     uDistanceAttenuation, // 0.05
	uniform sampler2D uClouds : register(s0),
	uniform sampler2D uCloudsNormal : register(s1),
	uniform sampler2D uCloudsTile : register(s2))
{
    // Get the cloud pixel lenght on the projected plane
    float vh = uHeight / iPosition.y;
    // Get the 3D position of the cloud pixel
    float3 CloudPosition = iPosition * vh;
    
    // Get texture coords
    float2 TexCoord = CloudPosition.xz*uScale;
    float Density   = tex2D(uClouds, TexCoord+uTime*uWindDirection*0.25f).r;
    float3 Normal    = -(2*tex2D(uCloudsNormal, TexCoord+uTime*uWindDirection*0.25f)-1);
    Normal.zy = Normal.yz;
 
    ///------------ Volumetric effect:
    float CloudLayerHeightVolume = uCloudLayerHeightVolume*iPosition.y;
    float CloudLayerVolumetricDisplacement = uCloudLayerVolumetricDisplacement*iPosition.y;
    float3 iNewPosition = normalize(iPosition + CloudLayerVolumetricDisplacement*float3(Normal.x,0,Normal.z));
    vh = (uHeight+uHeight*(1-Density)*CloudLayerHeightVolume) / iNewPosition.y;
    CloudPosition = iNewPosition * vh;
    TexCoord = CloudPosition.xz*uScale;                               // Little offset
    Density    = tex2D(uClouds, TexCoord+uTime*uWindDirection*0.25f + float2(0.2,0.6)).r;
    ///------------
    
    float  CloudTile     = tex2D(uCloudsTile, TexCoord-uTime*uWindDirection*0.25).r;

    float3 PixelColor = uAmbientLuminosity + uSunColor*Density;
    
    // AMBIENT addition
    // PixelColor += uAmbientLuminosity;
    
    // SUN addition 
    // PixelColor  += uSunColor*saturate(dot(-normalize(Normal), normalize(uSunPosition)));
    
    // FINAL colour
    float Alpha = Density * saturate(10*saturate(-uDistanceAttenuation+iPosition.y));
    
    oColor = float4(PixelColor*(1-Density*0.35), Alpha*saturate(1-CloudTile*uDetailAttenuation));
    
#ifdef LDR
    oColor.xyz = float3(1 - exp(-uExposure * oColor.xyz));
#else // HDR
    oColor.xyz *= pow(uExposure, 0.5);
#endif
}