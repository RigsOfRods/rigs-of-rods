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
    mGroupingSceneNode = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode("SkyX");

    // Init CaelumPort
    CaelumPort::CaelumSystem* caelumPort = new CaelumPort::CaelumSystem(
        RoR::App::GetAppContext()->GetOgreRoot(),
        App::GetGfxScene()->GetSceneManager(),
        mGroupingSceneNode
    );
    caelumPort->attachViewport(RoR::App::GetAppContext()->GetViewport());
    RoR::App::GetAppContext()->GetRenderWindow()->addListener(caelumPort);
    RoR::App::GetAppContext()->GetOgreRoot()->addFrameListener(caelumPort);

    mSkyX = new SkyX::SkyX(App::GetGfxScene()->GetSceneManager(), mGroupingSceneNode, caelumPort);

    mCfgFileManager = new SkyX::CfgFileManager(mSkyX, App::GetCameraManager()->GetCamera());
    mCfgFileManager->load(configFile);

    mSkyX->create();

    RoR::App::GetAppContext()->GetOgreRoot()->addFrameListener(mSkyX);
    RoR::App::GetAppContext()->GetRenderWindow()->addListener(mSkyX);

    // Needed for precipitation (ported from Caelum) to know which viewports to create compositor instances for.
    mSkyX->attachViewport(RoR::App::GetAppContext()->GetViewport());
}

SkyXManager::~SkyXManager()
{
    // Destroy CaelumPort
    CaelumPort::CaelumSystem* caelumPort = mSkyX->getCaelumPort();
    RoR::App::GetAppContext()->GetRenderWindow()->removeListener(caelumPort);
    RoR::App::GetAppContext()->GetOgreRoot()->removeFrameListener(caelumPort);
    delete caelumPort;

    // Needed for precipitation (ported from Caelum) to know which viewports to create compositor instances for.
    mSkyX->detachViewport(RoR::App::GetAppContext()->GetViewport());

    RoR::App::GetAppContext()->GetRenderWindow()->removeListener(mSkyX);
    RoR::App::GetAppContext()->GetOgreRoot()->removeFrameListener(mSkyX);

    mSkyX->remove();
    delete mSkyX;
    mSkyX = nullptr;

    App::GetGfxScene()->GetSceneManager()->destroySceneNode(mGroupingSceneNode);
}

Ogre::Light* SkyXManager::GetCaelumPortMainLight()
{
    CaelumPort::CaelumSystem* caelumPort = mSkyX->getCaelumPort();
    if (caelumPort && caelumPort->getSun())
    {
        return caelumPort->getSun()->getMainLight();
    }
    return nullptr;
}

bool SkyXManager::update(float dt)
{
    DetectPlayerMovement(dt);
    mSkyX->update(dt);
    DetectTerrainLightmapUpdateFromCaelumPort();
    return true;
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

void SkyXManager::NotifyCaelumPortCameraChanged(Ogre::Camera* newCamera)
{
    mSkyX->getCaelumPort()->notifyCameraChanged(newCamera);
}

void SkyXManager::DetectTerrainLightmapUpdateFromCaelumPort()
{
    if (!App::GetGameContext()->GetTerrain())
    {
        return;
    }

    Caelum::LongReal c = mSkyX->getCaelumPort()->getUniversalClock()->getJulianDay();

    if (c - mLastLightmapUpdateCaelumClock > 0.001f)
    {
        TerrainGeometryManager* gm = App::GetGameContext()->GetTerrain()->getGeometryManager();
        if (gm)
            gm->updateLightMap();
    }

    mLastLightmapUpdateCaelumClock = c;
}

std::string SkyXManager::GetCaelumPortPrettyTime()
{
    int ignore;
    int hour;
    int minute;
    Caelum::LongReal second;
    Caelum::Astronomy::getGregorianDateTimeFromJulianDay(mSkyX->getCaelumPort()->getUniversalClock()->getJulianDay()
        , ignore, ignore, ignore, hour, minute, second);

    char buf[100];
    snprintf(buf, 100, "%02d:%02d:%02d", hour, minute, static_cast<int>(second));
    return buf;
}

double SkyXManager::GetCaelumPortTime()
{
    return mSkyX->getCaelumPort()->getUniversalClock()->getJulianDay();
}

void SkyXManager::SetCaelumPortTime(double time)
{
    mSkyX->getCaelumPort()->getUniversalClock()->setJulianDay(time);
}