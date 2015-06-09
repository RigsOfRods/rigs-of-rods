/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "ShadowManager.h"

#include "Ogre.h"
#include "OgreShadowCameraSetup.h"
#include "OgreTerrain.h"
#include "OgreTerrainMaterialGeneratorA.h"

#include "Settings.h"

using namespace Ogre;

ShadowManager::ShadowManager()
{
	PSSM_Shadows.mPSSMSetup.setNull();
	PSSM_Shadows.mDepthShadows = false;
	PSSM_Shadows.ShadowsTextureNum = 3;
}

ShadowManager::~ShadowManager()
{
}

void ShadowManager::loadConfiguration()
{
	Ogre::String s = SSETTING("Shadow technique", "Parallel-split Shadow Maps");
	if (s == "Texture shadows")
		ShadowsType = SHADOWS_TEXTURE;
	else if (s == "Parallel-split Shadow Maps")
		ShadowsType = SHADOWS_PSSM;
	else
		ShadowsType = SHADOWS_NONE;

	updateShadowTechnique();
}

int ShadowManager::updateShadowTechnique()
{
	float shadowFarDistance = FSETTING("SightRange", 2000);
	float scoef = 0.12;
	gEnv->sceneManager->setShadowColour(Ogre::ColourValue(0.563 + scoef, 0.578 + scoef, 0.625 + scoef));
	gEnv->sceneManager->setShowDebugShadows(false);

	if (ShadowsType == SHADOWS_TEXTURE)
	{
		gEnv->sceneManager->setShadowFarDistance(shadowFarDistance);
		processTextureShadows();
	}
	else if (ShadowsType == SHADOWS_PSSM)
	{
		processPSSM();
		if (gEnv->sceneManager->getShowDebugShadows())
		{
			// add the overlay elements to show the shadow maps:
			// init overlay elements
			OverlayManager& mgr = Ogre::OverlayManager::getSingleton();
			Overlay* overlay = mgr.create("DebugOverlay");

			for (int i = 0; i < PSSM_Shadows.ShadowsTextureNum; ++i) {
				TexturePtr tex = gEnv->sceneManager->getShadowTexture(i);


				// Set up a debug panel to display the shadow
				MaterialPtr debugMat = MaterialManager::getSingleton().create("Ogre/DebugTexture" + StringConverter::toString(i), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
				debugMat->getTechnique(0)->getPass(0)->setLightingEnabled(false);
				TextureUnitState *t = debugMat->getTechnique(0)->getPass(0)->createTextureUnitState(tex->getName());
				t->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);

				OverlayContainer* debugPanel = (OverlayContainer*)(OverlayManager::getSingleton().createOverlayElement("Panel", "Ogre/DebugTexPanel" + StringConverter::toString(i)));
				debugPanel->_setPosition(0.8, i*0.25);
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
	gEnv->sceneManager->setShadowFarDistance(2000);
	gEnv->sceneManager->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);

	gEnv->sceneManager->setShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL, PSSM_Shadows.ShadowsTextureNum);

	if (PSSM_Shadows.mPSSMSetup.isNull())
	{
		// shadow camera setup
		Ogre::PSSMShadowCameraSetup* pssmSetup = new Ogre::PSSMShadowCameraSetup();
		//pssmSetup->setCameraLightDirectionThreshold(Ogre::Degree(15.0));
		//pssmSetup->setUseAggressiveFocusRegion(true);
		//pssmSetup->setUseSimpleOptimalAdjust(false);

		/*pssmSetup->calculateSplitPoints(3, 0.5, 600, 0.91f);
		*/
		pssmSetup->setSplitPadding(5);
		pssmSetup->setOptimalAdjustFactor(0, 2);
		pssmSetup->setOptimalAdjustFactor(1, 1);
		pssmSetup->setOptimalAdjustFactor(2, 0.5);

		PSSM_Shadows.mPSSMSetup.bind(pssmSetup);

	}
	gEnv->sceneManager->setShadowCameraSetup(PSSM_Shadows.mPSSMSetup);


	gEnv->sceneManager->setShadowTextureCount(PSSM_Shadows.ShadowsTextureNum);

	gEnv->sceneManager->setShadowTextureConfig(0, 2048, 2048, PF_FLOAT16_RGB);
	gEnv->sceneManager->setShadowTextureConfig(1, 1024, 1024, PF_FLOAT16_RGB);
	gEnv->sceneManager->setShadowTextureConfig(2, 512,  512,  PF_FLOAT16_RGB);

	gEnv->sceneManager->setShadowTextureSelfShadow(true);
	gEnv->sceneManager->setShadowCasterRenderBackFaces(true);

	gEnv->sceneManager->setShadowTextureCasterMaterial(PSSM_Shadows.mDepthShadows ? "PSSM/shadow_caster" : Ogre::StringUtil::BLANK);

	updatePSSM();
}

void ShadowManager::updatePSSM(Ogre::Terrain* terrain)
{
	if (!PSSM_Shadows.mPSSMSetup.get())  return;

	Ogre::TerrainMaterialGeneratorA::SM2Profile *matProfile = 0;
	if (Ogre::TerrainGlobalOptions::getSingletonPtr())
	{
		matProfile = static_cast<Ogre::TerrainMaterialGeneratorA::SM2Profile*>(Ogre::TerrainGlobalOptions::getSingleton().getDefaultMaterialGenerator()->getActiveProfile());
		matProfile->setReceiveDynamicShadowsEnabled(true);
		matProfile->setReceiveDynamicShadowsLowLod(true);
		matProfile->setGlobalColourMapEnabled(true);
	}


	Ogre::PSSMShadowCameraSetup* pssmSetup = static_cast<Ogre::PSSMShadowCameraSetup*>(PSSM_Shadows.mPSSMSetup.get());
	Ogre::PSSMShadowCameraSetup::SplitPointList splitPointList = pssmSetup->getSplitPoints();

	
	splitPointList[0] = 1.0;
	splitPointList[1] = 95.0;
	splitPointList[2] = 255.0;
	splitPointList[3] = 512.0;
	pssmSetup->setSplitPoints(splitPointList);
	
	
	Ogre::Vector4 splitPoints;
	for (int i = 0; i < PSSM_Shadows.ShadowsTextureNum; ++i)
		splitPoints[i] = splitPointList[i];

	MaterialPtr mat = MaterialManager::getSingleton().getByName("RoR/Managed_Mats/Base");
	mat->getTechnique("ShadowTechnique")->getPass(0)->getFragmentProgramParameters()->setNamedConstant("pssmSplitPoints", splitPoints);

	// TODO: fix this
/*	setMaterialSplitPoints("road", splitPoints);
	setMaterialSplitPoints("road2", splitPoints);
	*/

	if (matProfile && terrain)
	{
		matProfile->generateForCompositeMap(terrain);
		matProfile->setReceiveDynamicShadowsDepth(PSSM_Shadows.mDepthShadows);
		matProfile->setReceiveDynamicShadowsPSSM(static_cast<Ogre::PSSMShadowCameraSetup*>(PSSM_Shadows.mPSSMSetup.get()));
	}
}

void ShadowManager::setMaterialSplitPoints(Ogre::String materialName, Ogre::Vector4 &splitPoints)
{
	/*	Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName(materialName);
	if (!mat.isNull())
	{
	unsigned short np = mat->getTechnique(0)->getNumPasses()-1;  // last
	try {
	mat->getTechnique(0)->getPass(np)->getFragmentProgramParameters()->setNamedConstant("pssmSplitPoints", splitPoints);
	} catch(...)
	{
	// this material is not prepared for PSSM usage !
	}
	}*/
}
