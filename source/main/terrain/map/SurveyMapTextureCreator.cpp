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

int SurveyMapTextureCreator::mCounter = 0;

SurveyMapTextureCreator::SurveyMapTextureCreator(Ogre::Vector2 terrain_size) :
    mCamera(nullptr)
    , mRttTex(nullptr)
    , mStatics(nullptr)
    , mTextureUnitState(nullptr)
    , mViewport(nullptr)
    , mMapCenter(Vector2::ZERO)
    , mMapSize(terrain_size)
    , mMapZoom(0.0f)
{
    mCounter++;
    init();
}

bool SurveyMapTextureCreator::init()
{
    TexturePtr texture = TextureManager::getSingleton().createManual(getTextureName(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 2048, 2048, TU_RENDERTARGET, PF_R8G8B8, TU_RENDERTARGET);

    if (texture.isNull())
        return false;;

    mRttTex = texture->getBuffer()->getRenderTarget();

    if (!mRttTex)
        return false;

    mRttTex->setAutoUpdated(false);

    mCamera = gEnv->sceneManager->createCamera(getCameraName());

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

void SurveyMapTextureCreator::setStaticGeometry(StaticGeometry* staticGeometry)
{
    mStatics = staticGeometry;
}

void SurveyMapTextureCreator::update()
{
    if (!mRttTex)
        return;

    mMapCenter = Vector2::ZERO;
    mMapZoom = 0.0f;

    SurveyMapManager* survey_map = App::GetSimController()->GetGfxScene().GetSurveyMap();
    if (survey_map)
    {
        mMapCenter = survey_map->getMapCenter();
        mMapZoom = survey_map->getMapZoom();
    }

    float orthoWindowWidth = mMapSize.x - (mMapSize.x - 20.0f) * mMapZoom;
    float orthoWindowHeight = mMapSize.y - (mMapSize.y - 20.0f) * mMapZoom;

    mCamera->setFarClipDistance(mMapSize.y + 3.0f);
    mCamera->setOrthoWindow(orthoWindowWidth, orthoWindowHeight);
    mCamera->setPosition(Vector3(mMapCenter.x, mMapSize.y + 2.0f, mMapCenter.y));
    mCamera->lookAt(Vector3(mMapCenter.x, 0.0f, mMapCenter.y));

    preRenderTargetUpdate();

    mRttTex->update();

    postRenderTargetUpdate();
}

String SurveyMapTextureCreator::getMaterialName()
{
    return "MapRttMat" + TOSTRING(mCounter);
}

String SurveyMapTextureCreator::getCameraName()
{
    return "MapRttCam" + TOSTRING(mCounter);
}

String SurveyMapTextureCreator::getTextureName()
{
    return "MapRttTex" + TOSTRING(mCounter);
}

void SurveyMapTextureCreator::preRenderTargetUpdate()
{
    if (mStatics)
        mStatics->setRenderingDistance(0);

    IWater* water = App::GetSimTerrain()->getWater();
    if (water)
    {
        water->WaterSetCamera(mCamera);
        water->SetReflectionPlaneHeight(water->GetStaticWaterHeight());
        water->UpdateWater();
    }
}

void SurveyMapTextureCreator::postRenderTargetUpdate()
{
    if (mStatics)
        mStatics->setRenderingDistance(1000);

    IWater* water = App::GetSimTerrain()->getWater();
    if (water)
    {
        water->WaterSetCamera(gEnv->mainCamera);
        water->SetReflectionPlaneHeight(water->GetStaticWaterHeight());
        water->UpdateWater();
    }
}
