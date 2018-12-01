/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#include "RoRPrerequisites.h"
#include "ConfigFile.h"
#include "OTCFileformat.h"

#include <OgreVector3.h>
#include <Terrain/OgreTerrain.h>

// forward
namespace Ogre { class Terrain; class TerrainGroup; class TerrainPaging; class PageManager; }

/// this class handles all interactions with the Ogre Terrain system
class TerrainGeometryManager : public ZeroedMemoryAllocator
{
public:
    TerrainGeometryManager(TerrainManager* terrainManager);
    ~TerrainGeometryManager();

    void InitTerrain(std::string otc_filename);

    Ogre::TerrainGroup* getTerrainGroup() { return m_ogre_terrain_group; };

    float getHeightAt(float x, float z);
    float getHeightAtPoint(long x, long z);
    float getHeightAtTerrainPosition(float x, float z);
    float getHeightAtWorldPosition(float x, float z);
    void getTerrainPositionAlign(Ogre::Real x, Ogre::Real y, Ogre::Real z, Ogre::Terrain::Alignment align, Ogre::Vector3* outWSpos);

    Ogre::Vector3 getNormalAt(float x, float y, float z, float precision = 0.1f);

    Ogre::Vector3 getMaxTerrainSize();

    void UpdateMainLightPosition();
    void updateLightMap();

private:

    bool getTerrainImage(int x, int y, Ogre::Image& img);
    bool loadTerrainConfig(Ogre::String filename);
    void configureTerrainDefaults();
    void SetupGeometry(RoR::OTCPage& page, bool flat=false);
    void SetupBlendMaps(RoR::OTCPage& page, Ogre::Terrain* t);
    void initTerrain();
    void SetupLayers(RoR::OTCPage& page, Ogre::Terrain *terrain);
    Ogre::DataStreamPtr getPageConfig(int x, int z);

    std::shared_ptr<RoR::OTCFile> m_spec;
    TerrainManager*      terrainManager;
    Ogre::TerrainGroup*  m_ogre_terrain_group;
    bool                 m_was_new_geometry_generated;

    // Terrn position lookup - ported from OGRE engine.
    Ogre::Terrain::Alignment mAlign;
    Ogre::Vector3 mPos;
    Ogre::Real mBase;
    Ogre::Real mScale;
    Ogre::uint16 mSize;
    float* mHeightData;

};

