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
#include "ApproxMath.h"
#include "Console.h"
#include "DustPool.h"
#include "HydraxWater.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "GUI_DirectionArrow.h"
#include "OverlayWrapper.h"
#include "SkyManager.h"
#include "SkyXManager.h"
#include "TerrainGeometryManager.h"
#include "Terrain.h"
#include "TerrainObjectManager.h"
#include "Utils.h"

#include "imgui_internal.h"

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
    m_gfx_freebeams_grouping_node = nullptr;

    // Recover from the wipe
    App::GetCameraManager()->ReCreateCameraNode();
    App::GetGuiManager()->DirectionArrow.CreateArrow();
    m_gfx_freebeams_grouping_node = m_scene_manager->getRootSceneNode()->createChildSceneNode("FreeBeam Visuals");
}

void GfxScene::Init()
{
    ROR_ASSERT(!m_scene_manager);
    m_scene_manager = App::GetAppContext()->GetOgreRoot()->createSceneManager();
    m_gfx_freebeams_grouping_node = m_scene_manager->getRootSceneNode()->createChildSceneNode("FreeBeam Visuals");

    m_skidmark_conf.LoadDefaultSkidmarkDefs();
}

void GfxScene::UpdateScene(float dt)
{
    // NOTE: The `dt` parameter here is simulation time (0 when paused), not real time!
    // ================================================================================

    // Actors - start threaded tasks
    for (GfxActor* gfx_actor: m_live_gfx_actors)
    {
        gfx_actor->UpdateFlexbodies(); // Push flexbody tasks to threadpool
        gfx_actor->UpdateWheelVisuals(); // Push flexwheel tasks to threadpool
    }

    // Var
    GfxActor* player_gfx_actor = nullptr;
    if (m_simbuf.simbuf_player_actor != nullptr)
    {
        player_gfx_actor = m_simbuf.simbuf_player_actor->GetGfxActor();
    }

    // FOV
    if (m_simbuf.simbuf_camera_behavior != CameraManager::CAMERA_BEHAVIOR_STATIC)
    {
        float fov = (m_simbuf.simbuf_camera_behavior == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)
            ? App::gfx_fov_internal->getFloat() : App::gfx_fov_external->getFloat();
        RoR::App::GetCameraManager()->GetCamera()->setFOVy(Ogre::Degree(fov));
    }

    // Particles
    if (App::gfx_particles_mode->getInt() == 1)
    {
        // Generate particles as needed
        for (GfxActor* gfx_actor: m_all_gfx_actors)
        {
            float dt_actor = (!gfx_actor->GetSimDataBuffer().simbuf_physics_paused) ? dt : 0.f;
            gfx_actor->UpdateParticles(dt_actor);
        }

        // Update particle movement
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
    App::GetGameContext()->GetTerrain()->getObjectManager()->UpdateTerrainObjects(dt);

    // Terrain - lightmap; TODO: ported as-is from Terrain::update(), is it needed? ~ only_a_ptr, 05/2018
    App::GetGameContext()->GetTerrain()->getGeometryManager()->UpdateMainLightPosition(); // TODO: Is this necessary? I'm leaving it here just in case ~ only_a_ptr, 04/2017

    // Terrain - water
    IWater* water = App::GetGameContext()->GetTerrain()->getWater();
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
        water->FrameStepWater(dt);
    }

    // Terrain - sky
#ifdef USE_CAELUM
    SkyManager* sky = App::GetGameContext()->GetTerrain()->getSkyManager();
    if (sky != nullptr)
    {
        sky->DetectSkyUpdate();
    }
#endif

    SkyXManager* skyx_man = App::GetGameContext()->GetTerrain()->getSkyXManager();
    if (skyx_man != nullptr)
    {
       skyx_man->update(dt); // Light update
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
        gfx_actor->UpdateNetLabels(dt);
    }

    // Player avatars
    for (GfxCharacter* a: m_all_gfx_characters)
    {
        a->UpdateCharacterInScene();
    }

    // Actors - update misc visuals
    for (GfxActor* gfx_actor: m_all_gfx_actors)
    {
        float dt_actor = (!gfx_actor->GetSimDataBuffer().simbuf_physics_paused) ? dt : 0.f;
        if (gfx_actor->IsActorLive())
        {
            gfx_actor->UpdateRods();
            gfx_actor->UpdateCabMesh();
            gfx_actor->UpdateWingMeshes();
            gfx_actor->UpdateAirbrakes();
            gfx_actor->UpdateCParticles();
            gfx_actor->UpdateExhausts();
            gfx_actor->UpdateAeroEngines();
            gfx_actor->UpdatePropAnimations(dt_actor);
            gfx_actor->UpdateRenderdashRTT();
        }
        // Beacon flares must always be updated
        gfx_actor->UpdateProps(dt_actor, (gfx_actor == player_gfx_actor));
        // Blinkers (turn signals) must always be updated
        gfx_actor->UpdateFlares(dt_actor, (gfx_actor == player_gfx_actor));
    }
    if (player_gfx_actor != nullptr)
    {
        float dt_actor = (!player_gfx_actor->GetSimDataBuffer().simbuf_physics_paused) ? dt : 0.f;
        player_gfx_actor->UpdateVideoCameras(dt_actor);

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

    this->UpdateFreeBeamGfx(dt);

    // Actors - finalize threaded tasks
    for (GfxActor* gfx_actor: m_live_gfx_actors)
    {
        gfx_actor->FinishWheelUpdates();
        gfx_actor->FinishFlexbodyTasks();
    }
}

void GfxScene::SetParticlesVisible(bool visible)
{
    for (auto itor : m_dustpools)
    {
        itor.second->setVisible(visible);
    }
}

DustPool* GfxScene::GetDustPool(const char* name)
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

void GfxScene::RegisterGfxActor(RoR::GfxActor* gfx_actor)
{
    m_all_gfx_actors.push_back(gfx_actor);
}

void GfxScene::BufferSimulationData()
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

    m_live_gfx_actors.clear();
    for (GfxActor* a: m_all_gfx_actors)
    {
        if (a->IsActorLive() || !a->IsActorInitialized())
        {
            a->UpdateSimDataBuffer();
            m_live_gfx_actors.push_back(a);
            a->InitializeActor();
        }
    }

    for (GfxCharacter* a: m_all_gfx_characters)
    {
        a->BufferSimulationData();
    }
}

void GfxScene::RemoveGfxActor(RoR::GfxActor* remove_me)
{
    auto itor = std::remove(m_all_gfx_actors.begin(), m_all_gfx_actors.end(), remove_me);
    if (itor != m_all_gfx_actors.end())
    {
        m_all_gfx_actors.erase(itor, m_all_gfx_actors.end());
    }
}

void GfxScene::ForceUpdateSingleGfxActor(RoR::GfxActor* gfx_actor)
{
    // Do the work `UpdateScene()` would, but for a single actor.
    // Needed for i.e. terrain editor mode.
    // ------------------------------------------------------

    // Start threaded stuff
    gfx_actor->UpdateFlexbodies(); // Push flexbody tasks to threadpool
    gfx_actor->UpdateWheelVisuals(); // Push flexwheel tasks to threadpool

    // Do sync stuff
    gfx_actor->UpdateRods();
    gfx_actor->UpdateCabMesh();
    gfx_actor->UpdateWingMeshes();
    gfx_actor->UpdateAirbrakes();

    // Finish threaded stuff
    gfx_actor->FinishWheelUpdates();
    gfx_actor->FinishFlexbodyTasks();
}

void GfxScene::RegisterGfxCharacter(RoR::GfxCharacter* gfx_character)
{
    m_all_gfx_characters.push_back(gfx_character);
}

void GfxScene::RemoveGfxCharacter(RoR::GfxCharacter* remove_me)
{
    auto itor = std::remove(m_all_gfx_characters.begin(), m_all_gfx_characters.end(), remove_me);
    if (itor != m_all_gfx_characters.end())
    {
        m_all_gfx_characters.erase(itor, m_all_gfx_characters.end());
    }
}

void GfxScene::DrawNetLabel(Ogre::Vector3 scene_pos, float cam_dist, std::string const& nick, int colornum)
{
#if USE_SOCKETW

        // this ensures that the nickname is always in a readable size
        float font_size = std::max(0.6, cam_dist / 40.0);
        std::string caption;
        if (cam_dist > 1000) // 1000 ... vlen
        {
            caption =
                nick + " (" + TOSTRING((float)(ceil(cam_dist / 100) / 10.0) ) + " km)";
        }
        else if (cam_dist > 20) // 20 ... vlen ... 1000
        {
            caption =
                nick + " (" + TOSTRING((int)cam_dist) + " m)";
        }
        else // 0 ... vlen ... 20
        {
            caption = nick;
        }

        // draw with DearIMGUI

    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    World2ScreenConverter world2screen(
        App::GetCameraManager()->GetCamera()->getViewMatrix(true), App::GetCameraManager()->GetCamera()->getProjectionMatrix(), Ogre::Vector2(screen_size.x, screen_size.y));

    Ogre::Vector3 pos_xyz = world2screen.Convert(scene_pos);

    // only draw when in front of camera
    if (pos_xyz.z < 0.f)
    {
        // Align position to whole pixels, to minimize jitter.
        ImVec2 pos((int)pos_xyz.x+0.5, (int)pos_xyz.y+0.5);

        ImVec2 text_size = ImGui::CalcTextSize(caption.c_str());
        GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

        ImDrawList* drawlist = GetImDummyFullscreenWindow();
        ImGuiContext* g = ImGui::GetCurrentContext();

        ImVec2 text_pos(pos.x - ((text_size.x / 2)), pos.y - ((text_size.y / 2)));

        // Draw background rectangle
        const float PADDING = 4.f;
        drawlist->AddRectFilled(
            text_pos - ImVec2(PADDING, PADDING),
            text_pos + text_size + ImVec2(PADDING, PADDING),
            ImColor(theme.semitransparent_window_bg),
            ImGui::GetStyle().WindowRounding);

        // draw colored text
        Ogre::ColourValue color = App::GetNetwork()->GetPlayerColor(colornum);
        ImVec4 text_color(color.r, color.g, color.b, 1.f);
        drawlist->AddText(g->Font, g->FontSize, text_pos, ImColor(text_color), caption.c_str());
    }

#endif // USE_SOCKETW
}

void GfxScene::AdjustParticleSystemTimeFactor(Ogre::ParticleSystem* psys)
{
    float speed_factor = 0.f;
    if (App::sim_state->getEnum<SimState>() == SimState::RUNNING && !App::GetGameContext()->GetActorManager()->IsSimulationPaused())
    {
        speed_factor = m_simbuf.simbuf_sim_speed;
    }

    psys->setSpeedFactor(speed_factor);
}

void GfxScene::AddFreeBeamGfx(FreeBeamGfxRequest* rq)
{
    auto itor = std::find_if(m_gfx_freebeams.begin(), m_gfx_freebeams.end(),
        [rq](const FreeBeamGfx& obj) { return obj.fbx_id == rq->fbr_id; });
    if (itor != m_gfx_freebeams.end())
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("FreeBeamGfx with ID %d already exists, ignoring request.",rq->fbr_id));
        return;
    }

    FreeBeamGfx obj;
    obj.fbx_id = rq->fbr_id;
    obj.fbx_freeforce_primary = rq->fbr_freeforce_primary;
    obj.fbx_freeforce_secondary = rq->fbr_freeforce_secondary;
    obj.fbx_diameter = rq->fbr_diameter;

    Ogre::Entity* e = m_scene_manager->createEntity(fmt::format("FreeBeamGfx_{}", rq->fbr_id), rq->fbr_mesh_name);
    e->setMaterialName(rq->fbr_material_name);

    obj.fbx_scenenode = m_gfx_freebeams_grouping_node->createChildSceneNode(fmt::format("FreeBeamGfx_{}", rq->fbr_id));
    obj.fbx_scenenode->setScale(rq->fbr_diameter, -1, rq->fbr_diameter);
    obj.fbx_scenenode->attachObject(e);

    m_gfx_freebeams.push_back(obj);
}

void GfxScene::ModifyFreeBeamGfx(FreeBeamGfxRequest* rq)
{
    auto itor = std::find_if(m_gfx_freebeams.begin(), m_gfx_freebeams.end(),
        [rq](const FreeBeamGfx& obj) { return obj.fbx_id == rq->fbr_id; });
    if (itor == m_gfx_freebeams.end())
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("FreeBeamGfx with ID %d not found, ignoring request.", rq->fbr_id));
        return;
    }

    FreeBeamGfx& obj = *itor;
    this->RemoveFreeBeamGfx(rq->fbr_id);
    this->AddFreeBeamGfx(rq);
}

void GfxScene::RemoveFreeBeamGfx(FreeBeamGfxID_t id)
{
    auto itor = std::find_if(m_gfx_freebeams.begin(), m_gfx_freebeams.end(),
        [id](const FreeBeamGfx& obj) { return obj.fbx_id == id; });
    if (itor == m_gfx_freebeams.end())
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("FreeBeamGfx with ID %d not found, ignoring request.", id));
        return;
    }

    FreeBeamGfx& obj = *itor;
    m_scene_manager->destroyEntity((Ogre::Entity*)obj.fbx_scenenode->getAttachedObject(0));
    m_gfx_freebeams_grouping_node->removeChild(obj.fbx_scenenode);
    m_gfx_freebeams.erase(itor);
}

void GfxScene::UpdateFreeBeamGfx(float dt)
{
    for (FreeBeamGfx& freebeam : m_gfx_freebeams)
    {
        // Sanity checks - primary freeforce
        ROR_ASSERT(freebeam.fbx_id != FREEBEAMGFXID_INVALID);
        ROR_ASSERT(freebeam.fbx_freeforce_primary != FREEFORCEID_INVALID);
        ActorManager::FreeForceVec_t::iterator itor;
        const bool exists = App::GetGameContext()->GetActorManager()->FindFreeForce(freebeam.fbx_freeforce_primary, itor);
        ROR_ASSERT(exists);
        if (!exists)
        {
            continue;
        }
        FreeForce& freeforce = *itor;
        
        // Sanity checks - base actor
        ROR_ASSERT(freeforce.ffc_base_actor);
        ROR_ASSERT(freeforce.ffc_base_actor->ar_state != ActorState::DISPOSED);
        GfxActor* gfx_actor_base = freeforce.ffc_base_actor->GetGfxActor();
        ROR_ASSERT(gfx_actor_base);
        ROR_ASSERT(freeforce.ffc_base_node != NODENUM_INVALID);
        ROR_ASSERT(freeforce.ffc_base_node < freeforce.ffc_base_actor->ar_num_nodes);

        // Sanity checks - target actor
        ROR_ASSERT(freeforce.ffc_target_actor);
        ROR_ASSERT(freeforce.ffc_target_actor->ar_state != ActorState::DISPOSED);
        GfxActor* gfx_actor_target = freeforce.ffc_target_actor->GetGfxActor();
        ROR_ASSERT(gfx_actor_target);
        ROR_ASSERT(freeforce.ffc_target_node != NODENUM_INVALID);
        ROR_ASSERT(freeforce.ffc_target_node < freeforce.ffc_target_actor->ar_num_nodes);

        // Get node positions
        Ogre::Vector3 basenode_pos = gfx_actor_base->GetSimNodeBuffer()[freeforce.ffc_base_node].AbsPosition;
        Ogre::Vector3 targetnode_pos = gfx_actor_target->GetSimNodeBuffer()[freeforce.ffc_target_node].AbsPosition;

        // Do the transforms
        freebeam.fbx_scenenode->setPosition(basenode_pos.midPoint(targetnode_pos));
        freebeam.fbx_scenenode->setOrientation(GfxScene::SpecialGetRotationTo(Ogre::Vector3::UNIT_Y, (basenode_pos - targetnode_pos)));
        freebeam.fbx_scenenode->setScale(freebeam.fbx_diameter, basenode_pos.distance(targetnode_pos), freebeam.fbx_diameter);
    }
}

void GfxScene::OnFreeForceRemoved(FreeForceID_t id)
{
    auto itor_secondary = std::find_if(m_gfx_freebeams.begin(), m_gfx_freebeams.end(),
        [id](const FreeBeamGfx& obj) { return obj.fbx_freeforce_secondary == id; });
    if (itor_secondary != m_gfx_freebeams.end())
    {
        // Just clear the freeforce ID
        itor_secondary->fbx_freeforce_secondary = FREEFORCEID_INVALID;
    }
    else
    {
        auto itor_primary = std::find_if(m_gfx_freebeams.begin(), m_gfx_freebeams.end(),
            [id](const FreeBeamGfx& obj) { return obj.fbx_freeforce_primary == id; });
        if (itor_primary != m_gfx_freebeams.end())
        {
            // Remove the whole freebeam
            this->RemoveFreeBeamGfx(itor_primary->fbx_id);
        }
    }
}

void GfxScene::OnFreeForceBroken(FreeForceID_t id)
{
    auto itor = std::find_if(m_gfx_freebeams.begin(), m_gfx_freebeams.end(),
        [id](const FreeBeamGfx& obj) { return obj.fbx_freeforce_primary == id || obj.fbx_freeforce_secondary == id; });
    if (itor != m_gfx_freebeams.end())
    {
        // Remove the whole freebeam if either freeforce broke
        this->RemoveFreeBeamGfx(itor->fbx_id);
    }
}

Ogre::Quaternion RoR::GfxScene::SpecialGetRotationTo(const Ogre::Vector3& src, const Ogre::Vector3& dest)
{
    // Based on Stan Melax's article in Game Programming Gems
    Ogre::Quaternion q;
    // Copy, since cannot modify local
    Ogre::Vector3 v0 = src;
    Ogre::Vector3 v1 = dest;
    v0.normalise();
    v1.normalise();

    // NB if the crossProduct approaches zero, we get unstable because ANY axis will do
    // when v0 == -v1
    Ogre::Real d = v0.dotProduct(v1);
    // If dot == 1, vectors are the same
    if (d >= 1.0f)
    {
        return Ogre::Quaternion::IDENTITY;
    }
    if (d < (1e-6f - 1.0f))
    {
        // Generate an axis
        Ogre::Vector3 axis = Ogre::Vector3::UNIT_X.crossProduct(src);
        if (axis.isZeroLength()) // pick another if colinear
            axis = Ogre::Vector3::UNIT_Y.crossProduct(src);
        axis.normalise();
        q.FromAngleAxis(Ogre::Radian(Ogre::Math::PI), axis);
    }
    else
    {
        Ogre::Real s = fast_sqrt((1 + d) * 2);
        if (s == 0)
            return Ogre::Quaternion::IDENTITY;

        Ogre::Vector3 c = v0.crossProduct(v1);
        Ogre::Real invs = 1 / s;

        q.x = c.x * invs;
        q.y = c.y * invs;
        q.z = c.z * invs;
        q.w = s * 0.5;
    }
    return q;
}
