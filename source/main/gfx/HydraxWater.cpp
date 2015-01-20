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
#include "SkyManager.h"

#include <Caelum.h>

using namespace Ogre;

// Taken from Hydrax Demo
// ----------------------------------------------------------------------------
// Hydrax Rtt Listener class (Since Hydrax 0.5.1 version) 
// ----------------------------------------------------------------------------
class HydraxRttListener : public Hydrax::RttManager::RttListener
{
public:
	void preRenderTargetUpdate(const Hydrax::RttManager::RttType& Rtt)
	{
		// If needed in any case...
		//bool underwater = gEnv->terrainManager->getHydraxWater()->isUnderWater();

		switch (Rtt)
		{
		case Hydrax::RttManager::RTT_REFLECTION:
		{

		}
			break;

		case Hydrax::RttManager::RTT_REFRACTION:
		{
		}
			break;

		case Hydrax::RttManager::RTT_DEPTH: case Hydrax::RttManager::RTT_DEPTH_REFLECTION:
		{
	
		}
			break;
		}
	}

	void postRenderTargetUpdate(const Hydrax::RttManager::RttType& Rtt)
	{
		//bool underwater = gEnv->terrainManager->getHydraxWater()->isUnderWater();

		switch (Rtt)
		{
		case Hydrax::RttManager::RTT_REFLECTION:
		{
											
		}
			break;

		case Hydrax::RttManager::RTT_REFRACTION:
		{
		}
			break;

		case Hydrax::RttManager::RTT_DEPTH: case Hydrax::RttManager::RTT_DEPTH_REFLECTION:
		{
											

		}
			break;
		}
	}
};

// HydraxWater
HydraxWater::HydraxWater(const Ogre::ConfigFile &mTerrainConfig):
waternoise(0)
, mHydrax(0)
, waterHeight(5)
{
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

}

bool HydraxWater::CreateHydrax()
{
	mHydrax->create();
	//mHydrax->getRttManager()->addRttListener(new HydraxRttListener());
	mHydrax->setPosition(Ogre::Vector3(0, waterHeight, 0));
	/*
	if (haswaves)
		mModule->setOptions(Hydrax::Module::ProjectedGrid::Options(mModule->getOptions().Complexity, 3, mModule->getOptions().Elevation, mModule->getOptions().Smooth, mModule->getOptions().ForceRecalculateGeometry, mModule->getOptions().ChoppyWaves, mModule->getOptions().ChoppyStrength)); //Not so flat water, flat doesn't look nice with hydrax
		*/
	//mHydrax->getMesh()->getSceneNode()->setScale(Ogre::Vector3(0.7, 0.7, 0.7));
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

	mHydrax->loadCfg("HydraxDefault.hdx");


	//All of this is temporary
	// Add some waves
/*	waternoise->addWave(
		Ogre::Vector2(1.f, 0.f),
		0.3f,
		10.f);
	waternoise->addWave(
		Ogre::Vector2(0.85f, 0.15f),
		0.15f,
		8.f);
	waternoise->addWave(
		Ogre::Vector2(0.95f, 0.1f),
		0.1f,
		7.f);
		*/
	/**
	//Bascily, Hydrax's configs
	mHydrax->setComponents(
		static_cast<Hydrax::HydraxComponent>(Hydrax::HYDRAX_COMPONENT_SUN |
		Hydrax::HYDRAX_COMPONENT_FOAM |
		Hydrax::HYDRAX_COMPONENT_DEPTH |
		Hydrax::HYDRAX_COMPONENT_SMOOTH |
		Hydrax::HYDRAX_COMPONENT_CAUSTICS |
		Hydrax::HYDRAX_COMPONENT_UNDERWATER |
		Hydrax::HYDRAX_COMPONENT_UNDERWATER_REFLECTIONS |
		Hydrax::HYDRAX_COMPONENT_UNDERWATER_GODRAYS));

	mHydrax->setShaderMode(Hydrax::MaterialManager::SM_HLSL);
	mHydrax->setDepthLimit(140);
	mHydrax->setGlobalTransparency(0.1);
	mHydrax->setFullReflectionDistance(100000000.0);
	mHydrax->setCausticsPower(2);
	mHydrax->setDistLimit(800);
	mHydrax->setCausticsEnd(0.8);
	mHydrax->setCausticsScale(135);
	mHydrax->setFoamScale(0.5);
	*/

}

void HydraxWater::AddMaterial(Ogre::Terrain *terrain)
{
	try
	{
		mHydrax->getMaterialManager()->addDepthTechnique(static_cast<Ogre::MaterialPtr>(Ogre::MaterialManager::getSingleton().getByName(terrain->getMaterialName()))->createTechnique());
		HydraxLOG("Added depth technique to: " + terrain->getMaterialName());
	}
	catch (...)
	{
		HydraxLOG("Error while adding Terrain Depth Technique");
	}
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
	if (gEnv->sky->getCaelumSys()) //Caelum way of doing things
	{
		Ogre::Vector3 sunPosition = gEnv->mainCamera->getDerivedPosition();
		sunPosition -= gEnv->sky->getCaelumSys()->getSun()->getLightDirection() * 80000;
		mHydrax->setSunPosition(sunPosition);
		mHydrax->setSunColor(Ogre::Vector3(gEnv->sky->getCaelumSys()->getSun()->getBodyColour().r, gEnv->sky->getCaelumSys()->getSun()->getBodyColour().g, gEnv->sky->getCaelumSys()->getSun()->getBodyColour().b));
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
