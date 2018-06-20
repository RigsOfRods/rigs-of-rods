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

#include <Ogre.h>
#include <Terrain/OgreTerrain.h>
#include <Overlay/OgreOverlayManager.h>
#include <Overlay/OgreOverlayContainer.h>
#include <Overlay/OgreOverlay.h>
#include <OgreMaterialManager.h>

#include "Settings.h"

using namespace Ogre;

ShadowManager::ShadowManager()
{
    PSSM_Shadows.mPSSMSetup.setNull();
    PSSM_Shadows.mDepthShadows = false;
    PSSM_Shadows.ShadowsTextureNum = 3;
    PSSM_Shadows.Quality = ISETTING("Shadows Quality", 2); //0 = Low quality, 1 = mid, 2 = hq, 3 = ultra
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
    gEnv->sceneManager->setShadowColour(Ogre::ColourValue(0.563 + scoef, 0.578 + scoef, 0.625 + scoef));
    gEnv->sceneManager->setShowDebugShadows(false);

    RoR::GfxShadowType type = RoR::App::gfx_shadow_type.GetActive();

    if (type == RoR::GfxShadowType::TEXTURE)
    {
        gEnv->sceneManager->setShadowFarDistance(RoR::App::gfx_sight_range.GetActive());
        processTextureShadows();
    }
    else if (type == RoR::GfxShadowType::PSSM)
    {
        processPSSM();
        if (gEnv->sceneManager->getShowDebugShadows())
        {
            // add the overlay elements to show the shadow maps:
            // init overlay elements
            OverlayManager& mgr = Ogre::OverlayManager::getSingleton();
            Overlay* overlay = mgr.create("DebugOverlay");

            for (int i = 0; i < PSSM_Shadows.ShadowsTextureNum; ++i)
            {
                TexturePtr tex = gEnv->sceneManager->getShadowTexture(i);

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

void ShadowManager::processTextureShadows()
{
    gEnv->sceneManager->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE);
    gEnv->sceneManager->setShadowTextureSettings(2048, 2);
}

void ShadowManager::processPSSM()
{
    gEnv->sceneManager->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);

    gEnv->sceneManager->setShadowDirectionalLightExtrusionDistance(299.0f);
    gEnv->sceneManager->setShadowFarDistance(350.0f);
    gEnv->sceneManager->setShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL, PSSM_Shadows.ShadowsTextureNum);
    gEnv->sceneManager->setShadowTextureCount(PSSM_Shadows.ShadowsTextureNum);

    gEnv->sceneManager->setShadowTextureSelfShadow(true);
    gEnv->sceneManager->setShadowCasterRenderBackFaces(true);

    //Caster is set via materials
    MaterialPtr shadowMat = MaterialManager::getSingleton().getByName("Ogre/shadow/depth/caster");
    gEnv->sceneManager->setShadowTextureCasterMaterial(shadowMat);

    if (PSSM_Shadows.Quality == 3)
    {
        gEnv->sceneManager->setShadowTextureConfig(0, 4096, 4096, PF_FLOAT32_R);
        gEnv->sceneManager->setShadowTextureConfig(1, 3072, 3072, PF_FLOAT32_R);
        gEnv->sceneManager->setShadowTextureConfig(2, 2048, 2048, PF_FLOAT32_R);
        PSSM_Shadows.lambda = 0.965f;
    }
    else if (PSSM_Shadows.Quality == 2)
    {
        gEnv->sceneManager->setShadowTextureConfig(0, 3072, 3072, PF_FLOAT32_R);
        gEnv->sceneManager->setShadowTextureConfig(1, 2048, 2048, PF_FLOAT32_R);
        gEnv->sceneManager->setShadowTextureConfig(2, 2048, 2048, PF_FLOAT32_R);
        PSSM_Shadows.lambda = 0.97f;
    }
    else if (PSSM_Shadows.Quality == 1)
    {
        gEnv->sceneManager->setShadowTextureConfig(0, 2048, 2048, PF_FLOAT32_R);
        gEnv->sceneManager->setShadowTextureConfig(1, 1024, 1024, PF_FLOAT32_R);
        gEnv->sceneManager->setShadowTextureConfig(2, 1024, 1024, PF_FLOAT32_R);
        PSSM_Shadows.lambda = 0.975f;
    }
    else
    {
        gEnv->sceneManager->setShadowTextureConfig(0, 1024, 1024, PF_FLOAT32_R);
        gEnv->sceneManager->setShadowTextureConfig(1, 1024, 1024, PF_FLOAT32_R);
        gEnv->sceneManager->setShadowTextureConfig(2, 512, 512, PF_FLOAT32_R);
        PSSM_Shadows.lambda = 0.98f;
    }

    if (PSSM_Shadows.mPSSMSetup.isNull())
    {
        // shadow camera setup
        Ogre::PSSMShadowCameraSetup* pssmSetup = new Ogre::PSSMShadowCameraSetup();

        pssmSetup->calculateSplitPoints(3, gEnv->mainCamera->getNearClipDistance(), gEnv->sceneManager->getShadowFarDistance(), PSSM_Shadows.lambda);
        pssmSetup->setSplitPadding(gEnv->mainCamera->getNearClipDistance());

        pssmSetup->setOptimalAdjustFactor(0, -1);
        pssmSetup->setOptimalAdjustFactor(1, -1);
        pssmSetup->setOptimalAdjustFactor(2, -1);

        PSSM_Shadows.mPSSMSetup.bind(pssmSetup);

        //Send split info to managed materials
        setManagedMaterialSplitPoints(pssmSetup->getSplitPoints());
    }
    gEnv->sceneManager->setShadowCameraSetup(PSSM_Shadows.mPSSMSetup);
}

void ShadowManager::updatePSSM()
{
    if (!PSSM_Shadows.mPSSMSetup.get())
        return;
    //Ugh what here?
}

void ShadowManager::updateTerrainMaterial(Ogre::TerrainPSSMMaterialGenerator::SM2Profile* matProfile)
{
    if (RoR::App::gfx_shadow_type.GetActive() == RoR::GfxShadowType::PSSM)
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
