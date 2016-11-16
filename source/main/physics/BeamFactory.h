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
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 24th of August 2009

#pragma once
#ifndef __BeamFactory_H_
#define __BeamFactory_H_

#include "RoRPrerequisites.h"

#include "Beam.h"
#include "DustManager.h" // Particle systems manager
#include "Network.h"
#include "Singleton.h"

#define PHYSICS_DT 0.0005 // fixed dt of 0.5 ms

class ThreadPool;

/**
* Builds and manages vehicles; Manages multithreading.
*/
class BeamFactory : public RoRSingleton< BeamFactory >, public ZeroedMemoryAllocator
{
public:

    BeamFactory();
    ~BeamFactory();

    /**
    * @param cache_entry_number Needed for flexbody caching. Pass -1 if unavailable (flexbody caching will be disabled)
    */
    Beam* CreateLocalRigInstance(
        Ogre::Vector3 pos, 
        Ogre::Quaternion rot, 
        Ogre::String fname,
        int cache_entry_number = -1, 
        collision_box_t *spawnbox = NULL, 
        bool ismachine = false, 
        const std::vector<Ogre::String> *truckconfig = nullptr, 
        Skin *skin = nullptr, 
        bool freePosition = false,
        bool preloaded_with_terrain = false
        );
    
    void update(float dt);

#ifdef USE_SOCKETW
    void handleStreamData(std::vector<RoR::Networking::recv_packet_t> packet);
#endif // USE_SOCKETW
    int checkStreamsOK(int sourceid);
    int checkStreamsRemoteOK(int sourceid);

    int getNumCpuCores() { return m_num_cpu_cores; };

    Beam *getBeam(int source_id, int stream_id); // used by character

    Beam *getCurrentTruck();
    Beam *getTruck(int number);
    Beam **getTrucks() { return m_trucks; };
    int getPreviousTruckNumber() { return m_previous_truck; };
    int getCurrentTruckNumber() { return m_current_truck; };
    int getTruckCount() { return m_free_truck; };

    void enterNextTruck();
    void enterPreviousTruck();
    void setCurrentTruck(int new_truck);

    void setSimulationSpeed(float speed) { m_simulation_speed = std::max(0.0f, speed); };
    float getSimulationSpeed() { return m_simulation_speed; };

    void removeCurrentTruck();
    void removeAllTrucks();
    void removeTruck(Collisions *collisions, const Ogre::String &inst, const Ogre::String &box);
    void removeTruck(int truck);
    
    void MuteAllTrucks();
    void UnmuteAllTrucks();

    bool enterRescueTruck();
    void repairTruck(Collisions *collisions, const Ogre::String &inst, const Ogre::String &box, bool keepPosition=false);

    /**
    * TIGHT-LOOP; Logic: display, particles, sound; 
    */
    void updateVisual(float dt);

    /**
    * TIGHT-LOOP; Logic: flexbodies 
    */
    void updateFlexbodiesPrepare();
    void updateFlexbodiesFinal();

    /**
     * Waits until all flexbody tasks are finished, but does not update the hardware buffers
     */
    void joinFlexbodyTasks();

    void UpdatePhysicsSimulation();

    inline unsigned long getPhysFrame() { return m_physics_frames; };

    void recalcGravityMasses();

    void activateAllTrucks();
    void sendAllTrucksSleeping();
    void setTrucksForcedActive(bool forced) { m_forced_active = forced; };

    void prepareShutdown();

    void windowResized();

#ifdef USE_ANGELSCRIPT
    // we have to add this to be able to use the class as reference inside scripts
    void addRef(){};
    void release(){};
#endif

    void SyncWithSimThread();

    DustManager& GetParticleManager() { return m_particle_manager; }

    // A list of all beams interconnecting two trucks
    std::map<beam_t*, std::pair<Beam*, Beam*>> interTruckLinks;

protected:

    /** 
    * Returns whether or not the two (scaled) bounding boxes intersect.
    */
    bool intersectionAABB(Ogre::AxisAlignedBox a, Ogre::AxisAlignedBox b, float scale = 1.0f);

    /** 
    * Returns whether or not the bounding boxes of truck a and truck b intersect. Based on the default truck bounding boxes.
    */
    bool truckIntersectionAABB(int a, int b, float scale = 1.0f);

    /** 
    * Returns whether or not the bounding boxes of truck a and truck b might intersect during the next framestep. Based on the default truck bounding boxes.
    */
    bool predictTruckIntersectionAABB(int a, int b, float scale = 1.0f);

    /** 
    * Returns whether or not the bounding boxes of truck a and truck b intersect. Based on the truck collision bounding boxes.
    */
    bool truckIntersectionCollAABB(int a, int b, float scale = 1.0f);

    /** 
    * Returns whether or not the bounding boxes of truck a and truck b might intersect during the next framestep. Based on the truck collision bounding boxes.
    */
    bool predictTruckIntersectionCollAABB(int a, int b, float scale = 1.0f);

    int CreateRemoteInstance(stream_register_trucks_t *reg);
    void RemoveStreamSource(int sourceid);

    // A list of streams without a corresponding truck in the truck array for each stream source
    std::map<int, std::vector<int>> m_stream_mismatches;

    std::unique_ptr<ThreadPool> m_sim_thread_pool;
    std::shared_ptr<Task> m_sim_task;
    
    int m_num_cpu_cores;

    Beam *m_trucks[MAX_TRUCKS];
    int m_free_truck;
    int m_previous_truck;
    int m_current_truck;
    int m_simulated_truck;

    bool m_forced_active; // disables sleepcount

    unsigned long m_physics_frames;
    int m_physics_steps;

    // Keeps track of the rounding error in the time step calculation
    float m_dt_remainder;

    float m_simulation_speed; // slow motion < 1.0 < fast motion

    DustManager m_particle_manager;

    void LogParserMessages();
    void LogSpawnerMessages();

    void RecursiveActivation(int j, std::bitset<MAX_TRUCKS> &visited);
    void UpdateSleepingState(float dt);

    int GetMostRecentTruckSlot();

    int GetFreeTruckSlot();
    int FindTruckInsideBox(Collisions *collisions, const Ogre::String &inst, const Ogre::String &box);

    void DeleteTruck(Beam *b);
};

#endif // __BeamFactory_H_
