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

#include <GPUNormalMapManager.h>

#include <Hydrax.h>

namespace Hydrax
{
	GPUNormalMapManager::GPUNormalMapManager(Hydrax* h)
		: mHydrax(h)
		, mRttManager(h->getRttManager())
		, mCreated(false)
	{
		mRttManager->setBitsPerChannel(RttManager::RTT_GPU_NORMAL_MAP, RttManager::BPC_16);
		mRttManager->setNumberOfChannels(RttManager::RTT_GPU_NORMAL_MAP, RttManager::NOC_3);

		mNormalMapMaterial.setNull();
	}

	GPUNormalMapManager::~GPUNormalMapManager()
	{
		remove();
	}

	void GPUNormalMapManager::create()
	{
		mRttManager->initialize(RttManager::RTT_GPU_NORMAL_MAP);

		mHydrax->getMaterialManager()->reload(MaterialManager::MAT_WATER);

		if (mHydrax->isComponent(HYDRAX_COMPONENT_UNDERWATER))
		{
		    mHydrax->getMaterialManager()->reload(MaterialManager::MAT_UNDERWATER);
		}

		mCreated = true;
	}

	void GPUNormalMapManager::remove()
	{
		if (!mCreated)
		{
			return;
		}

		for (unsigned int k = 0; k < mTextures.size(); k++)
		{
			Ogre::TextureManager::getSingleton().remove(mTextures.at(k)->getName());
		}

		mTextures.clear();

		mRttManager->remove(RttManager::RTT_GPU_NORMAL_MAP);

		Ogre::HighLevelGpuProgramManager::getSingleton().unload(mNormalMapMaterial->getTechnique(0)->getPass(0)->getVertexProgramName());
        Ogre::HighLevelGpuProgramManager::getSingleton().unload(mNormalMapMaterial->getTechnique(0)->getPass(0)->getFragmentProgramName());
		Ogre::HighLevelGpuProgramManager::getSingleton().remove(mNormalMapMaterial->getTechnique(0)->getPass(0)->getVertexProgramName());
        Ogre::HighLevelGpuProgramManager::getSingleton().remove(mNormalMapMaterial->getTechnique(0)->getPass(0)->getFragmentProgramName());

		Ogre::MaterialManager::getSingleton().remove(mNormalMapMaterial->getName());
		mNormalMapMaterial.setNull();
		
		mCreated = false;
	}
}
