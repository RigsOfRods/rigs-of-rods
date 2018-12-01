/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#include "SurveyMapTextureCreator.h"

#include "Application.h"
#include "IWater.h"
#include "TerrainManager.h"

using namespace Ogre;
using namespace RoR;

static int counter = 0;

SurveyMapTextureCreator::SurveyMapTextureCreator(Ogre::Vector2 terrain_size) :
    mMapSize(terrain_size)
{
    counter++;
    mTextureName = "MapRttTex-" + TOSTRING(counter);
}

SurveyMapTextureCreator::~SurveyMapTextureCreator()
{
    if (mCamera)
        gEnv->sceneManager->destroyCamera(mCamera);
    if (mTexture)
        Ogre::TextureManager::getSingleton().remove(mTexture);
}

bool SurveyMapTextureCreator::init()
{
    mTexture = Ogre::TextureManager::getSingleton().createManual(mTextureName,
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, 2048, 2048, 
        Ogre::TU_RENDERTARGET, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);

    if (mTexture.isNull())
        return false;;

    mRttTex = mTexture->getBuffer()->getRenderTarget();

    if (!mRttTex)
        return false;

    mRttTex->addListener(this);
    mRttTex->setAutoUpdated(false);

    mCamera = gEnv->sceneManager->createCamera("MapRttCam-" + TOSTRING(counter));
    mCamera->setFixedYawAxis(false);
    mCamera->setProjectionType(PT_ORTHOGRAPHIC);
    mCamera->setNearClipDistance(1.0f);

    auto mViewport = mRttTex->addViewport(mCamera);
    mViewport->setBackgroundColour(ColourValue::Black);
    mViewport->setOverlaysEnabled(false);
    mViewport->setShadowsEnabled(false);
    mViewport->setSkiesEnabled(false);

    return true;
}

void SurveyMapTextureCreator::update(Vector2 center, float zoom)
{
    if (!mRttTex)
        return;

    float farclip = gEnv->mainCamera->getFarClipDistance();
    float cameraHeight = std::max(center.x, farclip) * 0.1f;
    float orthoWindowWidth = mMapSize.x - (mMapSize.x - 20.0f) * zoom;
    float orthoWindowHeight = mMapSize.y - (mMapSize.y - 20.0f) * zoom;

    center.x = Math::Clamp(center.x, orthoWindowWidth  / 2, mMapSize.x - orthoWindowWidth  / 2);
    center.y = Math::Clamp(center.y, orthoWindowHeight / 2, mMapSize.y - orthoWindowHeight / 2);

    mCamera->setFarClipDistance(farclip);
    mCamera->setOrthoWindow(orthoWindowWidth, orthoWindowHeight);
    mCamera->setPosition(Vector3(center.x, cameraHeight, center.y));
    mCamera->lookAt(Vector3(center.x, 0.0f, center.y));

    mRttTex->update();
}

String SurveyMapTextureCreator::getTextureName()
{
    return mTextureName;
}

void SurveyMapTextureCreator::preRenderTargetUpdate(const RenderTargetEvent &evt)
{
    auto water = App::GetSimTerrain()->getWater();
    if (water)
    {
        water->WaterSetCamera(mCamera);
        water->SetStaticWaterHeight(water->GetStaticWaterHeight());
        water->UpdateWater();
    }
}

void SurveyMapTextureCreator::postRenderTargetUpdate(const RenderTargetEvent &evt)
{
    auto water = App::GetSimTerrain()->getWater();
    if (water)
    {
        water->WaterSetCamera(gEnv->mainCamera);
        water->UpdateWater();
    }
}
