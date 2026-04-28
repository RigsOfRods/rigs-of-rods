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

#ifndef _Hydrax_Modules_RadialGrid_H_
#define _Hydrax_Modules_RadialGrid_H_



#include "Hydrax.h"
#include "Mesh.h"
#include "Module.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax{ namespace Module
{
	/** Hydrax radial grid module
	 */
	class RadialGrid : public Module
	{
	public:
		/** Struct wich contains Hydrax simple grid module options
		 */
		struct Options
		{
			/// Number of steps (Per circle)
			int Steps;
			/// Number of circles
			int Circles;
			/// Radius (In world units)
			float Radius;
			/// Smooth
			bool Smooth;
			/// Choppy waves
			bool ChoppyWaves;
			/// Choppy waves strength
			float ChoppyStrength;
			/// Step cube size
			float StepSizeCube;
			/// Step size five
			float StepSizeFive;
			/// Step lin size
			float StepSizeLin;
			/// Water strength
			float Strength;

			/** Default constructor
			 */
			Options()
				: Steps(250)
				, Circles(250)
				, Radius(100)
				, Smooth(false)
				, ChoppyWaves(true)
				, ChoppyStrength(3.5f)
				, StepSizeCube(0.00001f)
				, StepSizeFive(0.0f)
				, StepSizeLin(0.1f)
				, Strength(32.5f)
			{
			}

			/** Constructor
			    @param _Steps Number of steps per circle
				@param _Circles Number of circles
				@param _Radius Mesh radius
			 */
			Options(const int &_Steps,
				    const int &_Circles,
					const float &_Radius)
				: Steps(_Steps)
				, Circles(_Circles)
				, Radius(_Radius)
				, Smooth(false)
				, ChoppyWaves(true)
				, ChoppyStrength(3.5f)
				, StepSizeCube(0.00001f)
				, StepSizeFive(0.0f)
				, StepSizeLin(0.1f)
				, Strength(32.5f)
			{
			}

			/** Constructor
			    @param _Steps Number of steps per circle
				@param _Circles Number of circles
				@param _Radius Mesh radius
				@param _Smooth Smooth vertex?
				@param _ChoppyWaves Choppy waves enabled? Note: Only with Materialmanager::NM_VERTEX normal mode.
				@param _ChoppyStrength Choppy waves strength Note: Only with Materialmanager::NM_VERTEX normal mode.
				@param _StepSizeCube Step cube size
				@param _StepSizeFive Step five size
				@param _StepSizeLin Step lin size
				@param _Strength Water strength
			 */
			Options(const int &_Steps,
				    const int &_Circles,
					const float &_Radius,
					const bool  &_Smooth,
					const bool  &_ChoppyWaves,
					const float &_ChoppyStrength,
					const float &_StepSizeCube,
					const float &_StepSizeFive,
					const float &_StepSizeLin,
					const float &_Strength)
				: Steps(_Steps)
				, Circles(_Circles)
				, Radius(_Radius)
				, Smooth(_Smooth)
				, ChoppyWaves(_ChoppyWaves)
				, ChoppyStrength(_ChoppyStrength)
				, StepSizeCube(_StepSizeCube)
				, StepSizeFive(_StepSizeFive)
				, StepSizeLin(_StepSizeLin)
				, Strength(_Strength)
			{
			}
		};

		/** Constructor
		    @param h Hydrax manager pointer
			@param n Hydrax noise module
			@param NormalMode Switch between MaterialManager::NM_VERTEX and Materialmanager::NM_RTT
		 */
		RadialGrid(Hydrax *h, Noise::Noise *n, const MaterialManager::NormalMode& NormalMode);

		/** Constructor
		    @param h Hydrax manager pointer
			@param n Hydrax noise module
			@param NormalMode Switch between MaterialManager::NM_VERTEX and Materialmanager::NM_RTT
			@param Options Perlin options
		 */
		RadialGrid(Hydrax *h, Noise::Noise *n, const MaterialManager::NormalMode& NormalMode, const Options &Options);

		/** Destructor
		 */
        ~RadialGrid();

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

		/** Create geometry in module(If special geometry is needed)
		    @param mMesh Mesh
			@return false if it must be create by default Mesh::_createGeometry() fnc.
		 */
		const bool _createGeometry(Mesh *mMesh) const;

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