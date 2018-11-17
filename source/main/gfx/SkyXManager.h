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

#include "RoRPrerequisites.h"

#include "gfx/skyx/SkyX.h"

class SkyXManager : public ZeroedMemoryAllocator
{
public:
	SkyXManager(Ogre::String configFile);
	~SkyXManager();

	Ogre::Vector3 getMainLightDirection();

	Ogre::Light* getMainLight();

    /** Get time
            @return Current time, where x = time in [0, 24]h range, y = sunrise hour in [0, 24]h range, z = sunset hour in [0, 24] range
         */
    Ogre::Vector3 GetTime () { return mBasicController->getTime (); };

    /** Set time
            @param t Time, where x = time in [0, 24]h range, y = sunrise hour in [0, 24]h range, z = sunset hour in [0, 24] range
         */
    void SetTime (Ogre::Vector3 time) { mBasicController->setTime (time); };

	bool update( float dt );

    bool InitLight();

	size_t getMemoryUsage();

	void freeResources();

	bool UpdateSkyLight(bool DayTime);

	SkyX::SkyX* GetSkyX() { return mSkyX; }

protected:

    Ogre::Light *mLightSun;
	Ogre::Light *mLightMoon;
	
	SkyX::SkyX* mSkyX;
	SkyX::BasicController* mBasicController;

	SkyX::ColorGradient mWaterGradient, 
		                mSunGradient, 
						mAmbientGradient;

	SkyX::CfgFileManager* mCfgFileManager;

    float ColorGradientPoint = 0;

    int mLastHour = 0;
};
