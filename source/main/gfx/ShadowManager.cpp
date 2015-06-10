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

	if (ShadowsType == SHADOWS_TEXTURE)
	{
		gEnv->sceneManager->setShadowFarDistance(shadowFarDistance);
		processTextureShadows();
	}
	else if (ShadowsType == SHADOWS_PSSM)
	{
		processPSSM();
		
	}
	return 0;
}

void ShadowManager::processTextureShadows()
{

}

void ShadowManager::processPSSM()
{

}

void ShadowManager::updatePSSM(Ogre::Terrain* terrain)
{

}

void ShadowManager::setMaterialSplitPoints(Ogre::String materialName, Ogre::Vector4 &splitPoints)
{

}
