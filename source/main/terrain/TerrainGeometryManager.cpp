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
#include "LoadingWindow.h"
#include "Language.h"

using namespace Ogre;

#define HASOPTION(X) !terrainConfig.getSetting(X).empty()
#define IOPTION(X)   PARSEINT(terrainConfig.getSetting(X))
#define BOPTION(X)   StringConverter::parseBool(terrainConfig.getSetting(X))
#define FOPTION(X)   PARSEREAL(terrainConfig.getSetting(X))
#define SOPTION(X)   terrainConfig.getSetting(X)
#define IOPT(X, Y)   (HASOPTION(X)?IOPTION(X):Y)
#define FOPT(X, Y)   (HASOPTION(X)?FOPTION(X):Y)
#define BOPT(X, Y)   (HASOPTION(X)?BOPTION(X):Y)
#define SOPT(X)      (SOPTION(X))

#define XZSTR(X,Z)   String("[") + TOSTRING(X) + String(",") + TOSTRING(Z) + String("]")

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

	disableCaching = BOPT("disableCaching", false);

	initTerrain();
}


bool TerrainGeometryManager::loadTerrainConfig(String filename)
{
	try
	{
		DataStreamPtr ds_config = ResourceGroupManager::getSingleton().openResource(filename, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
		terrainConfig.load(ds_config, "\t:=", false);
		return true;
	}catch(...)
	{
		terrainConfig.clear();
	}
	return false;
}

Ogre::String TerrainGeometryManager::getPageHeightmap(int x, int z)
{
	String cfg = pageConfigFormat;
	cfg = StringUtil::replaceAll(cfg, "{X}", TOSTRING(x));
	cfg = StringUtil::replaceAll(cfg, "{Z}", TOSTRING(z));
	try
	{
		LOG("loading page config for page " + XZSTR(x,z) + " : " + cfg);
		DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(cfg, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
		char buf[4096];
		ds->readLine(buf, 4096);
		return String(buf);
	}catch(...)
	{
	}
	return String();
}

void TerrainGeometryManager::initTerrain()
{
	// X, Y and Z scale
	mapsizex = IOPT("WorldSizeX", 1000);
	mapsizey = IOPT("WorldSizeY", 50);
	mapsizez = IOPT("WorldSizeZ", 1000);
	terrainSize = IOPT("PageSize", 1025);


	worldSize = std::max(mapsizex, mapsizez);

	String filenameSuffix = "mapbin";
	pageMinX = 0;
	pageMaxX = IOPT("PagesX", 0);
	pageMinZ = 0;
	pageMaxZ = IOPT("PagesZ", 0);

	pageConfigFormat = SOPT("PageFileFormat");

	bool is_flat = BOPT("Flat", false);

	terrainPos = Vector3(mapsizex / 2.0f, 0.0f, mapsizez / 2.0f);

	mTerrainGroup = OGRE_NEW TerrainGroup(gEnv->sceneManager, Terrain::ALIGN_X_Z, terrainSize, worldSize);
	mTerrainGroup->setFilenameConvention(baseName, filenameSuffix);
	mTerrainGroup->setOrigin(terrainPos);
	mTerrainGroup->setResourceGroup("cache");

	configureTerrainDefaults();

	String filename = mTerrainGroup->generateFilename(0, 0);

	for (long x = pageMinX; x <= pageMaxX; ++x)
	{
		for (long z = pageMinZ; z <= pageMaxZ; ++z)
		{
			LoadingWindow::getSingleton().setProgress(23, _L("preparing terrain page ") + XZSTR(x,z));
			defineTerrain(x, z, is_flat);
			LoadingWindow::getSingleton().setProgress(23, _L("loading terrain page ") + XZSTR(x,z));

			/*
			Terrain *terrain = mTerrainGroup->getTerrain(x, z);
			if(!terrain) continue;
			terrain->load();
			*/
		}
	}

	// sync load since we want everything in place when we start
	LoadingWindow::getSingleton().setProgress(23, _L("loading terrain pages"));
	mTerrainGroup->loadAllTerrains(true);


	// update the blend maps
	if (mTerrainsImported)
	{
		for (long x = pageMinX; x <= pageMaxX; ++x)
		{
			for (long z = pageMinZ; z <= pageMaxZ; ++z)
			{
				Terrain *terrain = mTerrainGroup->getTerrain(x, z);
				if(!terrain) continue;
				//ShadowManager::getSingleton().updatePSSM(terrain);
				LoadingWindow::getSingleton().setProgress(23, _L("loading terrain page layers ") + XZSTR(x,z));
				loadLayers(x, z, terrain);
				LoadingWindow::getSingleton().setProgress(23, _L("loading terrain page blend maps ") + XZSTR(x,z));
				initBlendMaps(x, z, terrain);


			}
		}
		
		// always save the results when it was imported
		if(!disableCaching)
		{
			LoadingWindow::getSingleton().setProgress(23, _L("saving all terrain pages ..."));
			mTerrainGroup->saveAllTerrains(false);
		}
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
		if(!terrain) continue;
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
	terrainOptions->setMaxPixelError(IOPT("MaxPixelError", 5));

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
	defaultimp.minBatchSize = IOPT("minBatchSize", 33);
	defaultimp.maxBatchSize = IOPT("maxBatchSize", 65);

	// optimizations
	TerrainMaterialGeneratorA::SM2Profile* matProfile = static_cast<TerrainMaterialGeneratorA::SM2Profile*>(terrainOptions->getDefaultMaterialGenerator()->getActiveProfile());
	if(matProfile)
	{
		matProfile->setLightmapEnabled(BOPT("LightmapEnabled", false));
		matProfile->setLayerNormalMappingEnabled(BOPT("NormalMappingEnabled", false));
		matProfile->setLayerSpecularMappingEnabled(BOPT("SpecularMappingEnabled", false));
		matProfile->setLayerParallaxMappingEnabled(BOPT("ParallaxMappingEnabled", false));
		matProfile->setGlobalColourMapEnabled(BOPT("GlobalColourMapEnabled", false));
		matProfile->setReceiveDynamicShadowsDepth(BOPT("ReceiveDynamicShadowsDepth", false));
	}

	terrainOptions->setLayerBlendMapSize(IOPT("LayerBlendMapSize", 1024));
	terrainOptions->setCompositeMapSize(IOPT("CompositeMapSize", 1024));
	terrainOptions->setCompositeMapDistance(IOPT("CompositeMapDistance", 4000));
	terrainOptions->setSkirtSize(IOPT("SkirtSize", 30));
	terrainOptions->setLightMapSize(IOPT("LightMapSize", 1024));
	terrainOptions->setCastsDynamicShadows(BOPT("CastsDynamicShadows", false));

	terrainOptions->setUseRayBoxDistanceCalculation(false);

	// load layer basics now
	loadLayers(0, 0, 0);
}

// if terrain is set, we operate on the already loaded terrain
void TerrainGeometryManager::loadLayers(int x, int z, Terrain *terrain)
{
	String cfg = pageConfigFormat;
	cfg = StringUtil::replaceAll(cfg, "{X}", TOSTRING(x));
	cfg = StringUtil::replaceAll(cfg, "{Z}", TOSTRING(z));
	DataStreamPtr ds;
	try
	{
		LOG("loading page config for page " + XZSTR(x,z) + " : " + cfg);
		ds = ResourceGroupManager::getSingleton().openResource(cfg, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
	}catch(...)
	{
	}
	if(!ds->isReadable())
	{
		LOG("error loading layers for page " + XZSTR(x,z));
		return;
	}

	char line[4096];
	ds->readLine(line, 4096);
	String heightmapImage = String(line);
	ds->readLine(line, 4096);
	terrainLayers = PARSEINT(String(line));

	Ogre::Terrain::ImportData &defaultimp = mTerrainGroup->getDefaultImportSettings();
	if (terrainLayers == 0)
		return;
	if(!terrain)
		defaultimp.layerList.resize(terrainLayers);
	blendInfo.clear();
	blendInfo.resize(terrainLayers);

	int layer = 0;
	while (!ds->eof())
	{
		size_t ll = ds->readLine(line, 4096);
		if (ll==0 || line[0]=='/' || line[0]==';') continue;

		StringVector args = StringUtil::split(String(line), ",");
		if(args.size() < 3)
		{
			LOG("invalid page config line: '" + String(line) + "'");
			continue;
		}

		StringUtil::trim(args[1]);
		StringUtil::trim(args[2]);

		float worldSize = PARSEREAL(args[0]);
		if(!terrain)
		{
			defaultimp.layerList[layer].worldSize = worldSize;
			defaultimp.layerList[layer].textureNames.push_back(args[1]);
			defaultimp.layerList[layer].textureNames.push_back(args[2]);
		} else
		{
			terrain->setLayerWorldSize(layer, worldSize);
			terrain->setLayerTextureName(layer, 0, args[1]);
			terrain->setLayerTextureName(layer, 1, args[2]);
		}
			
		blendLayerInfo_t &bi = blendInfo[layer];
		bi.blendMode = 'R';
		bi.alpha = 'R';

		if(args.size() > 3)
		{
			StringUtil::trim(args[3]);
			bi.blendMapTextureFilename = args[3];
		}
		if(args.size() > 4)
		{
			StringUtil::trim(args[4]);
			bi.blendMode = args[4][0];
		}
		if(args.size() > 5)
			bi.alpha = PARSEREAL(args[5]);

		layer++;
		if(layer >= terrainLayers)
			break;
	}
	LOG("done loading page: loaded " + TOSTRING(layer) + " layers");
}

void TerrainGeometryManager::initBlendMaps(int x, int z, Ogre::Terrain* terrain )
{
	bool debugBlendMaps = BOPT("DebugBlendMaps", false);

	int layerCount = terrain->getLayerCount();
	for (int i = 1; i < layerCount; i++)
	{
		blendLayerInfo_t &bi = blendInfo[i];

		if(bi.blendMapTextureFilename.empty()) continue;

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
			for (Ogre::uint32 z = 0; z != blendmapSize; z++)
			{
				Ogre::ColourValue c = img.getColourAt(x, z, 0);
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
				for (unsigned int z = 0; z < blendmapSize; z++)
					idata[x + z * blendmapSize] = (unsigned short)(blendMap->getBlendValue(x, blendmapSize - z) * scale);
			img.loadDynamicImage((Ogre::uchar*)(idata), blendmapSize, blendmapSize, Ogre::PF_L16);
			std::string fileName = "blendmap_layer_" + Ogre::StringConverter::toString(i) + ".png";
			img.save(fileName);
			OGRE_FREE(idata, Ogre::MEMCATEGORY_RESOURCE);
		}
	}
}

bool TerrainGeometryManager::getTerrainImage(int x, int z, Image& img)
{
	String heightmapString = "HeightmapImage";
	String heightmapFilename = getPageHeightmap(x, z);
	StringUtil::trim(heightmapFilename);

	if(heightmapFilename.empty())
	{
		LOG("empty Heightmap provided, please use 'Flat=1' instead");
		return false;
	}


	if (heightmapFilename.find(".raw") != String::npos)
	{
		int rawSize = IOPT(heightmapString+".raw.size", 1025);
		int bpp     = IOPT(heightmapString+".raw.bpp", 2);

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

	if(BOPT(heightmapString + ".flipX", false))
		img.flipAroundX();
	if(BOPT(heightmapString + ".flipY", false))
		img.flipAroundY();

	return true;
}

void TerrainGeometryManager::defineTerrain( int x, int z, bool flat )
{
	if (flat)
	{
		// very simple, no height data to load at all
		mTerrainGroup->defineTerrain(x, z, 0.0f);
		return;
	}
	String filename = mTerrainGroup->generateFilename(x, z);
	if (!disableCaching && ResourceGroupManager::getSingleton().resourceExists(mTerrainGroup->getResourceGroup(), filename))
	{
		// load from cache
		mTerrainGroup->defineTerrain(x, z);
	}
	else
	{
		Image img;
		if(getTerrainImage(x, z, img))
		{
			mTerrainGroup->defineTerrain(x, z, &img);
			mTerrainsImported = true;
		} else
		{
			// fall back to no heightmap
			mTerrainGroup->defineTerrain(x, z, 0.0f);
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
		if(!terrain) continue;
		MaterialPtr mat = terrain->getCompositeMapMaterial();
		if(!mat.isNull())
			return mat->getName();
	}
	return String();
}

