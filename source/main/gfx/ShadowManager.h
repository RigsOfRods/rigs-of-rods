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

/// @file
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   5th of July 2010

#pragma once

#include <Terrain/OgreTerrain.h>
#include <OgreShadowCameraSetupPSSM.h>
#include <Terrain/OgreTerrainMaterialGeneratorA.h>

#include "Application.h"
#include "Application.h"

#include "OgreTerrainPSSMMaterialGenerator.h"

namespace RoR {

/// @addtogroup Gfx
/// @{

//Store datas using structs
struct PSSM_Shadows_Data
{
    Ogre::ShadowCameraSetupPtr mPSSMSetup;
    bool mDepthShadows;
    int ShadowsTextureNum;
    int Quality;
    float lambda;
};

class ShadowManager
{
public:

    ShadowManager();
    ~ShadowManager();

    void loadConfiguration();

    void updatePSSM();

    void updateTerrainMaterial(Ogre::TerrainPSSMMaterialGenerator::SM2Profile* matProfile);

protected:

    void processPSSM();
    void setManagedMaterialSplitPoints(Ogre::PSSMShadowCameraSetup::SplitPointList splitPointList);

    int updateShadowTechnique();

    PSSM_Shadows_Data PSSM_Shadows;
};

/// @} // addtogroup Gfx

} // namespace RoR
