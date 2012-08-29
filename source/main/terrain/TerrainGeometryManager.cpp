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
#include "TerrainGeometryManager.h"

#include "SkyManager.h"
#include "TerrainManager.h"

using namespace Ogre;

TerrainGeometryManager::TerrainGeometryManager(TerrainManager *terrainManager) :
	  terrainManager(terrainManager)
	, disableCaching(false)
	, mTerrainsImported(false)
{
}

TerrainGeometryManager::~TerrainGeometryManager()
{

}

void TerrainGeometryManager::loadOgreTerrainConfig(String filename)
{
	String ext;
	Ogre::StringUtil::splitBaseFilename(filename, baseName, ext);

	loadTerrainConfig(filename);

	if (!terrainConfig.getSetting("disableCaching").empty())
		disableCaching = StringConverter::parseBool(terrainConfig.getSetting("disableCaching"));

	initTerrain();
}


bool TerrainGeometryManager::loadTerrainConfig(String filename)
{
	try
	{
		DataStreamPtr ds_config = ResourceGroupManager::getSingleton().openResource(filename, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
		terrainConfig.load(ds_config, "\t:=", false);
	}catch(...)
	{
		terrainConfig.clear();
	}
	return true;
}

void TerrainGeometryManager::initTerrain()
{
	// X, Y and Z scale
	mapsizex = PARSEINT(terrainConfig.getSetting("WorldSizeX"));
	mapsizey = PARSEINT(terrainConfig.getSetting("WorldSizeY"));
	mapsizez = PARSEINT(terrainConfig.getSetting("WorldSizeZ"));
	terrainSize = mapsizex+1;
	worldSize = std::max(mapsizex, mapsizez);

	String filenameSuffix = "mapbin";
	pageMinX = 0;
	pageMaxX = 0;
	pageMinY = 0;
	pageMaxY = 0;

	pageMaxX = PARSEINT(terrainConfig.getSetting("Pages_X"));
	pageMaxY = PARSEINT(terrainConfig.getSetting("Pages_Y"));

	bool is_flat = Ogre::StringConverter::parseBool(terrainConfig.getSetting("Flat"));

	terrainPos = Vector3(mapsizex / 2.0f, 0.0f, mapsizez / 2.0f);

	mTerrainGroup = OGRE_NEW TerrainGroup(gEnv->sceneManager, Terrain::ALIGN_X_Z, terrainSize, worldSize);
	mTerrainGroup->setFilenameConvention(baseName, filenameSuffix);
	mTerrainGroup->setOrigin(terrainPos);
	mTerrainGroup->setResourceGroup("cache");

	configureTerrainDefaults();

	String filename = mTerrainGroup->generateFilename(0, 0);
	for (long x = pageMinX; x <= pageMaxX; ++x)
		for (long y = pageMinY; y <= pageMaxY; ++y)
			defineTerrain(x, y, is_flat);

	// sync load since we want everything in place when we start
	mTerrainGroup->loadAllTerrains(true);


	// update the blend maps
	if (mTerrainsImported)
	{
		TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();
		while(ti.hasMoreElements())
		{
			Terrain *terrain = ti.getNext()->instance;
			//ShadowManager::getSingleton().updatePSSM(terrain);
			initBlendMaps(terrain);
		}

		// always save the results when it was imported
		mTerrainGroup->saveAllTerrains(false);
	} else
	{
		LOG(" *** Terrain loaded from cache ***");
	}

	mTerrainGroup->freeTemporaryResources();
}

void TerrainGeometryManager::updateLightMap()
{
	TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();
	while(ti.hasMoreElements())
	{
		Terrain *terrain = ti.getNext()->instance;
		//ShadowManager::getSingleton().updatePSSM(terrain);
		if(!terrain->isDerivedDataUpdateInProgress())
		{
			terrain->dirtyLightmap();
			terrain->updateDerivedData();
		}
	}
}

bool TerrainGeometryManager::update(float dt)
{
	Light *light = gEnv->terrainManager->getMainLight();
	TerrainGlobalOptions *terrainOptions = TerrainGlobalOptions::getSingletonPtr();
	if (light)
	{
		terrainOptions->setLightMapDirection(light->getDerivedDirection());
		terrainOptions->setCompositeMapDiffuse(light->getDiffuseColour());
	}
	terrainOptions->setCompositeMapAmbient(gEnv->sceneManager->getAmbientLight());

	mTerrainGroup->update();
	return true;
}

void TerrainGeometryManager::configureTerrainDefaults()
{
	OGRE_NEW TerrainGlobalOptions();

	Light *light = gEnv->terrainManager->getMainLight();
	TerrainGlobalOptions *terrainOptions = TerrainGlobalOptions::getSingletonPtr();
	// Configure global
	terrainOptions->setMaxPixelError(PARSEINT(terrainConfig.getSetting("MaxPixelError")));

	// Important to set these so that the terrain knows what to use for derived (non-realtime) data
	if (light)
	{
		terrainOptions->setLightMapDirection(light->getDerivedDirection());
		terrainOptions->setCompositeMapDiffuse(light->getDiffuseColour());
	}
	terrainOptions->setCompositeMapAmbient(gEnv->sceneManager->getAmbientLight());

	// Configure default import settings for if we use imported image
	Ogre::Terrain::ImportData& defaultimp = mTerrainGroup->getDefaultImportSettings();
	defaultimp.terrainSize  = terrainSize; // the heightmap size
	defaultimp.worldSize    = worldSize; // this is the scaled up size, like 12km
	defaultimp.inputScale   = mapsizey;
	defaultimp.minBatchSize = 33;
	defaultimp.maxBatchSize = 65;

	if (!terrainConfig.getSetting("minBatchSize").empty())
		defaultimp.minBatchSize = PARSEINT(terrainConfig.getSetting("minBatchSize"));

	if (!terrainConfig.getSetting("maxBatchSize").empty())
		defaultimp.maxBatchSize = PARSEINT(terrainConfig.getSetting("maxBatchSize"));

	// optimizations
	TerrainMaterialGeneratorA::SM2Profile* matProfile = static_cast<TerrainMaterialGeneratorA::SM2Profile*>(terrainOptions->getDefaultMaterialGenerator()->getActiveProfile());
	matProfile->setLightmapEnabled(StringConverter::parseBool(terrainConfig.getSetting("LightmapEnabled")));
	matProfile->setLayerNormalMappingEnabled(StringConverter::parseBool(terrainConfig.getSetting("NormalMappingEnabled")));
	matProfile->setLayerSpecularMappingEnabled(StringConverter::parseBool(terrainConfig.getSetting("SpecularMappingEnabled")));
	matProfile->setLayerParallaxMappingEnabled(StringConverter::parseBool(terrainConfig.getSetting("ParallaxMappingEnabled")));
	matProfile->setGlobalColourMapEnabled(StringConverter::parseBool(terrainConfig.getSetting("GlobalColourMapEnabled")));
	matProfile->setReceiveDynamicShadowsDepth(StringConverter::parseBool(terrainConfig.getSetting("ReceiveDynamicShadowsDepth")));

	terrainOptions->setLayerBlendMapSize(PARSEINT(terrainConfig.getSetting("LayerBlendMapSize")));
	terrainOptions->setCompositeMapSize(PARSEINT(terrainConfig.getSetting("CompositeMapSize")));
	terrainOptions->setCompositeMapDistance(PARSEINT(terrainConfig.getSetting("CompositeMapDistance")));
	terrainOptions->setSkirtSize(PARSEINT(terrainConfig.getSetting("SkirtSize")));
	terrainOptions->setLightMapSize(PARSEINT(terrainConfig.getSetting("LightMapSize")));
	terrainOptions->setCastsDynamicShadows(StringConverter::parseBool(terrainConfig.getSetting("CastsDynamicShadows")));

	terrainOptions->setUseRayBoxDistanceCalculation(false);

	// load the textures and blendmaps into our data structures
	blendInfo.clear();
	terrainLayers = StringConverter::parseInt(terrainConfig.getSetting("Layers.count"));
	if (terrainLayers > 0)
	{
		defaultimp.layerList.resize(terrainLayers);
		blendInfo.resize(terrainLayers);
		for (int i = 0; i < terrainLayers; i++)
		{
			defaultimp.layerList[i].worldSize = PARSEINT(terrainConfig.getSetting("Layers."+TOSTRING(i)+".size"));
			defaultimp.layerList[i].textureNames.push_back(terrainConfig.getSetting("Layers."+TOSTRING(i)+".diffusespecular"));
			defaultimp.layerList[i].textureNames.push_back(terrainConfig.getSetting("Layers."+TOSTRING(i)+".normalheight"));

			blendLayerInfo_t &bi = blendInfo[i];
			bi.blendMapTextureFilename = terrainConfig.getSetting("Layers."+TOSTRING(i)+".blendmap");
			bi.blendMode = *terrainConfig.getSetting("Layers."+TOSTRING(i)+".blendmapmode").c_str();
			bi.alpha = Ogre::StringConverter::parseReal(terrainConfig.getSetting("Layers."+TOSTRING(i)+".alpha"));
		}
	}
}

void TerrainGeometryManager::initBlendMaps( Ogre::Terrain* terrain )
{
	bool debugBlendMaps = StringConverter::parseBool(terrainConfig.getSetting("DebugBlendMaps"));

	int layerCount = terrain->getLayerCount();
	for (int i = 1; i < layerCount; i++)
	{
		blendLayerInfo_t &bi = blendInfo[i];
		Ogre::Image img;
		//std::pair<uint8,uint8> textureIndex = terrain->getLayerBlendTextureIndex(i);
		//uint8 bti = terrain->getBlendTextureIndex(i);
		try
		{
			img.load(bi.blendMapTextureFilename, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
		} catch(Exception &e)
		{
			LOG("Error loading blendmap: " + bi.blendMapTextureFilename + " : " + e.getFullDescription());
			continue;
		}

		TerrainLayerBlendMap *blendmap = terrain->getLayerBlendMap(i);

		// resize that blending map so it will fit
		Ogre::uint32 blendmapSize = terrain->getLayerBlendMapSize();
		if (img.getWidth() != blendmapSize)
			img.resize(blendmapSize, blendmapSize);

		// now to the ugly part
		float* ptr = blendmap->getBlendPointer();
		for (Ogre::uint32 x = 0; x != blendmapSize; x++)
		{
			for (Ogre::uint32 y = 0; y != blendmapSize; y++)
			{
				Ogre::ColourValue c = img.getColourAt(x, y, 0);
				float alpha = bi.alpha;
				if      (bi.blendMode == 'R')
					*ptr++ = c.r * alpha;
				else if (bi.blendMode == 'G')
					*ptr++ = c.g * alpha;
				else if (bi.blendMode == 'B')
					*ptr++ = c.b * alpha;
				else if (bi.blendMode == 'A')
					*ptr++ = c.a * alpha;
			}
		}
		blendmap->dirty();
		blendmap->update();
	}

	if (debugBlendMaps)
	{
		for (int i = 1; i < layerCount; i++)
		{
			Ogre::TerrainLayerBlendMap* blendMap = terrain->getLayerBlendMap(i);
			Ogre::uint32 blendmapSize = terrain->getLayerBlendMapSize();
			Ogre::Image img;
			unsigned short *idata = OGRE_ALLOC_T(unsigned short, blendmapSize * blendmapSize, Ogre::MEMCATEGORY_RESOURCE);
			float scale = 65535.0f;
			for (unsigned int x = 0; x < blendmapSize; x++)
				for (unsigned int y = 0; y < blendmapSize; y++)
					idata[x + y * blendmapSize] = (unsigned short)(blendMap->getBlendValue(x, blendmapSize - y) * scale);
			img.loadDynamicImage((Ogre::uchar*)(idata), blendmapSize, blendmapSize, Ogre::PF_L16);
			std::string fileName = "blendmap_layer_" + Ogre::StringConverter::toString(i) + ".png";
			img.save(fileName);
			OGRE_FREE(idata, Ogre::MEMCATEGORY_RESOURCE);
		}
	}
}

bool TerrainGeometryManager::getTerrainImage(int x, int y, Image& img)
{
	// create new from image
	String heightmapString = "HeightmapImage." + TOSTRING(x) + "." + TOSTRING(y);
	String heightmapFilename = terrainConfig.getSetting(heightmapString);

	if(heightmapFilename.empty())
	{
		LOG("empty Heightmap provided, please use 'Flat=1' instead");
		return false;
	}

	if (heightmapFilename.find(".raw") != String::npos)
	{
		int rawSize = StringConverter::parseInt(terrainConfig.getSetting("Heightmap.raw.size"));
		int bpp = StringConverter::parseInt(terrainConfig.getSetting("Heightmap.raw.bpp"));

		// load raw data
		DataStreamPtr stream = ResourceGroupManager::getSingleton().openResource(heightmapFilename);
		LOG(" loading RAW image: " + TOSTRING(stream->size()) + " / " + TOSTRING(rawSize*rawSize*bpp));
		PixelFormat pformat = PF_L8;
		if (bpp == 2)
			pformat = PF_L16;
		img.loadRawData(stream, rawSize, rawSize, 1, pformat);
	} else
	{
		img.load(heightmapFilename, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	}

	if (!terrainConfig.getSetting("Heightmap.flip").empty()  && StringConverter::parseBool(terrainConfig.getSetting("Heightmap.flip")))
		img.flipAroundX();

	//if (flipX)
	//	img.flipAroundY();
	//if (flipY)
	//	img.flipAroundX();
	return true;
}

void TerrainGeometryManager::defineTerrain( int x, int y, bool flat )
{
	if (flat)
	{
		// very simple, no height data to load at all
		mTerrainGroup->defineTerrain(x, y, 0.0f);
		return;
	}
	String filename = mTerrainGroup->generateFilename(x, y);
	if (!disableCaching && ResourceGroupManager::getSingleton().resourceExists(mTerrainGroup->getResourceGroup(), filename))
	{
		// load from cache
		mTerrainGroup->defineTerrain(x, y);
	}
	else
	{
		Image img;
		if(getTerrainImage(x, y, img))
		{
			mTerrainGroup->defineTerrain(x, y, &img);
			mTerrainsImported = true;
		} else
		{
			// fall back to no heightmap
			mTerrainGroup->defineTerrain(x, y, 0.0f);
		}
	}
}

Ogre::Vector3 TerrainGeometryManager::getMaxTerrainSize()
{
	return Vector3(mapsizex, mapsizey, mapsizez);
}

Ogre::Vector3 TerrainGeometryManager::getTerrainPosition()
{
	return terrainPos;
}

size_t TerrainGeometryManager::getMemoryUsage()
{
	// TODO
	return 0;
}

void TerrainGeometryManager::freeResources()
{
	// TODO
}

Ogre::String TerrainGeometryManager::getCompositeMaterialName()
{
	TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();
	while(ti.hasMoreElements())
	{
		Terrain *terrain = ti.getNext()->instance;
		MaterialPtr mat = terrain->getCompositeMapMaterial();
		if(!mat.isNull())
			return mat->getName();
	}
	return String();
}

