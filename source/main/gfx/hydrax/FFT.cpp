/*
--------------------------------------------------------------------------------
This currentWaves file is part of Hydrax.
Visit ---

Copyright (C) 2008 Xavier Verguín González <xavierverguin@hotmail.com>
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
*/

#include "FFT.h"
#include "OgrePixelFormat.h"

#include <Hydrax.h>

namespace Hydrax{namespace Noise
{
	inline float uniform_deviate()
	{
		return rand() * ( 1.0f / ( RAND_MAX + 1.0f ) );
	}

	FFT::FFT()
		: Noise("FFT", true)
		, resolution(128)
		, re(0)
		, img(0)
		, maximalValue(2)
		, initialWaves(0)
		, currentWaves(0)
		, angularFrequencies(0)
		, time(10)
		, mGPUNormalMapManager(0)
	{
	}

	FFT::FFT(const Options &Options)
		: Noise("FFT", true)
		, mOptions(Options)
		, resolution(128)
		, re(0)
		, img(0)
		, maximalValue(2)
		, initialWaves(0)
		, currentWaves(0)
		, angularFrequencies(0)
		, time(10)
		, mGPUNormalMapManager(0)
	{
	}

	FFT::~FFT()
	{
		remove();

		HydraxLOG(getName() + " destroyed.");
	}

	void FFT::create()
	{
		if (isCreated())
		{
			return;
		}

		_initNoise();

		Noise::create();
	}

	void FFT::remove()
	{
		if (areGPUNormalMapResourcesCreated())
		{
			Noise::removeGPUNormalMapResources(mGPUNormalMapManager);
		}

		if (!isCreated())
		{
			return;
		}

        if (currentWaves)
		{
			delete [] currentWaves;
		}
		if (re)
		{
			delete [] re;
		}
	    if (img)
		{
			delete [] img;
		}
		if (initialWaves)
		{
			delete [] initialWaves;
		}

	    if (angularFrequencies)
		{
			delete [] angularFrequencies;
		}

		maximalValue = 2;
		time = 10;

		Noise::remove();
	}

	void FFT::setOptions(const Options &Options)
	{
		if (isCreated())
		{
			if (mOptions.Resolution != Options.Resolution ||
				mOptions.Amplitude != Options.Amplitude ||
				mOptions.KwPower != Options.KwPower ||
				mOptions.PhysicalResolution != Options.PhysicalResolution ||
				mOptions.WindDirection != Options.WindDirection)
			{
			   remove();

			   mOptions = Options;
		       resolution = Options.Resolution;

			   create();

			   if (mGPUNormalMapManager)
			   {
				   createGPUNormalMapResources(mGPUNormalMapManager);
			   }

			   return;
			}
			else
			{
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
		}

		mOptions = Options;
		resolution = Options.Resolution;
	}

	bool FFT::createGPUNormalMapResources(GPUNormalMapManager *g)
	{
		if (!Noise::createGPUNormalMapResources(g))
		{
			return false;
		}

		mGPUNormalMapManager = g;

		// Create our FFT texture
		Ogre::TexturePtr mFFTTexture
			= Ogre::TextureManager::getSingleton().
			createManual("_Hydrax_FFT_Noise",
			             Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
						 Ogre::TEX_TYPE_2D,
						 resolution, resolution, 0,
						 Ogre::PF_L16,
						 Ogre::TU_DYNAMIC_WRITE_ONLY);

		mGPUNormalMapManager->addTexture(mFFTTexture);

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
						"uniform sampler2D uFFT : register(s0))\n" +
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
									  "tex2D(uFFT, iWorldCoord.xy+dx).x,\n" +
									  // z
									  "iPosition.z);\n" +

					    "m_dx = float3(\n" +
						              // x-
						              "iPosition.x-uLODParameters.x,\n" +
									  // y-
									  "tex2D(uFFT, iWorldCoord.xy-dx).x, \n" +
									  // z
									  "iPosition.z);\n" +

                       "p_dy = float3(\n" +
						              // x
						              "iPosition.x,\n" +
									  // y+
									  "tex2D(uFFT, iWorldCoord.xy+dy).x,\n" +
									  // z+
									  "iPosition.z+uLODParameters.x);\n" +

					   "m_dy = float3(\n" +
						              // x
						              "iPosition.x,\n" +
									  // y-
									  "tex2D(uFFT, iWorldCoord.xy-dy).x,\n" +
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
			->setTextureAddressingMode(Ogre::TextureUnitState::TAM_WRAP);

		mNormalMapMaterial->load();

		mGPUNormalMapManager->create();

		return true;
	}

	void FFT::_updateGPUNormalMapResources()
	{
		Ogre::uchar *Data;
		Ogre::HardwarePixelBufferSharedPtr PixelBuffer
			= mGPUNormalMapManager->getTexture(0)->getBuffer();

		PixelBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD);

		const Ogre::PixelBox& PixelBox = PixelBuffer->getCurrentLock();

		Data = PixelBox.data;

		for (int u = 0; u < resolution*resolution; u++)
		{
			Data[u] = (re[u]*65535);
		}

		PixelBuffer->unlock();
	}

	void FFT::saveCfg(Ogre::String &Data)
	{
		Noise::saveCfg(Data);

		Data += CfgFileManager::_getCfgString("FFT_Resolution", mOptions.Resolution);
		Data += CfgFileManager::_getCfgString("FFT_PhysycalResolution", mOptions.PhysicalResolution);
		Data += CfgFileManager::_getCfgString("FFT_Scale", mOptions.Scale);
		Data += CfgFileManager::_getCfgString("FFT_WindDirection", mOptions.WindDirection);
		Data += CfgFileManager::_getCfgString("FFT_AnimationSpeed", mOptions.AnimationSpeed);
		Data += CfgFileManager::_getCfgString("FFT_KwPower", mOptions.KwPower);
		Data += CfgFileManager::_getCfgString("FFT_Amplitude", mOptions.Amplitude); Data += "\n";
	}

	bool FFT::loadCfg(Ogre::ConfigFile &CfgFile)
	{
		if (!Noise::loadCfg(CfgFile))
		{
			return false;
		}

		setOptions(
			Options(CfgFileManager::_getIntValue(CfgFile,"FFT_Resolution"),
			        CfgFileManager::_getFloatValue(CfgFile,"FFT_PhysycalResolution"),
					CfgFileManager::_getFloatValue(CfgFile,"FFT_Scale"),
					CfgFileManager::_getVector2Value(CfgFile,"FFT_WindDirection"),
					CfgFileManager::_getFloatValue(CfgFile,"FFT_AnimationSpeed"),
					CfgFileManager::_getFloatValue(CfgFile,"FFT_KwPower"),
					CfgFileManager::_getFloatValue(CfgFile,"FFT_Amplitude")));

		return true;
	}

	void FFT::update(const Ogre::Real &timeSinceLastFrame)
	{
		_calculeNoise(timeSinceLastFrame);

		if (areGPUNormalMapResourcesCreated())
		{
			_updateGPUNormalMapResources();
		}
	}

	void FFT::_initNoise()
	{
		initialWaves = new std::complex<float>[resolution*resolution];
		currentWaves = new std::complex<float>[resolution*resolution];
		angularFrequencies = new float[resolution*resolution];

		re  = new float[resolution*resolution];
		img = new float[resolution*resolution];

		Ogre::Vector2 wave = Ogre::Vector2(0,0);

		std::complex<float>* pInitialWavesData = initialWaves;
		float* pAngularFrequenciesData = angularFrequencies;

		float u, v,
		      temp;

		for (u = 0; u < resolution; u++)
		{
			wave.x = (-0.5f * resolution + u) * (2.0f* Ogre::Math::PI / mOptions.PhysicalResolution);

			for (v = 0; v < resolution; v++)
			{
				wave.y = (-0.5f * resolution + v) * (2.0f* Ogre::Math::PI / mOptions.PhysicalResolution);

				temp = Ogre::Math::Sqrt(0.5f * _getPhillipsSpectrum(wave, mOptions.WindDirection, mOptions.KwPower));
				*pInitialWavesData++ = std::complex<float>(_getGaussianRandomFloat() * temp, _getGaussianRandomFloat() * temp);

				temp=9.81f * wave.length();
				*pAngularFrequenciesData++ = Ogre::Math::Sqrt(temp);
			}
		}

		_calculeNoise(0);
	}

	void FFT::_calculeNoise(const float &delta)
	{
		time += delta*mOptions.AnimationSpeed;

		std::complex<float>* pData = currentWaves;

		int u, v;

		float wt,
			  coswt, sinwt,
			  realVal, imagVal;

		for (u = 0; u < resolution; u++)
		{
			for (v = 0; v< resolution ; v++)
			{
				const std::complex<float>& positive_h0 = initialWaves[u * (resolution)+v];
				const std::complex<float>& negative_h0 = initialWaves[(resolution-1 - u) * (resolution) + (resolution-1- v)];

				wt = angularFrequencies[u * (resolution) + v] * time;

				coswt = Ogre::Math::Cos(wt);
				sinwt = Ogre::Math::Sin(wt);

				realVal =
					positive_h0.real() * coswt - positive_h0.imag() * sinwt + negative_h0.real() * coswt - (-negative_h0.imag()) * (-sinwt),
				imagVal =
					positive_h0.real() * sinwt + positive_h0.imag() * coswt + negative_h0.real() * (-sinwt) + (-negative_h0.imag()) * coswt;

				*pData++ = std::complex<float>(realVal, imagVal);
			}
		}

		_executeInverseFFT();
		_normalizeFFTData(0);
	}

	const float FFT::_getGaussianRandomFloat() const
	{
		float x1, x2, w, y1;

		do
		{
			x1 = 2.0f * uniform_deviate() - 1.0f;
			x2 = 2.0f * uniform_deviate() - 1.0f;

			w = x1 * x1 + x2 * x2;

		} while ( w >= 1.0f );

		w = Ogre::Math::Sqrt( (-2.0f * Ogre::Math::Log( w ) ) / w );
		y1 = x1 * w;

		return y1;
	}

	const float FFT::_getPhillipsSpectrum(const Ogre::Vector2& waveVector, const Ogre::Vector2& wind, const float& kwPower_) const
	{
		// Compute the length of the vector
		float k = waveVector.length();

		// To avoid division by 0
		if (k < 0.0000001f)
		{
			return 0;
		}
		else
		{
			float windVelocity = wind.length(),
			      l = pow(windVelocity,2.0f)/9.81f,
			      dot=waveVector.dotProduct(wind);

			return mOptions.Amplitude*
				(Ogre::Math::Exp(-1 / pow(k * l,2)) / (Ogre::Math::Pow(k,2) *
				 Ogre::Math::Pow(k,2))) * Ogre::Math::Pow(-dot/ (k * windVelocity), kwPower_);
		}
	}

	void FFT::_executeInverseFFT()
	{
		int l2n = 0, p = 1;
		while (p < resolution)
		{
			p *= 2; l2n++;
		}
		int l2m = 0; p = 1;
		while (p < resolution)
		{
			p *= 2; l2m++;
		}

		resolution = 1<<l2m;
		resolution = 1<<l2n;

		int x, y, i;

		for(x = 0; x <resolution; x++)
		{
			for(y = 0; y <resolution; y++)
			{
				re[resolution * x + y] = currentWaves[resolution * x + y].real();
				img[resolution * x + y] = currentWaves[resolution * x + y].imag();
			}
		}

		//Bit reversal of each row
		int j, k;
		for(y = 0; y < resolution; y++) //for each row
		{
			j = 0;
			for(i = 0; i < resolution - 1; i++)
			{
				re[resolution * i + y] = currentWaves[resolution * j + y].real();
				img[resolution * i + y] = currentWaves[resolution * j + y].imag();

				k = resolution / 2;
				while (k <= j)
				{
					j -= k;
					k/= 2;
				}

				j += k;
			}
		}

		//Bit reversal of each column
		float tx = 0, ty = 0;
		for(x = 0; x < resolution; x++) //for each column
		{
			j = 0;
			for(i = 0; i < resolution - 1; i++)
			{
				if(i < j)
				{
					tx = re[resolution * x + i];
					ty = img[resolution * x + i];
					re[resolution * x + i] = re[resolution * x + j];
					img[resolution * x + i] = img[resolution * x + j];
					re[resolution * x + j] = tx;
					img[resolution * x + j] = ty;
				}
				k = resolution / 2;
				while (k <= j)
				{
					j -= k;
					k/= 2;
				}
				j += k;
			}
		}

		//Calculate the FFT of the columns
		float ca, sa,
			  u1, u2,
			  t1, t2,
			  z;

		int l1, l2,
			l,  i1;

		for(x = 0; x < resolution; x++) //for each column
		{
			//This is the 1D FFT:
			ca = -1.0;
			sa = 0.0;
			l1 = 1, l2 = 1;

			for(l=0;l<l2n;l++)
			{
				l1 = l2;
				l2 *= 2;
				u1 = 1.0;
				u2 = 0.0;
				for(j = 0; j < l1; j++)
				{
					for(i = j; i < resolution; i += l2)
					{
						i1 = i + l1;
						t1 = u1 * re[resolution * x + i1] - u2 * img[resolution * x + i1];
						t2 = u1 * img[resolution * x + i1] + u2 * re[resolution * x + i1];
						re[resolution * x + i1] = re[resolution * x + i] - t1;
						img[resolution * x + i1] = img[resolution * x + i] - t2;
						re[resolution * x + i] += t1;
						img[resolution * x + i] += t2;
					}
					z =  u1 * ca - u2 * sa;
					u2 = u1 * sa + u2 * ca;
					u1 = z;
				}
				sa = Ogre::Math::Sqrt((1.0f - ca) / 2.0f);
				ca = Ogre::Math::Sqrt((1.0f+ca) / 2.0f);
			}
		}
		//Calculate the FFT of the rows
		for(y = 0; y < resolution; y++) //for each row
		{
			//This is the 1D FFT:
			ca = -1.0;
			sa = 0.0;
			l1= 1, l2 = 1;

			for(l = 0; l < l2m; l++)
			{
				l1 = l2;
				l2 *= 2;
				u1 = 1.0;
				u2 = 0.0;
				for(j = 0; j < l1; j++)
				{
					for(i = j; i < resolution; i += l2)
					{
						i1 = i + l1;
					    t1 = u1 * re[resolution * i1 + y] - u2 * img[resolution * i1 + y];
						t2 = u1 * img[resolution * i1 + y] + u2 * re[resolution* i1 + y];
						re[resolution * i1 + y] = re[resolution * i + y] - t1;
						img[resolution * i1 + y] = img[resolution * i + y] - t2;
						re[resolution * i + y] += t1;
						img[resolution * i + y] += t2;
					}
					z =  u1 * ca - u2 * sa;
					u2 = u1 * sa + u2 * ca;
					u1 = z;
				}
				sa = Ogre::Math::Sqrt((1.0f - ca) / 2.0f);
				ca = Ogre::Math::Sqrt((1.0f+ca) / 2.0f);
			}
		}

		for(x=0;x<resolution;x++)
		{
			for(y=0;y<resolution;y++)
			{
				if (((x+y) & 0x1)==1)
				{
					re[x*resolution+y]*=1;
				}
				else
				{
					re[x*resolution+y]*=-1;
				}
			}
		}
	}

	void FFT::_normalizeFFTData(const float& scale)
	{
		float scaleCoef = 0.000001f;
		int i;

		// Perform automatic detection of maximum value
		if (scale == 0.0f)
		{
			float min=re[0], max=re[0],
				  currentMax=maximalValue;;

			for(i=1;i<resolution*resolution;i++)
			{
				if (min>re[i]) min=re[i];
				if (max<re[i]) max=re[i];
			}

			min=Ogre::Math::Abs(min);
			max=Ogre::Math::Abs(max);

			currentMax = (min>max) ? min : max;

			if (currentMax>maximalValue) maximalValue=currentMax;

			scaleCoef += maximalValue;
		}
		else
		{	// User defined scale
			scaleCoef=scale;
		}

		// Scale all the value, and clamp to [0,1] range
		int x, y;
		for(x=0;x<resolution;x++)
		{
			for(y=0;y<resolution;y++)
			{
				i=x*resolution+y;
				re[i]=(re[i]+scaleCoef)/(scaleCoef*2);
			}
		}
	}

	float FFT::getValue(const float &x, const float &y)
	{
		// Scale world coords
		float xScale = x*mOptions.Scale,
		      yScale = y*mOptions.Scale;

		// Convert coords from world-space to data-space
        int xs = static_cast<int>(xScale)%resolution,
	        ys = static_cast<int>(yScale)%resolution;

		// If data-space coords are negative, transform it to positive
		if (x<0) xs += resolution-1;
		if (y<0) ys += resolution-1;

		// Determine x and y diff for linear interpolation
		int xINT = (x>0) ? static_cast<int>(xScale) : static_cast<int>(xScale-1),
		    yINT = (y>0) ? static_cast<int>(yScale) : static_cast<int>(yScale-1);

		// Calculate interpolation coeficients
		float xDIFF  = xScale-xINT,
			  yDIFF  = yScale-yINT,
			  _xDIFF = 1-xDIFF,
			  _yDIFF = 1-yDIFF;

		// To adjust the index if coords are out of range
		int xxs = (xs==resolution-1) ? -1 : xs,
			yys = (ys==resolution-1) ? -1 : ys;

		//   A      B
		//
		//
		//   C      D
		float A = re[(ys*resolution+xs)],
			  B = re[(ys*resolution+xxs+1)],
			  C = re[((yys+1)*resolution+xs)],
			  D = re[((yys+1)*resolution+xxs+1)];

		// Return the result of the linear interpolation
		return (A*_xDIFF*_yDIFF +
			    B* xDIFF*_yDIFF +
			    C*_xDIFF* yDIFF +
			    D* xDIFF* yDIFF) // Range [-0.3, 0.3]
				                 *0.6f-0.3f;
	}
}}
