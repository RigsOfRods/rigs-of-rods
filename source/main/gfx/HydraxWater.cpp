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
HydraxWater::HydraxWater(Hydrax::Noise::Noise* noise, float water_height, Ogre::TerrainGroup* terrain_grp, Ogre::String conf_file):
    waternoise(noise)
    , mHydrax(0)
    , waterHeight(water_height)
    , waveHeight(water_height)
    , CurrentConfigFile(conf_file)
{
    App::GetCameraManager()->GetCamera()->setNearClipDistance(0.1f);

    InitHydrax();

    //Apply depth technique to the terrain
    TerrainGroup::TerrainIterator ti = terrain_grp->getTerrainIterator();
    while (ti.hasMoreElements())
    {
        Ogre::Terrain* t = ti.getNext()->instance;
        MaterialPtr ptr = t->getMaterial();
        mHydrax->getMaterialManager()->addDepthTechnique(ptr->createTechnique());
    }
}

HydraxWater::~HydraxWater()
{
    mHydrax->remove();
    mHydrax = nullptr;
}

void HydraxWater::InitHydrax()
{
    mHydrax = new Hydrax::Hydrax(App::GetGfxScene()->GetSceneManager(), App::GetCameraManager()->GetCamera(), RoR::App::GetAppContext()->GetViewport());

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

    // Choose shader language based on renderer
    if (Root::getSingleton().getRenderSystem()->getName() == "Direct3D9 Rendering Subsystem" || Root::getSingleton().getRenderSystem()->getName() == "Direct3D11 Rendering Subsystem")
    {
        mHydrax->setShaderMode(Hydrax::MaterialManager::SM_HLSL);
    }
    else
    {
        mHydrax->setShaderMode(Hydrax::MaterialManager::SM_GLSL);
    }

    mHydrax->create();
    mHydrax->setPosition(Ogre::Vector3(0, waterHeight, 0));
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

void HydraxWater::SetWaterVisible(bool value)
{
    if (mHydrax)
        mHydrax->setVisible(value);
}

void HydraxWater::SetWaterSunPosition(Ogre::Vector3 pos)
{
    if (mHydrax)
        mHydrax->setSunPosition(pos);
}

void HydraxWater::FrameStepWater(float dt)
{
    const float curWaterHeight = App::GetGameContext()->GetTerrain()->getWater()->GetStaticWaterHeight();
    if (waterHeight != curWaterHeight)
    {
        waterHeight = curWaterHeight;
    }
    if (mHydrax)
    {
        mHydrax->update(dt);
    }
    this->UpdateWater();
}

void HydraxWater::SetWaterColor(Ogre::ColourValue color)
{
    if (mHydrax)
        mHydrax->setWaterColor(Ogre::Vector3(color.r, color.g, color.b));
}
