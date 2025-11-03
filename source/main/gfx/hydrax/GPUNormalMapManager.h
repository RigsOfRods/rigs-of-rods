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

#ifndef _Hydrax_GPUNormalMapManager_H_
#define _Hydrax_GPUNormalMapManager_H_

#include "Prerequisites.h"

#include "Enums.h"
#include "RttManager.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax
{
	class Hydrax;

	/** Class to manager GPU normal maps
	 */
	class GPUNormalMapManager
	{
	public:
		/** Constructor
		    @param h Hydrax main pointer
		 */
		GPUNormalMapManager(Hydrax* h);

		/** Destructor
		 */
		~GPUNormalMapManager();

		/** Create
		    @remarks mNormalMapMaterial must have been created by the noise module before calling create()
		 */
		void create();

		/** Remove
		 */
		void remove();

		/** Set active
		    @param Active true for yes, false for not
		 */
		inline void setActive(const bool& Active)
		{
			if (Active)
			{
			    mRttManager->initialize(RttManager::RTT_GPU_NORMAL_MAP);
			}
			else
			{
			    mRttManager->remove(RttManager::RTT_GPU_NORMAL_MAP);
			}
		}

		/** Has been created() already called?
		    @return true if yes, false if not
		  */
		inline const bool& isCreated() const
		{
			return mCreated;
		}

		/** Get the Hydrax parent pointer
		    @remarks Needed by noise module in order to acced to the
			         MaterialManager to create vertex/fragment programs
					 and more if needed.
		 */
		inline Hydrax* getHydrax()
		{
			return mHydrax;
		}

		/** Get the normal map material
		    @return Normal map generator material
		 */
		inline Ogre::MaterialPtr &getNormalMapMaterial()
		{
			return mNormalMapMaterial;
		}

		/** Get a texture
		    @param Index Texture index
		    @return Ogre::TexturePtr
		 */
		inline Ogre::TexturePtr &getTexture(const int& Index)
		{
			return mTextures.at(Index);
		}

		/** Create a texture
		    @param Texture Ogre::TexturePtr
		 */
		void addTexture(Ogre::TexturePtr &Texture)
		{
			mTextures.push_back(Texture);
		}

		/** Remove a texture
		    @param Index Texture index
		 */
		inline void removeTexture(const int& Index)
		{
			Ogre::TextureManager::getSingleton().remove(mTextures.at(Index)->getName());

			std::vector<Ogre::TexturePtr>::iterator TexIt = mTextures.begin() + Index;

			mTextures.erase(TexIt);
		}

	private:

		/// Hydrax main pointer
		Hydrax *mHydrax;
		/// Rtt manager pointer
		RttManager *mRttManager;

		/// Has been create() already called?
		bool mCreated;

		/** This material must be created in the noise module,
		    in order to be according with the Hydrax modulable interface
			in all cases the Fragment program must have a 'uStrength' parameter
			will represents the intensity of the normals.
		 */
		Ogre::MaterialPtr mNormalMapMaterial;

		/** The normal map material needs some textures wich will be filled by code.
		    The following std::vector will store all textures, and after they need
			to be modified in the noise module.
	     */
		std::vector<Ogre::TexturePtr> mTextures;
	};
}

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif