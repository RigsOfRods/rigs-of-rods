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
#include "IHeightFinder.h"
#include "OTCFileformat.h"

#include <OgreVector3.h>
#include <OgreTerrain.h>

// forward
namespace Ogre { class Terrain; class TerrainGroup; class TerrainPaging; class PageManager; }
namespace Json { class Value; }

/// this class handles all interactions with the Ogre Terrain system
class TerrainGeometryManager : public ZeroedMemoryAllocator, public IHeightFinder
{
public:
    TerrainGeometryManager(TerrainManager* terrainManager);
    ~TerrainGeometryManager();

    void InitTerrain(Json::Value* j_terrn);

    Ogre::TerrainGroup* getTerrainGroup() { return m_ogre_terrain_group; };

    float getHeightAt(float x, float z);
    float getHeightAtPoint(long x, long z);
    float getHeightAtTerrainPosition(float x, float z);
    float getHeightAtWorldPosition(float x, float z);
    void getTerrainPositionAlign(Ogre::Real x, Ogre::Real y, Ogre::Real z, Ogre::Terrain::Alignment align, Ogre::Vector3* outWSpos);

    Ogre::Vector3 getNormalAt(float x, float y, float z, float precision = 0.1f);

    inline Ogre::Vector3 getMaxTerrainSize() const { return m_world_size; }

    bool update(float dt);
    void updateLightMap();

private:

    bool getTerrainImage(int x, int y, Ogre::Image& img);
    bool loadTerrainConfig(Ogre::String filename);
    void ConfigureTerrainDefaults(Json::Value* j_otc);
    void SetupGeometry(Json::Value* j_page, bool disable_cache);
    void SetupTerrnBlendMaps(Json::Value* j_page, Ogre::Terrain* t, bool enable_debug);
    void initTerrain();
    void SetupLayers(Json::Value* j_page, Ogre::Terrain *terrain);
    Ogre::DataStreamPtr getPageConfig(int x, int z);

    TerrainManager*      m_terrain_mgr;
    Ogre::TerrainGroup*  m_ogre_terrain_group;
    bool                 m_was_new_geometry_generated;
    bool                 m_terrain_is_flat;
    Ogre::Vector3        m_world_size;

    // Terrn position lookup - ported from OGRE engine.
    Ogre::Terrain::Alignment mAlign;
    Ogre::Vector3 mPos;
    Ogre::Real mBase;
    Ogre::Real mScale;
    Ogre::uint16 mSize;
    float* mHeightData;

};

