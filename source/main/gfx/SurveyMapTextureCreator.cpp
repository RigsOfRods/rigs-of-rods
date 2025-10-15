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

#include "Actor.h"
#include "Application.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "IGfxWater.h"
#include "Terrain.h"

using namespace Ogre;
using namespace RoR;

static int counter = 0;

SurveyMapTextureCreator::SurveyMapTextureCreator(Ogre::Real terrain_height) :
    mCamera(nullptr),
    mRttTex(nullptr),
    mTerrainHeight(Math::Clamp(terrain_height + 100.0f, 150.0f, 2500.0f))
{
    counter++;
    mTextureName = "MapRttTex-" + TOSTRING(counter);
}

SurveyMapTextureCreator::~SurveyMapTextureCreator()
{
    if (mRttTex)
        mRttTex->removeAllViewports();
    if (mCamera)
        App::GetGfxScene()->GetSceneManager()->destroyCamera(mCamera);
    if (mTexture)
        Ogre::TextureManager::getSingleton().remove(mTexture);
}

bool SurveyMapTextureCreator::init(int res, int fsaa)
{
    mTexture = Ogre::TextureManager::getSingleton().createManual(mTextureName,
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, res, res,
        Ogre::TU_RENDERTARGET, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET, 0, false, fsaa);

    if (!mTexture)
        return false;

    mRttTex = mTexture->getBuffer()->getRenderTarget();

    if (!mRttTex)
        return false;

    mRttTex->addListener(this);
    mRttTex->setAutoUpdated(false);

    mCamera = App::GetGfxScene()->GetSceneManager()->createCamera("MapRttCam-" + TOSTRING(counter));
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
    mCamera->setPosition(Vector3(center.x, mTerrainHeight, center.y));

    mRttTex->update();
}

void SurveyMapTextureCreator::preRenderTargetUpdate(const RenderTargetEvent &evt)
{
    auto water = App::GetGameContext()->GetTerrain()->getGfxWater();
    if (water)
    {
        water->SetForcedCameraTransform(mCamera->getFOVy(),
            mCamera->getPosition(), mCamera->getOrientation()); // FIXME: Legacy OGRE API! Use camera node instead!
        water->UpdateWater();
        water->ClearForcedCameraTransform();
    }
}

void SurveyMapTextureCreator::postRenderTargetUpdate(const RenderTargetEvent &evt)
{
    auto water = App::GetGameContext()->GetTerrain()->getGfxWater();
    if (water)
    {
        water->UpdateWater();
    }
}

Ogre::TexturePtr SurveyMapTextureCreator::convertTextureToStatic(const std::string& texName, const std::string& rgName)
{
    Ogre::Image img;
    mTexture->convertToImage(img);
    return Ogre::TextureManager::getSingleton().loadImage(texName, rgName, img);
}
