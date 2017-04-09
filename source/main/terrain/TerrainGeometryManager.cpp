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

#include "TerrainGeometryManager.h"

#include "Application.h"
#include "Language.h"
#include "GUIManager.h"
#include "GUI_LoadingWindow.h"
#include "TerrainManager.h"
#include "ShadowManager.h"
#include "OgreTerrainPSSMMaterialGenerator.h"
#include "Utils.h"

#include <OgreTerrainGroup.h>

using namespace Ogre;

#define XZSTR(X,Z)   String("[") + TOSTRING(X) + String(",") + TOSTRING(Z) + String("]")

TerrainGeometryManager::TerrainGeometryManager(TerrainManager* terrainManager) :
    m_terrn_disable_caching(false)
    , mHeightData(nullptr)
    , m_was_new_geometry_generated(false)
    , m_terrain_is_flat(true)
    , m_terrain_mgr(terrainManager)
{
}

TerrainGeometryManager::~TerrainGeometryManager()
{
    m_ogre_terrain_group->removeAllTerrains();
}

/// @author Ported from OGRE engine, www.ogre3d.org, file OgreTerrain.cpp
void TerrainGeometryManager::getTerrainPositionAlign(Real x, Real y, Real z, Terrain::Alignment align, Vector3* outTSpos)
{
    switch (align)
    {
    case Terrain::ALIGN_X_Z:
        outTSpos->x = (x - mBase - mPos.x) / ((mSize - 1) * mScale);
        outTSpos->y = (z + mBase - mPos.z) / ((mSize - 1) * -mScale);
        outTSpos->z = y;
        break;
    case Terrain::ALIGN_Y_Z:
        outTSpos->x = (z - mBase - mPos.z) / ((mSize - 1) * -mScale);
        outTSpos->y = (y + mBase - mPos.y) / ((mSize - 1) * mScale);
        outTSpos->z = x;
        break;
    case Terrain::ALIGN_X_Y:
        outTSpos->x = (x - mBase - mPos.x) / ((mSize - 1) * mScale);
        outTSpos->y = (y - mBase - mPos.y) / ((mSize - 1) * mScale);
        outTSpos->z = z;
        break;
    };
}

/// @author Ported from OGRE engine, www.ogre3d.org, file OgreTerrain.cpp
float TerrainGeometryManager::getHeightAtPoint(long x, long y)
{
    // clamp
    x = std::min(x, (long)mSize - 1L);
    x = std::max(x, 0L);
    y = std::min(y, (long)mSize - 1L);
    y = std::max(y, 0L);

    return mHeightData[y * mSize + x];
}

/// @author Ported from OGRE engine, www.ogre3d.org, file OgreTerrain.cpp
float TerrainGeometryManager::getHeightAtTerrainPosition(Real x, Real y)
{
    // get left / bottom points (rounded down)
    Real factor = (Real)mSize - 1.0f;
    Real invFactor = 1.0f / factor;

    long startX = static_cast<long>(x * factor);
    long startY = static_cast<long>(y * factor);
    long endX = startX + 1;
    long endY = startY + 1;

    // now get points in terrain space (effectively rounding them to boundaries)
    // note that we do not clamp! We need a valid plane
    Real startXTS = startX * invFactor;
    Real startYTS = startY * invFactor;
    Real endXTS = endX * invFactor;
    Real endYTS = endY * invFactor;

    // now clamp
    endX = std::min(endX, (long)mSize - 1);
    endY = std::min(endY, (long)mSize - 1);

    // get parametric from start coord to next point
    Real xParam = (x - startXTS) / invFactor;
    Real yParam = (y - startYTS) / invFactor;

    /* For even / odd tri strip rows, triangles are this shape:
    even     odd
    3---2   3---2
    | / |   | \ |
    0---1   0---1
    */

    // Build all 4 positions in terrain space, using point-sampled height
    Vector3 v0(startXTS, startYTS, getHeightAtPoint(startX, startY));
    Vector3 v1(endXTS, startYTS, getHeightAtPoint(endX, startY));
    Vector3 v2(endXTS, endYTS, getHeightAtPoint(endX, endY));
    Vector3 v3(startXTS, endYTS, getHeightAtPoint(startX, endY));
    // define this plane in terrain space
    Plane plane;
    if (startY % 2)
    {
        // odd row
        bool secondTri = ((1.0 - yParam) > xParam);
        if (secondTri)
            plane.redefine(v0, v1, v3);
        else
            plane.redefine(v1, v2, v3);
    }
    else
    {
        // even row
        bool secondTri = (yParam > xParam);
        if (secondTri)
            plane.redefine(v0, v2, v3);
        else
            plane.redefine(v0, v1, v2);
    }

    // Solve plane equation for z
    return (-plane.normal.x * x
            - plane.normal.y * y
            - plane.d) / plane.normal.z;
}

/// @author Ported from OGRE engine, www.ogre3d.org, file OgreTerrain.cpp
float TerrainGeometryManager::getHeightAtWorldPosition(float x, float z)
{
    Vector3 terrPos;
    getTerrainPositionAlign(x, 0.0f, z, mAlign, &terrPos);

    if (terrPos.x <= 0.0f || terrPos.y <= 0.0f || terrPos.x >= 1.0f || terrPos.y >= 1.0f)
        return 0.0f;

    return getHeightAtTerrainPosition(terrPos.x, terrPos.y);
}

float TerrainGeometryManager::getHeightAt(float x, float z)
{
    if (m_terrain_is_flat)
        return 0.0f;
    else
    //return m_ogre_terrain_group->getHeightAtWorldPosition(x, 1000, z);
        return getHeightAtWorldPosition(x, z);
}

Ogre::Vector3 TerrainGeometryManager::getNormalAt(float x, float y, float z, float precision)
{
    Ogre::Vector3 left(-precision, getHeightAt(x - precision, z) - y, 0.0f);
    Ogre::Vector3 down(0.0f, getHeightAt(x, z + precision) - y, precision);
    down = left.crossProduct(down);
    down.normalise();
    return down;
}

void TerrainGeometryManager::loadOgreTerrainConfig(String filename)
{
    String ext;
    Ogre::StringUtil::splitBaseFilename(filename, m_terrn_base_name, ext);

    loadTerrainConfig(filename);

    m_terrn_disable_caching = m_terrain_config.GetBool("m_terrn_disable_caching", false);

    initTerrain();
}

bool TerrainGeometryManager::loadTerrainConfig(String filename)
{
    try
    {
        DataStreamPtr ds_config = ResourceGroupManager::getSingleton().openResource(filename, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
        m_terrain_config.load(ds_config, "\t:=", false);
        return true;
    }
    catch (...)
    {
        m_terrain_config.clear();
    }
    return false;
}

Ogre::String TerrainGeometryManager::getPageConfigFilename(int x, int z)
{
    String cfg = m_pageconf_filename_format;

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
    }
    catch (...)
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
    m_map_size_x = m_terrain_config.GetInt("WorldSizeX", 1024);
    m_map_size_y = m_terrain_config.GetInt("WorldSizeY", 50);
    m_map_size_z = m_terrain_config.GetInt("WorldSizeZ", 1024);
    m_terrain_page_size = m_terrain_config.GetInt("PageSize", 1025);

    m_terrain_world_size = std::max(m_map_size_x, m_map_size_z);

    int pageMinX = 0;
    int pageMaxX = m_terrain_config.GetInt("PagesX", 0);
    int pageMinZ = 0;
    int pageMaxZ = m_terrain_config.GetInt("PagesZ", 0);

    m_pageconf_filename_format = m_terrain_config.GetString("PageFileFormat", m_terrn_base_name + "-page-{X}-{Z}.otc");

    m_terrain_is_flat = m_terrain_config.GetBool("Flat", false);

    String Filename = m_terrn_base_name + "_OGRE_" + TOSTRING(OGRE_VERSION) + "_";

    m_ogre_terrain_group 
        = OGRE_NEW Ogre::TerrainGroup(gEnv->sceneManager,
                                      Terrain::ALIGN_X_Z,
                                      static_cast<Ogre::uint16>(m_terrain_page_size),
                                      m_terrain_world_size);
    m_ogre_terrain_group->setFilenameConvention(Filename, "mapbin");
    m_ogre_terrain_group->setOrigin(Vector3(m_map_size_x / 2.0f, 0.0f, m_map_size_z / 2.0f));
    m_ogre_terrain_group->setResourceGroup("cache");

    configureTerrainDefaults();

    auto* loading_win = RoR::App::GetGuiManager()->GetLoadingWindow();

    for (long x = pageMinX; x <= pageMaxX; ++x)
    {
        for (long z = pageMinZ; z <= pageMaxZ; ++z)
        {
            loading_win->setProgress(23, _L("preparing terrain page ") + XZSTR(x,z));
            defineTerrain(x, z, m_terrain_is_flat);
            loading_win->setProgress(23, _L("loading terrain page ") + XZSTR(x,z));
        }
    }

    // sync load since we want everything in place when we start
    loading_win->setProgress(23, _L("loading terrain pages"));
    m_ogre_terrain_group->loadAllTerrains(true);

    Terrain* terrain = m_ogre_terrain_group->getTerrain(0, 0);
    mHeightData = terrain->getHeightData();
    mAlign = terrain->getAlignment();
    mSize = terrain->getSize();
    const float world_size = terrain->getWorldSize();
    mBase = -world_size * 0.5f;
    mScale = world_size / (Real)(mSize - 1);
    mPos = terrain->getPosition();

    // update the blend maps
    if (m_was_new_geometry_generated)
    {
        for (long x = pageMinX; x <= pageMaxX; ++x)
        {
            for (long z = pageMinZ; z <= pageMaxZ; ++z)
            {
                Terrain* terrain = m_ogre_terrain_group->getTerrain(x, z);
                if (!terrain)
                    continue;
                loading_win->setProgress(23, _L("loading terrain page layers ") + XZSTR(x,z));
                loadLayers(x, z, terrain);
                loading_win->setProgress(23, _L("loading terrain page blend maps ") + XZSTR(x,z));
                initBlendMaps(x, z, terrain);
            }
        }

        // always save the results when it was imported
        if (!m_terrn_disable_caching)
        {
            loading_win->setProgress(23, _L("saving all terrain pages ..."));
            m_ogre_terrain_group->saveAllTerrains(false);
        }
    }
    else
    {
        LOG(" *** Terrain loaded from cache ***");
    }

    m_ogre_terrain_group->freeTemporaryResources();
}

void TerrainGeometryManager::updateLightMap()
{
    TerrainGroup::TerrainIterator ti = m_ogre_terrain_group->getTerrainIterator();

    while (ti.hasMoreElements())
    {
        Terrain* terrain = ti.getNext()->instance;
        if (!terrain)
            continue;

        if (!terrain->isDerivedDataUpdateInProgress())
        {
            terrain->dirtyLightmap();
            terrain->updateDerivedData();
        }
    }
}

void TerrainGeometryManager::UpdateMainLightPosition()
{
    Light* light = gEnv->terrainManager->getMainLight();
    TerrainGlobalOptions* terrainOptions = TerrainGlobalOptions::getSingletonPtr();
    if (light)
    {
        terrainOptions->setLightMapDirection(light->getDerivedDirection());
        terrainOptions->setCompositeMapDiffuse(light->getDiffuseColour());
    }
    terrainOptions->setCompositeMapAmbient(gEnv->sceneManager->getAmbientLight());

    m_ogre_terrain_group->update();
}

void TerrainGeometryManager::configureTerrainDefaults()
{
    OGRE_NEW TerrainGlobalOptions();

    Ogre::TerrainPSSMMaterialGenerator* matGen = new Ogre::TerrainPSSMMaterialGenerator();
    Ogre::TerrainMaterialGeneratorPtr ptr = Ogre::TerrainMaterialGeneratorPtr();
    ptr.bind(matGen);

    Light* light = gEnv->terrainManager->getMainLight();
    TerrainGlobalOptions* terrainOptions = TerrainGlobalOptions::getSingletonPtr();

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
    Ogre::Terrain::ImportData& defaultimp = m_ogre_terrain_group->getDefaultImportSettings();
    defaultimp.terrainSize = static_cast<Ogre::uint16>(m_terrain_page_size); // the heightmap size
    defaultimp.worldSize = m_terrain_world_size; // this is the scaled up size, like 12km
    defaultimp.inputScale = m_map_size_y;
    defaultimp.minBatchSize = m_terrain_config.GetInt("minBatchSize", 33);
    defaultimp.maxBatchSize = m_terrain_config.GetInt("maxBatchSize", 65);

    // optimizations
    TerrainPSSMMaterialGenerator::SM2Profile* matProfile = static_cast<TerrainPSSMMaterialGenerator::SM2Profile*>(terrainOptions->getDefaultMaterialGenerator()->getActiveProfile());
    if (matProfile)
    {
        matProfile->setLightmapEnabled(m_terrain_config.GetBool("LightmapEnabled", false));
        // Fix for OpenGL, otherwise terrains are black
        if (Root::getSingleton().getRenderSystem()->getName() == "OpenGL Rendering Subsystem")
        {
            matProfile->setLayerNormalMappingEnabled(true);
            matProfile->setLayerSpecularMappingEnabled(true);
        }
        else
        {
            matProfile->setLayerNormalMappingEnabled(m_terrain_config.GetBool("NormalMappingEnabled", false));
            matProfile->setLayerSpecularMappingEnabled(m_terrain_config.GetBool("SpecularMappingEnabled", false));
        }
        matProfile->setLayerParallaxMappingEnabled(m_terrain_config.GetBool("ParallaxMappingEnabled", false));
        matProfile->setGlobalColourMapEnabled(m_terrain_config.GetBool("GlobalColourMapEnabled", false));
        matProfile->setReceiveDynamicShadowsDepth(m_terrain_config.GetBool("ReceiveDynamicShadowsDepth", false));

        m_terrain_mgr->getShadowManager()->updateTerrainMaterial(matProfile);
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
void TerrainGeometryManager::loadLayers(int x, int z, Terrain* terrain)
{
    if (m_pageconf_filename_format.empty())
        return;

    DataStreamPtr ds = getPageConfig(x, z);

    if (ds.isNull())
        return;

    char line_buf[4096];
    ds->readLine(line_buf, 4096);
    String heightmapImage = RoR::Utils::SanitizeUtf8String(String(line_buf));
    ds->readLine(line_buf, 4096);
    size_t num_layers = static_cast<size_t>(Ogre::StringConverter::parseUnsignedInt(RoR::Utils::SanitizeUtf8String(std::string(line_buf))));

    if (num_layers == 0)
        return;

    Ogre::Terrain::ImportData& defaultimp = m_ogre_terrain_group->getDefaultImportSettings();

    if (!terrain)
        defaultimp.layerList.resize(num_layers);

    m_terrn_blend_layers.clear();
    m_terrn_blend_layers.resize(num_layers);

    size_t layer = 0;

    while (!ds->eof())
    {
        size_t ll = ds->readLine(line_buf, 4096);
        std::string line = RoR::Utils::SanitizeUtf8String(std::string(line_buf));
        if (ll == 0 || line[0] == '/' || line[0] == ';')
            continue;

        StringVector args = StringUtil::split(String(line), ",");
        if (args.size() < 3)
        {
            LOG("invalid page config line: '" + String(line) + "'");
            continue;
        }

        StringUtil::trim(args[1]);
        StringUtil::trim(args[2]);

        float m_terrain_world_size = PARSEREAL(args[0]);
        if (!terrain)
        {
            defaultimp.layerList[layer].worldSize = m_terrain_world_size;
            defaultimp.layerList[layer].textureNames.push_back(args[1]);
            defaultimp.layerList[layer].textureNames.push_back(args[2]);
        }
        else
        {
            Ogre::uint8 layer_u8 = static_cast<Ogre::uint8>(layer);
            terrain->setLayerWorldSize(layer_u8, m_terrain_world_size);
            terrain->setLayerTextureName(layer_u8, 0, args[1]);
            terrain->setLayerTextureName(layer_u8, 1, args[2]);
        }

        TerrnBlendLayerDef& bi = m_terrn_blend_layers[layer];
        bi.blend_mode = 'R';
        bi.alpha_value = 'R'; // WTF?? Kept around for bug-to-bug compatibility ~ only_a_ptr, 03/2017

        if (args.size() > 3)
        {
            StringUtil::trim(args[3]);
            bi.blendmap_tex_filename = args[3];
        }
        if (args.size() > 4)
        {
            StringUtil::trim(args[4]);
            bi.blend_mode = args[4][0];
        }
        if (args.size() > 5)
            bi.alpha_value = PARSEREAL(args[5]);

        layer++;
        if (layer >= num_layers)
            break;
    }
    LOG("done loading page: loaded " + TOSTRING(layer) + " layers");
}

void TerrainGeometryManager::initBlendMaps(int x, int z, Ogre::Terrain* terrain)
{
    bool debugBlendMaps = m_terrain_config.GetBool("DebugBlendMaps", false);

    int layerCount = terrain->getLayerCount();
    for (int i = 1; i < layerCount; i++)
    {
        TerrnBlendLayerDef& bi = m_terrn_blend_layers[i];

        if (bi.blendmap_tex_filename.empty())
            continue;

        Ogre::Image img;
        try
        {
            img.load(bi.blendmap_tex_filename, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
        }
        catch (Exception& e)
        {
            LOG("Error loading blendmap: " + bi.blendmap_tex_filename + " : " + e.getFullDescription());
            continue;
        }

        TerrainLayerBlendMap* blendmap = terrain->getLayerBlendMap(i);

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
                float alpha = bi.alpha_value;
                if (bi.blend_mode == 'R')
                    *ptr++ = c.r * alpha;
                else if (bi.blend_mode == 'G')
                    *ptr++ = c.g * alpha;
                else if (bi.blend_mode == 'B')
                    *ptr++ = c.b * alpha;
                else if (bi.blend_mode == 'A')
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
            unsigned short* idata = OGRE_ALLOC_T(unsigned short, blendmapSize * blendmapSize, Ogre::MEMCATEGORY_RESOURCE);
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
    String heightmapString = "Heightmap." + TOSTRING(x) + "." + TOSTRING(z);
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
        int bpp = m_terrain_config.GetInt(heightmapString + ".raw.bpp", 2);

        // load raw data
        DataStreamPtr stream = ResourceGroupManager::getSingleton().openResource(heightmapFilename);
        LOG(" loading RAW image: " + TOSTRING(stream->size()) + " / " + TOSTRING(rawSize*rawSize*bpp));
        PixelFormat pformat = PF_L8;
        if (bpp == 2)
            pformat = PF_L16;
        img.loadRawData(stream, rawSize, rawSize, 1, pformat);
    }
    else
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
        m_ogre_terrain_group->defineTerrain(x, z, 0.0f);
        return;
    }

    String filename = m_ogre_terrain_group->generateFilename(x, z);

    if (!m_terrn_disable_caching && ResourceGroupManager::getSingleton().resourceExists(m_ogre_terrain_group->getResourceGroup(), filename))
    {
        // load from cache
        m_ogre_terrain_group->defineTerrain(x, z);
    }
    else
    {
        Image img;
        if (getTerrainImage(x, z, img))
        {
            m_ogre_terrain_group->defineTerrain(x, z, &img);
            m_was_new_geometry_generated = true;
        }
        else
        {
            // fall back to no heightmap
            m_ogre_terrain_group->defineTerrain(x, z, 0.0f);
        }
    }
}

Ogre::Vector3 TerrainGeometryManager::getMaxTerrainSize()
{
    return Vector3(m_map_size_x, m_map_size_y, m_map_size_z);
}

