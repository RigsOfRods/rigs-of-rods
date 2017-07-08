/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2017 Petr Ohlidal & contributors

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

#pragma once

#include "BeamData.h"
#include "GfxActor.h"
#include "PerVehicleCameraContext.h"
#include "RigDef_Prerequisites.h"
#include "RoRPrerequisites.h"

#include <OgrePrerequisites.h>
#include <OgreTimer.h>
#include <memory>

class Task;

/**
* Represents an entire rig (any vehicle type)
* Contains logic related to physics, network, sound, threading, rendering. It's a bit of a monster class :(
*/
class Beam :
    public rig_t,
    public ZeroedMemoryAllocator
{
    friend class RigSpawner;
    friend class RigInspector; // Debug utility class

public:
    Beam() {}; // for wrapper, DO NOT USE!
    ~Beam();

#ifdef USE_ANGELSCRIPT
    // we have to add this to be able to use the class as reference inside scripts
    void addRef(){};
    void release(){};
#endif

    /**
    * Constructor
    *
    * @param tnum Vehicle number (alias Truck Number)
    * @param pos
    * @param rot
    * @param fname Rig file name.
    * @param ismachine (see BeamData.h)
    * @param truckconfig Networking related.
    * @param preloaded_with_terrain Is this rig being pre-loaded along with terrain?
    * @param cache_entry_number Needed for flexbody caching. Pass -1 if unavailable (flexbody caching will be disabled)
    */
    Beam(
          RoRFrameListener* sim_controller
        , int tnum
        , Ogre::Vector3 pos
        , Ogre::Quaternion rot
        , const char* fname
        , RoR::RigLoadingProfiler* rig_loading_profiler
        , bool networked = false
        , bool networking = false
        , collision_box_t *spawnbox = nullptr
        , bool ismachine = false
        , const std::vector<Ogre::String> *truckconfig = nullptr
        , RoR::SkinDef *skin = nullptr
        , bool freeposition = false
        , bool preloaded_with_terrain = false
        , int cache_entry_number = -1
        );

    /**
    * Parses network data; fills truck data buffers and flips them. Called by the network thread.
    */
    void pushNetwork(char* data, int size);
    void calcNetwork();

    void updateNetworkInfo();

    bool addPressure(float v);
    float getPressure();
    void resetAngle(float rot);
    void resetPosition(float px, float pz, bool setInitPosition, float miny);

    float getRotation();
    Ogre::Vector3 getDirection();
    Ogre::Vector3 getPosition();

    /**
    * Returns the average truck velocity calculated using the truck positions of the last two frames
    */
    Ogre::Vector3 getVelocity() { return velocity; };

    /**
    * Moves vehicle.
    * @param translation Offset to move vehicle.
    * @param setInitPosition Set initial positions of nodes to current position?
    */
    void resetPosition(Ogre::Vector3 translation, bool setInitPosition);

    /**
    * Call this one to reset a truck from any context
    */
    void reset(bool keepPosition = false);

    /**
    * Call this one to displace a truck
    */
    void displace(Ogre::Vector3 translation, float rotation);

    /**
     * Return the rotation center of the truck
     */
    Ogre::Vector3 getRotationCenter();

    /**
    * Spawns vehicle.
    */
    bool LoadTruck(
        RoR::RigLoadingProfiler* rig_loading_profiler,
        Ogre::String const & file_name,
        Ogre::SceneNode *parent_scene_node,
        Ogre::Vector3 const & spawn_position,
        Ogre::Quaternion & spawn_rotation,
        collision_box_t *spawn_box,
        int cache_entry_number = -1
    );

    VehicleAI* getVehicleAI() { return vehicle_ai; }

    bool replayStep();

    void ForceFeedbackStep(int steps);
    void updateAngelScriptEvents(float dt);
    void handleResetRequests(float dt);

    void setupDefaultSoundSources();

    void updateSoundSources();

    /**
    * Event handler
    */
    void mouseMove(int node, Ogre::Vector3 pos, float force);

    /**
    * Event handler
    */
    void lightsToggle();

    /**
    * Event handler
    */
    void tieToggle(int group=-1);

    /**
    * Event handler
    */
    void ropeToggle(int group=-1);

    /**
    * Event handler
    */
    void hookToggle(int group=-1, hook_states mode=HOOK_TOGGLE, int node_number=-1);
    void engineTriggerHelper(int engineNumber, int type, float triggerValue);
    void toggleSlideNodeLock();
    void toggleCustomParticles();
    void toggleAxleLock();	//! diff lock on or off

    /**
    * Event handler
    */
    void parkingbrakeToggle();

    /**
    * Event handler
    */
    void antilockbrakeToggle();

    /**
    * Event handler
    */
    void tractioncontrolToggle();

    /**
    * Event handler
    */
    void cruisecontrolToggle();

    /**
    * Event handler
    */
    void beaconsToggle();
    void forwardCommands();

    /**
    * Event handler; toggle replay mode.
    */
    void setReplayMode(bool rm);
    int savePosition(int position);
    int loadPosition(int position);

    /**
     * Virtually moves the truck at most 'direction.length()' meters towards 'direction' trying to resolve any collisions
     * Returns a minimal offset by which the truck needs to be moved to resolve any collisions
     */
    Ogre::Vector3 calculateCollisionOffset(Ogre::Vector3 direction);

    /**
     * Moves the truck at most 'direction.length()' meters towards 'direction' to resolve any collisions
     */
    void resolveCollisions(Ogre::Vector3 direction);

    /**
     * Auto detects an ideal collision avoidance direction (front, back, left, right, up)
     * Then moves the truck at most 'max_distance' meters towards that direction to resolve any collisions
     */
    void resolveCollisions(float max_distance, bool consider_up);

    /**
    * Ground.
    */
    ground_model_t *getLastFuzzyGroundModel();

    /**
    * Creates or updates skidmarks. No side effects.
    */
    void updateSkidmarks();

    /**
    * Prepares vehicle for in-cabin camera use.
    */
    void prepareInside(bool inside);

    /**
    * Display.
    */
    void updateFlares(float dt, bool isCurrent=false);

    /**
    * TIGHT-LOOP; Display; updates positions of props.
    * Each prop has scene node parented to root-node (only_a_ptr 11/2013).
    */
    void updateProps();

    /**
    * TIGHT-LOOP; Logic: display (+overlays +particles), sound
    * Does a mixture of tasks:
    * - Sound: updates sound sources; plays aircraft radio chatter;
    * - Particles: updates particles (dust, exhausts, custom)
    * - Display: updates wings; updates props; updates rig-skeleton + cab fade effect; updates debug overlay
    */
    void updateVisual(float dt=0);

    /**
    * TIGHT-LOOP; Logic: flexbodies
    */
    void updateFlexbodiesPrepare();
    void updateFlexbodiesFinal();

    /**
     * Waits until all flexbody tasks are finished, but does not update the hardware buffers
     */
    void joinFlexbodyTasks();

    void updateLabels(float dt=0); /// Networking: player name labels. TODO: Handle this in `GfxActor` via DearIMGUI

    /**
    * Display
    * @param v 0 = full detail, 1 = no beams
    */
    void setDetailLevel(int v);

    void resetAutopilot();
    void disconnectAutopilot();
    void scaleTruck(float value);
    float currentScale;

    /**
    * TIGHT-LOOP (optional);
    */
    void updateDebugOverlay();
    void setDebugOverlayState(int mode);

    //! @{ dynamic physical properties
    Ogre::Real brake;
    Ogre::Vector3 affforce;
    Ogre::Vector3 ffforce;
    Ogre::Real affhydro;
    Ogre::Real ffhydro;

    bool left_blink_on, right_blink_on, warn_blink_on;
    //! @}


    //! @{ calc forces euler division
    void calcTruckEngine(bool doUpdate, Ogre::Real dt);
    void calcAnimatedProps(bool doUpdate, Ogre::Real dt);
    void calcHooks(bool doUpdate);
    void calcForceFeedBack(bool doUpdate);
    void calcMouse();
    void calcNodes_(bool doUpdate, Ogre::Real dt, int step, int maxsteps);
    void calcSlideNodes(Ogre::Real dt);
    void calcTurboProp(bool doUpdate, Ogre::Real dt);
    void calcScrewProp(bool doUpdate);
    void calcWing();
    void calcFuseDrag();
    void calcAirBrakes();
    void calcBuoyance(bool doUpdate, Ogre::Real dt, int step, int maxsteps);
    void calcAxles(bool doUpdate, Ogre::Real dt);
    void calcWheels(bool doUpdate, Ogre::Real dt, int step, int maxsteps);
    void calcShocks(bool doUpdate, Ogre::Real dt);
    void calcHydros(bool doUpdate, Ogre::Real dt);
    void calcCommands(bool doUpdate, Ogre::Real dt);
    void calcReplay(bool doUpdate, Ogre::Real dt);
    //! @}

    //! @{ helper routines

    void calcBeam(beam_t& beam, bool doUpdate, Ogre::Real dt, int& increased_accuracy);
    //! @}


    /* functions to be sorted */
    Ogre::Quaternion specialGetRotationTo(const Ogre::Vector3& src, const Ogre::Vector3& dest) const;
    Ogre::String getAxleLockName();	//! get the name of the current differential model
    int getAxleLockCount();
    std::vector< std::vector< int > > nodetonodeconnections;
    std::vector< std::vector< int > > nodebeamconnections;

    // wheel speed in m/s
    float WheelSpeed = 0.f;
    float getWheelSpeed() { return WheelSpeed; }
    Ogre::Vector3 getGForces();

    int stabcommand; //!< Stabilization; values: { -1, 0, 1 }
    float stabratio;
    //direction
    float hydrodircommand;
    bool hydroSpeedCoupling;
    float hydrodirstate;
    Ogre::Real hydrodirwheeldisplay;
    //extra airplane axes
    float hydroaileroncommand;
    float hydroaileronstate;
    float hydroruddercommand;
    float hydrorudderstate;
    float hydroelevatorcommand;
    float hydroelevatorstate;

    bool canwork;
    bool replaymode;
    Ogre::Real replayPrecision;
    bool watercontact;
    bool watercontactold;
    int locked;
    int lockedold;
    int oldreplaypos;
    int replaylen;
    int replaypos;
    float sleeptime;
    //can this be driven?
    int previousGear;
    ground_model_t *submesh_ground_model;
    int parkingbrake;
    int lights;
    bool reverselight;
    float leftMirrorAngle;
    float rightMirrorAngle;
    float refpressure;

    bool hasDriverSeat();
    void calculateDriverPos(Ogre::Vector3 &pos, Ogre::Quaternion &rot);
    float getSteeringAngle();
    void triggerGUIFeaturesChanged();

    float elevator;
    float rudder;
    float aileron;
    int flap;

    Ogre::Vector3 fusedrag;

    bool disableDrag;
    bool disableTruckTruckCollisions;
    bool disableTruckTruckSelfCollisions;
    int currentcamera;
    int m_custom_camera_node;

    int wheel_node_count;
    int first_wheel_node;
    int netbuffersize;
    int nodebuffersize;
    Ogre::SceneNode *netLabelNode;

    std::string getTruckName();
    std::string getTruckFileName();
    int getTruckType();

    std::vector<authorinfo_t> getAuthors();
    std::vector<std::string> getDescription();

    int getBeamCount();
    beam_t *getBeams();

    int getNodeCount();
    node_t *getNodes();

    /**
    * Returns the number of active (non bounded) beams connected to a node
    */
    int nodeBeamConnections(int nodeid);

    /**
    * Logic: sound, display; Notify this vehicle that camera changed;
    */
    void changedCamera();

    void StopAllSounds();
    void UnmuteAllSounds();

    float getTotalMass(bool withLocked=true);
    void recalc_masses();
    int getWheelNodeCount();
    void setMass(float m);

    Ogre::String realtruckfilename;

    /**
     * @return Returns a list of all connected (hooked) beams
     */
    std::list<Beam*> getAllLinkedBeams() { return linkedBeams; };

    /**
    * This must be in the header as the network stuff is using it...
    */
    bool getBrakeLightVisible();

    /**
    * Tells if the reverse-light is currently lit.
    */
    bool getReverseLightVisible();

    bool getCustomLightVisible(int number);

    void setCustomLightVisible(int number, bool visible);

    bool getBeaconMode();
    void setBlinkType(blinktype blink);
    blinktype getBlinkType();

    float getHeadingDirectionAngle();
    bool getCustomParticleMode();
    int getLowestNode();

    bool simulated;
    int airbrakeval;
    Ogre::Vector3 cameranodeacc;
    int cameranodecount;

    int m_source_id;
    int m_stream_id;
    std::map<int, int> m_stream_results;

    Ogre::Timer netTimer;
    unsigned long lastNetUpdateTime;

    void receiveStreamData(unsigned int type, int source, unsigned int streamid, char *buffer, unsigned int len);

    /**
    * Sets visibility of all beams on this vehicle
    * @param visibility
    */
    void setBeamVisibility(bool visible);

    bool beamsVisible; //!< Are beams visible? @see setBeamVisibility

    /**
    * Sets visibility of all meshes on this vehicle
    * @param visibility
    */
    void setMeshVisibility(bool visible);

    bool meshesVisible; //!< Are meshes visible? @see setMeshVisibility

    bool inRange(float num, float min, float max);

    /**
    * Returns the position of the node
    * @param the nuber of the node
    * @return vector3 of the world position for the node
    */
    Ogre::Vector3 getNodePosition(int nodeNumber);
    Ogre::Real getMinimalCameraRadius();

    Replay *getReplay();

    bool getSlideNodesLockInstant();
    void sendStreamData();
    bool isTied();
    bool isLocked();

    bool isPreloadedWithTerrain() { return m_preloaded_with_terrain; };

    // Inline getters
    inline Ogre::SceneNode*                 getSceneNode()            { return beamsRoot; }
    inline RoR::PerVehicleCameraContext*    GetCameraContext()        { return &m_camera_context; }
    inline RoR::GfxActor*                   GetGfxActor()             { return m_gfx_actor.get(); }

    DashBoardManager *dash;

    void updateDashBoards(float dt);

    void updateBoundingBox();
    void calculateAveragePosition();

    //! @{ physic related functions
    void preUpdatePhysics(float dt);
    void postUpdatePhysics(float dt);

    /**
    * TIGHT LOOP; Physics;
    */
    bool calcForcesEulerPrepare(int doUpdate, Ogre::Real dt, int step = 0, int maxsteps = 1);

    /**
    * TIGHT LOOP; Physics;
    */
    void calcForcesEulerCompute(int doUpdate, Ogre::Real dt, int step = 0, int maxsteps = 1);

    /**
    * TIGHT LOOP; Physics;
    */
    void calcForcesEulerFinal(int doUpdate, Ogre::Real dt, int step = 0, int maxsteps = 1);

        // TODO may be removed soon
    PointColDetector* IntraPointCD() { return intraPointCD; }
    PointColDetector* InterPointCD() { return interPointCD; }


    void UpdatePropAnimations(const float dt);

protected:

    /**
    * TIGHT LOOP; Physics & sound;
    * @param doUpdate Only passed to Beam::calcShocks2()
    */
    void calcBeams(int doUpdate, Ogre::Real dt, int step, int maxsteps);

    /**
    * TIGHT LOOP; Physics & sound - only beams between multiple truck (noshock or ropes)
    */
    void calcBeamsInterTruck(int doUpdate, Ogre::Real dt, int step, int maxsteps);

    /**
    * TIGHT LOOP; Physics;
    */
    void calcNodes(int doUpdate, Ogre::Real dt, int step, int maxsteps);

    /**
    * TIGHT LOOP; Physics;
    */
    void calcHooks();

    /**
    * TIGHT LOOP; Physics;
    */
    void calcRopes();
    void calcShocks2(int beam_i, Ogre::Real difftoBeamL, Ogre::Real &k, Ogre::Real &d, Ogre::Real dt, int update);
    void calcAnimators(const int flag_state, float &cstate, int &div, float timer, const float lower_limit, const float upper_limit, const float option3);

    void SyncReset(); //this one should be called only synchronously (without physics running in background)

    void SetPropsCastShadows(bool do_cast_shadows);

    float avichatter_timer;

    // inter-/intra truck collision stuff
    PointColDetector* interPointCD;
    PointColDetector* intraPointCD;

    // flexable stuff
    std::bitset<MAX_WHEELS> flexmesh_prepare;
    std::bitset<MAX_FLEXBODIES> flexbody_prepare;
    std::vector<std::shared_ptr<Task>> flexbody_tasks;

    // linked beams (hooks)
    std::list<Beam*> linkedBeams;
    void determineLinkedBeams();

    void calc_masses2(Ogre::Real total, bool reCalc=false);
    void calcNodeConnectivityGraph();
    void moveOrigin(Ogre::Vector3 offset); //move physics origin

    Ogre::Vector3 position; // average node position
    Ogre::Vector3 iPosition; // initial position
    Ogre::Real minCameraRadius;

    Ogre::Vector3 lastposition;
    Ogre::Vector3 velocity; // average node velocity (compared to the previous frame step)

    Ogre::Real replayTimer;

    ground_model_t *lastFuzzyGroundModel;

    bool high_res_wheelnode_collisions;

    void addInterTruckBeam(beam_t* beam, Beam* a, Beam* b);
    void removeInterTruckBeam(beam_t* beam);
    /**
    * Destroys all inter truck beams which are connected with this truck
    */
    void disjoinInterTruckBeams();

    // this is for managing the blinkers on the truck:
    blinktype blinkingtype;

    Ogre::Real hydrolen;

    Ogre::SceneNode *smokeNode;
    Ogre::ParticleSystem* smoker;
    float stabsleep;
    Replay *replay;
    PositionStorage *posStorage;
    RoRFrameListener* m_sim_controller; // Temporary ~ only_a_ptr, 01/2017
    std::unique_ptr<RoR::GfxActor> m_gfx_actor;

    RoR::PerVehicleCameraContext m_camera_context;

    bool cparticle_mode;
    int detailLevel;
    bool increased_accuracy;
    bool isInside;
    bool m_beacon_light_is_active;
    float totalmass;

    int mousenode;
    Ogre::Vector3 mousepos;
    float mousemoveforce;
    float m_spawn_rotation;
    bool m_is_cinecam_rotation_center;
    bool m_preloaded_with_terrain;

    enum ResetRequest {
        REQUEST_RESET_NONE,
        REQUEST_RESET_ON_INIT_POS,
        REQUEST_RESET_ON_SPOT,
        REQUEST_RESET_FINAL
    };

    ResetRequest m_reset_request;

    std::vector<Ogre::String> m_truck_config;

    RoRnet::TruckState *oob1; //!< Network; Triple buffer for incoming data (truck properties)
    RoRnet::TruckState *oob2; //!< Network; Triple buffer for incoming data (truck properties)
    RoRnet::TruckState *oob3; //!< Network; Triple buffer for incoming data (truck properties)
    char *netb1; //!< Network; Triple buffer for incoming data
    char *netb2; //!< Network; Triple buffer for incoming data
    char *netb3; //!< Network; Triple buffer for incoming data
    int net_toffset;
    int netcounter;
    Ogre::MovableText *netMT; //, *netDist;
    bool m_hide_own_net_label;

    // network properties
    Ogre::String networkUsername;
    int networkAuthlevel;

    bool netBrakeLight, netReverseLight;
    Ogre::Real mTimeUntilNextToggle;

    /**
     * Resets the turn signal when the steering wheel is turned back.
     */
    void autoBlinkReset();
    bool blinktreshpassed;

#ifdef FEAT_TIMING
    BeamThreadStats *statistics, *statistics_gfx;
#endif

    void sendStreamSetup();

    // dustpools
    DustPool *dustp;
    DustPool *dripp;
    DustPool *sparksp;
    DustPool *clumpp;
    DustPool *splashp;
    DustPool *ripplep;

    // SLIDE NODES /////////////////////////////////////////////////////////////
    //! true if SlideNodes are locked, false if not
    bool SlideNodesLocked;
    bool GUIFeaturesChanged;

    /**
     * calculate and apply Corrective forces
     * @param dt delta time in seconds
     */
    void updateSlideNodeForces(const Ogre::Real dt);
    //! Recalculate SlideNode positions
    void resetSlideNodePositions();
    //! Reset all the SlideNodes
    void resetSlideNodes();
    //! incrementally update the position of all SlideNodes
    void updateSlideNodePositions();

    /**
     *
     * @param truck which truck to retrieve the closest Rail from
     * @param node which SlideNode is being checked against
     * @return a pair containing the rail, and the distant to the SlideNode
     */
    std::pair<RailGroup*, Ogre::Real> getClosestRailOnTruck( Beam* truck, const SlideNode& node);

};
