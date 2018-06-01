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
#include "BeamEngine.h"
#include "BeamStats.h"
#include "CacheSystem.h"
#include "ChatSystem.h"
#include "Collisions.h"
#include "DynamicCollisions.h"
#include "GUIManager.h"
#include "GUI_GameConsole.h"
#include "GUI_TopMenubar.h"
#include "Language.h"
#include "MainMenu.h"
#include "MovableText.h"
#include "Network.h"
#include "PointColDetector.h"
#include "PositionStorage.h"
#include "Replay.h"
#include "RigDef_Parser.h"
#include "RigDef_Validator.h"
#include "RigLoadingProfilerControl.h"
#include "RigSpawner.h"
#include "RoRFrameListener.h"
#include "Scripting.h"
#include "Settings.h"
#include "SoundScriptManager.h"
#include "ThreadPool.h"
#include "Utils.h"
#include "VehicleAI.h"

#ifdef _GNU_SOURCE
#include <sys/sysinfo.h>
#endif

#if defined(__APPLE__) || defined (__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif // __APPLE__ || __FREEBSD__

#include "DashBoardManager.h"

#include <algorithm>
#include <cstring>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
  #include <intrin.h>
#endif

using namespace Ogre;

void cpuID(unsigned i, unsigned regs[4])
{
#ifdef _WIN32
    __cpuid((int *)regs, (int)i);
#elif defined(__x86_64__) || defined(__i386)
    asm volatile
        ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
         : "a" (i), "c" (0));
#endif
}

static unsigned hardware_concurrency()
{
#if defined(_GNU_SOURCE)
        return get_nprocs();
#elif defined(__APPLE__) || defined(__FreeBSD__)
        int count;
        size_t size = sizeof(count);
        return sysctlbyname("hw.ncpu", &count, &size, NULL, 0) ? 0 : count;
#elif defined(BOOST_HAS_UNISTD_H) && defined(_SC_NPROCESSORS_ONLN)
        int const count = sysconf(_SC_NPROCESSORS_ONLN);
        return (count > 0) ? count : 0;
#else
    return std::thread::hardware_concurrency();
#endif
}

unsigned int getNumberOfCPUCores()
{
#if defined(_WIN32) || defined(__x86_64__) || defined(__i386)
    unsigned regs[4];

    // Get CPU vendor
    char vendor[12];
    cpuID(0, regs);
    memcpy((void *)vendor,     (void *)&regs[1], 4); // EBX
    memcpy((void *)&vendor[4], (void *)&regs[3], 4); // EDX
    memcpy((void *)&vendor[8], (void *)&regs[2], 4); // ECX
    std::string cpuVendor = std::string(vendor, 12);

    // Get CPU features
    cpuID(1, regs);
    unsigned cpuFeatures = regs[3]; // EDX

    // Logical core count per CPU
    cpuID(1, regs);
    unsigned logical = (regs[1] >> 16) & 0xff; // EBX[23:16]
    unsigned cores = logical;

    if (cpuVendor == "GenuineIntel")
    {
        // Get DCP cache info
        cpuID(4, regs);
        cores = ((regs[0] >> 26) & 0x3f) + 1; // EAX[31:26] + 1
    }
    else if (cpuVendor == "AuthenticAMD")
    {
        // Get NC: Number of CPU cores - 1
        cpuID(0x80000008, regs);
        cores = ((unsigned)(regs[2] & 0xff)) + 1; // ECX[7:0] + 1
    }

    // Detect hyper-threads
    bool hyperThreads = cpuFeatures & (1 << 28) && cores < logical;

    LOG("BEAMFACTORY: " + TOSTRING(logical) + " logical CPU cores" + " found");
    LOG("BEAMFACTORY: " + TOSTRING(cores) + " CPU cores" + " found");
    LOG("BEAMFACTORY: Hyper-Threading " + TOSTRING(hyperThreads));
#else
    unsigned cores = hardware_concurrency();
    LOG("BEAMFACTORY: " + TOSTRING(cores) + " CPU cores" + " found");
#endif
    return cores;
}

using namespace RoR;

ActorManager::ActorManager(RoRFrameListener* sim_controller)
    : m_dt_remainder(0.0f)
    , m_forced_awake(false)
    , m_free_actor_slot(0)
    , m_num_cpu_cores(0)
    , m_physics_frames(0)
    , m_physics_steps(2000)
    , m_simulated_actor(0)
    , m_simulation_speed(1.0f)
    , m_sim_controller(sim_controller)
    , m_actors() // Array
{

    if (RoR::App::app_multithread.GetActive())
    {
        // Create thread pool
        int numThreadsInPool = ISETTING("NumThreadsInThreadPool", 0);

        if (numThreadsInPool > 1)
        {
            m_num_cpu_cores = numThreadsInPool;
        }
        else
        {
            int logical_cpus = hardware_concurrency();
            int physical_cpus = getNumberOfCPUCores();

            if (physical_cpus < 6 && logical_cpus > physical_cpus)
                m_num_cpu_cores = logical_cpus - 1;
            else
                m_num_cpu_cores = physical_cpus - 1;
        }

        bool disableThreadPool = BSETTING("DisableThreadPool", false);

        if (m_num_cpu_cores < 2)
        {
            disableThreadPool = true;
            LOG("BEAMFACTORY: Not enough CPU cores to enable the thread pool");
        }
        else if (!disableThreadPool)
        {
            gEnv->threadPool = new ThreadPool(m_num_cpu_cores);
            LOG("BEAMFACTORY: Creating " + TOSTRING(m_num_cpu_cores) + " threads");
        }

        // Create worker thread (used for physics calculations)
        m_sim_thread_pool = std::unique_ptr<ThreadPool>(new ThreadPool(1));
    }
}

ActorManager::~ActorManager()
{
    this->SyncWithSimThread(); // Wait for sim task to finish
    delete gEnv->threadPool;
    m_particle_manager.DustManDiscard(gEnv->sceneManager); // TODO: de-globalize SceneManager
}

void ActorManager::SetupActor(
        Actor* actor,
        std::shared_ptr<RigDef::File> def,
        Ogre::Vector3 const& spawn_position,
        Ogre::Quaternion const& spawn_rotation,
        collision_box_t* spawn_box,
        bool free_positioned,
        bool _networked,
        int cache_entry_number // = -1
)
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
    spawner.Setup(actor, def, parent_scene_node, spawn_position, cache_entry_number);
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
        if (BSETTING("AutoActorSpawnerReport", false))
        {
            RoR::App::GetGuiManager()->SetVisible_SpawnerReport(true);
        }
    }
    /* POST-PROCESSING (Old-spawn code from Actor::loadTruck2) */

    // Apply spawn position & spawn rotation
    for (int i = 0; i < actor->ar_num_nodes; i++)
    {
        actor->ar_nodes[i].AbsPosition = spawn_position + spawn_rotation * (actor->ar_nodes[i].AbsPosition - spawn_position);
        actor->ar_nodes[i].RelPosition = actor->ar_nodes[i].AbsPosition - actor->ar_origin;
    };

    /* Place correctly */
    if (! def->HasFixes())
    {
        Ogre::Vector3 vehicle_position = spawn_position;

        // check if over-sized
        ActorSpawner::RecalculateBoundingBoxes(actor);
        vehicle_position.x -= (actor->ar_bounding_box.getMaximum().x + actor->ar_bounding_box.getMinimum().x) / 2.0 - vehicle_position.x;
        vehicle_position.z -= (actor->ar_bounding_box.getMaximum().z + actor->ar_bounding_box.getMinimum().z) / 2.0 - vehicle_position.z;

        float miny = 0.0f;

        if (!actor->m_preloaded_with_terrain)
        {
            miny = vehicle_position.y;
        }

        if (spawn_box != nullptr)
        {
            miny = spawn_box->relo.y + spawn_box->center.y;
        }

        if (free_positioned)
            actor->ResetPosition(vehicle_position, true);
        else
            actor->ResetPosition(vehicle_position.x, vehicle_position.z, true, miny);

        if (spawn_box != nullptr)
        {
            bool inside = true;

            for (int i = 0; i < actor->ar_num_nodes; i++)
                inside = (inside && gEnv->collisions->isInside(actor->ar_nodes[i].AbsPosition, spawn_box, 0.2f));

            if (!inside)
            {
                Vector3 gpos = Vector3(vehicle_position.x, 0.0f, vehicle_position.z);

                gpos -= spawn_rotation * Vector3((spawn_box->hi.x - spawn_box->lo.x + actor->ar_bounding_box.getMaximum().x - actor->ar_bounding_box.getMinimum().x) * 0.6f, 0.0f, 0.0f);

                actor->ResetPosition(gpos.x, gpos.z, true, miny);
            }
        }
    }
    else
    {
        actor->ResetPosition(spawn_position, true);
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

    ActorSpawner::RecalculateBoundingBoxes(actor);

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
        actor->ar_beams[i].default_beam_plastic_coef   = actor->ar_beams[i].plastic_coef;
    }

    // TODO: Check cam. nodes once on spawn! They never change --> no reason to repeat the check. ~only_a_ptr, 06/2017
    if (actor->ar_camera_node_pos[0] != actor->ar_camera_node_dir[0] && actor->IsNodeIdValid(actor->ar_camera_node_pos[0]) && actor->IsNodeIdValid(actor->ar_camera_node_dir[0]))
    {
        Vector3 cur_dir = actor->ar_nodes[actor->ar_camera_node_pos[0]].RelPosition - actor->ar_nodes[actor->ar_camera_node_dir[0]].RelPosition;
        actor->m_spawn_rotation = atan2(cur_dir.dotProduct(Vector3::UNIT_X), cur_dir.dotProduct(-Vector3::UNIT_Z));
    }
    else if (actor->ar_num_nodes > 1)
    {
        float max_dist = 0.0f;
        int furthest_node = 1;
        for (int i = 0; i < actor->ar_num_nodes; i++)
        {
            float dist = actor->ar_nodes[i].RelPosition.squaredDistance(actor->ar_nodes[0].RelPosition);
            if (dist > max_dist)
            {
                max_dist = dist;
                furthest_node = i;
            }
        }
        Vector3 cur_dir = actor->ar_nodes[0].RelPosition - actor->ar_nodes[furthest_node].RelPosition;
        actor->m_spawn_rotation = atan2(cur_dir.dotProduct(Vector3::UNIT_X), cur_dir.dotProduct(-Vector3::UNIT_Z));
    }

    Vector3 cinecam = actor->ar_nodes[0].AbsPosition;
    if (actor->IsNodeIdValid(actor->ar_camera_node_pos[0]))
    {
        cinecam = actor->ar_nodes[actor->ar_camera_node_pos[0]].AbsPosition;
    }

    // Calculate the approximate median
    std::vector<Real> mx(actor->ar_num_nodes, 0.0f);
    std::vector<Real> my(actor->ar_num_nodes, 0.0f);
    std::vector<Real> mz(actor->ar_num_nodes, 0.0f);
    for (int i = 0; i < actor->ar_num_nodes; i++)
    {
        mx[i] = actor->ar_nodes[i].AbsPosition.x;
        my[i] = actor->ar_nodes[i].AbsPosition.y;
        mz[i] = actor->ar_nodes[i].AbsPosition.z;
    }
    std::nth_element(mx.begin(), mx.begin() + actor->ar_num_nodes / 2, mx.end());
    std::nth_element(my.begin(), my.begin() + actor->ar_num_nodes / 2, my.end());
    std::nth_element(mz.begin(), mz.begin() + actor->ar_num_nodes / 2, mz.end());
    Vector3 median = Vector3(mx[actor->ar_num_nodes / 2], my[actor->ar_num_nodes / 2], mz[actor->ar_num_nodes / 2]);

    // Calculate the average
    Vector3 sum = Vector3::ZERO;
    for (int i = 0; i < actor->ar_num_nodes; i++)
    {
        sum += actor->ar_nodes[i].AbsPosition;
    }
    Vector3 average = sum / actor->ar_num_nodes;

    // Decide whether or not the cinecam node is an appropriate rotation center
    actor->m_cinecam_is_rotation_center = cinecam.squaredDistance(median) < average.squaredDistance(median);

    TRIGGER_EVENT(SE_GENERIC_NEW_TRUCK, actor->ar_instance_id);

    // ~~~~~~~~~~~~~~~~ (continued)  code ported from Actor::Actor()

    actor->NotifyActorCameraChanged(); // setup sounds properly

    if (App::sim_replay_enabled.GetActive() && !_networked && !actor->ar_uses_networking)     // setup replay mode
    {
        actor->ar_replay_length = App::sim_replay_length.GetActive();
        actor->m_replay_handler = new Replay(actor, actor->ar_replay_length);

        int steps = App::sim_replay_stepping.GetActive();

        if (steps <= 0)
            actor->ar_replay_precision = 0.0f;
        else
            actor->ar_replay_precision = 1.0f / ((float)steps);
    }

    // add storage
    if (App::sim_position_storage.GetActive())
    {
        actor->m_position_storage = new PositionStorage(actor->ar_num_nodes, 10);
    }

    // calculate the number of wheel nodes
    actor->m_wheel_node_count = 0;
    for (int i = 0; i < actor->ar_num_nodes; i++)
    {
        if (actor->ar_nodes[i].iswheel != NOWHEEL)
            actor->m_wheel_node_count++;
    }

    // search m_net_first_wheel_node
    actor->m_net_first_wheel_node = actor->ar_num_nodes;
    for (int i = 0; i < actor->ar_num_nodes; i++)
    {
        if (actor->ar_nodes[i].iswheel == WHEEL_DEFAULT)
        {
            actor->m_net_first_wheel_node = i;
            break;
        }
    }

    // network buffer layout (without RoRnet::VehicleState):
    //
    //  - 3 floats (x,y,z) for the reference node 0
    //  - ar_num_nodes - 1 times 3 short ints (compressed position info)
    //  - ar_num_wheels times a float for the wheel rotation
    //
    actor->m_net_node_buf_size = sizeof(float) * 3 + (actor->m_net_first_wheel_node - 1) * sizeof(short int) * 3;
    actor->m_net_buffer_size = actor->m_net_node_buf_size + actor->ar_num_wheels * sizeof(float);
    actor->UpdateFlexbodiesPrepare();
    actor->UpdateFlexbodiesFinal();
    actor->updateVisual();
    // stop lights
    actor->ToggleLights();

    actor->updateFlares(0);
    actor->updateProps();
    if (actor->ar_engine)
    {
        actor->ar_engine->OffStart();
    }
    // pressurize tires
    actor->AddTyrePressure(0.0);

    actor->CreateSimpleSkeletonMaterial();

    actor->ar_sim_state = Actor::SimState::LOCAL_SLEEPING;

    // start network stuff
    if (_networked)
    {
        actor->ar_sim_state = Actor::SimState::NETWORKED_OK;
        // malloc memory
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
    }

    if (actor->ar_uses_networking)
    {
        if (actor->ar_sim_state != Actor::SimState::NETWORKED_OK)
        {
            actor->sendStreamSetup();
        }

        if (actor->ar_sim_state == Actor::SimState::NETWORKED_OK || !actor->m_hide_own_net_label)
        {
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

    LOG(" ===== DONE LOADING VEHICLE");
}

Actor* ActorManager::CreateLocalActor(
    Ogre::Vector3 pos,
    Ogre::Quaternion rot,
    Ogre::String fname,
    int cache_entry_number, // = -1,
    collision_box_t* spawnbox /* = nullptr */,
    const std::vector<Ogre::String>* actor_config /* = nullptr */,
    RoR::SkinDef* skin /* = nullptr */,
    bool free_position, /* = false */
    bool preloaded_with_terrain /* = false */
)
{
#ifdef ROR_PROFILE_RIG_LOADING
    ::Profiler::reset();
#endif

    std::shared_ptr<RigDef::File> def = this->FetchActorDef(fname.c_str(), preloaded_with_terrain);
    if (def == nullptr)
    {
        return nullptr; // Error already reported
    }

    int actor_id = this->GetFreeActorSlot();
    if (actor_id == -1)
    {
        LOG("ERROR: Could not add beam to main list");
        return 0;
    }

    Actor* actor = new Actor(
        actor_id,
        def,
        pos,
        rot,
        fname.c_str(),
        false, // networked
        (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED), // networking
        spawnbox,
        actor_config,
        skin,
        preloaded_with_terrain,
        cache_entry_number
    );

    this->SetupActor(actor, def, pos, rot, spawnbox, free_position, false, cache_entry_number);

    m_actors[actor_id] = actor;

    // lock slide nodes after spawning the actor?
    if (actor->getSlideNodesLockInstant())
    {
        actor->ToggleSlideNodeLock();
    }

    RoR::App::GetGuiManager()->GetTopMenubar()->triggerUpdateVehicleList();

    // add own username to the actor
    if (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
    {
        actor->UpdateNetworkInfo();
    }

#ifdef ROR_PROFILE_RIG_LOADING
    std::string out_path = std::string(App::sys_user_dir.GetActive()) + PATH_SLASH + "profiler" + PATH_SLASH + ROR_PROFILE_RIG_LOADING_OUTFILE;
    ::Profiler::DumpHtml(out_path.c_str());
#endif
    return actor;
}

int ActorManager::CreateRemoteInstance(RoRnet::ActorStreamRegister* reg)
{
    LOG("[RoR] Creating remote actor for " + TOSTRING(reg->origin_sourceid) + ":" + TOSTRING(reg->origin_streamid));

#ifdef USE_SOCKETW
    RoRnet::UserInfo info;
    RoR::Networking::GetUserInfo(reg->origin_sourceid, info);

    UTFString message = RoR::ChatSystem::GetColouredName(info.username, info.colournum) + RoR::Color::CommandColour + _L(" spawned a new vehicle: ") + RoR::Color::NormalColour + reg->name;
    RoR::App::GetGuiManager()->pushMessageChatBox(message);
#endif // USE_SOCKETW

    // check if we got this actor installed
    String filename = String(reg->name);
    String group = "";
    if (!RoR::App::GetCacheSystem()->checkResourceLoaded(filename, group))
    {
        LOG("[RoR] Cannot create remote actor (not installed), filename: '"+filename+"'");
        return -1;
    }

    // fill config
    std::vector<String> actor_config;
    for (int t = 0; t < 10; t++)
    {
        if (!strnlen(reg->actorconfig[t], 60))
            break;
        actor_config.push_back(String(reg->actorconfig[t]));
    }

    std::shared_ptr<RigDef::File> def = this->FetchActorDef(filename.c_str(), false);
    if (def == nullptr)
    {
        RoR::LogFormat("[RoR] Cannot create remote actor '%s', error parsing truckfile", filename.c_str());
        return -1;
    }

    // DO NOT spawn the actor far off anywhere
    // the actor parsing will break flexbodies initialization when using huge numbers here
    Vector3 pos = Vector3::ZERO;

    int actor_id = this->GetFreeActorSlot();
    if (actor_id == -1)
    {
        LOG("ERROR: could not add beam to main list");
        return -1;
    }

    Actor* actor = new Actor(
        actor_id,
        def,
        pos,
        Quaternion::ZERO,
        reg->name,
        true, // networked
        (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED), // networking
        nullptr, // spawnbox
        &actor_config,
        nullptr // skin
    );

    this->SetupActor(actor, def, pos, Ogre::Quaternion::ZERO, nullptr, true, -1);

    m_actors[actor_id] = actor;

    actor->ar_net_source_id = reg->origin_sourceid;
    actor->ar_net_stream_id = reg->origin_streamid;
    actor->UpdateNetworkInfo();


    RoR::App::GetGuiManager()->GetTopMenubar()->triggerUpdateVehicleList();


    return 1;
}

void ActorManager::RemoveStreamSource(int sourceid)
{
    m_stream_mismatches.erase(sourceid);

    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (!m_actors[t])
            continue;
        if (m_actors[t]->ar_sim_state != Actor::SimState::NETWORKED_OK)
            continue;

        if (m_actors[t]->ar_net_source_id == sourceid)
        {
            this->DeleteActorInternal(m_actors[t]);
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
                reg->status = this->CreateRemoteInstance((RoRnet::ActorStreamRegister *)packet.buffer);
                RoR::Networking::AddPacket(0, RoRnet::MSG2_STREAM_REGISTER_RESULT, sizeof(RoRnet::StreamRegister), (char *)reg);
            }
        }
        else if (packet.header.command == RoRnet::MSG2_STREAM_REGISTER_RESULT)
        {
            RoRnet::StreamRegister* reg = (RoRnet::StreamRegister *)packet.buffer;
            for (int t = 0; t < m_free_actor_slot; t++)
            {
                if (!m_actors[t])
                    continue;
                if (m_actors[t]->ar_sim_state == Actor::SimState::NETWORKED_OK)
                    continue;
                if (m_actors[t]->ar_net_stream_id == reg->origin_streamid)
                {
                    int sourceid = packet.header.source;
                    m_actors[t]->ar_net_stream_results[sourceid] = reg->status;

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
                this->DeleteActorInternal(b);
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
            for (int t = 0; t < m_free_actor_slot; t++)
            {
                if (!m_actors[t])
                    continue;
                if (m_actors[t]->ar_sim_state != Actor::SimState::NETWORKED_OK)
                    continue;

                m_actors[t]->receiveStreamData(packet.header.command, packet.header.source, packet.header.streamid, packet.buffer, packet.header.size);
            }
        }
    }
}
#endif // USE_SOCKETW

int ActorManager::CheckNetworkStreamsOk(int sourceid)
{
    if (m_stream_mismatches[sourceid].size() > 0)
        return 0;

    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (!m_actors[t])
            continue;
        if (m_actors[t]->ar_sim_state != Actor::SimState::NETWORKED_OK)
            continue;

        if (m_actors[t]->ar_net_source_id == sourceid)
        {
            return 1;
        }
    }

    return 2;
}

int ActorManager::CheckNetRemoteStreamsOk(int sourceid)
{
    int result = 2;

    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (!m_actors[t])
            continue;
        if (m_actors[t]->ar_sim_state == Actor::SimState::NETWORKED_OK)
            continue;

        int stream_result = m_actors[t]->ar_net_stream_results[sourceid];
        if (stream_result == -1)
            return 0;
        if (stream_result == 1)
            result = 1;
    }

    return result;
}

Actor* ActorManager::GetActorByNetworkLinks(int source_id, int stream_id)
{
    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (!m_actors[t])
            continue;
        if (m_actors[t]->ar_sim_state != Actor::SimState::NETWORKED_OK)
            continue;

        if (m_actors[t]->ar_net_source_id == source_id && m_actors[t]->ar_net_stream_id == stream_id)
        {
            return m_actors[t];
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

void ActorManager::RecursiveActivation(int j, std::bitset<MAX_ACTORS>& visited)
{
    if (visited[j] || !m_actors[j] || m_actors[j]->ar_sim_state != Actor::SimState::LOCAL_SIMULATED)
        return;

    visited.set(j, true);

    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (t == j || !m_actors[t] || visited[t])
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

void ActorManager::UpdateSleepingState(Actor* player_actor, float dt)
{
    if (!m_forced_awake)
    {
        for (int t = 0; t < m_free_actor_slot; t++)
        {
            if (!m_actors[t])
                continue;
            if (m_actors[t]->ar_sim_state != Actor::SimState::LOCAL_SIMULATED)
                continue;
            if (m_actors[t]->getVelocity().squaredLength() > 0.01f)
                continue;

            m_actors[t]->ar_sleep_counter += dt;

            if (m_actors[t]->ar_sleep_counter >= 10.0f)
            {
                m_actors[t]->ar_sim_state = Actor::SimState::LOCAL_SLEEPING;
            }
        }
    }

    if (player_actor && player_actor->ar_sim_state == Actor::SimState::LOCAL_SLEEPING)
    {
        player_actor->ar_sim_state = Actor::SimState::LOCAL_SIMULATED;
    }

    std::bitset<MAX_ACTORS> visited;
    // Recursivly activate all actors which can be reached from current actor
    if (player_actor && player_actor->ar_sim_state == Actor::SimState::LOCAL_SIMULATED)
    {
        player_actor->ar_sleep_counter = 0.0f;
        this->RecursiveActivation(player_actor->ar_instance_id, visited);
    }
    // Snowball effect (activate all actors which might soon get hit by a moving actor)
    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (m_actors[t] && m_actors[t]->ar_sim_state == Actor::SimState::LOCAL_SIMULATED && m_actors[t]->ar_sleep_counter == 0.0f)
            this->RecursiveActivation(t, visited);
    }
}

int ActorManager::GetFreeActorSlot()
{
    // find a free slot for the actor
    for (int t = 0; t < MAX_ACTORS; t++)
    {
        if (!m_actors[t] && t >= m_free_actor_slot) // XXX: TODO: remove this hack
        {
            // reuse slots
            if (t >= m_free_actor_slot)
                m_free_actor_slot = t + 1;
            return t;
        }
    }
    return -1;
}

void ActorManager::WakeUpAllActors()
{
    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (m_actors[t] && m_actors[t]->ar_sim_state == Actor::SimState::LOCAL_SLEEPING)
        {
            m_actors[t]->ar_sim_state = Actor::SimState::LOCAL_SIMULATED;
            m_actors[t]->ar_sleep_counter = 0.0f;

            if (m_actors[m_simulated_actor])
            {
                m_actors[t]->ar_disable_aerodyn_turbulent_drag = (m_actors[m_simulated_actor]->ar_driveable == AIRPLANE);
            }
        }
    }
}

void ActorManager::SendAllActorsSleeping()
{
    m_forced_awake = false;
    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (m_actors[t] && m_actors[t]->ar_sim_state == Actor::SimState::LOCAL_SIMULATED)
        {
            m_actors[t]->ar_sim_state = Actor::SimState::LOCAL_SLEEPING;
        }
    }
}

void ActorManager::RecalcGravityMasses()
{
    // update the mass of all actors
    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (m_actors[t])
        {
            m_actors[t]->recalc_masses();
        }
    }
}

int ActorManager::FindActorInsideBox(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box)
{
    // try to find the desired actor (the one in the box)
    int id = -1;
    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (!m_actors[t])
            continue;
        if (collisions->isInside(m_actors[t]->ar_nodes[0].AbsPosition, inst, box))
        {
            if (id == -1)
            // first actor found
                id = t;
            else
            // second actor found -> unclear which one was meant
                return -1;
        }
    }
    return id;
}

void ActorManager::RepairActor(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box, bool keepPosition)
{
    int actor_id = this->FindActorInsideBox(collisions, inst, box);
    if (actor_id >= 0)
    {
        // take a position reference
        SOUND_PLAY_ONCE(actor_id, SS_TRIG_REPAIR);
        Vector3 ipos = m_actors[actor_id]->ar_nodes[0].AbsPosition;
        m_actors[actor_id]->RequestActorReset();
        m_actors[actor_id]->ResetPosition(ipos.x, ipos.z, false, 0);
        m_actors[actor_id]->updateVisual();
    }
}

void ActorManager::MuteAllActors()
{
    for (int i = 0; i < m_free_actor_slot; i++)
    {
        if (m_actors[i])
        {
            m_actors[i]->StopAllSounds();
        }
    }
}

void ActorManager::UnmuteAllActors()
{
    for (int i = 0; i < m_free_actor_slot; i++)
    {
        if (m_actors[i])
        {
            m_actors[i]->UnmuteAllSounds();
        }
    }
}

void ActorManager::RemoveActorByCollisionBox(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box)
{
    RemoveActorInternal(this->FindActorInsideBox(collisions, inst, box));
}

void ActorManager::RemoveActorInternal(int actor_id)
{
    if (actor_id < 0 || actor_id > m_free_actor_slot)
        return;

    if (!m_actors[actor_id])
        return;

    if (m_actors[actor_id]->ar_sim_state == Actor::SimState::NETWORKED_OK)
        return;

    this->DeleteActorInternal(m_actors[actor_id]);
}

void ActorManager::CleanUpAllActors() // Called after simulation finishes
{
    for (int i = 0; i < m_free_actor_slot; i++)
    {
        if (m_actors[i] == nullptr)
            continue; // This is how things currently work (but not for long...) ~ only_a_ptr, 01/2017

        delete m_actors[i];
        m_actors[i] = nullptr;
    }
}

void ActorManager::DeleteActorInternal(Actor* actor)
{
    if (actor == 0)
        return;

    this->SyncWithSimThread();

#ifdef USE_SOCKETW
    if (actor->ar_uses_networking && actor->ar_sim_state != Actor::SimState::NETWORKED_OK && actor->ar_sim_state != Actor::SimState::INVALID)
    {
        RoR::Networking::AddPacket(actor->ar_net_stream_id, RoRnet::MSG2_STREAM_UNREGISTER, 0, 0);
    }
#endif // USE_SOCKETW

    m_actors[actor->ar_instance_id] = 0;
    delete actor;


    RoR::App::GetGuiManager()->GetTopMenubar()->triggerUpdateVehicleList();

}

int FindPivotActorId(Actor* player, Actor* prev_player)
{
    if (player != nullptr)
    {
        return player->ar_instance_id;
    }
    else if (prev_player != nullptr)
    {
        return prev_player->ar_instance_id;
    }
    else
    {
        return -1;
    }
}

Actor* ActorManager::FetchNextVehicleOnList(Actor* player, Actor* prev_player)
{
    int pivot_index = FindPivotActorId(player, prev_player);

    for (int i = pivot_index + 1; i < m_free_actor_slot; i++)
    {
        if (m_actors[i] && m_actors[i]->ar_sim_state != Actor::SimState::NETWORKED_OK && !m_actors[i]->isPreloadedWithTerrain())
        {
            return m_actors[i];
        }
    }

    for (int i = 0; i < pivot_index; i++)
    {
        if (m_actors[i] && m_actors[i]->ar_sim_state != Actor::SimState::NETWORKED_OK && !m_actors[i]->isPreloadedWithTerrain())
        {
            return m_actors[i];
        }
    }

    if (pivot_index >= 0 && m_actors[pivot_index] && m_actors[pivot_index]->ar_sim_state != Actor::SimState::NETWORKED_OK && !m_actors[pivot_index]->isPreloadedWithTerrain())
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
        if (m_actors[i] && m_actors[i]->ar_sim_state != Actor::SimState::NETWORKED_OK && !m_actors[i]->isPreloadedWithTerrain())
        {
            return m_actors[i];
        }
    }

    for (int i = m_free_actor_slot - 1; i > pivot_index; i--)
    {
        if (m_actors[i] && m_actors[i]->ar_sim_state != Actor::SimState::NETWORKED_OK && !m_actors[i]->isPreloadedWithTerrain())
        {
            return m_actors[i];
        }
    }

    if (pivot_index >= 0 && m_actors[pivot_index] && m_actors[pivot_index]->ar_sim_state != Actor::SimState::NETWORKED_OK && !m_actors[pivot_index]->isPreloadedWithTerrain())
    {
        return m_actors[pivot_index];
    }

    return nullptr;
}

Actor* ActorManager::FetchRescueVehicle()
{
    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (m_actors[t] && m_actors[t]->ar_rescuer_flag)
        {
            return m_actors[t];
        }
    }
    return nullptr;
}

void ActorManager::UpdateFlexbodiesPrepare()
{
    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (m_actors[t] && m_actors[t]->ar_sim_state < Actor::SimState::LOCAL_SLEEPING)
        {
            m_actors[t]->UpdateFlexbodiesPrepare();
        }
    }
}

void ActorManager::JoinFlexbodyTasks()
{
    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (m_actors[t] && m_actors[t]->ar_sim_state < Actor::SimState::LOCAL_SLEEPING)
        {
            m_actors[t]->JoinFlexbodyTasks();
        }
    }
}

void ActorManager::UpdateFlexbodiesFinal()
{
    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (m_actors[t] && m_actors[t]->ar_sim_state < Actor::SimState::LOCAL_SLEEPING)
        {
            m_actors[t]->UpdateFlexbodiesFinal();
        }
    }
}

void ActorManager::UpdateActorVisuals(float dt,  Actor* player_actor)
{
    dt *= m_simulation_speed;

    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (!m_actors[t])
            continue;

        // always update the labels
        m_actors[t]->UpdateActorNetLabels(dt);

        if (m_actors[t]->ar_sim_state < Actor::SimState::LOCAL_SLEEPING)
        {
            m_actors[t]->updateVisual(dt);
            m_actors[t]->updateSkidmarks();
            m_actors[t]->updateFlares(dt, (m_actors[t] == player_actor));
        }
    }
}

void ActorManager::UpdateActors(Actor* player_actor, float dt)
{
    m_physics_frames++;

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

    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (!m_actors[t])
            continue;

        m_actors[t]->HandleResetRequests(dt);
        m_actors[t]->UpdateAngelScriptEvents(dt);

#ifdef USE_ANGELSCRIPT
        if (m_actors[t]->ar_vehicle_ai && (m_actors[t]->ar_vehicle_ai->IsActive()))
            m_actors[t]->ar_vehicle_ai->update(dt, 0);
#endif // USE_ANGELSCRIPT

        switch (m_actors[t]->ar_sim_state)
        {
        case Actor::SimState::NETWORKED_OK:
            m_actors[t]->CalcNetwork();
            break;

        case Actor::SimState::INVALID:
            break;

        default:
            if (m_actors[t]->ar_sim_state != Actor::SimState::LOCAL_SIMULATED && m_actors[t]->ar_engine)
                m_actors[t]->ar_engine->UpdateEngineSim(dt, 1);
            if (m_actors[t]->ar_sim_state < Actor::SimState::LOCAL_SLEEPING)
                m_actors[t]->UpdatePropAnimations(dt);
            if (m_actors[t]->ar_uses_networking)
            {
                if (m_actors[t]->ar_sim_state == Actor::SimState::LOCAL_SIMULATED)
                    m_actors[t]->sendStreamData();
                else if (m_actors[t]->ar_sim_state == Actor::SimState::LOCAL_SLEEPING && m_actors[t]->ar_net_timer.getMilliseconds() < 10000)
                // Also send update messages for 'Actor::SimState::LOCAL_SLEEPING' actors during the first 10 seconds of lifetime
                    m_actors[t]->sendStreamData();
                else if (m_actors[t]->ar_sim_state == Actor::SimState::LOCAL_SLEEPING && m_actors[t]->ar_net_timer.getMilliseconds() - m_actors[t]->ar_net_last_update_time > 5000)
                // Also send update messages for 'Actor::SimState::LOCAL_SLEEPING' actors periodically every 5 seconds
                    m_actors[t]->sendStreamData();
            }
            break;
        }
    }

    m_simulated_actor = (player_actor != nullptr) ? player_actor->ar_instance_id : 1;

    if (m_simulated_actor == -1)
    {
        for (int t = 0; t < m_free_actor_slot; t++)
        {
            if (m_actors[t] && m_actors[t]->ar_sim_state == Actor::SimState::LOCAL_SIMULATED)
            {
                m_simulated_actor = t;
                break;
            }
        }
    }

    if (m_simulated_actor >= 0 && m_simulated_actor < m_free_actor_slot)
    {
        if ((player_actor != nullptr) && (m_simulated_actor == player_actor->ar_instance_id))
        {

            m_actors[m_simulated_actor]->updateDashBoards(dt);

#ifdef FEAT_TIMING
            if (m_actors[m_simulated_actor]->statistics)     m_actors[m_simulated_actor]->statistics->frameStep(dt);
            if (m_actors[m_simulated_actor]->statistics_gfx) m_actors[m_simulated_actor]->statistics_gfx->frameStep(dt);
#endif // FEAT_TIMING
        }
        if (!m_actors[m_simulated_actor]->ReplayStep())
        {
            m_actors[m_simulated_actor]->ForceFeedbackStep(m_physics_steps);
            if (m_sim_thread_pool)
            {
                auto func = std::function<void()>([this]()
                    {
                        this->UpdatePhysicsSimulation();
                    });
                m_sim_task = m_sim_thread_pool->RunTask(func);
            }
            else
            {
                this->UpdatePhysicsSimulation();
            }
        }
    }
}

void ActorManager::NotifyActorsWindowResized()
{

    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (m_actors[t])
        {
            m_actors[t]->ar_dashboard->windowResized();
        }
    }

}

Actor* ActorManager::GetActorByIdInternal(int number)
{
    if (number >= 0 && number < m_free_actor_slot)
    {
        return m_actors[number];
    }
    return 0;
}

void ActorManager::UpdatePhysicsSimulation()
{
    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (!m_actors[t])
            continue;
        m_actors[t]->preUpdatePhysics(m_physics_steps * PHYSICS_DT);
    }
    if (gEnv->threadPool)
    {
        for (int i = 0; i < m_physics_steps; i++)
        {
            bool have_actors_to_simulate = false;

            {
                std::vector<std::function<void()>> tasks;
                for (int t = 0; t < m_free_actor_slot; t++)
                {
                    if (m_actors[t] && (m_actors[t]->ar_update_physics = m_actors[t]->CalcForcesEulerPrepare(i == 0, PHYSICS_DT, i, m_physics_steps)))
                    {
                        have_actors_to_simulate = true;
                        auto func = std::function<void()>([this, i, t]()
                            {
                                m_actors[t]->calcForcesEulerCompute(i == 0, PHYSICS_DT, i, m_physics_steps);
                                if (!m_actors[t]->ar_disable_self_collision)
                                {
                                    m_actors[t]->IntraPointCD()->UpdateIntraPoint(m_actors[t]);
                                    ResolveIntraActorCollisions(PHYSICS_DT,
                                        *(m_actors[t]->IntraPointCD()),
                                        m_actors[t]->ar_num_collcabs,
                                        m_actors[t]->ar_collcabs,
                                        m_actors[t]->ar_cabs,
                                        m_actors[t]->ar_intra_collcabrate,
                                        m_actors[t]->ar_nodes,
                                        m_actors[t]->ar_collision_range,
                                        *(m_actors[t]->ar_submesh_ground_model));
                                }
                            });
                        tasks.push_back(func);
                    }
                }
                gEnv->threadPool->Parallelize(tasks);
            }

            for (int t = 0; t < m_free_actor_slot; t++)
            {
                if (m_actors[t] && m_actors[t]->ar_update_physics)
                    m_actors[t]->calcForcesEulerFinal(i == 0, PHYSICS_DT, i, m_physics_steps);
            }

            if (have_actors_to_simulate)
            {
                std::vector<std::function<void()>> tasks;
                for (int t = 0; t < m_free_actor_slot; t++)
                {
                    if (m_actors[t] && m_actors[t]->ar_update_physics && !m_actors[t]->ar_disable_actor2actor_collision)
                    {
                        auto func = std::function<void()>([this, t]()
                            {
                                m_actors[t]->InterPointCD()->UpdateInterPoint(m_actors[t], m_actors, m_free_actor_slot);
                                if (m_actors[t]->ar_collision_relevant)
                                {
                                    ResolveInterActorCollisions(PHYSICS_DT,
                                        *(m_actors[t]->InterPointCD()),
                                        m_actors[t]->ar_num_collcabs,
                                        m_actors[t]->ar_collcabs,
                                        m_actors[t]->ar_cabs,
                                        m_actors[t]->ar_inter_collcabrate,
                                        m_actors[t]->ar_nodes,
                                        m_actors[t]->ar_collision_range,
                                        *(m_actors[t]->ar_submesh_ground_model));
                                }
                            });
                        tasks.push_back(func);
                    }
                }
                gEnv->threadPool->Parallelize(tasks);
            }
        }
    }
    else
    {
        for (int i = 0; i < m_physics_steps; i++)
        {
            bool have_actors_to_simulate = false;

            for (int t = 0; t < m_free_actor_slot; t++)
            {
                if (m_actors[t] && (m_actors[t]->ar_update_physics = m_actors[t]->CalcForcesEulerPrepare(i == 0, PHYSICS_DT, i, m_physics_steps)))
                {
                    have_actors_to_simulate = true;

                    m_actors[t]->calcForcesEulerCompute(i == 0, PHYSICS_DT, i, m_physics_steps);
                    m_actors[t]->calcForcesEulerFinal(i == 0, PHYSICS_DT, i, m_physics_steps);
                    if (!m_actors[t]->ar_disable_self_collision)
                    {
                        m_actors[t]->IntraPointCD()->UpdateIntraPoint(m_actors[t]);
                        ResolveIntraActorCollisions(PHYSICS_DT,
                            *(m_actors[t]->IntraPointCD()),
                            m_actors[t]->ar_num_collcabs,
                            m_actors[t]->ar_collcabs,
                            m_actors[t]->ar_cabs,
                            m_actors[t]->ar_intra_collcabrate,
                            m_actors[t]->ar_nodes,
                            m_actors[t]->ar_collision_range,
                            *(m_actors[t]->ar_submesh_ground_model));
                    }
                }
            }

            if (have_actors_to_simulate)
            {
                BES_START(BES_CORE_Contacters);
                for (int t = 0; t < m_free_actor_slot; t++)
                {
                    if (m_actors[t] && m_actors[t]->ar_update_physics && !m_actors[t]->ar_disable_actor2actor_collision)
                    {
                        m_actors[t]->InterPointCD()->UpdateInterPoint(m_actors[t], m_actors, m_free_actor_slot);
                        if (m_actors[t]->ar_collision_relevant)
                        {
                            ResolveInterActorCollisions(
                                PHYSICS_DT,
                                *(m_actors[t]->InterPointCD()),
                                m_actors[t]->ar_num_collcabs,
                                m_actors[t]->ar_collcabs,
                                m_actors[t]->ar_cabs,
                                m_actors[t]->ar_inter_collcabrate,
                                m_actors[t]->ar_nodes,
                                m_actors[t]->ar_collision_range,
                                *(m_actors[t]->ar_submesh_ground_model));
                        }
                    }
                }
                BES_STOP(BES_CORE_Contacters);
            }
        }
    }
    for (int t = 0; t < m_free_actor_slot; t++)
    {
        if (!m_actors[t])
            continue;
        if (!m_actors[t]->ar_update_physics)
            continue;
        m_actors[t]->postUpdatePhysics(m_physics_steps * PHYSICS_DT);
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

#define FETCHACTORDEF_PROF_CHECKPOINT(_ENTRYNAME_) \
    { if (prof != nullptr) { prof->Checkpoint(RoR::RigLoadingProfiler::_ENTRYNAME_); } }

std::shared_ptr<RigDef::File> ActorManager::FetchActorDef(const char* filename, bool predefined_on_terrain)
{
    // First check the loaded defs
    auto search_res = m_actor_defs.find(filename);
    if (search_res != m_actor_defs.end())
    {
        return search_res->second;
    }

    // Load the 'truckfile'
    try
    {
        Ogre::String resource_filename = filename;
        Ogre::String resource_groupname;
        RoR::App::GetCacheSystem()->checkResourceLoaded(resource_filename, resource_groupname); // Validates the filename and finds resource group
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

        m_actor_defs.insert(std::make_pair(filename, def));
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
