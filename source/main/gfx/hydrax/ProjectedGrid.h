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

Based on the Projected Grid concept from Claes Johanson thesis:
http://graphics.cs.lth.se/theses/projects/projgrid/
and Ren Cheng Ogre3D implementation:
http://www.cnblogs.com/ArenAK/archive/2007/11/07/951713.html
--------------------------------------------------------------------------------
*/

#ifndef _Hydrax_Modules_ProjectedGrid_H_
#define _Hydrax_Modules_ProjectedGrid_H_



#include "Hydrax.h"
#include "Mesh.h"
#include "Module.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax{ namespace Module
{
	/** Hydrax projected grid module
	 */
	class ProjectedGrid : public Module
	{
	public:
		/** Struct wich contains Hydrax projected grid module options
		 */
		struct Options
		{
			/// Projected grid complexity (N*N)
			int Complexity;
			/// Strength
			float Strength;
			/// Elevation
			float Elevation;
			/// Smooth
			bool Smooth;
			/// Force recalculate mesh geometry each frame
			bool ForceRecalculateGeometry;
			/// Choppy waves
			bool ChoppyWaves;
			/// Choppy waves strength
			float ChoppyStrength;

			/** Default constructor
			 */
			Options()
				: Complexity(256)
				, Strength(35.0f)
				, Elevation(50.0f)
				, Smooth(false)
				, ForceRecalculateGeometry(false)
				, ChoppyWaves(true)
				, ChoppyStrength(3.75f)
			{
			}

			/** Constructor
			    @param _Complexity Projected grid complexity
			 */
			Options(const int &_Complexity)
				: Complexity(_Complexity)
				, Strength(35.0f)
				, Elevation(50.0f)
				, Smooth(false)
				, ForceRecalculateGeometry(false)
				, ChoppyWaves(true)
				, ChoppyStrength(3.75f)
			{
			}

			/** Constructor
			    @param _Complexity Projected grid complexity
				@param _Strength Perlin noise strength
				@param _Elevation Elevation
				@param _Smooth Smooth vertex?
			 */
			Options(const int   &_Complexity,
				    const float &_Strength,
					const float &_Elevation,
					const bool  &_Smooth)
				: Complexity(_Complexity)
				, Strength(_Strength)
				, Elevation(_Elevation)
				, Smooth(_Smooth)
				, ForceRecalculateGeometry(false)
				, ChoppyWaves(true)
				, ChoppyStrength(3.75f)
			{
			}

			/** Constructor
			    @param _Complexity Projected grid complexity
				@param _Strength Perlin noise strength
				@param _Elevation Elevation
				@param _Smooth Smooth vertex?
				@param _ForceRecalculateGeometry Force to recalculate the projected grid geometry each frame
				@param _ChoppyWaves Choppy waves enabled? Note: Only with Materialmanager::NM_VERTEX normal mode.
				@param _ChoppyStrength Choppy waves strength
			 */
			Options(const int   &_Complexity,
				    const float &_Strength,
					const float &_Elevation,
					const bool  &_Smooth,
					const bool  &_ForceRecalculateGeometry,
					const bool  &_ChoppyWaves,
					const float &_ChoppyStrength)
				: Complexity(_Complexity)
				, Strength(_Strength)
				, Elevation(_Elevation)
				, Smooth(_Smooth)
				, ForceRecalculateGeometry(_ForceRecalculateGeometry)
				, ChoppyWaves(_ChoppyWaves)
				, ChoppyStrength(_ChoppyStrength)
			{
			}
		};

		/** Constructor
		    @param h Hydrax manager pointer
			@param n Hydrax noise module
			@param BasePlane Noise base plane
			@param NormalMode Switch between MaterialManager::NM_VERTEX and Materialmanager::NM_RTT
		 */
		ProjectedGrid(Hydrax *h, Noise::Noise *n, const Ogre::Plane &BasePlane, const MaterialManager::NormalMode& NormalMode);

		/** Constructor
		    @param h Hydrax manager pointer
			@param n Hydrax noise module
			@param BasePlane Noise base plane
			@param NormalMode Switch between MaterialManager::NM_VERTEX and Materialmanager::NM_RTT
			@param Options Perlin options
		 */
		ProjectedGrid(Hydrax *h, Noise::Noise *n, const Ogre::Plane &BasePlane, const MaterialManager::NormalMode& NormalMode, const Options &Options);

		/** Destructor
		 */
        ~ProjectedGrid();

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

		/** Set options - recreates geometry if needed
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

		/** Render geometry
		    @param m Range
			@param _viewMat View matrix
			@param WorldPos Origin world position
			@return true if it's sucesfful
		 */
		bool _renderGeometry(const Ogre::Matrix4& m,const Ogre::Matrix4& _viewMat, const Ogre::Vector3& WorldPos);

		/** Calcule world position
		    @param uv uv
			@param m Range
			@param _viewMat View matrix
			@return The position in homogenous coordinates
		 */
		Ogre::Vector4 _calculeWorldPosition(const Ogre::Vector2 &uv, const Ogre::Matrix4& m,const Ogre::Matrix4& _viewMat);

		/** Get min/max
		    @param range Range
			@return true if it's in min/max
		 */
	    bool _getMinMax(Ogre::Matrix4 *range);

		/** Set displacement amplitude
		    @param Amplitude Amplitude to set
		 */
		void _setDisplacementAmplitude(const float &Amplitude);

		/// Vertex pointer (Mesh::POS_NORM_VERTEX or Mesh::POS_VERTEX)
		void *mVertices;

		/// Use it to store vertex positions when choppy displacement is enabled
		Mesh::POS_NORM_VERTEX* mVerticesChoppyBuffer;

		/// For corners
		Ogre::Vector4 t_corners0,t_corners1,t_corners2,t_corners3;

		/// Range matrix
		Ogre::Matrix4 mRange;

		/// Planes
	    Ogre::Plane	mBasePlane,
			        mUpperBoundPlane,
					mLowerBoundPlane;

		/// Cameras
	    Ogre::Camera *mProjectingCamera,	// The camera that does the actual projection
		             *mRenderingCamera,		// The camera whose frustum the projection is created for
					 *mTmpRndrngCamera;     // Used to allow cameras with any inherited from a node or nodes

		/// Normal and position
	    Ogre::Vector3 mNormal, mPos;

		/// Last camera position, orientation
		Ogre::Vector3 mLastPosition;
		Ogre::Quaternion mLastOrientation;
		bool mLastMinMax;

		/// Our projected grid options
		Options mOptions;

		/// Our Hydrax pointer
		Hydrax* mHydrax;
	};
}}

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif