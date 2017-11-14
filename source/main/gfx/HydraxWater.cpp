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

#include "Application.h"
#include "OgreSubsystem.h"
#include "SkyManager.h"

#ifdef USE_CAELUM
#include <Caelum.h>
#endif // USE_CAELUM

using namespace Ogre;

// HydraxWater
HydraxWater::HydraxWater(float water_height, Ogre::String conf_file):
    waternoise(0)
    , mHydrax(0)
    , waterHeight(5)
, waveHeight(water_height)
    , mRenderCamera(gEnv->mainCamera)
, CurrentConfigFile(conf_file)
{
    mRenderCamera->setNearClipDistance(0.1f);

    InitHydrax();
}

HydraxWater::~HydraxWater()
{
    mHydrax->remove();
    mHydrax = nullptr;
}

void HydraxWater::InitHydrax()
{
    mHydrax = new Hydrax::Hydrax(gEnv->sceneManager, mRenderCamera, RoR::App::GetOgreSubsystem()->GetViewport());

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

bool HydraxWater::isUnderWater(Ogre::Vector3 pos)
{
    if (pos.y < getHeightWaves(Ogre::Vector3(pos.x, pos.y, pos.z)))
        return true;
    return false;
}

void HydraxWater::setCamera(Ogre::Camera* cam)
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
#ifdef USE_CAELUM
    if (gEnv->sky) //Caelum way of doing things
    {
        Ogre::Vector3 sunPosition = gEnv->mainCamera->getDerivedPosition();
        sunPosition -= gEnv->sky->getCaelumSys()->getSun()->getLightDirection() * 80000;
        mHydrax->setSunPosition(sunPosition);
        mHydrax->setSunColor(Ogre::Vector3(gEnv->sky->getCaelumSys()->getSun()->getBodyColour().r, gEnv->sky->getCaelumSys()->getSun()->getBodyColour().g, gEnv->sky->getCaelumSys()->getSun()->getBodyColour().b));
    }
    else
#endif // USE_CAELUM
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
    if (!RoR::App::gfx_water_waves.GetActive())
    {
        return waterHeight;
    }
    waveHeight = mHydrax->getHeigth(pos);
    return waveHeight;
}

Vector3 HydraxWater::getVelocity(Vector3 pos)
{
    if (!RoR::App::gfx_water_waves.GetActive())
        return Vector3(0, 0, 0);

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
