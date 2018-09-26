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
#include <Terrain/OgreTerrainGroup.h>

using namespace RoR;
using namespace Ogre;

TerrainManager::TerrainManager()
    : m_collisions(0)
    , m_dashboard(0)
    , m_geometry_manager(0)
    , m_hdr_listener(0)
    , m_object_manager(0)
    , m_shadow_manager(0)
    , m_sky_manager(0)
    , SkyX_manager(0)
    , m_survey_map(0)
    , m_water(0)
    , m_sight_range(1000)
    , m_main_light(0)
    , m_paged_detail_factor(0.0f)
    , m_paged_mode(0)
    , m_cur_gravity(DEFAULT_GRAVITY)
{
}

TerrainManager::~TerrainManager()
{

    //I think that the order is important

    if (m_sky_manager != nullptr)
    {
        delete(m_sky_manager);
        m_sky_manager = nullptr;
    }

    if (SkyX_manager != nullptr)
    {
        delete(SkyX_manager);
        gEnv->SkyX = nullptr;
        SkyX_manager = nullptr;
    }

    if (m_main_light != nullptr)
    {
        gEnv->sceneManager->destroyAllLights();
        m_main_light = nullptr;
    }

    if (m_dashboard != nullptr)
    {
        delete(m_dashboard);
        m_dashboard = nullptr;
    }

    if (m_water != nullptr)
    {
        delete(m_water);
        m_water = nullptr;
    }

    if (m_object_manager != nullptr)
    {
        delete(m_object_manager);
        m_object_manager = nullptr;
    }

    if (m_geometry_manager != nullptr)
    {
        delete(m_geometry_manager);
        m_geometry_manager = nullptr;
    }

    if (m_shadow_manager != nullptr)
    {
        delete(m_shadow_manager);
        m_shadow_manager = nullptr;
    }

    if (m_survey_map != nullptr)
    {
        delete m_survey_map;
        m_survey_map = nullptr;
    }
}

// some shortcut to remove ugly code
#   define PROGRESS_WINDOW(x, y) { LOG(Ogre::String("  ## ") + y); RoR::App::GetGuiManager()->GetLoadingWindow()->setProgress(x, y); }

void TerrainManager::loadTerrain(const String& filename)
{
    DataStreamPtr ds;

    try
    {
        String group = ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
        ds = ResourceGroupManager::getSingleton().openResource(filename, group);
    }
    catch (...)
    {
        LOG("[RoR|Terrain] File not found: " + filename);
        return;
    }

    PROGRESS_WINDOW(10, _L("Loading Terrain Configuration"));

    LOG(" ===== LOADING TERRAIN " + filename);

    Terrn2Parser parser;
    if (! parser.LoadTerrn2(m_def, ds))
    {
        LOG("[RoR|Terrain] Failed to parse: " + filename);
        for (std::string msg : parser.GetMessages())
        {
            LOG("[RoR|Terrain] \tMessage: " + msg);
        }
        return;
    }
    m_cur_gravity = m_def.gravity;

    // then, init the subsystems, order is important :)
    initSubSystems();

    fixCompositorClearColor();

    LOG(" ===== LOADING TERRAIN GEOMETRY " + filename);

    // load the terrain geometry
    PROGRESS_WINDOW(80, _L("Loading Terrain Geometry"));
    m_geometry_manager->InitTerrain(m_def.ogre_ter_conf_filename);

    LOG(" ===== LOADING TERRAIN WATER " + filename);
    // must happen here
    initWater();

    LOG(" ===== LOADING TERRAIN OBJECTS " + filename);

    PROGRESS_WINDOW(90, _L("Loading Terrain Objects"));
    loadTerrainObjects();

    // bake the decals
    //finishTerrainDecal();

    // init things after loading the terrain
    initTerrainCollisions();

    // init the survey map
    if (!RoR::App::gfx_minimap_disabled.GetActive())
    {
        PROGRESS_WINDOW(45, _L("Initializing Overview Map Subsystem"));
        m_survey_map = new SurveyMapManager();
    }

    PROGRESS_WINDOW(95, _L("Initializing terrain light properties"));
    m_geometry_manager->UpdateMainLightPosition(); // Initial update takes a while
    m_collisions->finishLoadingTerrain();
    LOG(" ===== TERRAIN LOADING DONE " + filename);

    // causing visual ground glitches
/* if (App::gfx_sky_mode.GetActive() == GfxSkyMode::SKYX)
    {
        TerrainGroup::TerrainIterator ti = geometry_manager->getTerrainGroup()->getTerrainIterator();
        while (ti.hasMoreElements())
        {
            Terrain* t = ti.getNext()->instance;
            MaterialPtr ptr = t->getMaterial();
            gEnv->SkyX->GetSkyX()->getGPUManager()->addGroundPass(
                    static_cast<Ogre::MaterialPtr>(ptr)->getTechnique(0)->createPass(), 5000, Ogre::SBT_TRANSPARENT_COLOUR);
        }
    }*/

}

void TerrainManager::initSubSystems()
{
    // geometry - ogre terrain things
    initShadows();

    PROGRESS_WINDOW(15, _L("Initializing Geometry Subsystem"));
    m_geometry_manager = new TerrainGeometryManager(this);

    // objects  - .odef support
    PROGRESS_WINDOW(17, _L("Initializing Object Subsystem"));
    initObjects();

    PROGRESS_WINDOW(19, _L("Initializing Collision Subsystem"));
    m_collisions = new Collisions();
    gEnv->collisions = m_collisions;

    PROGRESS_WINDOW(19, _L("Initializing Script Subsystem"));
    initScripting();

    PROGRESS_WINDOW(21, _L("Initializing Shadow Subsystem"));

    PROGRESS_WINDOW(25, _L("Initializing Camera Subsystem"));
    initCamera();

    // sky, must come after camera due to m_sight_range
    PROGRESS_WINDOW(23, _L("Initializing Sky Subsystem"));
    initSkySubSystem();

    PROGRESS_WINDOW(27, _L("Initializing Light Subsystem"));
    initLight();

    if (App::gfx_sky_mode.GetActive() != GfxSkyMode::CAELUM) //Caelum has its own fog management
    {
        PROGRESS_WINDOW(29, _L("Initializing Fog Subsystem"));
        initFog();
    }

    PROGRESS_WINDOW(31, _L("Initializing Vegetation Subsystem"));
    initVegetation();

    // water must be done later on
    //PROGRESS_WINDOW(33, _L("Initializing Water Subsystem"));
    //initWater();

    if (App::gfx_enable_hdr.GetActive())
    {
        PROGRESS_WINDOW(35, _L("Initializing HDR Subsystem"));
        initHDR();
    }
    if (RoR::App::gfx_enable_glow.GetActive())
    {
        PROGRESS_WINDOW(37, _L("Initializing Glow Subsystem"));
        initGlow();
    }
    if (App::gfx_motion_blur.GetActive())
    {
        PROGRESS_WINDOW(39, _L("Initializing Motion Blur Subsystem"));
        initMotionBlur();
    }
    if (RoR::App::gfx_enable_sunburn.GetActive())
    {
        PROGRESS_WINDOW(41, _L("Initializing Sunburn Subsystem"));
        initSunburn();
    }

    PROGRESS_WINDOW(47, _L("Initializing Dashboards Subsystem"));
    initDashboards();
}

void TerrainManager::initCamera()
{
    gEnv->mainCamera->getViewport()->setBackgroundColour(m_def.ambient_color);
    gEnv->mainCamera->setPosition(m_def.start_position);

    m_sight_range = App::gfx_sight_range.GetActive();

    if (m_sight_range < UNLIMITED_SIGHTRANGE)
        gEnv->mainCamera->setFarClipDistance(m_sight_range);
    else
    {
        // disabled in global config
        if (App::gfx_water_mode.GetActive() != GfxWaterMode::HYDRAX)
            gEnv->mainCamera->setFarClipDistance(0); //Unlimited
        else
            gEnv->mainCamera->setFarClipDistance(9999 * 6); //Unlimited for hydrax and stuff
    }
}

void TerrainManager::initSkySubSystem()
{
#ifdef USE_CAELUM
    // Caelum skies
    if (App::gfx_sky_mode.GetActive() == GfxSkyMode::CAELUM)
    {
        m_sky_manager = new SkyManager();

        // try to load caelum config
        if (!m_def.caelum_config.empty() && ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(m_def.caelum_config))
        {
            // config provided and existing, use it :)
            m_sky_manager->LoadCaelumScript(m_def.caelum_config, m_def.caelum_fog_start, m_def.caelum_fog_end);
        }
        else
        {
            // no config provided, fall back to the default one
            m_sky_manager->LoadCaelumScript("ror_default_sky");
        }
    }
    else
#endif //USE_CAELUM
    // SkyX skies
    if (App::gfx_sky_mode.GetActive() == GfxSkyMode::SKYX)
    {
         // try to load SkyX config
         if (!m_def.skyx_config.empty() && ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(m_def.skyx_config))
            SkyX_manager = new SkyXManager(m_def.skyx_config);
         else
            SkyX_manager = new SkyXManager("SkyXDefault.skx");

         gEnv->SkyX = SkyX_manager;
    }
    else
    {

        if (!m_def.cubemap_config.empty())
        {
            // use custom
            gEnv->sceneManager->setSkyBox(true, m_def.cubemap_config, 100, true);
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
    if (App::gfx_sky_mode.GetActive() == GfxSkyMode::CAELUM)
    {
#ifdef USE_CAELUM
        m_main_light = m_sky_manager->GetSkyMainLight();
#endif
    }
    else if (App::gfx_sky_mode.GetActive() == GfxSkyMode::SKYX)
    {
        m_main_light = SkyX_manager->getMainLight();
    }
    else
    {
        // screw caelum, we will roll our own light

        // Create a light
        m_main_light = gEnv->sceneManager->createLight("MainLight");
        //directional light for shadow
        m_main_light->setType(Light::LT_DIRECTIONAL);
        m_main_light->setDirection(Ogre::Vector3(0.785, -0.423, 0.453).normalisedCopy());

        m_main_light->setDiffuseColour(m_def.ambient_color);
        m_main_light->setSpecularColour(m_def.ambient_color);
        m_main_light->setCastShadows(true);
        m_main_light->setShadowFarDistance(1000.0f);
        m_main_light->setShadowNearClipDistance(-1);
    }
}

void TerrainManager::initFog()
{
    if (m_sight_range >= UNLIMITED_SIGHTRANGE)
        gEnv->sceneManager->setFog(FOG_NONE);
    else
        gEnv->sceneManager->setFog(FOG_LINEAR, m_def.ambient_color, 0.000f, m_sight_range * 0.65f, m_sight_range*0.9);
}

void TerrainManager::initVegetation()
{
    m_paged_mode = static_cast<int>(App::gfx_vegetation_mode.GetActive()); // TODO: don't cast enum to int!

    switch (m_paged_mode)
    {
    case 0:
        m_paged_detail_factor = 0.001f;
        break;
    case 1:
        m_paged_detail_factor = 0.2f;
        break;
    case 2:
        m_paged_detail_factor = 0.5f;
        break;
    case 3:
        m_paged_detail_factor = 1.0f;
        break;
    default:
        m_paged_mode = 0;
        m_paged_detail_factor = 0.0f;
        break;
    }
}

void TerrainManager::initHDR()
{
    Viewport* vp = gEnv->mainCamera->getViewport();
    CompositorInstance* instance = CompositorManager::getSingleton().addCompositor(vp, "HDR", 0);
    CompositorManager::getSingleton().setCompositorEnabled(vp, "HDR", true);

    // HDR needs a special listener
    m_hdr_listener = new HDRListener();
    instance->addListener(m_hdr_listener);
    m_hdr_listener->notifyViewportSize(vp->getActualWidth(), vp->getActualHeight());
    m_hdr_listener->notifyCompositor(instance);
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

void TerrainManager::initWater()
{
    // disabled in global config
    if (App::gfx_water_mode.GetActive() == GfxWaterMode::NONE)
        return;

    // disabled in map config
    if (!m_def.has_water)
    {
        return;
    }

    if (App::gfx_water_mode.GetActive() == GfxWaterMode::HYDRAX)
    {
        // try to load hydrax config
        if (!m_def.hydrax_conf_file.empty() && ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(m_def.hydrax_conf_file))
        {
            m_hydrax_water = new HydraxWater(m_def.water_height, m_def.hydrax_conf_file);
        }
        else
        {
            // no config provided, fall back to the default one
            m_hydrax_water = new HydraxWater(m_def.water_height);
        }

        m_water = m_hydrax_water;

        //Apply depth technique to the terrain
        TerrainGroup::TerrainIterator ti = m_geometry_manager->getTerrainGroup()->getTerrainIterator();
        while (ti.hasMoreElements())
        {
            Terrain* t = ti.getNext()->instance;
            MaterialPtr ptr = t->getMaterial();
            m_hydrax_water->GetHydrax()->getMaterialManager()->addDepthTechnique(ptr->createTechnique());
        }
    }
    else
    {
        if (m_water == nullptr)
           m_water = new Water();
        else if (m_water != nullptr)
        {
            delete(m_water);
            m_water = new Water();
        }

        m_water->SetStaticWaterHeight(m_def.water_height);
    }
}

void TerrainManager::initDashboards()
{
    m_dashboard = new Dashboard();
}

void TerrainManager::initShadows()
{
    m_shadow_manager = new ShadowManager();
    m_shadow_manager->loadConfiguration();
}

void TerrainManager::loadTerrainObjects()
{
    for (const auto& tobj_filename : m_def.tobj_files)
    {
        m_object_manager->LoadTObjFile(tobj_filename);
    }

    m_object_manager->PostLoadTerrain(); // bakes the geometry and things
}

void TerrainManager::initTerrainCollisions()
{
    if (!m_def.traction_map_file.empty())
    {
        m_collisions->setupLandUse(m_def.traction_map_file.c_str());
    }
}

bool TerrainManager::update(float dt)
{
    if (m_object_manager)
        m_object_manager->UpdateTerrainObjects(dt);

    if (m_geometry_manager)
        m_geometry_manager->UpdateMainLightPosition(); // TODO: Is this necessary? I'm leaving it here just in case ~ only_a_ptr, 04/2017

    return true;
}

void TerrainManager::initScripting()
{
#ifdef USE_ANGELSCRIPT
    bool loaded = false;

    // only load terrain scripts while not in multiplayer
    if (RoR::App::mp_state.GetActive() != RoR::MpState::CONNECTED)
    {
        for (std::string as_filename : m_def.as_files)
        {
            if (ScriptEngine::getSingleton().loadScript(as_filename) == 0)
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
    m_cur_gravity = value;
    App::GetSimController()->GetBeamFactory()->RecalcGravityMasses();
}

void TerrainManager::initObjects()
{
    m_object_manager = new TerrainObjectManager(this);
}

Ogre::Vector3 TerrainManager::getMaxTerrainSize()
{
    if (!m_geometry_manager)
        return Vector3::ZERO;
    return m_geometry_manager->getMaxTerrainSize();
}

float TerrainManager::GetHeightAt(float x, float z)
{
    return m_geometry_manager->getHeightAt(x, z);
}

Ogre::Vector3 TerrainManager::GetNormalAt(float x, float y, float z)
{
    return m_geometry_manager->getNormalAt(x, y, z);
}

SkyManager* TerrainManager::getSkyManager()
{
    return m_sky_manager;
}

void TerrainManager::LoadPredefinedActors()
{
    if (m_object_manager)
        m_object_manager->LoadPredefinedActors();
}

bool TerrainManager::HasPredefinedActors()
{
    if (m_object_manager)
        return m_object_manager->HasPredefinedActors();
    return false;
}

std::string TerrainManager::GetMinimapTextureName()
{
    if (m_survey_map != nullptr) // Can be disabled by user
        return m_survey_map->GetMinimapTextureName();
    else
        return "";
}

