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

#include "SkyXManager.h"

#include "Application.h"
#include "HydraxWater.h"
#include "OgreSubsystem.h"
#include "TerrainManager.h"
#include "TerrainGeometryManager.h"

using namespace Ogre;
using namespace RoR;

SkyXManager::SkyXManager(Ogre::String configFile)
{
	InitLight();

	mBasicController = new SkyX::BasicController();
	mSkyX = new SkyX::SkyX(gEnv->sceneManager, mBasicController);

	mCfgFileManager = new SkyX::CfgFileManager(mSkyX, mBasicController, gEnv->mainCamera);
	mCfgFileManager->load(configFile);

	mSkyX->create();

	RoR::App::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(mSkyX);
	RoR::App::GetOgreSubsystem()->GetRenderWindow()->addListener(mSkyX);
}

SkyXManager::~SkyXManager()
{
    RoR::App::GetOgreSubsystem()->GetRenderWindow()->removeListener(mSkyX);
    mSkyX->remove();

    mSkyX = nullptr;

    delete mBasicController;
    mBasicController = nullptr;
}

Vector3 SkyXManager::getMainLightDirection()
{
	if (mBasicController != nullptr)
		return mBasicController->getSunDirection();
	return Ogre::Vector3(0.0,0.0,0.0);
}

Light *SkyXManager::getMainLight()
{
	return mLightSun;
}

bool SkyXManager::update(float dt)
{
    if (mBasicController->getTime ().x < 12)
    {
        ColorGradientPoint = abs ((mBasicController->getTime ().x - mBasicController->getTime ().y) / mBasicController->getTime ().y);

        if (mBasicController->getTime ().x > mBasicController->getTime ().y)
            UpdateSkyLight (true);
        else
            UpdateSkyLight (false);
    }
    else
    {
        ColorGradientPoint = abs (((mBasicController->getTime ().x) - mBasicController->getTime ().z) / mBasicController->getTime ().z) * 3;

        if (mBasicController->getTime ().x < mBasicController->getTime ().z)
            UpdateSkyLight (true);
        else
            UpdateSkyLight (false);
    }

	mSkyX->update(dt);
	return true;
}


bool SkyXManager::UpdateSkyLight(bool DayTime)
{
    Ogre::Vector3 sunDir = mBasicController->getSunDirection ();
    Ogre::Vector3 sunPos = gEnv->mainCamera->getDerivedPosition () - sunDir * mSkyX->getMeshManager ()->getSkydomeRadius (gEnv->mainCamera);

    if (DayTime)
    {
        mLightSun->setVisible (true);
        mLightMoon->setVisible (false);

        Ogre::Vector3 sunCol = mSunGradient.getColor (ColorGradientPoint);
        mLightSun->setDiffuseColour (sunCol.x, sunCol.y, sunCol.z);
        mLightSun->setDirection (-sunDir);
    }
    else
    {
        mLightSun->setVisible (false);
        mLightMoon->setVisible (true);

        Ogre::Vector3 ambientCol = mAmbientGradient.getColor (ColorGradientPoint);
        mLightMoon->setDiffuseColour (ambientCol.x, ambientCol.y, ambientCol.z);
        mLightMoon->setDirection (-sunDir);
    }

	

    if (App::GetSimTerrain ()->getHydraxManager ()) 
    {
        App::GetSimTerrain ()->getHydraxManager ()->GetHydrax ()->setWaterColor (mWaterGradient.getColor (ColorGradientPoint));
        App::GetSimTerrain ()->getHydraxManager ()->GetHydrax ()->setSunPosition (sunPos*0.1);
    }

    if (App::GetSimTerrain ()->getWater ())
    {
        App::GetSimTerrain ()->getWater ()->WaterSetSunPosition (sunPos*0.1);
    }
	
    if (round (mBasicController->getTime ().x) != mLastHour)
    {
        TerrainGeometryManager* gm = App::GetSimTerrain ()->getGeometryManager ();
        if (gm)
            gm->updateLightMap ();

        mLastHour = round (mBasicController->getTime ().x);
    }

	return true;
}


bool SkyXManager::InitLight()
{
    // Water
    mWaterGradient = SkyX::ColorGradient ();
    mWaterGradient.addCFrame (SkyX::ColorGradient::ColorFrame (Ogre::Vector3 (0.058209, 0.535822, 0.779105)*0.4, 1));
    mWaterGradient.addCFrame (SkyX::ColorGradient::ColorFrame (Ogre::Vector3 (0.058209, 0.535822, 0.729105)*0.3, 0.8));
    mWaterGradient.addCFrame (SkyX::ColorGradient::ColorFrame (Ogre::Vector3 (0.058209, 0.535822, 0.679105)*0.25, 0.6));
    mWaterGradient.addCFrame (SkyX::ColorGradient::ColorFrame (Ogre::Vector3 (0.058209, 0.535822, 0.679105)*0.2, 0.5));
    mWaterGradient.addCFrame (SkyX::ColorGradient::ColorFrame (Ogre::Vector3 (0.058209, 0.535822, 0.679105)*0.1, 0));
    // Sun
    mSunGradient = SkyX::ColorGradient ();
    mSunGradient.addCFrame (SkyX::ColorGradient::ColorFrame (Ogre::Vector3 (0.8, 0.75, 0.70)*1.2, 1.0f));
    mSunGradient.addCFrame (SkyX::ColorGradient::ColorFrame (Ogre::Vector3 (0.8, 0.75, 0.70)*1.1, 0.75f));
    mSunGradient.addCFrame (SkyX::ColorGradient::ColorFrame (Ogre::Vector3 (0.8, 0.75, 0.70), 0.5f));
    mSunGradient.addCFrame (SkyX::ColorGradient::ColorFrame (Ogre::Vector3 (0.5, 0.5, 0.4), 0.1f));
    mSunGradient.addCFrame (SkyX::ColorGradient::ColorFrame (Ogre::Vector3 (1, 0.7, 0.01), 0.0f));
    // Ambient
    mAmbientGradient = SkyX::ColorGradient ();
    mAmbientGradient.addCFrame (SkyX::ColorGradient::ColorFrame (Ogre::Vector3 (0.25, 0.25, 0.25) * 1, 1.0f));
    mAmbientGradient.addCFrame (SkyX::ColorGradient::ColorFrame (Ogre::Vector3 (0.25, 0.25, 0.25) * 1, 0.6f));
    mAmbientGradient.addCFrame (SkyX::ColorGradient::ColorFrame (Ogre::Vector3 (0.25, 0.25, 0.25)*0.6, 0.5f));
    mAmbientGradient.addCFrame (SkyX::ColorGradient::ColorFrame (Ogre::Vector3 (0.25, 0.25, 0.25)*0.3, 0.45f));
    mAmbientGradient.addCFrame (SkyX::ColorGradient::ColorFrame (Ogre::Vector3 (0.25, 0.25, 0.25)*0.1, 0.0f));

	//gEnv->sceneManager->setAmbientLight(ColourValue(0.1,0.1,0.1)); //Not needed because terrn2 has ambientlight settings

	// Light
	mLightMoon = gEnv->sceneManager->createLight("SkyX_Moon");
	mLightMoon->setDiffuseColour(1, 1, 1);
    mLightMoon->setType (Ogre::Light::LT_DIRECTIONAL);
	//mLightMoon->setCastShadows(false);

	mLightSun = gEnv->sceneManager->createLight("SkyX_Sun");
	mLightSun->setType(Ogre::Light::LT_DIRECTIONAL);
    //mLightSun->setPosition (100, 100, 100);

	return true;
}

size_t SkyXManager::getMemoryUsage()
{
	//TODO
	return 0;
}

void SkyXManager::freeResources()
{
	//TODO
}
