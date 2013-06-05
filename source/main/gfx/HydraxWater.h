/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

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
// Thomas Fischer, 23 of April 2008

#ifndef __HydraxWater_H_
#define __HydraxWater_H_

// this is a workaround for fixing compilation/linking issues in conjunction with caelum
#include "Prerequisites.h"
#include "Hydrax.h"
#include "MaterialManager.h"
#include "Noise.h"
#include "Perlin.h"
#include "Real.h"
#include "ProjectedGrid.h"
#include "SimpleGrid.h"

#include "Ogre.h"
#include "Water.h"
#include "IWater.h"

class HydraxWater : public IWater
{
public:

	HydraxWater();

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
	
	int loadConfig(Ogre::String configfile);

protected:

	void registerDust(DustPool* dp);

	Hydrax::Hydrax *mHydrax;
	Hydrax::Noise::Real *waternoise;

	Ogre::Camera *mRenderCamera;
	float wheight;
	float waveStrength;

	bool visible;
};

#endif // __HydraxWater_H_
