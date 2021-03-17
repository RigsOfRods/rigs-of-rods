/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

#include "AppContext.h"
#include "Actor.h"
#include "ActorManager.h"
#include "Console.h"
#include "DustPool.h"
#include "HydraxWater.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GUI_DirectionArrow.h"
#include "OverlayWrapper.h"
#include "SkyManager.h"
#include "SkyXManager.h"
#include "TerrainGeometryManager.h"
#include "TerrainManager.h"
#include "TerrainObjectManager.h"

#include <Ogre.h>

using namespace Ogre;
using namespace RoR;

void GfxScene::CreateDustPools()
{
    ROR_ASSERT(m_dustpools.size() == 0);
    m_dustpools["dust"]   = new DustPool(m_scene_manager, "tracks/Dust",   20);
    m_dustpools["clump"]  = new DustPool(m_scene_manager, "tracks/Clump",  20);
    m_dustpools["sparks"] = new DustPool(m_scene_manager, "tracks/Sparks", 10);
    m_dustpools["drip"]   = new DustPool(m_scene_manager, "tracks/Drip",   50);
    m_dustpools["splash"] = new DustPool(m_scene_manager, "tracks/Splash", 20);
    m_dustpools["ripple"] = new DustPool(m_scene_manager, "tracks/Ripple", 20);
}

void GfxScene::ClearScene()
{
    // Delete dustpools
    for (auto itor : m_dustpools)
    {
        itor.second->Discard(m_scene_manager);
        delete itor.second;
    }
    m_dustpools.clear();

    // Delete game elements
    m_all_gfx_actors.clear();
    m_all_gfx_characters.clear();

    // Wipe scene manager
    m_scene_manager->clearScene();

    // Recover from the wipe
    App::GetCameraManager()->ReCreateCameraNode();
    App::GetGuiManager()->GetDirectionArrow()->CreateArrow();
}

void RoR::GfxScene::Init()
{
    ROR_ASSERT(!m_scene_manager);
    m_scene_manager = App::GetAppContext()->GetOgreRoot()->createSceneManager(Ogre::ST_EXTERIOR_CLOSE, "main_scene_manager");

    m_skidmark_conf.LoadDefaultSkidmarkDefs();
}

void RoR::GfxScene::UpdateScene(float dt_sec)
{
    // Actors - start threaded tasks
    for (GfxActor* gfx_actor: m_all_gfx_actors)
    {
        if (gfx_actor->IsActorLive())
        {
            gfx_actor->UpdateFlexbodies(); // Push flexbody tasks to threadpool
            gfx_actor->UpdateWheelVisuals(); // Push flexwheel tasks to threadpool
        }
    }

    // Var
    GfxActor* player_gfx_actor = nullptr;
    std::set<GfxActor*> player_connected_gfx_actors;
    if (m_simbuf.simbuf_player_actor != nullptr)
    {
        player_gfx_actor = m_simbuf.simbuf_player_actor->GetGfxActor();
        player_connected_gfx_actors = player_gfx_actor->GetLinkedGfxActors();
    }

    // FOV
    if (m_simbuf.simbuf_camera_behavior != CameraManager::CAMERA_BEHAVIOR_STATIC)
    {
        float fov = (m_simbuf.simbuf_camera_behavior == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)
            ? App::gfx_fov_internal->GetFloat() : App::gfx_fov_external->GetFloat();
        RoR::App::GetCameraManager()->GetCamera()->setFOVy(Ogre::Degree(fov));
    }

    // Particles
    if (App::gfx_particles_mode->GetInt() == 1)
    {
        for (GfxActor* gfx_actor: m_all_gfx_actors)
        {
            if (!m_simbuf.simbuf_sim_paused && !gfx_actor->GetSimDataBuffer().simbuf_physics_paused)
            {
                gfx_actor->UpdateParticles(m_simbuf.simbuf_sim_speed * dt_sec);
            }
        }
        for (auto itor : m_dustpools)
        {
            itor.second->update();
        }
    }

    // Realtime reflections on player vehicle
    // IMPORTANT: Toggles visibility of all meshes -> must be done before any other visibility control is evaluated (i.e. aero propellers)
    if (player_gfx_actor != nullptr)
    {
        // Safe to be called here, only modifies OGRE objects, doesn't read any physics state.
        m_envmap.UpdateEnvMap(player_gfx_actor->GetSimDataBuffer().simbuf_pos, player_gfx_actor);
    }

    // Terrain - animated meshes and paged geometry
    App::GetSimTerrain()->getObjectManager()->UpdateTerrainObjects(dt_sec);

    // Terrain - lightmap; TODO: ported as-is from TerrainManager::update(), is it needed? ~ only_a_ptr, 05/2018
    App::GetSimTerrain()->getGeometryManager()->UpdateMainLightPosition(); // TODO: Is this necessary? I'm leaving it here just in case ~ only_a_ptr, 04/2017

    // Terrain - water
    IWater* water = App::GetSimTerrain()->getWater();
    if (water)
    {
        if (player_gfx_actor != nullptr)
        {
            water->SetReflectionPlaneHeight(water->CalcWavesHeight(player_gfx_actor->GetSimDataBuffer().simbuf_pos));
        }
        else
        {
            water->SetReflectionPlaneHeight(water->GetStaticWaterHeight());
        }
        water->FrameStepWater(dt_sec);
    }

    // Terrain - sky
#ifdef USE_CAELUM
    SkyManager* sky = App::GetSimTerrain()->getSkyManager();
    if (sky != nullptr)
    {
        sky->DetectSkyUpdate();
    }
#endif

    SkyXManager* skyx_man = App::GetSimTerrain()->getSkyXManager();
    if (skyx_man != nullptr)
    {
       skyx_man->update(dt_sec); // Light update
    }

    // GUI - race
    if (m_simbuf.simbuf_race_in_progress != m_simbuf.simbuf_race_in_progress_prev)
    {
        if (m_simbuf.simbuf_race_in_progress) // Started
        {
            RoR::App::GetOverlayWrapper()->ShowRacingOverlay();
        }
        else // Ended
        {
            RoR::App::GetOverlayWrapper()->HideRacingOverlay();
        }
    }
    if (m_simbuf.simbuf_race_in_progress)
    {
        RoR::App::GetOverlayWrapper()->UpdateRacingGui(this);
    }

    // GUI - vehicle pressure
    if (m_simbuf.simbuf_player_actor)
    {
        App::GetOverlayWrapper()->UpdatePressureOverlay(m_simbuf.simbuf_player_actor->GetGfxActor());
    }

    // HUD - network labels (always update)
    for (GfxActor* gfx_actor: m_all_gfx_actors)
    {
        gfx_actor->UpdateNetLabels(m_simbuf.simbuf_sim_speed * dt_sec);
    }

    // Player avatars
    for (GfxCharacter* a: m_all_gfx_characters)
    {
        a->UpdateCharacterInScene();
    }

    // Actors - update misc visuals
    for (GfxActor* gfx_actor: m_all_gfx_actors)
    {
        bool is_player_connected = (player_connected_gfx_actors.find(gfx_actor) != player_connected_gfx_actors.end());
        if (gfx_actor->IsActorLive())
        {
            gfx_actor->UpdateRods();
            gfx_actor->UpdateCabMesh();
            gfx_actor->UpdateWingMeshes();
            gfx_actor->UpdateAirbrakes();
            gfx_actor->UpdateCParticles();
            gfx_actor->UpdateAeroEngines();
            gfx_actor->UpdatePropAnimations(dt_sec, (gfx_actor == player_gfx_actor) || is_player_connected);
            gfx_actor->UpdateRenderdashRTT();
        }
        // Beacon flares must always be updated
        gfx_actor->UpdateProps(dt_sec, (gfx_actor == player_gfx_actor));
        // Blinkers (turn signals) must always be updated
        gfx_actor->UpdateFlares(dt_sec, (gfx_actor == player_gfx_actor));
    }
    if (player_gfx_actor != nullptr)
    {
        player_gfx_actor->UpdateVideoCameras(dt_sec);

        // The old-style render-to-texture dashboard (based on OGRE overlays)
        if (m_simbuf.simbuf_player_actor->ar_driveable == TRUCK && m_simbuf.simbuf_player_actor->ar_engine != nullptr)
        {
            RoR::App::GetOverlayWrapper()->UpdateLandVehicleHUD(player_gfx_actor);
        }
        else if (m_simbuf.simbuf_player_actor->ar_driveable == AIRPLANE)
        {
            RoR::App::GetOverlayWrapper()->UpdateAerialHUD(player_gfx_actor);
        }
    }

    App::GetGuiManager()->DrawSimGuiBuffered(player_gfx_actor);

    App::GetGameContext()->GetSceneMouse().UpdateVisuals();

    // Actors - finalize threaded tasks
    for (GfxActor* gfx_actor: m_all_gfx_actors)
    {
        if (gfx_actor->IsActorLive())
        {
            gfx_actor->FinishWheelUpdates();
            gfx_actor->FinishFlexbodyTasks();
        }
    }
}

void RoR::GfxScene::SetParticlesVisible(bool visible)
{
    for (auto itor : m_dustpools)
    {
        itor.second->setVisible(visible);
    }
}

DustPool* RoR::GfxScene::GetDustPool(const char* name)
{
    auto found = m_dustpools.find(name);
    if (found != m_dustpools.end())
    {
        return found->second;
    }
    else
    {
        return nullptr;
    }
}

void RoR::GfxScene::RegisterGfxActor(RoR::GfxActor* gfx_actor)
{
    m_all_gfx_actors.push_back(gfx_actor);
}

void RoR::GfxScene::BufferSimulationData()
{
    m_simbuf.simbuf_player_actor = App::GetGameContext()->GetPlayerActor();
    m_simbuf.simbuf_character_pos = App::GetGameContext()->GetPlayerCharacter()->getPosition();
    m_simbuf.simbuf_sim_paused = App::GetGameContext()->GetActorManager()->IsSimulationPaused();
    m_simbuf.simbuf_sim_speed = App::GetGameContext()->GetActorManager()->GetSimulationSpeed();
    m_simbuf.simbuf_camera_behavior = App::GetCameraManager()->GetCurrentBehavior();

    // Race system
    m_simbuf.simbuf_race_time = App::GetGameContext()->GetRaceSystem().GetRaceTime();
    m_simbuf.simbuf_race_best_time = App::GetGameContext()->GetRaceSystem().GetRaceBestTime();
    m_simbuf.simbuf_race_time_diff = App::GetGameContext()->GetRaceSystem().GetRaceTimeDiff();
    m_simbuf.simbuf_race_in_progress_prev = m_simbuf.simbuf_race_in_progress;
    m_simbuf.simbuf_race_in_progress = App::GetGameContext()->GetRaceSystem().IsRaceInProgress();
    m_simbuf.simbuf_dir_arrow_target = App::GetGameContext()->GetRaceSystem().GetDirArrowTarget();
    m_simbuf.simbuf_dir_arrow_text = App::GetGameContext()->GetRaceSystem().GetDirArrowText();
    m_simbuf.simbuf_dir_arrow_visible = App::GetGameContext()->GetRaceSystem().IsDirArrowVisible();

    for (GfxActor* a: m_all_gfx_actors)
    {
        if (a->IsActorLive() || !a->IsActorInitialized())
        {
            a->UpdateSimDataBuffer();
            a->InitializeActor();
        }
    }

    for (GfxCharacter* a: m_all_gfx_characters)
    {
        a->BufferSimulationData();
    }
}

void RoR::GfxScene::RemoveGfxActor(RoR::GfxActor* remove_me)
{
    auto itor = std::remove(m_all_gfx_actors.begin(), m_all_gfx_actors.end(), remove_me);
    m_all_gfx_actors.erase(itor, m_all_gfx_actors.end());
}

void RoR::GfxScene::RegisterGfxCharacter(RoR::GfxCharacter* gfx_character)
{
    m_all_gfx_characters.push_back(gfx_character);
}

void RoR::GfxScene::RemoveGfxCharacter(RoR::GfxCharacter* remove_me)
{
    auto itor = std::remove(m_all_gfx_characters.begin(), m_all_gfx_characters.end(), remove_me);
    m_all_gfx_characters.erase(itor, m_all_gfx_characters.end());
}

