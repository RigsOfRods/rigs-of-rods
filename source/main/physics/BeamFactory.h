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

#include "Beam.h"
#include "DustManager.h" // Particle systems manager
#include "Network.h"
#include "Singleton.h"

#define PHYSICS_DT 0.0005 // fixed dt of 0.5 ms

class ThreadPool;

namespace RoR {

/// Builds and manages softbody actors. Manage physics and threading.
/// TODO: Currently also manages gfx, which should be done by GfxActor
/// HISTORICAL NOTE: Until 01/2018, this class was named `BeamFactory` (because `Actor` was `Beam`)
class ActorManager
{
    friend class GameScript; // needs to call RemoveActorByCollisionBox()
public:

    ActorManager(RoRFrameListener* sim_controller);
    ~ActorManager();

    /// @param cache_entry_number Needed for flexbody caching. Pass -1 if unavailable (flexbody caching will be disabled)
    Actor* CreateLocalActor(
        Ogre::Vector3 pos,
        Ogre::Quaternion rot,
        Ogre::String fname,
        int cache_entry_number = -1,
        collision_box_t* spawnbox = NULL,
        bool ismachine = false,
        const std::vector<Ogre::String>* actorconfig = nullptr,
        RoR::SkinDef* skin = nullptr,
        bool freePosition = false,
        bool preloaded_with_terrain = false
    );

    void           UpdateActors(Actor* player_actor, float dt);
    void           SyncWithSimThread();
    void           UpdatePhysicsSimulation();
    void           UpdateActorVisuals(float dt, Actor* player_actor); // TODO: This should be done by GfxActor
    void           WakeUpAllActors();
    void           SendAllActorsSleeping();
    int            CheckNetworkStreamsOk(int sourceid);
    int            CheckNetRemoteStreamsOk(int sourceid);
    void           NotifyActorsWindowResized();
    void           RecalcGravityMasses();
    void           MuteAllActors();
    void           UnmuteAllActors();
    void           JoinFlexbodyTasks(); /// Waits until all flexbody tasks are finished, but does not update the hardware buffers
    void           UpdateFlexbodiesPrepare();
    void           UpdateFlexbodiesFinal();
    DustManager&   GetParticleManager()                    { return m_particle_manager; }
    void           SetTrucksForcedAwake(bool forced)       { m_forced_awake = forced; };
    int            GetNumUsedActorSlots() const            { return m_free_actor_slot; }; // TODO: Tasks requiring search over all actors should be done internally. ~ only_a_ptr, 01/2018
    Actor**        GetInternalActorSlots()                 { return m_actors; }; // TODO: Tasks requiring search over all actors should be done internally. ~ only_a_ptr, 01/2018
    void           SetSimulationSpeed(float speed)         { m_simulation_speed = std::max(0.0f, speed); };
    float          GetSimulationSpeed() const              { return m_simulation_speed; };
    Actor*         FetchNextVehicleOnList(Actor* player, Actor* prev_player);
    Actor*         FetchPreviousVehicleOnList(Actor* player, Actor* prev_player);
    Actor*         FetchRescueVehicle();
    void           CleanUpAllActors(); //!< Call this after simulation loop finishes.
    Actor*         GetActorByNetworkLinks(int source_id, int stream_id); // used by character
    void           RepairActor(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box, bool keepPosition = false);
    void           UpdateSleepingState(Actor* player_actor, float dt);
    void           RemoveActorByCollisionBox(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box); //!< Only for scripting
    void           RemoveActorInternal(int actor_id); //!< DO NOT CALL DIRECTLY! Use `RoRFrameListener` for public interface
    Actor*         GetActorByIdInternal(int number); //!< DO NOT CALL DIRECTLY! Use `RoRFrameListener` for public interface

#ifdef USE_SOCKETW
    void           HandleActorStreamData(std::vector<RoR::Networking::recv_packet_t> packet);
#endif

#ifdef USE_ANGELSCRIPT
    void           AddRef() {};  // we have to add this to be able to use the class as reference inside scripts
    void           Release() {};
#endif

    // A list of all beams interconnecting two actors
    std::map<beam_t*, std::pair<Actor*, Actor*>> inter_actor_links;

private:

    void           SetupActor(Actor* actor,
                              std::shared_ptr<RigDef::File> def,
                              Ogre::Vector3 const& spawn_position,
                              Ogre::Quaternion const& spawn_rotation,
                              collision_box_t* spawn_box,
                              bool free_positioned,
                              bool _networked,
                              int cache_entry_number = -1);
    bool           CheckAabbIntersection(Ogre::AxisAlignedBox a, Ogre::AxisAlignedBox b, float scale = 1.0f); //!< Returns whether or not the two (scaled) bounding boxes intersect.
    bool           CheckActorAabbIntersection(int a, int b, float scale = 1.0f);     //!< Returns whether or not the bounding boxes of truck a and truck b intersect. Based on the default truck bounding boxes.
    bool           PredictActorAabbIntersection(int a, int b, float scale = 1.0f);   //!< Returns whether or not the bounding boxes of truck a and truck b might intersect during the next framestep. Based on the default truck bounding boxes.
    bool           CheckActorCollAabbIntersect(int a, int b, float scale = 1.0f);    //!< Returns whether or not the bounding boxes of truck a and truck b intersect. Based on the truck collision bounding boxes.
    bool           PredictActorCollAabbIntersect(int a, int b, float scale = 1.0f);  //!< Returns whether or not the bounding boxes of truck a and truck b might intersect during the next framestep. Based on the truck collision bounding boxes.
    int            CreateRemoteInstance(RoRnet::ActorStreamRegister* reg);
    void           RemoveStreamSource(int sourceid);
    void           LogParserMessages();
    void           LogSpawnerMessages();
    void           RecursiveActivation(int j, std::bitset<MAX_ACTORS>& visited);
    int            GetFreeActorSlot();
    int            FindActorInsideBox(Collisions* collisions, const Ogre::String& inst, const Ogre::String& box);
    void           DeleteActorInternal(Actor* b);
    std::shared_ptr<RigDef::File>   FetchActorDef(const char* filename, bool predefined_on_terrain = false);

    std::map<std::string, std::shared_ptr<RigDef::File>>   m_actor_defs;
    std::map<int, std::vector<int>> m_stream_mismatches; //!< Networking: A list of streams without a corresponding actor in the actor-array for each stream source
    std::unique_ptr<ThreadPool>     m_sim_thread_pool;
    std::shared_ptr<Task>           m_sim_task;
    RoRFrameListener*               m_sim_controller;
    int             m_num_cpu_cores;
    Actor*          m_actors[MAX_ACTORS];//!< All actors; slots are not reused
    int             m_free_actor_slot;   //!< Slots are not reused
    int             m_simulated_actor;   //!< A player actor if present, or any other local actor if present.
    bool            m_forced_awake;      //!< disables sleep counters
    unsigned long   m_physics_frames;
    int             m_physics_steps;
    float           m_dt_remainder;     ///< Keeps track of the rounding error in the time step calculation
    float           m_simulation_speed; ///< slow motion < 1.0 < fast motion
    DustManager     m_particle_manager;
};

} // namespace RoR
