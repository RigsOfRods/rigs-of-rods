/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#pragma once

#include "Application.h"

#include "SimData.h"
#include "CmdKeyInertia.h"
#include "Network.h"
#include "TruckFileFormat.h"
#include "ThreadPool.h"

#include <string>
#include <vector>

#define PHYSICS_DT App::diag_physics_dt->GetFloat()

namespace RoR {

/// Builds and manages softbody actors (physics on background thread, networking)
class ActorManager
{
public:

    ActorManager();
    ~ActorManager();

    Actor*         CreateActorInstance(ActorSpawnRequest rq, Truck::DocumentPtr def);
    void           UpdateActors(Actor* player_actor);
    void           SyncWithSimThread();
    void           UpdatePhysicsSimulation();
    void           WakeUpAllActors();
    void           SendAllActorsSleeping();
    unsigned long  GetNetTime()                            { return m_net_timer.getMilliseconds(); };
    int            GetNetTimeOffset(int sourceid);
    void           UpdateNetTimeOffset(int sourceid, int offset);
    void           AddStreamMismatch(int sourceid, int streamid) { m_stream_mismatches[sourceid].insert(streamid); };
    int            CheckNetworkStreamsOk(int sourceid);
    int            CheckNetRemoteStreamsOk(int sourceid);
    void           MuteAllActors();
    void           UnmuteAllActors();
    void           SetTrucksForcedAwake(bool forced)       { m_forced_awake = forced; };
    bool           AreTrucksForcedAwake() const            { return m_forced_awake; }
    void           SetSimulationSpeed(float speed)         { m_simulation_speed = std::max(0.0f, speed); };
    float          GetSimulationSpeed() const              { return m_simulation_speed; };
    bool           IsSimulationPaused() const              { return m_simulation_paused; }
    void           SetSimulationPaused(bool v)             { m_simulation_paused = v; }
    float          GetTotalTime() const                    { return m_total_sim_time; }
    RoR::CmdKeyInertiaConfig& GetInertiaConfig()           { return m_inertia_config; }
    Actor*         FetchNextVehicleOnList(Actor* player, Actor* prev_player);
    Actor*         FetchPreviousVehicleOnList(Actor* player, Actor* prev_player);
    Actor*         FetchRescueVehicle();
    void           CleanUpSimulation(); //!< Call this after simulation loop finishes.
    Actor*         GetActorByNetworkLinks(int source_id, int stream_id); // used by character
    void           RepairActor(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box, bool keepPosition = false);
    void           UpdateSleepingState(Actor* player_actor, float dt);
    void           DeleteActorInternal(Actor* b); //!< Use `GameContext::DeleteActor()`
    Actor*         GetActorById(int actor_id);
    Actor*         FindActorInsideBox(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box);
    void           UpdateInputEvents(float dt);

    // Truck file handling
    void               ExportTruckDocument(Truck::DocumentPtr def, std::string filename, std::string rg_name);
    Truck::DocumentPtr FetchTruckDocument(RoR::ActorSpawnRequest& rq);

#ifdef USE_SOCKETW
    void           HandleActorStreamData(std::vector<RoR::NetRecvPacket> packet);
#endif

#ifdef USE_ANGELSCRIPT
    void           AddRef() {};  // we have to add this to be able to use the class as reference inside scripts
    void           Release() {};
#endif

    // Savegames (defined in Savegame.cpp)

    bool           LoadScene(Ogre::String filename);
    bool           SaveScene(Ogre::String filename);
    void           RestoreSavedState(Actor* actor, rapidjson::Value const& j_entry);

    std::vector<Actor*> GetActors() const                  { return m_actors; };
    std::vector<Actor*> GetLocalActors();

    std::pair<Actor*, float> GetNearestActor(Ogre::Vector3 position);

    // A list of all beams interconnecting two actors
    std::map<beam_t*, std::pair<Actor*, Actor*>> inter_actor_links;

private:

    void           SetupActor(Actor* actor, ActorSpawnRequest rq, Truck::DocumentPtr def);
    bool           CheckActorCollAabbIntersect(int a, int b);    //!< Returns whether or not the bounding boxes of truck a and truck b intersect. Based on the truck collision bounding boxes.
    bool           PredictActorCollAabbIntersect(int a, int b);  //!< Returns whether or not the bounding boxes of truck a and truck b might intersect during the next framestep. Based on the truck collision bounding boxes.
    void           RemoveStreamSource(int sourceid);
    void           RecursiveActivation(int j, std::vector<bool>& visited);
    void           ForwardCommands(Actor* source_actor); //!< Fowards things to trailers
    void           UpdateTruckFeatures(Actor* vehicle, float dt);
    Truck::DocumentPtr  LoadTruckDocument(std::string const& filename, std::string const& rg_name);

    // Networking
    std::map<int, std::set<int>> m_stream_mismatches; //!< Networking: A set of streams without a corresponding actor in the actor-array for each stream source
    std::map<int, int>  m_stream_time_offsets;       //!< Networking: A network time offset for each stream source
    Ogre::Timer         m_net_timer;

    // Physics
    std::vector<Actor*> m_actors;
    bool                m_forced_awake           = false; //!< disables sleep counters
    int                 m_physics_steps          = 0;
    float               m_dt_remainder           = 0.f;   //!< Keeps track of the rounding error in the time step calculation
    float               m_simulation_speed       = 1.f;   //!< slow motion < 1.0 < fast motion
    float               m_last_simulation_speed  = 0.1f;  //!< previously used time ratio between real time (evt.timeSinceLastFrame) and physics time ('dt' used in calcPhysics)
    float               m_simulation_time        = 0.f;   //!< Amount of time the physics simulation is going to be advanced
    bool                m_simulation_paused      = false;
    float               m_total_sim_time         = 0.f;

    // Utils
    std::unique_ptr<ThreadPool> m_sim_thread_pool;
    std::shared_ptr<Task>       m_sim_task;
    RoR::CmdKeyInertiaConfig    m_inertia_config;
};

} // namespace RoR
