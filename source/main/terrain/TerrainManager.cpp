/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal

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

#include "TerrainManager.h"

#include "Application.h"
#include "BeamData.h"
#include "BeamFactory.h"
#include "Collisions.h"
#include "Dashboard.h"
#include "EnvironmentMap.h"
#include "ErrorUtils.h"
#include "GlowMaterialListener.h"
#include "GUIManager.h"
#include "GUI_LoadingWindow.h"
#include "HDRListener.h"
#include "HydraxWater.h"
#include "Language.h"
#include "RoRFrameListener.h"
#include "Scripting.h"
#include "Settings.h"
#include "ShadowManager.h"
#include "SkyManager.h"
#include "SoundScriptManager.h"
#include "SurveyMapManager.h"
#include "TerrainGeometryManager.h"
#include "TerrainObjectManager.h"
#include "Utils.h"
#include "Water.h"

#include <Terrain/OgreTerrainPaging.h>

using namespace RoR;
using namespace Ogre;

TerrainManager::TerrainManager() :
    m_terrain_config()
    , character(0)
    , collisions(0)
    , dashboard(0)
    , envmap(0)
    , geometry_manager(0)
    , hdr_listener(0)
    , object_manager(0)
    , shadow_manager(0)
    , sky_manager(0)
    , survey_map(0)
    , water(0)
    , authors()
    , ambient_color(ColourValue::White)
    , category_id(0)
    , fade_color(ColourValue::Black)
    , far_clip(1000)
    , file_hash("")
    , gravity(DEFAULT_GRAVITY)
    , guid("")
    , main_light(0)
    , ogre_terrain_config_filename("")
    , paged_detail_factor(0.0f)
    , paged_mode(0)
    , start_position(Vector3::ZERO)
    , terrain_name("")
    , version(1)
    , water_line(0.0f)
{
}

TerrainManager::~TerrainManager()
{
    m_terrain_config.clear();

    //I think that the order is important

    if (sky_manager != nullptr)
    {
        delete(sky_manager);
        gEnv->sky = nullptr;
        sky_manager = nullptr;
    }

    if (main_light != nullptr)
    {
        gEnv->sceneManager->destroyAllLights();
        main_light = nullptr;
    }

    if (envmap != nullptr)
    {
        delete(envmap);
        envmap = nullptr;
    }

    if (dashboard != nullptr)
    {
        delete(dashboard);
        dashboard = nullptr;
    }

    if (water != nullptr)
    {
        delete(water);
        water = nullptr;
    }

    if (object_manager != nullptr)
    {
        delete(object_manager);
        object_manager = nullptr;
    }

    if (geometry_manager != nullptr)
    {
        delete(geometry_manager);
        geometry_manager = nullptr;
    }

    if (shadow_manager != nullptr)
    {
        delete(shadow_manager);
        shadow_manager = nullptr;
    }

    if (survey_map != nullptr)
    {
        delete survey_map;
        survey_map = nullptr;
    }
}

// some shortcut to remove ugly code
#ifdef USE_MYGUI
#   define PROGRESS_WINDOW(x, y) { LOG(Ogre::String("  ## ") + y); RoR::App::GetGuiManager()->GetLoadingWindow()->setProgress(x, y); }
#else
#   define PROGRESS_WINDOW(x, y) { LOG(Ogre::String("  ## ") + y) }
#endif //USE_MYGUI

void TerrainManager::loadTerrainConfigBasics(Ogre::DataStreamPtr& ds)
{
    // now generate the hash of it
    generateHashFromDataStream(ds, file_hash);

    m_terrain_config.load(ds, "\t:=", true);

    // read in the settings
    terrain_name = m_terrain_config.GetStringEx("Name", "General");
    if (terrain_name.empty())
    {
        ErrorUtils::ShowError(_L("Terrain loading error"), _L("the terrain name cannot be empty"));
        exit(125);
    }

    ogre_terrain_config_filename = m_terrain_config.GetStringEx("GeometryConfig", "General");
    // otc = ogre terrain config
    if (ogre_terrain_config_filename.find(".otc") == String::npos)
    {
        ErrorUtils::ShowError(_L("Terrain loading error"), _L("the new terrain mode only supports .otc configurations"));
        exit(125);
    }

    ambient_color = m_terrain_config.GetColourValue("AmbientColor", "General", ColourValue::White);
    category_id = m_terrain_config.GetInt("CategoryID", "General", 129);
    guid = m_terrain_config.GetStringEx("GUID", "General");
    start_position = StringConverter::parseVector3(m_terrain_config.GetStringEx("StartPosition", "General"), Vector3(512.0f, 0.0f, 512.0f));
    version = m_terrain_config.GetInt("Version", "General", 1);
    gravity = m_terrain_config.GetFloat("Gravity", "General", -9.81);

    // parse author info
    Ogre::ConfigFile::SettingsIterator it = m_terrain_config.getSettingsIterator("Authors");

    authors.clear();

    while (it.hasMoreElements())
    {
        String type = RoR::Utils::SanitizeUtf8String(it.peekNextKey()); // e.g. terrain
        String name = RoR::Utils::SanitizeUtf8String(it.peekNextValue()); // e.g. john doe

        if (!name.empty())
        {
            authorinfo_t author;

            author.type = type;
            author.name = name;

            authors.push_back(author);
        }

        it.moveNext();
    }
}

void TerrainManager::loadTerrain(String filename)
{
    DataStreamPtr ds;

    try
    {
        String group = ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
        ds = ResourceGroupManager::getSingleton().openResource(filename, group);
    }
    catch (...)
    {
        LOG("Terrain not found: " + String(filename));
        ErrorUtils::ShowError(_L("Terrain loading error"), _L("Terrain not found: ") + filename);
        exit(125);
    }

    PROGRESS_WINDOW(10, _L("Loading Terrain Configuration"));

    LOG(" ===== LOADING TERRAIN " + filename);

    loadTerrainConfigBasics(ds);

    // then, init the subsystems, order is important :)
    initSubSystems();

    fixCompositorClearColor();

    LOG(" ===== LOADING TERRAIN GEOMETRY " + filename);

    // load the terrain geometry
    PROGRESS_WINDOW(80, _L("Loading Terrain Geometry"));
    geometry_manager->loadOgreTerrainConfig(ogre_terrain_config_filename);

    LOG(" ===== LOADING TERRAIN WATER " + filename);
    // must happen here
    initWater();

    LOG(" ===== LOADING TERRAIN OBJECTS " + filename);

    PROGRESS_WINDOW(90, _L("Loading Terrain Objects"));
    loadTerrainObjects();

    collisions->printStats();

    // bake the decals
    //finishTerrainDecal();

    // init things after loading the terrain
    initTerrainCollisions();

    // init the survey map
    if (!RoR::App::GetGfxMinimapDisabled())
    {
        PROGRESS_WINDOW(45, _L("Initializing Overview Map Subsystem"));
        initSurveyMap();
    }

    collisions->finishLoadingTerrain();
    LOG(" ===== TERRAIN LOADING DONE " + filename);
}

void TerrainManager::initSubSystems()
{
    // geometry - ogre terrain things
    initShadows();

    PROGRESS_WINDOW(15, _L("Initializing Geometry Subsystem"));
    initGeometry();

    // objects  - .odef support
    PROGRESS_WINDOW(17, _L("Initializing Object Subsystem"));
    initObjects();

    PROGRESS_WINDOW(19, _L("Initializing Collision Subsystem"));
    initCollisions();

    PROGRESS_WINDOW(19, _L("Initializing Script Subsystem"));
    initScripting();

    PROGRESS_WINDOW(21, _L("Initializing Shadow Subsystem"));

    PROGRESS_WINDOW(25, _L("Initializing Camera Subsystem"));
    initCamera();

    // sky, must come after camera due to far_clip
    PROGRESS_WINDOW(23, _L("Initializing Sky Subsystem"));
    initSkySubSystem();

    PROGRESS_WINDOW(27, _L("Initializing Light Subsystem"));
    initLight();

    if (App::GetGfxSkyMode() != App::GFX_SKY_CAELUM) //Caelum has its own fog management
    {
        PROGRESS_WINDOW(29, _L("Initializing Fog Subsystem"));
        initFog();
    }

    PROGRESS_WINDOW(31, _L("Initializing Vegetation Subsystem"));
    initVegetation();

    // water must be done later on
    //PROGRESS_WINDOW(33, _L("Initializing Water Subsystem"));
    //initWater();

    if (App::GetGfxEnableHdr())
    {
        PROGRESS_WINDOW(35, _L("Initializing HDR Subsystem"));
        initHDR();
    }
    if (RoR::App::GetGfxEnableGlow())
    {
        PROGRESS_WINDOW(37, _L("Initializing Glow Subsystem"));
        initGlow();
    }
    if (BSETTING("Motion blur", false))
    {
        PROGRESS_WINDOW(39, _L("Initializing Motion Blur Subsystem"));
        initMotionBlur();
    }
    if (RoR::App::GetGfxEnableSunburn())
    {
        PROGRESS_WINDOW(41, _L("Initializing Sunburn Subsystem"));
        initSunburn();
    }
    if (!BSETTING("Envmapdisable", false))
    {
        PROGRESS_WINDOW(43, _L("Initializing Environment Map Subsystem"));
        initEnvironmentMap();
    }

    PROGRESS_WINDOW(47, _L("Initializing Dashboards Subsystem"));
    initDashboards();
}

void TerrainManager::initCamera()
{
    gEnv->mainCamera->getViewport()->setBackgroundColour(ambient_color);
    gEnv->mainCamera->setPosition(start_position);

    far_clip = App::GetGfxSightRange();

    if (far_clip < UNLIMITED_SIGHTRANGE)
        gEnv->mainCamera->setFarClipDistance(far_clip);
    else
    {
        // disabled in global config
        if (App::GetGfxWaterMode() != App::GFX_WATER_HYDRAX)
            gEnv->mainCamera->setFarClipDistance(0); //Unlimited
        else
            gEnv->mainCamera->setFarClipDistance(9999 * 6); //Unlimited for hydrax and stuff
    }
}

void TerrainManager::initSkySubSystem()
{
#ifdef USE_CAELUM
    // Caelum skies
    if (App::GetGfxSkyMode() == App::GFX_SKY_CAELUM)
    {
        sky_manager = new SkyManager();
        gEnv->sky = sky_manager;

        // try to load caelum config
        String caelumConfig = m_terrain_config.GetStringEx("CaelumConfigFile", "General");

        if (!caelumConfig.empty() && ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(caelumConfig))
        {
            // config provided and existing, use it :)
            int caelumFogStart = m_terrain_config.GetInt("CaelumFogStart", "General", -1);
            int caelumFogEnd = m_terrain_config.GetInt("CaelumFogEnd", "General", -1);
            sky_manager->loadScript(caelumConfig, caelumFogStart, caelumFogEnd);
        }
        else
        {
            // no config provided, fall back to the default one
            sky_manager->loadScript("ror_default_sky");
        }
    }
    else
#endif //USE_CAELUM
    {
        String sandStormConfig = m_terrain_config.GetStringEx("SandStormCubeMap", "General");

        if (!sandStormConfig.empty())
        {
            // use custom
            gEnv->sceneManager->setSkyBox(true, sandStormConfig, 100, true);
        }
        else
        {
            // use default
            gEnv->sceneManager->setSkyBox(true, "tracks/skyboxcol", 100, true);
        }
    }
}

void TerrainManager::initLight()
{
    if (App::GetGfxSkyMode() == App::GFX_SKY_CAELUM)
    {
#ifdef USE_CAELUM
        main_light = sky_manager->getMainLight();
#endif
    }
    else
    {
        // screw caelum, we will roll our own light

        // Create a light
        main_light = gEnv->sceneManager->createLight("MainLight");
        //directional light for shadow
        main_light->setType(Light::LT_DIRECTIONAL);
        main_light->setDirection(Ogre::Vector3(0.785, -0.423, 0.453).normalisedCopy());

        main_light->setDiffuseColour(ambient_color);
        main_light->setSpecularColour(ambient_color);
        main_light->setCastShadows(true);
        main_light->setShadowFarDistance(1000.0f);
        main_light->setShadowNearClipDistance(-1);
    }
}

void TerrainManager::initFog()
{
    if (far_clip >= UNLIMITED_SIGHTRANGE)
        gEnv->sceneManager->setFog(FOG_NONE);
    else
        gEnv->sceneManager->setFog(FOG_LINEAR, ambient_color, 0.000f, far_clip * 0.65f, far_clip * 0.9);
}

void TerrainManager::initVegetation()
{
    paged_mode = App::GetGfxVegetationMode();

    switch (paged_mode)
    {
    case 0:
        paged_detail_factor = 0.001f;
        break;
    case 1:
        paged_detail_factor = 0.2f;
        break;
    case 2:
        paged_detail_factor = 0.5f;
        break;
    case 3:
        paged_detail_factor = 1.0f;
        break;
    default:
        paged_mode = 0;
        paged_detail_factor = 0.0f;
        break;
    }
}

void TerrainManager::initHDR()
{
    Viewport* vp = gEnv->mainCamera->getViewport();
    CompositorInstance* instance = CompositorManager::getSingleton().addCompositor(vp, "HDR", 0);
    CompositorManager::getSingleton().setCompositorEnabled(vp, "HDR", true);

    // HDR needs a special listener
    hdr_listener = new HDRListener();
    instance->addListener(hdr_listener);
    hdr_listener->notifyViewportSize(vp->getActualWidth(), vp->getActualHeight());
    hdr_listener->notifyCompositor(instance);
}

void TerrainManager::initGlow()
{
    CompositorManager::getSingleton().addCompositor(gEnv->mainCamera->getViewport(), "Glow");
    CompositorManager::getSingleton().setCompositorEnabled(gEnv->mainCamera->getViewport(), "Glow", true);
    GlowMaterialListener* gml = new GlowMaterialListener();
    MaterialManager::getSingleton().addListener(gml);
}

void TerrainManager::initMotionBlur()
{
    // Motion blur effect taken from Ogre 1.8 Compositor demos
    Ogre::CompositorPtr comp3 = Ogre::CompositorManager::getSingleton().create("MotionBlur", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    {
        Ogre::CompositionTechnique* t = comp3->createTechnique();
        {
            Ogre::CompositionTechnique::TextureDefinition* def = t->createTextureDefinition("scene");
            def->width = 0;
            def->height = 0;
            def->formatList.push_back(Ogre::PF_R8G8B8);
        }
        {
            Ogre::CompositionTechnique::TextureDefinition* def = t->createTextureDefinition("sum");
            def->width = 0;
            def->height = 0;
            def->formatList.push_back(Ogre::PF_R8G8B8);
        }
        {
            Ogre::CompositionTechnique::TextureDefinition* def = t->createTextureDefinition("temp");
            def->width = 0;
            def->height = 0;
            def->formatList.push_back(Ogre::PF_R8G8B8);
        }
        /// Render scene
        {
            Ogre::CompositionTargetPass* tp = t->createTargetPass();
            tp->setInputMode(Ogre::CompositionTargetPass::IM_PREVIOUS);
            tp->setOutputName("scene");
        }
        /// Initialisation pass for sum texture
        {
            Ogre::CompositionTargetPass* tp = t->createTargetPass();
            tp->setInputMode(Ogre::CompositionTargetPass::IM_PREVIOUS);
            tp->setOutputName("sum");
            tp->setOnlyInitial(true);
        }
        /// Do the motion blur
        {
            Ogre::CompositionTargetPass* tp = t->createTargetPass();
            tp->setInputMode(Ogre::CompositionTargetPass::IM_NONE);
            tp->setOutputName("temp");
            {
                Ogre::CompositionPass* pass = tp->createPass();
                pass->setType(Ogre::CompositionPass::PT_RENDERQUAD);
                pass->setMaterialName("Ogre/Compositor/Combine");
                pass->setInput(0, "scene");
                pass->setInput(1, "sum");
            }
        }
        /// Copy back sum texture
        {
            Ogre::CompositionTargetPass* tp = t->createTargetPass();
            tp->setInputMode(Ogre::CompositionTargetPass::IM_NONE);
            tp->setOutputName("sum");
            {
                Ogre::CompositionPass* pass = tp->createPass();
                pass->setType(Ogre::CompositionPass::PT_RENDERQUAD);
                pass->setMaterialName("Ogre/Compositor/Copyback");
                pass->setInput(0, "temp");
            }
        }
        /// Display result
        {
            Ogre::CompositionTargetPass* tp = t->getOutputTargetPass();
            tp->setInputMode(Ogre::CompositionTargetPass::IM_NONE);
            {
                Ogre::CompositionPass* pass = tp->createPass();
                pass->setType(Ogre::CompositionPass::PT_RENDERQUAD);
                pass->setMaterialName("Ogre/Compositor/MotionBlur");
                pass->setInput(0, "sum");
            }
        }
    }

    Ogre::CompositorManager::getSingleton().addCompositor(gEnv->mainCamera->getViewport(), "MotionBlur");
    Ogre::CompositorManager::getSingleton().setCompositorEnabled(gEnv->mainCamera->getViewport(), "MotionBlur", true);
}

void TerrainManager::initSunburn()
{
    CompositorManager::getSingleton().addCompositor(gEnv->mainCamera->getViewport(), "Sunburn");
    CompositorManager::getSingleton().setCompositorEnabled(gEnv->mainCamera->getViewport(), "Sunburn", true);
}

void TerrainManager::fixCompositorClearColor()
{
    // hack
    // now with extensive error checking
    if (CompositorManager::getSingleton().hasCompositorChain(gEnv->mainCamera->getViewport()))
    {
        //	//CompositorManager::getSingleton().getCompositorChain(gEnv->ogreCamera->getViewport())->getCompositor(0)->getTechnique()->getOutputTargetPass()->getPass(0)->setClearColour(fade_color);
        CompositorInstance* co = CompositorManager::getSingleton().getCompositorChain(gEnv->mainCamera->getViewport())->_getOriginalSceneCompositor();
        if (co)
        {
            CompositionTechnique* ct = co->getTechnique();
            if (ct)
            {
                CompositionTargetPass* ctp = ct->getOutputTargetPass();
                if (ctp)
                {
                    CompositionPass* p = ctp->getPass(0);
                    if (p)
                    {
                        p->setClearColour(fade_color);
                    }
                }
            }
        }
    }
}

void TerrainManager::initWater()
{
    // disabled in global config
    if (App::GetGfxWaterMode() == App::GFX_WATER_NONE)
        return;

    // disabled in map config
    bool has_water = m_terrain_config.GetBool("Water", "General", false);
    if (!has_water)
    {
        return;
    }

    if (App::GetGfxWaterMode() == App::GFX_WATER_HYDRAX)
    {
        // try to load hydrax config
        String hydraxConfig = m_terrain_config.GetStringEx("HydraxConfigFile", "General");

        if (!hydraxConfig.empty() && ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(hydraxConfig))
        {
            hw = new HydraxWater(m_terrain_config, hydraxConfig);
        }
        else
        {
            // no config provided, fall back to the default one
            hw = new HydraxWater(m_terrain_config);
        }

        water = hw;

        //Apply depth technique to the terrain
        TerrainGroup::TerrainIterator ti = geometry_manager->getTerrainGroup()->getTerrainIterator();
        while (ti.hasMoreElements())
        {
            Terrain* t = ti.getNext()->instance;
            MaterialPtr ptr = t->getMaterial();
            hw->GetHydrax()->getMaterialManager()->addDepthTechnique(ptr->createTechnique());
        }
    }
    else
    {
        if (water == nullptr)
            water = new Water(m_terrain_config);
        else if (water != nullptr)
        {
            delete(water);
            water = new Water(m_terrain_config);
        }
    }
}

void TerrainManager::initEnvironmentMap()
{
    envmap = new Envmap();
}

void TerrainManager::initDashboards()
{
    dashboard = new Dashboard();
}

void TerrainManager::initShadows()
{
    shadow_manager = new ShadowManager();
    shadow_manager->loadConfiguration();
}

void TerrainManager::loadTerrainObjects()
{
    try
    {
        Ogre::ConfigFile::SettingsIterator objectsIterator = m_terrain_config.getSettingsIterator("Objects");

        while (objectsIterator.hasMoreElements())
        {
            String sname = objectsIterator.peekNextKey();
            StringUtil::trim(sname);

            object_manager->loadObjectConfigFile(sname);
            objectsIterator.moveNext();
        }
    }
    catch (...)
    {
        // no objects found
    }

    // bakes the geometry and things
    object_manager->postLoad();
}

void TerrainManager::initCollisions()
{
    collisions = new Collisions();
    gEnv->collisions = collisions;
}

void TerrainManager::initTerrainCollisions()
{
    String tractionMapConfig = m_terrain_config.GetStringEx("TractionMap", "General");
    if (!tractionMapConfig.empty())
    {
        gEnv->collisions->setupLandUse(tractionMapConfig.c_str());
    }
}

bool TerrainManager::update(float dt)
{
    if (object_manager)
        object_manager->update(dt);

    if (geometry_manager)
        geometry_manager->update(dt);

    return true;
}

void TerrainManager::initScripting()
{
#ifdef USE_ANGELSCRIPT
    bool loaded = false;

    // only load terrain scripts while not in multiplayer
    if (RoR::App::GetActiveMpState() != RoR::App::MP_STATE_CONNECTED)
    {
        try
        {
            Ogre::ConfigFile::SettingsIterator objectsIterator = m_terrain_config.getSettingsIterator("Scripts");
            while (objectsIterator.hasMoreElements())
            {
                String sname = objectsIterator.peekNextKey();
                StringUtil::trim(sname);
                String svalue = objectsIterator.getNext();
                StringUtil::trim(svalue);

                if (!ScriptEngine::getSingleton().loadScript(sname))
                    loaded = true;
            }
        }
        catch (...)
        {
            // simply no script section
            //LOG("Exception while trying load script: " + e.getFullDescription());
        }
    }

    if (!loaded)
    {
        // load a default script that does the most basic things
        ScriptEngine::getSingleton().loadScript("default.as");
    }
    // finally activate AS logging, so we dont spam the users screen with initialization messages
    ScriptEngine::getSingleton().activateLogging();
#endif //USE_ANGELSCRIPT
}

void TerrainManager::setGravity(float value)
{
    gravity = value;
    BeamFactory::getSingleton().recalcGravityMasses();
}

void TerrainManager::initSurveyMap()
{
    survey_map = new SurveyMapManager();

    // Uses the terrain composite material as survey map background image
#if 0
	if (geometry_manager)
		survey_map->setMapTexture(geometry_manager->getCompositeMaterialName());
#endif
}

void TerrainManager::initGeometry()
{
    geometry_manager = new TerrainGeometryManager(this);
}

void TerrainManager::initObjects()
{
    object_manager = new TerrainObjectManager(this);
}

Ogre::Vector3 TerrainManager::getMaxTerrainSize()
{
    if (!geometry_manager)
        return Vector3::ZERO;
    return geometry_manager->getMaxTerrainSize();
}

IHeightFinder* TerrainManager::getHeightFinder()
{
    return geometry_manager;
}

SkyManager* TerrainManager::getSkyManager()
{
    return sky_manager;
}

void TerrainManager::loadPreloadedTrucks()
{
    if (object_manager)
        object_manager->loadPreloadedTrucks();
}

bool TerrainManager::hasPreloadedTrucks()
{
    if (object_manager)
        return object_manager->hasPreloadedTrucks();
    return false;
}

std::vector<authorinfo_t>& TerrainManager::GetAuthors()
{
    return authors;
}
