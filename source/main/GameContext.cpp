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
#include "GUI_TopMenubar.h"
#include "GUI_SurveyMap.h"
#include "InputEngine.h"
#include "OverlayWrapper.h"
#include "Replay.h"
#include "ScrewProp.h"
#include "ScriptEngine.h"
#include "SkyManager.h"
#include "SkyXManager.h"
#include "SoundScriptManager.h"
#include "Terrain.h"
#include "Utils.h"
#include "VehicleAI.h"
#include "GUI_VehicleButtons.h"

using namespace RoR;

// --------------------------------
// Message queue

void GameContext::PushMessage(Message m)
{
    std::lock_guard<std::mutex> lock(m_msg_mutex);
    m_msg_queue.push(m);
    m_msg_chain_end = &m_msg_queue.back();
}

void GameContext::ChainMessage(Message m)
{
    std::lock_guard<std::mutex> lock(m_msg_mutex);
    if (m_msg_chain_end)
    {
        m_msg_chain_end->chain.push_back(m);
        m_msg_chain_end = &m_msg_chain_end->chain.back();
    }
    else
    {
        this->PushMessage(m);
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
    ROR_ASSERT(m_msg_queue.size() > 0);
    if (m_msg_chain_end == &m_msg_queue.front())
    {
        m_msg_chain_end = nullptr;
    }
    Message m = m_msg_queue.front();
    m_msg_queue.pop();
    return m;
}

// --------------------------------
// Terrain

bool GameContext::LoadTerrain(std::string const& filename_part)
{
    m_last_spawned_actor = nullptr;

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

    // Load the terrain
    Terrn2Def terrn2;
    std::string const& filename = terrn_entry->fname;
    try
    {
        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(filename);
        LOG(" ===== LOADING TERRAIN " + filename);
        Terrn2Parser parser;
        if (! parser.LoadTerrn2(terrn2, stream))
        {
            return false; // Errors already logged to console
        }
    }
    catch (Ogre::Exception& e)
    {
        App::GetGuiManager()->ShowMessageBox(_L("Terrain loading error"), e.getFullDescription().c_str());
        return false;
    }

    // CAUTION - the global instance must be set during init! Needed by:
    // * GameScript::spawnObject()
    // * ProceduralManager
    // * Landusemap
    // * SurveyMapTextureCreator
    // * Collisions (debug visualization)
    m_terrain = TerrainPtr::Bind(new RoR::Terrain(terrn_entry, terrn2));
    if (!m_terrain->initialize())
    {
        m_terrain = TerrainPtr(); // release local reference - object will be deleted when all references are released.
        return false; // Message box already displayed
    }

    // Initialize envmap textures by rendering center of map
    Ogre::Vector3 center = m_terrain->getMaxTerrainSize() / 2;
    center.y = m_terrain->GetHeightAt(center.x, center.z) + 1.0f;
    App::GetGfxScene()->GetEnvMap().UpdateEnvMap(center, /*gfx_actor:*/nullptr, /*full:*/true);

    // Scan groundmodels
    App::GetGuiManager()->FrictionSettings.AnalyzeTerrain();

    return true;
}

void GameContext::UnloadTerrain()
{
    if (m_terrain != nullptr)
    {
        // dispose(), do not `delete` - script may still hold reference to the object.
        m_terrain->dispose();
        // release local reference - object will be deleted when all references are released.
        m_terrain = TerrainPtr();
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
                float h = m_player_actor->getMaxHeight(true);
                rq.asr_rotation = Ogre::Quaternion(Ogre::Degree(270) - Ogre::Radian(m_player_actor->getRotation()), Ogre::Vector3::UNIT_Y);
                rq.asr_position = m_player_actor->getRotationCenter();
                rq.asr_position.y = m_terrain->GetCollisions()->getSurfaceHeightBelow(rq.asr_position.x, rq.asr_position.z, h);
                rq.asr_position.y += m_player_actor->getHeightAboveGroundBelow(h, true); // retain height above ground
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

    RigDef::DocumentPtr def = m_actor_manager.FetchActorDef(
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
        if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
        {
            RoRnet::UserInfo info = App::GetNetwork()->GetLocalUserData();
            rq.asr_net_username = tryConvertUTF(info.username);
            rq.asr_net_color    = info.colournum;
        }
    }
#endif //SOCKETW

    Actor* fresh_actor = m_actor_manager.CreateNewActor(rq, def);

    // lock slide nodes after spawning the actor?
    if (def->slide_nodes_connect_instantly)
    {
        fresh_actor->toggleSlideNodeLock();
    }

    if (rq.asr_origin == ActorSpawnRequest::Origin::USER)
    {
        m_last_spawned_actor = fresh_actor;
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
            App::diag_preset_veh_enter->getBool())
        {
            this->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, (void*)fresh_actor));
        }
        if (fresh_actor->ar_driveable != NOT_DRIVEABLE &&
            fresh_actor->ar_num_nodes > 0 &&
            App::cli_preset_veh_enter->getBool())
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
    else if (rq.asr_origin == ActorSpawnRequest::Origin::AI)
    {
        fresh_actor->ar_driveable = AI;
        fresh_actor->ar_state = ActorState::LOCAL_SIMULATED;

        if (fresh_actor->ar_engine)
        {
            fresh_actor->ar_engine->SetAutoMode(RoR::SimGearboxMode::AUTO);
            fresh_actor->ar_engine->autoShiftSet(EngineSim::DRIVE);
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
    else if (rq.amr_type == ActorModifyRequest::Type::RESET_ON_SPOT)
    {
        rq.amr_actor->SyncReset(/*reset_position:*/false);
    }
    else if (rq.amr_type == ActorModifyRequest::Type::RESET_ON_INIT_POS)
    {
        rq.amr_actor->SyncReset(/*reset_position:*/true);
    }
    else if (rq.amr_type == ActorModifyRequest::Type::RESTORE_SAVED)
    {
        m_actor_manager.RestoreSavedState(rq.amr_actor, *rq.amr_saved_state.get());
    }
    else if (rq.amr_type == ActorModifyRequest::Type::WAKE_UP &&
        rq.amr_actor->ar_state == ActorState::LOCAL_SLEEPING)
    {
        rq.amr_actor->ar_state = ActorState::LOCAL_SIMULATED;
        rq.amr_actor->ar_sleep_counter = 0.0f;
    }
    else if (rq.amr_type == ActorModifyRequest::Type::RELOAD)
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
        srq->asr_position   = Ogre::Vector3(rq.amr_actor->getPosition().x, rq.amr_actor->getMinHeight(), rq.amr_actor->getPosition().z);
        srq->asr_rotation   = Ogre::Quaternion(Ogre::Degree(270) - Ogre::Radian(rq.amr_actor->getRotation()), Ogre::Vector3::UNIT_Y);
        srq->asr_config     = rq.amr_actor->getSectionConfig();
        srq->asr_skin_entry = rq.amr_actor->getUsedSkin();
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
        Ogre::Vector3 center = m_player_actor->getRotationCenter();
        this->ChangePlayerActor(nullptr); // Get out of the vehicle
        this->GetPlayerCharacter()->setPosition(center);
        // Update scene SimBuffer immediatelly to prevent having dangling pointer.
        App::GetGfxScene()->GetSimDataBuffer().simbuf_player_actor = nullptr;
    }

    if (actor == m_prev_player_actor)
    {
        m_prev_player_actor = nullptr;
    }

    if (actor == m_last_spawned_actor)
    {
        m_last_spawned_actor = nullptr;
    }

    // Find linked actors and un-tie if tied
    auto linked_actors = actor->getAllLinkedActors();
    for (auto actorx : m_actor_manager.GetLocalActors())
    {
        if (actorx->isTied() && std::find(linked_actors.begin(), linked_actors.end(), actorx) != linked_actors.end())
        {
            actorx->tieToggle();
        }

        if (actorx->isLocked() && std::find(linked_actors.begin(), linked_actors.end(), actorx) != linked_actors.end())
        {
            actorx->hookToggle();
        }
    }

    App::GetGfxScene()->RemoveGfxActor(actor->GetGfxActor());

#ifdef USE_SOCKETW
    if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
    {
        m_character_factory.UndoRemoteActorCoupling(actor);
    }
#endif //SOCKETW

    TRIGGER_EVENT(SE_GENERIC_DELETED_TRUCK, actor->ar_instance_id);
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
            if (prev_player_actor->GetGfxActor()->GetVideoCamState() == VideoCamState::VCSTATE_ENABLED_ONLINE)
            {
                prev_player_actor->GetGfxActor()->SetVideoCamState(VideoCamState::VCSTATE_ENABLED_OFFLINE);
            }

            prev_player_actor->prepareInside(false);

            // get player out of the vehicle
            float h = prev_player_actor->getMinCameraRadius();
            float rotation = prev_player_actor->getRotation() - Ogre::Math::HALF_PI;
            Ogre::Vector3 position = prev_player_actor->getPosition();
            if (prev_player_actor->ar_cinecam_node[0] != NODENUM_INVALID)
            {
                // actor has a cinecam (find optimal exit position)
                Ogre::Vector3 l = position - 2.0f * prev_player_actor->GetCameraRoll();
                Ogre::Vector3 r = position + 2.0f * prev_player_actor->GetCameraRoll();
                float l_h = m_terrain->GetCollisions()->getSurfaceHeightBelow(l.x, l.z, l.y + h);
                float r_h = m_terrain->GetCollisions()->getSurfaceHeightBelow(r.x, r.z, r.y + h);
                position  = std::abs(r.y - r_h) * 1.2f < std::abs(l.y - l_h) ? r : l;
            }
            position.y = m_terrain->GetCollisions()->getSurfaceHeightBelow(position.x, position.z, position.y + h);

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

        if (m_player_actor->GetGfxActor()->GetVideoCamState() == VideoCamState::VCSTATE_ENABLED_OFFLINE)
        {
            m_player_actor->GetGfxActor()->SetVideoCamState(VideoCamState::VCSTATE_ENABLED_ONLINE);
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

        App::GetGuiManager()->FlexbodyDebug.AnalyzeFlexbodies();

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
    return m_actor_manager.FindActorInsideBox(m_terrain->GetCollisions(),
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
    if (!(App::mp_state->getEnum<MpState>() == MpState::CONNECTED))
    {
        collision_box_t* spawnbox = m_terrain->GetCollisions()->getBox(instance, box);
        for (auto actor : this->GetActorManager()->GetActors())
        {
            for (int i = 0; i < actor->ar_num_nodes; i++)
            {
                if (m_terrain->GetCollisions()->isInside(actor->ar_nodes[i].AbsPosition, spawnbox))
                {
                    App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Please clear the place first"), "error.png");
                    return;
                }
            }
        }
    }

    m_current_selection.asr_position = m_terrain->GetCollisions()->getPosition(instance, box);
    m_current_selection.asr_rotation = m_terrain->GetCollisions()->getDirection(instance, box);
    m_current_selection.asr_spawnbox = m_terrain->GetCollisions()->getBox(instance, box);

    RoR::Message m(MSG_GUI_OPEN_SELECTOR_REQUESTED);
    m.payload = reinterpret_cast<void*>(new LoaderType(LoaderType(type)));
    App::GetGameContext()->PushMessage(m);
}

void GameContext::OnLoaderGuiCancel()
{
    m_current_selection = ActorSpawnRequest(); // Reset

    if (App::GetGuiManager()->TopMenubar.ai_select)
    {
        App::GetGuiManager()->TopMenubar.ai_select = false;
        App::GetGuiManager()->TopMenubar.ai_menu = true;
    }
    if (App::GetGuiManager()->TopMenubar.ai_select2)
    {
        App::GetGuiManager()->TopMenubar.ai_select2 = false;
        App::GetGuiManager()->TopMenubar.ai_menu = true;
    }
}

void GameContext::OnLoaderGuiApply(LoaderType type, CacheEntry* entry, std::string sectionconfig)
{
    bool spawn_now = false;
    switch (type)
    {
    case LT_Skin:
        if (entry != &m_dummy_cache_selection)
        {
            m_current_selection.asr_skin_entry = entry;
            if (App::GetGuiManager()->TopMenubar.ai_select)
            {
                App::GetGuiManager()->TopMenubar.ai_skin = entry->dname;
            }
            if (App::GetGuiManager()->TopMenubar.ai_select2)
            {
                App::GetGuiManager()->TopMenubar.ai_skin2 = entry->dname;
            }
        }
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
        if (App::GetGuiManager()->TopMenubar.ai_select)
        {
            App::GetGuiManager()->TopMenubar.ai_sectionconfig = sectionconfig;
        }
        if (App::GetGuiManager()->TopMenubar.ai_select2)
        {
            App::GetGuiManager()->TopMenubar.ai_sectionconfig2 = sectionconfig;
        }
        m_current_selection.asr_origin = ActorSpawnRequest::Origin::USER;
        // Look for extra skins
        if (!entry->guid.empty())
        {
            CacheQuery skin_query;
            skin_query.cqy_filter_guid = entry->guid;
            skin_query.cqy_filter_type = LT_Skin;
            size_t num_skins = App::GetCacheSystem()->Query(skin_query);
            // Determine default skin
            CacheEntry* default_skin_entry = nullptr;
            if (entry->default_skin != "")
            {
                for (CacheQueryResult& res : skin_query.cqy_results)
                {
                    if (res.cqr_entry->dname == entry->default_skin)
                        default_skin_entry = res.cqr_entry;
                }
                if (!default_skin_entry)
                {
                    App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
                        fmt::format(_L("Default skin '{}' for actor '{}' not found!"), entry->default_skin, entry->dname));
                }
                if (default_skin_entry && num_skins == 1)
                {
                    m_current_selection.asr_skin_entry = default_skin_entry;
                }
            }
            else
            {
                default_skin_entry = &m_dummy_cache_selection;
                default_skin_entry->dname = "Default skin";
                default_skin_entry->description = "Original, unmodified skin";
            }

            if (!m_current_selection.asr_skin_entry && num_skins > 0)
            {
                App::GetGuiManager()->MainSelector.Show(LT_Skin, entry->guid, default_skin_entry); // Intentionally not using MSG_
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
        if (App::GetGuiManager()->TopMenubar.ai_select)
        {
            App::GetGuiManager()->TopMenubar.ai_fname = m_current_selection.asr_cache_entry->fname;
            App::GetGuiManager()->TopMenubar.ai_dname = m_current_selection.asr_cache_entry->dname;
            App::GetGuiManager()->TopMenubar.ai_select = false;
            App::GetGuiManager()->TopMenubar.ai_menu = true;
        }
        else if (App::GetGuiManager()->TopMenubar.ai_select2)
        {
            App::GetGuiManager()->TopMenubar.ai_fname2 = m_current_selection.asr_cache_entry->fname;
            App::GetGuiManager()->TopMenubar.ai_dname2 = m_current_selection.asr_cache_entry->dname;
            App::GetGuiManager()->TopMenubar.ai_select2 = false;
            App::GetGuiManager()->TopMenubar.ai_menu = true;
        }
        else
        {
            ActorSpawnRequest* rq = new ActorSpawnRequest;
            *rq = m_current_selection;
            this->PushMessage(Message(MSG_SIM_SPAWN_ACTOR_REQUESTED, (void*)rq));
        }

        m_current_selection = ActorSpawnRequest(); // Reset
    }
}

// --------------------------------
// Characters

void GameContext::CreatePlayerCharacter()
{
    m_character_factory.CreateLocalCharacter();

    // Adjust character position
    Ogre::Vector3 spawn_pos = m_terrain->getSpawnPos();
    float spawn_rot = 0.0f;

    // Classic behavior, retained for compatibility.
    // Required for maps like N-Labs or F1 Track.
    if (!m_terrain->HasPredefinedActors())
    {
        spawn_rot = 180.0f;
    }

    // Preset position - commandline has precedence
    if (App::cli_preset_spawn_pos->getStr() != "")
    {
        spawn_pos = Ogre::StringConverter::parseVector3(App::cli_preset_spawn_pos->getStr(), spawn_pos);
        App::cli_preset_spawn_pos->setStr("");
    }
    else if (App::diag_preset_spawn_pos->getStr() != "")
    {
        spawn_pos = Ogre::StringConverter::parseVector3(App::diag_preset_spawn_pos->getStr(), spawn_pos);
        App::diag_preset_spawn_pos->setStr("");
    }

    // Preset rotation - commandline has precedence
    if (App::cli_preset_spawn_rot->getStr() != "")
    {
        spawn_rot = Ogre::StringConverter::parseReal(App::cli_preset_spawn_rot->getStr(), spawn_rot);
        App::cli_preset_spawn_rot->setStr("");
    }
    else if (App::diag_preset_spawn_rot->getStr() != "")
    {
        spawn_rot = Ogre::StringConverter::parseReal(App::diag_preset_spawn_rot->getStr(), spawn_rot);
        App::diag_preset_spawn_rot->setStr("");
    }

    spawn_pos.y = m_terrain->GetCollisions()->getSurfaceHeightBelow(spawn_pos.x, spawn_pos.z, spawn_pos.y + 1.8f);

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
    float y = m_terrain->GetCollisions()->getSurfaceHeight(x, z);
    if (!this->GetPlayerActor())
    {
        this->GetPlayerCharacter()->setPosition(Ogre::Vector3(x, y, z));
        return;
    }

    TRIGGER_EVENT(SE_TRUCK_TELEPORT, this->GetPlayerActor()->ar_instance_id);

    Ogre::Vector3 translation = Ogre::Vector3(x, y, z) - this->GetPlayerActor()->ar_nodes[0].AbsPosition;

    auto actors = this->GetPlayerActor()->getAllLinkedActors();
    actors.push_back(this->GetPlayerActor());

    float src_agl = std::numeric_limits<float>::max(); 
    float dst_agl = std::numeric_limits<float>::max(); 
    for (auto actor : actors)
    {
        for (int i = 0; i < actor->ar_num_nodes; i++)
        {
            Ogre::Vector3 pos = actor->ar_nodes[i].AbsPosition;
            src_agl = std::min(pos.y - m_terrain->GetCollisions()->getSurfaceHeight(pos.x, pos.z), src_agl);
            pos += translation;
            dst_agl = std::min(pos.y - m_terrain->GetCollisions()->getSurfaceHeight(pos.x, pos.z), dst_agl);
        }
    }

    translation += Ogre::Vector3::UNIT_Y * (std::max(0.0f, src_agl) - dst_agl);

    for (auto actor : actors)
    {
        actor->resetPosition(actor->ar_nodes[0].AbsPosition + translation, false);
    }
}

void GameContext::UpdateGlobalInputEvents()
{
    // Generic escape key event
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUIT_GAME))
    {
        if (App::app_state->getEnum<AppState>() == AppState::MAIN_MENU)
        {
            if (App::GetGuiManager()->GameAbout.IsVisible())
            {
                App::GetGuiManager()->GameAbout.SetVisible(false);
            }
            else if (App::GetGuiManager()->MainSelector.IsVisible())
            {
                App::GetGuiManager()->MainSelector.Close();
            }
            else if (App::GetGuiManager()->GameSettings.IsVisible())
            {
                App::GetGuiManager()->GameSettings.SetVisible(false);
            }
            else if (App::GetGuiManager()->GameControls.IsVisible())
            {
                App::GetGuiManager()->GameControls.SetVisible(false);
            }
            else if (App::GetGuiManager()->MultiplayerSelector.IsVisible())
            {
                App::GetGuiManager()->MultiplayerSelector.SetVisible(false);
            }
            else if (App::GetGuiManager()->RepositorySelector.IsVisible())
            {
                App::GetGuiManager()->RepositorySelector.SetVisible(false);
            }
            else
            {
                this->PushMessage(Message(MSG_APP_SHUTDOWN_REQUESTED));
            }
        }
        else if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
        {
            if (App::GetGuiManager()->MainSelector.IsVisible())
            {
                App::GetGuiManager()->MainSelector.Close();
            }
            else if (App::GetGuiManager()->GameControls.IsVisible())
            {
                App::GetGuiManager()->GameControls.SetVisible(false);
            }
            else if (App::sim_state->getEnum<SimState>() == SimState::RUNNING)
            {
                this->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
                if (App::mp_state->getEnum<MpState>() != MpState::CONNECTED)
                {
                    this->PushMessage(Message(MSG_SIM_PAUSE_REQUESTED));
                }
            }
            else if (App::sim_state->getEnum<SimState>() == SimState::PAUSED)
            {
                this->PushMessage(Message(MSG_SIM_UNPAUSE_REQUESTED));
                this->PushMessage(Message(MSG_GUI_CLOSE_MENU_REQUESTED));
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

    // render mode
    switch (App::gfx_polygon_mode->getEnum<Ogre::PolygonMode>())
    {
    case 1: App::GetCameraManager()->GetCamera()->setPolygonMode(Ogre::PM_SOLID);       break;
    case 2: App::GetCameraManager()->GetCamera()->setPolygonMode(Ogre::PM_WIREFRAME);   break;
    case 3: App::GetCameraManager()->GetCamera()->setPolygonMode(Ogre::PM_POINTS);      break;
    }

    // Write player position to log
    if (App::app_state->getEnum<AppState>() == AppState::SIMULATION &&
        App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_OUTPUT_POSITION))
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
        App::sim_soft_reset_mode->setVal(!App::sim_soft_reset_mode->getBool());
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
            (App::sim_soft_reset_mode->getBool()) ? _L("Enabled soft reset mode") : _L("Enabled hard reset mode"));
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
                if (actor->ar_cinecam_node[0] == NODENUM_INVALID)
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
                this->GetPlayerActor()->ar_state == ActorState::NETWORKED_OK || this->GetPlayerActor()->ar_state == ActorState::NETWORKED_HIDDEN ||
                this->GetPlayerActor()->ar_driveable == AI)
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
    if (!App::GetGameContext()->GetPlayerActor() && App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TERRAIN_EDITOR))
    {
        App::GetGameContext()->PushMessage(MSG_EDI_ENTER_TERRN_EDITOR_REQUESTED);
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

    // AI waypoint recording
    if (App::GetGuiManager()->TopMenubar.ai_rec)
    {
        if (m_timer.getMilliseconds() > 1000) // Don't spam it, record every 1 sec
        {
            if (App::GetGameContext()->GetPlayerActor()) // We are in vehicle
            {
                if (App::GetGameContext()->GetPlayerActor()->getPosition().distance(prev_pos) >= 5) // Skip very close positions
                {
                    App::GetGuiManager()->SurveyMap.ai_waypoints.push_back(App::GetGameContext()->GetPlayerActor()->getPosition());
                }
                prev_pos = App::GetGameContext()->GetPlayerActor()->getPosition();
            }
            else // We are in feet
            {
                if (App::GetGameContext()->GetPlayerCharacter()->getPosition() != prev_pos) // Skip same positions
                {
                    App::GetGuiManager()->SurveyMap.ai_waypoints.push_back(App::GetGameContext()->GetPlayerCharacter()->getPosition());
                }
                prev_pos = App::GetGameContext()->GetPlayerCharacter()->getPosition();
            }
            m_timer.reset();
        }
    }
    else
    {
        prev_pos = Ogre::Vector3::ZERO;
    }
}

void GameContext::UpdateSkyInputEvents(float dt)
{
#ifdef USE_CAELUM
    if (App::gfx_sky_mode->getEnum<GfxSkyMode>() == GfxSkyMode::CAELUM &&
        m_terrain->getSkyManager())
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
        else if (App::gfx_sky_time_cycle->getBool())
        {
            time_factor = App::gfx_sky_time_speed->getInt();
        }

        if (m_terrain->getSkyManager()->GetSkyTimeFactor() != time_factor)
        {
            m_terrain->getSkyManager()->SetSkyTimeFactor(time_factor);
            Str<200> msg; msg << _L("Time set to ") << m_terrain->getSkyManager()->GetPrettyTime();
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, msg.ToCStr());
        }
    }

#endif // USE_CAELUM
    if (App::gfx_sky_mode->getEnum<GfxSkyMode>() == GfxSkyMode::SKYX &&
        m_terrain->getSkyXManager())
    {
        if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_INCREASE_TIME))
        {
            m_terrain->getSkyXManager()->GetSkyX()->setTimeMultiplier(1.0f);
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_INCREASE_TIME_FAST))
        {
            m_terrain->getSkyXManager()->GetSkyX()->setTimeMultiplier(2.0f);
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_DECREASE_TIME))
        {
            m_terrain->getSkyXManager()->GetSkyX()->setTimeMultiplier(-1.0f);
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_DECREASE_TIME_FAST))
        {
            m_terrain->getSkyXManager()->GetSkyX()->setTimeMultiplier(-2.0f);
        }
        else
        {
            m_terrain->getSkyXManager()->GetSkyX()->setTimeMultiplier(0.01f);
        }
    }
}

void GameContext::UpdateCommonInputEvents(float dt)
{
    ROR_ASSERT(m_player_actor);

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

    // Front lights
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_LOW_BEAMS))
    {
        m_player_actor->setHeadlightsVisible(!m_player_actor->getHeadlightsVisible());
        // sync sidelights to lowbeams
        m_player_actor->setSideLightsVisible(m_player_actor->getHeadlightsVisible());
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_CYCLE_TRUCK_LIGHTS))
    {
        // Smart cycling:
        //  1) all lights off
        //  2) sidelights on but only if any installed, otherwise skip to 3).
        //  3) sidelights and lowbeams on.
        //  4) sidelights, lowbeams and highbeams on, but only if highbeams are installed, otherwise cycle to 1).
        if (m_player_actor->countFlaresByType(FlareType::SIDELIGHT) > 0 && !m_player_actor->getSideLightsVisible())
        {
            m_player_actor->setSideLightsVisible(true);
        }
        else if (!m_player_actor->getHeadlightsVisible())
        {
            m_player_actor->setSideLightsVisible(true);
            m_player_actor->setHeadlightsVisible(true);
        }
        else if (m_player_actor->countFlaresByType(FlareType::HIGH_BEAM) > 0 && !m_player_actor->getHighBeamsVisible())
        {
            m_player_actor->setSideLightsVisible(true);
            m_player_actor->setHeadlightsVisible(true);
            m_player_actor->setHighBeamsVisible(true);
        }
        else
        {
            m_player_actor->setSideLightsVisible(false);
            m_player_actor->setHeadlightsVisible(false);
            m_player_actor->setHighBeamsVisible(false);
        }
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_HIGH_BEAMS))
    {
        m_player_actor->setHighBeamsVisible(!m_player_actor->getHighBeamsVisible());
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_FOG_LIGHTS))
    {
        m_player_actor->setFogLightsVisible(!m_player_actor->getFogLightsVisible());
    }

    // Beacons
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_BEACONS))
    {
        m_player_actor->beaconsToggle();
    }

    // blinkers
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_LEFT))
        m_player_actor->toggleBlinkType(BLINK_LEFT);

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_RIGHT))
        m_player_actor->toggleBlinkType(BLINK_RIGHT);

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_WARN))
        m_player_actor->toggleBlinkType(BLINK_WARN);

    // custom lights
    for (int i = 0; i < MAX_CLIGHTS; i++)
    {
        if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LIGHTTOGGLE01 + i))
            m_player_actor->setCustomLightVisible(i, !m_player_actor->getCustomLightVisible(i));
    }

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
        m_player_actor->hookToggle(-1, HOOK_TOGGLE, -1);
        m_player_actor->toggleSlideNodeLock();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_AUTOLOCK))
    {
        m_player_actor->hookToggle(-2, HOOK_UNLOCK, -1); //unlock all autolocks
    }

    //strap
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_SECURE_LOAD))
    {
        m_player_actor->ar_toggle_ties = true;
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_CUSTOM_PARTICLES))
    {
        m_player_actor->toggleCustomParticles();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_DEBUG_VIEW))
    {
        m_player_actor->GetGfxActor()->ToggleDebugView();
        for (auto actor : m_player_actor->getAllLinkedActors())
        {
            actor->GetGfxActor()->SetDebugView(m_player_actor->GetGfxActor()->GetDebugView());
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_CYCLE_DEBUG_VIEWS))
    {
        m_player_actor->GetGfxActor()->CycleDebugViews();
        for (auto actor : m_player_actor->getAllLinkedActors())
        {
            actor->GetGfxActor()->SetDebugView(m_player_actor->GetGfxActor()->GetDebugView());
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESCUE_TRUCK, 0.5f) &&
        App::mp_state->getEnum<MpState>() != MpState::CONNECTED &&
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
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("No rescue truck found!"), "error.png");
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
            m_player_actor->parkingbrakeToggle();
    }

    // videocam
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_VIDEOCAMERA, 0.5f))
    {
        if (m_player_actor->GetGfxActor()->GetVideoCamState() == VideoCamState::VCSTATE_DISABLED)
        {
            m_player_actor->GetGfxActor()->SetVideoCamState(VideoCamState::VCSTATE_ENABLED_ONLINE);
        }
        else
        {
            m_player_actor->GetGfxActor()->SetVideoCamState(VideoCamState::VCSTATE_DISABLED);
        }
    }

    // enter/exit truck - Without a delay: the vehicle must brake like braking normally
    if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_ENTER_OR_EXIT_TRUCK))
    {
        if (m_player_actor->ar_driveable != AI)
        {
            m_player_actor->ar_brake = 0.66f;
        }
    }

    // toggle physics
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_PHYSICS))
    {
        for (auto actor : App::GetGameContext()->GetPlayerActor()->getAllLinkedActors())
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

        for (auto id: App::GetGuiManager()->VehicleButtons.GetCommandEventID())
        {
            if (id == eventID)
            {
                m_player_actor->ar_command_key[i].playerInputValue = 1.f;
            }
        }
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_FORWARDCOMMANDS))
    {
        m_player_actor->ar_forward_commands = !m_player_actor->ar_forward_commands;
        if (m_player_actor->ar_forward_commands)
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("forwardcommands enabled"), "information.png");
        }
        else
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("forwardcommands disabled"), "information.png");
        }
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_IMPORTCOMMANDS))
    {
        m_player_actor->ar_import_commands = !m_player_actor->ar_import_commands;
        if (m_player_actor->ar_import_commands)
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("importcommands enabled"), "information.png");
        }
        else
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("importcommands disabled"), "information.png");
        }
    }

    if (m_player_actor->getReplay())
    {
        m_player_actor->getReplay()->UpdateInputEvents();
    }
}

// Internal heper for UpdateAirplaneInputEvents()
void smoothValue(float& ref, float value, float rate)
{
    if (value < -1)
        value = -1;
    if (value > 1)
        value = 1;
    // smooth!
    if (ref > value)
    {
        ref -= rate;
        if (ref < value)
            ref = value;
    }
    else if (ref < value)
        ref += rate;
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
    smoothValue(m_player_actor->ar_aileron, sum_steer, dt * commandrate);
    m_player_actor->ar_hydro_dir_command = m_player_actor->ar_aileron;
    m_player_actor->ar_hydro_speed_coupling = !(App::GetInputEngine()->isEventAnalog(EV_AIRPLANE_STEER_LEFT) && App::GetInputEngine()->isEventAnalog(EV_AIRPLANE_STEER_RIGHT));

    // pitch
    float tmp_pitch_up = App::GetInputEngine()->getEventValue(EV_AIRPLANE_ELEVATOR_UP);
    float tmp_pitch_down = App::GetInputEngine()->getEventValue(EV_AIRPLANE_ELEVATOR_DOWN);
    float sum_pitch = tmp_pitch_down - tmp_pitch_up;
    smoothValue(m_player_actor->ar_elevator, sum_pitch, dt * commandrate);

    // rudder
    float tmp_rudder_left = App::GetInputEngine()->getEventValue(EV_AIRPLANE_RUDDER_LEFT);
    float tmp_rudder_right = App::GetInputEngine()->getEventValue(EV_AIRPLANE_RUDDER_RIGHT);
    float sum_rudder = tmp_rudder_left - tmp_rudder_right;
    smoothValue(m_player_actor->ar_rudder, sum_rudder, dt * commandrate);

    // brakes
    if (!m_player_actor->ar_parking_brake)
    {
        m_player_actor->ar_brake = App::GetInputEngine()->getEventValue(EV_AIRPLANE_BRAKE) * 0.66f;
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_PARKING_BRAKE))
    {
        m_player_actor->parkingbrakeToggle();
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
    float tmp_left_digital  = App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_LEFT , false, InputSourceType::IST_DIGITAL);
    float tmp_right_digital = App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_RIGHT, false, InputSourceType::IST_DIGITAL);
    float tmp_left_analog   = App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_LEFT , false, InputSourceType::IST_ANALOG);
    float tmp_right_analog  = App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_RIGHT, false, InputSourceType::IST_ANALOG);

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
        m_player_actor->toggleAxleDiffMode();
        m_player_actor->displayAxleDiffMode();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF))
    {
        m_player_actor->toggleWheelDiffMode();
        m_player_actor->displayWheelDiffMode();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_TCASE_4WD_MODE))
    {
        m_player_actor->toggleTransferCaseMode();
        m_player_actor->displayTransferCaseMode();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO))
    {
        m_player_actor->toggleTransferCaseGearRatio();
        m_player_actor->displayTransferCaseMode();
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
        if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_HORN) || App::GetGuiManager()->VehicleButtons.GetHornButtonState())
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
        m_player_actor->parkingbrakeToggle();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_ANTILOCK_BRAKE))
    {
        m_player_actor->antilockbrakeToggle();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TRACTION_CONTROL))
    {
        m_player_actor->tractioncontrolToggle();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_CRUISE_CONTROL))
    {
        m_player_actor->cruisecontrolToggle();
    }

    if (m_player_actor->getTyrePressure().IsEnabled())
    {
        m_player_actor->getTyrePressure().UpdateInputEvents(dt);
    }

    m_player_actor->UpdatePropAnimInputEvents();
    for (Actor* linked_actor : m_player_actor->getAllLinkedActors())
    {
        linked_actor->UpdatePropAnimInputEvents();
    }
}

