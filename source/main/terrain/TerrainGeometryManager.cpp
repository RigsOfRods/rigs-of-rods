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

#include <OgreLight.h>
#include <Terrain/OgreTerrainGroup.h>

using namespace Ogre;
using namespace RoR;

#define CUSTOM_MAT_PROFILE_NAME "Terrn2CustomMat"

/// @author: http://www.ogre3d.org/forums/viewtopic.php?f=5&t=72455
class Terrn2CustomMaterial : public Ogre::TerrainMaterialGenerator
{
public:

    Terrn2CustomMaterial(Ogre::String materialName, bool addNormalmap, bool cloneMaterial) 
      : m_material_name(materialName), m_add_normal_map(addNormalmap), m_clone_material(cloneMaterial)
    {
        mProfiles.push_back(OGRE_NEW Profile(this, CUSTOM_MAT_PROFILE_NAME, "Renders RoR terrn2 with custom material"));
        this->setActiveProfile(CUSTOM_MAT_PROFILE_NAME);
    }

    void setMaterialByName(const Ogre::String materialName)
    {
        m_material_name = materialName;
        this->_markChanged();
    };

    class Profile : public Ogre::TerrainMaterialGenerator::Profile
    {
    public:
        Profile(Ogre::TerrainMaterialGenerator* parent, const Ogre::String& name, const Ogre::String& desc)
        : Ogre::TerrainMaterialGenerator::Profile(parent, name, desc)
        {
        };
        ~Profile() override {};

        bool               isVertexCompressionSupported () const { return false; }
        void               setLightmapEnabled           (bool set) /*override*/ {} // OGRE 1.8 doesn't have this method
        Ogre::MaterialPtr  generate                     (const Ogre::Terrain* terrain) override;
        Ogre::uint8        getMaxLayers                 (const Ogre::Terrain* terrain) const override { return 0; };
        void               updateParams                 (const Ogre::MaterialPtr& mat, const Ogre::Terrain* terrain) override {};
        void               updateParamsForCompositeMap  (const Ogre::MaterialPtr& mat, const Ogre::Terrain* terrain) override {};

        Ogre::MaterialPtr generateForCompositeMap(const Ogre::Terrain* terrain) override
        {
            return terrain->_getCompositeMapMaterial();
        };

        void requestOptions(Ogre::Terrain* terrain) override
        {
            terrain->_setMorphRequired(false);
            terrain->_setNormalMapRequired(true); // enable global normal map
            terrain->_setLightMapRequired(false);
            terrain->_setCompositeMapRequired(false);
        };
    };

protected:
    Ogre::String m_material_name;
    bool m_clone_material;
    bool m_add_normal_map;
};

Ogre::MaterialPtr Terrn2CustomMaterial::Profile::generate(const Ogre::Terrain* terrain)
{
    const Ogre::String& matName = terrain->getMaterialName();

    Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName(matName);
    if (!mat.isNull()) 
        Ogre::MaterialManager::getSingleton().remove(matName);

    Terrn2CustomMaterial* parent = static_cast<Terrn2CustomMaterial*>(this->getParent());

    // Set Ogre material 
    mat = Ogre::MaterialManager::getSingleton().getByName(parent->m_material_name);

    // Clone material
    if(parent->m_clone_material)
    {
        mat = mat->clone(matName);
        parent->m_material_name = matName;
    }

    // Add normalmap
    if(parent->m_add_normal_map)
    {
        // Get default pass
        Ogre::Pass *p = mat->getTechnique(0)->getPass(0);

        // Add terrain's global normalmap to renderpass so the fragment program can find it.
        Ogre::TextureUnitState *tu = p->createTextureUnitState(matName+"/nm");

        Ogre::TexturePtr nmtx = terrain->getTerrainNormalMap();
        tu->_setTexturePtr(nmtx);
    }

    return mat;
};

// ----------------------------------------------------------------------------

#define XZSTR(X,Z)   String("[") + TOSTRING(X) + String(",") + TOSTRING(Z) + String("]")

TerrainGeometryManager::TerrainGeometryManager(TerrainManager* terrainManager)
    : mHeightData(nullptr)
    , mIsFlat(false)
    , mMinHeight(0.0f)
    , mMaxHeight(std::numeric_limits<float>::min())
    , m_was_new_geometry_generated(false)
    , terrainManager(terrainManager)
{
}

TerrainGeometryManager::~TerrainGeometryManager()
{
    if (m_ogre_terrain_group != nullptr)
    {
        m_ogre_terrain_group->removeAllTerrains();
    }
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

    // get parametric from start coord to next point
    Real xParam = (x * factor - startX);
    Real yParam = (y * factor - startY);

    /* For even / odd tri strip rows, triangles are this shape:
    even     odd
    3---2   3---2
    | / |   | \ |
    0---1   0---1
    */

    // Build all 4 positions in terrain space, using point-sampled height
    Vector3 v0(startXTS, startYTS, mHeightData[startY * mSize + startX]);
    Vector3 v1(endXTS  , startYTS, mHeightData[startY * mSize + endX]);
    Vector3 v2(endXTS  , endYTS  , mHeightData[endY   * mSize + endX]);
    Vector3 v3(startXTS, endYTS  , mHeightData[endY   * mSize + startX]);

    // define this plane in terrain space
    Vector3 normal;
    Real d;
    if (startY % 2)
    {
        // odd row
        bool secondTri = ((1.0 - yParam) > xParam);
        if (secondTri)
        {
            normal = (v1 - v0).crossProduct(v3 - v0);
            d = -normal.dotProduct(v0);
        }
        else
        {
            normal = (v2 - v1).crossProduct(v3 - v1);
            d = -normal.dotProduct(v1);
        }
    }
    else
    {
        // even row
        bool secondTri = (yParam > xParam);
        if (secondTri)
        {
            normal = (v2 - v0).crossProduct(v3 - v0);
            d = -normal.dotProduct(v0);
        }
        else
        {
            normal = (v1 - v0).crossProduct(v2 - v0);
            d = -normal.dotProduct(v0);
        }
    }

    // Solve plane equation for z
    return (-normal.x * x - normal.y * y - d) / normal.z;
}

float TerrainGeometryManager::getHeightAt(float x, float z)
{
    if (m_spec->is_flat)
        return 0.0f;

    float tx = (x - mBase - mPos.x) / ((mSize - 1) *  mScale);
    float ty = (z + mBase - mPos.z) / ((mSize - 1) * -mScale);

    if (tx <= 0.0f || ty <= 0.0f || tx >= 1.0f || ty >= 1.0f)
        return terrainManager->GetDef().water_bottom_height;
    else if (mIsFlat)
        return mMinHeight;

    return getHeightAtTerrainPosition(tx, ty);
}

Ogre::Vector3 TerrainGeometryManager::getNormalAt(float x, float y, float z)
{
    const float precision = 0.1f;
    Vector3 normal(getHeightAt(x - precision, z) - y, precision, y - getHeightAt(x, z + precision));
    normal.normalise();
    return normal;
}

bool TerrainGeometryManager::InitTerrain(std::string otc_filename)
{
    OTCParser otc_parser;

    // Load main *.otc file
    try
    {
        DataStreamPtr ds_config = ResourceGroupManager::getSingleton().openResource(otc_filename);
        if (ds_config.isNull() || !ds_config->isReadable())
        {
            RoR::LogFormat("[RoR|Terrain] Cannot read main *.otc file [%s].", otc_filename.c_str());
            return false;
        }
        if (!otc_parser.LoadMasterConfig(ds_config, otc_filename.c_str()))
        {
            return false; // Error already reported
        }
    }
    catch (...)
    {
        terrainManager->HandleException("Error reading main *.otc file");
        // If we stop parsing we might break some legacy maps
        //return false;
    }

    // Load *.otc files for pages
    for (OTCPage& page : otc_parser.GetDefinition()->pages)
    {
        if (page.pageconf_filename.empty())
        {
            continue; // For backwards compatibility.
        }

        try
        {
            DataStreamPtr ds_page = ResourceGroupManager::getSingleton().openResource(page.pageconf_filename);
            if (ds_page.isNull() || !ds_page->isReadable())
            {
                RoR::LogFormat("[RoR|Terrain] Cannot read file [%s].", page.pageconf_filename.c_str());
                return false;
            }

            // NOTE: Empty file is accepted (leaving all values to defaults) for backwards compatibility.
            if (!otc_parser.LoadPageConfig(ds_page, page, page.pageconf_filename.c_str()))
            {
                return false; // Error already logged
            }
        }
        catch (...)
        {
            terrainManager->HandleException("Error reading page config *.otc file");
            // If we stop parsing we might break some legacy maps
            // return false;
        }
    }

    m_spec = otc_parser.GetDefinition();

    const std::string cache_filename_format = m_spec->cache_filename_base + "_OGRE_" + TOSTRING(OGRE_VERSION) + "_";

    m_ogre_terrain_group = OGRE_NEW TerrainGroup(gEnv->sceneManager, Terrain::ALIGN_X_Z, m_spec->page_size, m_spec->world_size);
    m_ogre_terrain_group->setFilenameConvention(cache_filename_format, "mapbin");
    m_ogre_terrain_group->setOrigin(m_spec->origin_pos);
    m_ogre_terrain_group->setResourceGroup("cache");

    configureTerrainDefaults();

    for (OTCPage& page : m_spec->pages)
    {
        this->SetupGeometry(page, m_spec->is_flat);
    }

    // sync load since we want everything in place when we start
    App::GetGuiManager()->GetLoadingWindow()->setProgress(44, _L("Loading terrain pages ..."));
    m_ogre_terrain_group->loadAllTerrains(true);

    Terrain* terrain = m_ogre_terrain_group->getTerrain(0, 0);

    if (terrain == nullptr)
        return true;

    mHeightData = terrain->getHeightData();
    mSize = terrain->getSize();
    const float world_size = terrain->getWorldSize();
    mBase = -world_size * 0.5f;
    mScale = world_size / (Real)(mSize - 1);
    mPos = terrain->getPosition();

    // terrain->getMinHeight() / terrain->getMaxHeight() seem to be unreliable ~ ulteq 12/18
    for (int x = 0; x < mSize; x++)
    {
        for (int y = 0; y < mSize; y++)
        {
            float h = mHeightData[y * mSize + x];
            mMinHeight = std::min(h, mMinHeight);
            mMaxHeight = std::max(mMaxHeight, h);
        }
    }
    mIsFlat = std::abs(mMaxHeight - mMinHeight) < std::numeric_limits<float>::epsilon();

    if (m_was_new_geometry_generated)
    {
        // update the blend maps
        if (terrainManager->GetDef().custom_material_name.empty())
        {
            for (OTCPage& page : m_spec->pages)
            {
                Ogre::Terrain* terrain = m_ogre_terrain_group->getTerrain(page.pos_x, page.pos_z);

                if (terrain != nullptr)
                {
                    this->SetupLayers(page, terrain);
                    this->SetupBlendMaps(page, terrain);
                }
            }
        }

        // always save the results when it was imported
        if (!m_spec->disable_cache)
        {
            App::GetGuiManager()->GetLoadingWindow()->setProgress(50, _L("Saving all terrain pages ..."));
            m_ogre_terrain_group->saveAllTerrains(false);
        }
    }
    else
    {
        LOG(" *** Terrain loaded from cache ***");
    }

    m_ogre_terrain_group->freeTemporaryResources();
    return true;
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
    Light* light = terrainManager->getMainLight();
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
    if (!TerrainGlobalOptions::getSingletonPtr())
    {
        OGRE_NEW TerrainGlobalOptions();
    }

    TerrainGlobalOptions* terrainOptions = TerrainGlobalOptions::getSingletonPtr();
    std::string const & custom_mat = terrainManager->GetDef().custom_material_name;
    if (!custom_mat.empty())
    {
        terrainOptions->setDefaultMaterialGenerator(
            Ogre::TerrainMaterialGeneratorPtr(new Terrn2CustomMaterial(custom_mat, false, true)));
    }
    else
    {
        terrainOptions->setDefaultMaterialGenerator(
            Ogre::TerrainMaterialGeneratorPtr(new Ogre::TerrainPSSMMaterialGenerator()));
    }
    // Configure global
    terrainOptions->setMaxPixelError(m_spec->max_pixel_error);

    // Important to set these so that the terrain knows what to use for derived (non-realtime) data
    Light* light = terrainManager->getMainLight();
    if (light)
    {
        terrainOptions->setLightMapDirection(light->getDerivedDirection());
        if (custom_mat.empty())
        {
            terrainOptions->setCompositeMapDiffuse(light->getDiffuseColour());
        }
    }
    terrainOptions->setCompositeMapAmbient(gEnv->sceneManager->getAmbientLight());

    // Configure default import settings for if we use imported image
    Ogre::Terrain::ImportData& defaultimp = m_ogre_terrain_group->getDefaultImportSettings();
    defaultimp.terrainSize  = m_spec->page_size; // the heightmap size
    defaultimp.worldSize    = m_spec->world_size; // this is the scaled up size, like 12km
    defaultimp.inputScale   = m_spec->world_size_y;
    defaultimp.minBatchSize = m_spec->batch_size_min;
    defaultimp.maxBatchSize = m_spec->batch_size_max;

    // optimizations
    TerrainPSSMMaterialGenerator::SM2Profile* matProfile = nullptr;
    if (custom_mat.empty())
    {
        matProfile = static_cast<TerrainPSSMMaterialGenerator::SM2Profile*>(terrainOptions->getDefaultMaterialGenerator()->getActiveProfile());
        if (matProfile)
        {
            matProfile->setLightmapEnabled(m_spec->lightmap_enabled);
            // Fix for OpenGL, otherwise terrains are black
            if (Root::getSingleton().getRenderSystem()->getName() == "OpenGL Rendering Subsystem")
            {
                matProfile->setLayerNormalMappingEnabled(true);
                matProfile->setLayerSpecularMappingEnabled(true);
            }
            else
            {
                matProfile->setLayerNormalMappingEnabled(m_spec->norm_map_enabled);
                matProfile->setLayerSpecularMappingEnabled(m_spec->spec_map_enabled);
            }
            matProfile->setLayerParallaxMappingEnabled(m_spec->parallax_enabled);
            matProfile->setGlobalColourMapEnabled(m_spec->global_colormap_enabled);
            matProfile->setReceiveDynamicShadowsDepth(m_spec->recv_dyn_shadows_depth);

            terrainManager->getShadowManager()->updateTerrainMaterial(matProfile);
        }
    }

    terrainOptions->setLayerBlendMapSize   (m_spec->layer_blendmap_size);
    terrainOptions->setCompositeMapSize    (m_spec->composite_map_size);
    terrainOptions->setCompositeMapDistance(m_spec->composite_map_distance);
    terrainOptions->setSkirtSize           (m_spec->skirt_size);
    terrainOptions->setLightMapSize        (m_spec->lightmap_size);

    if (custom_mat.empty())
    {
        if (matProfile->getReceiveDynamicShadowsPSSM())
        {
            terrainOptions->setCastsDynamicShadows(true);
        }
    }

    terrainOptions->setUseRayBoxDistanceCalculation(false);

    //TODO: Make this only when hydrax is enabled.
    terrainOptions->setUseVertexCompressionWhenAvailable(false);

    // HACK: Load the single page config now
    // This is how it "worked before" ~ only_a_ptr, 04/2017
    if (!m_spec->pages.empty())
    {
        this->SetupLayers(*m_spec->pages.begin(), nullptr);
    }
}

// if terrain is set, we operate on the already loaded terrain
void TerrainGeometryManager::SetupLayers(RoR::OTCPage& page, Ogre::Terrain *terrain)
{
    if (page.num_layers == 0)
        return;

    Ogre::Terrain::ImportData& defaultimp = m_ogre_terrain_group->getDefaultImportSettings();

    if (!terrain)
        defaultimp.layerList.resize(page.num_layers);

    int layer_idx = 0;

    for (OTCLayer& layer : page.layers)
    {
        if (!terrain)
        {
            defaultimp.layerList[layer_idx].worldSize = layer.world_size;
            defaultimp.layerList[layer_idx].textureNames.push_back(layer.diffusespecular_filename);
            defaultimp.layerList[layer_idx].textureNames.push_back(layer.normalheight_filename);
        }
        else
        {
            terrain->setLayerWorldSize(layer_idx, layer.world_size);
            terrain->setLayerTextureName(layer_idx, 0, layer.diffusespecular_filename);
            terrain->setLayerTextureName(layer_idx, 1, layer.normalheight_filename);
        }

        layer_idx++;
    }
    LOG("done loading page: loaded " + TOSTRING(layer_idx) + " layers");
}

void TerrainGeometryManager::SetupBlendMaps(OTCPage& page, Ogre::Terrain* terrain )
{
    const int layerCount = terrain->getLayerCount();
    auto layer_def_itor = page.layers.begin();
    ++layer_def_itor;
    for (int i = 1; i < layerCount; i++)
    {
        if (layer_def_itor->blendmap_filename.empty())
            continue;

        Ogre::Image img;
        try
        {
            img.load(layer_def_itor->blendmap_filename, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
        }
        catch (Exception& e)
        {
            LOG("Error loading blendmap: " + layer_def_itor->blendmap_filename + " : " + e.getFullDescription());
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
                const float alpha = layer_def_itor->alpha;
                if (layer_def_itor->blend_mode == 'R')
                    *ptr++ = c.r * alpha;
                else if (layer_def_itor->blend_mode == 'G')
                    *ptr++ = c.g * alpha;
                else if (layer_def_itor->blend_mode == 'B')
                    *ptr++ = c.b * alpha;
                else if (layer_def_itor->blend_mode == 'A')
                    *ptr++ = c.a * alpha;
            }
        }
        blendmap->dirty();
        blendmap->update();
        ++layer_def_itor;
    }

    if (m_spec->blendmap_dbg_enabled)
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
bool LoadHeightmap(OTCPage& page, Image& img)
{
    if (page.heightmap_filename.empty())
    {
        LOG("[RoR|Terrain] Empty Heightmap provided in OTC, please use 'Flat=1' instead");
        return false;
    }

    if (page.heightmap_filename.find(".raw") != String::npos)
    {
        // load raw data
        DataStreamPtr stream = ResourceGroupManager::getSingleton().openResource(page.heightmap_filename);
        LOG("[RoR|Terrain] loading RAW image: " + TOSTRING(stream->size()) + " / " + TOSTRING(page.raw_size*page.raw_size*page.raw_bpp));
        PixelFormat pix_format = (page.raw_bpp == 2) ? PF_L16 : PF_L8;
        img.loadRawData(stream, page.raw_size, page.raw_size, 1, pix_format);
    }
    else
    {
        img.load(page.heightmap_filename, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    }

    if (page.raw_flip_x)
        img.flipAroundX();
    if (page.raw_flip_y)
        img.flipAroundY();

    return true;
}

void TerrainGeometryManager::SetupGeometry(RoR::OTCPage& page, bool flat)
{
    if (flat)
    {
        // very simple, no height data to load at all
        m_ogre_terrain_group->defineTerrain(page.pos_x, page.pos_z, 0.0f);
        return;
    }

    const std::string page_cache_filename = m_ogre_terrain_group->generateFilename(page.pos_x, page.pos_z);
    const std::string res_group = m_ogre_terrain_group->getResourceGroup();
    if (!m_spec->disable_cache && ResourceGroupManager::getSingleton().resourceExists(res_group, page_cache_filename))
    {
        // load from cache
        m_ogre_terrain_group->defineTerrain(page.pos_x, page.pos_z);
    }
    else
    {
        Image img;
        if (LoadHeightmap(page, img))
        {
            m_ogre_terrain_group->defineTerrain(page.pos_x, page.pos_z, &img);
            m_was_new_geometry_generated = true;
        }
        else
        {
            // fall back to no heightmap
            m_ogre_terrain_group->defineTerrain(page.pos_x, page.pos_z, 0.0f);
        }
    }
}

Ogre::Vector3 TerrainGeometryManager::getMaxTerrainSize()
{
    return Vector3(m_spec->world_size_x, mMaxHeight, m_spec->world_size_z);
}

