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

#include"Module.h"

namespace Hydrax{namespace Module
{
	Module::Module(const Ogre::String &Name,
		           Noise::Noise *n,
				   const Mesh::Options &MeshOptions,
				   const MaterialManager::NormalMode &NormalMode)
		: mName(Name)
		, mNoise(n)
		, mMeshOptions(MeshOptions)
		, mNormalMode(NormalMode)
	    , mCreated(false)
	{
	}

	Module::~Module()
	{
		delete mNoise;
	}

	void Module::create()
	{
		mNoise->create();

		mCreated = true;
	}

	void Module::remove()
	{
		mNoise->remove();

		mCreated = false;
	}

	void Module::setNoise(Noise::Noise* Noise, GPUNormalMapManager* g, const bool& DeleteOldNoise)
	{
		if (DeleteOldNoise)
		{
			delete mNoise;
		}

		mNoise = Noise;

		if (mCreated)
		{
			if (!mNoise->isCreated())
			{
				mNoise->create();
			}

			if (getNormalMode() == MaterialManager::NM_RTT)
			{
				if (!mNoise->createGPUNormalMapResources(g))
				{
					HydraxLOG(mNoise->getName() + " doesn't support GPU Normal map generation");
				}
			}
			else
			{
				mNoise->removeGPUNormalMapResources(g);
			}
		}
		else
		{
			mNoise->removeGPUNormalMapResources(g);
		}
	}

	void Module::update(const Ogre::Real &timeSinceLastFrame)
	{
		mNoise->update(timeSinceLastFrame);
	}

	void Module::saveCfg(Ogre::String &Data)
	{
		// RIGS OF RODS: nothing to do here - the module type is hardcoded in file 'HydraxWater.cpp'
	}

	bool Module::loadCfg(Ogre::ConfigFile &CfgFile)
	{
		// RIGS OF RODS: Nothing to do here - the module type is hardcoded in file 'HydraxWater.cpp'
		return true;
	}

	float Module::getHeigth(const Ogre::Vector2 &Position)
	{
		return -1;
	}
}}
