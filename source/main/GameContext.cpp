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
#include "Actor.h"
#include "AeroEngine.h"
#include "CacheSystem.h"
#include "Collisions.h"
#include "Console.h"
#include "DashBoardManager.h"
#include "EngineSim.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "GUI_FrictionSettings.h"
#include "GUI_MainSelector.h"
#include "InputEngine.h"
#include "OverlayWrapper.h"
#include "Replay.h"
#include "ScrewProp.h"
#include "ScriptEngine.h"
#include "SkyManager.h"
#include "SkyXManager.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "Utils.h"
#include "VehicleAI.h"

using namespace RoR;

// --------------------------------
// Message queue

void GameContext::PushMessage(Message m)
{
    std::lock_guard<std::mutex> lock(m_msg_mutex);
    m_msg_queue.push(m);
}

void GameContext::ChainMessage(Message m)
{
    std::lock_guard<std::mutex> lock(m_msg_mutex);
    if (!m_msg_queue.empty())
    {
        m_msg_queue.back().chain.push_back(m);
    }
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

    LOG(" ===== LOADING VEHICLE: " + rq.asr_filename);

    if (rq.asr_cache_entry != nullptr)
    {
        rq.asr_filename = rq.asr_cache_entry->fname;
    }

    Truck::DocumentPtr def = m_actor_manager.FetchTruckDocument(rq);
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

    if (!fresh_actor)
        return nullptr;

    // lock slide nodes after spawning the actor?
    if (fresh_actor->ar_slidenodes_connect_instantly)
    {
        fresh_actor->ToggleSlideNodeLock();
    }

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
    else if (rq.asr_origin == ActorSpawnRequest::Origin::SAVEGAME)
    {
        if (rq.asr_saved_state)
        {
            ActorModifyRequest* req = new ActorModifyRequest();
            req->amr_actor = fresh_actor;
            req->amr_type = ActorModifyRequest::Type::RESTORE_SAVED;
            req->amr_saved_state = rq.asr_saved_state;
            this->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)req));
        }
    }
    else
    {
        if (fresh_actor->ar_driveable != NOT_DRIVEABLE &&
            rq.asr_origin != ActorSpawnRequest::Origin::NETWORK)
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
    if (rq.amr_type == ActorModifyRequest::Type::RESTORE_SAVED)
    {
        m_actor_manager.RestoreSavedState(rq.amr_actor, *rq.amr_saved_state.get());
    }
    if (rq.amr_type == ActorModifyRequest::Type::RELOAD)
    {
        CacheEntry* entry = App::GetCacheSystem()->FindEntryByFilename(LT_AllBeam, /*partial=*/false, rq.amr_actor->ar_filename);
        if (!entry)
        {
            Str<500> msg; msg <<"Cannot reload vehicle; file '" << rq.amr_actor->ar_filename << "' not found in ModCache.";
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_ERROR, msg.ToCStr());
            return;
        }

        // Create spawn request while actor still exists
        ActorSpawnRequest* srq = new ActorSpawnRequest;
        srq->asr_position   = Ogre::Vector3(rq.amr_actor->getPosition().x, rq.amr_actor->GetMinHeight(), rq.amr_actor->getPosition().z);
        srq->asr_rotation   = Ogre::Quaternion(Ogre::Degree(270) - Ogre::Radian(rq.amr_actor->getRotation()), Ogre::Vector3::UNIT_Y);
        srq->asr_config     = rq.amr_actor->GetSectionConfig();
        srq->asr_skin_entry = rq.amr_actor->GetUsedSkin();
        srq->asr_cache_entry= entry;
        srq->asr_debugview  = (int)rq.amr_actor->GetGfxActor()->GetDebugView();
        srq->asr_origin     = ActorSpawnRequest::Origin::USER;

        // This deletes all actors using the resource bundle, including the one we're reloading.
        this->PushMessage(Message(MSG_EDI_RELOAD_BUNDLE_REQUESTED, (void*)entry));

        // Load our actor again, but only after all actors are deleted.
        this->ChainMessage(Message(MSG_SIM_SPAWN_ACTOR_REQUESTED, (void*)srq));
    }
}

void GameContext::DeleteActor(Actor* actor)
{
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

        if (actorx->isLocked() && std::find(linked_actors.begin(), linked_actors.end(), actorx) != linked_actors.end())
        {
            actorx->ToggleHooks();
        }
    }

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
            if (prev_player_actor->ar_cinecam_node[0] != node_t::INVALID_IDX)
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
            !App::GetGuiManager()->IsGuiHidden(), m_player_actor);


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

void GameContext::UpdateActors()
{
    m_actor_manager.UpdateActors(m_player_actor);
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
        this->PushMessage(Message(MSG_SIM_SPAWN_ACTOR_REQUESTED, (void*)rq));
    }
}

void GameContext::SpawnPreselectedActor(std::string const& preset_vehicle, std::string const& preset_veh_config)
{
    CacheEntry* entry = App::GetCacheSystem()->FindEntryByFilename(
        LT_AllBeam, /*partial=*/true, preset_vehicle);

    if (!entry)
        return;

    ActorSpawnRequest* rq = new ActorSpawnRequest;
    rq->asr_cache_entry = entry;
    rq->asr_position    = this->GetPlayerCharacter()->getPosition();
    rq->asr_rotation    = Ogre::Quaternion(Ogre::Degree(180) - this->GetPlayerCharacter()->getRotation(), Ogre::Vector3::UNIT_Y);
    rq->asr_origin      = ActorSpawnRequest::Origin::CONFIG_FILE;

    RoR::LogFormat("[RoR|Diag] Preselected Truck: %s (%s)", entry->dname.c_str(), entry->fname.c_str());

    // Section config lookup
    if (!entry->sectionconfigs.empty())
    {
        if (std::find(entry->sectionconfigs.begin(), entry->sectionconfigs.end(),
                      preset_veh_config)
            == entry->sectionconfigs.end())
        {
            // Preselected config doesn't exist -> use first available one
            rq->asr_config = entry->sectionconfigs[0];
        }
        else
        {
            rq->asr_config = preset_veh_config;
        }
        RoR::LogFormat("[RoR|Diag] Preselected Truck Config: %s", rq->asr_config.c_str());
    }

    this->PushMessage(Message(MSG_SIM_SPAWN_ACTOR_REQUESTED, (void*)rq));
}

void GameContext::ShowLoaderGUI(int type, const Ogre::String& instance, const Ogre::String& box)
{
    // first, test if the place if clear, BUT NOT IN MULTIPLAYER
    if (!(App::mp_state->GetEnum<MpState>() == MpState::CONNECTED))
    {
        collision_box_t* spawnbox = App::GetSimTerrain()->GetCollisions()->getBox(instance, box);
        for (auto actor : this->GetActorManager()->GetActors())
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

    RoR::Message m(MSG_GUI_OPEN_SELECTOR_REQUESTED);
    m.payload = reinterpret_cast<void*>(new LoaderType(LoaderType(type)));
    App::GetGameContext()->PushMessage(m);
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
                App::GetGuiManager()->GetMainSelector()->Show(LT_Skin, entry->guid); // Intentionally not using MSG_
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
        this->PushMessage(Message(MSG_SIM_SPAWN_ACTOR_REQUESTED, (void*)rq));
        m_current_selection = ActorSpawnRequest(); // Reset
    }
}

// --------------------------------
// Characters

void GameContext::CreatePlayerCharacter()
{
    m_character_factory.CreateLocalCharacter();

    // Adjust character position
    Ogre::Vector3 spawn_pos = App::GetSimTerrain()->getSpawnPos();
    float spawn_rot = 0.0f;

    // Classic behavior, retained for compatibility.
    // Required for maps like N-Labs or F1 Track.
    if (!App::GetSimTerrain()->HasPredefinedActors())
    {
        spawn_rot = 180.0f;
    }

    // Preset position - commandline has precedence
    if (App::cli_preset_spawn_pos->GetStr() != "")
    {
        spawn_pos = Ogre::StringConverter::parseVector3(App::cli_preset_spawn_pos->GetStr(), spawn_pos);
        App::cli_preset_spawn_pos->SetStr("");
    }
    else if (App::diag_preset_spawn_pos->GetStr() != "")
    {
        spawn_pos = Ogre::StringConverter::parseVector3(App::diag_preset_spawn_pos->GetStr(), spawn_pos);
        App::diag_preset_spawn_pos->SetStr("");
    }

    // Preset rotation - commandline has precedence
    if (App::cli_preset_spawn_rot->GetStr() != "")
    {
        spawn_rot = Ogre::StringConverter::parseReal(App::cli_preset_spawn_rot->GetStr(), spawn_rot);
        App::cli_preset_spawn_rot->SetStr("");
    }
    else if (App::diag_preset_spawn_rot->GetStr() != "")
    {
        spawn_rot = Ogre::StringConverter::parseReal(App::diag_preset_spawn_rot->GetStr(), spawn_rot);
        App::diag_preset_spawn_rot->SetStr("");
    }

    spawn_pos.y = App::GetSimTerrain()->GetCollisions()->getSurfaceHeightBelow(spawn_pos.x, spawn_pos.z, spawn_pos.y + 1.8f);

    this->GetPlayerCharacter()->setPosition(spawn_pos);
    this->GetPlayerCharacter()->setRotation(Ogre::Degree(spawn_rot));

    App::GetCameraManager()->GetCameraNode()->setPosition(this->GetPlayerCharacter()->getPosition());

    // Small hack to improve the spawn experience
    for (int i = 0; i < 100; i++)
    {
        App::GetCameraManager()->UpdateInputEvents(0.02f);
    }
}

Character* GameContext::GetPlayerCharacter() // Convenience ~ counterpart of `GetPlayerActor()`
{
    return m_character_factory.GetLocalCharacter();
}

// --------------------------------
// Gameplay feats

void GameContext::TeleportPlayer(float x, float z)
{
    float y = App::GetSimTerrain()->GetCollisions()->getSurfaceHeight(x, z);
    if (!this->GetPlayerActor())
    {
        this->GetPlayerCharacter()->setPosition(Ogre::Vector3(x, y, z));
        return;
    }

    TRIGGER_EVENT(SE_TRUCK_TELEPORT, this->GetPlayerActor()->ar_instance_id);

    Ogre::Vector3 translation = Ogre::Vector3(x, y, z) - this->GetPlayerActor()->ar_nodes[0].AbsPosition;

    auto actors = this->GetPlayerActor()->GetAllLinkedActors();
    actors.push_back(this->GetPlayerActor());

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

void GameContext::UpdateGlobalInputEvents()
{
    // Generic escape key event
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUIT_GAME))
    {
        if (App::app_state->GetEnum<AppState>() == AppState::MAIN_MENU)
        {
            if (App::GetGuiManager()->IsVisible_GameAbout())
            {
                App::GetGuiManager()->SetVisible_GameAbout(false);
            }
            else if (App::GetGuiManager()->IsVisible_MainSelector())
            {
                App::GetGuiManager()->GetMainSelector()->Close();
            }
            else if (App::GetGuiManager()->IsVisible_GameSettings())
            {
                App::GetGuiManager()->SetVisible_GameSettings(false);
            }
            else if (App::GetGuiManager()->IsVisible_MultiplayerSelector())
            {
                App::GetGuiManager()->SetVisible_MultiplayerSelector(false);
            }
            else
            {
                this->PushMessage(Message(MSG_APP_SHUTDOWN_REQUESTED));
            }
        }
        else if (App::app_state->GetEnum<AppState>() == AppState::SIMULATION)
        {
            if (App::GetGuiManager()->IsVisible_MainSelector())
            {
                App::GetGuiManager()->GetMainSelector()->Close();
            }
            else if (App::sim_state->GetEnum<SimState>() == SimState::RUNNING)
            {
                this->PushMessage(Message(MSG_SIM_PAUSE_REQUESTED));
            }
            else if (App::sim_state->GetEnum<SimState>() == SimState::PAUSED)
            {
                this->PushMessage(Message(MSG_SIM_UNPAUSE_REQUESTED));
            }
        }
    }

    // screenshot
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_SCREENSHOT, 0.25f))
    {
        this->PushMessage(Message(MSG_APP_SCREENSHOT_REQUESTED));
    }

    // fullscreen toggle
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FULLSCREEN_TOGGLE, 2.0f))
    {   
        if (App::GetAppContext()->GetRenderWindow()->isFullScreen())
            this->PushMessage(Message(MSG_APP_DISPLAY_WINDOWED_REQUESTED));
        else
            this->PushMessage(Message(MSG_APP_DISPLAY_FULLSCREEN_REQUESTED));
    }

    // toggle render mode
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_RENDER_MODE, 0.5f))
    {
        static int mSceneDetailIndex;
        mSceneDetailIndex = (mSceneDetailIndex + 1) % 3;
        switch (mSceneDetailIndex)
        {
        case 0: App::GetCameraManager()->GetCamera()->setPolygonMode(Ogre::PM_SOLID);       break;
        case 1: App::GetCameraManager()->GetCamera()->setPolygonMode(Ogre::PM_WIREFRAME);   break;
        case 2: App::GetCameraManager()->GetCamera()->setPolygonMode(Ogre::PM_POINTS);      break;
        }
    }

    // Write player position to log
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_OUTPUT_POSITION))
    {
        Ogre::Vector3 position(Ogre::Vector3::ZERO);
        Ogre::Radian rotation(0);
        if (this->GetPlayerActor() == nullptr)
        {
            position = this->GetPlayerCharacter()->getPosition();
            rotation = this->GetPlayerCharacter()->getRotation() + Ogre::Radian(Ogre::Math::PI);
        }
        else
        {
            position = this->GetPlayerActor()->getPosition();
            rotation = this->GetPlayerActor()->getRotation();
        }
        Ogre::String pos = Ogre::StringUtil::format("%8.3f, %8.3f, %8.3f"   , position.x, position.y, position.z);
        Ogre::String rot = Ogre::StringUtil::format("% 6.1f, % 6.1f, % 6.1f",       0.0f, rotation.valueDegrees()  ,       0.0f);
        LOG("Position: " + pos + ", " + rot);
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_RESET_MODE))
    {
        App::sim_soft_reset_mode->SetVal(!App::sim_soft_reset_mode->GetBool());
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
            (App::sim_soft_reset_mode->GetBool()) ? _L("Enabled soft reset mode") : _L("Enabled hard reset mode"));
    }
}

void GameContext::UpdateSimInputEvents(float dt)
{
    // get new vehicle
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_GET_NEW_VEHICLE))
    {
        RoR::Message m(MSG_GUI_OPEN_SELECTOR_REQUESTED);
        m.payload = reinterpret_cast<void*>(new LoaderType(LT_AllBeam));
        App::GetGameContext()->PushMessage(m);
    }

    // Enter/exit truck - With a toggle delay
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_OR_EXIT_TRUCK, 0.5f))
    {
        if (this->GetPlayerActor() == nullptr) // no vehicle
        {
            // find the nearest vehicle
            float mindist = 1000.0;
            Actor* nearest_actor = nullptr;
            for (auto actor : this->GetActorManager()->GetActors())
            {
                if (!actor->ar_driveable)
                    continue;
                if (actor->ar_cinecam_node[0] == node_t::INVALID_IDX)
                {
                    LOG("cinecam missing, cannot enter the actor!");
                    continue;
                }
                float len = 0.0f;
                if (this->GetPlayerCharacter())
                {
                    len = actor->ar_nodes[actor->ar_cinecam_node[0]].AbsPosition.distance(this->GetPlayerCharacter()->getPosition() + Ogre::Vector3(0.0, 2.0, 0.0));
                }
                if (len < mindist)
                {
                    mindist = len;
                    nearest_actor = actor;
                }
            }

            if (mindist < 20.0)
            {
                this->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, (void*)nearest_actor));
            }
        }
        else // We're in a vehicle -> If moving slowly enough, get out
        {
            if (this->GetPlayerActor()->ar_nodes[0].Velocity.squaredLength() < 1.0f ||
                this->GetPlayerActor()->ar_sim_state == Actor::SimState::NETWORKED_OK)
            {
                this->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, nullptr));
            }
        }
    }

    // enter next truck
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_NEXT_TRUCK, 0.25f))
    {
        Actor* actor = this->FetchNextVehicleOnList();
        if (actor != this->GetPlayerActor())
        {
            this->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, (void*)actor));
        }
    }

    // enter previous truck
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_PREVIOUS_TRUCK, 0.25f))
    {
        Actor* actor = this->FetchPrevVehicleOnList();
        if (actor != this->GetPlayerActor())
        {
            this->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, (void*)actor));
        }
    }

    // respawn last truck
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESPAWN_LAST_TRUCK, 0.25f))
    {
        this->RespawnLastActor();
    }

    // terrain editor toggle
    bool toggle_editor = (App::GetGameContext()->GetPlayerActor() && App::sim_state->GetEnum<SimState>() == SimState::EDITOR_MODE) ||
        (!App::GetGameContext()->GetPlayerActor() && RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TERRAIN_EDITOR));
    if (toggle_editor)
    {
        Message m(App::sim_state->GetEnum<SimState>() == SimState::EDITOR_MODE ?
                    MSG_EDI_LEAVE_TERRN_EDITOR_REQUESTED : MSG_EDI_ENTER_TERRN_EDITOR_REQUESTED);
        App::GetGameContext()->PushMessage(m);
    }

    // forward commands from character
    if (!m_player_actor)
    {
        // Find nearest actor
        const Ogre::Vector3 position = App::GetGameContext()->GetPlayerCharacter()->getPosition();
        Actor* nearest_actor = nullptr;
        float min_squared_distance = std::numeric_limits<float>::max();
        for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
        {
            float squared_distance = position.squaredDistance(actor->ar_nodes[0].AbsPosition);
            if (squared_distance < min_squared_distance)
            {
                min_squared_distance = squared_distance;
                nearest_actor = actor;
            }
        }

        // Evaluate
        if (nearest_actor != nullptr &&
            nearest_actor->ar_import_commands &&
            min_squared_distance < (nearest_actor->getMinCameraRadius()*nearest_actor->getMinCameraRadius()))
        {
            // get commands
            // -- here we should define a maximum numbers per actor. Some actors does not have that much commands
            // -- available, so why should we iterate till MAX_COMMANDS?
            for (int i = 1; i <= MAX_COMMANDS + 1; i++)
            {
                int eventID = EV_COMMANDS_01 + (i - 1);

                nearest_actor->ar_command_key[i].playerInputValue = RoR::App::GetInputEngine()->getEventValue(eventID);
            }
        }
    }
}

void GameContext::UpdateSkyInputEvents(float dt)
{
#ifdef USE_CAELUM
    if (App::gfx_sky_mode->GetEnum<GfxSkyMode>() == GfxSkyMode::CAELUM &&
        App::GetSimTerrain()->getSkyManager())
    {
        float time_factor = 1.0f;

        if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_INCREASE_TIME))
        {
            time_factor = 1000.0f;
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_INCREASE_TIME_FAST))
        {
            time_factor = 10000.0f;
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_DECREASE_TIME))
        {
            time_factor = -1000.0f;
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_DECREASE_TIME_FAST))
        {
            time_factor = -10000.0f;
        }

        if (App::GetSimTerrain()->getSkyManager()->GetSkyTimeFactor() != time_factor)
        {
            App::GetSimTerrain()->getSkyManager()->SetSkyTimeFactor(time_factor);
            Str<200> msg; msg << _L("Time set to ") << App::GetSimTerrain()->getSkyManager()->GetPrettyTime();
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, msg.ToCStr());
        }
    }

#endif // USE_CAELUM
    if (App::gfx_sky_mode->GetEnum<GfxSkyMode>() == GfxSkyMode::SKYX &&
        App::GetSimTerrain()->getSkyXManager())
    {
        if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_INCREASE_TIME))
        {
            App::GetSimTerrain()->getSkyXManager()->GetSkyX()->setTimeMultiplier(1.0f);
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_INCREASE_TIME_FAST))
        {
            App::GetSimTerrain()->getSkyXManager()->GetSkyX()->setTimeMultiplier(2.0f);
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_DECREASE_TIME))
        {
            App::GetSimTerrain()->getSkyXManager()->GetSkyX()->setTimeMultiplier(-1.0f);
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_DECREASE_TIME_FAST))
        {
            App::GetSimTerrain()->getSkyXManager()->GetSkyX()->setTimeMultiplier(-2.0f);
        }
        else
        {
            App::GetSimTerrain()->getSkyXManager()->GetSkyX()->setTimeMultiplier(0.01f);
        }
    }
}

void GameContext::UpdateCommonInputEvents(float dt)
{
    // reload current truck
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCKEDIT_RELOAD, 0.5f))
    {
        ActorModifyRequest* rq = new ActorModifyRequest;
        rq->amr_type = ActorModifyRequest::Type::RELOAD;
        rq->amr_actor = m_player_actor;
        this->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
    }

    // remove current truck
    if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_REMOVE_CURRENT_TRUCK))
    {
        App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, (void*)m_player_actor));
    }

    // blinkers
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_LEFT))
        m_player_actor->toggleBlinkType(BLINK_LEFT);

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_RIGHT))
        m_player_actor->toggleBlinkType(BLINK_RIGHT);

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_WARN))
        m_player_actor->toggleBlinkType(BLINK_WARN);

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TRUCK_REMOVE))
    {
        App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, (void*)m_player_actor));
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ROPELOCK))
    {
        m_player_actor->ar_toggle_ropes = true;
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_LOCK))
    {
        m_player_actor->ToggleHooks(-1, HOOK_TOGGLE, -1);
        m_player_actor->ToggleSlideNodeLock();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_AUTOLOCK))
    {
        m_player_actor->ToggleHooks(-2, HOOK_UNLOCK, -1); //unlock all autolocks
    }

    //strap
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_SECURE_LOAD))
    {
        m_player_actor->ar_toggle_ties = true;
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_CUSTOM_PARTICLES))
    {
        m_player_actor->ToggleCustomParticles();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_DEBUG_VIEW))
    {
        m_player_actor->GetGfxActor()->ToggleDebugView();
        for (auto actor : m_player_actor->GetAllLinkedActors())
        {
            actor->GetGfxActor()->SetDebugView(m_player_actor->GetGfxActor()->GetDebugView());
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_CYCLE_DEBUG_VIEWS))
    {
        m_player_actor->GetGfxActor()->CycleDebugViews();
        for (auto actor : m_player_actor->GetAllLinkedActors())
        {
            actor->GetGfxActor()->SetDebugView(m_player_actor->GetGfxActor()->GetDebugView());
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_LIGHTS))
    {
        m_player_actor->ToggleLights();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_BEACONS))
    {
        m_player_actor->ToggleBeacons();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESCUE_TRUCK, 0.5f) &&
        App::mp_state->GetEnum<MpState>() != MpState::CONNECTED &&
        m_player_actor->ar_driveable != AIRPLANE)
    {
        Actor* rescuer = nullptr;
        for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
        {
            if (actor->ar_rescuer_flag)
            {
                rescuer = actor;
            }
        }
        if (rescuer == nullptr)
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("No rescue truck found!"), "warning.png");
        }
        else
        {
            App::GetGameContext()->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, (void*)rescuer));
        }
    }

    // parking brake
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TRAILER_PARKING_BRAKE))
    {
        if (m_player_actor->ar_driveable == TRUCK)
            m_player_actor->ar_trailer_parking_brake = !m_player_actor->ar_trailer_parking_brake;
        else if (m_player_actor->ar_driveable == NOT_DRIVEABLE)
            m_player_actor->ToggleParkingBrake();
    }

    // videocam
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_VIDEOCAMERA, 0.5f))
    {
        if (m_player_actor->GetGfxActor()->GetVideoCamState() == GfxActor::VideoCamState::VCSTATE_DISABLED)
        {
            m_player_actor->GetGfxActor()->SetVideoCamState(GfxActor::VideoCamState::VCSTATE_ENABLED_ONLINE);
        }
        else
        {
            m_player_actor->GetGfxActor()->SetVideoCamState(GfxActor::VideoCamState::VCSTATE_DISABLED);
        }
    }

    // enter/exit truck - Without a delay: the vehicle must brake like braking normally
    if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_ENTER_OR_EXIT_TRUCK))
    {
        m_player_actor->ar_brake = 0.66f;
    }

    // toggle physics
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_PHYSICS))
    {
        for (auto actor : App::GetGameContext()->GetPlayerActor()->GetAllLinkedActors())
        {
            actor->ar_physics_paused = !App::GetGameContext()->GetPlayerActor()->ar_physics_paused;
        }
        App::GetGameContext()->GetPlayerActor()->ar_physics_paused = !App::GetGameContext()->GetPlayerActor()->ar_physics_paused;
    }

    // reset truck
    if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_RESET_TRUCK))
    {
        ActorModifyRequest* rq = new ActorModifyRequest;
        rq->amr_actor = App::GetGameContext()->GetPlayerActor();
        rq->amr_type  = ActorModifyRequest::Type::RESET_ON_INIT_POS;
        App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
    }

    // all commands
    for (int i = 1; i <= MAX_COMMANDS + 1; i++)
    {
        int eventID = EV_COMMANDS_01 + (i - 1);

        m_player_actor->ar_command_key[i].playerInputValue = RoR::App::GetInputEngine()->getEventValue(eventID);
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_FORWARDCOMMANDS))
    {
        m_player_actor->ar_forward_commands = !m_player_actor->ar_forward_commands;
        if (m_player_actor->ar_forward_commands)
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("forwardcommands enabled"), "information.png", 3000);
        }
        else
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("forwardcommands disabled"), "information.png", 3000);
        }
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_IMPORTCOMMANDS))
    {
        m_player_actor->ar_import_commands = !m_player_actor->ar_import_commands;
        if (m_player_actor->ar_import_commands)
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("importcommands enabled"), "information.png", 3000);
        }
        else
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("importcommands disabled"), "information.png", 3000);
        }
    }

    if (m_player_actor->GetReplay())
    {
        m_player_actor->GetReplay()->UpdateInputEvents();
    }
}

void GameContext::UpdateAirplaneInputEvents(float dt)
{
    if (m_player_actor->isBeingReset() || m_player_actor->ar_physics_paused)
        return;

    // autopilot
    if (m_player_actor->ar_autopilot && m_player_actor->ar_autopilot->wantsdisconnect)
    {
        m_player_actor->ar_autopilot->disconnect();
    }

    // steer
    float commandrate = 4.0;
    float tmp_left = App::GetInputEngine()->getEventValue(EV_AIRPLANE_STEER_LEFT);
    float tmp_right = App::GetInputEngine()->getEventValue(EV_AIRPLANE_STEER_RIGHT);
    float sum_steer = -tmp_left + tmp_right;
    App::GetInputEngine()->smoothValue(m_player_actor->ar_aileron, sum_steer, dt * commandrate);
    m_player_actor->ar_hydro_dir_command = m_player_actor->ar_aileron;
    m_player_actor->ar_hydro_speed_coupling = !(App::GetInputEngine()->isEventAnalog(EV_AIRPLANE_STEER_LEFT) && App::GetInputEngine()->isEventAnalog(EV_AIRPLANE_STEER_RIGHT));

    // pitch
    float tmp_pitch_up = App::GetInputEngine()->getEventValue(EV_AIRPLANE_ELEVATOR_UP);
    float tmp_pitch_down = App::GetInputEngine()->getEventValue(EV_AIRPLANE_ELEVATOR_DOWN);
    float sum_pitch = tmp_pitch_down - tmp_pitch_up;
    App::GetInputEngine()->smoothValue(m_player_actor->ar_elevator, sum_pitch, dt * commandrate);

    // rudder
    float tmp_rudder_left = App::GetInputEngine()->getEventValue(EV_AIRPLANE_RUDDER_LEFT);
    float tmp_rudder_right = App::GetInputEngine()->getEventValue(EV_AIRPLANE_RUDDER_RIGHT);
    float sum_rudder = tmp_rudder_left - tmp_rudder_right;
    App::GetInputEngine()->smoothValue(m_player_actor->ar_rudder, sum_rudder, dt * commandrate);

    // brakes
    if (!m_player_actor->ar_parking_brake)
    {
        m_player_actor->ar_brake = App::GetInputEngine()->getEventValue(EV_AIRPLANE_BRAKE) * 0.66f;
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_PARKING_BRAKE))
    {
        m_player_actor->ToggleParkingBrake();
    }

    // reverse
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_REVERSE))
    {
        for (int i = 0; i < m_player_actor->ar_num_aeroengines; i++)
        {
            m_player_actor->ar_aeroengines[i]->toggleReverse();
        }
    }

    // engines
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_TOGGLE_ENGINES))
    {
        for (int i = 0; i < m_player_actor->ar_num_aeroengines; i++)
        {
            m_player_actor->ar_aeroengines[i]->flipStart();
        }
    }

    // flaps
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_NONE))
    {
        if (m_player_actor->ar_aerial_flap > 0)
        {
            m_player_actor->ar_aerial_flap = 0;
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_FULL))
    {
        if (m_player_actor->ar_aerial_flap < 5)
        {
            m_player_actor->ar_aerial_flap = 5;
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_LESS))
    {
        if (m_player_actor->ar_aerial_flap > 0)
        {
            m_player_actor->ar_aerial_flap = (m_player_actor->ar_aerial_flap) - 1;
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_MORE))
    {
        if (m_player_actor->ar_aerial_flap < 5)
        {
            m_player_actor->ar_aerial_flap = (m_player_actor->ar_aerial_flap) + 1;
        }
    }

    // airbrakes
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_NONE))
    {
        if (m_player_actor->ar_airbrake_intensity > 0)
        {
            m_player_actor->setAirbrakeIntensity(0);
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_FULL))
    {
        if (m_player_actor->ar_airbrake_intensity < 5)
        {
            m_player_actor->setAirbrakeIntensity(5);
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_LESS))
    {
        if (m_player_actor->ar_airbrake_intensity > 0)
        {
            m_player_actor->setAirbrakeIntensity((m_player_actor->ar_airbrake_intensity) - 1);
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_MORE))
    {
        if (m_player_actor->ar_airbrake_intensity < 5)
        {
            m_player_actor->setAirbrakeIntensity((m_player_actor->ar_airbrake_intensity) + 1);
        }
    }

    // throttle
    float tmp_throttle = App::GetInputEngine()->getEventBoolValue(EV_AIRPLANE_THROTTLE);
    if (tmp_throttle > 0)
    {
        for (int i = 0; i < m_player_actor->ar_num_aeroengines; i++)
        {
            m_player_actor->ar_aeroengines[i]->setThrottle(tmp_throttle);
        }
    }

    if (App::GetInputEngine()->isEventDefined(EV_AIRPLANE_THROTTLE_AXIS))
    {
        float f = App::GetInputEngine()->getEventValue(EV_AIRPLANE_THROTTLE_AXIS);
        for (int i = 0; i < m_player_actor->ar_num_aeroengines; i++)
        {
            m_player_actor->ar_aeroengines[i]->setThrottle(f);
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_DOWN, 0.1f))
    {
        for (int i = 0; i < m_player_actor->ar_num_aeroengines; i++)
        {
            m_player_actor->ar_aeroengines[i]->setThrottle(m_player_actor->ar_aeroengines[i]->getThrottle() - 0.05);
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_UP, 0.1f))
    {
        for (int i = 0; i < m_player_actor->ar_num_aeroengines; i++)
        {
            m_player_actor->ar_aeroengines[i]->setThrottle(m_player_actor->ar_aeroengines[i]->getThrottle() + 0.05);
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_NO, 0.1f))
    {
        for (int i = 0; i < m_player_actor->ar_num_aeroengines; i++)
        {
            m_player_actor->ar_aeroengines[i]->setThrottle(0);
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_FULL, 0.1f))
    {
        for (int i = 0; i < m_player_actor->ar_num_aeroengines; i++)
        {
            m_player_actor->ar_aeroengines[i]->setThrottle(1);
        }
    }

    // autopilot
    if (m_player_actor->ar_autopilot)
    {
        for (int i = 0; i < m_player_actor->ar_num_aeroengines; i++)
        {
            m_player_actor->ar_aeroengines[i]->setThrottle(m_player_actor->ar_autopilot->getThrottle(m_player_actor->ar_aeroengines[i]->getThrottle(), dt));
        }
    }
}

void GameContext::UpdateBoatInputEvents(float dt)
{
    // throttle
    if (App::GetInputEngine()->isEventDefined(EV_BOAT_THROTTLE_AXIS))
    {
        float f = App::GetInputEngine()->getEventValue(EV_BOAT_THROTTLE_AXIS);
        // use negative values also!
        f = f * 2 - 1;
        for (int i = 0; i < m_player_actor->ar_num_screwprops; i++)
            m_player_actor->ar_screwprops[i]->setThrottle(-f);
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_THROTTLE_DOWN, 0.1f))
    {
        for (int i = 0; i < m_player_actor->ar_num_screwprops; i++)
            m_player_actor->ar_screwprops[i]->setThrottle(m_player_actor->ar_screwprops[i]->getThrottle() - 0.05);
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_THROTTLE_UP, 0.1f))
    {
        for (int i = 0; i < m_player_actor->ar_num_screwprops; i++)
            m_player_actor->ar_screwprops[i]->setThrottle(m_player_actor->ar_screwprops[i]->getThrottle() + 0.05);
    }

    // steer
    float tmp_steer_left = App::GetInputEngine()->getEventValue(EV_BOAT_STEER_LEFT);
    float tmp_steer_right = App::GetInputEngine()->getEventValue(EV_BOAT_STEER_RIGHT);
    float stime = App::GetInputEngine()->getEventBounceTime(EV_BOAT_STEER_LEFT) + App::GetInputEngine()->getEventBounceTime(EV_BOAT_STEER_RIGHT);
    float sum_steer = (tmp_steer_left - tmp_steer_right) * dt;
    // do not center the rudder!
    if (fabs(sum_steer) > 0 && stime <= 0)
    {
        for (int i = 0; i < m_player_actor->ar_num_screwprops; i++)
            m_player_actor->ar_screwprops[i]->setRudder(m_player_actor->ar_screwprops[i]->getRudder() + sum_steer);
    }

    if (App::GetInputEngine()->isEventDefined(EV_BOAT_STEER_LEFT_AXIS) && App::GetInputEngine()->isEventDefined(EV_BOAT_STEER_RIGHT_AXIS))
    {
        tmp_steer_left = App::GetInputEngine()->getEventValue(EV_BOAT_STEER_LEFT_AXIS);
        tmp_steer_right = App::GetInputEngine()->getEventValue(EV_BOAT_STEER_RIGHT_AXIS);
        sum_steer = (tmp_steer_left - tmp_steer_right);
        for (int i = 0; i < m_player_actor->ar_num_screwprops; i++)
            m_player_actor->ar_screwprops[i]->setRudder(sum_steer);
    }

    // rudder
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_CENTER_RUDDER, 0.1f))
    {
        for (int i = 0; i < m_player_actor->ar_num_screwprops; i++)
            m_player_actor->ar_screwprops[i]->setRudder(0);
    }

    // reverse
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_REVERSE))
    {
        for (int i = 0; i < m_player_actor->ar_num_screwprops; i++)
            m_player_actor->ar_screwprops[i]->toggleReverse();
    }
}

void GameContext::UpdateTruckInputEvents(float dt)
{
    if (m_player_actor->isBeingReset() || m_player_actor->ar_physics_paused)
        return;
#ifdef USE_ANGELSCRIPT
    if (m_player_actor->ar_vehicle_ai && m_player_actor->ar_vehicle_ai->IsActive())
        return;
#endif // USE_ANGELSCRIPT

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LEFT_MIRROR_LEFT))
        m_player_actor->ar_left_mirror_angle -= 0.001;

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LEFT_MIRROR_RIGHT))
        m_player_actor->ar_left_mirror_angle += 0.001;

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_RIGHT_MIRROR_LEFT))
        m_player_actor->ar_right_mirror_angle -= 0.001;

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_RIGHT_MIRROR_RIGHT))
        m_player_actor->ar_right_mirror_angle += 0.001;

    // steering
    float tmp_left_digital  = App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_LEFT , false, InputEngine::ET_DIGITAL);
    float tmp_right_digital = App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_RIGHT, false, InputEngine::ET_DIGITAL);
    float tmp_left_analog   = App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_LEFT , false, InputEngine::ET_ANALOG);
    float tmp_right_analog  = App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_RIGHT, false, InputEngine::ET_ANALOG);

    float sum = -std::max(tmp_left_digital, tmp_left_analog) + std::max(tmp_right_digital, tmp_right_analog);

    m_player_actor->ar_hydro_dir_command = Ogre::Math::Clamp(sum, -1.0f, 1.0f);

    m_player_actor->ar_hydro_speed_coupling = (tmp_left_digital >= tmp_left_analog) && (tmp_right_digital >= tmp_right_analog);

    if (m_player_actor->ar_engine)
    {
        m_player_actor->ar_engine->UpdateInputEvents(dt);
    }

    if (m_player_actor->ar_brake > 1.0f / 6.0f)
    {
        SOUND_START(m_player_actor, SS_TRIG_BRAKE);
    }
    else
    {
        SOUND_STOP(m_player_actor, SS_TRIG_BRAKE);
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_INTER_AXLE_DIFF))
    {
        m_player_actor->ToggleAxleDiffMode();
        m_player_actor->DisplayAxleDiffMode();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF))
    {
        m_player_actor->ToggleWheelDiffMode();
        m_player_actor->DisplayWheelDiffMode();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_TCASE_4WD_MODE))
    {
        m_player_actor->ToggleTransferCaseMode();
        m_player_actor->DisplayTransferCaseMode();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO))
    {
        m_player_actor->ToggleTransferCaseGearRatio();
        m_player_actor->DisplayTransferCaseMode();
    }

    if (m_player_actor->ar_is_police)
    {
        if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_HORN))
        {
            SOUND_TOGGLE(m_player_actor, SS_TRIG_HORN); // Police siren
        }
    }
    else
    {
        if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_HORN))
        {
            SOUND_START(m_player_actor, SS_TRIG_HORN);
        }
        else
        {
            SOUND_STOP(m_player_actor, SS_TRIG_HORN);
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_PARKING_BRAKE) &&
            !App::GetInputEngine()->getEventBoolValue(EV_TRUCK_TRAILER_PARKING_BRAKE))
    {
        m_player_actor->ToggleParkingBrake();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_ANTILOCK_BRAKE))
    {
        m_player_actor->ToggleAntiLockBrake();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TRACTION_CONTROL))
    {
        m_player_actor->ToggleTractionControl();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_CRUISE_CONTROL))
    {
        m_player_actor->ToggleCruiseControl();
    }

    if (m_player_actor->GetTyrePressure().IsEnabled())
    {
        m_player_actor->GetTyrePressure().UpdateInputEvents(dt);
    }
}

