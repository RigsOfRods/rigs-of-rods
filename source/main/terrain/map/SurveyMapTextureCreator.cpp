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
#include "RoRFrameListener.h"
#include "SurveyMapManager.h"
#include "TerrainManager.h"

using namespace Ogre;
using namespace RoR;

static Ogre::Camera* mCamera = nullptr;
static Ogre::MaterialPtr mMaterial;
static Ogre::RenderTarget* mRttTex = nullptr;
static Ogre::TextureUnitState* mTextureUnitState = nullptr;
static Ogre::Viewport* mViewport = nullptr;

SurveyMapTextureCreator::SurveyMapTextureCreator(Ogre::Vector2 terrain_size) :
    mMapSize(terrain_size)
{
    init();
}

bool SurveyMapTextureCreator::init()
{
    TexturePtr texture = Ogre::TextureManager::getSingleton().getByName("MapRttTex");

    if (texture.isNull())
        return false;;

    mRttTex = texture->getBuffer()->getRenderTarget();

    if (!mRttTex)
        return false;

    mRttTex->setAutoUpdated(false);

    try
    {
        mCamera = gEnv->sceneManager->getCamera(getCameraName());
    }
    catch (ItemIdentityException& e)
    {
        mCamera = gEnv->sceneManager->createCamera(getCameraName());
    }

    if (!(mViewport = mRttTex->getViewport(0)))
        mViewport = mRttTex->addViewport(mCamera);
    mViewport->setBackgroundColour(ColourValue::Black);
    mViewport->setOverlaysEnabled(false);
    mViewport->setShadowsEnabled(false);
    mViewport->setSkiesEnabled(false);

    mMaterial = MaterialManager::getSingleton().create(getMaterialName(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    if (mMaterial.isNull())
        return false;

    mTextureUnitState = mMaterial->getTechnique(0)->getPass(0)->createTextureUnitState(getTextureName());

    mRttTex->addListener(this);

    mCamera->setFixedYawAxis(false);
    mCamera->setProjectionType(PT_ORTHOGRAPHIC);
    mCamera->setNearClipDistance(1.0f);

    return true;
}

void SurveyMapTextureCreator::update()
{
    if (!mRttTex)
        return;

    Vector2 mMapCenter = Vector2::ZERO;
    float mMapZoom = 0.0f;

    SurveyMapManager* survey_map = App::GetSimController()->GetGfxScene().GetSurveyMap();
    if (survey_map)
    {
        mMapCenter = survey_map->getMapCenter();
        mMapZoom = survey_map->getMapZoom();
    }

    float farclip = gEnv->mainCamera->getFarClipDistance();
    float orthoWindowWidth = mMapSize.x - (mMapSize.x - 20.0f) * mMapZoom;
    float orthoWindowHeight = mMapSize.y - (mMapSize.y - 20.0f) * mMapZoom;

    mCamera->setFarClipDistance(farclip);
    mCamera->setOrthoWindow(orthoWindowWidth, orthoWindowHeight);
    mCamera->setPosition(Vector3(mMapCenter.x, farclip * 0.1f, mMapCenter.y));
    mCamera->lookAt(Vector3(mMapCenter.x, 0.0f, mMapCenter.y));

    preRenderTargetUpdate();

    mRttTex->update();

    postRenderTargetUpdate();
}

String SurveyMapTextureCreator::getMaterialName()
{
    return "MapRttMat";
}

String SurveyMapTextureCreator::getCameraName()
{
    return "MapRttCam";
}

String SurveyMapTextureCreator::getTextureName()
{
    return "MapRttTex";
}

void SurveyMapTextureCreator::preRenderTargetUpdate()
{
    auto water = App::GetSimTerrain()->getWater();
    if (water)
    {
        water->WaterSetCamera(mCamera);
        water->SetStaticWaterHeight(water->GetStaticWaterHeight());
        water->UpdateWater();
    }
}

void SurveyMapTextureCreator::postRenderTargetUpdate()
{
    auto water = App::GetSimTerrain()->getWater();
    if (water)
    {
        water->WaterSetCamera(gEnv->mainCamera);
        water->UpdateWater();
    }
}
