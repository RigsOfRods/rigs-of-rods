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
#ifndef __I_Water_H_
#define __I_Water_H_

#include "RoRPrerequisites.h"

// interface only
class IWater
{
public:
	IWater() {};
	IWater(int type, float wHeight, float *MapSizeX, float *MapSizeZ, bool useWaves);
	virtual ~IWater() {};

	virtual float getHeight() = 0;
	virtual float getHeightWaves(Ogre::Vector3 pos) = 0;
	virtual Ogre::Vector3 getVelocity(Ogre::Vector3 pos) = 0;

	virtual void setFadeColour(Ogre::ColourValue ambient) = 0;
	virtual void setHeight(float value) = 0;
	virtual void setSunPosition(Ogre::Vector3) = 0;
	virtual void setVisible(bool value) = 0;

	virtual bool allowUnderWater() = 0;
	virtual void framestep(float dt) = 0;
	virtual void moveTo(Ogre::Camera *cam, float centerheight) = 0;
	virtual void prepareShutdown() = 0;
	virtual void showWave(Ogre::Vector3 refpos) = 0;
	virtual void update() = 0;
	virtual void updateReflectionPlane(float h) = 0;
};

#endif // __I_Water_H_
