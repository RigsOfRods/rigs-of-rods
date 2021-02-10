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

#include "ActorManager.h"
#include "CacheSystem.h"
#include "Collisions.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "GUI_LoadingWindow.h"
#include "GUI_SurveyMap.h"
#include "Language.h"
#include "ScriptEngine.h"
#include "ShadowManager.h"
#include "SkyManager.h"
#include "TerrainObjectManager.h"
#include "Water.h"


using namespace RoR;
using namespace Ogre;

TerrainManager::TerrainManager()
    : m_collisions(0)
    , m_geometry_manager(0)
    , m_object_manager(0)
    , m_shadow_manager(0)
    , m_sky_manager(0)
    , m_sight_range(1000)
    , m_main_light(0)
    , m_paged_detail_factor(0.0f)
    , m_cur_gravity(DEFAULT_GRAVITY)
{
}

TerrainManager::~TerrainManager()
{
    if (App::app_state->GetEnum<AppState>() == AppState::SHUTDOWN)
    {
        // Rush to exit
        return;
    }

    //I think that the order is important

#ifdef USE_CAELUM
    if (m_sky_manager != nullptr)
    {
        delete(m_sky_manager);
        m_sky_manager = nullptr;
    }
#endif // USE_CAELUM



    if (m_main_light != nullptr)
    {
        App::GetGfxScene()->GetSceneManager()->destroyAllLights();
        m_main_light = nullptr;
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

    if (m_collisions != nullptr)
    {
        delete(m_collisions);
        m_collisions = nullptr;
    }
}

TerrainManager* TerrainManager::LoadAndPrepareTerrain(CacheEntry& entry)
{
    auto terrn_mgr = std::unique_ptr<TerrainManager>(new TerrainManager());
    auto loading_window = App::GetGuiManager()->GetLoadingWindow();

    std::string const& filename = entry.fname;
    try
    {
        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(filename);
        LOG(" ===== LOADING TERRAIN " + filename);
        Terrn2Parser parser;
        if (! parser.LoadTerrn2(terrn_mgr->m_def, stream))
        {
            return nullptr; // Errors already logged to console
        }
    }
    catch (Ogre::Exception& e)
    {
        App::GetGuiManager()->ShowMessageBox(_L("Terrain loading error"), e.getFullDescription().c_str());
        return nullptr;
    }

    terrn_mgr->setGravity(terrn_mgr->m_def.gravity);

    loading_window->SetProgress(15, _L("Initializing Shadow Subsystem"));
    terrn_mgr->initShadows();



    loading_window->SetProgress(19, _L("Initializing Object Subsystem"));
    terrn_mgr->initObjects(); // *.odef files

    loading_window->SetProgress(23, _L("Initializing Camera Subsystem"));
    terrn_mgr->initCamera();

    // sky, must come after camera due to m_sight_range
    loading_window->SetProgress(25, _L("Initializing Sky Subsystem"));
    terrn_mgr->initSkySubSystem();

    loading_window->SetProgress(27, _L("Initializing Light Subsystem"));
    terrn_mgr->initLight();

    if (App::gfx_sky_mode->GetEnum<GfxSkyMode>() != GfxSkyMode::CAELUM) //Caelum has its own fog management
    {
        loading_window->SetProgress(29, _L("Initializing Fog Subsystem"));
        terrn_mgr->initFog();
    }

    loading_window->SetProgress(31, _L("Initializing Vegetation Subsystem"));
    terrn_mgr->initVegetation();

    terrn_mgr->fixCompositorClearColor();



    loading_window->SetProgress(60, _L("Initializing Collision Subsystem"));
    terrn_mgr->m_collisions = new Collisions(terrn_mgr->getMaxTerrainSize());

    loading_window->SetProgress(75, _L("Initializing Script Subsystem"));
    App::SetSimTerrain(terrn_mgr.get()); // Hack for GameScript::spawnObject()
    terrn_mgr->initScripting();
    App::SetSimTerrain(nullptr); // END Hack for GameScript::spawnObject()

    loading_window->SetProgress(77, _L("Initializing Water Subsystem"));
    terrn_mgr->initWater();

    loading_window->SetProgress(80, _L("Loading Terrain Objects"));
    App::SetSimTerrain(terrn_mgr.get()); // Hack for the ProceduralManager
    terrn_mgr->loadTerrainObjects(); // *.tobj files
    App::SetSimTerrain(nullptr); // END Hack for the ProceduralManager

    // init things after loading the terrain
    App::SetSimTerrain(terrn_mgr.get()); // Hack for the Landusemap
    terrn_mgr->initTerrainCollisions();
    App::SetSimTerrain(nullptr); // END Hack for the Landusemap

    loading_window->SetProgress(90, _L("Initializing terrain light properties"));

    App::SetSimTerrain(terrn_mgr.get()); // Hack for the Collision debug visual
    terrn_mgr->m_collisions->finishLoadingTerrain();
    App::SetSimTerrain(nullptr); // END Hack for the Collision debug visual

    terrn_mgr->LoadTelepoints(); // *.terrn2 file feature

    App::GetGfxScene()->CreateDustPools(); // Particle effects

    loading_window->SetProgress(92, _L("Initializing Overview Map Subsystem"));
    App::SetSimTerrain(terrn_mgr.get()); // Hack for the SurveyMapTextureCreator
    App::GetGuiManager()->GetSurveyMap()->CreateTerrainTextures(); // Should be done before actors are loaded, otherwise they'd show up in the static texture
    App::SetSimTerrain(nullptr); // END Hack for the SurveyMapTextureCreator

    LOG(" ===== LOADING TERRAIN ACTORS " + filename);
    loading_window->SetProgress(95, _L("Loading Terrain Actors"));
    terrn_mgr->LoadPredefinedActors();

    LOG(" ===== TERRAIN LOADING DONE " + filename);

    App::sim_terrain_name->SetStr(filename);
    App::sim_terrain_gui_name->SetStr(terrn_mgr->m_def.name);

    return terrn_mgr.release();
}

void TerrainManager::initCamera()
{

    App::GetCameraManager()->GetCameraNode()->setPosition(m_def.start_position);

    if (App::gfx_sky_mode->GetEnum<GfxSkyMode>() == GfxSkyMode::SKYX)
    {
        m_sight_range = 5000;  //Force unlimited for SkyX, lower settings are glitchy
    } 
    else
    {
        m_sight_range = App::gfx_sight_range->GetInt();
    } 

    if (m_sight_range < UNLIMITED_SIGHTRANGE && App::gfx_sky_mode->GetEnum<GfxSkyMode>() != GfxSkyMode::SKYX)
    {
        App::GetCameraManager()->GetCamera()->setFarClipDistance(m_sight_range);
    }
    else
    {
        // disabled in global config
        if (App::gfx_water_mode->GetEnum<GfxWaterMode>() != GfxWaterMode::HYDRAX)
            App::GetCameraManager()->GetCamera()->setFarClipDistance(0); //Unlimited
        else
            App::GetCameraManager()->GetCamera()->setFarClipDistance(9999 * 6); //Unlimited for hydrax and stuff
    }
}

void TerrainManager::initSkySubSystem()
{
#ifdef USE_CAELUM
    // Caelum skies
    if (App::gfx_sky_mode->GetEnum<GfxSkyMode>() == GfxSkyMode::CAELUM)
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

            // use default
      // TODO OGRE2x //       App::GetGfxScene()->GetSceneManager()->setSkyBox(true, "tracks/skyboxcol", 100, true);

}

void TerrainManager::initLight()
{
// TODO OGRE2x // 
}

void TerrainManager::initFog()
{
    if (m_sight_range >= UNLIMITED_SIGHTRANGE)
        App::GetGfxScene()->GetSceneManager()->setFog(FOG_NONE);
    else
        App::GetGfxScene()->GetSceneManager()->setFog(FOG_LINEAR, m_def.ambient_color, 0.000f, m_sight_range * 0.65f, m_sight_range*0.9);
}

void TerrainManager::initVegetation()
{
    switch (App::gfx_vegetation_mode->GetEnum<GfxVegetation>())
    {
    case GfxVegetation::x20PERC:
        m_paged_detail_factor = 0.2f;
        break;
    case GfxVegetation::x50PERC:
        m_paged_detail_factor = 0.5f;
        break;
    case GfxVegetation::FULL:
        m_paged_detail_factor = 1.0f;
        break;
    default:
        m_paged_detail_factor = 0.0f;
        break;
    }
}

void TerrainManager::fixCompositorClearColor()
{
    // TODO OGRE2x // 
}

void TerrainManager::initWater()
{
    // disabled in global config
    if (App::gfx_water_mode->GetEnum<GfxWaterMode>() == GfxWaterMode::NONE)
        return;

    // disabled in map config
    if (!m_def.has_water)
    {
        return;
    }

    m_water = std::unique_ptr<IWater>(new Water(this->getMaxTerrainSize()));
    m_water->SetStaticWaterHeight(m_def.water_height);
    m_water->SetWaterBottomHeight(m_def.water_bottom_height);
}

void TerrainManager::initShadows()
{
    m_shadow_manager = new ShadowManager();
    m_shadow_manager->loadConfiguration();
}

void TerrainManager::loadTerrainObjects()
{
    for (std::string tobj_filename : m_def.tobj_files)
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

void TerrainManager::initScripting()
{
#ifdef USE_ANGELSCRIPT
    bool loaded = false;

    for (std::string as_filename : m_def.as_files)
    {
        if (App::GetScriptEngine()->loadScript(as_filename) == 0)
            loaded = true;
    }

    if (!loaded)
    {
        // load a default script that does the most basic things
        App::GetScriptEngine()->loadScript("default.as");
    }
    // finally activate AS logging, so we dont spam the users screen with initialization messages
    App::GetScriptEngine()->activateLogging();
#endif //USE_ANGELSCRIPT
}

void TerrainManager::setGravity(float value)
{
    m_cur_gravity = value;
}

void TerrainManager::initObjects()
{
    m_object_manager = new TerrainObjectManager(this);
}

Ogre::AxisAlignedBox TerrainManager::getTerrainCollisionAAB()
{
    return m_collisions->getCollisionAAB();
}

Ogre::Vector3 TerrainManager::getMaxTerrainSize()
{

        return Vector3::ZERO; 

}

float TerrainManager::GetHeightAt(float x, float z)
{
    return 0.f; // TODO OGRE2x // 
}

Ogre::Vector3 TerrainManager::GetNormalAt(float x, float y, float z)
{
    return Vector3::ZERO; // TODO OGRE2x // 
}

SkyManager* TerrainManager::getSkyManager()
{
    return m_sky_manager;
}

bool TerrainManager::isFlat()
{
    return true;// TODO OGRE2x // 
}

void TerrainManager::LoadTelepoints()
{
    if (m_object_manager)
        m_object_manager->LoadTelepoints();
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

void TerrainManager::HandleException(const char* summary)
{
    try
    {
        throw; // rethrow
    }
    catch (Ogre::Exception& oex)
    {
        RoR::LogFormat("[RoR|Terrain] %s, message: '%s', type: <Ogre::Exception>.", summary, oex.getFullDescription().c_str());
    }
    catch (std::exception& stex)
    {
        RoR::LogFormat("[RoR|Terrain] %s, message: '%s', type: <std::exception>.", summary, stex.what());
    }
    catch (...)
    {
        RoR::LogFormat("[RoR|Terrain] %s, unknown error occurred.", summary);
    }
}

