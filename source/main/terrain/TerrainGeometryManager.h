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

#include <OgreTerrain.h>
#include <OgreVector3.h>

/// this class handles all interactions with the Ogre Terrain system
class TerrainGeometryManager : public ZeroedMemoryAllocator, public IHeightFinder
{
public:
    TerrainGeometryManager(TerrainManager* terrainManager);
    ~TerrainGeometryManager();

    void loadOgreTerrainConfig(Ogre::String filename);

    Ogre::TerrainGroup* getTerrainGroup() { return m_ogre_terrain_group; };

    float getHeightAt(float x, float z);
    float getHeightAtPoint(long x, long z);
    float getHeightAtTerrainPosition(float x, float z);
    float getHeightAtWorldPosition(float x, float z);
    void getTerrainPositionAlign(Ogre::Real x, Ogre::Real y, Ogre::Real z, Ogre::Terrain::Alignment align, Ogre::Vector3* outWSpos);

    Ogre::Vector3 getNormalAt(float x, float y, float z, float precision = 0.1f);

    Ogre::Vector3 getMaxTerrainSize();

    bool update(float dt);
    void updateLightMap();

private:

    struct TerrnBlendLayerDef
    {
        std::string  blendmap_tex_filename;
        char         blend_mode;
        float        alpha_value;
    };

    bool getTerrainImage(int x, int y, Ogre::Image& img);
    bool loadTerrainConfig(Ogre::String filename);
    void configureTerrainDefaults();
    void defineTerrain(int x, int y, bool flat = false);
    void initBlendMaps(int x, int y, Ogre::Terrain* t);
    void initTerrain();
    void loadLayers(int x, int y, Ogre::Terrain* terrain = 0);
    Ogre::String getPageConfigFilename(int x, int z);
    Ogre::String getPageHeightmap(int x, int z);
    Ogre::DataStreamPtr getPageConfig(int x, int z);

    RoR::ConfigFile   m_terrain_config;
    std::string       m_terrn_base_name;               ///< Only for loading.
    std::string       m_pageconf_filename_format;      ///< Only for loading.
    bool              m_terrn_disable_caching;         ///< Only for loading.
    bool              m_was_new_geometry_generated;    ///< Only for loading.
    bool              m_terrain_is_flat;               ///< Only for loading.
    TerrainManager*   m_terrain_mgr;
    size_t            m_map_size_x;
    size_t            m_map_size_y;
    size_t            m_map_size_z;
    size_t            m_terrain_page_size;
    size_t            m_terrain_world_size;
    Ogre::TerrainGroup*  m_ogre_terrain_group;
    std::vector<TerrnBlendLayerDef>  m_terrn_blend_layers;

    // Terrn position lookup - ported from OGRE engine.
    Ogre::Terrain::Alignment mAlign;
    Ogre::Vector3 mPos;
    Ogre::Real mBase;
    Ogre::Real mScale;
    Ogre::uint16 mSize;
    float* mHeightData;

};

