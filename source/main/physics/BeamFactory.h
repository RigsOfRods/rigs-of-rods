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

#include "RoRPrerequisites.h"

#include "BeamData.h"
#include "CmdKeyInertia.h"
#include "Network.h"
#include "RigDef_Prerequisites.h"

#include <string>
#include <vector>

#define PHYSICS_DT App::diag_physics_dt->GetActiveVal<float>()

class ThreadPool;

namespace RoR {

/// Builds and manages softbody actors. Manage physics and threading.
/// TODO: Currently also manages gfx, which should be done by GfxActor
/// HISTORICAL NOTE: Until 01/2018, this class was named `BeamFactory` (because `Actor` was `Beam`)
class ActorManager
{
    friend class GameScript; // needs to call RemoveActorByCollisionBox()
public:

    ActorManager();
    ~ActorManager();

    Actor*         CreateActorInstance(ActorSpawnRequest rq, std::shared_ptr<RigDef::File> def);
    void           UpdateActors(Actor* player_actor, float dt);
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
    void           NotifyActorsWindowResized();
    void           MuteAllActors();
    void           UnmuteAllActors();
    void           SetTrucksForcedAwake(bool forced)       { m_forced_awake = forced; };
    bool           AreTrucksForcedAwake() const            { return m_forced_awake; }
    void           SetSimulationSpeed(float speed)         { m_simulation_speed = std::max(0.0f, speed); };
    float          GetSimulationSpeed() const              { return m_simulation_speed; };
    RoR::CmdKeyInertiaConfig& GetInertiaConfig()           { return m_inertia_config; }
    Actor*         FetchNextVehicleOnList(Actor* player, Actor* prev_player);
    Actor*         FetchPreviousVehicleOnList(Actor* player, Actor* prev_player);
    Actor*         FetchRescueVehicle();
    void           CleanUpAllActors(); //!< Call this after simulation loop finishes.
    Actor*         GetActorByNetworkLinks(int source_id, int stream_id); // used by character
    void           RepairActor(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box, bool keepPosition = false);
    void           UpdateSleepingState(Actor* player_actor, float dt);
    void           DeleteActorInternal(Actor* b);      //!< DO NOT CALL DIRECTLY! Use `SimController` for public interface
    Actor*         GetActorByIdInternal(int actor_id); //!< DO NOT CALL DIRECTLY! Use `SimController` for public interface
    Actor*         FindActorInsideBox(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box);
    std::shared_ptr<RigDef::File>   FetchActorDef(RoR::ActorSpawnRequest& rq);

#ifdef USE_SOCKETW
    void           HandleActorStreamData(std::vector<RoR::Networking::recv_packet_t> packet);
#endif

#ifdef USE_ANGELSCRIPT
    void           AddRef() {};  // we have to add this to be able to use the class as reference inside scripts
    void           Release() {};
#endif

    Ogre::String   GetQuicksaveFilename(Ogre::String filename = "");
    Ogre::String   ExtractSceneName(Ogre::String filename);
    bool           LoadScene(Ogre::String filename);
    bool           SaveScene(Ogre::String filename);

    std::vector<Actor*> GetActors() const                  { return m_actors; };
    std::vector<Actor*> GetLocalActors();

    std::pair<Actor*, float> GetNearestActor(Ogre::Vector3 position);

    // A list of all beams interconnecting two actors
    std::map<beam_t*, std::pair<Actor*, Actor*>> inter_actor_links;

private:

    void           SetupActor(Actor* actor, ActorSpawnRequest rq, std::shared_ptr<RigDef::File> def);
    bool           CheckActorCollAabbIntersect(int a, int b);    //!< Returns whether or not the bounding boxes of truck a and truck b intersect. Based on the truck collision bounding boxes.
    bool           PredictActorCollAabbIntersect(int a, int b);  //!< Returns whether or not the bounding boxes of truck a and truck b might intersect during the next framestep. Based on the truck collision bounding boxes.
    void           RemoveStreamSource(int sourceid);
    void           RecursiveActivation(int j, std::vector<bool>& visited);
    void           ForwardCommands(Actor* source_actor); //< Fowards things to trailers
    std::shared_ptr<RigDef::File>  LoadActorDef(std::string const& filename, std::string const& rg_name);

    std::map<int, std::set<int>> m_stream_mismatches; //!< Networking: A set of streams without a corresponding actor in the actor-array for each stream source
    std::map<int, int> m_stream_time_offsets; //!< Networking: A network time offset for each stream source
    std::unique_ptr<ThreadPool>     m_sim_thread_pool;
    std::shared_ptr<Task>           m_sim_task;
    std::vector<Actor*>             m_actors;
    Ogre::Timer                     m_net_timer;
    RoR::CmdKeyInertiaConfig        m_inertia_config;
    bool            m_forced_awake;      //!< disables sleep counters
    int             m_physics_steps;
    float           m_dt_remainder;     ///< Keeps track of the rounding error in the time step calculation
    float           m_simulation_speed; ///< slow motion < 1.0 < fast motion
};

} // namespace RoR
