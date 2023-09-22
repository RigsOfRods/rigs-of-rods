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

#include "RTSSManager.h"

#include "Actor.h"
#include "CameraManager.h"
#include "GfxScene.h"
#include "AppContext.h"

#include <Ogre.h>
#include <Terrain/OgreTerrain.h>
#include <Overlay/OgreOverlayContainer.h>
#include <OgreMaterialManager.h>
#include <RTShaderSystem/OgreRTShaderSystem.h>

using namespace Ogre;
using namespace RoR;

RTSSManager::RTSSManager()
{
}

RTSSManager::~RTSSManager()
{
}

void RTSSManager::SetupRTSS()
{
    // RTSS PSSM3
    if(App::gfx_shadow_type->getEnum<GfxShadowType>() == GfxShadowType::PSSM)
    {
        App::GetGfxScene()->GetSceneManager()->setShadowTechnique(SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED);
        App::GetGfxScene()->GetSceneManager()->setShadowFarDistance(350);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL, 3);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureSettings(2048, 3, PF_DEPTH16);
        App::GetGfxScene()->GetSceneManager()->setShadowTextureSelfShadow(true);

        pssmSetup = new PSSMShadowCameraSetup();
        pssmSetup->calculateSplitPoints(3, 1, 500);
        pssmSetup->setSplitPadding(App::GetCameraManager()->GetCamera()->getNearClipDistance());
        pssmSetup->setOptimalAdjustFactor(0, 2);
        pssmSetup->setOptimalAdjustFactor(1, 1);
        pssmSetup->setOptimalAdjustFactor(2, 0.5);

        auto* mShaderGenerator = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
        auto* schemRenderState = mShaderGenerator->getRenderState(Ogre::MSN_SHADERGEN);

        App::GetGfxScene()->GetSceneManager()->setShadowCameraSetup(ShadowCameraSetupPtr(pssmSetup));
        auto subRenderState = mShaderGenerator->createSubRenderState(RTShader::SRS_INTEGRATED_PSSM3);
        schemRenderState->addTemplateSubRenderState(subRenderState);
    }

    // PBR
    //auto subRenderStateNP = mShaderGenerator->createSubRenderState(Ogre::RTShader::SRS_NORMALMAP);
    //schemRenderState->addTemplateSubRenderState(subRenderStateNP);
    //subRenderStateNP->setParameter("texture", "Grass004_1K_NormalGL.jpg");

    //auto subRenderStateMR = mShaderGenerator->createSubRenderState(Ogre::RTShader::SRS_COOK_TORRANCE_LIGHTING);
    //schemRenderState->addTemplateSubRenderState(subRenderStateMR);
    //subRenderStateMR->setParameter("texture", "Grass004_1K_Roughness.jpg");
}

void RTSSManager::EnableRTSS(const MaterialPtr& mat)
{
    Ogre::RTShader::ShaderGenerator* mShaderGenerator = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
    mShaderGenerator->createShaderBasedTechnique(*mat, Ogre::MaterialManager::DEFAULT_SCHEME_NAME, Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
}