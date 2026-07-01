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

#include "ShadowManager.h"

#include "Actor.h"
#include "CameraManager.h"
#include "GfxScene.h"

#include <Ogre.h>
#include <Terrain/OgreTerrain.h>
#include <Overlay/OgreOverlayManager.h>
#include <Overlay/OgreOverlayContainer.h>
#include <Overlay/OgreOverlay.h>
#include <OgreMaterialManager.h>

using namespace Ogre;
using namespace RoR;


void ShadowManager::setupShadows()
{
    if (App::gfx_shadow_type->getEnum<GfxShadowType>() == GfxShadowType::NONE)
        return;

    this->processPSSM();
}

void ShadowManager::processPSSM()
{
    const int NUM_SHADOW_TEXTURES = 3;
    const int NUM_SPLITS = 3;

    App::GetGfxScene()->GetSceneManager()->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);
    App::GetGfxScene()->GetSceneManager()->setShadowDirLightTextureOffset(0);
    App::GetGfxScene()->GetSceneManager()->setShadowFarDistance(1000.0f);
    App::GetGfxScene()->GetSceneManager()->setShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL, NUM_SHADOW_TEXTURES);
    App::GetGfxScene()->GetSceneManager()->setShadowTextureCount(NUM_SHADOW_TEXTURES);

    App::GetGfxScene()->GetSceneManager()->setShadowTextureSelfShadow(true);
    App::GetGfxScene()->GetSceneManager()->setShadowCasterRenderBackFaces(true);

    //Caster is set via materials
    MaterialPtr shadowMat = MaterialManager::getSingleton().getByName("Ogre/shadow/depth/caster");
    App::GetGfxScene()->GetSceneManager()->setShadowTextureCasterMaterial(shadowMat);

    if (App::gfx_shadow_quality->getInt() == 3) // Ultra
    {
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(0, 4096, 4096, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(1, 3072, 3072, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(2, 2048, 2048, PF_FLOAT32_R);
    }
    else if (App::gfx_shadow_quality->getInt() == 2) // HQ
    {
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(0, 3072, 3072, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(1, 2048, 2048, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(2, 2048, 2048, PF_FLOAT32_R);
    }
    else if (App::gfx_shadow_quality->getInt() == 1) // Mid
    {
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(0, 2048, 2048, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(1, 1024, 1024, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(2, 1024, 1024, PF_FLOAT32_R);
    }
    else // Low
    {
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(0, 1024, 1024, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(1, 1024, 1024, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(2, 512, 512, PF_FLOAT32_R);
    }

    // shadow camera setup
    Ogre::PSSMShadowCameraSetup* pssmSetup = new Ogre::PSSMShadowCameraSetup();
    pssmSetup->calculateSplitPoints(NUM_SPLITS, App::GetCameraManager()->GetCamera()->getNearClipDistance(), App::GetGfxScene()->GetSceneManager()->getShadowFarDistance());
    pssmSetup->setSplitPadding(App::GetCameraManager()->GetCamera()->getNearClipDistance() * 2);
    pssmSetup->setOptimalAdjustFactor(0, 2);
    pssmSetup->setOptimalAdjustFactor(1, 1);
    pssmSetup->setOptimalAdjustFactor(2, 0.5);
    App::GetGfxScene()->GetSceneManager()->setShadowCameraSetup(Ogre::ShadowCameraSetupPtr(pssmSetup));

    //Send split info to managed materials
    setManagedMaterialSplitPoints(pssmSetup->getSplitPoints());
}

void ShadowManager::setManagedMaterialSplitPoints(Ogre::PSSMShadowCameraSetup::SplitPointList splitPointList)
{
    Ogre::Vector4 splitPoints;

    for (int i = 0; i < 3; ++i)
        splitPoints[i] = splitPointList[i];

    GpuSharedParametersPtr p = GpuProgramManager::getSingleton().getSharedParameters("pssm_params");
    p->setNamedConstant("pssmSplitPoints", splitPoints);
}
