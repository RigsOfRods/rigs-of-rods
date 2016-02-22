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

#include "Language.h"
#include "LoadingWindow.h"
#include "TerrainManager.h"
#include "ShadowManager.h"
#include "OgreTerrainPSSMMaterialGenerator.h"
#include "Utils.h"

using namespace Ogre;

#define XZSTR(X,Z)   String("[") + TOSTRING(X) + String(",") + TOSTRING(Z) + String("]")

TerrainGeometryManager::TerrainGeometryManager(TerrainManager *terrainManager) :
	  terrainManager(terrainManager)
	, disableCaching(false)
	, mTerrainsImported(false)
{
}

TerrainGeometryManager::~TerrainGeometryManager()
{
	mTerrainGroup->removeAllTerrains();
}

void TerrainGeometryManager::loadOgreTerrainConfig(String filename)
{
	String ext;
	Ogre::StringUtil::splitBaseFilename(filename, baseName, ext);

	loadTerrainConfig(filename);

	disableCaching = m_terrain_config.GetBool("disableCaching", false);

	initTerrain();
}

bool TerrainGeometryManager::loadTerrainConfig(String filename)
{
	try
	{
		DataStreamPtr ds_config = ResourceGroupManager::getSingleton().openResource(filename, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
		m_terrain_config.load(ds_config, "\t:=", false);
		return true;
	} catch(...)
	{
		m_terrain_config.clear();
	}
	return false;
}

Ogre::String TerrainGeometryManager::getPageConfigFilename(int x, int z)
{
	String cfg = pageConfigFormat;

	cfg = StringUtil::replaceAll(cfg, "{X}", TOSTRING(x));
	cfg = StringUtil::replaceAll(cfg, "{Z}", TOSTRING(z));

	return cfg;
}

Ogre::DataStreamPtr TerrainGeometryManager::getPageConfig(int x, int z)
{
	String cfg = getPageConfigFilename(x, z);

	try
	{
		LOG("loading page config for page " + XZSTR(x,z) + " : " + cfg);
		DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(cfg, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

		if (!ds.isNull() && ds->isReadable())
		{
			return ds;
		}
	} catch (...)
	{

	}

	LOG("error loading page config for page " + XZSTR(x,z) + " : " + cfg);

	if (x != 0 || z != 0)
	{
		LOG("loading default page config: " + cfg + " instead");
		return getPageConfig(0, 0);
	}

	return DataStreamPtr();
}

Ogre::String TerrainGeometryManager::getPageHeightmap(int x, int z)
{
	DataStreamPtr ds = getPageConfig(x, z);

	if (ds.isNull())
		return "";

	char buf[4096];
	ds->readLine(buf, 4096);
	return RoR::Utils::SanitizeUtf8String(String(buf));
}

void TerrainGeometryManager::initTerrain()
{
	// X, Y and Z scale
	mapsizex    = m_terrain_config.GetInt("WorldSizeX", 1024);
	mapsizey    = m_terrain_config.GetInt("WorldSizeY", 50);
	mapsizez    = m_terrain_config.GetInt("WorldSizeZ", 1024);
	terrainSize = m_terrain_config.GetInt("PageSize", 1025);

	worldSize = std::max(mapsizex, mapsizez);

	pageMinX = 0;
	pageMaxX = m_terrain_config.GetInt("PagesX", 0);
	pageMinZ = 0;
	pageMaxZ = m_terrain_config.GetInt("PagesZ", 0);

	pageConfigFormat = m_terrain_config.GetString("PageFileFormat", baseName + "-page-{X}-{Z}.otc");

	bool is_flat = m_terrain_config.GetBool("Flat", false);

	terrainPos = Vector3(mapsizex / 2.0f, 0.0f, mapsizez / 2.0f);

	String Filename = baseName + "_OGRE_" + TOSTRING(OGRE_VERSION) + "_";

	mTerrainGroup = OGRE_NEW TerrainGroup(gEnv->sceneManager, Terrain::ALIGN_X_Z, terrainSize, worldSize);
	mTerrainGroup->setFilenameConvention(Filename, "mapbin");
	mTerrainGroup->setOrigin(terrainPos);
	mTerrainGroup->setResourceGroup("cache");

	configureTerrainDefaults();

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
				if (!terrain) continue;
				//ShadowManager::getSingleton().updatePSSM(terrain);
				LoadingWindow::getSingleton().setProgress(23, _L("loading terrain page layers ") + XZSTR(x,z));
				loadLayers(x, z, terrain);
				LoadingWindow::getSingleton().setProgress(23, _L("loading terrain page blend maps ") + XZSTR(x,z));
				initBlendMaps(x, z, terrain);
			}
		}
		
#ifdef _WIN32
		// always save the results when it was imported
		if (!disableCaching)
		{
			LoadingWindow::getSingleton().setProgress(23, _L("saving all terrain pages ..."));
			mTerrainGroup->saveAllTerrains(false);
		}
#endif // _WIN32
	} else
	{
		LOG(" *** Terrain loaded from cache ***");
	}

	mTerrainGroup->freeTemporaryResources();
}

void TerrainGeometryManager::updateLightMap()
{
	TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();

	while (ti.hasMoreElements())
	{
		Terrain *terrain = ti.getNext()->instance;
		if (!terrain) continue;
		//ShadowManager::getSingleton().updatePSSM(terrain);
		if (!terrain->isDerivedDataUpdateInProgress())
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

	Ogre::TerrainPSSMMaterialGenerator *matGen = new Ogre::TerrainPSSMMaterialGenerator();
	Ogre::TerrainMaterialGeneratorPtr ptr = Ogre::TerrainMaterialGeneratorPtr();
	ptr.bind(matGen);

	Light *light = gEnv->terrainManager->getMainLight();
	TerrainGlobalOptions *terrainOptions = TerrainGlobalOptions::getSingletonPtr();

	terrainOptions->setDefaultMaterialGenerator(ptr);
	// Configure global
	terrainOptions->setMaxPixelError(m_terrain_config.GetInt("MaxPixelError", 5));

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
	defaultimp.minBatchSize = m_terrain_config.GetInt("minBatchSize", 33);
	defaultimp.maxBatchSize = m_terrain_config.GetInt("maxBatchSize", 65);

	// optimizations
	TerrainPSSMMaterialGenerator::SM2Profile* matProfile = static_cast<TerrainPSSMMaterialGenerator::SM2Profile*>(terrainOptions->getDefaultMaterialGenerator()->getActiveProfile());
	if (matProfile)
	{
		matProfile->setLightmapEnabled(m_terrain_config.GetBool("LightmapEnabled", false));
		// Fix for OpenGL, otherwise terrains are black
		if (Root::getSingleton().getRenderSystem()->getName() == "OpenGL Rendering Subsystem") {
			matProfile->setLayerNormalMappingEnabled(true);
			matProfile->setLayerSpecularMappingEnabled(true);
		} else {
			matProfile->setLayerNormalMappingEnabled(m_terrain_config.GetBool("NormalMappingEnabled", false));
			matProfile->setLayerSpecularMappingEnabled(m_terrain_config.GetBool("SpecularMappingEnabled", false));
		}
		matProfile->setLayerParallaxMappingEnabled(m_terrain_config.GetBool("ParallaxMappingEnabled", false));
		matProfile->setGlobalColourMapEnabled(m_terrain_config.GetBool("GlobalColourMapEnabled", false));
		matProfile->setReceiveDynamicShadowsDepth(m_terrain_config.GetBool("ReceiveDynamicShadowsDepth", false));

		terrainManager->getShadowManager()->updateTerrainMaterial(matProfile);
	}

	terrainOptions->setLayerBlendMapSize(m_terrain_config.GetInt("LayerBlendMapSize", 1024));
	terrainOptions->setCompositeMapSize(m_terrain_config.GetInt("CompositeMapSize", 1024));
	terrainOptions->setCompositeMapDistance(m_terrain_config.GetInt("CompositeMapDistance", 4000));
	terrainOptions->setSkirtSize(m_terrain_config.GetInt("SkirtSize", 30));
	terrainOptions->setLightMapSize(m_terrain_config.GetInt("LightMapSize", 1024));

	if (matProfile->getReceiveDynamicShadowsPSSM())
		terrainOptions->setCastsDynamicShadows(true);

	terrainOptions->setUseRayBoxDistanceCalculation(false);

	//TODO: Make this only when hydrax is enabled.
	terrainOptions->setUseVertexCompressionWhenAvailable(false);

	// load layer basics now
	loadLayers(0, 0, 0);
}

// if terrain is set, we operate on the already loaded terrain
void TerrainGeometryManager::loadLayers(int x, int z, Terrain *terrain)
{
	if (pageConfigFormat.empty()) return;
	
	DataStreamPtr ds = getPageConfig(x, z);

	if (ds.isNull())
		return;

	char line_buf[4096];
	ds->readLine(line_buf, 4096);
	String heightmapImage = RoR::Utils::SanitizeUtf8String(String(line_buf));
	ds->readLine(line_buf, 4096);
	terrainLayers = PARSEINT(RoR::Utils::SanitizeUtf8String(String(line_buf)));

	if (terrainLayers == 0)
		return;

	Ogre::Terrain::ImportData &defaultimp = mTerrainGroup->getDefaultImportSettings();

	if (!terrain)
		defaultimp.layerList.resize(terrainLayers);

	blendInfo.clear();
	blendInfo.resize(terrainLayers);

	int layer = 0;

	while (!ds->eof())
	{
		size_t ll = ds->readLine(line_buf, 4096);
		std::string line = RoR::Utils::SanitizeUtf8String(std::string(line_buf));
		if (ll==0 || line[0]=='/' || line[0]==';') continue;

		StringVector args = StringUtil::split(String(line), ",");
		if (args.size() < 3)
		{
			LOG("invalid page config line: '" + String(line) + "'");
			continue;
		}

		StringUtil::trim(args[1]);
		StringUtil::trim(args[2]);

		float worldSize = PARSEREAL(args[0]);
		if (!terrain)
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

		if (args.size() > 3)
		{
			StringUtil::trim(args[3]);
			bi.blendMapTextureFilename = args[3];
		}
		if (args.size() > 4)
		{
			StringUtil::trim(args[4]);
			bi.blendMode = args[4][0];
		}
		if (args.size() > 5)
			bi.alpha = PARSEREAL(args[5]);

		layer++;
		if (layer >= terrainLayers)
			break;
	}
	LOG("done loading page: loaded " + TOSTRING(layer) + " layers");
}

void TerrainGeometryManager::initBlendMaps(int x, int z, Ogre::Terrain* terrain )
{
	bool debugBlendMaps = m_terrain_config.GetBool("DebugBlendMaps", false);

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
		for (Ogre::uint32 z = 0; z != blendmapSize; z++)
		{
			for (Ogre::uint32 x = 0; x != blendmapSize; x++)
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
	String heightmapString = "Heightmap."+TOSTRING(x)+"."+TOSTRING(z);
	String heightmapFilename = getPageHeightmap(x, z);
	StringUtil::trim(heightmapFilename);

	if (heightmapFilename.empty())
	{
		LOG("empty Heightmap provided, please use 'Flat=1' instead");
		return false;
	}

	if (heightmapFilename.find(".raw") != String::npos)
	{
		int rawSize = m_terrain_config.GetInt(heightmapString + ".raw.size", 1025);
		int bpp     = m_terrain_config.GetInt(heightmapString + ".raw.bpp", 2);

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

	if (m_terrain_config.GetBool(heightmapString + ".flipX", false))
		img.flipAroundX();
	if (m_terrain_config.GetBool(heightmapString + ".flipY", false))
		img.flipAroundY();

	return true;
}

void TerrainGeometryManager::defineTerrain(int x, int z, bool flat)
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
	} else
	{
		Image img;
		if (getTerrainImage(x, z, img))
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

	while (ti.hasMoreElements())
	{
		Terrain *terrain = ti.getNext()->instance;
		if (!terrain) continue;
		MaterialPtr mat = terrain->getCompositeMapMaterial();
		if (!mat.isNull())
			return mat->getName();
	}

	return String();
}
