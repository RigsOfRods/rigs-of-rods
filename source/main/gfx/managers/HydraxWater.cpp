///WILL BE CLEANED UP

/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer
Copyright 2013-2014 Petr Ohlidal
For more information, see http://www.rigsofrods.com/
Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.
Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/*
Based on HydraxWater.cpp/.h
*/


#include "HydraxWater.h"

#include "Application.h"
#include "OgreSubsystem.h"
#include "Settings.h"
#include "CaelumManager.h"

#include <Caelum.h>

using namespace Ogre;

// HydraxWater
HydraxWater::HydraxWater(const Ogre::ConfigFile &mTerrainConfig, Ogre::String configFile):
waternoise(0)
, mHydrax(0)
, waterHeight(5)
{
	CurrentConfigFile = configFile;
	mRenderCamera = gEnv->mainCamera;
	mHydrax = new Hydrax::Hydrax(gEnv->sceneManager, mRenderCamera, RoR::Application::GetOgreSubsystem()->GetViewport());
	waveHeight = 0;
	waterHeight = PARSEREAL(mTerrainConfig.getSetting("WaterLine", "General"));
	haswaves = BSETTING("Waves", false);
	InitComponents();
	CreateHydrax();
}

HydraxWater::~HydraxWater()
{
	//delete(waternoise);
	//waternoise = nullptr;
	//delete(mModule);
	//mModule = nullptr;
	mHydrax->remove();
	//delete(mHydrax);
	mHydrax = nullptr;

	waveHeight = 0;
	waterHeight = 0;
	haswaves = false;
	mRenderCamera = nullptr;
	CurrentConfigFile = "";
}

bool HydraxWater::CreateHydrax()
{
	mHydrax->create();
	mHydrax->setPosition(Ogre::Vector3(0, waterHeight, 0));
	
	return true;
}

void HydraxWater::InitComponents()
{
	waternoise = new Hydrax::Noise::Perlin(Hydrax::Noise::Perlin::Options(waternoise->getOptions().Octaves, 0.5, waternoise->getOptions().Falloff, waternoise->getOptions().Animspeed, waternoise->getOptions().Timemulti));
	mModule = new Hydrax::Module::ProjectedGrid(// Hydrax parent pointer
		mHydrax,
		// Noise module
		waternoise,
		// Base plane
		Ogre::Plane(Ogre::Vector3::UNIT_Y, 0),
		// Normal mode
		Hydrax::MaterialManager::NM_VERTEX,
		// Projected grid options
		Hydrax::Module::ProjectedGrid::Options());

	mHydrax->setModule(static_cast<Hydrax::Module::Module*>(mModule));

	mHydrax->loadCfg(CurrentConfigFile);
}

bool HydraxWater::isUnderWater()
{
	//To avoid wired hydrax bug
	/*if(gEnv->mainCamera->getPosition().y < waterHeight)
	return true;*/
	return mHydrax->_isCurrentFrameUnderwater();
	//return false;
}

bool HydraxWater::isUnderWater(Ogre::Vector3 pos)
{
	if (pos.y < getHeightWaves(Ogre::Vector3(pos.x, pos.y, pos.z)))
		return true;
	return false;
}

void HydraxWater::setCamera(Ogre::Camera *cam)
{
}

bool HydraxWater::allowUnderWater()
{
	return true;
}

void HydraxWater::showWave(Vector3 refpos)
{
}

void HydraxWater::update()
{
	//This has to change in the next versions when SkyX will be added.
	if (gEnv->sky) //Caelum way of doing things
	{
		Ogre::Vector3 sunPosition = gEnv->mainCamera->getDerivedPosition();
		sunPosition -= gEnv->sky->getCaelumSys()->getSun()->getLightDirection() * 80000;
		mHydrax->setSunPosition(sunPosition);
		mHydrax->setSunColor(Ogre::Vector3(gEnv->sky->getCaelumSys()->getSun()->getBodyColour().r, gEnv->sky->getCaelumSys()->getSun()->getBodyColour().g, gEnv->sky->getCaelumSys()->getSun()->getBodyColour().b));
	}
	else
	{
		mHydrax->setSunPosition(gEnv->sceneManager->getLight("MainLight")->getPosition());
	}
}

void HydraxWater::prepareShutdown()
{
}

float HydraxWater::getHeight()
{
	return waterHeight;
}

void HydraxWater::setHeight(float value)
{
	waterHeight = value;
}

void HydraxWater::setVisible(bool value)
{
	if (mHydrax)
		mHydrax->setVisible(value);
}

float HydraxWater::getHeightWaves(Vector3 pos)
{
	if (!haswaves)
	{
		return waterHeight;
	}
	waveHeight = mHydrax->getHeigth(pos);
	return waveHeight;
}

Vector3 HydraxWater::getVelocity(Vector3 pos)
{
	if (!haswaves) return Vector3(0, 0, 0);

	return Vector3(0, 0, 0); //TODO
}

void HydraxWater::updateReflectionPlane(float h)
{

}

void HydraxWater::setFadeColour(ColourValue ambient)
{
	if (mHydrax)
		mHydrax->setSunColor(Vector3(ambient.r, ambient.g, ambient.b));
}

void HydraxWater::setSunPosition(Ogre::Vector3 pos)
{
	if (mHydrax)
		mHydrax->setSunPosition(pos);
}

void HydraxWater::framestep(float dt)
{
	if (mHydrax)
		mHydrax->update(dt);
	update();
}

void HydraxWater::moveTo(float centerheight)
{

}