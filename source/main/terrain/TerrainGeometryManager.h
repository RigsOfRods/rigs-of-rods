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
#ifndef __TerrainGeometryManager_H_
#define __TerrainGeometryManager_H_

#include "RoRPrerequisites.h"

#include "IHeightFinder.h"
#include "IManager.h"

#include <OgreTerrain.h>
#include <OgreTerrainMaterialGeneratorA.h>
#include <OgreTerrainPaging.h>
#include <OgreTerrainQuadTreeNode.h>
#include <OgreTerrainGroup.h>

#include <OgreConfigFile.h>

// this class handles all interactions with the Ogre Terrain system
class TerrainGeometryManager : public IManager, public IHeightFinder
{
public:

	TerrainGeometryManager(TerrainManager *terrainManager);
	~TerrainGeometryManager();

	void loadOgreTerrainConfig(Ogre::String filename);

	inline Ogre::TerrainGroup *getTerrainGroup() { return mTerrainGroup; };


	inline float getHeightAt(float x, float z)
	{
		return mTerrainGroup->getHeightAtWorldPosition(x, 1000, z);
	}

	inline Ogre::Vector3 getNormalAt(float x, float y, float z, float precision = 0.1f)
	{
		Ogre::Vector3 left(-precision, getHeightAt(x - precision, z) - y, 0.0f);
		Ogre::Vector3 down(0.0f, getHeightAt(x, z + precision) - y, precision);
		down = left.crossProduct(down);
		down.normalise();
		return down;
	}

	Ogre::String getCompositeMaterialName();

	Ogre::Vector3 getMaxTerrainSize();
	Ogre::Vector3 getTerrainPosition();

	bool update(float dt);
	void updateLightMap();


	size_t getMemoryUsage();
	void freeResources();

protected:

	Ogre::ConfigFile terrainConfig;
	Ogre::String baseName;
	Ogre::String pageConfigFormat;
	TerrainManager *terrainManager;
	TerrainObjectManager *objectManager;
	bool disableCaching;
	bool mTerrainsImported;

	int mapsizex, mapsizey, mapsizez, pageSize, terrainSize, worldSize;
	int pageMinX, pageMaxX, pageMinZ, pageMaxZ;
	int terrainLayers;

	Ogre::Vector3 terrainPos;

	// terrain engine specific
	Ogre::TerrainGroup *mTerrainGroup;
	Ogre::TerrainPaging* mTerrainPaging;
	Ogre::PageManager* mPageManager;

	typedef struct blendLayerInfo_t {
		Ogre::String blendMapTextureFilename;
		char blendMode;
		float alpha;
	} blendLayerInfo_t;

	std::vector<blendLayerInfo_t> blendInfo;

	bool getTerrainImage(int x, int y, Ogre::Image& img);
	bool loadTerrainConfig(Ogre::String filename);
	void configureTerrainDefaults();
	void defineTerrain(int x, int y, bool flat=false);
	void initBlendMaps(int x, int y, Ogre::Terrain* t );
	void initTerrain();
	void loadLayers(int x, int y, Ogre::Terrain *terrain = 0);
	Ogre::String getPageConfigFilename(int x, int z);
	Ogre::String getPageHeightmap(int x, int z);
	Ogre::DataStreamPtr getPageConfig(int x, int z);
};

#endif // __TerrainGeometryManager_H_
