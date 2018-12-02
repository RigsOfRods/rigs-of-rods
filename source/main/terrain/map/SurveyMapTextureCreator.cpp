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
#include "OgreSubsystem.h"
#include "TerrainManager.h"

using namespace Ogre;
using namespace RoR;

static int counter = 0;

SurveyMapTextureCreator::SurveyMapTextureCreator(Ogre::Vector2 terrain_size)
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
    ConfigOptionMap ropts = App::GetOgreSubsystem()->GetOgreRoot()->getRenderSystem()->getConfigOptions();
    int resolution = StringConverter::parseInt(StringUtil::split(ropts["Video Mode"].currentValue, " x ")[0], 1024);
    int fsaa = StringConverter::parseInt(ropts["FSAA"].currentValue, 0);
    int res = std::pow(2, std::floor(std::log2(resolution)));

    mTexture = Ogre::TextureManager::getSingleton().createManual(mTextureName,
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, res, res,
        Ogre::TU_RENDERTARGET, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET, 0, false, fsaa);

    if (mTexture.isNull())
        return false;;

    mRttTex = mTexture->getBuffer()->getRenderTarget();

    if (!mRttTex)
        return false;

    mRttTex->addListener(this);
    mRttTex->setAutoUpdated(false);

    mCamera = gEnv->sceneManager->createCamera("MapRttCam-" + TOSTRING(counter));
    mCamera->setFixedYawAxis(false);
    mCamera->setDirection(-Vector3::UNIT_Y);
    mCamera->setProjectionType(PT_ORTHOGRAPHIC);
    mCamera->setNearClipDistance(1.0f);
    mCamera->setFarClipDistance(0);

    auto mViewport = mRttTex->addViewport(mCamera);
    mViewport->setBackgroundColour(ColourValue::Black);
    mViewport->setOverlaysEnabled(false);
    mViewport->setShadowsEnabled(false);
    mViewport->setSkiesEnabled(false);

    return true;
}

void SurveyMapTextureCreator::update(Vector2 center, Vector2 size)
{
    if (!mRttTex)
        return;
 
    mCamera->setOrthoWindow(size.x, size.y);
    mCamera->setPosition(Vector3(center.x, 420.0f, center.y));

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
        water->SetStaticWaterHeight(water->GetStaticWaterHeight());
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
