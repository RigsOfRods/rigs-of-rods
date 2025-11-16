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

#include "Noise.h"
#include "Application.h"

namespace Hydrax{namespace Noise
{
    Noise::Noise(const Ogre::String &Name, const bool& GPUNormalMapSupported)
		: mName(Name)
	    , mCreated(false)
		, mGPUNormalMapSupported(GPUNormalMapSupported)
		, mGPUNormalMapResourcesCreated(false)
	{
	}

	Noise::~Noise()
	{
	}

	void Noise::create()
	{
		mCreated = true;
	}

	void Noise::remove()
	{
		mCreated = false;
	}

	bool Noise::createGPUNormalMapResources(GPUNormalMapManager *g)
	{
		if (mGPUNormalMapSupported)
		{
			if (mGPUNormalMapResourcesCreated)
			{
				removeGPUNormalMapResources(g);
			}

			mGPUNormalMapResourcesCreated = true;

			g->remove();

			return true;
		}

		return false;
	}

	void Noise::removeGPUNormalMapResources(GPUNormalMapManager *g)
	{
		if (mGPUNormalMapSupported && mGPUNormalMapResourcesCreated)
		{
			mGPUNormalMapResourcesCreated = false;

			g->remove();
		}
	}

	void Noise::saveCfg(Ogre::String &Data)
	{
		Data += "#Noise options\n";
		Data += "Noise="+mName+"\n\n";
	}

	bool Noise::loadCfg(Ogre::ConfigFile &CfgFile)
	{
		if (CfgFile.getSetting("Noise") == mName)
		{
		    HydraxLOG(mName + " options entry found.");
			return true;
		}

        HydraxLOG("Error (Noise::loadCfg):\t" + mName + " options entry can not be found.");
		return false;
	}
}}
