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
// created by thomas{AT}thomasfischer{DOT}biz, 5th of July 2010

#pragma once
#ifndef __ShadowManager_H_
#define __ShadowManager_H_

#include "RoRPrerequisites.h"

#include <OgreTerrain.h>
#include <OgreShadowCameraSetupPSSM.h>
#include <OgreTerrainMaterialGeneratorA.h>


enum {
	SHADOWS_NONE,
	SHADOWS_TEXTURE,
	SHADOWS_PSSM
};

//Store datas using structs
struct PSSM_Shadows_Data
{
	Ogre::ShadowCameraSetupPtr mPSSMSetup;
	bool mDepthShadows;
	Ogre::Vector4 splitPoints;
	int ShadowsTextureNum;
};

class ShadowManager : public ZeroedMemoryAllocator
{
public:

	ShadowManager();
	~ShadowManager();

	void loadConfiguration();

	void updatePSSM();

	void updateTerrainMaterial(Ogre::TerrainMaterialGeneratorA::SM2Profile* matProfile);
protected:

	void processTextureShadows();

	void processPSSM();
	void setManagedMaterialSplitPoints(Ogre::PSSMShadowCameraSetup::SplitPointList splitPointList);

	int updateShadowTechnique();

	int ShadowsType;

	PSSM_Shadows_Data PSSM_Shadows;
};


#endif // __ShadowManager_H_
