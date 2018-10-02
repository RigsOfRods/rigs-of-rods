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
#include "Network.h"
#include "Singleton.h"

#include <string>
#include <vector>

#define PHYSICS_DT 0.0005 // fixed dt of 0.5 ms

class ThreadPool;

namespace RoR {

struct ActorSpawnRequest
{
    ActorSpawnRequest();

    std::string       asr_filename;
    std::vector<Ogre::String> asr_config;
    Ogre::Vector3     asr_position;
    Ogre::Quaternion  asr_rotation;
    int               asr_cache_entry_num;
    collision_box_t*  asr_spawnbox;
    RoR::SkinDef*     asr_skin;
    bool              asr_free_position;
    bool              asr_loaded_with_terrn;
};

/// Builds and manages softbody actors. Manage physics and threading.
/// TODO: Currently also manages gfx, which should be done by GfxActor
/// HISTORICAL NOTE: Until 01/2018, this class was named `BeamFactory` (because `Actor` was `Beam`)
class ActorManager
{
    friend class GameScript; // needs to call RemoveActorByCollisionBox()
public:

    ActorManager();
    ~ActorManager();

    /// @param cache_entry_number Needed for flexbody caching. Pass -1 if unavailable (flexbody caching will be disabled)
    Actor* CreateLocalActor(
        Ogre::Vector3 pos,
        Ogre::Quaternion rot,
        Ogre::String fname,
        int cache_entry_number = -1,
        collision_box_t* spawnbox = NULL,
        const std::vector<Ogre::String>* actorconfig = nullptr,
        RoR::SkinDef* skin = nullptr,
        bool freePosition = false,
        bool preloaded_with_terrain = false
    );

    void           UpdateActors(Actor* player_actor, float dt);
    void           SyncWithSimThread();
    void           UpdatePhysicsSimulation();
    void           WakeUpAllActors();
    void           SendAllActorsSleeping();
    int            CheckNetworkStreamsOk(int sourceid);
    int            CheckNetRemoteStreamsOk(int sourceid);
    void           NotifyActorsWindowResized();
    void           RecalcGravityMasses();
    void           MuteAllActors();
    void           UnmuteAllActors();
    void           SetTrucksForcedAwake(bool forced)       { m_forced_awake = forced; };
    bool           AreTrucksForcedAwake() const            { return m_forced_awake; }
    void           SetSimulationSpeed(float speed)         { m_simulation_speed = std::max(0.0f, speed); };
    float          GetSimulationSpeed() const              { return m_simulation_speed; };
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

#ifdef USE_SOCKETW
    void           HandleActorStreamData(std::vector<RoR::Networking::recv_packet_t> packet);
#endif

#ifdef USE_ANGELSCRIPT
    void           AddRef() {};  // we have to add this to be able to use the class as reference inside scripts
    void           Release() {};
#endif

    std::vector<Actor*> GetActors() const                  { return m_actors; };

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
    int            GetActorIndex(Actor* actor);         //!< Returns the m_actors index of the actor (-1 if not found)
    void           RecursiveActivation(int j, std::vector<bool>& visited);
    std::shared_ptr<RigDef::File>   FetchActorDef(const char* filename, bool predefined_on_terrain = false);

    std::map<std::string, std::shared_ptr<RigDef::File>>   m_actor_defs;
    std::map<int, std::vector<int>> m_stream_mismatches; //!< Networking: A list of streams without a corresponding actor in the actor-array for each stream source
    std::unique_ptr<ThreadPool>     m_sim_thread_pool;
    std::shared_ptr<Task>           m_sim_task;
    std::vector<Actor*>             m_actors;
    bool            m_forced_awake;      //!< disables sleep counters
    unsigned long   m_physics_frames;
    int             m_physics_steps;
    float           m_dt_remainder;     ///< Keeps track of the rounding error in the time step calculation
    float           m_simulation_speed; ///< slow motion < 1.0 < fast motion
    int             m_actor_counter;    ///< Used to generate unique actor ids
};

} // namespace RoR
