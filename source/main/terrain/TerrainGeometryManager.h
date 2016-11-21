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

#pragma once

#include "RoRPrerequisites.h"

#include "ConfigFile.h"
#include "IHeightFinder.h"

#include <OgreTerrain.h>
#include <OgreTerrainMaterialGeneratorA.h>
#include <OgreTerrainPaging.h>
#include <OgreTerrainQuadTreeNode.h>
#include <OgreTerrainGroup.h>

// this class handles all interactions with the Ogre Terrain system
class TerrainGeometryManager : public ZeroedMemoryAllocator, public IHeightFinder
{
public:
    TerrainGeometryManager(TerrainManager* terrainManager);
    ~TerrainGeometryManager();

    void loadOgreTerrainConfig(Ogre::String filename);

    Ogre::TerrainGroup* getTerrainGroup() { return mTerrainGroup; };

    float getHeightAt(float x, float z);
    float getHeightAtPoint(long x, long z);
    float getHeightAtTerrainPosition(float x, float z);
    float getHeightAtWorldPosition(float x, float z);
    void getTerrainPositionAlign(Ogre::Real x, Ogre::Real y, Ogre::Real z, Ogre::Terrain::Alignment align, Ogre::Vector3* outWSpos);

    Ogre::Vector3 getNormalAt(float x, float y, float z, float precision = 0.1f);

    Ogre::String getCompositeMaterialName();

    Ogre::Vector3 getMaxTerrainSize();
    Ogre::Vector3 getTerrainPosition();

    bool update(float dt);
    void updateLightMap();

protected:

    RoR::ConfigFile m_terrain_config;
    Ogre::String baseName;
    Ogre::String pageConfigFormat;
    TerrainManager* terrainManager;
    TerrainObjectManager* objectManager;
    bool disableCaching;
    bool mTerrainsImported;

    int mapsizex, mapsizey, mapsizez, pageSize, terrainSize, worldSize;
    int pageMinX, pageMaxX, pageMinZ, pageMaxZ;
    int terrainLayers;

    Ogre::Vector3 terrainPos;

    bool m_is_flat;
    Ogre::Terrain* mTerrain;

    Ogre::Terrain::Alignment mAlign;
    Ogre::Vector3 mPos;
    Ogre::Real mBase;
    Ogre::Real mScale;
    Ogre::uint16 mSize;
    Ogre::Real mWorldSize;
    float* mHeightData;

    // terrain engine specific
    Ogre::TerrainGroup* mTerrainGroup;
    Ogre::TerrainPaging* mTerrainPaging;
    Ogre::PageManager* mPageManager;

    typedef struct blendLayerInfo_t
    {
        Ogre::String blendMapTextureFilename;
        char blendMode;
        float alpha;
    } blendLayerInfo_t;

    std::vector<blendLayerInfo_t> blendInfo;

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
};

