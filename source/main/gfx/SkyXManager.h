/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "Application.h"

#include "gfx/skyx/SkyX.h"

namespace RoR {

/// @addtogroup Gfx
/// @{

class SkyXManager
{
public:
	SkyXManager(Ogre::String configFile);
	~SkyXManager();

	Ogre::Vector3 getMainLightDirection();

	Ogre::Light* getMainLight();

	bool update( float dt );

	bool InitLight();

	size_t getMemoryUsage();

	void freeResources();

	bool UpdateSkyLight();

	SkyX::SkyX* GetSkyX() { return mSkyX; }

protected:
	Ogre::Light *mLight0 = nullptr;
	Ogre::Light *mLight1 = nullptr;
	
	SkyX::SkyX* mSkyX = nullptr;
	SkyX::BasicController* mBasicController = nullptr;

	SkyX::ColorGradient mWaterGradient, 
		                mSunGradient, 
						mAmbientGradient;

	SkyX::CfgFileManager* mCfgFileManager = nullptr;

    int mLastHour = 0;
};

/// @} // addtogroup Gfx

} // namespace RoR
