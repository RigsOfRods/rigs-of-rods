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

#ifndef _Hydrax_GodRaysManager_H_
#define _Hydrax_GodRaysManager_H_

#include "Prerequisites.h"
#include "Perlin.h"

#include "Enums.h"

#include <queue>

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax
{
	class Hydrax;

	/** Underwater god rays manager class
	    God rays
	 */
	class GodRaysManager
	{
	public:
		/** God rays material enumeration
		 */
		enum MaterialType
		{
			// Material used for god rays
			MAT_GODRAYS = 0,
			// Used for depth mat(for objects intersections)
			MAT_DEPTH   = 1
		};

		/** Constructor
		    @param h Hydrax parent pointer
		 */
		GodRaysManager(Hydrax *h);

		/** Destructor
		 */
		~GodRaysManager();

		/** Create
		    @param HC Current Hydrax components
		 */
		void create(const HydraxComponent& HC);

		/** Remove
		 */
		void remove();

		/** Call each frame
		    @param timeSinceLastFrame Time since last frame
         */
        void update(const Ogre::Real& timeSinceLastFrame);

		/** Has been create() already called?
		    @return true If yes
		 */
		inline const bool& isCreated() const
		{
			return mCreated;
		}

		/** Add god rays depth technique to an especified material
		    @param Technique Technique where depth technique will be added
			@param AutoUpdate The technique will be automatically updated when god rays parameters change
			@remarks Call it after Hydrax::create()/Hydrax::setComponents(...)

			         The technique will be automatically updated when god rays parameters change if parameter AutoUpdate == true
			         Add depth technique when a material is not an Ogre::Entity, such terrains, PLSM2 materials, etc.
					 This depth technique will be added with "HydraxGodRaysDepth" scheme in ordeto can use it in the G.R. depth RTT.
		 */
		void addDepthTechnique(Ogre::Technique *Technique, const bool& AutoUpdate = true);

		/** Set god rays simulation speed
		    @param Speed Simulation speed
		 */
		inline void SetSimulationSpeed(const Ogre::Real& Speed)
		{
			mSimulationSpeed = Speed;
		}

		/** Get god rays simulation speed
		    @return Simlation speed
		 */
		inline const Ogre::Real& getSimulationSpeed() const
		{
			return mSimulationSpeed;
		}

		/** Set the number of god rays
		    @param NumberOfRays Number of god rays
		 */
		void setNumberOfRays(const int& NumberOfRays);

		/** Get number of god rays
		    @return Number of god rays
		 */
		inline const int& getNumberOfRays() const
		{
			return mNumberOfRays;
		}

		/** Set god rays size
		    @param Size God rays size
		 */
		inline void setRaysSize(const Ogre::Real& Size)
		{
			mRaysSize = Size;
		}

		/** Get god rays size
		    @return Rays size
		 */
		inline const Ogre::Real& getRaysSize() const
		{
			return mRaysSize;
		}

		/** Get perlin noise module
		    @return Perlin noise module
		 */
		inline Noise::Perlin* getPerlin()
		{
			return mPerlin;
		}

		/** Get good rays scene node
		    @return God rays scene node
		 */
		inline Ogre::SceneNode* getSceneNode()
		{
			return mProjectorSN;
		}

		/** Set visible
		    @param Visible true = yes; false = no
		 */
		inline void setVisible(const bool& Visible)
		{
			if (mCreated)
			{
				mManualGodRays->setVisible(Visible);
			}
		}

		/** Is visible?
		    @return true if it's visible, false if not
		 */
		inline const bool isVisible() const
		{
			if (!mCreated)
			{
				return false;
			}

			return mManualGodRays->isVisible();
		}

		/** Set objects intersections enabled
		    @param Enable true for yes, false for not
		 */
		void setObjectIntersectionsEnabled(const bool& Enable);

		/** Are rays objects intersections enabled?
		    @return true if yes, false if not
		 */
		inline const bool& areObjectsIntersectionsEnabled() const
		{
			return mObjectsIntersections;
		}

		/** Get noise params
		    @return Ogre::Vector4 that stores 4 parameters:
			                x-> Noise derivation
			                y-> Position multiplier
							z-> Y normal component multiplier
							w-> Normal multiplier
		 */
		inline const Ogre::Vector4 getNoiseParameters() const
		{
			return Ogre::Vector4(mNoiseDerivation,
				                 mNoisePositionMultiplier,
                                 mNoiseYNormalMultiplier,
								 mNoiseNormalMultiplier);
		}

		/** Set noise params
		    @param Params:  x-> Noise derivation
			                y-> Position multiplier
							z-> Y normal component multiplier
							w-> Normal multiplier
		 */
		inline void setNoiseParameters(Ogre::Vector4 Params)
		{
			mNoiseDerivation         = Params.x;
			mNoisePositionMultiplier = Params.y;
			mNoiseYNormalMultiplier  = Params.z;
			mNoiseNormalMultiplier   = Params.w;
		}

	private:
		/** GodRaysManager::DepthMapListener class
		 */
		class DepthMapListener : public Ogre::RenderTargetListener
		{
		public:
			/// God rays manager pointer
			GodRaysManager* mGodRaysManager;

			/// std::string to store entity's original materials name
			std::queue<std::string> mMaterials;

			/** Funtion that is called before the Rtt will render
			    @param evt Ogre RenderTargetEvent
			    @remarks We've to override it
			 */
			void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);

			/** Funtion that is called after the Rtt will render
			    @param evt Ogre RenderTargetEvent
			    @remarks We've to override it
			 */
			void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
		};

		/** Calculate the current position of a ray
		    @param RayNumber Number of the ray range[0,NumerOfRays]
			@return Ray position range[-1,1]x[-1,1]
		 */
		Ogre::Vector2 _calculateRayPosition(const int& RayNumber);

		/** Create god rays manual object
		 */
		void _createGodRays();

		/** Create materials that we need(God rays depth too if it's needed)
		    @param HC Current Hydrax components
		 */
		void _createMaterials(const HydraxComponent& HC);

		/** Update god rays
		 */
		void _updateRays();

		/** Update materials parameters
		 */
		void _updateMaterialsParameters();

		/** Update projector
		 */
		void _updateProjector();

		/** Create depth RTT
		  */
		void _createDepthRTT();

		/** Is component in the given list?
		    @param List Components list
			@param ToCheck Component to check
		    @return true if the component is in the given list.
		 */
		bool _isComponent(const HydraxComponent &List, const HydraxComponent &ToCheck) const;

		/// Has been create() already called?
		bool mCreated;

		/// Manual object to create god rays
        Ogre::ManualObject* mManualGodRays;
		/// Camera used to project rays
		Ogre::Camera *mProjectorCamera;
		/// Projector scene node
		Ogre::SceneNode *mProjectorSN;

		/// Our Perlin noise module
		Noise::Perlin *mPerlin;

		/** Noise parameters (Used in _calculateRayPosition(...))
		 */
		/// Normal derivation value
		Ogre::Real mNoiseDerivation;
		/// PositionMultiplier value
		Ogre::Real mNoisePositionMultiplier;
		/// Y normal component multiplier
		Ogre::Real mNoiseYNormalMultiplier;
		/// Normal multiplier
		Ogre::Real mNoiseNormalMultiplier;

		/// God rays materials
		/// 0-God rays, 1-Depth
		Ogre::MaterialPtr mMaterials[2];
		/// Technique vector for addDepthTechnique(...)
		std::vector<Ogre::Technique*> mDepthTechniques;

		/// God rays simulation speed
		Ogre::Real mSimulationSpeed;
		/// Number of rays
		int mNumberOfRays;
		/// God rays size
		Ogre::Real mRaysSize;
		/// Are god rays objects intersections active?
		bool mObjectsIntersections;

		/// For rays intersection with objects we use a depth map based technique
		/// Depth RTT texture
		Ogre::TexturePtr mProjectorRTT;
		/// Depth RTT listener
		DepthMapListener mDepthMapListener;

		/// Hydrax parent pointer
		Hydrax *mHydrax;
	};
};

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif
