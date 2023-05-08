/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

#pragma once

#include "Application.h"
#include "ConfigFile.h"
#include "OTCFileFormat.h"

#include <OgreVector3.h>
#include <Terrain/OgreTerrain.h>
#include <Terrain/OgreTerrainGroup.h>

namespace RoR {

/// @addtogroup Terrain
/// @{

/// this class handles all interactions with the Ogre Terrain system
class TerrainGeometryManager
{
public:
    TerrainGeometryManager(Terrain* terrainManager);
    ~TerrainGeometryManager();

    bool InitTerrain(std::string otc_filename);

    Ogre::TerrainGroup* getTerrainGroup() { return m_ogre_terrain_group; };

    float getHeightAt(float x, float z);

    Ogre::Vector3 getNormalAt(float x, float y, float z);

    Ogre::Vector3 getMaxTerrainSize();

    bool isFlat() { return mIsFlat; };

    void UpdateMainLightPosition();
    void updateLightMap();

private:

    float getHeightAtTerrainPosition(float x, float z);

    bool getTerrainImage(int x, int y, Ogre::Image& img);
    bool loadTerrainConfig(Ogre::String filename);
    void configureTerrainDefaults();
    void SetupGeometry(RoR::OTCPage& page, bool flat=false);
    void SetupBlendMaps(RoR::OTCPage& page, Ogre::Terrain* t);
    void initTerrain();
    void SetupLayers(RoR::OTCPage& page, Ogre::Terrain *terrain);
    Ogre::DataStreamPtr getPageConfig(int x, int z);

    std::shared_ptr<RoR::OTCFile> m_spec;
    RoR::Terrain*      terrainManager;
    Ogre::TerrainGroup*  m_ogre_terrain_group;
    bool                 m_was_new_geometry_generated;

    // Terrn position lookup - ported from OGRE engine.
    Ogre::Vector3 mPos = Ogre::Vector3::ZERO;
    Ogre::Real mBase = 0.f;
    Ogre::Real mScale = 0.f;
    Ogre::uint16 mSize = 0;
    float* mHeightData = nullptr;

    bool  mIsFlat;
    float mMinHeight;
    float mMaxHeight;
};

/// @} // addtogroup Terrain

} // namespace RoR
