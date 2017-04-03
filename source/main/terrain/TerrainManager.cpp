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
#include "GUI_TeleportWindow.h"
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
#include "Terrn2Deployment.h"
#include "Utils.h"
#include "Water.h"

#include <Terrain/OgreTerrainPaging.h>
#include <Terrain/OgreTerrainGroup.h>

using namespace RoR;
using namespace Ogre;

TerrainManager::TerrainManager(RoRFrameListener* sim_controller) :
      m_sim_controller(sim_controller)
    , character(0)
    , m_terrn_collisions(nullptr)
    , dashboard(0)
    , envmap(0)
    , geometry_manager(0)
    , hdr_listener(0)
    , object_manager(0)
    , shadow_manager(0)
    , sky_manager(0)
    , m_survey_map(0)
    , water(0)
    , far_clip(1000)
    , main_light(0)
    , paged_detail_factor(0.0f)
    , paged_mode(0)
    , m_terrn_gravity(DEFAULT_GRAVITY)
{
}

TerrainManager::~TerrainManager()
{

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

    if (m_survey_map != nullptr)
    {
        delete m_survey_map;
        m_survey_map = nullptr;
    }
}

// some shortcut to remove ugly code
#   define PROGRESS_WINDOW(x, y) { LOG(Ogre::String("  ## ") + y); RoR::App::GetGuiManager()->GetLoadingWindow()->setProgress(x, y); }

void TerrainManager::LoadTerrain(std::string const & filename)
{
    PROGRESS_WINDOW(10, _L("Deploying terrain"));

    LOG(" ===== DEPLOYING TERRAIN " + filename);

    Terrn2Deployer deployer;
    deployer.DeployTerrn2(filename);

    Json::Value& j_terrn = deployer.GetJson(); // TEST MODE - don't save to cache, just use the output directly

    PROGRESS_WINDOW(10, _L("Loading Terrain Configuration"));

    LOG(" ===== LOADING TERRAIN " + filename);

    m_terrn_gravity = j_terrn["terrn2"]["gravity"].asFloat();
    Json::Value j_spawn_pos = j_terrn["terrn2"]["start_position"];
    m_spawn_pos = Ogre::Vector3(j_spawn_pos["x"].asFloat(), j_spawn_pos["y"].asFloat(), j_spawn_pos["z"].asFloat());

    // then, init the subsystems, order is important :)
    this->InitSubSystems(j_terrn);

    fixCompositorClearColor();

    LOG(" ===== LOADING TERRAIN GEOMETRY " + filename);

    // load the terrain geometry
    PROGRESS_WINDOW(80, _L("Loading Terrain Geometry"));
    geometry_manager->InitTerrain(&j_terrn);

    LOG(" ===== LOADING TERRAIN WATER " + filename);
    // must happen here
    initWater(j_terrn);

    LOG(" ===== LOADING TERRAIN OBJECTS " + filename);

    PROGRESS_WINDOW(90, _L("Loading Terrain Objects"));
    loadTerrainObjects(j_terrn);

    m_terrn_collisions->printStats();

    // init things after loading the terrain
    if (j_terrn["terrn2"]["traction_map_file"] != Json::nullValue)
    {
        gEnv->collisions->setupLandUse(j_terrn["traction_map_file"].asString().c_str());
    }

    // init the survey map
    if (!RoR::App::GetGfxMinimapDisabled())
    {
        PROGRESS_WINDOW(45, _L("Initializing Overview Map Subsystem"));
        m_survey_map = new SurveyMapManager();
    }

    m_terrn_collisions->finishLoadingTerrain();

    App::GetGuiManager()->GetTeleport()->SetupMap(
        m_sim_controller,
        &j_terrn,
        gEnv->terrainManager->getMaxTerrainSize(),
        gEnv->terrainManager->GetMinimapTextureName());

    LOG(" ===== TERRAIN LOADING DONE " + filename);
}

void TerrainManager::InitSubSystems(Json::Value& j_terrn)
{
    Json::Value j_ambient_color = j_terrn["terrn2"]["ambient_color"];
    Ogre::ColourValue ambient_color(
        j_ambient_color["r"].asFloat(),
        j_ambient_color["g"].asFloat(),
        j_ambient_color["b"].asFloat(),
        j_ambient_color["a"].asFloat()
    );

    // geometry - ogre terrain things
    this->initShadows();

    PROGRESS_WINDOW(15, _L("Initializing Geometry Subsystem"));
    this->initGeometry();

    // objects  - .odef support
    PROGRESS_WINDOW(17, _L("Initializing Object Subsystem"));
    this->initObjects();

    PROGRESS_WINDOW(19, _L("Initializing Collision Subsystem"));
    m_terrn_collisions = new Collisions();
    gEnv->collisions = m_terrn_collisions;

    PROGRESS_WINDOW(19, _L("Initializing Script Subsystem"));
    initScripting(j_terrn);

    PROGRESS_WINDOW(25, _L("Initializing Camera Subsystem"));
    initCamera(j_terrn, ambient_color);

    // sky, must come after camera due to far_clip
    PROGRESS_WINDOW(23, _L("Initializing Sky Subsystem"));
    initSkySubSystem(j_terrn);

    PROGRESS_WINDOW(27, _L("Initializing Light Subsystem"));
    initLight(ambient_color);

    if (App::GetGfxSkyMode() != App::GFX_SKY_CAELUM) //Caelum has its own fog management
    {
        PROGRESS_WINDOW(29, _L("Initializing Fog Subsystem"));
        if (far_clip >= UNLIMITED_SIGHTRANGE)
            gEnv->sceneManager->setFog(FOG_NONE);
        else
            gEnv->sceneManager->setFog(FOG_LINEAR, ambient_color, 0.000f, far_clip * 0.65f, far_clip*0.9);
    }

    PROGRESS_WINDOW(31, _L("Initializing Vegetation Subsystem"));
    initVegetation();

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
        envmap = new Envmap();
    }

    PROGRESS_WINDOW(47, _L("Initializing Dashboards Subsystem"));
    initDashboards();
}

void TerrainManager::initCamera(Json::Value& j_terrn, Ogre::ColourValue const & ambient_color)
{
    gEnv->mainCamera->getViewport()->setBackgroundColour(ambient_color);
    gEnv->mainCamera->setPosition(m_spawn_pos);

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

void TerrainManager::initSkySubSystem(Json::Value& j_terrn)
{
#ifdef USE_CAELUM
    // Caelum skies
    if (App::GetGfxSkyMode() == App::GFX_SKY_CAELUM)
    {
        sky_manager = new SkyManager();
        gEnv->sky = sky_manager;

        if (j_terrn["terrn2"]["caelum_config"] == Json::nullValue)
            return;

        // try to load caelum config
        const std::string caelum_file = j_terrn["terrn2"]["caelum_config"].asString();
        if (ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(caelum_file))
        {
            // config provided and existing, use it :)
            const int fog_start = j_terrn["terrn2"]["caelum_fog_start"].asInt();
            const int fog_end   = j_terrn["terrn2"]["caelum_fog_end"].asInt();
            sky_manager->loadScript(caelum_file, fog_start, fog_end);
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
        if (j_terrn["terrn2"]["cubemap_config"] != Json::nullValue)
        {
            // use custom
            gEnv->sceneManager->setSkyBox(true, j_terrn["terrn2"]["cubemap_config"].asString(), 100, true);
        }
        else
        {
            // use default
            gEnv->sceneManager->setSkyBox(true, "tracks/skyboxcol", 100, true);
        }
    }
}

void TerrainManager::initLight(Ogre::ColourValue const & ambient_color)
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
                        p->setClearColour(Ogre::ColourValue::Black);
                    }
                }
            }
        }
    }
}

void TerrainManager::initWater(Json::Value& j_terrn)
{
    // disabled in global config
    if (App::GetGfxWaterMode() == App::GFX_WATER_NONE)
        return;

    // disabled in map config
    if (! j_terrn["terrn2"]["has_water"].asBool())
        return;

    if (App::GetGfxWaterMode() == App::GFX_WATER_HYDRAX)
    {
        const float water_height = j_terrn["terrn2"]["water_height"].asFloat();

        // try to load hydrax config
        if ((j_terrn["terrn2"]["hydrax_conf_file"] != Json::nullValue)
            && ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(j_terrn["terrn2"]["hydrax_conf_file"].asString()))
        {
            hw = new HydraxWater(water_height, j_terrn["terrn2"]["hydrax_conf_file"].asString());
        }
        else
        {
            // no config provided, fall back to the default one
            hw = new HydraxWater(water_height);
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
           water = new Water();
        else if (water != nullptr)
        {
            delete(water);
            water = new Water();
        }
    }
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

void TerrainManager::loadTerrainObjects(Json::Value& j_terrn)
{
    for (Json::Value& j_tobj_filename : j_terrn["terrn2"]["tobj_files"])
    {
        object_manager->loadObjectConfigFile(j_tobj_filename.asString());
    }

    object_manager->postLoad(); // bakes the geometry and things
}

bool TerrainManager::update(float dt)
{
    if (object_manager)
        object_manager->update(dt);

    if (geometry_manager)
        geometry_manager->update(dt);

    return true;
}

void TerrainManager::initScripting(Json::Value& j_terrn)
{
#ifdef USE_ANGELSCRIPT
    bool loaded = false;

    // only load terrain scripts while not in multiplayer
    if (RoR::App::GetActiveMpState() != RoR::App::MP_STATE_CONNECTED)
    {
        for (const Json::Value& as_filename : j_terrn["terrn2"]["as_scripts"])
        {
            if (ScriptEngine::getSingleton().loadScript(as_filename.asString()) == 0)
                loaded = true;
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
    m_terrn_gravity = value;
    BeamFactory::getSingleton().recalcGravityMasses();
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

std::string TerrainManager::GetMinimapTextureName()
{
    if (m_survey_map != nullptr) // Can be disabled by user
        return m_survey_map->GetMinimapTextureName();
    else
        return "";
}

