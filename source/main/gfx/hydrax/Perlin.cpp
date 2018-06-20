/*
--------------------------------------------------------------------------------
This source file is part of Hydrax.
Visit ---

Copyright (C) 2008 Xavier Vergu�n Gonz�lez <xavierverguin@hotmail.com>
                                           <xavyiy@gmail.com>

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

Based on the perlin noise code from Claes Johanson thesis:
http://graphics.cs.lth.se/theses/projects/projgrid/
--------------------------------------------------------------------------------
*/

#include <Perlin.h>

#include <Hydrax.h>

#define _def_PackedNoise true

namespace Hydrax{namespace Noise
{
	Perlin::Perlin()
		: Noise("Perlin", true)
		, time(0)
		, r_noise(0)
		, magnitude(n_dec_magn * 0.085f)
		, mGPUNormalMapManager(0)
	{
	}

	Perlin::Perlin(const Options &Options)
		: Noise("Perlin", true)
		, mOptions(Options)
		, time(0)
		, r_noise(0)
		, magnitude(n_dec_magn * Options.Scale)
		, mGPUNormalMapManager(0)
	{
	}

	Perlin::~Perlin()
	{
		remove();

		HydraxLOG(getName() + " destroyed.");
	}

	void Perlin::create()
	{
		if (isCreated())
		{
			return;
		}

		Noise::create();
		_initNoise();
	}

	void Perlin::remove()
	{
		if (areGPUNormalMapResourcesCreated())
		{
			Noise::removeGPUNormalMapResources(mGPUNormalMapManager);
		}

		if (!isCreated())
		{
			return;
		}

		time = 0;

		Noise::remove();
	}

	void Perlin::setOptions(const Options &Options)
	{
		if (isCreated())
		{
			int Octaves_ = Options.Octaves;
			mOptions = Options;
			mOptions.Octaves = Octaves_;

			if (isGPUNormalMapSupported() && areGPUNormalMapResourcesCreated())
			{
				mGPUNormalMapManager->getNormalMapMaterial()->
					getTechnique(0)->getPass(0)->
					    getVertexProgramParameters()->
						    setNamedConstant("uScale", Options.Scale);

				mGPUNormalMapManager->getNormalMapMaterial()->
					getTechnique(0)->getPass(0)->
					    getFragmentProgramParameters()->
						    setNamedConstant("uStrength", Options.GPU_Strength);

				mGPUNormalMapManager->getNormalMapMaterial()->
					getTechnique(0)->getPass(0)->
					    getFragmentProgramParameters()->
						    setNamedConstant("uLODParameters", Options.GPU_LODParameters);
			}
		}
		else
		{
			mOptions = Options;
			mOptions.Octaves = (mOptions.Octaves<max_octaves) ? mOptions.Octaves : max_octaves;
		}

		magnitude = n_dec_magn * mOptions.Scale;
	}

	bool Perlin::createGPUNormalMapResources(GPUNormalMapManager *g)
	{
		if (!Noise::createGPUNormalMapResources(g))
		{
			return false;
		}

		mGPUNormalMapManager = g;

		// Create our perlin textures
		Ogre::TexturePtr mPerlinTexture0
			= Ogre::TextureManager::getSingleton().
			createManual("_Hydrax_Perlin_Noise0",
			             Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
						 Ogre::TEX_TYPE_2D,
						 np_size, np_size, 0,
						 Ogre::PF_L16,
						 Ogre::TU_DYNAMIC_WRITE_ONLY);

		Ogre::TexturePtr mPerlinTexture1
			= Ogre::TextureManager::getSingleton().
			createManual("_Hydrax_Perlin_Noise1",
			             Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
						 Ogre::TEX_TYPE_2D,
						 np_size, np_size, 0,
						 Ogre::PF_L16,
						 Ogre::TU_DYNAMIC_WRITE_ONLY);

		mGPUNormalMapManager->addTexture(mPerlinTexture0);
		mGPUNormalMapManager->addTexture(mPerlinTexture1);

		// Create our normal map generator material

		MaterialManager *mMaterialManager = g->getHydrax()->getMaterialManager();

		Ogre::String VertexProgramData, FragmentProgramData;
		Ogre::GpuProgramParametersSharedPtr VP_Parameters, FP_Parameters;
		Ogre::String EntryPoints[2]     = {"main_vp", "main_fp"};
		Ogre::String GpuProgramsData[2]; Ogre::String GpuProgramNames[2];

		// Vertex program

		switch (g->getHydrax()->getShaderMode())
		{
		    case MaterialManager::SM_HLSL: case MaterialManager::SM_CG:
			{
				VertexProgramData +=
					Ogre::String(
					"void main_vp(\n") +
					    // IN
						"float4 iPosition       : POSITION,\n" +
						// OUT
						"out float4 oPosition   : POSITION,\n" +
						"out float3 oPosition_  : TEXCOORD0,\n" +
						"out float4 oWorldUV    : TEXCOORD1,\n" +
						"out float  oScale      : TEXCOORD2,\n" +
						"out float3 oCameraPos  : TEXCOORD3,\n" +
						"out float3 oCameraToPixel : TEXCOORD4,\n" +
						// UNIFORM
						"uniform float4x4 uWorldViewProj,\n" +
						"uniform float4x4 uWorld, \n" +
						"uniform float3   uCameraPos,\n"+
						"uniform float    uScale)\n" +
					"{\n" +
					    "oPosition    = mul(uWorldViewProj, iPosition);\n" +
						"oPosition_   = iPosition.xyz;\n" +
						"float2 Scale = uScale*mul(uWorld, iPosition).xz*0.0078125;\n" +
						"oWorldUV.xy  = Scale;\n" +
						"oWorldUV.zw  = Scale*16;\n" +
						"oScale = uScale;\n" +
						"oCameraPos = uCameraPos,\n" +
						"oCameraToPixel = iPosition - uCameraPos;\n"+
					"}\n";
			}
			break;

			case MaterialManager::SM_GLSL:
			{}
			break;
		}

		// Fragment program

		switch (g->getHydrax()->getShaderMode())
		{
		    case MaterialManager::SM_HLSL: case MaterialManager::SM_CG:
			{
				FragmentProgramData +=
					Ogre::String(
				    "void main_fp(\n") +
						// IN
	                    "float3 iPosition     : TEXCOORD0,\n" +
						"float4 iWorldCoord   : TEXCOORD1,\n" +
						"float  iScale        : TEXCOORD2,\n" +
						"float3 iCameraPos    : TEXCOORD3,\n" +
						"float3 iCameraToPixel : TEXCOORD4,\n" +
					    // OUT
						"out float4 oColor    : COLOR,\n" +
						// UNIFORM
						"uniform float     uStrength,\n" +
						"uniform float3    uLODParameters,\n" +  // x: Initial derivation, y: Final derivation, z: Step
						"uniform float3    uCameraPos,\n" +
						"uniform sampler2D uNoise0 : register(s0),\n" +
	                    "uniform sampler2D uNoise1 : register(s1))\n" +
					"{\n" +
						"float Distance = length(iCameraToPixel);\n" +
						"float Attenuation = saturate(Distance/uLODParameters.z);\n" +

						"uLODParameters.x += (uLODParameters.y-uLODParameters.x)*Attenuation;\n"+
						"uLODParameters.x *= iScale;\n" +

						"float AngleAttenuation = 1/abs(normalize(iCameraToPixel).y);\n"+
						"uLODParameters.x *= AngleAttenuation;\n"+

						"float2 dx = float2(uLODParameters.x*0.0078125, 0);\n" +
						"float2 dy = float2(0, dx.x);\n" +

						"float3 p_dx, m_dx, p_dy, m_dy;\n" +

						"p_dx = float3(\n" +
						              // x+
						              "iPosition.x+uLODParameters.x,\n" +
									  // y+
									  "tex2D(uNoise0, iWorldCoord.xy+dx).x + tex2D(uNoise1, iWorldCoord.zw+dx*16).x,\n" +
									  // z
									  "iPosition.z);\n" +

					    "m_dx = float3(\n" +
						              // x-
						              "iPosition.x-uLODParameters.x,\n" +
									  // y-
									  "tex2D(uNoise0, iWorldCoord.xy-dx).x + tex2D(uNoise1, iWorldCoord.zw-dx*16).x,\n" +
									  // z
									  "iPosition.z);\n" +

                       "p_dy = float3(\n" +
						              // x
						              "iPosition.x,\n" +
									  // y+
									  "tex2D(uNoise0, iWorldCoord.xy+dy).x + tex2D(uNoise1, iWorldCoord.zw+dy*16).x,\n" +
									  // z+
									  "iPosition.z+uLODParameters.x);\n" +

					   "m_dy = float3(\n" +
						              // x
						              "iPosition.x,\n" +
									  // y-
									  "tex2D(uNoise0, iWorldCoord.xy-dy).x + tex2D(uNoise1, iWorldCoord.zw-dy*16).x,\n" +
									  // z-
									  "iPosition.z-uLODParameters.x);\n" +

		               "uStrength *= (1-Attenuation);\n" +
					   "p_dx.y *= uStrength; m_dx.y *= uStrength;\n" +
	                   "p_dy.y *= uStrength; m_dy.y *= uStrength;\n" +

					   "float3 normal = normalize(cross(p_dx-m_dx, p_dy-m_dy));\n" +

					   "oColor = float4(saturate(1-(0.5+0.5*normal)),1);\n" +
					"}\n";

			}
			break;

			case MaterialManager::SM_GLSL:
			{}
			break;
		}

		// Build our material

		Ogre::MaterialPtr &mNormalMapMaterial = mGPUNormalMapManager->getNormalMapMaterial();
		mNormalMapMaterial = Ogre::MaterialManager::getSingleton().create("_Hydrax_GPU_Normal_Map_Material", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		Ogre::Pass *Technique0_Pass0 = mNormalMapMaterial->getTechnique(0)->getPass(0);

		Technique0_Pass0->setLightingEnabled(false);
		Technique0_Pass0->setCullingMode(Ogre::CULL_NONE);
		Technique0_Pass0->setDepthWriteEnabled(true);
		Technique0_Pass0->setDepthCheckEnabled(true);

		GpuProgramsData[0] = VertexProgramData; GpuProgramsData[1] =  FragmentProgramData;
		GpuProgramNames[0] = "_Hydrax_GPU_Normal_Map_VP"; GpuProgramNames[1] = "_Hydrax_GPU_Normal_Map_FP";

		mMaterialManager->fillGpuProgramsToPass(Technique0_Pass0, GpuProgramNames, g->getHydrax()->getShaderMode(), EntryPoints, GpuProgramsData);

		VP_Parameters = Technique0_Pass0->getVertexProgramParameters();

		VP_Parameters->setNamedAutoConstant("uWorldViewProj", Ogre::GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
		VP_Parameters->setNamedAutoConstant("uWorld", Ogre::GpuProgramParameters::ACT_WORLD_MATRIX);
		VP_Parameters->setNamedAutoConstant("uCameraPos", Ogre::GpuProgramParameters::ACT_CAMERA_POSITION_OBJECT_SPACE);
		VP_Parameters->setNamedConstant("uScale", mOptions.Scale);

		FP_Parameters = Technique0_Pass0->getFragmentProgramParameters();

		FP_Parameters->setNamedConstant("uStrength", mOptions.GPU_Strength);
		FP_Parameters->setNamedConstant("uLODParameters", mOptions.GPU_LODParameters);

		Technique0_Pass0->createTextureUnitState(mGPUNormalMapManager->getTexture(0)->getName(), 0)
			->setTextureAddressingMode(Ogre::TextureUnitState::TAM_MIRROR);
		Technique0_Pass0->createTextureUnitState(mGPUNormalMapManager->getTexture(1)->getName(), 1)
			->setTextureAddressingMode(Ogre::TextureUnitState::TAM_MIRROR);

		mNormalMapMaterial->load();

		mGPUNormalMapManager->create();

		return true;
	}

	void Perlin::saveCfg(Ogre::String &Data)
	{
		Noise::saveCfg(Data);

		Data += CfgFileManager::_getCfgString("Perlin_Octaves", mOptions.Octaves);
		Data += CfgFileManager::_getCfgString("Perlin_Scale", mOptions.Scale);
		Data += CfgFileManager::_getCfgString("Perlin_Falloff", mOptions.Falloff);
		Data += CfgFileManager::_getCfgString("Perlin_Animspeed", mOptions.Animspeed);
		Data += CfgFileManager::_getCfgString("Perlin_Timemulti", mOptions.Timemulti);
		Data += CfgFileManager::_getCfgString("Perlin_GPU_Strength", mOptions.GPU_Strength);
		Data += CfgFileManager::_getCfgString("Perlin_GPU_LODParameters", mOptions.GPU_LODParameters); Data += "\n";
	}

	bool Perlin::loadCfg(Ogre::ConfigFile &CfgFile)
	{
		if (!Noise::loadCfg(CfgFile))
		{
			return false;
		}

		setOptions(
			Options(CfgFileManager::_getIntValue(CfgFile,"Perlin_Octaves"),
			        CfgFileManager::_getFloatValue(CfgFile,"Perlin_Scale"),
					CfgFileManager::_getFloatValue(CfgFile,"Perlin_Falloff"),
					CfgFileManager::_getFloatValue(CfgFile,"Perlin_Animspeed"),
					CfgFileManager::_getFloatValue(CfgFile,"Perlin_Timemulti"),
					CfgFileManager::_getFloatValue(CfgFile,"Perlin_GPU_Strength"),
					CfgFileManager::_getVector3Value(CfgFile,"Perlin_GPU_LODParameters")));

		return true;
	}

	void Perlin::update(const Ogre::Real &timeSinceLastFrame)
	{
		time += timeSinceLastFrame*mOptions.Animspeed;
		_calculeNoise();

		if (areGPUNormalMapResourcesCreated())
		{
			_updateGPUNormalMapResources();
		}
	}

	void Perlin::_updateGPUNormalMapResources()
	{
        unsigned short *Data;
		int Offset;
		Ogre::HardwarePixelBufferSharedPtr PixelBuffer;

		for (int k = 0; k < 2; k++)
		{
			Offset = np_size_sq*k;
			PixelBuffer = mGPUNormalMapManager->getTexture(k)->getBuffer();

			PixelBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD);
			const Ogre::PixelBox& PixelBox = PixelBuffer->getCurrentLock();

            Data = reinterpret_cast<unsigned short*>(PixelBox.data);

			for (int u = 0; u < np_size_sq; u++)
			{
				Data[u] = 32768+p_noise[u+Offset];//std::cout << p_noise[u+Offset] << std::endl;
			}

			PixelBuffer->unlock();
		}
	}

	float Perlin::getValue(const float &x, const float &y)
	{
		return _getHeigthDual(x, y);
	}

	void Perlin::_initNoise()
	{
		// Create noise (uniform)
		float tempnoise[n_size_sq*noise_frames], temp;

		int i, frame, v, u,
            v0, v1, v2, u0, u1, u2, f;

		for(i=0; i<(n_size_sq*noise_frames); i++)
		{
			temp = static_cast<float>(rand())/RAND_MAX;
			tempnoise[i] = 4*(temp - 0.5f);
		}

		for(frame=0; frame<noise_frames; frame++)
		{
			for(v=0; v<n_size; v++)
			{
				for(u=0; u<n_size; u++)
				{
					v0 = ((v-1)&n_size_m1)*n_size;
					v1 = v*n_size;
					v2 = ((v+1)&n_size_m1)*n_size;
					u0 = ((u-1)&n_size_m1);
					u1 = u;
					u2 = ((u+1)&n_size_m1);
					f  = frame*n_size_sq;

					temp = (1.0f/14.0f) *
					   (tempnoise[f + v0 + u0] +      tempnoise[f + v0 + u1] + tempnoise[f + v0 + u2] +
						tempnoise[f + v1 + u0] + 6.0f*tempnoise[f + v1 + u1] + tempnoise[f + v1 + u2] +
						tempnoise[f + v2 + u0] +      tempnoise[f + v2 + u1] + tempnoise[f + v2 + u2]);

					noise[frame*n_size_sq + v*n_size + u] = noise_magnitude*temp;
				}
			}
		}
	}

	void Perlin::_calculeNoise()
	{
		int i, o, v, u,
			multitable[max_octaves],
			amount[3],
			iImage;

		unsigned int image[3];

		float sum = 0.0f,
			  f_multitable[max_octaves];

		double dImage, fraction;

		// calculate the strength of each octave
		for(i=0; i<mOptions.Octaves; i++)
		{
			f_multitable[i] = powf(mOptions.Falloff,1.0f*i);
			sum += f_multitable[i];
		}

		for(i=0; i<mOptions.Octaves; i++)
		{
			f_multitable[i] /= sum;
		}

		for(i=0; i<mOptions.Octaves; i++)
		{
			multitable[i] = scale_magnitude*f_multitable[i];
		}

		double r_timemulti = 1.0;
		const float PI_3 = Ogre::Math::PI/3;

		for(o=0; o<mOptions.Octaves; o++)
		{
			fraction = modf(time*r_timemulti,&dImage);
			iImage = static_cast<int>(dImage);

			amount[0] = scale_magnitude*f_multitable[o]*(pow(sin((fraction+2)*PI_3),2)/1.5);
			amount[1] = scale_magnitude*f_multitable[o]*(pow(sin((fraction+1)*PI_3),2)/1.5);
			amount[2] = scale_magnitude*f_multitable[o]*(pow(sin((fraction  )*PI_3),2)/1.5);

			image[0] = (iImage  ) & noise_frames_m1;
			image[1] = (iImage+1) & noise_frames_m1;
			image[2] = (iImage+2) & noise_frames_m1;

			for (i=0; i<n_size_sq; i++)
			{
			    o_noise[i + n_size_sq*o] = (
				   ((amount[0] * noise[i + n_size_sq * image[0]])>>scale_decimalbits) +
				   ((amount[1] * noise[i + n_size_sq * image[1]])>>scale_decimalbits) +
				   ((amount[2] * noise[i + n_size_sq * image[2]])>>scale_decimalbits));
			}

			r_timemulti *= mOptions.Timemulti;
		}

		if(_def_PackedNoise)
		{
			int octavepack = 0;
			for(o=0; o<mOptions.Octaves; o+=n_packsize)
			{
				for(v=0; v<np_size; v++)
				{
					for(u=0; u<np_size; u++)
					{
						p_noise[v*np_size+u+octavepack*np_size_sq]  = o_noise[(o+3)*n_size_sq + (v&n_size_m1)*n_size + (u&n_size_m1)];
						p_noise[v*np_size+u+octavepack*np_size_sq] += _mapSample( u, v, 3, o);
						p_noise[v*np_size+u+octavepack*np_size_sq] += _mapSample( u, v, 2, o+1);
						p_noise[v*np_size+u+octavepack*np_size_sq] += _mapSample( u, v, 1, o+2);
					}
				}

				octavepack++;
			}
		}
	}

	int Perlin::_readTexelLinearDual(const int &u, const int &v,const int &o)
	{
		int iu, iup, iv, ivp, fu, fv,
			ut01, ut23, ut;

		iu = (u>>n_dec_bits)&np_size_m1;
		iv = ((v>>n_dec_bits)&np_size_m1)*np_size;

		iup = ((u>>n_dec_bits) + 1)&np_size_m1;
		ivp = (((v>>n_dec_bits) + 1)&np_size_m1)*np_size;

		fu = u & n_dec_magn_m1;
		fv = v & n_dec_magn_m1;

		ut01 = ((n_dec_magn-fu)*r_noise[iv + iu] + fu*r_noise[iv + iup])>>n_dec_bits;
		ut23 = ((n_dec_magn-fu)*r_noise[ivp + iu] + fu*r_noise[ivp + iup])>>n_dec_bits;
		ut = ((n_dec_magn-fv)*ut01 + fv*ut23) >> n_dec_bits;

		return ut;
	}

	float Perlin::_getHeigthDual(float u, float v)
	{
		// Pointer to the current noise source octave
		r_noise = p_noise;

		int ui = u*magnitude,
		    vi = v*magnitude,
			i,
			value = 0,
			hoct = mOptions.Octaves / n_packsize;

		for(i=0; i<hoct; i++)
		{
			value += _readTexelLinearDual(ui,vi,0);
			ui = ui << n_packsize;
			vi = vi << n_packsize;
			r_noise += np_size_sq;
		}

		return static_cast<float>(value)/noise_magnitude;
	}

	int Perlin::_mapSample(const int &u, const int &v, const int &upsamplepower, const int &octave)
	{
		int magnitude = 1<<upsamplepower,

		    pu = u >> upsamplepower,
		    pv = v >> upsamplepower,

		    fu = u & (magnitude-1),
		    fv = v & (magnitude-1),

		    fu_m = magnitude - fu,
		    fv_m = magnitude - fv,

		    o = fu_m*fv_m*o_noise[octave*n_size_sq + ((pv)  &n_size_m1)*n_size + ((pu)  &n_size_m1)] +
			    fu*  fv_m*o_noise[octave*n_size_sq + ((pv)  &n_size_m1)*n_size + ((pu+1)&n_size_m1)] +
			    fu_m*fv*  o_noise[octave*n_size_sq + ((pv+1)&n_size_m1)*n_size + ((pu)  &n_size_m1)] +
			    fu*  fv*  o_noise[octave*n_size_sq + ((pv+1)&n_size_m1)*n_size + ((pu+1)&n_size_m1)];

		return o >> (upsamplepower+upsamplepower);
	}
}}
