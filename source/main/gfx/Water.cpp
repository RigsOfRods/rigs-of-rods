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
#include "Water.h"

#include "Application.h"
#include "OgreSubsystem.h"
#include "ResourceBuffer.h"
#include "RoRFrameListener.h"
#include "Settings.h"
#include "TerrainManager.h"

using namespace Ogre;

//Some ugly code here..
Entity* pPlaneEnt;

Plane waterPlane;
Plane bottomPlane;
Plane reflectionPlane;
Plane refractionPlane;
SceneManager *waterSceneMgr;

class RefractionTextureListener : public RenderTargetListener, public ZeroedMemoryAllocator
{
public:

	void preRenderTargetUpdate(const RenderTargetEvent& evt)
	{

	}

	void postRenderTargetUpdate(const RenderTargetEvent& evt)
	{

	}
};

class ReflectionTextureListener : public RenderTargetListener, public ZeroedMemoryAllocator
{
public:
	void preRenderTargetUpdate(const RenderTargetEvent& evt)
	{


	}
	void postRenderTargetUpdate(const RenderTargetEvent& evt)
	{

	}
};

RefractionTextureListener mRefractionListener;
ReflectionTextureListener mReflectionListener;
//End ugly code

Water::Water(const Ogre::ConfigFile &mTerrainConfig) 
{

}

Water::~Water()
{


}

void Water::processWater(int mType)
{

}

bool Water::allowUnderWater()
{
	return false;
}

void Water::setVisible(bool value)
{

}

void Water::setFadeColour(ColourValue ambient)
{
}


void Water::moveTo(float centerheight)
{

}

void Water::showWave(Vector3 refpos)
{
}

bool Water::isCameraUnderWater()
{
	return false;
}

void Water::update()
{

}

void Water::prepareShutdown()
{

}

float Water::getHeight()
{
	return wHeight;
};

void Water::setHeight(float value)
{
	wHeight = value;
	ForceUpdate = true;
}

float Water::getHeightWaves(Vector3 pos)
{
	return 0;
}

bool Water::isUnderWater(Vector3 pos)
{
	return false;
}

Vector3 Water::getVelocity(Vector3 pos)
{
	return Ogre::Vector3(0,0,0);
}

void Water::updateReflectionPlane(float h)
{

}

void Water::setSunPosition(Vector3)
{
	// not used here!
}

void Water::setCamera(Ogre::Camera *cam)
{
	mRenderCamera = cam;
}

void Water::framestep(float dt)
{
	if (dt)
		update();
}

float Water::getWaveHeight(Vector3 pos)
{
	return 0;
}
