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
#include "RigDef_Prerequisites.h"
#include "ThreadPool.h"

#include <string>
#include <vector>

namespace RoR {

/// @addtogroup Physics
/// @{

/// Builds and manages softbody actors (physics on background thread, networking)
class ActorManager
{
public:

    typedef std::vector<FreeForce> FreeForceVec_t;

    ActorManager();
    ~ActorManager();

    /// @name Actor lifetime
    /// @{
    ActorPtr       CreateNewActor(ActorSpawnRequest rq, RigDef::DocumentPtr def);
    void           DeleteActorInternal(ActorPtr actor); //!< Do not call directly; use `GameContext::DeleteActor()`
    /// @}

    /// @name Actor lookup
    /// @{
    const ActorPtr& GetActorById(ActorInstanceID_t actor_id);
    const ActorPtr& GetActorByNetworkLinks(int source_id, int stream_id); // used by character
    ActorPtr        FindActorInsideBox(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box);
    const ActorPtr& FetchNextVehicleOnList(ActorPtr player, ActorPtr prev_player);
    const ActorPtr& FetchPreviousVehicleOnList(ActorPtr player, ActorPtr prev_player);
    const ActorPtr& FetchRescueVehicle();
    /// @}

    /// @name Free forces
    /// @{
    void           AddFreeForce(FreeForceRequest* rq);
    void           ModifyFreeForce(FreeForceRequest* rq);
    void           RemoveFreeForce(FreeForceID_t id);
    FreeForceVec_t::iterator FindFreeForce(FreeForceID_t id);
    FreeForceID_t  GetFreeForceNextId() { return m_free_force_next_id++; }
    /// @}

    void           UpdateActors(ActorPtr player_actor);
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
    

    void           CleanUpSimulation(); //!< Call this after simulation loop finishes.

    void           RepairActor(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box, bool keepPosition = false);
    void           UpdateSleepingState(ActorPtr player_actor, float dt);
    

    void           UpdateInputEvents(float dt);

    // Truck file handling
    RigDef::DocumentPtr   FetchActorDef(std::string filename, bool predefined_on_terrain = false);
    void                  ExportActorDef(RigDef::DocumentPtr def, std::string filename, std::string rg_name);

#ifdef USE_SOCKETW
    void           HandleActorStreamData(std::vector<RoR::NetRecvPacket> packet);
#endif

    // Savegames (defined in Savegame.cpp)

    bool           LoadScene(Ogre::String filename);
    bool           SaveScene(Ogre::String filename);
    void           RestoreSavedState(ActorPtr actor, rapidjson::Value const& j_entry);

    ActorPtrVec& GetActors() { return m_actors; };
    std::vector<ActorPtr> GetLocalActors();

    std::pair<ActorPtr, float> GetNearestActor(Ogre::Vector3 position);

    std::map<beam_t*, std::pair<ActorPtr, ActorPtr>> inter_actor_links;
    bool AreActorsDirectlyLinked(const ActorPtr& a1, const ActorPtr& a2);

    static const ActorPtr ACTORPTR_NULL; // Dummy value to be returned as const reference.

private:

    bool           CheckActorCollAabbIntersect(int a, int b);    //!< Returns whether or not the bounding boxes of truck a and truck b intersect. Based on the truck collision bounding boxes.
    bool           PredictActorCollAabbIntersect(int a, int b);  //!< Returns whether or not the bounding boxes of truck a and truck b might intersect during the next framestep. Based on the truck collision bounding boxes.
    void           RemoveStreamSource(int sourceid);
    void           RecursiveActivation(int j, std::vector<bool>& visited);
    void           ForwardCommands(ActorPtr source_actor); //!< Fowards things to trailers
    void           UpdateTruckFeatures(ActorPtr vehicle, float dt);
    void           CalcFreeForces();                             //!< Apply FreeForces - intentionally as a separate pass over all actors

    // Networking
    std::map<int, std::set<int>> m_stream_mismatches; //!< Networking: A set of streams without a corresponding actor in the actor-array for each stream source
    std::map<int, int>  m_stream_time_offsets;       //!< Networking: A network time offset for each stream source
    Ogre::Timer         m_net_timer;

    // Physics
    ActorPtrVec         m_actors;
    bool                m_forced_awake           = false; //!< disables sleep counters
    int                 m_physics_steps          = 0;
    float               m_dt_remainder           = 0.f;   //!< Keeps track of the rounding error in the time step calculation
    float               m_simulation_speed       = 1.f;   //!< slow motion < 1.0 < fast motion
    float               m_last_simulation_speed  = 0.1f;  //!< previously used time ratio between real time (evt.timeSinceLastFrame) and physics time ('dt' used in calcPhysics)
    float               m_simulation_time        = 0.f;   //!< Amount of time the physics simulation is going to be advanced
    bool                m_simulation_paused      = false;
    float               m_total_sim_time         = 0.f;
    FreeForceVec_t      m_free_forces;                    //!< Global forces added ad-hoc by scripts
    FreeForceID_t       m_free_force_next_id     = 0;     //!< Unique ID for each FreeForce

    // Utils
    std::unique_ptr<ThreadPool> m_sim_thread_pool;
    std::shared_ptr<Task>       m_sim_task;
    RoR::CmdKeyInertiaConfig    m_inertia_config;
};

/// @} // addtogroup Physics

} // namespace RoR
