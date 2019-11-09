/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2018 Petr Ohlidal & contributors

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

#include "GfxScene.h"

#include "Application.h"
#include "Beam.h"
#include "BeamFactory.h"
#include "DustPool.h"
#include "HydraxWater.h"
#include "OverlayWrapper.h"
#include "RoRFrameListener.h" // SimController
#include "SkyManager.h"
#include "SkyXManager.h"
#include "SurveyMapManager.h"
#include "TerrainGeometryManager.h"
#include "TerrainManager.h"
#include "TerrainObjectManager.h"

using namespace Ogre;

RoR::GfxScene::GfxScene() : m_ogre_scene(nullptr)
{
    m_survey_map = std::unique_ptr<SurveyMapManager>(new SurveyMapManager());
}

RoR::GfxScene::~GfxScene()
{
    for (auto itor : m_dustpools)
    {
        itor.second->Discard(m_ogre_scene);
        delete itor.second;
    }
    m_dustpools.clear();
}

void RoR::GfxScene::InitScene(Ogre::SceneManager *sm)
{
    m_dustpools["dust"]   = new DustPool(sm, "tracks/Dust", 20);
    m_dustpools["clump"]  = new DustPool(sm, "tracks/Clump", 20);
    m_dustpools["sparks"] = new DustPool(sm, "tracks/Sparks", 10);
    m_dustpools["drip"]   = new DustPool(sm, "tracks/Drip", 50);
    m_dustpools["splash"] = new DustPool(sm, "tracks/Splash", 20);
    m_dustpools["ripple"] = new DustPool(sm, "tracks/Ripple", 20);

    m_ogre_scene = sm;

    m_survey_map->init();
    m_envmap.SetupEnvMap();
}

void RoR::GfxScene::UpdateScene(float dt_sec)
{
    // Actors - start threaded tasks
    for (GfxActor *gfx_actor : m_live_gfx_actors)
    {
        gfx_actor->UpdateFlexbodies();   // Push flexbody tasks to threadpool
        gfx_actor->UpdateWheelVisuals(); // Push flexwheel tasks to threadpool
    }

    // Var
    GfxActor *           player_gfx_actor = nullptr;
    std::set<GfxActor *> player_connected_gfx_actors;
    if (m_simbuf.simbuf_player_actor != nullptr)
    {
        player_gfx_actor            = m_simbuf.simbuf_player_actor->GetGfxActor();
        player_connected_gfx_actors = player_gfx_actor->GetLinkedGfxActors();
    }

    // Particles
    if (RoR::App::gfx_particles_mode.GetActive() == 1)
    {
        for (GfxActor *gfx_actor : m_all_gfx_actors)
        {
            if (!m_simbuf.simbuf_sim_paused && !gfx_actor->GetSimDataBuffer().simbuf_physics_paused)
            { gfx_actor->UpdateParticles(m_simbuf.simbuf_sim_speed * dt_sec); }
        }
        for (auto itor : m_dustpools)
        {
            itor.second->update();
        }
    }

    // Realtime reflections on player vehicle
    // IMPORTANT: Toggles visibility of all meshes -> must be done before any other visibility control is evaluated (i.e. aero
    // propellers)
    if (player_gfx_actor != nullptr)
    {
        // Safe to be called here, only modifies OGRE objects, doesn't read any physics state.
        m_envmap.UpdateEnvMap(player_gfx_actor->GetSimDataBuffer().simbuf_pos, player_gfx_actor);
    }

    // Terrain - animated meshes and paged geometry
    App::GetSimTerrain()->getObjectManager()->UpdateTerrainObjects(dt_sec);

    // Terrain - lightmap; TODO: ported as-is from TerrainManager::update(), is it needed? ~ only_a_ptr, 05/2018
    App::GetSimTerrain()
        ->getGeometryManager()
        ->UpdateMainLightPosition(); // TODO: Is this necessary? I'm leaving it here just in case ~ only_a_ptr, 04/2017

    // Terrain - water
    IWater *water = App::GetSimTerrain()->getWater();
    if (water)
    {
        water->WaterSetCamera(gEnv->mainCamera);
        if (player_gfx_actor != nullptr)
        { water->SetReflectionPlaneHeight(water->CalcWavesHeight(player_gfx_actor->GetSimDataBuffer().simbuf_pos)); }
        else
        {
            water->SetReflectionPlaneHeight(water->GetStaticWaterHeight());
        }
        water->FrameStepWater(dt_sec);
    }

    // Terrain - sky
#ifdef USE_CAELUM
    SkyManager *sky = App::GetSimTerrain()->getSkyManager();
    if (sky != nullptr) { sky->DetectSkyUpdate(); }
#endif

    SkyXManager *skyx_man = App::GetSimTerrain()->getSkyXManager();
    if (skyx_man != nullptr)
    {
        skyx_man->update(dt_sec); // Light update
    }

    // GUI - Direction arrow
    if (App::GetOverlayWrapper() && App::GetOverlayWrapper()->IsDirectionArrowVisible())
    {
        App::GetOverlayWrapper()->UpdateDirectionArrowHud(player_gfx_actor, m_simbuf.simbuf_dir_arrow_target,
                                                          m_simbuf.simbuf_character_pos);
    }

    // GUI - Survey map
    if (m_survey_map != nullptr)
    {
        m_survey_map->Update(dt_sec, m_simbuf.simbuf_player_actor);
        for (GfxActor *gfx_actor : m_all_gfx_actors)
        {
            auto & simbuf  = gfx_actor->GetSimDataBuffer();
            String caption = (App::mp_state.GetActive() == MpState::CONNECTED) ? simbuf.simbuf_net_username : "";
            m_survey_map->UpdateMapEntity(gfx_actor->GetSurveyMapEntity(), caption, simbuf.simbuf_pos, simbuf.simbuf_rotation,
                                          gfx_actor->GetActorState(), true);
        }
    }

    // GUI - race
    if (m_simbuf.simbuf_race_in_progress != m_simbuf.simbuf_race_in_progress_prev)
    {
        if (m_simbuf.simbuf_race_in_progress) // Started
        { RoR::App::GetOverlayWrapper()->ShowRacingOverlay(); }
        else // Ended
        {
            RoR::App::GetOverlayWrapper()->HideRacingOverlay();
        }
    }
    if (m_simbuf.simbuf_race_in_progress) { RoR::App::GetOverlayWrapper()->UpdateRacingGui(this); }

    // GUI - vehicle pressure
    if (m_simbuf.simbuf_tyrepressurize_active)
    { RoR::App::GetOverlayWrapper()->UpdatePressureTexture(m_simbuf.simbuf_player_actor->GetGfxActor()); }

    // HUD - network labels (always update)
    for (GfxActor *gfx_actor : m_all_gfx_actors)
    {
        gfx_actor->UpdateNetLabels(m_simbuf.simbuf_sim_speed * dt_sec);
    }

    // Player avatars
    for (GfxCharacter *a : m_all_gfx_characters)
    {
        a->UpdateCharacterInScene();
    }

    // Actors - update misc visuals
    for (GfxActor *gfx_actor : m_live_gfx_actors)
    {
        bool is_player_connected = (player_connected_gfx_actors.find(gfx_actor) != player_connected_gfx_actors.end());
        gfx_actor->UpdateRods();
        gfx_actor->UpdateCabMesh();
        gfx_actor->UpdateWingMeshes();
        gfx_actor->UpdateAirbrakes();
        gfx_actor->UpdateCParticles();
        gfx_actor->UpdateAeroEngines();
        gfx_actor->UpdatePropAnimations(dt_sec, (gfx_actor == player_gfx_actor) || is_player_connected);
        gfx_actor->UpdateProps(dt_sec, (gfx_actor == player_gfx_actor));
        gfx_actor->UpdateFlares(dt_sec, (gfx_actor == player_gfx_actor));
    }
    if (player_gfx_actor != nullptr)
    {
        player_gfx_actor->UpdateVideoCameras(dt_sec);

        // The old-style render-to-texture dashboard (based on OGRE overlays)
        if (m_simbuf.simbuf_player_actor->ar_driveable == TRUCK && m_simbuf.simbuf_player_actor->ar_engine != nullptr)
        { RoR::App::GetOverlayWrapper()->UpdateLandVehicleHUD(player_gfx_actor); }
        else if (m_simbuf.simbuf_player_actor->ar_driveable == AIRPLANE)
        {
            RoR::App::GetOverlayWrapper()->UpdateAerialHUD(player_gfx_actor);
        }
    }

    App::GetSimController()->GetSceneMouse().UpdateVisuals();

    // Actors - finalize threaded tasks
    for (GfxActor *gfx_actor : m_live_gfx_actors)
    {
        gfx_actor->FinishWheelUpdates();
        gfx_actor->FinishFlexbodyTasks();
    }
}

void RoR::GfxScene::SetParticlesVisible(bool visible)
{
    for (auto itor : m_dustpools)
    {
        itor.second->setVisible(visible);
    }
}

DustPool *RoR::GfxScene::GetDustPool(const char *name)
{
    auto found = m_dustpools.find(name);
    if (found != m_dustpools.end()) { return found->second; }
    else
    {
        return nullptr;
    }
}

void RoR::GfxScene::RegisterGfxActor(RoR::GfxActor *gfx_actor)
{
    m_all_gfx_actors.push_back(gfx_actor);

    if (m_survey_map != nullptr)
    {
        auto entity = m_survey_map->createMapEntity(SurveyMapManager::getTypeByDriveable(gfx_actor->GetActorDriveable()));
        gfx_actor->SetSurveyMapEntity(entity);
    }
}

void RoR::GfxScene::BufferSimulationData()
{
    m_simbuf.simbuf_player_actor          = App::GetSimController()->GetPlayerActor();
    m_simbuf.simbuf_character_pos         = gEnv->player->getPosition();
    m_simbuf.simbuf_dir_arrow_target      = App::GetSimController()->GetDirArrowTarget();
    m_simbuf.simbuf_tyrepressurize_active = App::GetSimController()->IsPressurizingTyres();
    m_simbuf.simbuf_sim_paused            = App::GetSimController()->GetPhysicsPaused();
    m_simbuf.simbuf_sim_speed             = App::GetSimController()->GetBeamFactory()->GetSimulationSpeed();
    m_simbuf.simbuf_race_time             = App::GetSimController()->GetRaceTime();
    m_simbuf.simbuf_race_best_time        = App::GetSimController()->GetRaceBestTime();
    m_simbuf.simbuf_race_time_diff        = App::GetSimController()->GetRaceTimeDiff();
    m_simbuf.simbuf_race_in_progress_prev = m_simbuf.simbuf_race_in_progress;
    m_simbuf.simbuf_race_in_progress      = App::GetSimController()->IsRaceInProgress();

    m_live_gfx_actors.clear();
    for (GfxActor *a : m_all_gfx_actors)
    {
        if (a->IsActorLive() || !a->IsActorInitialized())
        {
            a->UpdateSimDataBuffer();
            m_live_gfx_actors.push_back(a);
            a->InitializeActor();
        }
    }

    for (GfxCharacter *a : m_all_gfx_characters)
    {
        a->BufferSimulationData();
    }
}

void RoR::GfxScene::RemoveGfxActor(RoR::GfxActor *remove_me)
{
    auto itor = std::remove(m_all_gfx_actors.begin(), m_all_gfx_actors.end(), remove_me);
    m_all_gfx_actors.erase(itor, m_all_gfx_actors.end());

    if (m_survey_map != nullptr) { m_survey_map->deleteMapEntity(remove_me->GetSurveyMapEntity()); }
}

void RoR::GfxScene::RegisterGfxCharacter(RoR::GfxCharacter *gfx_character)
{
    m_all_gfx_characters.push_back(gfx_character);
}

void RoR::GfxScene::RemoveGfxCharacter(RoR::GfxCharacter *remove_me)
{
    auto itor = std::remove(m_all_gfx_characters.begin(), m_all_gfx_characters.end(), remove_me);
    m_all_gfx_characters.erase(itor, m_all_gfx_characters.end());
}

RoR::GfxScene::SimBuffer::SimBuffer()
    : simbuf_player_actor(nullptr), simbuf_character_pos(Ogre::Vector3::ZERO), simbuf_tyrepressurize_active(false),
      simbuf_race_in_progress(false), simbuf_race_in_progress_prev(false), simbuf_sim_speed(1.0f), simbuf_race_time(0.0f),
      simbuf_race_best_time(0.0f), simbuf_race_time_diff(0.0f), simbuf_dir_arrow_target(Ogre::Vector3::ZERO)
{
}
