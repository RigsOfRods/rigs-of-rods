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

#ifndef _Hydrax_DecalsManager_H_
#define _Hydrax_DecalsManager_H_

#include "Prerequisites.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax
{
	class Hydrax;

	/** Decal class.
	 */
	class Decal
	{
	public:
		/** Constructor
		    @param h Hydrax parent pointer
			@param TextureName Texture name
			@param Id Decal Id
		 */
		Decal(Hydrax *h, const Ogre::String &TextureName, const int& Id);

		/** Destructor
		 */
        ~Decal();

		/** Register the decal int the specified pass
		    @param _Pass Pass to be registred
		 */
		void registerPass(Ogre::Pass* _Pass);

		/** Unregister from current technique
	     */
		void unregister();

		/** Get decal texture name
		    @return Decal texture name
		 */
		inline const Ogre::String& getTextureName() const
		{
			return mTextureName;
		}

		/** Get the decal Id
		    @return Decal Id
		 */
		inline const int& getId() const
		{
			return mId;
		}

		/** Get the decal projector
		    @return Projector frustum
		 */
		inline Ogre::Frustum* getProjector()
		{
			return mProjector;
		}

		/** Get the decal scene node
		    @return Decal scene node
		 */
		inline Ogre::SceneNode* getSceneNode()
		{
			return mSceneNode;
		}

		/** Get the pass the decal is in
		    @return Registered pass
			@remarks return NULL if decal isn't registered
		 */
		inline Ogre::Pass *getRegisteredPass()
		{
			return mRegisteredPass;
		}

		/** Get decal position
		    @return Decal position
		 */
		inline const Ogre::Vector2& getPosition() const
		{
			return mPosition;
		}

		/** Get decal size
		    @return Decal size
		 */
		inline const Ogre::Vector2& getSize() const
		{
			return mSize;
		}

		/** Get decal orientation
		    @return Decal orientation
		 */
		inline const Ogre::Radian& getOrientation() const
		{
			return mOrientation;
		}

		/** Get decal transparency
		    @return Decal transparency
		 */
		inline const Ogre::Real& getTransparency() const
		{
			return mTransparency;
		}

		/** Is decal visile?
		    @return true if decal is visible
		 */
		inline const bool& isVisible() const
		{
			return mVisible;
		}

		/** Set decal position
		    @param Position Decal position
		 */
		void setPosition(const Ogre::Vector2& Position);

		/** Set decal size
		    @param Size Decal size in world coordinates
		 */
		void setSize(const Ogre::Vector2& Size);

		/** Set decal orientation
		    @param Orientation Decal orientation
		 */
		void setOrientation(const Ogre::Radian& Orientation);

		/** Set decal transparency
		    @param Transparency Decal transparency in [0,1] range
			@remarks 0 = Full transparent, 1 = Full opacity
		 */
		void setTransparency(const Ogre::Real& Transparency);

		/** Set decal visibile or not
		    @param Visible true if yes, false if not
		 */
		void setVisible(const bool& Visible);

	private:
		/// Decal texture name
		Ogre::String mTextureName;
		/// Decal Id
		int mId;
		/// Decal projector
		Ogre::Frustum *mProjector;
		/// Decal scene node
		Ogre::SceneNode *mSceneNode;
		/// Registered pass
		Ogre::Pass *mRegisteredPass;

		/// Position
		Ogre::Vector2 mPosition;
		/// Size
		Ogre::Vector2 mSize;
		/// Orientation
		Ogre::Radian mOrientation;
		/// Transparency
		Ogre::Real mTransparency;
		/// Is decal visible?
		bool mVisible;

		/// Hydrax parent pointer
		Hydrax *mHydrax;
	};

	/** Decals manager class. Use it for place any kind of texture
	    over the water! Like ship trails, overwater vegetables, ...
	 */
	class DecalsManager
	{
	public:
		/** Constructor
		    @param h Hydrax parent pointer
		 */
		DecalsManager(Hydrax *h);

		/** Destructor
		 */
        ~DecalsManager();

		/** Update decal manager
		    @remarks Call each frame
		 */
		void update();

		/** Add decal
		    @param TextureName Texture name
			@return Hydrax::Decal* Use it as a usual Ogre::SceneNode(Decal::getSceneNode()) for position, rotate...etc!
		 */
		Decal* add(const Ogre::String& TextureName);

		/** Get decal
		    @param Id Decal Id
			@return Hydrax::Decal*
		 */
		Decal* get(const int& Id);

		/** Remove decal
		    @param Id Decal Id
		 */
		void remove(const int& Id);

		/** Remove all decals
		 */
		void removeAll();

		/** Register all decals
		    @remarks Use it when water material is (re)created
		 */
		void registerAll();

		/** Get decals std::vector
		    @return std::vector<Decal*> list
		 */
		inline std::vector<Decal*> getDecals()
		{
			return mDecals;
		}

		/** Get water strength (used for decals culling)
		    @return Water strength
		 */
		inline const Ogre::Real _getWaterStrength() const
		{
			return mWaterStrength;
		}

		/** Set water strength (used for decals culling)
		    @param WaterStrength Water strength
		 */
		inline void _setWaterStrength(const Ogre::Real &WaterStrength)
		{
			mWaterStrength = WaterStrength;
		}

		/** Call to force to update decals
		 */
		inline void _forceToUpdate()
		{
			mForceToUpdate = true;
		}

	private:
		/// Decals std::vector
		std::vector<Decal*> mDecals;
		/// Decal iterator
		std::vector<Decal*>::iterator DecalIt;
		/// Next Id
		int mNextId;

		/// Water strength (For decals culling)
		Ogre::Real mWaterStrength;

		/// Last camera position, orientation, underwater
		Ogre::Vector3 mLastPosition;
		Ogre::Quaternion mLastOrientation;
		bool mLastUnderwater;
		bool mForceToUpdate;

		/// Hydrax parent pointer
		Hydrax *mHydrax;
	};
};

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif
