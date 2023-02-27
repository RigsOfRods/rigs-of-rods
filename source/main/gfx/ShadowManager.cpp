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

ShadowManager::ShadowManager()
{
    PSSM_Shadows.mPSSMSetup.setNull();
    PSSM_Shadows.mDepthShadows = false;
    PSSM_Shadows.ShadowsTextureNum = 3;
    PSSM_Shadows.Quality = RoR::App::gfx_shadow_quality->getInt(); //0 = Low quality, 1 = mid, 2 = hq, 3 = ultra
}

ShadowManager::~ShadowManager()
{
}

void ShadowManager::loadConfiguration()
{
    this->updateShadowTechnique(); // Config handled by RoR::App
}

int ShadowManager::updateShadowTechnique()
{
    float scoef = 0.5;
    App::GetGfxScene()->GetSceneManager()->setShadowColour(Ogre::ColourValue(0.563 + scoef, 0.578 + scoef, 0.625 + scoef));
    App::GetGfxScene()->GetSceneManager()->setShowDebugShadows(false);

    if (App::gfx_shadow_type->getEnum<GfxShadowType>() == GfxShadowType::PSSM)
    {
        processPSSM();
        if (App::GetGfxScene()->GetSceneManager()->getShowDebugShadows())
        {
            // add the overlay elements to show the shadow maps:
            // init overlay elements
            OverlayManager& mgr = Ogre::OverlayManager::getSingleton();
            Overlay* overlay = mgr.create("DebugOverlay");

            for (int i = 0; i < PSSM_Shadows.ShadowsTextureNum; ++i)
            {
                TexturePtr tex = App::GetGfxScene()->GetSceneManager()->getShadowTexture(i);

                // Set up a debug panel to display the shadow
                MaterialPtr debugMat = MaterialManager::getSingleton().create("Ogre/DebugTexture" + StringConverter::toString(i), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                debugMat->getTechnique(0)->getPass(0)->setLightingEnabled(false);
                TextureUnitState* t = debugMat->getTechnique(0)->getPass(0)->createTextureUnitState(tex->getName());
                t->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);

                OverlayContainer* debugPanel = (OverlayContainer*)(OverlayManager::getSingleton().createOverlayElement("Panel", "Ogre/DebugTexPanel" + StringConverter::toString(i)));
                debugPanel->_setPosition(0.8, i * 0.25);
                debugPanel->_setDimensions(0.2, 0.24);
                debugPanel->setMaterialName(debugMat->getName());
                debugPanel->setEnabled(true);
                overlay->add2D(debugPanel);
                overlay->show();
            }
        }
    }
    return 0;
}

void ShadowManager::processPSSM()
{
    App::GetGfxScene()->GetSceneManager()->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);

    App::GetGfxScene()->GetSceneManager()->setShadowDirectionalLightExtrusionDistance(299.0f);
    App::GetGfxScene()->GetSceneManager()->setShadowFarDistance(350.0f);
    App::GetGfxScene()->GetSceneManager()->setShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL, PSSM_Shadows.ShadowsTextureNum);
    App::GetGfxScene()->GetSceneManager()->setShadowTextureCount(PSSM_Shadows.ShadowsTextureNum);

    App::GetGfxScene()->GetSceneManager()->setShadowTextureSelfShadow(true);
    App::GetGfxScene()->GetSceneManager()->setShadowCasterRenderBackFaces(true);

    //Caster is set via materials
    MaterialPtr shadowMat = MaterialManager::getSingleton().getByName("Ogre/shadow/depth/caster");
    App::GetGfxScene()->GetSceneManager()->setShadowTextureCasterMaterial(shadowMat);

    if (PSSM_Shadows.Quality == 3)
    {
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(0, 4096, 4096, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(1, 3072, 3072, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(2, 2048, 2048, PF_FLOAT32_R);
        PSSM_Shadows.lambda = 0.965f;
    }
    else if (PSSM_Shadows.Quality == 2)
    {
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(0, 3072, 3072, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(1, 2048, 2048, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(2, 2048, 2048, PF_FLOAT32_R);
        PSSM_Shadows.lambda = 0.97f;
    }
    else if (PSSM_Shadows.Quality == 1)
    {
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(0, 2048, 2048, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(1, 1024, 1024, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(2, 1024, 1024, PF_FLOAT32_R);
        PSSM_Shadows.lambda = 0.975f;
    }
    else
    {
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(0, 1024, 1024, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(1, 1024, 1024, PF_FLOAT32_R);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureConfig(2, 512, 512, PF_FLOAT32_R);
        PSSM_Shadows.lambda = 0.98f;
    }

    if (PSSM_Shadows.mPSSMSetup.isNull())
    {
        // shadow camera setup
        Ogre::PSSMShadowCameraSetup* pssmSetup = new Ogre::PSSMShadowCameraSetup();

        pssmSetup->calculateSplitPoints(3, App::GetCameraManager()->GetCamera()->getNearClipDistance(), App::GetGfxScene()->GetSceneManager()->getShadowFarDistance(), PSSM_Shadows.lambda);
        pssmSetup->setSplitPadding(App::GetCameraManager()->GetCamera()->getNearClipDistance());

        pssmSetup->setOptimalAdjustFactor(0, -1);
        pssmSetup->setOptimalAdjustFactor(1, -1);
        pssmSetup->setOptimalAdjustFactor(2, -1);

        PSSM_Shadows.mPSSMSetup.bind(pssmSetup);

        //Send split info to managed materials
        setManagedMaterialSplitPoints(pssmSetup->getSplitPoints());
    }
    App::GetGfxScene()->GetSceneManager()->setShadowCameraSetup(PSSM_Shadows.mPSSMSetup);
}

void ShadowManager::updatePSSM()
{
    if (!PSSM_Shadows.mPSSMSetup.get())
        return;
    //Ugh what here?
}

void ShadowManager::updateTerrainMaterial(Ogre::TerrainPSSMMaterialGenerator::SM2Profile* matProfile)
{
    if (App::gfx_shadow_type->getEnum<GfxShadowType>() == GfxShadowType::PSSM)
    {
        Ogre::PSSMShadowCameraSetup* pssmSetup = static_cast<Ogre::PSSMShadowCameraSetup*>(PSSM_Shadows.mPSSMSetup.get());
        matProfile->setReceiveDynamicShadowsDepth(true);
        matProfile->setReceiveDynamicShadowsLowLod(false);
        matProfile->setReceiveDynamicShadowsEnabled(true);
        matProfile->setReceiveDynamicShadowsPSSM(pssmSetup);
        matProfile->setLightmapEnabled(false);
    }
}

void ShadowManager::setManagedMaterialSplitPoints(Ogre::PSSMShadowCameraSetup::SplitPointList splitPointList)
{
    Ogre::Vector4 splitPoints;

    for (int i = 0; i < 3; ++i)
        splitPoints[i] = splitPointList[i];

    GpuSharedParametersPtr p = GpuProgramManager::getSingleton().getSharedParameters("pssm_params");
    p->setNamedConstant("pssmSplitPoints", splitPoints);
}
