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
#include "BasicController.h"

#include "Actor.h"
#include "AppContext.h"
#include "CameraManager.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "HydraxWater.h"
#include "Terrain.h"
#include "TerrainGeometryManager.h"

#include <fmt/format.h>

using namespace Ogre;
using namespace RoR;

SkyXManager::SkyXManager(Ogre::String configFile)
{
    mBasicController = new SkyX::BasicController(/* deleteBySkyX: */false);
    mSkyX = new SkyX::SkyX(App::GetGfxScene()->GetSceneManager(), mBasicController);

    InitSkyLight();

    mCfgFileManager = new SkyX::CfgFileManager(mSkyX, mBasicController, App::GetCameraManager()->GetCamera());
    mCfgFileManager->load(configFile);

    mSkyX->create();

    RoR::App::GetAppContext()->GetOgreRoot()->addFrameListener(mSkyX);
    RoR::App::GetAppContext()->GetRenderWindow()->addListener(mSkyX);

    // Needed for precipitation (ported from Caelum) to know which viewports to create compositor instances for.
    mSkyX->attachViewport(RoR::App::GetAppContext()->GetViewport());
}

SkyXManager::~SkyXManager()
{
    // Needed for precipitation (ported from Caelum) to know which viewports to create compositor instances for.
    mSkyX->detachViewport(RoR::App::GetAppContext()->GetViewport());

    RoR::App::GetAppContext()->GetRenderWindow()->removeListener(mSkyX);
    RoR::App::GetAppContext()->GetOgreRoot()->removeFrameListener(mSkyX);

    DestroySkyLight();

    mSkyX->remove();
    delete mSkyX;
    mSkyX = nullptr;

    delete mBasicController;
    mBasicController = nullptr;
}

Light *SkyXManager::getMainLight()
{
    return mSunLight;
}

bool SkyXManager::update(float dt)
{
    UpdateSkyLight();
    DetectPlayerMovement(dt);
    mSkyX->update(dt);
    return true;
}


bool SkyXManager::UpdateSkyLight()
{
    Ogre::Vector3 lightDir = -mBasicController->getSunDirection();
    const Ogre::Vector3 camPos = App::GetCameraManager()->GetCameraNode()->_getDerivedPosition();
    Ogre::Vector3 sunPos = camPos - lightDir*mSkyX->getMeshManager()->getSkydomeRadius(App::GetCameraManager()->GetCamera());
    Ogre::Vector3 moonPos = camPos - (-lightDir)*mSkyX->getMeshManager()->getSkydomeRadius(App::GetCameraManager()->GetCamera());

    // Calculate current color gradients point
    const float point = -lightDir.y;
    const Ogre::Vector3 sunCol = mSunGradient.getColor(point+0.1f);
    const Ogre::Vector3 waterCol = mWaterGradient.getColor(point);
    const Ogre::Vector3 ambientColDay = (point > -0.3f) ? mDayAmbientGradient.getColor(point+0.3f) : Ogre::Vector3::ZERO;
    const Ogre::Vector3 ambientColNight = (point < 0) ? mNightAmbientGradient.getColor(-point) : Ogre::Vector3::ZERO;
    const Ogre::Vector3 ambientCol = ambientColDay + ambientColNight;

    if (App::GetGameContext()->GetTerrain()->getWater())
    {
        App::GetGameContext()->GetTerrain()->getGfxWater()->WaterSetSunPosition(sunPos*0.1);
        if (App::GetGameContext()->GetTerrain()->getHydraxManager ()) 
        {
            App::GetGameContext()->GetTerrain()->getHydraxManager ()->GetHydrax ()->setWaterColor (waterCol);
            App::GetGameContext()->GetTerrain()->getHydraxManager ()->GetHydrax ()->setSunColor (sunCol);
        }
    }

    // Update sun light position and color
    const float SUN_Y_THRESHOLD = 0.1f;
    mSunLightNode->setDirection(lightDir, Ogre::Node::TS_WORLD);
    mSunLight->setDiffuseColour(Ogre::ColourValue(sunCol.x, sunCol.y, sunCol.z));
    mSunLight->setSpecularColour(Ogre::ColourValue(sunCol.x, sunCol.y, sunCol.z));
    mSunLight->setVisible(lightDir.y < SUN_Y_THRESHOLD); // Only show sun light when sun is above horizon

    // Update moon light position and color
    mMoonLightNode->setDirection(-lightDir, Ogre::Node::TS_WORLD);
    mMoonLight->setVisible(lightDir.y > SUN_Y_THRESHOLD); // Only show moon light when sun is below horizon

    // Update ambient light color
    App::GetGfxScene()->GetSceneManager()->setAmbientLight(Ogre::ColourValue(ambientCol.x, ambientCol.y, ambientCol.z));
    
    if (round (mBasicController->getTime ().x) != mLastHour)
    {
        TerrainGeometryManager* gm = App::GetGameContext()->GetTerrain()->getGeometryManager ();
        if (gm)
            gm->updateLightMap ();

        mLastHour = round (mBasicController->getTime ().x);
    }

    return true;
}

void SkyXManager::InitSkyLight()
{
    // Water
    mWaterGradient = SkyX::ColorGradient();
    mWaterGradient.addCFrame(SkyX::ColorGradient::ColorFrame(Ogre::Vector3(0.058209,0.535822,0.779105)*0.4, 1));
    mWaterGradient.addCFrame(SkyX::ColorGradient::ColorFrame(Ogre::Vector3(0.058209,0.535822,0.729105)*0.3, 0.8));
    mWaterGradient.addCFrame(SkyX::ColorGradient::ColorFrame(Ogre::Vector3(0.058209,0.535822,0.679105)*0.25, 0.6));
    mWaterGradient.addCFrame(SkyX::ColorGradient::ColorFrame(Ogre::Vector3(0.058209,0.535822,0.679105)*0.2, 0.5));
    mWaterGradient.addCFrame(SkyX::ColorGradient::ColorFrame(Ogre::Vector3(0.058209,0.535822,0.679105)*0.1, 0.45));
    mWaterGradient.addCFrame(SkyX::ColorGradient::ColorFrame(Ogre::Vector3(0.058209,0.535822,0.679105)*0.025, 0));
    // Sun
    const float SUN_FACTOR = 1.1f;
    mSunGradient = SkyX::ColorGradient();
    mSunGradient.addCFrame(SkyX::ColorGradient::ColorFrame(SUN_FACTOR * Ogre::Vector3(0.8,0.79,0.77)*1.5, 1.0f));
    mSunGradient.addCFrame(SkyX::ColorGradient::ColorFrame(SUN_FACTOR * Ogre::Vector3(0.8,0.79,0.77)*1.4, 0.75f));
    mSunGradient.addCFrame(SkyX::ColorGradient::ColorFrame(SUN_FACTOR * Ogre::Vector3(0.8,0.79,0.75)*1.3, 0.5625f));
    mSunGradient.addCFrame(SkyX::ColorGradient::ColorFrame(SUN_FACTOR * Ogre::Vector3(0.6,0.58,0.4)*1.5, 0.4f));
    mSunGradient.addCFrame(SkyX::ColorGradient::ColorFrame(SUN_FACTOR * Ogre::Vector3(0.5,0.45,0.3)*1.2, 0.15f));
    mSunGradient.addCFrame(SkyX::ColorGradient::ColorFrame(SUN_FACTOR * Ogre::Vector3(0.5,0.3,0.2)*1.1, 0.05f));
    mSunGradient.addCFrame(SkyX::ColorGradient::ColorFrame(SUN_FACTOR * Ogre::Vector3(0.0,0.0,0.0), 0.0f));
    // Ambient - day
    const float DAY_FACTOR = 0.6f;
    mDayAmbientGradient = SkyX::ColorGradient();
    mDayAmbientGradient.addCFrame(SkyX::ColorGradient::ColorFrame(DAY_FACTOR * Ogre::Vector3(1,1,1)*0.3, 1.0f));
    mDayAmbientGradient.addCFrame(SkyX::ColorGradient::ColorFrame(DAY_FACTOR * Ogre::Vector3(1,1,1)*0.35, 0.6f));
    mDayAmbientGradient.addCFrame(SkyX::ColorGradient::ColorFrame(DAY_FACTOR * Ogre::Vector3(1,1,1)*0.37, 0.5f));
    mDayAmbientGradient.addCFrame(SkyX::ColorGradient::ColorFrame(DAY_FACTOR * Ogre::Vector3(1,1,1)*0.4, 0.25f));
    mDayAmbientGradient.addCFrame(SkyX::ColorGradient::ColorFrame(DAY_FACTOR * Ogre::Vector3(1,1,1)*0.2, 0.05f));
    mDayAmbientGradient.addCFrame(SkyX::ColorGradient::ColorFrame(DAY_FACTOR * Ogre::Vector3(1,1,1)*0.05, 0.0f));
    // Ambient - night
    const float NIGHT_FACTOR = 0.5f;
    mNightAmbientGradient = SkyX::ColorGradient();
    mNightAmbientGradient.addCFrame(SkyX::ColorGradient::ColorFrame(NIGHT_FACTOR * Ogre::Vector3(0.30, 0.35, 0.48) * 1.5, 1.0f));
    mNightAmbientGradient.addCFrame(SkyX::ColorGradient::ColorFrame(NIGHT_FACTOR * Ogre::Vector3(0.27, 0.3, 0.45) * 1.4, 0.35f));
    mNightAmbientGradient.addCFrame(SkyX::ColorGradient::ColorFrame(NIGHT_FACTOR * Ogre::Vector3(0.23, 0.25, 0.35) * 1.3, 0.25f));
    mNightAmbientGradient.addCFrame(SkyX::ColorGradient::ColorFrame(NIGHT_FACTOR * Ogre::Vector3(0.2, 0.2, 0.22) * 1.2, 0.05f));
    mNightAmbientGradient.addCFrame(SkyX::ColorGradient::ColorFrame(NIGHT_FACTOR * Ogre::Vector3(0.0, 0.0, 0.0), 0.f));

    // Sun
    mSunLight = App::GetGfxScene()->GetSceneManager()->createLight("SkyX/SunLight");
    mSunLight->setType(Light::LT_DIRECTIONAL);
    mSunLight->setDiffuseColour(1, 1, 1);
    mSunLight->setSpecularColour(1, 1, 1);
    mSunLight->setCastShadows(true);

    mSunLightNode = mSkyX->getSkyXGroupingNode()->createChildSceneNode("SkyX/SunLightNode");
    mSunLightNode->attachObject(mSunLight);

    // Moon
    mMoonLight = App::GetGfxScene()->GetSceneManager()->createLight("SkyX/MoonLight");
    mMoonLight->setType(Light::LT_DIRECTIONAL);
    mMoonLight->setDiffuseColour(0.2, 0.2, 0.25);
    mMoonLight->setSpecularColour(0.4, 0.4, 0.45);
    mMoonLight->setCastShadows(true);

    mMoonLightNode = mSkyX->getSkyXGroupingNode()->createChildSceneNode("SkyX/MoonLightNode");
    mMoonLightNode->attachObject(mMoonLight);
}

void SkyXManager::DestroySkyLight()
{
    App::GetGfxScene()->GetSceneManager()->destroyLight(mSunLight);
    mSunLight = nullptr;

    App::GetGfxScene()->GetSceneManager()->destroyLight(mMoonLight);
    mMoonLight = nullptr;
}

// SkyX stores time data as Vector3 :/
//  x = time-of-day in [0, 24]h range
//  y = sunrise hour in [0, 24]h range
//  z = sunset hour in [0, 24] range

void SkyXManager::setTimeOfDay24Hour(float timeOfDay24Hour)
{
    Ogre::Vector3 skyxTime = mBasicController->getTime();
    skyxTime.x = timeOfDay24Hour;
    mBasicController->setTime(skyxTime);
}

float SkyXManager::getTimeOfDay24Hour()
{
    return mBasicController->getTime().x;
}

void SkyXManager::setLatitudeDeg(float latitudeDeg)
{
    mBasicController->setLatitudeDeg(latitudeDeg);
}

float SkyXManager::getLatitudeDeg()
{
    return mBasicController->getLatitudeDeg();
}

void SkyXManager::setDayOfYear(int dayOfYear)
{
    mBasicController->setDayOfYear(static_cast<Ogre::Real>(dayOfYear));
}

int SkyXManager::getDayOfYear()
{
    return static_cast<int>(mBasicController->getDayOfYear());
}

std::string SkyXManager::getPrettyTimeHMS()
{
    const float timeOfDay = mBasicController->getTime().x;
    return fmt::format("{}:{:02}:{:02}",
        static_cast<int>(timeOfDay),
        static_cast<int>((timeOfDay - static_cast<int>(timeOfDay)) * 60),
        static_cast<int>(static_cast<int>((timeOfDay * 3600) - (static_cast<int>(timeOfDay) * 3600)) % 60)
    );
}


void SkyXManager::DetectPlayerMovement(float dt)
{
    // We only want precipitation to react to player movement, not camera orbiting/zooming.
    // ------------------------------------------------------------------------------------

    const ActorPtr currentPlayerActor = App::GetGameContext()->GetPlayerActor();
    if (currentPlayerActor == mLastPlayerActor
        && App::GetCameraManager()->GetCurrentBehavior() == mLastCameraBehavior
        && mLastCameraBehavior != CameraManager::CAMERA_BEHAVIOR_FREE
        && mLastCameraBehavior != CameraManager::CAMERA_BEHAVIOR_FIXED)
    {
        const Ogre::Vector3 playerPos = (currentPlayerActor) 
            ? currentPlayerActor->getPosition()
            : App::GetGameContext()->GetCharacterFactory()->GetLocalCharacter()->getPosition();
        
        mSkyX->getPrecipitationController()->setManualCameraSpeed(playerPos - mLastPlayerPosition);
        mLastPlayerPosition = playerPos;
    }
    else
    {
        mSkyX->getPrecipitationController()->setManualCameraSpeed(Ogre::Vector3::ZERO);
    }

    // Update last known player and camera state for next time.
    mLastPlayerActor = currentPlayerActor;
    mLastCameraBehavior = App::GetCameraManager()->GetCurrentBehavior();
}
