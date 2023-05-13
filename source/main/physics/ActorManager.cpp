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

/// @file
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   24th of August 2009

#include "ActorManager.h"

#include "Application.h"
#include "Actor.h"
#include "CacheSystem.h"
#include "ContentManager.h"
#include "ChatSystem.h"
#include "Collisions.h"
#include "DashBoardManager.h"
#include "DynamicCollisions.h"
#include "EngineSim.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "Console.h"
#include "GUI_TopMenubar.h"
#include "InputEngine.h"
#include "Language.h"
#include "MovableText.h"
#include "Network.h"
#include "PointColDetector.h"
#include "Replay.h"
#include "RigDef_Validator.h"
#include "ActorSpawner.h"
#include "ScriptEngine.h"
#include "SoundScriptManager.h"
#include "Terrain.h"
#include "ThreadPool.h"
#include "Utils.h"
#include "VehicleAI.h"

using namespace Ogre;
using namespace RoR;

static ActorInstanceID_t m_actor_counter = 0;

ActorManager::ActorManager()
    : m_dt_remainder(0.0f)
    , m_forced_awake(false)
    , m_physics_steps(2000)
    , m_simulation_speed(1.0f)
{
    // Create worker thread (used for physics calculations)
    m_sim_thread_pool = std::unique_ptr<ThreadPool>(new ThreadPool(1));
}

ActorManager::~ActorManager()
{
    this->SyncWithSimThread(); // Wait for sim task to finish
}

ActorPtr ActorManager::CreateNewActor(ActorSpawnRequest rq, RigDef::DocumentPtr def)
{
    ActorPtr actor = new Actor(m_actor_counter++, static_cast<int>(m_actors.size()), def, rq);

    if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED && rq.asr_origin != ActorSpawnRequest::Origin::NETWORK)
    {
        actor->sendStreamSetup();
    }

    LOG(" == Spawning vehicle: " + def->name);

    ActorSpawner spawner;
    spawner.ConfigureSections(actor->m_section_config, def);
    spawner.ProcessNewActor(actor, rq, def);

    if (App::diag_actor_dump->getBool())
    {
        actor->WriteDiagnosticDump(actor->ar_filename + "_dump_raw.txt"); // Saves file to 'logs'
    }

    /* POST-PROCESSING */

    actor->ar_initial_node_positions.resize(actor->ar_num_nodes);
    actor->ar_initial_beam_defaults.resize(actor->ar_num_beams);
    actor->ar_initial_node_masses.resize(actor->ar_num_nodes);

    actor->UpdateBoundingBoxes(); // (records the unrotated dimensions for 'veh_aab_size')

    if (App::mp_state->getEnum<MpState>() == RoR::MpState::CONNECTED)
    {
        // Calculate optimal node position compression (for network transfer)
        Vector3 aabb_size = actor->ar_bounding_box.getSize();
        float max_dimension = std::max(1.0f, aabb_size.x);
        max_dimension = std::max(max_dimension, aabb_size.y);
        max_dimension = std::max(max_dimension, aabb_size.z);
        actor->m_net_node_compression = std::numeric_limits<short int>::max() / std::ceil(max_dimension * 1.5f);
    }
    // Apply spawn position & spawn rotation
    for (int i = 0; i < actor->ar_num_nodes; i++)
    {
        actor->ar_nodes[i].AbsPosition = rq.asr_position + rq.asr_rotation * (actor->ar_nodes[i].AbsPosition - rq.asr_position);
        actor->ar_nodes[i].RelPosition = actor->ar_nodes[i].AbsPosition - actor->ar_origin;
    };

    /* Place correctly */
    if (spawner.GetMemoryRequirements().num_fixes == 0)
    {
        Ogre::Vector3 vehicle_position = rq.asr_position;

        // check if over-sized
        actor->UpdateBoundingBoxes();
        vehicle_position.x += vehicle_position.x - actor->ar_bounding_box.getCenter().x;
        vehicle_position.z += vehicle_position.z - actor->ar_bounding_box.getCenter().z;

        float miny = 0.0f;

        if (!actor->m_preloaded_with_terrain)
        {
            miny = vehicle_position.y;
        }

        if (rq.asr_spawnbox != nullptr)
        {
            miny = rq.asr_spawnbox->relo.y + rq.asr_spawnbox->center.y;
        }

        if (rq.asr_free_position)
            actor->resetPosition(vehicle_position, true);
        else
            actor->resetPosition(vehicle_position.x, vehicle_position.z, true, miny);

        if (rq.asr_spawnbox != nullptr)
        {
            bool inside = true;

            for (int i = 0; i < actor->ar_num_nodes; i++)
                inside = (inside && App::GetGameContext()->GetTerrain()->GetCollisions()->isInside(actor->ar_nodes[i].AbsPosition, rq.asr_spawnbox, 0.2f));

            if (!inside)
            {
                Vector3 gpos = Vector3(vehicle_position.x, 0.0f, vehicle_position.z);

                gpos -= rq.asr_rotation * Vector3((rq.asr_spawnbox->hi.x - rq.asr_spawnbox->lo.x + actor->ar_bounding_box.getMaximum().x - actor->ar_bounding_box.getMinimum().x) * 0.6f, 0.0f, 0.0f);

                actor->resetPosition(gpos.x, gpos.z, true, miny);
            }
        }
    }
    else
    {
        actor->resetPosition(rq.asr_position, true);
    }
    actor->UpdateBoundingBoxes();

    //compute final mass
    actor->RecalculateNodeMasses(actor->m_dry_mass);
    actor->ar_initial_total_mass = actor->m_total_mass;
    for (int i = 0; i < actor->ar_num_nodes; i++)
    {
        actor->ar_initial_node_masses[i] = actor->ar_nodes[i].mass;
    }

    //setup default sounds
    if (!actor->m_disable_default_sounds)
    {
        ActorSpawner::SetupDefaultSoundSources(actor);
    }

    //compute node connectivity graph
    actor->calcNodeConnectivityGraph();

    actor->UpdateBoundingBoxes();
    actor->calculateAveragePosition();

    // calculate minimum camera radius
    actor->calculateAveragePosition();
    for (int i = 0; i < actor->ar_num_nodes; i++)
    {
        Real dist = actor->ar_nodes[i].AbsPosition.squaredDistance(actor->m_avg_node_position);
        if (dist > actor->m_min_camera_radius)
        {
            actor->m_min_camera_radius = dist;
        }
    }
    actor->m_min_camera_radius = std::sqrt(actor->m_min_camera_radius) * 1.2f; // twenty percent buffer

    // fix up submesh collision model
    std::string subMeshGroundModelName = spawner.GetSubmeshGroundmodelName();
    if (!subMeshGroundModelName.empty())
    {
        actor->ar_submesh_ground_model = App::GetGameContext()->GetTerrain()->GetCollisions()->getGroundModelByString(subMeshGroundModelName);
        if (!actor->ar_submesh_ground_model)
        {
            actor->ar_submesh_ground_model = App::GetGameContext()->GetTerrain()->GetCollisions()->defaultgm;
        }
    }

    // Set beam defaults
    for (int i = 0; i < actor->ar_num_beams; i++)
    {
        actor->ar_beams[i].initial_beam_strength       = actor->ar_beams[i].strength;
        actor->ar_beams[i].default_beam_deform         = actor->ar_beams[i].minmaxposnegstress;
        actor->ar_initial_beam_defaults[i]             = std::make_pair(actor->ar_beams[i].k, actor->ar_beams[i].d);
    }

    actor->m_spawn_rotation = actor->getRotation();

    TRIGGER_EVENT_ASYNC(SE_GENERIC_NEW_TRUCK, actor->ar_instance_id);

    actor->NotifyActorCameraChanged(); // setup sounds properly

    // calculate the number of wheel nodes
    actor->m_wheel_node_count = 0;
    for (int i = 0; i < actor->ar_num_nodes; i++)
    {
        if (actor->ar_nodes[i].nd_tyre_node)
            actor->m_wheel_node_count++;
    }

    // search m_net_first_wheel_node
    actor->m_net_first_wheel_node = actor->ar_num_nodes;
    for (int i = 0; i < actor->ar_num_nodes; i++)
    {
        if (actor->ar_nodes[i].nd_tyre_node || actor->ar_nodes[i].nd_rim_node)
        {
            actor->m_net_first_wheel_node = i;
            break;
        }
    }

    // Initialize visuals
    actor->updateVisual();
    actor->GetGfxActor()->SetDebugView((DebugViewType)rq.asr_debugview);

    // perform full visual update only if the vehicle won't be immediately driven by player.
    if (actor->isPreloadedWithTerrain() ||                         // .tobj file - Spawned sleeping somewhere on terrain
        rq.asr_origin == ActorSpawnRequest::Origin::CONFIG_FILE || // RoR.cfg or commandline - not entered by default
        actor->ar_num_cinecams == 0)                               // Not intended for player-controlling
    {
        actor->GetGfxActor()->UpdateSimDataBuffer(); // Initial fill of sim data buffers

        actor->GetGfxActor()->UpdateFlexbodies(); // Push tasks to threadpool
        actor->GetGfxActor()->UpdateWheelVisuals(); // Push tasks to threadpool
        actor->GetGfxActor()->UpdateCabMesh();
        actor->GetGfxActor()->UpdateWingMeshes();
        actor->GetGfxActor()->UpdateProps(0.f, false);
        actor->GetGfxActor()->UpdateRods(); // beam visuals
        actor->GetGfxActor()->FinishWheelUpdates(); // Sync tasks from threadpool
        actor->GetGfxActor()->FinishFlexbodyTasks(); // Sync tasks from threadpool
    }

    App::GetGfxScene()->RegisterGfxActor(actor->GetGfxActor());

    if (actor->ar_engine)
    {
        if (!actor->m_preloaded_with_terrain && App::sim_spawn_running->getBool())
            actor->ar_engine->StartEngine();
        else
            actor->ar_engine->OffStart();
    }
    // pressurize tires
    if (actor->getTyrePressure().IsEnabled())
    {
        actor->getTyrePressure().ModifyTyrePressure(0.f); // Initialize springiness of pressure-beams.
    }

    actor->ar_state = ActorState::LOCAL_SLEEPING;

    if (App::mp_state->getEnum<MpState>() == RoR::MpState::CONNECTED)
    {
        // network buffer layout (without RoRnet::VehicleState):
        // -----------------------------------------------------

        //  - 3 floats (x,y,z) for the reference node 0
        //  - ar_num_nodes - 1 times 3 short ints (compressed position info)
        actor->m_net_node_buf_size = sizeof(float) * 3 + (actor->m_net_first_wheel_node - 1) * sizeof(short int) * 3;
        actor->m_net_total_buffer_size += actor->m_net_node_buf_size;
        //  - ar_num_wheels times a float for the wheel rotation
        actor->m_net_wheel_buf_size = actor->ar_num_wheels * sizeof(float);
        actor->m_net_total_buffer_size += actor->m_net_wheel_buf_size;
        //  - bit array (made of ints) for the prop animation key states
        actor->m_net_propanimkey_buf_size = 
            (actor->m_prop_anim_key_states.size() / 8) + // whole chars
            (size_t)(actor->m_prop_anim_key_states.size() % 8 != 0); // remainder: 0 or 1 chars
        actor->m_net_total_buffer_size += actor->m_net_propanimkey_buf_size;

        if (rq.asr_origin == ActorSpawnRequest::Origin::NETWORK)
        {
            actor->ar_state = ActorState::NETWORKED_OK;
            if (actor->ar_engine)
            {
                actor->ar_engine->StartEngine();
            }
        }

        actor->m_net_username = rq.asr_net_username;
        actor->m_net_color_num = rq.asr_net_color;
    }
    else if (App::sim_replay_enabled->getBool())
    {
        actor->m_replay_handler = new Replay(actor, App::sim_replay_length->getInt());
    }

    // Launch scripts (FIXME: ignores sectionconfig)
    for (RigDef::Script const& script_def : def->root_module->scripts)
    {
        App::GetScriptEngine()->loadScript(script_def.filename, ScriptCategory::ACTOR, actor);
    }

    LOG(" ===== DONE LOADING VEHICLE");

    if (App::diag_actor_dump->getBool())
    {
        actor->WriteDiagnosticDump(actor->ar_filename + "_dump_recalc.txt"); // Saves file to 'logs'
    }

    m_actors.push_back(ActorPtr(actor));

    return actor;
}

void ActorManager::RemoveStreamSource(int sourceid)
{
    m_stream_mismatches.erase(sourceid);

    for (ActorPtr& actor : m_actors)
    {
        if (actor->ar_state != ActorState::NETWORKED_OK)
            continue;

        if (actor->ar_net_source_id == sourceid)
        {
            App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, static_cast<void*>(new ActorPtr(actor))));
        }
    }
}

#ifdef USE_SOCKETW
void ActorManager::HandleActorStreamData(std::vector<RoR::NetRecvPacket> packet_buffer)
{
    // Sort by stream source
    std::stable_sort(packet_buffer.begin(), packet_buffer.end(),
            [](const RoR::NetRecvPacket& a, const RoR::NetRecvPacket& b)
            { return a.header.source > b.header.source; });
    // Compress data stream by eliminating all but the last update from every consecutive group of stream data updates
    auto it = std::unique(packet_buffer.rbegin(), packet_buffer.rend(),
            [](const RoR::NetRecvPacket& a, const RoR::NetRecvPacket& b)
            { return !memcmp(&a.header, &b.header, sizeof(RoRnet::Header)) &&
            a.header.command == RoRnet::MSG2_STREAM_DATA; });
    packet_buffer.erase(packet_buffer.begin(), it.base());
    for (auto& packet : packet_buffer)
    {
        if (packet.header.command == RoRnet::MSG2_STREAM_REGISTER)
        {
            RoRnet::StreamRegister* reg = (RoRnet::StreamRegister *)packet.buffer;
            if (reg->type == 0)
            {
                reg->name[127] = 0;
                std::string filename = SanitizeUtf8CString(reg->name);

                RoRnet::UserInfo info;
                if (!App::GetNetwork()->GetUserInfo(reg->origin_sourceid, info))
                {
                    RoR::LogFormat("[RoR] Invalid STREAM_REGISTER, user id %d does not exist", reg->origin_sourceid);
                    reg->status = -1;
                }
                else if (filename.empty())
                {
                    RoR::LogFormat("[RoR] Invalid STREAM_REGISTER (user '%s', ID %d), filename is empty string", info.username, reg->origin_sourceid);
                    reg->status = -1;
                }
                else
                {
                    Str<200> text;
                    text << _L("spawned a new vehicle: ") << filename;
                    App::GetConsole()->putNetMessage(
                        reg->origin_sourceid, Console::CONSOLE_SYSTEM_NOTICE, text.ToCStr());

                    LOG("[RoR] Creating remote actor for " + TOSTRING(reg->origin_sourceid) + ":" + TOSTRING(reg->origin_streamid));

                    if (!App::GetCacheSystem()->CheckResourceLoaded(filename))
                    {
                        App::GetConsole()->putMessage(
                            Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
                            _L("Mod not installed: ") + filename);
                        RoR::LogFormat("[RoR] Cannot create remote actor (not installed), filename: '%s'", filename.c_str());
                        AddStreamMismatch(reg->origin_sourceid, reg->origin_streamid);
                        reg->status = -1;
                    }
                    else
                    {
                        auto actor_reg = reinterpret_cast<RoRnet::ActorStreamRegister*>(reg);
                        if (m_stream_time_offsets.find(reg->origin_sourceid) == m_stream_time_offsets.end())
                        {
                            int offset = actor_reg->time - m_net_timer.getMilliseconds();
                            m_stream_time_offsets[reg->origin_sourceid] = offset - 100;
                        }
                        ActorSpawnRequest* rq = new ActorSpawnRequest;
                        rq->asr_origin = ActorSpawnRequest::Origin::NETWORK;
                        // TODO: Look up cache entry early (eliminate asr_filename) and fetch skin by name+guid! ~ 03/2019
                        rq->asr_filename = filename;
                        if (strnlen(actor_reg->skin, 60) < 60 && actor_reg->skin[0] != '\0')
                        {
                            rq->asr_skin_entry = App::GetCacheSystem()->FetchSkinByName(actor_reg->skin);
                        }
                        if (strnlen(actor_reg->sectionconfig, 60) < 60)
                        {
                            rq->asr_config = actor_reg->sectionconfig;
                        }
                        rq->asr_net_username = tryConvertUTF(info.username);
                        rq->asr_net_color    = info.colournum;
                        rq->net_source_id    = reg->origin_sourceid;
                        rq->net_stream_id    = reg->origin_streamid;

                        App::GetGameContext()->PushMessage(Message(
                            MSG_SIM_SPAWN_ACTOR_REQUESTED, (void*)rq));

                        reg->status = 1;
                    }
                }

                App::GetNetwork()->AddPacket(reg->origin_streamid, RoRnet::MSG2_STREAM_REGISTER_RESULT, sizeof(RoRnet::StreamRegister), (char *)reg);
            }
        }
        else if (packet.header.command == RoRnet::MSG2_STREAM_REGISTER_RESULT)
        {
            RoRnet::StreamRegister* reg = (RoRnet::StreamRegister *)packet.buffer;
            for (ActorPtr& actor: m_actors)
            {
                if (actor->ar_net_source_id == reg->origin_sourceid && actor->ar_net_stream_id == reg->origin_streamid)
                {
                    int sourceid = packet.header.source;
                    actor->ar_net_stream_results[sourceid] = reg->status;

                    String message = "";
                    switch (reg->status)
                    {
                        case  1: message = "successfully loaded stream"; break;
                        case -2: message = "detected mismatch stream"; break;
                        default: message = "could not load stream"; break;
                    }
                    LOG("Client " + TOSTRING(sourceid) + " " + message + " " + TOSTRING(reg->origin_streamid) +
                            " with name '" + reg->name + "', result code: " + TOSTRING(reg->status));
                    break;
                }
            }
        }
        else if (packet.header.command == RoRnet::MSG2_STREAM_UNREGISTER)
        {
            ActorPtr b = this->GetActorByNetworkLinks(packet.header.source, packet.header.streamid);
            if (b)
            {
                if (b->ar_state == ActorState::NETWORKED_OK || b->ar_state == ActorState::NETWORKED_HIDDEN)
                {
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, static_cast<void*>(new ActorPtr(b))));
                }
            }
            m_stream_mismatches[packet.header.source].erase(packet.header.streamid);
        }
        else if (packet.header.command == RoRnet::MSG2_USER_LEAVE)
        {
            this->RemoveStreamSource(packet.header.source);
        }
        else if (packet.header.command == RoRnet::MSG2_STREAM_DATA)
        {
            for (ActorPtr& actor: m_actors)
            {
                if (actor->ar_state != ActorState::NETWORKED_OK)
                    continue;
                if (packet.header.source == actor->ar_net_source_id && packet.header.streamid == actor->ar_net_stream_id)
                {
                    actor->pushNetwork(packet.buffer, packet.header.size);
                    break;
                }
            }
        }
    }
}
#endif // USE_SOCKETW

int ActorManager::GetNetTimeOffset(int sourceid)
{
    auto search = m_stream_time_offsets.find(sourceid);
    if (search != m_stream_time_offsets.end())
    {
        return search->second;
    }
    return 0;
}

void ActorManager::UpdateNetTimeOffset(int sourceid, int offset)
{
    if (m_stream_time_offsets.find(sourceid) != m_stream_time_offsets.end())
    {
        m_stream_time_offsets[sourceid] += offset;
    }
}

int ActorManager::CheckNetworkStreamsOk(int sourceid)
{
    if (!m_stream_mismatches[sourceid].empty())
        return 0;

    for (ActorPtr& actor: m_actors)
    {
        if (actor->ar_state != ActorState::NETWORKED_OK)
            continue;

        if (actor->ar_net_source_id == sourceid)
        {
            return 1;
        }
    }

    return 2;
}

int ActorManager::CheckNetRemoteStreamsOk(int sourceid)
{
    int result = 2;

    for (ActorPtr& actor: m_actors)
    {
        if (actor->ar_state == ActorState::NETWORKED_OK)
            continue;

        int stream_result = actor->ar_net_stream_results[sourceid];
        if (stream_result == -1 || stream_result == -2)
            return 0;
        if (stream_result == 1)
            result = 1;
    }

    return result;
}

ActorPtr ActorManager::GetActorByNetworkLinks(int source_id, int stream_id)
{
    for (ActorPtr& actor: m_actors)
    {
        if (actor->ar_net_source_id == source_id && actor->ar_net_stream_id == stream_id)
        {
            return actor;
        }
    }

    return nullptr;
}

bool ActorManager::CheckActorCollAabbIntersect(int a, int b)
{
    if (m_actors[a]->ar_collision_bounding_boxes.empty() && m_actors[b]->ar_collision_bounding_boxes.empty())
    {
        return m_actors[a]->ar_bounding_box.intersects(m_actors[b]->ar_bounding_box);
    }
    else if (m_actors[a]->ar_collision_bounding_boxes.empty())
    {
        for (const auto& bbox_b : m_actors[b]->ar_collision_bounding_boxes)
            if (bbox_b.intersects(m_actors[a]->ar_bounding_box))
                return true;
    }
    else if (m_actors[b]->ar_collision_bounding_boxes.empty())
    {
        for (const auto& bbox_a : m_actors[a]->ar_collision_bounding_boxes)
            if (bbox_a.intersects(m_actors[b]->ar_bounding_box))
                return true;
    }
    else
    {
        for (const auto& bbox_a : m_actors[a]->ar_collision_bounding_boxes)
            for (const auto& bbox_b : m_actors[b]->ar_collision_bounding_boxes)
                if (bbox_a.intersects(bbox_b))
                    return true;
    }

    return false;
}

bool ActorManager::PredictActorCollAabbIntersect(int a, int b)
{
    if (m_actors[a]->ar_predicted_coll_bounding_boxes.empty() && m_actors[b]->ar_predicted_coll_bounding_boxes.empty())
    {
        return m_actors[a]->ar_predicted_bounding_box.intersects(m_actors[b]->ar_predicted_bounding_box);
    }
    else if (m_actors[a]->ar_predicted_coll_bounding_boxes.empty())
    {
        for (const auto& bbox_b : m_actors[b]->ar_predicted_coll_bounding_boxes)
            if (bbox_b.intersects(m_actors[a]->ar_predicted_bounding_box))
                return true;
    }
    else if (m_actors[b]->ar_predicted_coll_bounding_boxes.empty())
    {
        for (const auto& bbox_a : m_actors[a]->ar_predicted_coll_bounding_boxes)
            if (bbox_a.intersects(m_actors[b]->ar_predicted_bounding_box))
                return true;
    }
    else
    {
        for (const auto& bbox_a : m_actors[a]->ar_predicted_coll_bounding_boxes)
            for (const auto& bbox_b : m_actors[b]->ar_predicted_coll_bounding_boxes)
                if (bbox_a.intersects(bbox_b))
                    return true;
    }

    return false;
}

void ActorManager::RecursiveActivation(int j, std::vector<bool>& visited)
{
    if (visited[j] || m_actors[j]->ar_state != ActorState::LOCAL_SIMULATED)
        return;

    visited[j] = true;

    for (unsigned int t = 0; t < m_actors.size(); t++)
    {
        if (t == j || visited[t])
            continue;
        if (m_actors[t]->ar_state == ActorState::LOCAL_SIMULATED && CheckActorCollAabbIntersect(t, j))
        {
            m_actors[t]->ar_sleep_counter = 0.0f;
            this->RecursiveActivation(t, visited);
        }
        if (m_actors[t]->ar_state == ActorState::LOCAL_SLEEPING && PredictActorCollAabbIntersect(t, j))
        {
            m_actors[t]->ar_sleep_counter = 0.0f;
            m_actors[t]->ar_state = ActorState::LOCAL_SIMULATED;
            this->RecursiveActivation(t, visited);
        }
    }
}

void ActorManager::ForwardCommands(ActorPtr source_actor)
{
    if (source_actor->ar_forward_commands)
    {
        auto linked_actors = source_actor->getAllLinkedActors();

        for (ActorPtr& actor : this->GetActors())
        {
            if (actor != source_actor && actor->ar_import_commands &&
                    (actor->getPosition().distance(source_actor->getPosition()) < 
                     actor->m_min_camera_radius + source_actor->m_min_camera_radius))
            {
                // activate the truck
                if (actor->ar_state == ActorState::LOCAL_SLEEPING)
                {
                    actor->ar_sleep_counter = 0.0f;
                    actor->ar_state = ActorState::LOCAL_SIMULATED;
                }

                if (App::sim_realistic_commands->getBool())
                {
                    if (std::find(linked_actors.begin(), linked_actors.end(), actor) == linked_actors.end())
                        continue;
                }

                // forward commands
                for (int j = 1; j <= MAX_COMMANDS; j++)
                {
                    actor->ar_command_key[j].playerInputValue = std::max(source_actor->ar_command_key[j].playerInputValue,
                                                                         source_actor->ar_command_key[j].commandValue);
                }
                if (source_actor->ar_toggle_ties)
                {
                    actor->tieToggle();
                }
                if (source_actor->ar_toggle_ropes)
                {
                    actor->ropeToggle(-1);
                }
            }
        }
        // just send brake and lights to the connected trucks, and no one else :)
        for (auto hook : source_actor->ar_hooks)
        {
            if (!hook.hk_locked_actor || hook.hk_locked_actor == source_actor)
                continue;

            // forward brakes
            hook.hk_locked_actor->ar_brake = source_actor->ar_brake;
            if (hook.hk_locked_actor->ar_parking_brake != source_actor->ar_trailer_parking_brake)
            {
                hook.hk_locked_actor->parkingbrakeToggle();
            }

            // forward lights
            hook.hk_locked_actor->setLightStateMask(source_actor->getLightStateMask());
        }
    }
}

void ActorManager::UpdateSleepingState(ActorPtr player_actor, float dt)
{
    if (!m_forced_awake)
    {
        for (ActorPtr& actor: m_actors)
        {
            if (actor->ar_state != ActorState::LOCAL_SIMULATED)
                continue;
            if (actor->ar_driveable == AI)
                continue;
            if (actor->getVelocity().squaredLength() > 0.01f)
            {
                actor->ar_sleep_counter = 0.0f;
                continue;
            }

            actor->ar_sleep_counter += dt;

            if (actor->ar_sleep_counter >= 10.0f)
            {
                actor->ar_state = ActorState::LOCAL_SLEEPING;
            }
        }
    }

    if (player_actor && player_actor->ar_state == ActorState::LOCAL_SLEEPING)
    {
        player_actor->ar_state = ActorState::LOCAL_SIMULATED;
    }

    std::vector<bool> visited(m_actors.size());
    // Recursivly activate all actors which can be reached from current actor
    if (player_actor && player_actor->ar_state == ActorState::LOCAL_SIMULATED)
    {
        player_actor->ar_sleep_counter = 0.0f;
        this->RecursiveActivation(player_actor->ar_vector_index, visited);
    }
    // Snowball effect (activate all actors which might soon get hit by a moving actor)
    for (unsigned int t = 0; t < m_actors.size(); t++)
    {
        if (m_actors[t]->ar_state == ActorState::LOCAL_SIMULATED && m_actors[t]->ar_sleep_counter == 0.0f)
            this->RecursiveActivation(t, visited);
    }
}

void ActorManager::WakeUpAllActors()
{
    for (ActorPtr& actor: m_actors)
    {
        if (actor->ar_state == ActorState::LOCAL_SLEEPING)
        {
            actor->ar_state = ActorState::LOCAL_SIMULATED;
            actor->ar_sleep_counter = 0.0f;
        }
    }
}

void ActorManager::SendAllActorsSleeping()
{
    m_forced_awake = false;
    for (ActorPtr& actor: m_actors)
    {
        if (actor->ar_state == ActorState::LOCAL_SIMULATED)
        {
            actor->ar_state = ActorState::LOCAL_SLEEPING;
        }
    }
}

ActorPtr ActorManager::FindActorInsideBox(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box)
{
    // try to find the desired actor (the one in the box)
    ActorPtr ret = nullptr;
    for (ActorPtr& actor: m_actors)
    {
        if (collisions->isInside(actor->ar_nodes[0].AbsPosition, inst, box))
        {
            if (ret == nullptr)
            // first actor found
                ret = actor;
            else
            // second actor found -> unclear which one was meant
                return nullptr;
        }
    }
    return ret;
}

void ActorManager::RepairActor(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box, bool keepPosition)
{
    ActorPtr actor = this->FindActorInsideBox(collisions, inst, box);
    if (actor != nullptr)
    {
        SOUND_PLAY_ONCE(actor, SS_TRIG_REPAIR);

        ActorModifyRequest* rq = new ActorModifyRequest;
        rq->amr_actor = actor->ar_instance_id;
        rq->amr_type = ActorModifyRequest::Type::RESET_ON_SPOT;
        App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
    }
}

void ActorManager::MuteAllActors()
{
    for (ActorPtr& actor: m_actors)
    {
        actor->muteAllSounds();
    }
}

void ActorManager::UnmuteAllActors()
{
    for (ActorPtr& actor: m_actors)
    {
        actor->unmuteAllSounds();
    }
}

std::pair<ActorPtr, float> ActorManager::GetNearestActor(Vector3 position)
{
    ActorPtr nearest_actor = nullptr;
    float min_squared_distance = std::numeric_limits<float>::max();
    for (ActorPtr& actor : m_actors)
    {
        float squared_distance = position.squaredDistance(actor->ar_nodes[0].AbsPosition);
        if (squared_distance < min_squared_distance)
        {
            min_squared_distance = squared_distance;
            nearest_actor = actor;
        }
    }
    return std::make_pair(nearest_actor, std::sqrt(min_squared_distance));
}

void ActorManager::CleanUpSimulation() // Called after simulation finishes
{
    while (m_actors.size() > 0)
    {
        this->DeleteActorInternal(m_actors.back());
    }

    m_total_sim_time = 0.f;
    m_last_simulation_speed = 0.1f;
    m_simulation_paused = false;
    m_simulation_speed = 1.f;
}

void ActorManager::DeleteActorInternal(ActorPtr actor)
{
    if (actor == nullptr)
        return;

    this->SyncWithSimThread();

#ifdef USE_SOCKETW
    if (App::mp_state->getEnum<MpState>() == RoR::MpState::CONNECTED)
    {
        if (actor->ar_state != ActorState::NETWORKED_OK)
        {
            App::GetNetwork()->AddPacket(actor->ar_net_stream_id, RoRnet::MSG2_STREAM_UNREGISTER, 0, 0);
        }
        else if (std::count_if(m_actors.begin(), m_actors.end(), [actor](ActorPtr& b)
                    { return b->ar_net_source_id == actor->ar_net_source_id; }) == 1)
        {
            // We're deleting the last actor from this stream source, reset the stream time offset
            m_stream_time_offsets.erase(actor->ar_net_source_id);
        }
    }
#endif // USE_SOCKETW

    // Only dispose(), do not `delete`; a script may still hold pointer to the object.
    actor->dispose();

    EraseIf(m_actors, [actor](ActorPtr& curActor) { return actor == curActor; });

    // Upate actor indices
    for (unsigned int i = 0; i < m_actors.size(); i++)
        m_actors[i]->ar_vector_index = i;
}

int FindPivotActorId(ActorPtr player, ActorPtr prev_player)
{
    if (player != nullptr)
        return player->ar_vector_index;
    else if (prev_player != nullptr)
        return prev_player->ar_vector_index + 1;
    return -1;
}

ActorPtr ActorManager::FetchNextVehicleOnList(ActorPtr player, ActorPtr prev_player)
{
    int pivot_index = FindPivotActorId(player, prev_player);

    for (int i = pivot_index + 1; i < m_actors.size(); i++)
    {
        if (m_actors[i]->ar_state != ActorState::NETWORKED_OK && !m_actors[i]->isPreloadedWithTerrain())
        {
            return m_actors[i].GetRef();
        }
    }

    for (int i = 0; i < pivot_index; i++)
    {
        if (m_actors[i]->ar_state != ActorState::NETWORKED_OK && !m_actors[i]->isPreloadedWithTerrain())
        {
            return m_actors[i].GetRef();
        }
    }

    if (pivot_index >= 0 && m_actors[pivot_index]->ar_state != ActorState::NETWORKED_OK && !m_actors[pivot_index]->isPreloadedWithTerrain())
    {
        return m_actors[pivot_index].GetRef();
    }

    return nullptr;
}

ActorPtr ActorManager::FetchPreviousVehicleOnList(ActorPtr player, ActorPtr prev_player)
{
    int pivot_index = FindPivotActorId(player, prev_player);

    for (int i = pivot_index - 1; i >= 0; i--)
    {
        if (m_actors[i]->ar_state != ActorState::NETWORKED_OK && !m_actors[i]->isPreloadedWithTerrain())
        {
            return m_actors[i].GetRef();
        }
    }

    for (int i = static_cast<int>(m_actors.size()) - 1; i > pivot_index; i--)
    {
        if (m_actors[i]->ar_state != ActorState::NETWORKED_OK && !m_actors[i]->isPreloadedWithTerrain())
        {
            return m_actors[i].GetRef();
        }
    }

    if (pivot_index >= 0 && m_actors[pivot_index]->ar_state != ActorState::NETWORKED_OK && !m_actors[pivot_index]->isPreloadedWithTerrain())
    {
        return m_actors[pivot_index].GetRef();
    }

    return nullptr;
}

ActorPtr ActorManager::FetchRescueVehicle()
{
    for (ActorPtr& actor: m_actors)
    {
        if (actor->ar_rescuer_flag)
        {
            return actor;
        }
    }
    return nullptr;
}

void ActorManager::UpdateActors(ActorPtr player_actor)
{
    float dt = m_simulation_time;

    // do not allow dt > 1/20
    dt = std::min(dt, 1.0f / 20.0f);

    dt *= m_simulation_speed;

    dt += m_dt_remainder;
    m_physics_steps = dt / PHYSICS_DT;
    if (m_physics_steps == 0)
    {
        return;
    }

    m_dt_remainder = dt - (m_physics_steps * PHYSICS_DT);
    dt = PHYSICS_DT * m_physics_steps;

    this->SyncWithSimThread();

    this->UpdateSleepingState(player_actor, dt);

    for (ActorPtr& actor: m_actors)
    {
        actor->HandleInputEvents(dt);
        actor->HandleAngelScriptEvents(dt);

#ifdef USE_ANGELSCRIPT
        if (actor->ar_vehicle_ai && actor->ar_vehicle_ai->isActive())
            actor->ar_vehicle_ai->update(dt, 0);
#endif // USE_ANGELSCRIPT

        if (actor->ar_engine)
        {
            if (actor->ar_driveable == TRUCK)
            {
                this->UpdateTruckFeatures(actor, dt);
            }
            if (actor->ar_state == ActorState::LOCAL_SLEEPING)
            {
                actor->ar_engine->UpdateEngineSim(dt, 1);
            }
            actor->ar_engine->UpdateEngineAudio();
        }

        // Always update indicator states - used by 'u' type flares.
        actor->updateDashBoards(dt);

        // Blinkers (turn signals) must always be updated
        actor->updateFlareStates(dt);

        if (actor->ar_state != ActorState::LOCAL_SLEEPING)
        {
            actor->updateVisual(dt);
            if (actor->ar_update_physics && App::gfx_skidmarks_mode->getInt() > 0)
            {
                actor->updateSkidmarks();
            }
        }
        if (App::mp_state->getEnum<MpState>() == RoR::MpState::CONNECTED)
        {
            // FIXME: Hidden actors must also be updated to workaround a glitch, see https://github.com/RigsOfRods/rigs-of-rods/issues/2911
            if (actor->ar_state == ActorState::NETWORKED_OK || actor->ar_state == ActorState::NETWORKED_HIDDEN)
                actor->calcNetwork();
            else
                actor->sendStreamData();
        }
    }

    if (player_actor != nullptr)
    {
        this->ForwardCommands(player_actor);
        if (player_actor->ar_toggle_ties)
        {
            player_actor->tieToggle();
            player_actor->ar_toggle_ties = false;
        }
        if (player_actor->ar_toggle_ropes)
        {
            player_actor->ropeToggle(-1);
            player_actor->ar_toggle_ropes = false;
        }

        player_actor->ForceFeedbackStep(m_physics_steps);

        if (player_actor->ar_state == ActorState::LOCAL_REPLAY)
        {
            player_actor->getReplay()->replayStepActor();
        }
    }

    auto func = std::function<void()>([this]()
        {
            this->UpdatePhysicsSimulation();
        });
    m_sim_task = m_sim_thread_pool->RunTask(func);

    m_total_sim_time += dt;

    if (!App::app_async_physics->getBool())
        m_sim_task->join();
}

ActorPtr ActorManager::GetActorById(ActorInstanceID_t actor_id)
{
    for (ActorPtr& actor: m_actors)
    {
        if (actor->ar_instance_id == actor_id)
        {
            return actor;
        }
    }
    return 0;
}

void ActorManager::UpdatePhysicsSimulation()
{
    for (ActorPtr& actor: m_actors)
    {
        actor->UpdatePhysicsOrigin();
    }
    for (int i = 0; i < m_physics_steps; i++)
    {
        {
            std::vector<std::function<void()>> tasks;
            for (ActorPtr& actor: m_actors)
            {
                if (actor->ar_update_physics = actor->CalcForcesEulerPrepare(i == 0))
                {
                    ActorPtr actor_raw = actor;
                    auto func = std::function<void()>([this, i, actor_raw]()
                        {
                            actor_raw->CalcForcesEulerCompute(i == 0, m_physics_steps);
                        });
                    tasks.push_back(func);
                }
            }
            App::GetThreadPool()->Parallelize(tasks);
            for (ActorPtr& actor: m_actors)
            {
                if (actor->ar_update_physics)
                {
                    actor->CalcBeamsInterActor();
                }
            }
        }
        {
            std::vector<std::function<void()>> tasks;
            for (ActorPtr& actor: m_actors)
            {
                if (actor->m_inter_point_col_detector != nullptr && (actor->ar_update_physics ||
                        (App::mp_pseudo_collisions->getBool() && actor->ar_state == ActorState::NETWORKED_OK)))
                {
                    ActorPtr actor_raw = actor;
                    auto func = std::function<void()>([this, actor_raw]()
                        {
                            actor_raw->m_inter_point_col_detector->UpdateInterPoint();
                            if (actor_raw->ar_collision_relevant)
                            {
                                ResolveInterActorCollisions(PHYSICS_DT,
                                   *actor_raw->m_inter_point_col_detector,
                                    actor_raw->ar_num_collcabs,
                                    actor_raw->ar_collcabs,
                                    actor_raw->ar_cabs,
                                    actor_raw->ar_inter_collcabrate,
                                    actor_raw->ar_nodes,
                                    actor_raw->ar_collision_range,
                                   *actor_raw->ar_submesh_ground_model);
                            }
                        });
                    tasks.push_back(func);
                }
            }
            App::GetThreadPool()->Parallelize(tasks);
        }
    }
    for (ActorPtr& actor: m_actors)
    {
        actor->m_ongoing_reset = false;
        if (actor->ar_update_physics && m_physics_steps > 0)
        {
            Vector3  camera_gforces = actor->m_camera_gforces_accu / m_physics_steps;
            actor->m_camera_gforces_accu = Vector3::ZERO;
            actor->m_camera_gforces = actor->m_camera_gforces * 0.5f + camera_gforces * 0.5f;
            actor->calculateLocalGForces();
            actor->calculateAveragePosition();
            actor->m_avg_node_velocity  = actor->m_avg_node_position - actor->m_avg_node_position_prev;
            actor->m_avg_node_velocity /= (m_physics_steps * PHYSICS_DT);
            actor->m_avg_node_position_prev = actor->m_avg_node_position;
            actor->ar_top_speed = std::max(actor->ar_top_speed, actor->ar_nodes[0].Velocity.length());
        }
    }
}

void ActorManager::SyncWithSimThread()
{
    if (m_sim_task)
        m_sim_task->join();
}

void HandleErrorLoadingFile(std::string type, std::string filename, std::string exception_msg)
{
    RoR::Str<200> msg;
    msg << "Failed to load '" << filename << "' (type: '" << type << "'), message: " << exception_msg;
    App::GetConsole()->putMessage(
        Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, msg.ToCStr(), "error.png");
}

void HandleErrorLoadingTruckfile(std::string filename, std::string exception_msg)
{
    HandleErrorLoadingFile("actor", filename, exception_msg);
}

RigDef::DocumentPtr ActorManager::FetchActorDef(std::string filename, bool predefined_on_terrain)
{
    // Find the user content
    CacheEntry* cache_entry = App::GetCacheSystem()->FindEntryByFilename(LT_AllBeam, /*partial=*/false, filename);
    if (cache_entry == nullptr)
    {
        HandleErrorLoadingTruckfile(filename, "Truckfile not found in ModCache (probably not installed)");
        return nullptr;
    }

    // If already parsed, re-use
    if (cache_entry->actor_def != nullptr)
    {
        return cache_entry->actor_def;
    }

    // Load the 'truckfile'
    try
    {
        Ogre::String resource_filename = filename;
        Ogre::String resource_groupname;
        if (!App::GetCacheSystem()->CheckResourceLoaded(resource_filename, resource_groupname)) // Validates the filename and finds resource group
        {
            HandleErrorLoadingTruckfile(filename, "Truckfile not found");
            return nullptr;
        }
        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(resource_filename, resource_groupname);

        if (stream.isNull() || !stream->isReadable())
        {
            HandleErrorLoadingTruckfile(filename, "Unable to open/read truckfile");
            return nullptr;
        }

        RoR::LogFormat("[RoR] Parsing truckfile '%s'", resource_filename.c_str());
        RigDef::Parser parser;
        parser.Prepare();
        parser.ProcessOgreStream(stream.getPointer(), resource_groupname);
        parser.Finalize();

        auto def = parser.GetFile();

        // VALIDATING
        LOG(" == Validating vehicle: " + def->name);

        RigDef::Validator validator;
        validator.Setup(def);

        if (predefined_on_terrain)
        {
            // Workaround: Some terrains pre-load truckfiles with special purpose:
            //     "soundloads" = play sound effect at certain spot
            //     "fixes"      = structures of N/B fixed to the ground
            // These files can have no beams. Possible extensions: .load or .fixed
            std::string file_extension = filename.substr(filename.find_last_of('.'));
            Ogre::StringUtil::toLowerCase(file_extension);
            if ((file_extension == ".load") | (file_extension == ".fixed"))
            {
                validator.SetCheckBeams(false);
            }
        }

        validator.Validate(); // Sends messages to console

        def->hash = Sha1Hash(stream->getAsString());

        cache_entry->actor_def = def;
        return def;
    }
    catch (Ogre::Exception& oex)
    {
        HandleErrorLoadingTruckfile(filename, oex.getFullDescription().c_str());
        return nullptr;
    }
    catch (std::exception& stex)
    {
        HandleErrorLoadingTruckfile(filename, stex.what());
        return nullptr;
    }
    catch (...)
    {
        HandleErrorLoadingTruckfile(filename, "<Unknown exception occurred>");
        return nullptr;
    }
}

std::vector<ActorPtr> ActorManager::GetLocalActors()
{
    std::vector<ActorPtr> actors;
    for (ActorPtr& actor: m_actors)
    {
        if (actor->ar_state != ActorState::NETWORKED_OK)
            actors.push_back(actor);
    }
    return actors;
}

void ActorManager::UpdateInputEvents(float dt)
{
    // Simulation pace adjustment (slowmotion)
    if (!App::GetGameContext()->GetRaceSystem().IsRaceInProgress())
    {
        // EV_COMMON_ACCELERATE_SIMULATION
        if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_ACCELERATE_SIMULATION))
        {
            float simulation_speed = this->GetSimulationSpeed() * pow(2.0f, dt / 2.0f);
            this->SetSimulationSpeed(simulation_speed);
            String ssmsg = _L("New simulation speed: ") + TOSTRING(Round(simulation_speed * 100.0f, 1)) + "%";
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg);
        }

        // EV_COMMON_DECELERATE_SIMULATION
        if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_DECELERATE_SIMULATION))
        {
            float simulation_speed = this->GetSimulationSpeed() * pow(0.5f, dt / 2.0f);
            this->SetSimulationSpeed(simulation_speed);
            String ssmsg = _L("New simulation speed: ") + TOSTRING(Round(simulation_speed * 100.0f, 1)) + "%";
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg);
        }

        // EV_COMMON_RESET_SIMULATION_PACE
        if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESET_SIMULATION_PACE))
        {
            float simulation_speed = this->GetSimulationSpeed();
            if (simulation_speed != 1.0f)
            {
                m_last_simulation_speed = simulation_speed;
                this->SetSimulationSpeed(1.0f);
                UTFString ssmsg = _L("Simulation speed reset.");
                App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg);
            }
            else if (m_last_simulation_speed != 1.0f)
            {
                this->SetSimulationSpeed(m_last_simulation_speed);
                String ssmsg = _L("New simulation speed: ") + TOSTRING(Round(m_last_simulation_speed * 100.0f, 1)) + "%";
                App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg);
            }
        }

        // Special adjustment while racing
        if (App::GetGameContext()->GetRaceSystem().IsRaceInProgress() && this->GetSimulationSpeed() != 1.0f)
        {
            m_last_simulation_speed = this->GetSimulationSpeed();
            this->SetSimulationSpeed(1.f);
        }
    }

    // EV_COMMON_TOGGLE_PHYSICS - Freeze/unfreeze physics
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_PHYSICS))
    {
        m_simulation_paused = !m_simulation_paused;

        if (m_simulation_paused)
        {
            String ssmsg = _L("Physics paused");
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg);
        }
        else
        {
            String ssmsg = _L("Physics unpaused");
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg);
        }
    }

    // Calculate simulation time
    if (m_simulation_paused)
    {
        m_simulation_time = 0.f;

        // Frozen physics stepping
        if (this->GetSimulationSpeed() > 0.0f)
        {
            // EV_COMMON_REPLAY_FAST_FORWARD - Advance simulation while pressed
            // EV_COMMON_REPLAY_FORWARD - Advanced simulation one step
            if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPLAY_FAST_FORWARD) ||
                App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FORWARD))
            {
                m_simulation_time = PHYSICS_DT / this->GetSimulationSpeed();
            }
        }
    }
    else
    {
        m_simulation_time = dt;
    }
}

void ActorManager::UpdateTruckFeatures(ActorPtr vehicle, float dt)
{
    if (vehicle->isBeingReset() || vehicle->ar_physics_paused)
        return;
#ifdef USE_ANGELSCRIPT
    if (vehicle->ar_vehicle_ai && vehicle->ar_vehicle_ai->isActive())
        return;
#endif // USE_ANGELSCRIPT

    EngineSim* engine = vehicle->ar_engine;

    if (engine && engine->hasContact() &&
        engine->GetAutoShiftMode() == SimGearboxMode::AUTO &&
        engine->getAutoShift() != EngineSim::NEUTRAL)
    {
        Ogre::Vector3 dirDiff = vehicle->getDirection();
        Ogre::Degree pitchAngle = Ogre::Radian(asin(dirDiff.dotProduct(Ogre::Vector3::UNIT_Y)));

        if (std::abs(pitchAngle.valueDegrees()) > 2.0f)
        {
            if (engine->getAutoShift() > EngineSim::NEUTRAL && vehicle->ar_avg_wheel_speed < +0.02f && pitchAngle.valueDegrees() > 0.0f ||
                engine->getAutoShift() < EngineSim::NEUTRAL && vehicle->ar_avg_wheel_speed > -0.02f && pitchAngle.valueDegrees() < 0.0f)
            {
                // anti roll back in SimGearboxMode::AUTO (DRIVE, TWO, ONE) mode
                // anti roll forth in SimGearboxMode::AUTO (REAR) mode
                float g = std::abs(App::GetGameContext()->GetTerrain()->getGravity());
                float downhill_force = std::abs(sin(pitchAngle.valueRadians()) * vehicle->getTotalMass()) * g;
                float engine_force = std::abs(engine->GetTorque()) / vehicle->getAvgPropedWheelRadius();
                float ratio = std::max(0.0f, 1.0f - (engine_force / downhill_force));
                if (vehicle->ar_avg_wheel_speed * pitchAngle.valueDegrees() > 0.0f)
                {
                    ratio *= sqrt((0.02f - vehicle->ar_avg_wheel_speed) / 0.02f);
                }
                vehicle->ar_brake = sqrt(ratio);
            }
        }
        else if (vehicle->ar_brake == 0.0f && !vehicle->ar_parking_brake && engine->GetTorque() == 0.0f)
        {
            float ratio = std::max(0.0f, 0.2f - std::abs(vehicle->ar_avg_wheel_speed)) / 0.2f;
            vehicle->ar_brake = ratio;
        }
    }

    if (vehicle->cc_mode)
    {
        vehicle->UpdateCruiseControl(dt);
    }
    if (vehicle->sl_enabled)
    {
        // check speed limit
        if (engine && engine->GetGear() != 0)
        {
            float accl = (vehicle->sl_speed_limit - std::abs(vehicle->ar_wheel_speed / 1.02f)) * 2.0f;
            engine->SetAcceleration(Ogre::Math::Clamp(accl, 0.0f, engine->GetAcceleration()));
        }
    }

    BITMASK_SET(vehicle->m_lightmask, RoRnet::LIGHTMASK_BRAKES, (vehicle->ar_brake > 0.01f && !vehicle->ar_parking_brake));
    BITMASK_SET(vehicle->m_lightmask, RoRnet::LIGHTMASK_REVERSE, (vehicle->ar_engine && vehicle->ar_engine->GetGear() < 0));
}

