/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer
Copyright 2013-2016 Petr Ohlidal

For more information, see http://www.rigsofrods.org/
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
#ifndef __HydraxWater_H_
#define __HydraxWater_H_

#include "Hydrax.h"
#include "IWater.h"
#include "ProjectedGrid.h"

#include <Ogre.h>

class HydraxWater : public IWater
{
public:

	HydraxWater(const Ogre::ConfigFile &mTerrainConfig, Ogre::String configFile = "HydraxDefault.hdx");
	~HydraxWater();

	float getHeight();
	float getHeightWaves(Ogre::Vector3 pos);
	Ogre::Vector3 getVelocity(Ogre::Vector3 pos);

	void setCamera(Ogre::Camera *cam);
	void setFadeColour(Ogre::ColourValue ambient);
	void setHeight(float value);
	void setSunPosition(Ogre::Vector3);
	void setVisible(bool value);

	bool isUnderWater(Ogre::Vector3 pos);
	bool allowUnderWater();
	void framestep(float dt);
	void moveTo(float centerheight);
	void prepareShutdown();
	void showWave(Ogre::Vector3 refpos);
	void update();
	void updateReflectionPlane(float h);

	Hydrax::Hydrax* GetHydrax() { return mHydrax; }

protected:

	void InitHydrax();
	Hydrax::Hydrax *mHydrax;
	float waveHeight;
	float waterHeight;
	Ogre::Camera * mRenderCamera;
	bool haswaves;
	Hydrax::Noise::Perlin *waternoise;
	Hydrax::Module::ProjectedGrid *mModule;
	Ogre::String CurrentConfigFile;
};

#endif // __HydraxWater_H_
