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
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "HydraxWater.h"

#include "Actor.h"
#include "AppContext.h"
#include "CameraManager.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "SkyManager.h"
#include "Terrain.h"

#ifdef USE_CAELUM
#include <Caelum.h>
#endif // USE_CAELUM

using namespace Ogre;
using namespace RoR;

// HydraxWater
HydraxWater::HydraxWater(float water_height, Ogre::String conf_file):
    waternoise(0)
    , mHydrax(0)
    , waterHeight(water_height)
    , waveHeight(water_height)
    , CurrentConfigFile(conf_file)
{
    App::GetCameraManager()->GetCamera()->setNearClipDistance(0.1f);

    InitHydrax();
}

HydraxWater::~HydraxWater()
{
    mHydrax->remove();
    mHydrax = nullptr;
}

void HydraxWater::InitHydrax()
{
    mHydrax = new Hydrax::Hydrax(App::GetGfxScene()->GetSceneManager(), App::GetCameraManager()->GetCamera(), RoR::App::GetAppContext()->GetViewport());

    waternoise = new Hydrax::Noise::Perlin();
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

    // Choose shader language based on renderer (HLSL=0, CG=1, GLSL=2)
    if (Root::getSingleton().getRenderSystem()->getName() == "Direct3D9 Rendering Subsystem" || Root::getSingleton().getRenderSystem()->getName() == "Direct3D11 Rendering Subsystem")
    {
        mHydrax->setShaderMode(static_cast<Hydrax::MaterialManager::ShaderMode>(0));
    }
    else
    {
        mHydrax->setShaderMode(static_cast<Hydrax::MaterialManager::ShaderMode>(2));
    }

    mHydrax->create();
    mHydrax->setPosition(Ogre::Vector3(0, waterHeight, 0));
}

bool HydraxWater::IsUnderWater(Ogre::Vector3 pos)
{
    if (pos.y < CalcWavesHeight(Ogre::Vector3(pos.x, pos.y, pos.z)))
        return true;
    return false;
}

void HydraxWater::UpdateWater()
{
#ifdef USE_CAELUM
    if (RoR::App::GetGameContext()->GetTerrain()->getSkyManager() != nullptr)
    {
        SkyManager* sky = RoR::App::GetGameContext()->GetTerrain()->getSkyManager();
        Ogre::Vector3 sunPosition = App::GetCameraManager()->GetCameraNode()->_getDerivedPosition();
        sunPosition -= sky->GetCaelumSys()->getSun()->getLightDirection() * 80000;
        mHydrax->setSunPosition(sunPosition);
        mHydrax->setSunColor(Ogre::Vector3(sky->GetCaelumSys()->getSun()->getBodyColour().r, sky->GetCaelumSys()->getSun()->getBodyColour().g, sky->GetCaelumSys()->getSun()->getBodyColour().b));
    }
#endif // USE_CAELUM
}

float HydraxWater::GetStaticWaterHeight()
{
    return waterHeight;
}

void HydraxWater::SetStaticWaterHeight(float value)
{
    waterHeight = value;
}

void HydraxWater::SetWaterVisible(bool value)
{
    if (mHydrax)
        mHydrax->setVisible(value);
}

float HydraxWater::CalcWavesHeight(Vector3 pos)
{
    if (!RoR::App::gfx_water_waves->getBool())
    {
        return waterHeight;
    }
    waveHeight = mHydrax->getHeigth(pos);
    return waveHeight;
}

Vector3 HydraxWater::CalcWavesVelocity(Vector3 pos)
{
    if (!RoR::App::gfx_water_waves->getBool())
        return Vector3(0, 0, 0);

    return Vector3(0, 0, 0); //TODO
}

void HydraxWater::WaterSetSunPosition(Ogre::Vector3 pos)
{
    if (mHydrax)
        mHydrax->setSunPosition(pos);
}

void HydraxWater::FrameStepWater(float dt)
{
    if (mHydrax)
    {
        mHydrax->update(dt);
    }
    this->UpdateWater();
}

