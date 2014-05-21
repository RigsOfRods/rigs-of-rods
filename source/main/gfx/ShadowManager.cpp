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

ShadowManager::ShadowManager() : mPSSMSetup()
{
	mDepthShadows = false;
}

ShadowManager::~ShadowManager()
{
}

void ShadowManager::loadConfiguration()
{
	Ogre::String s = SSETTING("Shadow technique", "Parallel-split Shadow Maps");
	if (s == "Stencil shadows")
		changeShadowTechnique(Ogre::SHADOWTYPE_STENCIL_MODULATIVE);
	else if (s == "Texture shadows")
		changeShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE);
	else if (s == "Parallel-split Shadow Maps")
		changeShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED);
	else 
		changeShadowTechnique(Ogre::SHADOWTYPE_NONE);
}

int ShadowManager::changeShadowTechnique(Ogre::ShadowTechnique tech)
{
	float shadowFarDistance = std::min(200.0f, (FSETTING("SightRange", 2000)* 0.8f));
	float scoef=0.2;
	gEnv->sceneManager->setShadowColour(Ogre::ColourValue(0.563+scoef, 0.578+scoef, 0.625+scoef));

	gEnv->sceneManager->setShadowTechnique(tech);
	gEnv->sceneManager->setShadowFarDistance(shadowFarDistance);
	gEnv->sceneManager->setShowDebugShadows(false);

	if (tech == Ogre::SHADOWTYPE_STENCIL_MODULATIVE)
	{
		//		gEnv->ogreSceneManager->setShadowIndexBufferSize(2000000);
		gEnv->sceneManager->setShadowDirectionalLightExtrusionDistance(100);

		//important optimization
		gEnv->sceneManager->getRenderQueue()->getQueueGroup(Ogre::RENDER_QUEUE_WORLD_GEOMETRY_1)->setShadowsEnabled(false);

		//		gEnv->ogreSceneManager->setUseCullCamera(false);
		//		gEnv->ogreSceneManager->setShowBoxes(true);
		//		gEnv->ogreSceneManager->showBoundingBoxes(true);
	} else if (tech == Ogre::SHADOWTYPE_TEXTURE_MODULATIVE)
	{
		gEnv->sceneManager->setShadowTextureSettings(2048,2);
	} else if (tech == Ogre::SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED)
	{
#if OGRE_VERSION>0x010602

		// General scene setup

		// 3 textures per directional light (PSSM)
		int num = 3;

		gEnv->sceneManager->setShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL, num);

		if (mPSSMSetup.isNull())
		{
			// shadow camera setup
			Ogre::PSSMShadowCameraSetup* pssmSetup = new Ogre::PSSMShadowCameraSetup();
			pssmSetup->setSplitPadding(gEnv->mainCamera->getNearClipDistance());
			pssmSetup->calculateSplitPoints(3, gEnv->mainCamera->getNearClipDistance(), gEnv->sceneManager->getShadowFarDistance());
			for (int i=0; i < num; ++i)
			{	//int size = i==0 ? 2048 : 1024;
				const Ogre::Real cAdjfA[5] = {2, 1, 0.5, 0.25, 0.125};
				pssmSetup->setOptimalAdjustFactor(i, cAdjfA[std::min(i, 4)]);
			}
			mPSSMSetup.bind(pssmSetup);

		}
		gEnv->sceneManager->setShadowCameraSetup(mPSSMSetup);
		
		
		gEnv->sceneManager->setShadowTextureCount(num);
		for (int i=0; i < num; ++i)
		{	int size = i==0 ? 2048 : 1024;
			gEnv->sceneManager->setShadowTextureConfig(i, size, size, mDepthShadows ? Ogre::PF_FLOAT32_R : Ogre::PF_X8B8G8R8);
		}

		gEnv->sceneManager->setShadowTextureSelfShadow(mDepthShadows);
		gEnv->sceneManager->setShadowCasterRenderBackFaces(false);

		gEnv->sceneManager->setShadowTextureCasterMaterial(mDepthShadows?"PSSM/shadow_caster":Ogre::StringUtil::BLANK);

		updatePSSM();

#else
		ErrorUtils::ShowError("Parallel-split Shadow Maps as shadow technique is only available when you build with Ogre 1.6 support.", "PSSM error");
		exit(1);
#endif //OGRE_VERSION
	}
	return 0;
}

void ShadowManager::updatePSSM(Ogre::Terrain* terrain)
{
	if (!mPSSMSetup.get())  return;

	Ogre::TerrainMaterialGeneratorA::SM2Profile *matProfile  = 0;
	if (Ogre::TerrainGlobalOptions::getSingletonPtr())
	{
		matProfile = static_cast<Ogre::TerrainMaterialGeneratorA::SM2Profile*>(Ogre::TerrainGlobalOptions::getSingleton().getDefaultMaterialGenerator()->getActiveProfile());
		matProfile->setReceiveDynamicShadowsEnabled(true);
		matProfile->setReceiveDynamicShadowsLowLod(true);
		matProfile->setGlobalColourMapEnabled(false);
	}


	Ogre::PSSMShadowCameraSetup* pssmSetup = static_cast<Ogre::PSSMShadowCameraSetup*>(mPSSMSetup.get());
	const Ogre::PSSMShadowCameraSetup::SplitPointList& splitPointList = pssmSetup->getSplitPoints();

	Ogre::Vector4 splitPoints;
	for (size_t i = 0; i < /*3*/splitPointList.size(); ++i)
		splitPoints[i] = splitPointList[i];

	// TODO: fix this
	//setMaterialSplitPoints("road", splitPoints);
	//setMaterialSplitPoints("road2", splitPoints);


	if (matProfile && terrain)
	{
		matProfile->generateForCompositeMap(terrain);
		matProfile->setReceiveDynamicShadowsDepth(mDepthShadows);
		matProfile->setReceiveDynamicShadowsPSSM(static_cast<Ogre::PSSMShadowCameraSetup*>(mPSSMSetup.get()));
	}
}

void ShadowManager::setMaterialSplitPoints(Ogre::String materialName, Ogre::Vector4 &splitPoints)
{
	Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName(materialName);
	if (!mat.isNull())
	{
		unsigned short np = mat->getTechnique(0)->getNumPasses()-1;  // last
		try {
			mat->getTechnique(0)->getPass(np)->getFragmentProgramParameters()->setNamedConstant("pssmSplitPoints", splitPoints);
		} catch(...)
		{
			// this material is not prepared for PSSM usage !
		}
	}
}
