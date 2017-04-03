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
#include "OTCFileformat.h"
#include "Utils.h"

#include <OgreLight.h>
#include <OgreTerrainGroup.h>

using namespace Ogre;
using namespace RoR;

#define XZSTR(X,Z)   String("[") + TOSTRING(X) + String(",") + TOSTRING(Z) + String("]")

TerrainGeometryManager::TerrainGeometryManager(TerrainManager* terrainManager)
    : mHeightData(nullptr)
    , m_was_new_geometry_generated(false)
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

void TerrainGeometryManager::InitTerrain(Json::Value* j_terrn)
{
    Json::Value& j_otc = (*j_terrn)["ogre_terrain_conf"];
    const std::string cache_filename_format = j_otc["cache_filename_base"].asString() + "_OGRE_" + TOSTRING(OGRE_VERSION) + "_";

    m_terrain_is_flat = j_otc["is_flat"].asBool();
    const bool disable_cache = j_otc["disable_cache"].asBool();
    const bool enable_blendmap_dbg = j_otc["blendmap_dbg_enabled"].asBool();
    m_world_size = Ogre::Vector3(
        j_otc["world_size_x"].asFloat(),
        j_otc["world_size_y"].asFloat(),
        j_otc["world_size_z"].asFloat());

    m_ogre_terrain_group = OGRE_NEW TerrainGroup(
        gEnv->sceneManager, Terrain::ALIGN_X_Z, j_otc["page_size"].asInt(), j_otc["world_size"].asInt());
    m_ogre_terrain_group->setFilenameConvention(cache_filename_format, "mapbin");
    m_ogre_terrain_group->setOrigin(Ogre::Vector3(
        j_otc["origin_pos"]["x"].asFloat(),
        j_otc["origin_pos"]["y"].asFloat(),
        j_otc["origin_pos"]["z"].asFloat()));
    m_ogre_terrain_group->setResourceGroup("cache");

    this->ConfigureTerrainDefaults(&j_otc);

    auto* loading_win = RoR::App::GetGuiManager()->GetLoadingWindow();
    for (Json::Value& j_page : j_otc["pages"])
    {
        loading_win->setProgress(23, _L("preparing terrain page ") + XZSTR(j_page["pos_x"].asInt(), j_page["pos_z"].asInt()));
        this->SetupGeometry(&j_page, disable_cache);
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
        for (Json::Value& j_page : j_otc["pages"])
        {
            const int pos_x = j_page["pos_x"].asInt();
            const int pos_z = j_page["pos_z"].asInt();
            Terrain* terrain = m_ogre_terrain_group->getTerrain(pos_x, pos_z);
            if (terrain != nullptr)
            {
                loading_win->setProgress(23, _L("loading terrain page layers ") + XZSTR(pos_x, pos_z));
                this->SetupLayers(&j_page, terrain);
                loading_win->setProgress(23, _L("loading terrain page blend maps ") + XZSTR(pos_x, pos_z));
                this->SetupTerrnBlendMaps(&j_page, terrain, enable_blendmap_dbg);
            }
        }

        // always save the results when it was imported
        if (!disable_cache)
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

bool TerrainGeometryManager::update(float dt)
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
    return true;
}

void TerrainGeometryManager::ConfigureTerrainDefaults(Json::Value* j_otc_ptr)
{
    OGRE_NEW TerrainGlobalOptions();
    Json::Value& j_otc = *j_otc_ptr;

    Ogre::TerrainPSSMMaterialGenerator* matGen = new Ogre::TerrainPSSMMaterialGenerator();
    Ogre::TerrainMaterialGeneratorPtr ptr = Ogre::TerrainMaterialGeneratorPtr();
    ptr.bind(matGen);

    Light* light = gEnv->terrainManager->getMainLight();
    TerrainGlobalOptions* terrainOptions = TerrainGlobalOptions::getSingletonPtr();

    terrainOptions->setDefaultMaterialGenerator(ptr);
    // Configure global
    terrainOptions->setMaxPixelError(j_otc["max_pixel_error"].asFloat());

    // Important to set these so that the terrain knows what to use for derived (non-realtime) data
    if (light)
    {
        terrainOptions->setLightMapDirection(light->getDerivedDirection());
        terrainOptions->setCompositeMapDiffuse(light->getDiffuseColour());
    }
    terrainOptions->setCompositeMapAmbient(gEnv->sceneManager->getAmbientLight());

    // Configure default import settings for if we use imported image
    Ogre::Terrain::ImportData& defaultimp = m_ogre_terrain_group->getDefaultImportSettings();
    defaultimp.terrainSize  = static_cast<Ogre::uint16>(j_otc["page_size"].asUInt()); // the heightmap size
    defaultimp.worldSize    = j_otc["world_size"].asFloat(); // this is the scaled up size, like 12km
    defaultimp.inputScale   = j_otc["world_size_y"].asFloat();
    defaultimp.minBatchSize = static_cast<Ogre::uint16>(j_otc["batch_size_min"].asUInt());
    defaultimp.maxBatchSize = static_cast<Ogre::uint16>(j_otc["batch_size_max"].asUInt());

    // optimizations
    TerrainPSSMMaterialGenerator::SM2Profile* matProfile = static_cast<TerrainPSSMMaterialGenerator::SM2Profile*>(terrainOptions->getDefaultMaterialGenerator()->getActiveProfile());
    if (matProfile)
    {
        matProfile->setLightmapEnabled(j_otc["lightmap_enabled"].asBool());
        // Fix for OpenGL, otherwise terrains are black
        if (Root::getSingleton().getRenderSystem()->getName() == "OpenGL Rendering Subsystem")
        {
            matProfile->setLayerNormalMappingEnabled(true);
            matProfile->setLayerSpecularMappingEnabled(true);
        }
        else
        {
            matProfile->setLayerNormalMappingEnabled(j_otc["norm_map_enabled"].asBool());
            matProfile->setLayerSpecularMappingEnabled(j_otc["spec_map_enabled"].asBool());
        }
        matProfile->setLayerParallaxMappingEnabled(j_otc["parallax_enabled"].asBool());
        matProfile->setGlobalColourMapEnabled(j_otc["global_colormap_enabled"].asBool());
        matProfile->setReceiveDynamicShadowsDepth(j_otc["recv_dyn_shadows_depth"].asBool());

        m_terrain_mgr->getShadowManager()->updateTerrainMaterial(matProfile);
    }

    terrainOptions->setLayerBlendMapSize   (j_otc["layer_blendmap_size"].asUInt());
    terrainOptions->setCompositeMapSize    (j_otc["composite_map_size"].asUInt());
    terrainOptions->setCompositeMapDistance(j_otc["composite_map_distance"].asFloat());
    terrainOptions->setSkirtSize           (j_otc["skirt_size"].asFloat());
    terrainOptions->setLightMapSize        (j_otc["lightmap_size"].asUInt());

    if (matProfile->getReceiveDynamicShadowsPSSM())
        terrainOptions->setCastsDynamicShadows(true);

    terrainOptions->setUseRayBoxDistanceCalculation(false);

    //TODO: Make this only when hydrax is enabled.
    terrainOptions->setUseVertexCompressionWhenAvailable(false);

    // HACK: Load the single page config now
    // This is how it "worked before" ~ only_a_ptr, 04/2017
    this->SetupLayers(&(j_otc["pages"][0]), nullptr);
}

// if terrain is set, we operate on the already loaded terrain
void TerrainGeometryManager::SetupLayers(Json::Value* j_page_ptr, Ogre::Terrain *terrain)
{
    Json::Value& j_page = *j_page_ptr;
    const int num_layers = j_page["num_layers"].asInt();
    if (num_layers == 0)
        return;

    Ogre::Terrain::ImportData& defaultimp = m_ogre_terrain_group->getDefaultImportSettings();

    if (!terrain)
        defaultimp.layerList.resize(num_layers);

    int layer_idx = 0;

    for (Json::Value& j_layer : j_page["layers"])
    {
        if (!terrain)
        {
            defaultimp.layerList[layer_idx].worldSize = j_layer["world_size"].asFloat();
            defaultimp.layerList[layer_idx].textureNames.push_back(j_layer["diffusespecular_filename"].asString());
            defaultimp.layerList[layer_idx].textureNames.push_back(j_layer["normalheight_filename"].asString());
        }
        else
        {
            terrain->setLayerWorldSize(layer_idx, j_layer["world_size"].asFloat());
            terrain->setLayerTextureName(layer_idx, 0, j_layer["diffusespecular_filename"].asString());
            terrain->setLayerTextureName(layer_idx, 1, j_layer["normalheight_filename"].asString());
        }

        layer_idx++;
    }
    LOG("done loading page: loaded " + TOSTRING(layer_idx) + " layers");
}

void TerrainGeometryManager::SetupTerrnBlendMaps(Json::Value* j_page, Ogre::Terrain* terrain, bool enable_debug)
{
    const int layerCount = terrain->getLayerCount();
    for (int i = 1; i < layerCount; i++)
    {
        Json::Value& j_layer = (*j_page)["layers"][i];

        if (j_layer["blendmap_filename"] == Json::nullValue)
            continue;

        Ogre::Image img;
        try
        {
            img.load(j_layer["blendmap_filename"].asString(), ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
        }
        catch (Exception& e)
        {
            LOG("Error loading blendmap: " + j_layer["blendmap_filename"].asString() + " : " + e.getFullDescription());
            continue;
        }

        TerrainLayerBlendMap* blendmap = terrain->getLayerBlendMap(i);

        // resize that blending map so it will fit
        const Ogre::uint32 blendmapSize = terrain->getLayerBlendMapSize();
        if (img.getWidth() != blendmapSize)
            img.resize(blendmapSize, blendmapSize);

        // now to the ugly part
        float* ptr = blendmap->getBlendPointer();
        for (Ogre::uint32 z = 0; z != blendmapSize; z++)
        {
            for (Ogre::uint32 x = 0; x != blendmapSize; x++)
            {
                Ogre::ColourValue c = img.getColourAt(x, z, 0);
                const char blend_mode = j_layer["blend_mode"].asString().at(0);
                const float alpha = j_layer["alpha"].asFloat();
                if (blend_mode == 'R')
                    *ptr++ = c.r * alpha;
                else if (blend_mode == 'G')
                    *ptr++ = c.g * alpha;
                else if (blend_mode == 'B')
                    *ptr++ = c.b * alpha;
                else if (blend_mode == 'A')
                    *ptr++ = c.a * alpha;
            }
        }
        blendmap->dirty();
        blendmap->update();
    }

    if (enable_debug)
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

// Internal helper
bool LoadHeightmap(Image& img, Json::Value& j_page)
{
    if (j_page["heightmap_filename"] == Json::nullValue)
    {
        LOG("[RoR|Terrain] Empty Heightmap provided in OTC, please use 'Flat=1' instead"); // TODO: Parser should say this, not spawner!
        return false;
    }

    const std::string filename = j_page["heightmap_filename"].asString();
    if (filename.find(".raw") != String::npos)
    {
        // load raw data
        int raw_size = j_page["raw_size"].asInt();
        int raw_bpp = j_page["raw_bpp"].asInt();
        DataStreamPtr stream = ResourceGroupManager::getSingleton().openResource(filename);
        LOG("[RoR|Terrain] loading RAW image: " + TOSTRING(stream->size()) + " / " + TOSTRING(raw_size*raw_size*raw_bpp));
        PixelFormat pix_format = (raw_bpp == 2) ? PF_L16 : PF_L8;
        img.loadRawData(stream, raw_size, raw_size, 1, pix_format);
    }
    else
    {
        img.load(filename, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    }

    if (j_page["raw_flip_x"] == true)
        img.flipAroundX();
    if (j_page["raw_flip_y"] == true)
        img.flipAroundY();

    return true;
}

void TerrainGeometryManager::SetupGeometry(Json::Value* j_page_ptr, bool disable_cache)
{
    Json::Value& j_page = *j_page_ptr;
    const int pos_x = j_page["pos_x"].asInt();
    const int pos_z = j_page["pos_z"].asInt();

    if (m_terrain_is_flat)
    {
        // very simple, no height data to load at all
        m_ogre_terrain_group->defineTerrain(pos_x, pos_z, 0.0f);
        return;
    }

    const std::string page_cache_filename = m_ogre_terrain_group->generateFilename(pos_x, pos_z);
    const std::string res_group = m_ogre_terrain_group->getResourceGroup();
    if (!disable_cache && ResourceGroupManager::getSingleton().resourceExists(res_group, page_cache_filename))
    {
        // load from cache
        m_ogre_terrain_group->defineTerrain(pos_x, pos_z);
    }
    else
    {
        Image img;
        if (LoadHeightmap(img, j_page))
        {
            m_ogre_terrain_group->defineTerrain(pos_x, pos_z, &img);
            m_was_new_geometry_generated = true;
        }
        else
        {
            // fall back to no heightmap
            m_ogre_terrain_group->defineTerrain(pos_x, pos_z, 0.0f);
        }
    }
}

