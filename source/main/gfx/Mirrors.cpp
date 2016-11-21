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

#include "Mirrors.h"

#include "Application.h"
#include "Beam.h"

#include <Ogre.h>
#include <OgreMaterial.h>
#include <OgreSubsystem.h>

using namespace Ogre;

namespace RoR {
namespace Mirrors {

static bool mInitialized = false;
static bool mEnabled = false;
static Camera* mMirrorCam = nullptr;
static RenderTexture* mRttTex = nullptr;
static MaterialPtr mMat;

void Init()
{
    if (mInitialized)
        return;

    TexturePtr rttTexPtr = TextureManager::getSingleton().createManual("mirrortexture"
        , ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
        , TEX_TYPE_2D
        , 128
        , 256
        , 0
        , PF_R8G8B8
        , TU_RENDERTARGET);

    mRttTex = rttTexPtr->getBuffer()->getRenderTarget();
    mRttTex->setActive(false);

    mMirrorCam = gEnv->sceneManager->createCamera("MirrorCam");
    mMirrorCam->setNearClipDistance(0.2f);
    mMirrorCam->setFarClipDistance(gEnv->mainCamera->getFarClipDistance());
    mMirrorCam->setFOVy(Degree(50));
    mMirrorCam->setAspectRatio(
    ((Real)RoR::App::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualWidth() /
        (Real)RoR::App::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualHeight()) / 2.0f);

    Viewport* v = mRttTex->addViewport(mMirrorCam);
    v->setClearEveryFrame(true);
    v->setBackgroundColour(gEnv->mainCamera->getViewport()->getBackgroundColour());
    v->setOverlaysEnabled(false);

    mMat = MaterialManager::getSingleton().getByName("mirror");
    mMat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("mirror.dds");
    mMat->getTechnique(0)->getPass(0)->setLightingEnabled(false);

    mInitialized = true;
}

void SetActive(bool state)
{
    if (mEnabled == state)
        return;

    mEnabled = state;
    mRttTex->setActive(state);
    if (state)
        mMat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("mirrortexture");
    else
        mMat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("mirror.dds");
}

void Update(Beam* truck)
{
    if (!truck || !mEnabled)
    {
        SetActive(false);
        return;
    }

    // We can only render one mirror, so we choose the nearest one in the fov
    float minDist = 10000.0f;
    int mirrorType = 0;
    SceneNode* mirrorNode = nullptr;
    for (int i = 0; i < truck->free_prop; i++)
    {
        if (truck->props[i].mirror != 0)
        {
            Vector3 dist = truck->props[i].scene_node->getPosition() - gEnv->mainCamera->getPosition();
            if (dist.directionEquals(gEnv->mainCamera->getDirection(), gEnv->mainCamera->getFOVy() * gEnv->mainCamera->getAspectRatio() / 1.2f))
            {
                float fdist = dist.length();
                if (fdist < minDist)
                {
                    minDist = fdist;
                    mirrorNode = truck->props[i].scene_node;
                    mirrorType = truck->props[i].mirror;
                };
            }
        }
    }
    if (mirrorType == 0 || mirrorNode == nullptr)
    {
        SetActive(false);
        return;
    }

    Vector3 normal = Vector3::ZERO;
    Vector3 center = Vector3::ZERO;
    Radian roll = Degree(360) - Radian(asin(truck->getDirection().dotProduct(Vector3::UNIT_Y)));

    if (mirrorType == 1)
    {
        normal = mirrorNode->getOrientation() * Vector3(cos(truck->leftMirrorAngle), sin(truck->leftMirrorAngle), 0.0f);
        center = mirrorNode->getPosition() + mirrorNode->getOrientation() * Vector3(0.07f, -0.22f, 0);
    }
    else
    {
        normal = mirrorNode->getOrientation() * Vector3(cos(truck->rightMirrorAngle), sin(truck->rightMirrorAngle), 0.0f);
        center = mirrorNode->getPosition() + mirrorNode->getOrientation() * Vector3(0.07f, +0.22f, 0);
    }

    Plane plane = Plane(normal, center);
    Vector3 project = plane.projectVector(gEnv->mainCamera->getPosition() - center);

    mMirrorCam->setPosition(center);
    mMirrorCam->lookAt(gEnv->mainCamera->getPosition() - 2.0f * project);
    mMirrorCam->roll(roll);
}

} // namespace Mirrors
} // namespace RoR
