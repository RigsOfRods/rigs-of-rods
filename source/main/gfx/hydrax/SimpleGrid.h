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

#ifndef _Hydrax_Modules_SimpleGrid_H_
#define _Hydrax_Modules_SimpleGrid_H_



#include "Hydrax.h"
#include "Mesh.h"
#include "Module.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax{ namespace Module
{
	/** Hydrax simple grid module
	 */
	class SimpleGrid : public Module
	{
	public:
		/** Struct wich contains Hydrax simple grid module options
		 */
		struct Options
		{
			/// Projected grid complexity (N*N)
			int Complexity;
			/// Size
			Size MeshSize;
			/// Strength
			float Strength;
			/// Smooth
			bool Smooth;
			/// Choppy waves
			bool ChoppyWaves;
			/// Choppy waves strength
			float ChoppyStrength;

			/** Default constructor
			 */
			Options()
				: Complexity(256)
				, MeshSize(Size(100))
				, Strength(32.5f)
				, Smooth(false)
				, ChoppyWaves(true)
				, ChoppyStrength(0.065f)
			{
			}

			/** Constructor
			    @param _Complexity Projected grid complexity
				@param _MeshSize Water mesh size
			 */
			Options(const int &_Complexity,
				    const Size &_MeshSize)
				: Complexity(_Complexity)
				, MeshSize(_MeshSize)
				, Strength(32.5f)
				, Smooth(false)
				, ChoppyWaves(true)
				, ChoppyStrength(0.065f)
			{
			}

			/** Constructor
			    @param _Complexity Projected grid complexity
				@param _MeshSize Water mesh size
				@param _Strength Perlin noise strength
				@param _Smooth Smooth vertex?
				@param _ChoppyWaves Choppy waves enabled? Note: Only with Materialmanager::NM_VERTEX normal mode.
				@param _ChoppyStrength Choppy waves strength Note: Only with Materialmanager::NM_VERTEX normal mode.
			 */
			Options(const int   &_Complexity,
				    const Size  &_MeshSize,
				    const float &_Strength,
					const bool  &_Smooth,
					const bool  &_ChoppyWaves,
					const float &_ChoppyStrength)
				: Complexity(_Complexity)
				, MeshSize(_MeshSize)
				, Strength(_Strength)
				, Smooth(_Smooth)
				, ChoppyWaves(_ChoppyWaves)
				, ChoppyStrength(_ChoppyStrength)
			{
			}
		};

		/** Constructor
		    @param h Hydrax manager pointer
			@param n Hydrax noise module
			@param NormalMode Switch between MaterialManager::NM_VERTEX and Materialmanager::NM_RTT
		 */
		SimpleGrid(Hydrax *h, Noise::Noise *n, const MaterialManager::NormalMode& NormalMode);

		/** Constructor
		    @param h Hydrax manager pointer
			@param n Hydrax noise module
			@param NormalMode Switch between MaterialManager::NM_VERTEX and Materialmanager::NM_RTT
			@param Options Perlin options
		 */
		SimpleGrid(Hydrax *h, Noise::Noise *n, const MaterialManager::NormalMode& NormalMode, const Options &Options);

		/** Destructor
		 */
        ~SimpleGrid();

		/** Create
		 */
		void create();

		/** Remove
		 */
		void remove();

		/** Call it each frame
		    @param timeSinceLastFrame Time since last frame(delta)
		 */
		void update(const Ogre::Real &timeSinceLastFrame);

		/** Set options
		    @param Options Options
		 */
		void setOptions(const Options &Options);

		/** Save config
		    @param Data String reference
		 */
		void saveCfg(Ogre::String &Data);

		/** Load config
			@return True if is the correct module config
		 */
		bool loadCfg(Ogre::ConfigFile &CfgFile);

		/** Get the current heigth at a especified world-space point
		    @param Position X/Z World position
			@return Heigth at the given position in y-World coordinates, if it's outside of the water return -1
		 */
		float getHeigth(const Ogre::Vector2 &Position);

		/** Get current options
		    @return Current options
		 */
		inline const Options& getOptions() const
		{
			return mOptions;
		}

	private:
		/** Calcule current normals
		 */
		void _calculeNormals();

		/** Perform choppy waves
		 */
		void _performChoppyWaves();

		/// Vertex pointer (Mesh::POS_NORM_VERTEX or Mesh::POS_VERTEX)
		void *mVertices;

		/// Use it to store vertex positions when choppy displacement is enabled
		Mesh::POS_NORM_VERTEX* mVerticesChoppyBuffer;

		/// Our projected grid options
		Options mOptions;

		/// Our Hydrax pointer
		Hydrax* mHydrax;
	};
}}

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif