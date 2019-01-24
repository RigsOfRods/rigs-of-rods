/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#include "BeamFactory.h"

#include "Application.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "CacheSystem.h"
#include "ChatSystem.h"
#include "Collisions.h"
#include "DashBoardManager.h"
#include "DynamicCollisions.h"
#include "GUIManager.h"
#include "GUI_GameConsole.h"
#include "GUI_TopMenubar.h"
#include "LandVehicleSimulation.h"
#include "Language.h"
#include "MovableText.h"
#include "Network.h"
#include "PointColDetector.h"
#include "Replay.h"
#include "RigDef_Validator.h"
#include "RigSpawner.h"
#include "RoRFrameListener.h"
#include "Scripting.h"
#include "SoundScriptManager.h"
#include "ThreadPool.h"
#include "Utils.h"
#include "VehicleAI.h"

using namespace Ogre;
using namespace RoR;

ActorManager::ActorManager()
    : m_dt_remainder(0.0f)
    , m_forced_awake(false)
    , m_physics_steps(2000)
    , m_simulation_speed(1.0f)
    , m_actor_counter(0)
    , m_savegame_terrain_has_changed(false)
{
    // Create worker thread (used for physics calculations)
    m_sim_thread_pool = std::unique_ptr<ThreadPool>(new ThreadPool(1));

    // Create general-purpose thread pool
    int logical_cores = std::thread::hardware_concurrency();
    LOG("BEAMFACTORY: " + TOSTRING(logical_cores) + " logical CPU cores" + " found");

    int thread_pool_workers = RoR::App::app_num_workers.GetActive();
    if (thread_pool_workers < 1 || thread_pool_workers > logical_cores)
    {
        thread_pool_workers = Math::Clamp(logical_cores - 1, 1, 8);
        RoR::App::app_num_workers.SetActive(thread_pool_workers);
    }

    gEnv->threadPool = new ThreadPool(thread_pool_workers);
    LOG("BEAMFACTORY: Creating " + TOSTRING(thread_pool_workers) + " worker threads");
}

ActorManager::~ActorManager()
{
    this->SyncWithSimThread(); // Wait for sim task to finish
    delete gEnv->threadPool;
}

void ActorManager::SetupActor(Actor* actor, ActorSpawnRequest rq, std::shared_ptr<RigDef::File> def)
{
    // ~~~~ Code ported from Actor::Actor()

    Ogre::SceneNode* parent_scene_node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();

    // ~~~~ Code ported from Actor::LoadActor()
    //      LoadActor(def, beams_parent, pos, rot, spawnbox, cache_entry_number)
    //      bool Actor::LoadActor(
    //          std::shared_ptr<RigDef::File> def,
    //          Ogre::SceneNode* parent_scene_node,
    //          Ogre::Vector3 const& spawn_position,
    //          Ogre::Quaternion& spawn_rotation,
    //          collision_box_t* spawn_box,
    //          int cache_entry_number // = -1

    LOG(" == Spawning vehicle: " + def->name);

    ActorSpawner spawner;
    spawner.Setup(actor, def, parent_scene_node, rq.asr_position, rq.asr_cache_entry_num);
    /* Setup modules */
    spawner.AddModule(def->root_module);
    if (def->user_modules.size() > 0) /* The vehicle-selector may return selected modules even for vehicle with no modules defined! Hence this check. */
    {
        std::vector<Ogre::String>::iterator itor = actor->m_actor_config.begin();
        for (; itor != actor->m_actor_config.end(); itor++)
        {
            spawner.AddModule(*itor);
        }
    }
    spawner.SpawnActor();
    def->report_num_errors += spawner.GetMessagesNumErrors();
    def->report_num_warnings += spawner.GetMessagesNumWarnings();
    def->report_num_other += spawner.GetMessagesNumOther();
    // Spawner log already printed to RoR.log
    def->loading_report += spawner.ProcessMessagesToString() + "\n\n";

    RoR::App::GetGuiManager()->AddRigLoadingReport(def->name, def->loading_report, def->report_num_errors, def->report_num_warnings, def->report_num_other);
    if (def->report_num_errors != 0)
    {
        if (App::diag_auto_spawner_report.GetActive())
        {
            RoR::App::GetGuiManager()->SetVisible_SpawnerReport(true);
        }
    }
    /* POST-PROCESSING (Old-spawn code from Actor::loadTruck2) */

    actor->ar_initial_node_positions.resize(actor->ar_num_nodes);

    // Apply spawn position & spawn rotation
    for (int i = 0; i < actor->ar_num_nodes; i++)
    {
        actor->ar_nodes[i].AbsPosition = rq.asr_position + rq.asr_rotation * (actor->ar_nodes[i].AbsPosition - rq.asr_position);
        actor->ar_nodes[i].RelPosition = actor->ar_nodes[i].AbsPosition - actor->ar_origin;
    };

    /* Place correctly */
    if (! def->HasFixes())
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
            actor->ResetPosition(vehicle_position, true);
        else
            actor->ResetPosition(vehicle_position.x, vehicle_position.z, true, miny);

        if (rq.asr_spawnbox != nullptr)
        {
            bool inside = true;

            for (int i = 0; i < actor->ar_num_nodes; i++)
                inside = (inside && gEnv->collisions->isInside(actor->ar_nodes[i].AbsPosition, rq.asr_spawnbox, 0.2f));

            if (!inside)
            {
                Vector3 gpos = Vector3(vehicle_position.x, 0.0f, vehicle_position.z);

                gpos -= rq.asr_rotation * Vector3((rq.asr_spawnbox->hi.x - rq.asr_spawnbox->lo.x + actor->ar_bounding_box.getMaximum().x - actor->ar_bounding_box.getMinimum().x) * 0.6f, 0.0f, 0.0f);

                actor->ResetPosition(gpos.x, gpos.z, true, miny);
            }
        }
    }
    else
    {
        actor->ResetPosition(rq.asr_position, true);
    }

    //compute final mass
    actor->RecalculateNodeMasses(actor->m_dry_mass);
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
        actor->ar_submesh_ground_model = gEnv->collisions->getGroundModelByString(subMeshGroundModelName);
        if (!actor->ar_submesh_ground_model)
        {
            actor->ar_submesh_ground_model = gEnv->collisions->defaultgm;
        }
    }

    // Set beam defaults
    for (int i = 0; i < actor->ar_num_beams; i++)
    {
        actor->ar_beams[i].initial_beam_strength       = actor->ar_beams[i].strength;
        actor->ar_beams[i].default_beam_deform         = actor->ar_beams[i].minmaxposnegstress;
    }

    actor->m_spawn_rotation = actor->getRotation();

    TRIGGER_EVENT(SE_GENERIC_NEW_TRUCK, actor->ar_instance_id);

    // ~~~~~~~~~~~~~~~~ (continued)  code ported from Actor::Actor()

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
    actor->ToggleLights();

    if (actor->isPreloadedWithTerrain())
    {
        actor->GetGfxActor()->UpdateSimDataBuffer(); // Initial fill of sim data buffers

        actor->GetGfxActor()->UpdateFlexbodies(); // Push tasks to threadpool
        actor->GetGfxActor()->UpdateWheelVisuals(); // Push tasks to threadpool
        actor->GetGfxActor()->UpdateCabMesh();
        actor->GetGfxActor()->UpdateProps(0.f, false);
        actor->GetGfxActor()->FinishWheelUpdates(); // Sync tasks from threadpool
        actor->GetGfxActor()->FinishFlexbodyTasks(); // Sync tasks from threadpool
    }

    App::GetSimController()->GetGfxScene().RegisterGfxActor(actor->GetGfxActor());

    if (actor->ar_engine)
    {
        actor->ar_engine->OffStart();
    }
    // pressurize tires
    actor->AddTyrePressure(0.0);

    actor->ar_sim_state = Actor::SimState::LOCAL_SLEEPING;

    if (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
    {
        // network buffer layout (without RoRnet::VehicleState):
        //
        //  - 3 floats (x,y,z) for the reference node 0
        //  - ar_num_nodes - 1 times 3 short ints (compressed position info)
        //  - ar_num_wheels times a float for the wheel rotation
        //
        actor->m_net_node_buf_size = sizeof(float) * 3 + (actor->m_net_first_wheel_node - 1) * sizeof(short int) * 3;
        actor->m_net_buffer_size = actor->m_net_node_buf_size + actor->ar_num_wheels * sizeof(float);

        if (rq.asr_origin != ActorSpawnRequest::Origin::NETWORK)
        {
            // local truck
            actor->sendStreamSetup();
        }
        else
        {
            // remote truck
            actor->ar_sim_state = Actor::SimState::NETWORKED_OK;
            actor->oob1 = (RoRnet::VehicleState*)malloc(sizeof(RoRnet::VehicleState));
            actor->oob2 = (RoRnet::VehicleState*)malloc(sizeof(RoRnet::VehicleState));
            actor->oob3 = (RoRnet::VehicleState*)malloc(sizeof(RoRnet::VehicleState));
            actor->netb1 = (char*)malloc(actor->m_net_buffer_size);
            actor->netb2 = (char*)malloc(actor->m_net_buffer_size);
            actor->netb3 = (char*)malloc(actor->m_net_buffer_size);
            actor->m_net_time_offset = 0;
            actor->m_net_update_counter = 0;
            if (actor->ar_engine)
            {
                actor->ar_engine->StartEngine();
            }

            RoR::Str<100> element_name;
            ActorSpawner::ComposeName(element_name, "NetLabel", 0, actor->ar_instance_id);
            actor->m_net_label_mt = new MovableText(element_name.ToCStr(), actor->m_net_username);
            actor->m_net_label_mt->setFontName("CyberbitEnglish");
            actor->m_net_label_mt->setTextAlignment(MovableText::H_CENTER, MovableText::V_ABOVE);
            actor->m_net_label_mt->showOnTop(false);
            actor->m_net_label_mt->setCharacterHeight(2);
            actor->m_net_label_mt->setColor(ColourValue::Black);
            actor->m_net_label_mt->setVisible(true);

            actor->m_net_label_node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
            actor->m_net_label_node->attachObject(actor->m_net_label_mt);
            actor->m_net_label_node->setVisible(true);
            actor->m_deletion_scene_nodes.emplace_back(actor->m_net_label_node);
        }
    }
    else if (App::sim_replay_enabled.GetActive())
    {
        actor->ar_replay_length = App::sim_replay_length.GetActive();
        actor->m_replay_handler = new Replay(actor, actor->ar_replay_length);

        int steps = App::sim_replay_stepping.GetActive();

        if (steps <= 0)
            actor->ar_replay_precision = 0.0f;
        else
            actor->ar_replay_precision = 1.0f / ((float)steps);
    }

    LOG(" ===== DONE LOADING VEHICLE");
}

Actor* ActorManager::CreateActorInstance(ActorSpawnRequest rq, std::shared_ptr<RigDef::File> def)
{
    Actor* actor = new Actor(m_actor_counter++, static_cast<int>(m_actors.size()), def, rq);

    this->SetupActor(actor, rq, def);

    m_actors.push_back(actor);

    return actor;
}

void ActorManager::RemoveStreamSource(int sourceid)
{
    m_stream_mismatches.erase(sourceid);

    for (auto actor : m_actors)
    {
        if (actor->ar_sim_state != Actor::SimState::NETWORKED_OK)
            continue;

        if (actor->ar_net_source_id == sourceid)
        {
            App::GetSimController()->QueueActorRemove(actor);
        }
    }
}

#ifdef USE_SOCKETW
void ActorManager::HandleActorStreamData(std::vector<RoR::Networking::recv_packet_t> packet_buffer)
{
    for (auto packet : packet_buffer)
    {
        if (packet.header.command == RoRnet::MSG2_STREAM_REGISTER)
        {
            RoRnet::StreamRegister* reg = (RoRnet::StreamRegister *)packet.buffer;
            if (reg->type == 0)
            {
                LOG("[RoR] Creating remote actor for " + TOSTRING(reg->origin_sourceid) + ":" + TOSTRING(reg->origin_streamid));
                Ogre::String filename(reg->name);
                int net_result = -1; // failure
                if (!RoR::App::GetCacheSystem()->checkResourceLoaded(filename))
                {
                    RoR::LogFormat("[RoR] Cannot create remote actor (not installed), filename: '%s'", reg->name);
                }
                else
                {
                    RoRnet::UserInfo info;
                    RoR::Networking::GetUserInfo(reg->origin_sourceid, info);

                    UTFString message = RoR::ChatSystem::GetColouredName(info.username, info.colournum) + RoR::Color::CommandColour + _L(" spawned a new vehicle: ") + RoR::Color::NormalColour + reg->name;
                    RoR::App::GetGuiManager()->pushMessageChatBox(message);

                    auto actor_reg = reinterpret_cast<RoRnet::ActorStreamRegister*>(reg);
                    ActorSpawnRequest rq;
                    rq.asr_origin = ActorSpawnRequest::Origin::NETWORK;
                    rq.asr_filename = filename;
                    for (int t = 0; t < 10; t++)
                    {
                        if (!strnlen(actor_reg->actorconfig[t], 60))
                            break;
                        rq.asr_config.push_back(String(actor_reg->actorconfig[t]));
                    }

                    Actor* actor = App::GetSimController()->SpawnActorDirectly(rq);
                    actor->ar_net_source_id = reg->origin_sourceid;
                    actor->ar_net_stream_id = reg->origin_streamid;
                    actor->UpdateNetworkInfo();

                    net_result = 1; // Success
                }

                RoR::Networking::AddPacket(0, RoRnet::MSG2_STREAM_REGISTER_RESULT, sizeof(RoRnet::StreamRegister), (char *)reg);
            }
        }
        else if (packet.header.command == RoRnet::MSG2_STREAM_REGISTER_RESULT)
        {
            RoRnet::StreamRegister* reg = (RoRnet::StreamRegister *)packet.buffer;
            for (auto actor : m_actors)
            {
                if (actor->ar_sim_state == Actor::SimState::NETWORKED_OK)
                    continue;
                if (actor->ar_net_stream_id == reg->origin_streamid)
                {
                    int sourceid = packet.header.source;
                    actor->ar_net_stream_results[sourceid] = reg->status;

                    if (reg->status == 1)
                    LOG("Client " + TOSTRING(sourceid) + " successfully loaded stream " + TOSTRING(reg->origin_streamid) + " with name '" + reg->name + "', result code: " + TOSTRING(reg->status));
                    else
                    LOG("Client " + TOSTRING(sourceid) + " could not load stream " + TOSTRING(reg->origin_streamid) + " with name '" + reg->name + "', result code: " + TOSTRING(reg->status));

                    break;
                }
            }
        }
        else if (packet.header.command == RoRnet::MSG2_STREAM_UNREGISTER)
        {
            Actor* b = this->GetActorByNetworkLinks(packet.header.source, packet.header.streamid);
            if (b && b->ar_sim_state == Actor::SimState::NETWORKED_OK)
            {
                App::GetSimController()->QueueActorRemove(b);
            }
            auto search = m_stream_mismatches.find(packet.header.source);
            if (search != m_stream_mismatches.end())
            {
                auto& mismatches = search->second;
                auto it = std::find(mismatches.begin(), mismatches.end(), packet.header.streamid);
                if (it != mismatches.end())
                    mismatches.erase(it);
            }
        }
        else if (packet.header.command == RoRnet::MSG2_USER_LEAVE)
        {
            this->RemoveStreamSource(packet.header.source);
        }
        else
        {
            for (auto actor : m_actors)
            {
                if (actor->ar_sim_state != Actor::SimState::NETWORKED_OK)
                    continue;

                actor->receiveStreamData(packet.header.command, packet.header.source, packet.header.streamid, packet.buffer, packet.header.size);
            }
        }
    }
}
#endif // USE_SOCKETW

int ActorManager::CheckNetworkStreamsOk(int sourceid)
{
    if (m_stream_mismatches[sourceid].size() > 0)
        return 0;

    for (auto actor : m_actors)
    {
        if (actor->ar_sim_state != Actor::SimState::NETWORKED_OK)
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

    for (auto actor : m_actors)
    {
        if (actor->ar_sim_state == Actor::SimState::NETWORKED_OK)
            continue;

        int stream_result = actor->ar_net_stream_results[sourceid];
        if (stream_result == -1)
            return 0;
        if (stream_result == 1)
            result = 1;
    }

    return result;
}

Actor* ActorManager::GetActorByNetworkLinks(int source_id, int stream_id)
{
    for (auto actor : m_actors)
    {
        if (actor->ar_sim_state != Actor::SimState::NETWORKED_OK)
            continue;

        if (actor->ar_net_source_id == source_id && actor->ar_net_stream_id == stream_id)
        {
            return actor;
        }
    }

    return nullptr;
}

bool ActorManager::CheckAabbIntersection(Ogre::AxisAlignedBox a, Ogre::AxisAlignedBox b, float scale)
{
    if (scale != 1.0f)
    {
        Vector3 a_center = a.getCenter();
        Vector3 a_half_size = a.getHalfSize();
        a.setMaximum(a_center + a_half_size * scale);
        a.setMinimum(a_center - a_half_size * scale);

        Vector3 b_center = b.getCenter();
        Vector3 b_half_size = b.getHalfSize();
        b.setMaximum(b_center + b_half_size * scale);
        b.setMinimum(b_center - b_half_size * scale);
    }

    return a.intersects(b);
}

bool ActorManager::CheckActorAabbIntersection(int a, int b, float scale)
{
    return CheckAabbIntersection(m_actors[a]->ar_bounding_box, m_actors[b]->ar_bounding_box, scale);
}

bool ActorManager::PredictActorAabbIntersection(int a, int b, float scale)
{
    return CheckAabbIntersection(m_actors[a]->ar_predicted_bounding_box, m_actors[b]->ar_predicted_bounding_box, scale);
}

bool ActorManager::CheckActorCollAabbIntersect(int a, int b, float scale)
{
    if (m_actors[a]->ar_collision_bounding_boxes.empty() && m_actors[b]->ar_collision_bounding_boxes.empty())
    {
        return CheckActorAabbIntersection(a, b, scale);
    }
    else if (m_actors[a]->ar_collision_bounding_boxes.empty())
    {
        for (std::vector<AxisAlignedBox>::iterator it = m_actors[b]->ar_collision_bounding_boxes.begin(); it != m_actors[b]->ar_collision_bounding_boxes.end(); ++it)
            if (CheckAabbIntersection(*it, m_actors[a]->ar_bounding_box, scale))
                return true;
    }
    else if (m_actors[b]->ar_collision_bounding_boxes.empty())
    {
        for (std::vector<AxisAlignedBox>::iterator it = m_actors[a]->ar_collision_bounding_boxes.begin(); it != m_actors[a]->ar_collision_bounding_boxes.end(); ++it)
            if (CheckAabbIntersection(*it, m_actors[b]->ar_bounding_box, scale))
                return true;
    }
    else
    {
        for (std::vector<AxisAlignedBox>::iterator it_a = m_actors[a]->ar_collision_bounding_boxes.begin(); it_a != m_actors[a]->ar_collision_bounding_boxes.end(); ++it_a)
            for (std::vector<AxisAlignedBox>::iterator it_b = m_actors[b]->ar_collision_bounding_boxes.begin(); it_b != m_actors[b]->ar_collision_bounding_boxes.end(); ++it_b)
                if (CheckAabbIntersection(*it_a, *it_b, scale))
                    return true;
    }

    return false;
}

bool ActorManager::PredictActorCollAabbIntersect(int a, int b, float scale)
{
    if (m_actors[a]->ar_predicted_coll_bounding_boxes.empty() && m_actors[b]->ar_predicted_coll_bounding_boxes.empty())
    {
        return PredictActorAabbIntersection(a, b, scale);
    }
    else if (m_actors[a]->ar_predicted_coll_bounding_boxes.empty())
    {
        for (std::vector<AxisAlignedBox>::iterator it = m_actors[b]->ar_predicted_coll_bounding_boxes.begin(); it != m_actors[b]->ar_predicted_coll_bounding_boxes.end(); ++it)
            if (CheckAabbIntersection(*it, m_actors[a]->ar_predicted_bounding_box, scale))
                return true;
    }
    else if (m_actors[b]->ar_predicted_coll_bounding_boxes.empty())
    {
        for (std::vector<AxisAlignedBox>::iterator it = m_actors[a]->ar_predicted_coll_bounding_boxes.begin(); it != m_actors[a]->ar_predicted_coll_bounding_boxes.end(); ++it)
            if (CheckAabbIntersection(*it, m_actors[b]->ar_predicted_bounding_box, scale))
                return true;
    }
    else
    {
        for (std::vector<AxisAlignedBox>::iterator it_a = m_actors[a]->ar_predicted_coll_bounding_boxes.begin(); it_a != m_actors[a]->ar_predicted_coll_bounding_boxes.end(); ++it_a)
            for (std::vector<AxisAlignedBox>::iterator it_b = m_actors[b]->ar_predicted_coll_bounding_boxes.begin(); it_b != m_actors[b]->ar_predicted_coll_bounding_boxes.end(); ++it_b)
                if (CheckAabbIntersection(*it_a, *it_b, scale))
                    return true;
    }

    return false;
}

void ActorManager::RecursiveActivation(int j, std::vector<bool>& visited)
{
    if (visited[j] || m_actors[j]->ar_sim_state != Actor::SimState::LOCAL_SIMULATED)
        return;

    visited[j] = true;

    for (unsigned int t = 0; t < m_actors.size(); t++)
    {
        if (t == j || visited[t])
            continue;
        if (m_actors[t]->ar_sim_state == Actor::SimState::LOCAL_SIMULATED && CheckActorCollAabbIntersect(t, j, 1.2f))
        {
            m_actors[t]->ar_sleep_counter = 0.0f;
            this->RecursiveActivation(t, visited);
        }
        if (m_actors[t]->ar_sim_state == Actor::SimState::LOCAL_SLEEPING && PredictActorCollAabbIntersect(t, j))
        {
            m_actors[t]->ar_sleep_counter = 0.0f;
            m_actors[t]->ar_sim_state = Actor::SimState::LOCAL_SIMULATED;
            this->RecursiveActivation(t, visited);
        }
    }
}

void ActorManager::ForwardCommands(Actor* source_actor)
{
    if (source_actor->ar_forward_commands)
    {
        auto linked_actors = source_actor->GetAllLinkedActors();

        for (auto actor : RoR::App::GetSimController()->GetActors())
        {
            if (actor != source_actor && actor->ar_import_commands &&
                    (actor->getPosition().distance(source_actor->getPosition()) < 
                     actor->m_min_camera_radius + source_actor->m_min_camera_radius))
            {
                // activate the truck
                if (actor->ar_sim_state == Actor::SimState::LOCAL_SLEEPING)
                {
                    actor->ar_sleep_counter = 0.0f;
                    actor->ar_sim_state = Actor::SimState::LOCAL_SIMULATED;
                }

                if (App::sim_realistic_commands.GetActive())
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
                    actor->ToggleTies();
                }
                if (source_actor->ar_toggle_ropes)
                {
                    actor->ToggleRopes(-1);
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
                hook.hk_locked_actor->ToggleParkingBrake();
            }

            // forward lights
            hook.hk_locked_actor->ar_lights = source_actor->ar_lights;
            hook.hk_locked_actor->m_blink_type = source_actor->m_blink_type;
            hook.hk_locked_actor->m_reverse_light_active = source_actor->getReverseLightVisible();
        }
    }
}

void ActorManager::UpdateSleepingState(Actor* player_actor, float dt)
{
    if (!m_forced_awake)
    {
        for (auto actor : m_actors)
        {
            if (actor->ar_sim_state != Actor::SimState::LOCAL_SIMULATED)
                continue;
            if (actor->getVelocity().squaredLength() > 0.01f)
            {
                actor->ar_sleep_counter = 0.0f;
                continue;
            }

            actor->ar_sleep_counter += dt;

            if (actor->ar_sleep_counter >= 10.0f)
            {
                actor->ar_sim_state = Actor::SimState::LOCAL_SLEEPING;
            }
        }
    }

    if (player_actor && player_actor->ar_sim_state == Actor::SimState::LOCAL_SLEEPING)
    {
        player_actor->ar_sim_state = Actor::SimState::LOCAL_SIMULATED;
    }

    std::vector<bool> visited(m_actors.size());
    // Recursivly activate all actors which can be reached from current actor
    if (player_actor && player_actor->ar_sim_state == Actor::SimState::LOCAL_SIMULATED)
    {
        player_actor->ar_sleep_counter = 0.0f;
        this->RecursiveActivation(player_actor->ar_vector_index, visited);
    }
    // Snowball effect (activate all actors which might soon get hit by a moving actor)
    for (unsigned int t = 0; t < m_actors.size(); t++)
    {
        if (m_actors[t]->ar_sim_state == Actor::SimState::LOCAL_SIMULATED && m_actors[t]->ar_sleep_counter == 0.0f)
            this->RecursiveActivation(t, visited);
    }
}

void ActorManager::WakeUpAllActors()
{
    for (auto actor : m_actors)
    {
        if (actor->ar_sim_state == Actor::SimState::LOCAL_SLEEPING)
        {
            actor->ar_sim_state = Actor::SimState::LOCAL_SIMULATED;
            actor->ar_sleep_counter = 0.0f;
        }
    }
}

void ActorManager::SendAllActorsSleeping()
{
    m_forced_awake = false;
    for (auto actor : m_actors)
    {
        if (actor->ar_sim_state == Actor::SimState::LOCAL_SIMULATED)
        {
            actor->ar_sim_state = Actor::SimState::LOCAL_SLEEPING;
        }
    }
}

void ActorManager::RecalcGravityMasses()
{
    for (auto actor : m_actors)
    {
        actor->recalc_masses();
    }
}

Actor* ActorManager::FindActorInsideBox(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box)
{
    // try to find the desired actor (the one in the box)
    Actor* ret = nullptr;
    for (auto actor : m_actors)
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
    Actor* actor = this->FindActorInsideBox(collisions, inst, box);
    if (actor != nullptr)
    {
        SOUND_PLAY_ONCE(actor, SS_TRIG_REPAIR);

        ActorModifyRequest rq;
        rq.amr_actor = actor;
        rq.amr_type = ActorModifyRequest::Type::RESET_ON_SPOT;
        App::GetSimController()->QueueActorModify(rq);
    }
}

void ActorManager::MuteAllActors()
{
    for (auto actor : m_actors)
    {
        actor->StopAllSounds();
    }
}

void ActorManager::UnmuteAllActors()
{
    for (auto actor : m_actors)
    {
        actor->UnmuteAllSounds();
    }
}

void ActorManager::CleanUpAllActors() // Called after simulation finishes
{
    for (auto actor : m_actors)
    {
        delete actor;
    }
    m_actors.clear();
}

void ActorManager::DeleteActorInternal(Actor* actor)
{
    if (actor == 0)
        return;

    this->SyncWithSimThread();

#ifdef USE_SOCKETW
    if (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
    {
        if (actor->ar_sim_state != Actor::SimState::NETWORKED_OK && actor->ar_sim_state != Actor::SimState::INVALID)
        {
            RoR::Networking::AddPacket(actor->ar_net_stream_id, RoRnet::MSG2_STREAM_UNREGISTER, 0, 0);
        }
    }
#endif // USE_SOCKETW

    m_actors.erase(std::remove(m_actors.begin(), m_actors.end(), actor), m_actors.end());
    delete actor;

    // Upate actor indices
    for (unsigned int i = 0; i < m_actors.size(); i++)
        m_actors[i]->ar_vector_index = i;
}

int FindPivotActorId(Actor* player, Actor* prev_player)
{
    if (player != nullptr)
        return player->ar_vector_index;
    else if (prev_player != nullptr)
        return prev_player->ar_vector_index + 1;
    return -1;
}

Actor* ActorManager::FetchNextVehicleOnList(Actor* player, Actor* prev_player)
{
    int pivot_index = FindPivotActorId(player, prev_player);

    for (int i = pivot_index + 1; i < m_actors.size(); i++)
    {
        if (m_actors[i]->ar_sim_state != Actor::SimState::NETWORKED_OK && !m_actors[i]->isPreloadedWithTerrain())
        {
            return m_actors[i];
        }
    }

    for (int i = 0; i < pivot_index; i++)
    {
        if (m_actors[i]->ar_sim_state != Actor::SimState::NETWORKED_OK && !m_actors[i]->isPreloadedWithTerrain())
        {
            return m_actors[i];
        }
    }

    if (pivot_index >= 0 && m_actors[pivot_index]->ar_sim_state != Actor::SimState::NETWORKED_OK && !m_actors[pivot_index]->isPreloadedWithTerrain())
    {
        return m_actors[pivot_index];
    }

    return nullptr;
}

Actor* ActorManager::FetchPreviousVehicleOnList(Actor* player, Actor* prev_player)
{
    int pivot_index = FindPivotActorId(player, prev_player);

    for (int i = pivot_index - 1; i >= 0; i--)
    {
        if (m_actors[i]->ar_sim_state != Actor::SimState::NETWORKED_OK && !m_actors[i]->isPreloadedWithTerrain())
        {
            return m_actors[i];
        }
    }

    for (int i = static_cast<int>(m_actors.size()) - 1; i > pivot_index; i--)
    {
        if (m_actors[i]->ar_sim_state != Actor::SimState::NETWORKED_OK && !m_actors[i]->isPreloadedWithTerrain())
        {
            return m_actors[i];
        }
    }

    if (pivot_index >= 0 && m_actors[pivot_index]->ar_sim_state != Actor::SimState::NETWORKED_OK && !m_actors[pivot_index]->isPreloadedWithTerrain())
    {
        return m_actors[pivot_index];
    }

    return nullptr;
}

Actor* ActorManager::FetchRescueVehicle()
{
    for (auto actor : m_actors)
    {
        if (actor->ar_rescuer_flag)
        {
            return actor;
        }
    }
    return nullptr;
}

void ActorManager::UpdateActors(Actor* player_actor, float dt)
{
    // do not allow dt > 1/20
    dt = std::min(dt, 1.0f / 20.0f);

    dt *= m_simulation_speed;

    dt += m_dt_remainder;
    m_physics_steps = dt / PHYSICS_DT;
    m_dt_remainder = dt - (m_physics_steps * PHYSICS_DT);
    dt = PHYSICS_DT * m_physics_steps;

    gEnv->mrTime += dt;

    this->SyncWithSimThread();

    this->UpdateSleepingState(player_actor, dt);

    for (auto actor : m_actors)
    {
        actor->HandleInputEvents(dt);
        actor->HandleAngelScriptEvents(dt);

#ifdef USE_ANGELSCRIPT
        if (actor->ar_vehicle_ai && actor->ar_vehicle_ai->IsActive())
            actor->ar_vehicle_ai->update(dt, 0);
#endif // USE_ANGELSCRIPT

        if (actor->ar_engine)
        {
            if (actor->ar_driveable == TRUCK)
            {
                LandVehicleSimulation::UpdateVehicle(actor, dt);
            }
            if (actor->ar_sim_state == Actor::SimState::LOCAL_SLEEPING)
            {
                actor->ar_engine->UpdateEngineSim(dt, 1);
            }
            actor->ar_engine->UpdateEngineAudio();
        }

        if (actor->ar_sim_state == Actor::SimState::LOCAL_SLEEPING)
        {
            if (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
            {
                auto lifetime = actor->ar_net_timer.getMilliseconds();
                if (lifetime < 10000 || lifetime - actor->ar_net_last_update_time > 5000)
                {
                    actor->sendStreamData();
                }
            }
        }
        else if (actor->ar_sim_state != Actor::SimState::INVALID)
        {
            actor->updateVisual(dt);
            actor->UpdateFlareStates(dt); // Only state, visuals done by GfxActor
            if (actor->ar_update_physics && App::gfx_skidmarks_mode.GetActive() > 0)
            {
                actor->updateSkidmarks();
            }
            if (actor->ar_sim_state == Actor::SimState::NETWORKED_OK)
            {
                actor->CalcNetwork();
            }
            else if (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
            {
                actor->sendStreamData();
            }
        }
    }

    if (player_actor != nullptr)
    {
        this->ForwardCommands(player_actor);
        if (player_actor->ar_toggle_ties)
        {
            player_actor->ToggleTies();
            player_actor->ar_toggle_ties = false;
        }
        if (player_actor->ar_toggle_ropes)
        {
            player_actor->ToggleRopes(-1);
            player_actor->ar_toggle_ropes = false;
        }
        player_actor->updateDashBoards(dt);
        player_actor->ForceFeedbackStep(m_physics_steps);
        if (player_actor->ReplayStep())
            return; // Skip UpdatePhysicsSimulation()
    }

    auto func = std::function<void()>([this]()
        {
            this->UpdatePhysicsSimulation();
        });
    m_sim_task = m_sim_thread_pool->RunTask(func);

    if (!RoR::App::app_async_physics.GetActive())
        m_sim_task->join();
}

void ActorManager::NotifyActorsWindowResized()
{
    for (auto actor : m_actors)
    {
        actor->ar_dashboard->windowResized();
    }
}

Actor* ActorManager::GetActorByIdInternal(int actor_id)
{
    for (auto actor : m_actors)
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
    for (auto actor : m_actors)
    {
        actor->UpdatePhysicsOrigin();
    }
    for (int i = 0; i < m_physics_steps; i++)
    {
        {
            std::vector<std::function<void()>> tasks;
            for (auto actor : m_actors)
            {
                if (actor->ar_update_physics = actor->CalcForcesEulerPrepare(i == 0))
                {
                    auto func = std::function<void()>([this, i, actor]()
                        {
                            actor->CalcForcesEulerCompute(i == 0, m_physics_steps);
                        });
                    tasks.push_back(func);
                }
            }
            gEnv->threadPool->Parallelize(tasks);
            for (auto actor : m_actors)
            {
                if (actor->ar_update_physics)
                {
                    actor->CalcBeamsInterActor();
                }
            }
        }
        {
            std::vector<std::function<void()>> tasks;
            for (auto actor : m_actors)
            {
                if (actor->m_inter_point_col_detector != nullptr && (actor->ar_update_physics ||
                        (App::mp_pseudo_collisions.GetActive() && actor->ar_sim_state == Actor::SimState::NETWORKED_OK)))
                {
                    auto func = std::function<void()>([this, actor]()
                        {
                            actor->m_inter_point_col_detector->UpdateInterPoint();
                            if (actor->ar_collision_relevant)
                            {
                                ResolveInterActorCollisions(PHYSICS_DT,
                                    *actor->m_inter_point_col_detector,
                                    actor->ar_num_collcabs,
                                    actor->ar_collcabs,
                                    actor->ar_cabs,
                                    actor->ar_inter_collcabrate,
                                    actor->ar_nodes,
                                    actor->ar_collision_range,
                                    *actor->ar_submesh_ground_model);
                            }
                        });
                    tasks.push_back(func);
                }
            }
            gEnv->threadPool->Parallelize(tasks);
        }
    }
    for (auto actor : m_actors)
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

void HandleErrorLoadingTruckfile(const char* filename, const char* exception_msg)
{
    RoR::Str<200> msg;
    msg << "Failed to load actor '" << filename << "', message: " << exception_msg;
    RoR::App::GetGuiManager()->PushNotification("Error:", msg.ToCStr());
    RoR::LogFormat("[RoR] %s", msg.ToCStr());

    if (RoR::App::GetConsole())
    {
        RoR::App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, msg.ToCStr(), "error.png", 30000, true);
    }
}

std::shared_ptr<RigDef::File> ActorManager::FetchActorDef(const char* filename, bool predefined_on_terrain)
{
    // Find the user content
    CacheEntry* cache_entry = App::GetCacheSystem()->FindEntryByFilename(filename);
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
        if (!RoR::App::GetCacheSystem()->checkResourceLoaded(resource_filename, resource_groupname)) // Validates the filename and finds resource group
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

        RoR::LogFormat("[RoR] Parsing truckfile '%s'", filename);
        RigDef::Parser parser;
        parser.Prepare();
        parser.ProcessOgreStream(stream.getPointer());
        parser.Finalize();

        auto def = parser.GetFile();

        def->report_num_errors = parser.GetMessagesNumErrors();
        def->report_num_warnings = parser.GetMessagesNumWarnings();
        def->report_num_other = parser.GetMessagesNumOther();
        def->loading_report = parser.ProcessMessagesToString();
        def->loading_report += "\n\n";
        LOG(def->loading_report);

        auto* importer = parser.GetSequentialImporter();
        if (importer->IsEnabled() && App::diag_rig_log_messages.GetActive())
        {
            def->report_num_errors += importer->GetMessagesNumErrors();
            def->report_num_warnings += importer->GetMessagesNumWarnings();
            def->report_num_other += importer->GetMessagesNumOther();

            std::string importer_report = importer->ProcessMessagesToString();
            LOG(importer_report);

            def->loading_report += importer_report + "\n\n";
        }

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
            std::string file_name(filename);
            std::string file_extension = file_name.substr(file_name.find_last_of('.'));
            Ogre::StringUtil::toLowerCase(file_extension);
            if ((file_extension == ".load") | (file_extension == ".fixed"))
            {
                validator.SetCheckBeams(false);
            }
        }
        bool valid = validator.Validate();

        def->report_num_errors += validator.GetMessagesNumErrors();
        def->report_num_warnings += validator.GetMessagesNumWarnings();
        def->report_num_other += validator.GetMessagesNumOther();
        std::string validator_report = validator.ProcessMessagesToString();
        LOG(validator_report);
        def->loading_report += validator_report;
        def->loading_report += "\n\n";
        // Continue anyway...

        // Extra information to RoR.log
        if (importer->IsEnabled())
        {
            if (App::diag_rig_log_node_stats.GetActive())
            {
                LOG(importer->GetNodeStatistics());
            }
            if (App::diag_rig_log_node_import.GetActive())
            {
                LOG(importer->IterateAndPrintAllNodes());
            }
        }

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

std::vector<Actor*> ActorManager::GetLocalActors()
{
    std::vector<Actor*> actors;
    for (auto actor : m_actors)
    {
        if (actor->ar_sim_state != Actor::SimState::NETWORKED_OK && actor->ar_sim_state != Actor::SimState::INVALID)
            actors.push_back(actor);
    }
    return actors;
}

ActorSpawnRequest::ActorSpawnRequest()
    : asr_position(Ogre::Vector3::ZERO)
    , asr_rotation(Ogre::Quaternion::ZERO)
    , asr_cache_entry_num(-1) // flexbody cache disabled
    , asr_spawnbox(nullptr)
    , asr_skin(nullptr)
    , asr_origin(Origin::UNKNOWN)
    , asr_cache_entry(nullptr)
    , asr_free_position(false)
    , asr_terrn_machine(false)
{}
