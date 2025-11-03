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

Based on the perlin noise code from Claes Johanson thesis:
http://graphics.cs.lth.se/theses/projects/projgrid/
--------------------------------------------------------------------------------
*/

#ifndef _Hydrax_Noise_Perlin_H_
#define _Hydrax_Noise_Perlin_H_

#include "Noise.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

#define n_bits				5
#define n_size				(1<<(n_bits-1))
#define n_size_m1			(n_size - 1)
#define n_size_sq			(n_size*n_size)
#define n_size_sq_m1		(n_size_sq - 1)

#define n_packsize			4

#define np_bits				(n_bits+n_packsize-1)
#define np_size				(1<<(np_bits-1))
#define np_size_m1			(np_size-1)
#define np_size_sq			(np_size*np_size)
#define np_size_sq_m1		(np_size_sq-1)

#define n_dec_bits			12
#define n_dec_magn			4096
#define n_dec_magn_m1		4095

#define max_octaves			32

#define noise_frames		256
#define noise_frames_m1		(noise_frames-1)

#define noise_decimalbits	15
#define noise_magnitude		(1<<(noise_decimalbits-1))

#define scale_decimalbits	15
#define scale_magnitude		(1<<(scale_decimalbits-1))

namespace Hydrax{ namespace Noise
{
	/** Perlin noise module class
	 */
	class Perlin : public Noise
	{
	public:
		/** Struct wich contains Perlin noise module options
		 */
		struct Options
		{
			/// Octaves
			int Octaves;
			/// Scale
			float Scale;
			/// Falloff
			float Falloff;
			/// Animspeed
			float Animspeed;
			/// Timemulti
			float Timemulti;

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
				: Octaves(8)
				, Scale(0.085f)
				, Falloff(0.49f)
				, Animspeed(1.4f)
				, Timemulti(1.27f)
				, GPU_Strength(2.0f)
				, GPU_LODParameters(Ogre::Vector3(0.5f, 50, 150000))
			{
			}

			/** Constructor
				@param _Octaves Perlin noise octaves
				@param _Scale Noise scale
				@param _Falloff Noise fall off
				@param _Animspeed Animation speed
				@param _Timemulti Timemulti
			 */
			Options(const int   &_Octaves,
					const float &_Scale,
					const float &_Falloff,
					const float &_Animspeed,
					const float &_Timemulti)
				: Octaves(_Octaves)
				, Scale(_Scale)
				, Falloff(_Falloff)
				, Animspeed(_Animspeed)
				, Timemulti(_Timemulti)
				, GPU_Strength(2.0f)
				, GPU_LODParameters(Ogre::Vector3(0.5f, 50, 150000))
			{
			}

			/** Constructor
				@param _Octaves Perlin noise octaves
				@param _Scale Noise scale
				@param _Falloff Noise fall off
				@param _Animspeed Animation speed
				@param _Timemulti Timemulti
				@param _GPU_Strength GPU_Strength
				@param _GPU_LODParameters GPU_LODParameters
			 */
			Options(const int   &_Octaves,
					const float &_Scale,
					const float &_Falloff,
					const float &_Animspeed,
					const float &_Timemulti,
					const float &_GPU_Strength,
					const Ogre::Vector3 &_GPU_LODParameters)
				: Octaves(_Octaves)
				, Scale(_Scale)
				, Falloff(_Falloff)
				, Animspeed(_Animspeed)
				, Timemulti(_Timemulti)
				, GPU_Strength(_GPU_Strength)
				, GPU_LODParameters(_GPU_LODParameters)
			{
			}
		};

		/** Default constructor
		 */
		Perlin();

		/** Constructor
		    @param Options Perlin noise options
		 */
		Perlin(const Options &Options);

		/** Destructor
		 */
		~Perlin();

		/** Create
		 */
		void create();

		/** Remove
		 */
		void remove();

		/** Create GPUNormalMap resources
		    @param g GPUNormalMapManager pointer
			@return true if it needs to be created, false if not
		 */
		bool createGPUNormalMapResources(GPUNormalMapManager *g);

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
			@remarks range [~-0.2, ~0.2]
		 */
		float getValue(const float &x, const float &y);

		/** Set/Update perlin noise options
		    @param Options Perlin noise options
			@remarks If create() have been already called, Octaves option doesn't be updated.
		 */
		void setOptions(const Options &Options);

		/** Get current Perlin noise options
		    @return Current perlin noise options
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
		 */
		void _calculeNoise();

		/** Update gpu normal map resources
		 */
		void _updateGPUNormalMapResources();

		/** Read texel linear dual
		    @param u u
			@param v v
			@param o Octave
			@return int
		 */
	    int _readTexelLinearDual(const int &u, const int &v, const int &o);

		/** Read texel linear
		    @param u u
			@param v v
			@return Heigth
		 */
		float _getHeigthDual(float u, float v);

		/** Map sample
		    @param u u
			@param v v
			@param level Level
			@param octave Octave
			@return Map sample
		 */
		int _mapSample(const int &u, const int &v, const int &upsamplepower, const int &octave);

		/// Perlin noise variables
		int noise[n_size_sq*noise_frames];
		int o_noise[n_size_sq*max_octaves];
		int p_noise[np_size_sq*(max_octaves>>(n_packsize-1))];
		int *r_noise;
		float magnitude;

		/// Elapsed time
		double time;

		/// GPUNormalMapManager pointer
		GPUNormalMapManager *mGPUNormalMapManager;

		/// Perlin noise options
		Options mOptions;
	};
}}

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif
