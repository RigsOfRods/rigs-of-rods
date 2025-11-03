/*
--------------------------------------------------------------------------------
This source file is part of Hydrax.
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

#ifndef _Hydrax_Noise_FFT_H_
#define _Hydrax_Noise_FFT_H_



#include "Noise.h"

#include <complex>

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax{ namespace Noise
{
	/** FFT noise module class
	 */
	class FFT : public Noise
	{
	public:
		/** Struct wich contains fft noise module options
		 */
		struct Options
		{
			/// Noise resolution (2^n)
			int Resolution;
			/// Physical resolution
			float PhysicalResolution;
			/// Noise scale
			float Scale;
			/// Wind direction
			Ogre::Vector2 WindDirection;
			/// Animation speed
			float AnimationSpeed;
			/// KwPower
			float KwPower;
			/// Noise amplitude
			float Amplitude;

			/** GPU Normal map generator parameters
			    Only if GPU normal map generation is active
		     */
			/// Representes the strength of the normals (i.e. Amplitude)
			float GPU_Strength;
			/** LOD Parameters, in order to obtain a smooth normal map we need to
                decrease the detail level when the pixel is far to the camera.
				This parameters are stored in an Ogre::Vector3:
				x -> Initial LOD value (Bigger values -> less detail)
				y -> Final LOD value
				z -> Final distance
			 */
			Ogre::Vector3 GPU_LODParameters;

			/** Default constructor
			 */
			Options()
				: Resolution(128)
				, PhysicalResolution(32.0f)
			    , Scale(0.25f)
				, WindDirection(Ogre::Vector2(4,5))
				, AnimationSpeed(1)
				, KwPower(6.0f)
				, Amplitude(1.0f)
				, GPU_Strength(2.0f)
				, GPU_LODParameters(Ogre::Vector3(0.5f, 50, 150000))
			{
			}

			/** User constructor
			    @param _Resolution FFT Resolution (2^n)
				@param _PhysicalResolution Physical resolution of the surface
			    @param _Scale Noise scale
				@param _WindDirection Wind direction
				@param _AnimationSpeed Animation speed coeficient
				@param _KwPower KwPower
				@param _Amplitude Noise amplitude
			 */
			Options(const int&           _Resolution,
				    const float&         _PhysicalResolution,
				    const float&         _Scale,
					const Ogre::Vector2& _WindDirection,
					const float&         _AnimationSpeed,
					const float&         _KwPower,
					const float&         _Amplitude)
				: Resolution(_Resolution)
				, PhysicalResolution(_PhysicalResolution)
			    , Scale(_Scale)
				, WindDirection(_WindDirection)
				, AnimationSpeed(_AnimationSpeed)
				, KwPower(_KwPower)
				, Amplitude(_Amplitude)
				, GPU_Strength(2.0f)
				, GPU_LODParameters(Ogre::Vector3(0.5f, 50, 150000))
			{
			}

			/** User constructor
			    @param _Resolution FFT Resolution (2^n)
				@param _PhysicalResolution Physical resolution of the surface
			    @param _Scale Noise scale
				@param _WindDirection Wind direction
				@param _AnimationSpeed Animation speed coeficient
				@param _KwPower KwPower
				@param _Amplitude Noise amplitude
				@param _GPU_Strength GPU_Strength
				@param _GPU_LODParameters GPU_LODParameters
			 */
			Options(const int&           _Resolution,
				    const float&         _PhysicalResolution,
				    const float&         _Scale,
					const Ogre::Vector2& _WindDirection,
					const float&         _AnimationSpeed,
					const float&         _KwPower,
					const float&         _Amplitude,
					const float &_GPU_Strength,
					const Ogre::Vector3 &_GPU_LODParameters)
				: Resolution(_Resolution)
				, PhysicalResolution(_PhysicalResolution)
			    , Scale(_Scale)
				, WindDirection(_WindDirection)
				, AnimationSpeed(_AnimationSpeed)
				, KwPower(_KwPower)
				, Amplitude(_Amplitude)
				, GPU_Strength(_GPU_Strength)
				, GPU_LODParameters(_GPU_LODParameters)
			{
			}
		};

		/** Default constructor
		 */
		FFT();

		/** Constructor
		    @param Options FFT noise options
		 */
		FFT(const Options &Options);

		/** Destructor
		 */
		~FFT();

		/** Create
		 */
		void create();

		/** Create GPUNormalMap resources
		    @param g GPUNormalMapManager pointer
			@return true if it needs to be created, false if not
		 */
		bool createGPUNormalMapResources(GPUNormalMapManager *g);

		/** Remove
		 */
		void remove();

		/** Call it each frame
		    @param timeSinceLastFrame Time since last frame(delta)
		 */
		void update(const Ogre::Real &timeSinceLastFrame);

		/** Save config
		    @param Data String reference
		 */
		void saveCfg(Ogre::String &Data);

		/** Load config
			@return True if is the correct noise config
		 */
		bool loadCfg(Ogre::ConfigFile &CfgFile);

		/** Get the especified x/y noise value
		    @param x X Coord
			@param y Y Coord
			@return Noise value
		 */
		float getValue(const float &x, const float &y);

		/** Set/Update fft noise options
		    @param Options FFT noise options
		 */
		void setOptions(const Options &Options);

		/** Get current FFT noise options
		    @return Current fft noise options
		 */
		inline const Options& getOptions() const
		{
			return mOptions;
		}

	private:
		/** Initialize noise
		 */
		void _initNoise();

		/** Calcule noise
		    @param delta Time elapsed since last frame
		 */
		void _calculeNoise(const float &delta);

		/** Execute inverse fast fourier transform
		 */
		void _executeInverseFFT();

		/** Normalize fft data
		    @param scale User defined scale
		 */
		void _normalizeFFTData(const float& scale);

		/** Get the Philipps Spectrum, used to create the amplitudes and phases
		    @param waveVector Wave vector
			@param wind Wind direction
			@param kwPower_ kwPower
			@return Philipps Spectrum
		 */
		const float _getPhillipsSpectrum(const Ogre::Vector2& waveVector, const Ogre::Vector2& wind, const float& kwPower_ = 2.0f) const;

	    /** Get a Gaussian random number with mean 0 and standard deviation 1, using Box - muller transform
		    @return Gaussian random number with mean 0 and standard deviation 1, using Box - muller transform
		 */
	    const float _getGaussianRandomFloat() const;

		/** Update gpu normal map resources
		 */
		void _updateGPUNormalMapResources();

		/// FFT resolution
		int resolution;
		/// Pointers to resolution*resolution float size arrays
    	float *re, *img;
	    /// The minimal value of the result data of the fft transformation
    	float maximalValue;

		/// the data which is referred as h0{x,t), that is, the data of the simulation at the time 0.
	    std::complex<float> *initialWaves;
	    /// the data of the simulation at time t, which is formed using the data at time 0 and the angular frequencies at time t
	    std::complex<float> *currentWaves;
	    /// the angular frequencies
	    float  *angularFrequencies;
		/// Current time
		float time;

		/// GPUNormalMapManager pointer
		GPUNormalMapManager *mGPUNormalMapManager;

		/// Perlin noise options
		Options mOptions;
	};
}}

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif
