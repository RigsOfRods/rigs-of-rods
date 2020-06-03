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

#include "GameContext.h"

#include "AppContext.h"
#include "Beam.h" // class Actor
#include "CacheSystem.h"
#include "Collisions.h"
#include "Console.h"
#include "DashBoardManager.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "GUI_FrictionSettings.h"
#include "GUI_MainSelector.h"
#include "OverlayWrapper.h"
#include "RoRFrameListener.h" // SimController
#include "ScriptEngine.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "Utils.h"

using namespace RoR;

// --------------------------------
// Message queue

void GameContext::PushMessage(Message m)
{
    std::lock_guard<std::mutex> lock(m_msg_mutex);
    m_msg_queue.push(m);
}

bool GameContext::HasMessages()
{
    std::lock_guard<std::mutex> lock(m_msg_mutex);
    return !m_msg_queue.empty();
}

Message GameContext::PopMessage()
{
    std::lock_guard<std::mutex> lock(m_msg_mutex);
    Message m = m_msg_queue.front();
    m_msg_queue.pop();
    return m;
}

// --------------------------------
// Terrain

bool GameContext::LoadTerrain(std::string const& filename_part)
{
    // Load character - must be done first!
    m_character_factory.CreateLocalCharacter();

    // Find terrain in modcache
    CacheEntry* terrn_entry = App::GetCacheSystem()->FindEntryByFilename(LT_Terrain, /*partial=*/true, filename_part);
    if (!terrn_entry)
    {
        Str<200> msg; msg << _L("Terrain not found: ") << filename_part;
        RoR::Log(msg.ToCStr());
        App::GetGuiManager()->ShowMessageBox(_L("Terrain loading error"), msg.ToCStr());
        return false;
    }

    // Init resources
    App::GetCacheSystem()->LoadResource(*terrn_entry);

    // Perform the loading and setup
    App::SetSimTerrain(TerrainManager::LoadAndPrepareTerrain(*terrn_entry));
    if (!App::GetSimTerrain())
    {
        return false; // Message box already displayed
    }

    // Initialize envmap textures by rendering center of map
    Ogre::Vector3 center = App::GetSimTerrain()->getMaxTerrainSize() / 2;
    center.y = App::GetSimTerrain()->GetHeightAt(center.x, center.z) + 1.0f;
    App::GetGfxScene()->GetEnvMap().UpdateEnvMap(center, nullptr);

    // Scan groundmodels
    App::GetGuiManager()->GetFrictionSettings()->AnalyzeTerrain();

    // Adjust character position
    Ogre::Vector3 spawn_pos = App::GetSimTerrain()->getSpawnPos();
    float spawn_rot = 0.0f;

    // Classic behavior, retained for compatibility.
    // Required for maps like N-Labs or F1 Track.
    if (!App::GetSimTerrain()->HasPredefinedActors())
    {
        spawn_rot = 180.0f;
    }

    if (App::diag_preset_spawn_pos->GetStr() != "")
    {
        spawn_pos = Ogre::StringConverter::parseVector3(App::diag_preset_spawn_pos->GetStr(), spawn_pos);
        App::diag_preset_spawn_pos->SetStr("");
    }
    if (App::diag_preset_spawn_rot->GetStr() != "")
    {
        spawn_rot = Ogre::StringConverter::parseReal(App::diag_preset_spawn_rot->GetStr(), spawn_rot);
        App::diag_preset_spawn_rot->SetStr("");
    }

    spawn_pos.y = App::GetSimTerrain()->GetCollisions()->getSurfaceHeightBelow(spawn_pos.x, spawn_pos.z, spawn_pos.y + 1.8f);

    this->GetPlayerCharacter()->setPosition(spawn_pos);
    this->GetPlayerCharacter()->setRotation(Ogre::Degree(spawn_rot));

    App::GetCameraManager()->GetCameraNode()->setPosition(App::GetGameContext()->GetPlayerCharacter()->getPosition());

    // Small hack to improve the spawn experience
    for (int i = 0; i < 100; i++)
    {
        App::GetCameraManager()->Update(0.02f, nullptr, 1.0f);
    }

    return true;
}

void GameContext::UnloadTerrain()
{
    if (App::GetSimTerrain() != nullptr)
    {
        // remove old terrain
        delete(App::GetSimTerrain());
        App::SetSimTerrain(nullptr);
    }
}

// --------------------------------
// Actors (physics and netcode)

Actor* GameContext::SpawnActor(ActorSpawnRequest& rq)
{
    // --- From `SimController::UpdateSimulation() ~~ for (ActorSpawnRequest& rq: m_actor_spawn_queue)`

    if (rq.asr_origin == ActorSpawnRequest::Origin::USER)
    {
        m_last_cache_selection = rq.asr_cache_entry;
        m_last_skin_selection  = rq.asr_skin_entry;
        m_last_section_config  = rq.asr_config;

        if (rq.asr_spawnbox == nullptr)
        {
            if (m_player_actor != nullptr)
            {
                float h = m_player_actor->GetMaxHeight(true);
                rq.asr_rotation = Ogre::Quaternion(Ogre::Degree(270) - Ogre::Radian(m_player_actor->getRotation()), Ogre::Vector3::UNIT_Y);
                rq.asr_position = m_player_actor->GetRotationCenter();
                rq.asr_position.y = App::GetSimTerrain()->GetCollisions()->getSurfaceHeightBelow(rq.asr_position.x, rq.asr_position.z, h);
                rq.asr_position.y += m_player_actor->GetHeightAboveGroundBelow(h, true); // retain height above ground
            }
            else
            {
                Character* player_character = this->GetPlayerCharacter();
                rq.asr_rotation = Ogre::Quaternion(Ogre::Degree(180) - player_character->getRotation(), Ogre::Vector3::UNIT_Y);
                rq.asr_position = player_character->getPosition();
            }
        }
    }

    // --- From `SimController::SpawnActorDirectly()`
    LOG(" ===== LOADING VEHICLE: " + rq.asr_filename);

    if (rq.asr_cache_entry != nullptr)
    {
        rq.asr_filename = rq.asr_cache_entry->fname;
    }

    std::shared_ptr<RigDef::File> def = m_actor_manager.FetchActorDef(
        rq.asr_filename, rq.asr_origin == ActorSpawnRequest::Origin::TERRN_DEF);
    if (def == nullptr)
    {
        return nullptr; // Error already reported
    }

    if (rq.asr_skin_entry != nullptr)
    {
        std::shared_ptr<SkinDef> skin_def = App::GetCacheSystem()->FetchSkinDef(rq.asr_skin_entry); // Make sure it exists
        if (skin_def == nullptr)
        {
            rq.asr_skin_entry = nullptr; // Error already logged
        }
    }

#ifdef USE_SOCKETW
    if (rq.asr_origin != ActorSpawnRequest::Origin::NETWORK)
    {
        if (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED)
        {
            RoRnet::UserInfo info = App::GetNetwork()->GetLocalUserData();
            rq.asr_net_username = tryConvertUTF(info.username);
            rq.asr_net_color    = info.colournum;
        }
    }
#endif //SOCKETW

    Actor* fresh_actor = m_actor_manager.CreateActorInstance(rq, def);

    // lock slide nodes after spawning the actor?
    if (def->slide_nodes_connect_instantly)
    {
        fresh_actor->ToggleSlideNodeLock();
    }

    // --- Continued `SimController::UpdateSimulation() | for (ActorSpawnRequest& rq: m_actor_spawn_queue)`

    if (rq.asr_origin == ActorSpawnRequest::Origin::USER)
    {
        if (fresh_actor->ar_driveable != NOT_DRIVEABLE)
        {
            this->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, (void*)fresh_actor));
        }
        if (rq.asr_spawnbox == nullptr)
        {
            // Try to resolve collisions with other actors
            fresh_actor->resolveCollisions(50.0f, m_player_actor == nullptr);
        }
    }
    else if (rq.asr_origin == ActorSpawnRequest::Origin::CONFIG_FILE)
    {
        if (fresh_actor->ar_driveable != NOT_DRIVEABLE &&
            fresh_actor->ar_num_nodes > 0 &&
            App::diag_preset_veh_enter->GetBool())
        {
            this->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, (void*)fresh_actor));
        }
    }
    else if (rq.asr_origin == ActorSpawnRequest::Origin::TERRN_DEF)
    {
        if (rq.asr_terrn_machine)
        {
            fresh_actor->ar_driveable = MACHINE;
        }
    }
    else if (rq.asr_origin == ActorSpawnRequest::Origin::NETWORK)
    {
        fresh_actor->ar_net_source_id = rq.net_source_id;
        fresh_actor->ar_net_stream_id = rq.net_stream_id;
    }
    else
    {
        if (fresh_actor->ar_driveable != NOT_DRIVEABLE &&
            rq.asr_origin != ActorSpawnRequest::Origin::NETWORK &&
            rq.asr_origin != ActorSpawnRequest::Origin::SAVEGAME)
        {
            this->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, (void*)fresh_actor));
        }
    }

    return fresh_actor;
}

void GameContext::ModifyActor(ActorModifyRequest& rq)
{
    if (rq.amr_type == ActorModifyRequest::Type::SOFT_RESET)
    {
        rq.amr_actor->SoftReset();
    }
    if ((rq.amr_type == ActorModifyRequest::Type::RESET_ON_SPOT) ||
        (rq.amr_type == ActorModifyRequest::Type::RESET_ON_INIT_POS))
    {
        rq.amr_actor->SyncReset(rq.amr_type == ActorModifyRequest::Type::RESET_ON_INIT_POS);
    }
    if (rq.amr_type == ActorModifyRequest::Type::RELOAD)
    {
        auto reload_pos = rq.amr_actor->getPosition();
        auto reload_dir = Ogre::Quaternion(Ogre::Degree(270) - Ogre::Radian(m_player_actor->getRotation()), Ogre::Vector3::UNIT_Y);
        auto debug_view = rq.amr_actor->GetGfxActor()->GetDebugView();
        auto asr_config = rq.amr_actor->GetSectionConfig();
        auto used_skin  = rq.amr_actor->GetUsedSkin();

        reload_pos.y = m_player_actor->GetMinHeight();

        m_prev_player_actor = nullptr;
        std::string filename = rq.amr_actor->ar_filename;
        this->DeleteActor(rq.amr_actor);
        App::GetCacheSystem()->UnloadActorFromMemory(filename); // Force reload from filesystem

        ActorSpawnRequest* srq = new ActorSpawnRequest;
        srq->asr_position   = reload_pos;
        srq->asr_rotation   = reload_dir;
        srq->asr_config     = asr_config;
        srq->asr_skin_entry = used_skin;
        srq->asr_filename   = filename;
        srq->asr_debugview  = (int)debug_view;
        srq->asr_origin     = ActorSpawnRequest::Origin::USER;
        this->PushMessage(Message(MSG_SIM_SPAWN_ACTOR_REQUESTED, (void*)srq));
    }    
}

void GameContext::DeleteActor(Actor* actor)
{
    // --- From `SimController::UpdateSimulation() | for (Actor* a: m_actor_remove_queue)`
    if (actor == m_player_actor)
    {
        Ogre::Vector3 center = m_player_actor->GetRotationCenter();
        this->ChangePlayerActor(nullptr); // Get out of the vehicle
        this->GetPlayerCharacter()->setPosition(center);
    }

    if (actor == m_prev_player_actor)
    {
        m_prev_player_actor = nullptr;
    }

    // Find linked actors and un-tie if tied
    auto linked_actors = actor->GetAllLinkedActors();
    for (auto actorx : m_actor_manager.GetLocalActors())
    {
        if (actorx->isTied() && std::find(linked_actors.begin(), linked_actors.end(), actorx) != linked_actors.end())
        {
            actorx->ToggleTies();
        }
    }

    // --- From `SimController::RemoveActorDirectly()`

    App::GetGfxScene()->RemoveGfxActor(actor->GetGfxActor());

#ifdef USE_SOCKETW
    if (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED)
    {
        m_character_factory.UndoRemoteActorCoupling(actor);
    }
#endif //SOCKETW

    m_actor_manager.DeleteActorInternal(actor);
}

void GameContext::ChangePlayerActor(Actor* actor)
{
    Actor* prev_player_actor = m_player_actor;
    m_player_actor = actor;

    // hide any old dashes
    if (prev_player_actor && prev_player_actor->ar_dashboard)
    {
        prev_player_actor->ar_dashboard->setVisible3d(false);
    }
    // show new
    if (m_player_actor && m_player_actor->ar_dashboard)
    {
        m_player_actor->ar_dashboard->setVisible3d(true);
    }

    if (prev_player_actor)
    {
        App::GetOverlayWrapper()->showDashboardOverlays(false, prev_player_actor);

        prev_player_actor->GetGfxActor()->SetRenderdashActive(false);

        SOUND_STOP(prev_player_actor, SS_TRIG_AIR);
        SOUND_STOP(prev_player_actor, SS_TRIG_PUMP);
    }

    if (m_player_actor == nullptr)
    {
        // getting outside

        if (prev_player_actor)
        {
            if (prev_player_actor->GetGfxActor()->GetVideoCamState() == GfxActor::VideoCamState::VCSTATE_ENABLED_ONLINE)
            {
                prev_player_actor->GetGfxActor()->SetVideoCamState(GfxActor::VideoCamState::VCSTATE_ENABLED_OFFLINE);
            }

            prev_player_actor->prepareInside(false);

            // get player out of the vehicle
            float h = prev_player_actor->getMinCameraRadius();
            float rotation = prev_player_actor->getRotation() - Ogre::Math::HALF_PI;
            Ogre::Vector3 position = prev_player_actor->getPosition();
            if (prev_player_actor->ar_cinecam_node[0] != -1)
            {
                // actor has a cinecam (find optimal exit position)
                Ogre::Vector3 l = position - 2.0f * prev_player_actor->GetCameraRoll();
                Ogre::Vector3 r = position + 2.0f * prev_player_actor->GetCameraRoll();
                float l_h = App::GetSimTerrain()->GetCollisions()->getSurfaceHeightBelow(l.x, l.z, l.y + h);
                float r_h = App::GetSimTerrain()->GetCollisions()->getSurfaceHeightBelow(r.x, r.z, r.y + h);
                position  = std::abs(r.y - r_h) * 1.2f < std::abs(l.y - l_h) ? r : l;
            }
            position.y = App::GetSimTerrain()->GetCollisions()->getSurfaceHeightBelow(position.x, position.z, position.y + h);

            Character* player_character = this->GetPlayerCharacter();
            if (player_character)
            {
                player_character->SetActorCoupling(false, nullptr);
                player_character->setRotation(Ogre::Radian(rotation));
                player_character->setPosition(position);
            }
        }

        App::GetAppContext()->GetForceFeedback().SetEnabled(false);

        TRIGGER_EVENT(SE_TRUCK_EXIT, prev_player_actor?prev_player_actor->ar_instance_id:-1);
    }
    else
    {
        // getting inside
        App::GetOverlayWrapper()->showDashboardOverlays(
            !App::GetSimController()->IsGUIHidden(), m_player_actor);


        if (m_player_actor->GetGfxActor()->GetVideoCamState() == GfxActor::VideoCamState::VCSTATE_ENABLED_OFFLINE)
        {
            m_player_actor->GetGfxActor()->SetVideoCamState(GfxActor::VideoCamState::VCSTATE_ENABLED_ONLINE);
        }

        m_player_actor->GetGfxActor()->SetRenderdashActive(true);

        // force feedback
        App::GetAppContext()->GetForceFeedback().SetEnabled(m_player_actor->ar_driveable == TRUCK); //only for trucks so far

        // attach player to vehicle
        Character* player_character = this->GetPlayerCharacter();
        if (player_character)
        {
            player_character->SetActorCoupling(true, m_player_actor);
        }

        TRIGGER_EVENT(SE_TRUCK_ENTER, m_player_actor?m_player_actor->ar_instance_id:-1);
    }

    if (prev_player_actor != nullptr || m_player_actor != nullptr)
    {
        App::GetCameraManager()->NotifyVehicleChanged(m_player_actor);
    }

    m_actor_manager.UpdateSleepingState(m_player_actor, 0.f);
}

Actor* GameContext::FetchPrevVehicleOnList()
{
    return m_actor_manager.FetchPreviousVehicleOnList(m_player_actor, m_prev_player_actor);
}

Actor* GameContext::FetchNextVehicleOnList()
{
    return m_actor_manager.FetchNextVehicleOnList(m_player_actor, m_prev_player_actor);
}

void GameContext::UpdateActors(float dt_sec)
{
    m_actor_manager.UpdateActors(m_player_actor, dt_sec);   
}

Actor* GameContext::FindActorByCollisionBox(std::string const & ev_src_instance_name, std::string const & box_name)
{
    return m_actor_manager.FindActorInsideBox(App::GetSimTerrain()->GetCollisions(),
                                              ev_src_instance_name, box_name);
}

void GameContext::RespawnLastActor()
{
    if (m_last_cache_selection != nullptr)
    {
        ActorSpawnRequest* rq = new ActorSpawnRequest;
        rq->asr_cache_entry     = m_last_cache_selection;
        rq->asr_config          = m_last_section_config;
        rq->asr_skin_entry      = m_last_skin_selection;
        rq->asr_origin          = ActorSpawnRequest::Origin::USER;
        App::GetGameContext()->PushMessage(Message(MSG_SIM_SPAWN_ACTOR_REQUESTED, (void*)rq));
    }
}

void GameContext::ShowLoaderGUI(int type, const Ogre::String& instance, const Ogre::String& box)
{
    // first, test if the place if clear, BUT NOT IN MULTIPLAYER
    if (!(App::mp_state->GetEnum<MpState>() == MpState::CONNECTED))
    {
        collision_box_t* spawnbox = App::GetSimTerrain()->GetCollisions()->getBox(instance, box);
        for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
        {
            for (int i = 0; i < actor->ar_num_nodes; i++)
            {
                if (App::GetSimTerrain()->GetCollisions()->isInside(actor->ar_nodes[i].AbsPosition, spawnbox))
                {
                    App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Please clear the place first"), "error.png");
                    return;
                }
            }
        }
    }

    m_current_selection.asr_position = App::GetSimTerrain()->GetCollisions()->getPosition(instance, box);
    m_current_selection.asr_rotation = App::GetSimTerrain()->GetCollisions()->getDirection(instance, box);
    m_current_selection.asr_spawnbox = App::GetSimTerrain()->GetCollisions()->getBox(instance, box);
    App::GetGuiManager()->GetMainSelector()->Show(LoaderType(type));
}

void GameContext::OnLoaderGuiCancel()
{
    m_current_selection = ActorSpawnRequest(); // Reset
}

void GameContext::OnLoaderGuiApply(LoaderType type, CacheEntry* entry, std::string sectionconfig)
{
    bool spawn_now = false;
    switch (type)
    {
    case LT_Skin:
        m_current_selection.asr_skin_entry = entry;
        spawn_now = true;
        break;

    case LT_Vehicle:
    case LT_Truck:
    case LT_Car:
    case LT_Boat:
    case LT_Airplane:
    case LT_Trailer:
    case LT_Train:
    case LT_Load:
    case LT_Extension:
    case LT_AllBeam:
        m_current_selection.asr_cache_entry = entry;
        m_current_selection.asr_config = sectionconfig;
        m_current_selection.asr_origin = ActorSpawnRequest::Origin::USER;
        // Look for extra skins
        if (!entry->guid.empty())
        {
            CacheQuery skin_query;
            skin_query.cqy_filter_guid = entry->guid;
            skin_query.cqy_filter_type = LT_Skin;
            if (App::GetCacheSystem()->Query(skin_query) > 0)
            {
                App::GetGuiManager()->GetMainSelector()->Show(LT_Skin, entry->guid);
            }
            else
            {
                spawn_now = true;
            }
        }
        else
        {
            spawn_now = true;
        }
        break;

    default:;
    }

    if (spawn_now)
    {
        ActorSpawnRequest* rq = new ActorSpawnRequest;
        *rq = m_current_selection;
        App::GetGameContext()->PushMessage(Message(MSG_SIM_SPAWN_ACTOR_REQUESTED, (void*)rq));
        m_current_selection = ActorSpawnRequest(); // Reset
    }
}

// --------------------------------
// Characters

Character* GameContext::GetPlayerCharacter() // Convenience ~ counterpart of `GetPlayerActor()`
{
    return m_character_factory.GetLocalCharacter();
}

// --------------------------------
// Gameplay feats

void GameContext::TeleportPlayer(float x, float z)
{
    float y = App::GetSimTerrain()->GetCollisions()->getSurfaceHeight(x, z);
    if (!App::GetGameContext()->GetPlayerActor())
    {
        App::GetGameContext()->GetPlayerCharacter()->setPosition(Ogre::Vector3(x, y, z));
        return;
    }

    TRIGGER_EVENT(SE_TRUCK_TELEPORT, App::GetGameContext()->GetPlayerActor()->ar_instance_id);

    Ogre::Vector3 translation = Ogre::Vector3(x, y, z) - App::GetGameContext()->GetPlayerActor()->ar_nodes[0].AbsPosition;

    auto actors = App::GetGameContext()->GetPlayerActor()->GetAllLinkedActors();
    actors.push_back(App::GetGameContext()->GetPlayerActor());

    float src_agl = std::numeric_limits<float>::max(); 
    float dst_agl = std::numeric_limits<float>::max(); 
    for (auto actor : actors)
    {
        for (int i = 0; i < actor->ar_num_nodes; i++)
        {
            Ogre::Vector3 pos = actor->ar_nodes[i].AbsPosition;
            src_agl = std::min(pos.y - App::GetSimTerrain()->GetCollisions()->getSurfaceHeight(pos.x, pos.z), src_agl);
            pos += translation;
            dst_agl = std::min(pos.y - App::GetSimTerrain()->GetCollisions()->getSurfaceHeight(pos.x, pos.z), dst_agl);
        }
    }

    translation += Ogre::Vector3::UNIT_Y * (std::max(0.0f, src_agl) - dst_agl);

    for (auto actor : actors)
    {
        actor->ResetPosition(actor->ar_nodes[0].AbsPosition + translation, false);
    }
}
