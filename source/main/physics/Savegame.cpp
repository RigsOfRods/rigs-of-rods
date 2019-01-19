/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal

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

#include "RoRPrerequisites.h"

#include "AeroEngine.h"
#include "Application.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "Buoyance.h"
#include "CacheSystem.h"
#include "GUIManager.h"
#include "Language.h"
#include "PlatformUtils.h"
#include "RoRFrameListener.h"
#include "Skidmark.h"
#include "TerrainManager.h"
#include "ScrewProp.h"
#include "SkyManager.h"

#ifdef Bool // Conflicts with RapidJSON, see https://github.com/Tencent/rapidjson/issues/628
#   undef Bool
#endif
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <fstream>

#define SAVEGAME_FILE_FORMAT 1

using namespace Ogre;
using namespace RoR;

Ogre::String ActorManager::GetQuicksaveFilename(Ogre::String terrain_name)
{
    if (terrain_name.empty())
    {
        terrain_name = App::sim_terrain_name.GetActive();
    }
    return String("quicksave_") + StringUtil::replaceAll(terrain_name, ".terrn2", "") + String(".sav");
}

Ogre::String ActorManager::ExtractSceneName(Ogre::String filename)
{
    std::ifstream ifs(PathCombine(App::sys_savegames_dir.GetActive(), filename));
    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document j_doc;
    j_doc.ParseStream<rapidjson::kParseNanAndInfFlag>(isw);

    if (!j_doc.IsObject() || !j_doc.HasMember("scene_name") || !j_doc["scene_name"].IsString())
        return "";

    return j_doc["scene_name"].GetString();
}

bool ActorManager::LoadScene(Ogre::String filename)
{
    if (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
    {
        if (filename != "autosave.sav")
        {
            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Savegames are not available in multiplayer"));
        }
        return false;
    }

    // Read from disk
    String path = PathCombine(App::sys_savegames_dir.GetActive(), filename);
    RoR::LogFormat("[RoR|Savegame] Reading savegame from file '%s' ...", path.c_str());

    std::ifstream ifs(path);
    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document j_doc;
    j_doc.ParseStream<rapidjson::kParseNanAndInfFlag>(isw);
    if (!j_doc.IsObject() || !j_doc.HasMember("format_version") || !j_doc["format_version"].IsNumber())
    {
        RoR::Log("[RoR|Savegame] Invalid or missing savegame file.");
        RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Error while loading scene: File invalid or missing"));
        App::sim_load_savegame.SetActive(false);
        return false;
    }
    if (j_doc["format_version"].GetInt() != SAVEGAME_FILE_FORMAT)
    {
        RoR::Log("[RoR|Savegame] Savegame file format mismatch.");
        RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Error while loading scene: File format mismatch"));
        App::sim_load_savegame.SetActive(false);
        return false;
    }

    // Terrain
    String terrain_name = j_doc["terrain_name"].GetString();

    if (terrain_name != App::sim_terrain_name.GetActive() || App::sim_state.GetActive() != SimState::RUNNING)
    {
        RoR::LogFormat("[RoR|Savegame] Loading terrain '%s' ...", terrain_name.c_str());
        App::sim_terrain_name.SetPending(terrain_name.c_str());
        if (App::app_state.GetActive() == AppState::SIMULATION)
        {
            App::app_state.SetPending(AppState::MAIN_MENU);
            App::sim_savegame.SetActive(filename.c_str());
            App::sim_load_savegame.SetActive(true);
        }
        else
        {
            App::GetSimController()->SetPhysicsPausedInternal(j_doc["physics_paused"].GetBool());
        }
        return true;
    }

#ifdef USE_CAELUM
    if (App::gfx_sky_mode.GetActive() == GfxSkyMode::CAELUM)
    {
        if (j_doc.HasMember("daytime"))
        {
            App::GetSimTerrain()->getSkyManager()->SetTime(j_doc["daytime"].GetDouble());
        }
    }
#endif // USE_CAELUM

    m_forced_awake = j_doc["forced_awake"].GetBool();

    // Character
    auto player_data = j_doc["player_position"].GetArray();
    gEnv->player->setPosition(Vector3(player_data[0].GetFloat(), player_data[1].GetFloat(), player_data[2].GetFloat()));
    gEnv->player->setRotation(Radian(j_doc["player_rotation"].GetFloat()));

    // Actors
    App::GetSimController()->SetPendingPlayerActor(nullptr);
    App::GetSimController()->SetPrevPlayerActorInternal(nullptr);

    std::vector<Actor*> actors;
    for (rapidjson::Value& j_entry: j_doc["actors"].GetArray())
    {
        Actor* actor = nullptr;
        int index = actors.size();

        if (index < m_actors.size())
        {
            if (j_entry["filename"].GetString() != m_actors[index]->ar_filename)
            {
                if (m_actors[index] == App::GetSimController()->GetPlayerActor())
                {
                    App::GetSimController()->ChangePlayerActor(nullptr);
                }
                App::GetSimController()->RemoveActorDirectly(m_actors[index]);
            }
            else
            {
                actor = m_actors[index];
                actor->SyncReset(false);
            }
        }

        if (actor == nullptr)
        {
            bool preloaded = j_entry["preloaded_with_terrain"].GetBool();

            ActorSpawnRequest rq;
            rq.asr_filename      = j_entry["filename"].GetString();
            rq.asr_position.x    = j_entry["position"][0].GetFloat();
            rq.asr_position.y    = preloaded ? j_entry["position"][1].GetFloat() : j_entry["min_height"].GetFloat();
            rq.asr_position.z    = j_entry["position"][2].GetFloat();
            rq.asr_rotation      = Quaternion(Degree(270) - Radian(j_entry["rotation"].GetFloat()), Vector3::UNIT_Y);
            rq.asr_origin        = preloaded ? ActorSpawnRequest::Origin::TERRN_DEF : ActorSpawnRequest::Origin::USER;
            rq.asr_free_position = preloaded;
            actor                = App::GetSimController()->SpawnActorDirectly(rq);
        }

        actors.push_back(actor);
    }
    for (int index = actors.size(); index < m_actors.size(); index++)
    {
        if (m_actors[index] == App::GetSimController()->GetPlayerActor())
        {
            App::GetSimController()->ChangePlayerActor(nullptr);
        }
        App::GetSimController()->RemoveActorDirectly(m_actors[index]);
    }

    for (int index = 0; index < j_doc["actors"].Size(); index++)
    {
        if (actors[index] == nullptr)
            continue;

        Actor* actor = actors[index];
        rapidjson::Value& j_entry = j_doc["actors"][index];

        actor->m_spawn_rotation = j_entry["spawn_rotation"].GetFloat();
        actor->ar_sim_state = static_cast<Actor::SimState>(j_entry["sim_state"].GetInt());
        actor->ar_physics_paused = j_entry["physics_paused"].GetBool();

        if (j_entry["player_actor"].GetBool())
        {
            App::GetSimController()->SetPendingPlayerActor(actor);
        } else if (j_entry["prev_player_actor"].GetBool())
        {
            App::GetSimController()->SetPrevPlayerActorInternal(actor);
        }

        if (actor->ar_engine)
        {
            int gear = j_entry["engine_gear"].GetInt();
            float rpm = j_entry["engine_rpm"].GetFloat();
            int automode = j_entry["engine_auto_mode"].GetInt();
            int autoselect = j_entry["engine_auto_select"].GetInt();
            bool running = j_entry["engine_is_running"].GetBool();
            bool contact = j_entry["engine_has_contact"].GetBool();
            if (running != actor->ar_engine->IsRunning())
            {
                if (running)
                    actor->ar_engine->StartEngine();
                else
                    actor->ar_engine->StopEngine();
            }
            actor->ar_engine->PushNetworkState(rpm, 0.0f, 0.0f, gear, running, contact, automode, autoselect);
            actor->ar_engine->SetWheelSpin(j_entry["wheel_spin"].GetFloat() * RAD_PER_SEC_TO_RPM);
            actor->alb_mode = j_entry["alb_mode"].GetBool();
            actor->tc_mode = j_entry["tc_mode"].GetBool();
            actor->cc_mode = j_entry["cc_mode"].GetBool();
            actor->cc_target_rpm = j_entry["cc_target_rpm"].GetFloat();
            actor->cc_target_speed = j_entry["cc_target_speed"].GetFloat();
        }

        actor->ar_hydro_dir_state = j_entry["hydro_dir_state"].GetFloat();
        actor->ar_hydro_aileron_state = j_entry["hydro_aileron_state"].GetFloat();
        actor->ar_hydro_rudder_state = j_entry["hydro_rudder_state"].GetFloat();
        actor->ar_hydro_elevator_state = j_entry["hydro_elevator_state"].GetFloat();
        actor->ar_parking_brake = j_entry["parking_brake"].GetBool();

        if (actor->ar_lights != j_entry["lights"].GetInt())
        {
            actor->ToggleLights();
        }
        actor->m_beacon_light_is_active = j_entry["beacon_light"].GetBool();
        if (actor->m_custom_particles_enabled != j_entry["custom_particles"].GetBool())
        {
            actor->ToggleCustomParticles();
        }

        auto flares = j_entry["flares"].GetArray();
        for (int i = 0; i < static_cast<int>(actor->ar_flares.size()); i++)
        {
            actor->ar_flares[i].controltoggle_status = flares[i].GetBool();
        }

        if (actor->m_buoyance)
        {
            actor->m_buoyance->sink = j_entry["buoyance_sink"].GetBool();
        }

        auto aeroengines = j_entry["aeroengines"].GetArray();
        for (int i = 0; i < actor->ar_num_aeroengines; i++)
        {
            actor->ar_aeroengines[i]->setRPM(aeroengines[i]["rpm"].GetFloat());
            actor->ar_aeroengines[i]->setReverse(aeroengines[i]["reverse"].GetBool());
            actor->ar_aeroengines[i]->setIgnition(aeroengines[i]["ignition"].GetBool());
            actor->ar_aeroengines[i]->setThrottle(aeroengines[i]["throttle"].GetFloat());
        }

        auto screwprops = j_entry["screwprops"].GetArray();
        for (int i = 0; i < actor->ar_num_screwprops; i++)
        {
            actor->ar_screwprops[i]->setRudder(screwprops[i]["rudder"].GetFloat());
            actor->ar_screwprops[i]->setThrottle(screwprops[i]["throttle"].GetFloat());
        }

        for (int i = 0; i < actor->ar_num_rotators; i++)
        {
            actor->ar_rotators[i].angle = j_entry["rotators"][i].GetFloat();
        }

        for (int i = 0; i < actor->ar_num_wheels; i++)
        {
            if (actor->m_skid_trails[i])
            {
                actor->m_skid_trails[i]->reset();
            }
            actor->ar_wheels[i].wh_is_detached = j_entry["wheels"][i].GetBool();
        }

        for (int i = 0; i < actor->m_num_wheel_diffs; i++)
        {
            for (int k = 0; k < actor->m_wheel_diffs[i]->GetNumDiffTypes(); k++)
            {
                if (actor->m_wheel_diffs[i]->GetActiveDiffType() != j_entry["wheel_diffs"][i].GetInt())
                    actor->m_wheel_diffs[i]->ToggleDifferentialMode();
            }
        }

        for (int i = 0; i < actor->m_num_axle_diffs; i++)
        {
            for (int k = 0; k < actor->m_axle_diffs[i]->GetNumDiffTypes(); k++)
            {
                if (actor->m_axle_diffs[i]->GetActiveDiffType() != j_entry["axle_diffs"][i].GetInt())
                    actor->m_axle_diffs[i]->ToggleDifferentialMode();
            }
        }

        if (actor->m_transfer_case)
        {
            actor->m_transfer_case->tr_4wd_mode = j_entry["transfercase"]["4WD"].GetBool();
            for (int k = 0; k < actor->m_transfer_case->tr_gear_ratios.size(); k++)
            {
                if (actor->m_transfer_case->tr_gear_ratios[0] != j_entry["transfercase"]["GearRatio"].GetFloat())
                    actor->ToggleTransferCaseGearRatio();
            }
        }

        auto commands = j_entry["commands"].GetArray();
        for (int i = 0; i < MAX_COMMANDS; i++)
        {
            auto& command_key = actor->ar_command_key[i];
            command_key.commandValue = commands[i]["command_value"].GetFloat();
            command_key.triggerInputValue = commands[i]["trigger_input_value"].GetFloat();
            auto command_beams = commands[i]["command_beams"].GetArray();
            for (int j = 0; j < (int)command_key.beams.size(); j++)
            {
                command_key.beams[j].cmb_state->auto_moving_mode = command_beams[j]["auto_moving_mode"].GetInt();
                command_key.beams[j].cmb_state->pressed_center_mode = command_beams[j]["pressed_center_mode"].GetBool();
            }
        }

        auto nodes = j_entry["nodes"].GetArray();
        for (rapidjson::SizeType i = 0; i < nodes.Size(); i++)
        {
            auto data = nodes[i].GetArray();
            actor->ar_nodes[i].AbsPosition      = Vector3(data[0].GetFloat(), data[1].GetFloat(), data[2].GetFloat());
            actor->ar_nodes[i].RelPosition      = actor->ar_nodes[i].AbsPosition - actor->ar_origin;
            actor->ar_nodes[i].Velocity         = Vector3(data[3].GetFloat(), data[4].GetFloat(), data[5].GetFloat());
            actor->ar_initial_node_positions[i] = Vector3(data[6].GetFloat(), data[7].GetFloat(), data[8].GetFloat());
        }

        auto beams = j_entry["beams"].GetArray();
        for (rapidjson::SizeType i = 0; i < beams.Size(); i++)
        {
            auto data = beams[i].GetArray();
            actor->ar_beams[i].maxposstress       = data[0].GetFloat();
            actor->ar_beams[i].maxnegstress       = data[1].GetFloat();
            actor->ar_beams[i].minmaxposnegstress = data[2].GetFloat();
            actor->ar_beams[i].strength           = data[3].GetFloat();
            actor->ar_beams[i].L                  = data[4].GetFloat();
            actor->ar_beams[i].bm_broken          = data[5].GetBool();
            actor->ar_beams[i].bm_disabled        = data[6].GetBool();
            actor->ar_beams[i].bm_inter_actor     = data[7].GetBool();
            int locked_actor                      = data[8].GetInt();
            if (locked_actor != -1 && actors[locked_actor] != nullptr)
            {
                actor->AddInterActorBeam(&actor->ar_beams[i], actor, actors[locked_actor]);
            }
        }

        auto hooks = j_entry["hooks"].GetArray();
        for (int i = 0; i < actor->ar_hooks.size(); i++)
        {
            int lock_node = hooks[i]["lock_node"].GetInt();
            int locked_actor = hooks[i]["locked_actor"].GetInt();
            if (lock_node != -1 && locked_actor != -1 && actors[locked_actor] != nullptr)
            {
                actor->ar_hooks[i].hk_locked = hooks[i]["locked"].GetInt();
                actor->ar_hooks[i].hk_locked_actor = actors[locked_actor];
                actor->ar_hooks[i].hk_lock_node = &actors[locked_actor]->ar_nodes[lock_node];
                if (actor->ar_hooks[i].hk_beam->bm_inter_actor)
                {
                    actor->ar_hooks[i].hk_beam->p2 = actor->ar_hooks[i].hk_lock_node;
                }
            }
        }

        auto ropes = j_entry["ropes"].GetArray();
        for (int i = 0; i < actor->ar_ropes.size(); i++)
        {
            int ropable = ropes[i]["locked_ropable"].GetInt();
            int locked_actor = ropes[i]["locked_actor"].GetInt();
            if (ropable != -1 && locked_actor != -1 && actors[locked_actor] != nullptr)
            {
                actor->ar_ropes[i].rp_locked = ropes[i]["locked"].GetInt();
                actor->ar_ropes[i].rp_locked_actor = actors[locked_actor];
                actor->ar_ropes[i].rp_locked_ropable = &actors[locked_actor]->ar_ropables[ropable];
            }
        }

        auto ties = j_entry["ties"].GetArray();
        for (int i = 0; i < actor->ar_ties.size(); i++)
        {
            int ropable = ties[i]["locked_ropable"].GetInt();
            int locked_actor = ties[i]["locked_actor"].GetInt();
            if (ropable != -1 && locked_actor != -1 && actors[locked_actor] != nullptr)
            {
                actor->ar_ties[i].ti_tied  = ties[i]["tied"].GetBool();
                actor->ar_ties[i].ti_tying = ties[i]["tying"].GetBool();
                actor->ar_ties[i].ti_locked_actor = actors[locked_actor];
                actor->ar_ties[i].ti_locked_ropable = &actors[locked_actor]->ar_ropables[ropable];
                if (actor->ar_ties[i].ti_beam->bm_inter_actor)
                {
                    actor->ar_ties[i].ti_beam->p2 = actor->ar_ties[i].ti_locked_ropable->node;
                }
            }
        }

        auto ropables = j_entry["ropables"].GetArray();
        for (int i = 0; i < actor->ar_ropables.size(); i++)
        {
            actor->ar_ropables[i].attached_ties  = ropables[i]["attached_ties"].GetInt();
            actor->ar_ropables[i].attached_ropes = ropables[i]["attached_ropes"].GetInt();
        }

        actor->resetSlideNodes();
        if (actor->m_slidenodes_locked != j_entry["slidenodes_locked"].GetBool())
        {
            actor->ToggleSlideNodeLock();
        }
    }

    App::sim_load_savegame.SetActive(false);

    if (filename != "autosave.sav")
    {
        RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Scene loaded"));
    }

    return true;
}

bool ActorManager::SaveScene(Ogre::String filename)
{
    if (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
    {
        if (filename != "autosave.sav")
        {
            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Savegames are not available in multiplayer"));
        }
        return false;
    }

    rapidjson::Document j_doc;
    j_doc.SetObject();
    j_doc.AddMember("format_version", SAVEGAME_FILE_FORMAT, j_doc.GetAllocator());

    // Pretty name
    String pretty_name = App::GetCacheSystem()->getPrettyName(App::sim_terrain_name.GetActive());
    String scene_name = StringUtil::format("%s [%d]", pretty_name.c_str(), m_actors.size());
    j_doc.AddMember("scene_name", rapidjson::StringRef(scene_name.c_str()), j_doc.GetAllocator());

    // Terrain
    j_doc.AddMember("terrain_name", rapidjson::StringRef(App::sim_terrain_name.GetActive()), j_doc.GetAllocator());

#ifdef USE_CAELUM
    if (App::gfx_sky_mode.GetActive() == GfxSkyMode::CAELUM)
    {
        j_doc.AddMember("daytime", App::GetSimTerrain()->getSkyManager()->GetTime(), j_doc.GetAllocator());
    }
#endif // USE_CAELUM

    j_doc.AddMember("forced_awake", m_forced_awake, j_doc.GetAllocator());

    j_doc.AddMember("physics_paused", App::GetSimController()->GetPhysicsPaused(), j_doc.GetAllocator());

    // Character
    rapidjson::Value j_player_position(rapidjson::kArrayType);
    j_player_position.PushBack(gEnv->player->getPosition().x, j_doc.GetAllocator());
    j_player_position.PushBack(gEnv->player->getPosition().y, j_doc.GetAllocator());
    j_player_position.PushBack(gEnv->player->getPosition().z, j_doc.GetAllocator());
    j_doc.AddMember("player_position", j_player_position, j_doc.GetAllocator());
    j_doc.AddMember("player_rotation", gEnv->player->getRotation().valueRadians(), j_doc.GetAllocator());

    // Actors
    rapidjson::Value j_actors(rapidjson::kArrayType);
    for (auto actor : m_actors)
    {
        rapidjson::Value j_entry(rapidjson::kObjectType);

        j_entry.AddMember("filename", rapidjson::StringRef(actor->ar_filename.c_str()), j_doc.GetAllocator());
        rapidjson::Value j_actor_position(rapidjson::kArrayType);
        j_actor_position.PushBack(actor->ar_nodes[0].AbsPosition.x, j_doc.GetAllocator());
        j_actor_position.PushBack(actor->ar_nodes[0].AbsPosition.y, j_doc.GetAllocator());
        j_actor_position.PushBack(actor->ar_nodes[0].AbsPosition.z, j_doc.GetAllocator());
        j_entry.AddMember("position", j_actor_position, j_doc.GetAllocator());
        j_entry.AddMember("rotation", actor->getRotation(), j_doc.GetAllocator());
        j_entry.AddMember("min_height", actor->GetMinHeight(), j_doc.GetAllocator());
        j_entry.AddMember("spawn_rotation", actor->m_spawn_rotation, j_doc.GetAllocator());
        j_entry.AddMember("preloaded_with_terrain", actor->isPreloadedWithTerrain(), j_doc.GetAllocator());
        j_entry.AddMember("sim_state", static_cast<int>(actor->ar_sim_state), j_doc.GetAllocator());
        j_entry.AddMember("physics_paused", actor->ar_physics_paused, j_doc.GetAllocator());
        j_entry.AddMember("player_actor", actor==App::GetSimController()->GetPlayerActor(), j_doc.GetAllocator());
        j_entry.AddMember("prev_player_actor", actor==App::GetSimController()->GetPrevPlayerActor(), j_doc.GetAllocator());
        if (actor->ar_engine)
        {
            j_entry.AddMember("engine_gear", actor->ar_engine->GetGear(), j_doc.GetAllocator());
            j_entry.AddMember("engine_rpm", actor->ar_engine->GetEngineRpm(), j_doc.GetAllocator());
            j_entry.AddMember("engine_auto_mode", actor->ar_engine->GetAutoMode(), j_doc.GetAllocator());
            j_entry.AddMember("engine_auto_select", actor->ar_engine->getAutoShift(), j_doc.GetAllocator());
            j_entry.AddMember("engine_is_running", actor->ar_engine->IsRunning(), j_doc.GetAllocator());
            j_entry.AddMember("engine_has_contact", actor->ar_engine->HasStarterContact(), j_doc.GetAllocator());
            j_entry.AddMember("engine_wheel_spin", actor->ar_wheel_spin, j_doc.GetAllocator());
            j_entry.AddMember("alb_mode", actor->alb_mode, j_doc.GetAllocator());
            j_entry.AddMember("tc_mode", actor->tc_mode, j_doc.GetAllocator());
            j_entry.AddMember("cc_mode", actor->cc_mode, j_doc.GetAllocator());
            j_entry.AddMember("cc_target_rpm", actor->cc_target_rpm, j_doc.GetAllocator());
            j_entry.AddMember("cc_target_speed", actor->cc_target_speed, j_doc.GetAllocator());
        }

        j_entry.AddMember("hydro_dir_state", actor->ar_hydro_dir_state, j_doc.GetAllocator());
        j_entry.AddMember("hydro_aileron_state", actor->ar_hydro_aileron_state, j_doc.GetAllocator());
        j_entry.AddMember("hydro_rudder_state", actor->ar_hydro_rudder_state, j_doc.GetAllocator());
        j_entry.AddMember("hydro_elevator_state", actor->ar_hydro_elevator_state, j_doc.GetAllocator());
        j_entry.AddMember("parking_brake", actor->ar_parking_brake, j_doc.GetAllocator());

        j_entry.AddMember("lights", actor->ar_lights, j_doc.GetAllocator());
        j_entry.AddMember("beacon_light", actor->m_beacon_light_is_active, j_doc.GetAllocator());
        j_entry.AddMember("custom_particles", actor->m_custom_particles_enabled, j_doc.GetAllocator());

        // Flares
        rapidjson::Value j_flares(rapidjson::kArrayType);
        for (const auto& flare : actor->ar_flares)
        {
            j_flares.PushBack(flare.controltoggle_status, j_doc.GetAllocator());
        }
        j_entry.AddMember("flares", j_flares, j_doc.GetAllocator());

        if (actor->m_buoyance)
        {
            j_entry.AddMember("buoyance_sink", actor->m_buoyance->sink, j_doc.GetAllocator());
        }

        // Turboprops / Turbojets
        rapidjson::Value j_aeroengines(rapidjson::kArrayType);
        for (int i = 0; i < actor->ar_num_aeroengines; i++)
        {
            rapidjson::Value j_aeroengine(rapidjson::kObjectType);
            j_aeroengine.AddMember("rpm", actor->ar_aeroengines[i]->getRPM(), j_doc.GetAllocator());
            j_aeroengine.AddMember("reverse", actor->ar_aeroengines[i]->getReverse(), j_doc.GetAllocator());
            j_aeroengine.AddMember("ignition", actor->ar_aeroengines[i]->getIgnition(), j_doc.GetAllocator());
            j_aeroengine.AddMember("throttle", actor->ar_aeroengines[i]->getThrottle(), j_doc.GetAllocator());
            j_aeroengines.PushBack(j_aeroengine, j_doc.GetAllocator());
        }
        j_entry.AddMember("aeroengines", j_aeroengines, j_doc.GetAllocator());

        // Screwprops
        rapidjson::Value j_screwprops(rapidjson::kArrayType);
        for (int i = 0; i < actor->ar_num_screwprops; i++)
        {
            rapidjson::Value j_screwprop(rapidjson::kObjectType);
            j_screwprop.AddMember("rudder", actor->ar_screwprops[i]->getRudder(), j_doc.GetAllocator());
            j_screwprop.AddMember("throttle", actor->ar_screwprops[i]->getThrottle(), j_doc.GetAllocator());
            j_screwprops.PushBack(j_screwprop, j_doc.GetAllocator());
        }
        j_entry.AddMember("screwprops", j_screwprops, j_doc.GetAllocator());

        // Rotators
        rapidjson::Value j_rotators(rapidjson::kArrayType);
        for (int i = 0; i < actor->ar_num_rotators; i++)
        {
            j_rotators.PushBack(actor->ar_rotators[i].angle, j_doc.GetAllocator());
        }
        j_entry.AddMember("rotators", j_rotators, j_doc.GetAllocator());

        // Wheels
        rapidjson::Value j_wheels(rapidjson::kArrayType);
        for (int i = 0; i < actor->ar_num_wheels; i++)
        {
            j_wheels.PushBack(actor->ar_wheels[i].wh_is_detached, j_doc.GetAllocator());
        }
        j_entry.AddMember("wheels", j_wheels, j_doc.GetAllocator());

        // Wheel differentials
        rapidjson::Value j_wheel_diffs(rapidjson::kArrayType);
        for (int i = 0; i < actor->m_num_wheel_diffs; i++)
        {
            j_wheel_diffs.PushBack(actor->m_wheel_diffs[i]->GetActiveDiffType(), j_doc.GetAllocator());
        }
        j_entry.AddMember("wheel_diffs", j_wheel_diffs, j_doc.GetAllocator());

        // Axle differentials
        rapidjson::Value j_axle_diffs(rapidjson::kArrayType);
        for (int i = 0; i < actor->m_num_axle_diffs; i++)
        {
            j_axle_diffs.PushBack(actor->m_axle_diffs[i]->GetActiveDiffType(), j_doc.GetAllocator());
        }
        j_entry.AddMember("axle_diffs", j_axle_diffs, j_doc.GetAllocator());

        // Transfercase
        if (actor->m_transfer_case)
        {
            rapidjson::Value j_transfer_case(rapidjson::kObjectType);
            j_transfer_case.AddMember("4WD", actor->m_transfer_case->tr_4wd_mode, j_doc.GetAllocator());
            j_transfer_case.AddMember("GearRatio", actor->m_transfer_case->tr_gear_ratios[0], j_doc.GetAllocator());
            j_entry.AddMember("transfercase", j_transfer_case, j_doc.GetAllocator());
        }

        // Commands
        rapidjson::Value j_commands(rapidjson::kArrayType);
        for (int i = 0; i < MAX_COMMANDS; i++)
        {
            rapidjson::Value j_command(rapidjson::kObjectType);
            j_command.AddMember("command_value", actor->ar_command_key[i].commandValue, j_doc.GetAllocator());
            j_command.AddMember("trigger_input_value", actor->ar_command_key[i].triggerInputValue, j_doc.GetAllocator());

            rapidjson::Value j_command_beams(rapidjson::kArrayType);
            for (int j = 0; j < (int)actor->ar_command_key[i].beams.size(); j++)
            {
                rapidjson::Value j_cmb(rapidjson::kObjectType);
                auto& beam = actor->ar_command_key[i].beams[j];
                j_cmb.AddMember("auto_moving_mode", beam.cmb_state->auto_moving_mode, j_doc.GetAllocator());
                j_cmb.AddMember("pressed_center_mode", beam.cmb_state->pressed_center_mode, j_doc.GetAllocator());
                j_command_beams.PushBack(j_cmb, j_doc.GetAllocator());
            }
            j_command.AddMember("command_beams", j_command_beams, j_doc.GetAllocator());

            j_commands.PushBack(j_command, j_doc.GetAllocator());
        }
        j_entry.AddMember("commands", j_commands, j_doc.GetAllocator());

        // Hooks
        rapidjson::Value j_hooks(rapidjson::kArrayType);
        for (const auto& h : actor->ar_hooks)
        {
            rapidjson::Value j_hook(rapidjson::kObjectType);
            int lock_node = h.hk_lock_node ? h.hk_lock_node->pos : -1;
            int locked_actor = h.hk_locked_actor ? h.hk_locked_actor->ar_vector_index : -1;
            j_hook.AddMember("locked", h.hk_locked, j_doc.GetAllocator());
            j_hook.AddMember("lock_node", lock_node, j_doc.GetAllocator());
            j_hook.AddMember("locked_actor", locked_actor, j_doc.GetAllocator());
            j_hooks.PushBack(j_hook, j_doc.GetAllocator());
        }
        j_entry.AddMember("hooks", j_hooks, j_doc.GetAllocator());

        // Ropes
        rapidjson::Value j_ropes(rapidjson::kArrayType);
        for (const auto& r : actor->ar_ropes)
        {
            rapidjson::Value j_rope(rapidjson::kObjectType);
            int locked_ropable = r.rp_locked_ropable ? r.rp_locked_ropable->pos : -1;
            int locked_actor = r.rp_locked_actor ? r.rp_locked_actor->ar_vector_index : -1;
            j_rope.AddMember("locked", r.rp_locked, j_doc.GetAllocator());
            j_rope.AddMember("locked_ropable", locked_ropable, j_doc.GetAllocator());
            j_rope.AddMember("locked_actor", locked_actor, j_doc.GetAllocator());
            j_ropes.PushBack(j_rope, j_doc.GetAllocator());
        }
        j_entry.AddMember("ropes", j_ropes, j_doc.GetAllocator());

        // Ties
        rapidjson::Value j_ties(rapidjson::kArrayType);
        for (const auto& t : actor->ar_ties)
        {
            rapidjson::Value j_tie(rapidjson::kObjectType);
            int locked_ropable = t.ti_locked_ropable ? t.ti_locked_ropable->pos : -1;
            int locked_actor = t.ti_locked_actor ? t.ti_locked_actor->ar_vector_index : -1;
            j_tie.AddMember("tied", t.ti_tied, j_doc.GetAllocator());
            j_tie.AddMember("tying", t.ti_tying, j_doc.GetAllocator());
            j_tie.AddMember("locked_ropable", locked_ropable, j_doc.GetAllocator());
            j_tie.AddMember("locked_actor", locked_actor, j_doc.GetAllocator());
            j_ties.PushBack(j_tie, j_doc.GetAllocator());
        }
        j_entry.AddMember("ties", j_ties, j_doc.GetAllocator());

        // Ropables
        rapidjson::Value j_ropables(rapidjson::kArrayType);
        for (const auto& r : actor->ar_ropables)
        {
            rapidjson::Value j_ropable(rapidjson::kObjectType);
            j_ropable.AddMember("attached_ties", r.attached_ties, j_doc.GetAllocator());
            j_ropable.AddMember("attached_ropes", r.attached_ropes, j_doc.GetAllocator());
            j_ropables.PushBack(j_ropable, j_doc.GetAllocator());
        }
        j_entry.AddMember("ropables", j_ropables, j_doc.GetAllocator());

        j_entry.AddMember("slidenodes_locked", actor->m_slidenodes_locked, j_doc.GetAllocator());

        // Nodes
        rapidjson::Value j_nodes(rapidjson::kArrayType);
        for (int i = 0; i < actor->ar_num_nodes; i++)
        {
            rapidjson::Value j_node(rapidjson::kArrayType);

            // Position
            j_node.PushBack(actor->ar_nodes[i].AbsPosition.x, j_doc.GetAllocator());
            j_node.PushBack(actor->ar_nodes[i].AbsPosition.y, j_doc.GetAllocator());
            j_node.PushBack(actor->ar_nodes[i].AbsPosition.z, j_doc.GetAllocator());

            // Velocity
            j_node.PushBack(actor->ar_nodes[i].Velocity.x, j_doc.GetAllocator());
            j_node.PushBack(actor->ar_nodes[i].Velocity.y, j_doc.GetAllocator());
            j_node.PushBack(actor->ar_nodes[i].Velocity.z, j_doc.GetAllocator());

            // Initial Position
            j_node.PushBack(actor->ar_initial_node_positions[i].x, j_doc.GetAllocator());
            j_node.PushBack(actor->ar_initial_node_positions[i].y, j_doc.GetAllocator());
            j_node.PushBack(actor->ar_initial_node_positions[i].z, j_doc.GetAllocator());

            j_nodes.PushBack(j_node, j_doc.GetAllocator());
        }
        j_entry.AddMember("nodes", j_nodes, j_doc.GetAllocator());

        // Beams
        rapidjson::Value j_beams(rapidjson::kArrayType);
        for (int i = 0; i < actor->ar_num_beams; i++)
        {
            rapidjson::Value j_beam(rapidjson::kArrayType);

            j_beam.PushBack(actor->ar_beams[i].maxposstress, j_doc.GetAllocator());
            j_beam.PushBack(actor->ar_beams[i].maxnegstress, j_doc.GetAllocator());
            j_beam.PushBack(actor->ar_beams[i].minmaxposnegstress, j_doc.GetAllocator());
            j_beam.PushBack(actor->ar_beams[i].strength, j_doc.GetAllocator());
            j_beam.PushBack(actor->ar_beams[i].L, j_doc.GetAllocator());
            j_beam.PushBack(actor->ar_beams[i].bm_broken, j_doc.GetAllocator());
            j_beam.PushBack(actor->ar_beams[i].bm_disabled, j_doc.GetAllocator());
            j_beam.PushBack(actor->ar_beams[i].bm_inter_actor, j_doc.GetAllocator());
            Actor* locked_actor = actor->ar_beams[i].bm_locked_actor;
            j_beam.PushBack(locked_actor ? locked_actor->ar_vector_index : -1, j_doc.GetAllocator());

            j_beams.PushBack(j_beam, j_doc.GetAllocator());
        }
        j_entry.AddMember("beams", j_beams, j_doc.GetAllocator());

        j_actors.PushBack(j_entry, j_doc.GetAllocator());
    }
    j_doc.AddMember("actors", j_actors, j_doc.GetAllocator());

    // Write to disk
    String path = PathCombine(App::sys_savegames_dir.GetActive(), filename);
    RoR::LogFormat("[RoR|Savegame] Writing savegame to file '%s' ...", path.c_str());

    std::ofstream ofs(path);
    rapidjson::OStreamWrapper j_ofs(ofs);
    rapidjson::Writer<rapidjson::OStreamWrapper, rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::CrtAllocator,
        rapidjson::kWriteNanAndInfFlag> j_writer(j_ofs);
    if (!j_doc.Accept(j_writer))
    {
        RoR::LogFormat("[RoR|Savegame] Error writing '%s'", path.c_str());
        RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Error while saving scene"));
        return false;
    }

    if (filename != "autosave.sav")
    {
        RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Scene saved"));
    }

    return true;
}
