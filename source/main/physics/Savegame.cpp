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

#include "Application.h"

#include "AeroEngine.h"
#include "Application.h"
#include "Actor.h"
#include "Buoyance.h"
#include "CacheSystem.h"
#include "ContentManager.h"
#include "Console.h"
#include "EngineSim.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GUI_MessageBox.h"
#include "InputEngine.h"
#include "Language.h"
#include "PlatformUtils.h"
#include "ScrewProp.h"
#include "Skidmark.h"
#include "SkyManager.h"
#include "Terrain.h"

#include <rapidjson/rapidjson.h>
#include <fstream>

#define SAVEGAME_FILE_FORMAT 3

using namespace Ogre;
using namespace RoR;

// --------------------------------
// GameContext functions

std::string GameContext::GetQuicksaveFilename()
{
    std::string terrain_name = App::sim_terrain_name->getStr();
    std::string mp = (App::mp_state->getEnum<MpState>() == RoR::MpState::CONNECTED) ? "_mp" : "";

    return "quicksave_" + StringUtil::replaceAll(terrain_name, ".terrn2", "") + mp + ".sav";
}

void GameContext::LoadScene(std::string const& filename)
{
    ROR_ASSERT(App::GetGameContext()->GetTerrain());
    m_actor_manager.LoadScene(filename);
}

void GameContext::SaveScene(std::string const& filename)
{
    m_actor_manager.SaveScene(filename);
}

std::string GameContext::ExtractSceneName(std::string const& filename)
{
    // Read from disk
    rapidjson::Document j_doc;
    if (!App::GetContentManager()->LoadAndParseJson(filename, RGN_SAVEGAMES, j_doc) ||
        !j_doc.IsObject() || !j_doc.HasMember("format_version") || !j_doc["format_version"].IsNumber() ||
        !j_doc.HasMember("scene_name") || !j_doc["scene_name"].IsString())
        return "";

    return j_doc["scene_name"].GetString();
}

std::string GameContext::ExtractSceneTerrain(std::string const& filename)
{
    // Read from disk
    rapidjson::Document j_doc;
    if (!App::GetContentManager()->LoadAndParseJson(filename, RGN_SAVEGAMES, j_doc) ||
        !j_doc.IsObject() || !j_doc.HasMember("format_version") || !j_doc["format_version"].IsNumber() ||
        !j_doc.HasMember("terrain_name") || !j_doc["terrain_name"].IsString())
        return "";

    return j_doc["terrain_name"].GetString();
}

void GameContext::HandleSavegameHotkeys()
{
    // Global savegames
    int slot = -1;
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_01, 1.0f))
    {
        slot = 1;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_02, 1.0f))
    {
        slot = 2;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_03, 1.0f))
    {
        slot = 3;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_04, 1.0f))
    {
        slot = 4;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_05, 1.0f))
    {
        slot = 5;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_06, 1.0f))
    {
        slot = 6;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_07, 1.0f))
    {
        slot = 7;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_08, 1.0f))
    {
        slot = 8;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_09, 1.0f))
    {
        slot = 9;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_10, 1.0f))
    {
        slot = 0;
    }
    if (slot != -1)
    {
        Ogre::String filename = Ogre::StringUtil::format("quicksave-%d.sav", slot);
        App::GetGameContext()->PushMessage(Message(MSG_SIM_LOAD_SAVEGAME_REQUESTED, filename));
    }

    if (App::sim_terrain_name->getStr() == "" || App::sim_state->getEnum<SimState>() != SimState::RUNNING)
        return;

    slot = -1;
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKSAVE_01, 1.0f))
    {
        slot = 1;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKSAVE_02, 1.0f))
    {
        slot = 2;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKSAVE_03, 1.0f))
    {
        slot = 3;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKSAVE_04, 1.0f))
    {
        slot = 4;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKSAVE_05, 1.0f))
    {
        slot = 5;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKSAVE_06, 1.0f))
    {
        slot = 6;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKSAVE_07, 1.0f))
    {
        slot = 7;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKSAVE_08, 1.0f))
    {
        slot = 8;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKSAVE_09, 1.0f))
    {
        slot = 9;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKSAVE_10, 1.0f))
    {
        slot = 0;
    }
    if (slot != -1)
    {
        Ogre::String filename = Ogre::StringUtil::format("quicksave-%d.sav", slot);
        App::GetGameContext()->SaveScene(filename);
    }

    // Terrain local savegames

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD, 1.0f))
    {
        if (App::sim_quickload_dialog->getBool())
        {
            GUI::MessageBoxConfig* dialog = new GUI::MessageBoxConfig;
            dialog->mbc_title = _LC("QuickloadDialog", "Load game?");
            dialog->mbc_text = _LC("QuickloadDialog", "You will lose all unsaved progress!");
            dialog->mbc_always_ask_conf = App::sim_quickload_dialog;

            GUI::MessageBoxButton ok_btn;
            ok_btn.mbb_caption = _LC("QuickloadDialog", "Load");
            ok_btn.mbb_mq_message = MsgType::MSG_SIM_LOAD_SAVEGAME_REQUESTED;
            ok_btn.mbb_mq_description = App::GetGameContext()->GetQuicksaveFilename();
            dialog->mbc_buttons.push_back(ok_btn);

            GUI::MessageBoxButton cancel_btn;
            cancel_btn.mbb_caption = _LC("QuickloadDialog", "Cancel");
            dialog->mbc_buttons.push_back(cancel_btn); // No action - just close the dialog.

            App::GetGameContext()->PushMessage(
                Message(MSG_GUI_SHOW_MESSAGE_BOX_REQUESTED, (void*)dialog));
        }
        else
        {
            App::GetGameContext()->PushMessage(
                Message(MSG_SIM_LOAD_SAVEGAME_REQUESTED,
                    App::GetGameContext()->GetQuicksaveFilename()));
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKSAVE))
    {
        App::GetGameContext()->SaveScene(App::GetGameContext()->GetQuicksaveFilename());
    }
}

// --------------------------------
// ActorManager functions

bool ActorManager::LoadScene(Ogre::String filename)
{
    // Read from disk
    rapidjson::Document j_doc;
    if (!App::GetContentManager()->LoadAndParseJson(filename, RGN_SAVEGAMES, j_doc) ||
        !j_doc.IsObject() || !j_doc.HasMember("format_version") || !j_doc["format_version"].IsNumber())
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("Error while loading scene: File invalid or missing"));
        return false;
    }
    if (j_doc["format_version"].GetInt() != SAVEGAME_FILE_FORMAT)
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("Error while loading scene: File format mismatch"));
        return false;
    }

    // Terrain
    String terrain_name = j_doc["terrain_name"].GetString();

    if (App::mp_state->getEnum<MpState>() == RoR::MpState::CONNECTED)
    {
        if (filename == "autosave.sav")
            return false;
        if (terrain_name != App::sim_terrain_name->getStr())
        {
            App::GetConsole()->putMessage(
                Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("Error while loading scene: Terrain mismatch"));
            return false;
        }
        if (j_doc["actors"].GetArray().Size() > 3)
        {
            App::GetConsole()->putMessage(
                Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("Error while loading scene: Too many vehicles"));
            return false;
        }
    }

    m_forced_awake = j_doc["forced_awake"].GetBool();

    App::GetGameContext()->GetActorManager()->SetSimulationPaused(j_doc["physics_paused"].GetBool());

#ifdef USE_CAELUM
    if (App::gfx_sky_mode->getEnum<GfxSkyMode>() == GfxSkyMode::CAELUM)
    {
        if (j_doc.HasMember("daytime"))
        {
            App::GetGameContext()->GetTerrain()->getSkyManager()->SetTime(j_doc["daytime"].GetDouble());
        }
    }
#endif // USE_CAELUM

    // Character
    auto data = j_doc["player_position"].GetArray();
    Vector3 position = Vector3(data[0].GetFloat(), data[1].GetFloat(), data[2].GetFloat());
    App::GetGameContext()->GetPlayerCharacter()->setPosition(position);
    App::GetGameContext()->GetPlayerCharacter()->setRotation(Radian(j_doc["player_rotation"].GetFloat()));

    // Actors
    auto actors_changed = false;
    auto player_actor = App::GetGameContext()->GetPlayerActor();
    auto prev_player_actor = App::GetGameContext()->GetPrevPlayerActor();
    std::vector<ActorPtr> actors;
    std::vector<ActorPtr> x_actors = GetLocalActors();
    for (rapidjson::Value& j_entry: j_doc["actors"].GetArray())
    {
        String filename = j_entry["filename"].GetString();
        if (!App::GetCacheSystem()->CheckResourceLoaded(filename))
        {
            Str<600> msg; msg << _L("Error while loading scene: Missing content (probably not installed)") << " '" << filename << "'";
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_ERROR, msg.ToCStr());
            actors.push_back(nullptr);
            continue;
        }

        CacheEntry* skin = nullptr;
        if (j_entry.HasMember("skin"))
        {
            skin = App::GetCacheSystem()->FetchSkinByName(j_entry["skin"].GetString());
        }

        String section_config = j_entry["section_config"].GetString();

        ActorPtr actor = nullptr;
        int index = static_cast<int>(actors.size());
        if (index < x_actors.size())
        {
            if (j_entry["filename"].GetString() != x_actors[index]->ar_filename ||
                    (skin != nullptr && skin->dname != x_actors[index]->m_used_skin_entry->dname) ||
                    section_config != x_actors[index]->getSectionConfig())
            {
                if (x_actors[index] == player_actor)
                {
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, nullptr));
                }
                else if (x_actors[index] == prev_player_actor)
                {
                    App::GetGameContext()->SetPrevPlayerActor(nullptr);
                }
                App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, static_cast<void*>(new ActorPtr(x_actors[index]))));
                actors_changed = true;
            }
            else
            {
                actor = x_actors[index];
                actor->SyncReset(false);
            }
        }

        if (actor == nullptr)
        {
            bool preloaded = j_entry["preloaded_with_terrain"].GetBool();

            ActorSpawnRequest* rq = new ActorSpawnRequest;
            rq->asr_filename      = filename;
            rq->asr_position.x    = j_entry["position"][0].GetFloat();
            rq->asr_position.y    = preloaded ? j_entry["position"][1].GetFloat() : j_entry["min_height"].GetFloat();
            rq->asr_position.z    = j_entry["position"][2].GetFloat();
            rq->asr_rotation      = Quaternion(Degree(270) - Radian(j_entry["rotation"].GetFloat()), Vector3::UNIT_Y);
            rq->asr_skin_entry    = skin;
            rq->asr_config        = section_config;
            rq->asr_origin        = preloaded ? ActorSpawnRequest::Origin::TERRN_DEF : ActorSpawnRequest::Origin::SAVEGAME;
            rq->asr_free_position = preloaded;
            // Copy saved state
            rq->asr_saved_state = std::shared_ptr<rapidjson::Document>(new rapidjson::Document());
            rq->asr_saved_state->CopyFrom(j_entry, rq->asr_saved_state->GetAllocator());

            App::GetGameContext()->PushMessage(Message(MSG_SIM_SPAWN_ACTOR_REQUESTED, (void*)rq));
            actors_changed = true;
            // CAUTION: `actor` remains nullptr!
        }

        actors.push_back(actor);
    }
    for (size_t index = actors.size(); index < x_actors.size(); index++)
    {
        if (x_actors[index] == player_actor)
        {
            App::GetGameContext()->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, nullptr));
        }
        else if (x_actors[index] == prev_player_actor)
        {
            App::GetGameContext()->SetPrevPlayerActor(nullptr);
        }
        App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, static_cast<void*>(new ActorPtr(x_actors[index]))));
        actors_changed = true;
    }

    const int num_actors = static_cast<int>(j_doc["actors"].Size());
    for (int index = 0; index < num_actors; index++)
    {
        if (actors[index] == nullptr)
            continue;

        ActorPtr actor = actors[index];
        rapidjson::Value& j_entry = j_doc["actors"][index];

        this->RestoreSavedState(actor, j_entry);
    }

    if (filename != "autosave.sav")
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Scene loaded"));
    }

    return true;
}

bool ActorManager::SaveScene(Ogre::String filename)
{
    std::vector<ActorPtr> x_actors = GetLocalActors();

    if (App::mp_state->getEnum<MpState>() == RoR::MpState::CONNECTED)
    {
        if (filename == "autosave.sav")
            return false;
        if (x_actors.size() > 3)
        {
            App::GetConsole()->putMessage(
                Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("Error while saving scene: Too many vehicles"));
            return false;
        }
    }

    rapidjson::Document j_doc;
    j_doc.SetObject();
    j_doc.AddMember("format_version", SAVEGAME_FILE_FORMAT, j_doc.GetAllocator());

    // Pretty name
    String pretty_name = App::GetCacheSystem()->GetPrettyName(App::sim_terrain_name->getStr());
    String scene_name = StringUtil::format("%s [%d]", pretty_name.c_str(), x_actors.size());
    j_doc.AddMember("scene_name", rapidjson::StringRef(scene_name.c_str()), j_doc.GetAllocator());

    // Terrain
    j_doc.AddMember("terrain_name", rapidjson::StringRef(App::sim_terrain_name->getStr().c_str()), j_doc.GetAllocator());

#ifdef USE_CAELUM
    if (App::gfx_sky_mode->getEnum<GfxSkyMode>() == GfxSkyMode::CAELUM)
    {
        j_doc.AddMember("daytime", App::GetGameContext()->GetTerrain()->getSkyManager()->GetTime(), j_doc.GetAllocator());
    }
#endif // USE_CAELUM

    j_doc.AddMember("forced_awake", m_forced_awake, j_doc.GetAllocator());

    j_doc.AddMember("physics_paused", App::GetGameContext()->GetActorManager()->IsSimulationPaused(), j_doc.GetAllocator());

    // Character
    rapidjson::Value j_player_position(rapidjson::kArrayType);
    j_player_position.PushBack(App::GetGameContext()->GetPlayerCharacter()->getPosition().x, j_doc.GetAllocator());
    j_player_position.PushBack(App::GetGameContext()->GetPlayerCharacter()->getPosition().y, j_doc.GetAllocator());
    j_player_position.PushBack(App::GetGameContext()->GetPlayerCharacter()->getPosition().z, j_doc.GetAllocator());
    j_doc.AddMember("player_position", j_player_position, j_doc.GetAllocator());
    j_doc.AddMember("player_rotation", App::GetGameContext()->GetPlayerCharacter()->getRotation().valueRadians(), j_doc.GetAllocator());

    std::map<int, int> vector_index_lookup;
    for (ActorPtr& actor : m_actors)
    {
        vector_index_lookup[actor->ar_vector_index] = -1;
        auto search = std::find_if(x_actors.begin(), x_actors.end(), [actor](ActorPtr b)
                { return actor->ar_instance_id == b->ar_instance_id; });
        if (search != x_actors.end())
        {
            vector_index_lookup[actor->ar_vector_index] = std::distance(x_actors.begin(), search);
        }
    }

    // Actors
    rapidjson::Value j_actors(rapidjson::kArrayType);
    for (ActorPtr actor : x_actors)
    {
        rapidjson::Value j_entry(rapidjson::kObjectType);

        j_entry.AddMember("filename", rapidjson::StringRef(actor->ar_filename.c_str()), j_doc.GetAllocator());
        rapidjson::Value j_actor_position(rapidjson::kArrayType);
        j_actor_position.PushBack(actor->ar_nodes[0].AbsPosition.x, j_doc.GetAllocator());
        j_actor_position.PushBack(actor->ar_nodes[0].AbsPosition.y, j_doc.GetAllocator());
        j_actor_position.PushBack(actor->ar_nodes[0].AbsPosition.z, j_doc.GetAllocator());
        j_entry.AddMember("position", j_actor_position, j_doc.GetAllocator());
        j_entry.AddMember("rotation", actor->getRotation(), j_doc.GetAllocator());
        j_entry.AddMember("min_height", actor->getMinHeight(), j_doc.GetAllocator());
        j_entry.AddMember("spawn_rotation", actor->m_spawn_rotation, j_doc.GetAllocator());
        j_entry.AddMember("preloaded_with_terrain", actor->isPreloadedWithTerrain(), j_doc.GetAllocator());
        j_entry.AddMember("sim_state", static_cast<int>(actor->ar_state), j_doc.GetAllocator());
        j_entry.AddMember("physics_paused", actor->ar_physics_paused, j_doc.GetAllocator());
        j_entry.AddMember("player_actor", actor==App::GetGameContext()->GetPlayerActor(), j_doc.GetAllocator());
        j_entry.AddMember("prev_player_actor", actor==App::GetGameContext()->GetPrevPlayerActor(), j_doc.GetAllocator());

        if (actor->m_used_skin_entry)
        {
            j_entry.AddMember("skin", rapidjson::StringRef(actor->m_used_skin_entry->dname.c_str()), j_doc.GetAllocator());
        }

        j_entry.AddMember("section_config", rapidjson::StringRef(actor->m_section_config.c_str()), j_doc.GetAllocator());

        // Engine, anti-lock brake, traction control
        if (actor->ar_engine)
        {
            j_entry.AddMember("engine_gear", actor->ar_engine->GetGear(), j_doc.GetAllocator());
            j_entry.AddMember("engine_rpm", actor->ar_engine->GetEngineRpm(), j_doc.GetAllocator());
            j_entry.AddMember("engine_auto_mode", actor->ar_engine->GetAutoMode(), j_doc.GetAllocator());
            j_entry.AddMember("engine_auto_select", actor->ar_engine->getAutoShift(), j_doc.GetAllocator());
            j_entry.AddMember("engine_is_running", actor->ar_engine->isRunning(), j_doc.GetAllocator());
            j_entry.AddMember("engine_has_contact", actor->ar_engine->hasContact(), j_doc.GetAllocator());
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
        j_entry.AddMember("trailer_parking_brake", actor->ar_trailer_parking_brake, j_doc.GetAllocator());
        j_entry.AddMember("avg_wheel_speed", actor->ar_avg_wheel_speed, j_doc.GetAllocator());
        j_entry.AddMember("wheel_speed", actor->ar_wheel_speed, j_doc.GetAllocator());
        j_entry.AddMember("wheel_spin", actor->ar_wheel_spin, j_doc.GetAllocator());

        j_entry.AddMember("custom_particles", actor->m_custom_particles_enabled, j_doc.GetAllocator());

        // Flares
        j_entry.AddMember("lights", (int)actor->getHeadlightsVisible(), j_doc.GetAllocator());
        j_entry.AddMember("blink_type", (int)actor->getBlinkType(), j_doc.GetAllocator());
        // "beacon_light" was "pp_beacon_light" since release 2021.02 (savegame file format 2).
        // It was caused by find-&-replace derp in commit 5a159ad9c0d0ffb1fa3e6f4f9c4577fab3910e3e.
        j_entry.AddMember("beacon_light", actor->getBeaconMode(), j_doc.GetAllocator());
        j_entry.AddMember("high_beams_on", actor->getHighBeamsVisible(), j_doc.GetAllocator());
        j_entry.AddMember("fog_lights_on", actor->getFogLightsVisible(), j_doc.GetAllocator());

        // User-defined flares
        rapidjson::Value j_custom_lights(rapidjson::kArrayType);
        for (int i = 0; i < MAX_CLIGHTS; i++)
        {
            j_custom_lights.PushBack(actor->getCustomLightVisible(i), j_doc.GetAllocator());
        }
        j_entry.AddMember("custom_lights", j_custom_lights, j_doc.GetAllocator());

        // Buoyance
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
        for (size_t i = 0; i < actor->ar_wheels.size(); i++)
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
            rapidjson::Value j_command(rapidjson::kArrayType);
            j_command.PushBack(actor->ar_command_key[i].commandValue, j_doc.GetAllocator());
            j_command.PushBack(actor->ar_command_key[i].triggerInputValue, j_doc.GetAllocator());

            rapidjson::Value j_command_beams(rapidjson::kArrayType);
            for (int j = 0; j < (int)actor->ar_command_key[i].beams.size(); j++)
            {
                rapidjson::Value j_cmb(rapidjson::kArrayType);
                auto& beam = actor->ar_command_key[i].beams[j];
                j_cmb.PushBack(beam.cmb_state->auto_moving_mode, j_doc.GetAllocator());
                j_cmb.PushBack(beam.cmb_state->pressed_center_mode, j_doc.GetAllocator());
                j_command_beams.PushBack(j_cmb, j_doc.GetAllocator());
            }
            j_command.PushBack(j_command_beams, j_doc.GetAllocator());

            j_commands.PushBack(j_command, j_doc.GetAllocator());
        }
        j_entry.AddMember("commands", j_commands, j_doc.GetAllocator());

        // Hooks
        rapidjson::Value j_hooks(rapidjson::kArrayType);
        for (const auto& h : actor->ar_hooks)
        {
            rapidjson::Value j_hook(rapidjson::kObjectType);
            int lock_node = h.hk_locked_node != NODENUM_INVALID ? h.hk_locked_node : -1;
            int locked_actor = h.hk_locked_actor ? vector_index_lookup[h.hk_locked_actor->ar_vector_index] : -1;
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
            int locked_ropable = r.rp_locked_ropable_id != ROPABLEID_INVALID ? r.rp_locked_ropable_id : -1;
            int locked_actor = r.rp_locked_actor ? vector_index_lookup[r.rp_locked_actor->ar_vector_index] : -1;
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
            int locked_ropable = t.ti_locked_ropable_id != ROPABLEID_INVALID ? t.ti_locked_ropable_id : -1;
            int locked_actor = t.ti_locked_actor ? vector_index_lookup[t.ti_locked_actor->ar_vector_index] : -1;
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
        for (int i = 0; i < static_cast<int>(actor->ar_nodes.size()); i++)
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
            j_node.PushBack(actor->ar_nodes_aux[i].nda_initial_node_position.x, j_doc.GetAllocator());
            j_node.PushBack(actor->ar_nodes_aux[i].nda_initial_node_position.y, j_doc.GetAllocator());
            j_node.PushBack(actor->ar_nodes_aux[i].nda_initial_node_position.z, j_doc.GetAllocator());

            j_nodes.PushBack(j_node, j_doc.GetAllocator());
        }
        j_entry.AddMember("nodes", j_nodes, j_doc.GetAllocator());

        // Beams
        rapidjson::Value j_beams(rapidjson::kArrayType);
        for (int i = 0; i < static_cast<int>(actor->ar_beams.size()); i++)
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
            ActorPtr locked_actor = actor->ar_beams[i].bm_locked_actor;
            j_beam.PushBack(locked_actor ? vector_index_lookup[locked_actor->ar_vector_index] : -1, j_doc.GetAllocator());

            j_beams.PushBack(j_beam, j_doc.GetAllocator());
        }
        j_entry.AddMember("beams", j_beams, j_doc.GetAllocator());

        j_actors.PushBack(j_entry, j_doc.GetAllocator());
    }
    j_doc.AddMember("actors", j_actors, j_doc.GetAllocator());

    // Write to disk
    if (!App::GetContentManager()->SerializeAndWriteJson(filename, RGN_SAVEGAMES, j_doc))
    {
        // Error already logged
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("Error while saving scene"));
        return false;
    }

    if (filename != "autosave.sav")
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Scene saved"));
    }

    return true;
}

void ActorManager::RestoreSavedState(ActorPtr actor, rapidjson::Value const& j_entry)
{
    actor->m_spawn_rotation = j_entry["spawn_rotation"].GetFloat();
    actor->ar_state = static_cast<ActorState>(j_entry["sim_state"].GetInt());
    actor->ar_physics_paused = j_entry["physics_paused"].GetBool();

    if (j_entry["player_actor"].GetBool())
    {
        App::GetGameContext()->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, static_cast<void*>(new ActorPtr(actor))));
    }
    else if (j_entry["prev_player_actor"].GetBool())
    {
        App::GetGameContext()->SetPrevPlayerActor(actor);
    }

    if (actor->ar_engine)
    {
        int gear = j_entry["engine_gear"].GetInt();
        float rpm = j_entry["engine_rpm"].GetFloat();
        int automode = j_entry["engine_auto_mode"].GetInt();
        int autoselect = j_entry["engine_auto_select"].GetInt();
        bool running = j_entry["engine_is_running"].GetBool();
        bool contact = j_entry["engine_has_contact"].GetBool();
        if (running != actor->ar_engine->isRunning())
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
    actor->ar_trailer_parking_brake = j_entry["trailer_parking_brake"].GetBool();
    actor->ar_avg_wheel_speed = j_entry["avg_wheel_speed"].GetFloat();
    actor->ar_wheel_speed = j_entry["wheel_speed"].GetFloat();
    actor->ar_wheel_spin = j_entry["wheel_spin"].GetFloat();

    if (actor->m_custom_particles_enabled != j_entry["custom_particles"].GetBool())
    {
        actor->toggleCustomParticles();
    }

    // Flares
    actor->setHeadlightsVisible((bool)j_entry["lights"].GetInt()); // legacy name
    actor->setBlinkType(BlinkType(j_entry["blink_type"].GetInt()));
    // "beacon_light" was "pp_beacon_light" since release 2021.02 (savegame file format 2).
    // It was caused by find-&-replace derp in commit 5a159ad9c0d0ffb1fa3e6f4f9c4577fab3910e3e.
    actor->setBeaconMode(j_entry.HasMember("beacon_light") ? j_entry["beacon_light"].GetBool() : j_entry["pp_beacon_light"].GetBool());
    actor->setHighBeamsVisible(j_entry.HasMember("high_beams_on") ? j_entry["high_beams_on"].GetBool() : false); // (added to savegame file format 3)
    actor->setFogLightsVisible(j_entry.HasMember("fog_lights_on") ? j_entry["fog_lights_on"].GetBool() : false); // (added to savegame file format 3)

    // User-defined flares
    if (j_entry.HasMember("custom_lights"))
    {
        auto flares = j_entry["custom_lights"].GetArray();
        for (int i = 0; i < MAX_CLIGHTS; i++)
        {
            actor->setCustomLightVisible(i, flares[i].GetBool());
        }
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

    for (size_t i = 0; i < actor->ar_wheels.size(); i++)
    {
        if (actor->ar_wheels[i].wh_skidmark)
        {
            actor->ar_wheels[i].wh_skidmark->reset();
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
                actor->toggleTransferCaseGearRatio();
        }
    }

    auto commands = j_entry["commands"].GetArray();
    for (int i = 0; i < MAX_COMMANDS; i++)
    {
        auto& command_key = actor->ar_command_key[i];
        command_key.commandValue = commands[i][0].GetFloat();
        command_key.triggerInputValue = commands[i][1].GetFloat();
        auto command_beams = commands[i][2].GetArray();
        for (int j = 0; j < (int)command_key.beams.size(); j++)
        {
            command_key.beams[j].cmb_state->auto_moving_mode = command_beams[j][0].GetInt();
            command_key.beams[j].cmb_state->pressed_center_mode = command_beams[j][1].GetBool();
        }
    }

    auto nodes = j_entry["nodes"].GetArray();
    for (rapidjson::SizeType i = 0; i < nodes.Size(); i++)
    {
        auto data = nodes[i].GetArray();
        actor->ar_nodes[i].AbsPosition      = Vector3(data[0].GetFloat(), data[1].GetFloat(), data[2].GetFloat());
        actor->ar_nodes[i].RelPosition      = actor->ar_nodes[i].AbsPosition - actor->ar_origin;
        actor->ar_nodes[i].Velocity         = Vector3(data[3].GetFloat(), data[4].GetFloat(), data[5].GetFloat());
        actor->ar_nodes_aux[i].nda_initial_node_position = Vector3(data[6].GetFloat(), data[7].GetFloat(), data[8].GetFloat());
    }

    std::vector<ActorPtr> actors = this->GetLocalActors();

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
        if (locked_actor != -1 &&
            locked_actor < (int)actors.size() &&
            actors[locked_actor] != nullptr)
        {
            actor->AddInterActorBeam(&actor->ar_beams[i], actor, actors[locked_actor]);
        }
    }

    auto hooks = j_entry["hooks"].GetArray();
    for (int i = 0; i < actor->ar_hooks.size(); i++)
    {
        int lock_node = hooks[i]["lock_node"].GetInt();
        int locked_actor = hooks[i]["locked_actor"].GetInt();
        if (lock_node != -1 &&
            locked_actor != -1 &&
            locked_actor < (int)actors.size() &&
            actors[locked_actor] != nullptr)
        {
            actor->ar_hooks[i].hk_locked = HookState(hooks[i]["locked"].GetInt());
            actor->ar_hooks[i].hk_locked_actor = actors[locked_actor];
            actor->ar_hooks[i].hk_locked_node = static_cast<NodeNum_t>(lock_node);

            beam_t& hookbeam = actor->ar_beams[actor->ar_hooks[i].hk_beam];
            if (hookbeam.bm_inter_actor)
            {
                hookbeam.p2num = actor->ar_hooks[i].hk_locked_node;
            }
        }
    }

    auto ropes = j_entry["ropes"].GetArray();
    for (int i = 0; i < actor->ar_ropes.size(); i++)
    {
        int ropable = ropes[i]["locked_ropable"].GetInt();
        int locked_actor = ropes[i]["locked_actor"].GetInt();
        if (ropable != -1 &&
            locked_actor != -1 &&
            locked_actor < (int)actors.size() &&
            actors[locked_actor] != nullptr)
        {
            actor->ar_ropes[i].rp_locked = static_cast<HookState>(ropes[i]["locked"].GetInt());
            actor->ar_ropes[i].rp_locked_actor = actors[locked_actor];
            actor->ar_ropes[i].rp_locked_ropable_id = static_cast<RopableID_t>(ropable);
        }
    }

    auto ties = j_entry["ties"].GetArray();
    for (int i = 0; i < actor->ar_ties.size(); i++)
    {
        int ropable = ties[i]["locked_ropable"].GetInt();
        int locked_actor = ties[i]["locked_actor"].GetInt();
        if (ropable != -1 &&
            locked_actor != -1 &&
            locked_actor < (int)actors.size() &&
            actors[locked_actor] != nullptr)
        {
            actor->ar_ties[i].ti_tied  = ties[i]["tied"].GetBool();
            actor->ar_ties[i].ti_tying = ties[i]["tying"].GetBool();
            actor->ar_ties[i].ti_locked_actor = actors[locked_actor];
            actor->ar_ties[i].ti_locked_ropable_id = static_cast<RopableID_t>(ropable);

            beam_t& tiebeam = actor->ar_beams[actor->ar_ties[i].ti_beamid];
            if (tiebeam.bm_inter_actor)
            {
                ActorPtr tied_actor = actor->ar_ties[i].ti_locked_actor;
                const RopableID_t tied_ropableid = actor->ar_ties[i].ti_locked_ropable_id;
                tiebeam.p2num = tied_actor->ar_ropables[tied_ropableid].rb_nodenum;
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
        actor->toggleSlideNodeLock(); // OK to be invoked here - RestoreSavedState() - processing MSG_SIM_MODIFY_ACTOR_REQUESTED
    }

    actor->UpdateBoundingBoxes();
    actor->calculateAveragePosition();
    actor->m_avg_node_position_prev = actor->m_avg_node_position;
}
