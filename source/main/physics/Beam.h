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

#include "Application.h"
#include "BeamData.h"
#include "GfxActor.h"
#include "PerVehicleCameraContext.h"
#include "RigDef_Prerequisites.h"
#include "RoRPrerequisites.h"

#include <OgrePrerequisites.h>
#include <OgreTimer.h>
#include <memory>

class Task;


/// Softbody object; can be anything from soda can to a space shuttle
/// Monsterclass; contains logic related to physics, network, sound, threading, rendering.
/// HISTORY: this class was derived from data-only `struct rig_t`. The data have been placed here directly. Refactor in progress, you may find leftovers.
/// FUTURE: Class will be renamed to `Actor` because it's more universal than "truck/vehicle/rig/beam"
///         Prefix of public variables is 'ar_' as 'Actor'
class Beam :
    public ZeroedMemoryAllocator
{
    friend class RigSpawner;

public:

    enum class SimState
    {
        LOCAL_SIMULATED,  //!< simulated (local) truck
        NETWORKED_OK,     //!< not simulated (remote) truck
        LOCAL_SLEEPING,   //!< sleeping (local) truck
        INVALID           //!< not simulated and not updated via the network (e.g. size differs from expected)
    };

    /// @param tnum Vehicle number (alias Truck Number)
    /// @param pos
    /// @param rot
    /// @param fname Rig file name.
    /// @param ismachine (see BeamData.h)
    /// @param truckconfig Networking related.
    /// @param preloaded_with_terrain Is this rig being pre-loaded along with terrain?
    /// @param cache_entry_number Needed for flexbody caching. Pass -1 if unavailable (flexbody caching will be disabled)
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

    ~Beam();

    bool LoadTruck( //!< Spawn helper
        RoR::RigLoadingProfiler* rig_loading_profiler,
        Ogre::String const & file_name,
        Ogre::SceneNode *parent_scene_node,
        Ogre::Vector3 const & spawn_position,
        Ogre::Quaternion & spawn_rotation,
        collision_box_t *spawn_box,
        int cache_entry_number = -1
    );

    //! @{ calc forces euler division
    void              calcTruckEngine(bool doUpdate, Ogre::Real dt);
    void              calcAnimatedProps(bool doUpdate, Ogre::Real dt);
    void              calcHooks(bool doUpdate);
    void              calcForceFeedBack(bool doUpdate);
    void              calcMouse();
    void              calcNodes_(bool doUpdate, Ogre::Real dt, int step, int maxsteps);
    void              calcSlideNodes(Ogre::Real dt);
    void              calcTurboProp(bool doUpdate, Ogre::Real dt);
    void              calcScrewProp(bool doUpdate);
    void              calcWing();
    void              calcFuseDrag();
    void              calcAirBrakes();
    void              calcBuoyance(bool doUpdate, Ogre::Real dt, int step, int maxsteps);
    void              calcAxles(bool doUpdate, Ogre::Real dt);
    void              calcWheels(bool doUpdate, Ogre::Real dt, int step, int maxsteps);
    void              calcShocks(bool doUpdate, Ogre::Real dt);
    void              calcHydros(bool doUpdate, Ogre::Real dt);
    void              calcCommands(bool doUpdate, Ogre::Real dt);
    void              calcReplay(bool doUpdate, Ogre::Real dt);
    //! @}

    void              pushNetwork(char* data, int size);   //!< Parses network data; fills truck data buffers and flips them. Called by the network thread.
    void              calcNetwork();
    void              updateNetworkInfo();
    bool              addPressure(float v);
    float             getPressure();
    void              resetAngle(float rot);
    void              resetPosition(float px, float pz, bool setInitPosition, float miny);
    float             getRotation();
    Ogre::Vector3     getDirection();
    Ogre::Vector3     getPosition();
    /// Moves vehicle.
    /// @param translation Offset to move vehicle.
    /// @param setInitPosition Set initial positions of nodes to current position?
    void              resetPosition(Ogre::Vector3 translation, bool setInitPosition);
    void              reset(bool keepPosition = false);    //!< reset the actor from any context
    void              displace(Ogre::Vector3 translation, float rotation);
    Ogre::Vector3     getRotationCenter();                 //!< Return the rotation center of the truck
    bool              replayStep();
    void              ForceFeedbackStep(int steps);
    void              updateAngelScriptEvents(float dt);
    void              handleResetRequests(float dt);
    void              setupDefaultSoundSources();
    void              updateSoundSources();
    void              mouseMove(int node, Ogre::Vector3 pos, float force); //!< Event handler
    void              lightsToggle();                      //!< Event handler
    void              tieToggle(int group=-1);
    void              ropeToggle(int group=-1);            //!< Event handler
    void              hookToggle(int group=-1, hook_states mode=HOOK_TOGGLE, int node_number=-1); //!< Event handler
    void              engineTriggerHelper(int engineNumber, int type, float triggerValue);
    void              toggleSlideNodeLock();
    void              toggleCustomParticles();
    void              toggleAxleLock();	                   //! diff lock on or off
    void              parkingbrakeToggle();                //!< Event handler
    void              antilockbrakeToggle();               //!< Event handler
    void              tractioncontrolToggle();             //!< Event handler
    void              cruisecontrolToggle();               //!< Event handler
    void              beaconsToggle();                     //!< Event handler
    void              forwardCommands();
    void              setReplayMode(bool rm);              //!< Event handler; toggle replay mode.
    int               savePosition(int position);
    int               loadPosition(int position);
    /// Virtually moves the truck at most 'direction.length()' meters towards 'direction' trying to resolve any collisions
    /// Returns a minimal offset by which the truck needs to be moved to resolve any collisions
    Ogre::Vector3     calculateCollisionOffset(Ogre::Vector3 direction);
    /// Moves the truck at most 'direction.length()' meters towards 'direction' to resolve any collisions
    void              resolveCollisions(Ogre::Vector3 direction);
    /// Auto detects an ideal collision avoidance direction (front, back, left, right, up)
    /// Then moves the truck at most 'max_distance' meters towards that direction to resolve any collisions
    void              resolveCollisions(float max_distance, bool consider_up);
    ground_model_t*   getLastFuzzyGroundModel();
    void              updateSkidmarks();                   //!< Creates or updates skidmarks. No side effects.
    void              prepareInside(bool inside);          //!< Prepares vehicle for in-cabin camera use.
    void              updateFlares(float dt, bool isCurrent=false);
    /// TIGHT-LOOP; Display; updates positions of props.
    /// Each prop has scene node parented to root-node (only_a_ptr 11/2013).
    void              updateProps();
    /// TIGHT-LOOP; Logic: display (+overlays +particles), sound
    /// Does a mixture of tasks:
    /// - Sound: updates sound sources; plays aircraft radio chatter;
    /// - Particles: updates particles (dust, exhausts, custom)
    /// - Display: updates wings; updates props; updates rig-skeleton + cab fade effect; updates debug overlay
    void              updateVisual(float dt=0);
    void              updateFlexbodiesPrepare();
    void              updateFlexbodiesFinal();
    void              joinFlexbodyTasks();                 //!< Waits until all flexbody tasks are finished, but does not update the hardware buffers
    void              updateLabels(float dt=0);            //!< Gfx;
    void              setDetailLevel(int v);               //!< @param v 0 = full detail, 1 = no beams
    void              showSkeleton(bool meshes=true, bool linked=true); //!< Gfx; shows "skeletonview" (diagnostic view) mesh.
    void              hideSkeleton(bool linked=true);      //!< Gfx; hides "skeletonview" (diagnostic view) mesh.
    void              updateSimpleSkeleton();              //!< Gfx; updates the "skeletonview" (diagnostic view) mesh.
    void              resetAutopilot();
    void              disconnectAutopilot();
    void              scaleTruck(float value);
    void              updateDebugOverlay();
    void              setDebugOverlayState(int mode);
    void              calcBeam(beam_t& beam, bool doUpdate, Ogre::Real dt, int& m_increased_accuracy);
    Ogre::Quaternion  specialGetRotationTo(const Ogre::Vector3& src, const Ogre::Vector3& dest) const;
    Ogre::String      getAxleLockName();                   //!< get the name of the current differential model
    int               getAxleLockCount();
    Ogre::Vector3     getGForces();
    bool              hasDriverSeat();
    void              calculateDriverPos(Ogre::Vector3 &pos, Ogre::Quaternion &rot);
    float             getSteeringAngle();
    std::string       getTruckName();
    std::string       getTruckFileName();
    int               getTruckType();
    int               getBeamCount();
    int               getNodeCount();
    int               nodeBeamConnections(int nodeid);     //!< Returns the number of active (non bounded) beams connected to a node
    void              changedCamera();                     //!< Logic: sound, display; Notify this vehicle that camera changed;
    void              StopAllSounds();
    void              UnmuteAllSounds();
    float             getTotalMass(bool withLocked=true);
    void              recalc_masses();
    int               getWheelNodeCount();
    void              setMass(float m);
    bool              getBrakeLightVisible();
    bool              getReverseLightVisible();            //!< Tells if the reverse-light is currently lit.
    bool              getCustomLightVisible(int number);
    void              setCustomLightVisible(int number, bool visible);
    bool              getBeaconMode();
    void              setBlinkType(blinktype blink);    
    float             getHeadingDirectionAngle();
    bool              getCustomParticleMode();
    int               getLowestNode();
    void              receiveStreamData(unsigned int type, int source, unsigned int streamid, char *buffer, unsigned int len);
    void              setBeamVisibility(bool visible);     //!< Gfx only; Sets visibility of all beams on this vehicle
    void              setMeshVisibility(bool visible);     //!< Gfx only; Sets visibility of all meshes on this vehicle
    bool              inRange(float num, float min, float max);
    bool              getSlideNodesLockInstant();
    void              sendStreamData();
    bool              isTied();
    bool              isLocked(); 
    void              updateDashBoards(float dt);
    void              updateBoundingBox(); 
    void              calculateAveragePosition();
    void              preUpdatePhysics(float dt);          //!< TIGHT LOOP; Physics;
    void              postUpdatePhysics(float dt);         //!< TIGHT LOOP; Physics;
    bool              calcForcesEulerPrepare(int doUpdate, Ogre::Real dt, int step = 0, int maxsteps = 1); //!< TIGHT LOOP; Physics;
    void              calcForcesEulerCompute(bool doUpdate, Ogre::Real dt, int step = 0, int maxsteps = 1); //!< TIGHT LOOP; Physics;
    void              calcForcesEulerFinal(int doUpdate, Ogre::Real dt, int step = 0, int maxsteps = 1); //!< TIGHT LOOP; Physics;
    void              UpdatePropAnimations(const float dt);
    blinktype         getBlinkType();
    std::vector<authorinfo_t>     getAuthors();
    std::vector<std::string>      getDescription();
    RoR::PerVehicleCameraContext* GetCameraContext()    { return &m_camera_context; }
    std::list<Beam*>  getAllLinkedBeams()               { return m_linked_actors; }; //!< Returns a list of all connected (hooked) actors
    Ogre::Vector3     GetFFbBodyForces() const          { return m_force_sensors.out_body_forces; }
    PointColDetector* IntraPointCD()                    { return m_intra_point_col_detector; }
    PointColDetector* InterPointCD()                    { return m_inter_point_col_detector; }
    Ogre::SceneNode*  getSceneNode()                    { return beamsRoot; }
    RoR::GfxActor*    GetGfxActor()                     { return m_gfx_actor.get(); }
    void              RequestUpdateHudFeatures()        { m_hud_features_ok = false; }
    Ogre::Vector3     getNodePosition(int nodeNumber);     //!< Returns world position of node
    Ogre::Real        getMinimalCameraRadius();
    Replay*           getReplay();
    float             GetFFbHydroForces() const         { return m_force_sensors.out_hydros_forces; }
    bool              isPreloadedWithTerrain()          { return m_preloaded_with_terrain; };
    VehicleAI*        getVehicleAI()                    { return ar_vehicle_ai; }
    bool              IsNodeIdValid(int id) const       { return (id > 0) && (id < ar_num_nodes); }
    float             getWheelSpeed() const             { return ar_wheel_speed; }
    Ogre::Vector3     getVelocity()                     { return m_avg_node_velocity; }; //!< average truck velocity calculated using the truck positions of the last two frames
#ifdef USE_ANGELSCRIPT
    // we have to add this to be able to use the class as reference inside scripts
    void              addRef()                          {};
    void              release()                         {};
#endif

    // -------------------- Public data -------------------- //

    node_t*           ar_nodes;
    int               ar_num_nodes;
    beam_t*           ar_beams;
    int               ar_num_beams;
    std::vector<beam_t*> ar_inter_beams;    //!< Beams connecting 2 actors
    shock_t*          ar_shocks;            //!< Shock absorbers
    int               ar_num_shocks;        //!< Number of shock absorbers
    bool              ar_has_active_shocks; //!< Are there active stabilizer shocks?
    rotator_t*        ar_rotators;
    int               ar_num_rotators;
    wing_t*           ar_wings;
    int               ar_num_wings;
    std::vector<std::string>  description;
    std::vector<authorinfo_t> authors;
    std::vector<exhaust_t>    exhausts;
    std::vector<rope_t>       ropes;
    std::vector<ropable_t>    ropables;
    std::vector<tie_t>        ties;
    std::vector<hook_t>       hooks;
    std::vector<flare_t>      flares;
    std::vector<debugtext_t>  nodes_debug;
    std::vector<debugtext_t>  beams_debug;
    Ogre::AxisAlignedBox      boundingBox;     //!< standard bounding box (surrounds all nodes of a truck)
    Ogre::AxisAlignedBox      predictedBoundingBox;
    std::vector< std::vector< int > >  nodetonodeconnections;
    std::vector< std::vector< int > >  nodebeamconnections;
    std::vector<Ogre::Entity*>         deletion_Entities;   //!< For unloading vehicle; filled at spawn.
    std::vector<Ogre::MovableObject *> deletion_Objects;    //!< For unloading vehicle; filled at spawn.
    std::vector<Ogre::SceneNode*>      deletion_sceneNodes; //!< For unloading vehicle; filled at spawn.
    std::vector<Ogre::AxisAlignedBox>  collisionBoundingBoxes; //!< smart bounding boxes, used for determining the state of a truck (every box surrounds only a subset of nodes)
    std::vector<Ogre::AxisAlignedBox>  predictedCollisionBoundingBoxes;
    contacter_t       contacters[MAX_CONTACTERS];
    int               free_contacter;
    wheel_t           wheels[MAX_WHEELS];
    vwheel_t          vwheels[MAX_WHEELS];
    int               free_wheel;
    command_t         commandkey[MAX_COMMANDS + 10]; // 0 for safety
    prop_t            ar_props[MAX_PROPS];
    prop_t*           ar_driverseat_prop;
    int               ar_num_props;
    cparticle_t       cparticles[MAX_CPARTICLES];
    int               free_cparticle;
    soundsource_t     soundsources[MAX_SOUNDSCRIPTS_PER_TRUCK];
    int               free_soundsource;
    int               pressure_beams[MAX_PRESSURE_BEAMS];
    int               free_pressure_beam;
    AeroEngine*       aeroengines[MAX_AEROENGINES];
    int               free_aeroengine;
    Screwprop*        screwprops[MAX_SCREWPROPS];
    int               free_screwprop;
    int               cabs[MAX_CABS*3];
    int               free_cab;
    int               hydro[MAX_HYDROS];
    int               free_hydro;
    int               collcabs[MAX_CABS];
    collcab_rate_t    inter_collcabrate[MAX_CABS];
    collcab_rate_t    intra_collcabrate[MAX_CABS];
    int               free_collcab;
    int               buoycabs[MAX_CABS];
    int               buoycabtypes[MAX_CABS];
    int               free_buoycab;
    Airbrake*         airbrakes[MAX_AIRBRAKES];
    int               free_airbrake;
    RoR::Skidmark*    skidtrails[MAX_WHEELS*2];
    bool              useSkidmarks;
    FlexBody*         flexbodies[MAX_FLEXBODIES];
    int               free_flexbody;
    int               ar_camera_rail[MAX_CAMERARAIL]; //!< Nodes defining camera-movement spline
    int               free_camerarail;
    bool              ar_hide_in_actor_list; //!< Hide in list of spawned actors (available in top menubar). Useful for fixed-place machinery, i.e. cranes.
    Ogre::String      realtruckname;
    bool              forwardcommands;
    bool              importcommands;
    bool              wheel_contact_requested;
    bool              rescuer;
    bool              disable_default_sounds;
    bool              has_slope_brake;
    float             slopeBrakeFactor;
    float             slopeBrakeAttAngle;
    float             slopeBrakeRelAngle;
    float             ar_anim_previous_crank; //!< For 'animator' with flag 'torque'
    float             alb_ratio;          //!< Anti-lock brake attribute: Regulating force
    float             alb_minspeed;       //!< Anti-lock brake attribute;
    int               alb_mode;           //!< Anti-lock brake status; Enabled? {1/0}
    float             alb_pulse_time;     //!< Anti-lock brake attribute;
    bool              alb_pulse_state;    //!< Anti-lock brake status;
    bool              alb_present;        //!< Anti-lock brake attribute: Display the dashboard indicator?
    bool              alb_notoggle;       //!< Anti-lock brake attribute: Disable in-game toggle?
    float             tc_ratio;           //!< Traction control
    float             tc_wheelslip;       //!< Traction control
    float             tc_fade;            //!< Traction control
    int               tc_mode;            //!< Traction control status; Enabled? {1/0}
    float             tc_pulse_time;      //!< Traction control attribute;
    bool              tc_pulse_state;     //!< Traction control
    bool              tc_present;         //!< Traction control attribute; Display the dashboard indicator?
    bool              tc_notoggle;        //!< Traction control attribute; Disable in-game toggle?
    float             tc_timer;           //!< Traction control
    float             alb_timer;
    int               antilockbrake;
    int               tractioncontrol;
    float             ar_anim_shift_timer;//!< For 'animator' with flag 'shifter'
    bool              cc_mode;            //!< Cruise Control
    bool              cc_can_brake;       //!< Cruise Control
    float             cc_target_rpm;      //!< Cruise Control
    float             cc_target_speed;    //!< Cruise Control
    float             cc_target_speed_lower_limit; //!< Cruise Control
    std::deque<float> cc_accs;            //!< Cruise Control
    bool              sl_enabled;         //!< Speed limiter;
    float             sl_speed_limit;     //!< Speed limiter;
    int               ar_extern_camera_mode;
    int               ar_extern_camera_node;
    float             collrange;
    int               masscount;          //!< Number of nodes loaded with l option
    bool              disable_smoke;
    int               ar_exhaust_pos_node; //!< Old-format exhaust (one per vehicle) emitter node
    int               ar_exhaust_dir_node; //!< Old-format exhaust (one per vehicle) backwards direction node
    char              truckname[256];
    bool              networking;
    bool              beambreakdebug;
    bool              beamdeformdebug;
    bool              triggerdebug;
    float             truckmass;
    float             loadmass;
    int               trucknum;
    RoR::SkinDef*     usedSkin;
    Buoyance*         buoyance;
    int               driveable;
    BeamEngine*       engine;
    int               hascommands;
    int               hashelp;
    char              helpmat[256];
    int               cinecameranodepos[MAX_CAMERAS];       //!< Cine-camera node indexes
    int               freecinecamera;                       //!< Number of cine-cameras (lowest free index)
    RoR::GfxFlaresMode m_flares_mode;
    unsigned int      netCustomLightArray[4];
    unsigned char     netCustomLightArray_counter;
    bool              ispolice;
    bool              collisionRelevant;
    bool              heathaze;
    Autopilot*        autopilot;
    HeightFinder*     hfinder;
    float             ar_brake_force;              //!< Physics attr; filled at spawn
    int               debugVisuals;                //!< Dbg. overlay type { NODES: 1-Numbers, 4-Mass, 5-Locked | BEAMS: 2-Numbers, 6-Compression, 7-Broken, 8-Stress, 9-Strength, 10-Hydros, 11-Commands, OTHER: 3-N&B numbers, 12-14 unknown }
    float             speedoMax;
    bool              useMaxRPMforGUI;
    float             minimass;
    bool              cparticle_enabled;
    bool              advanced_drag;
    float             advanced_node_drag;
    float             advanced_total_drag;
    Axle*             axles[MAX_WHEELS/2];
    int               free_axle;
    int               free_commands;
    Ogre::Vector3     ar_origin;
    Ogre::SceneNode*  beamsRoot;
    int               ar_num_cameras;
    int               ar_camera_node_pos[MAX_CAMERAS]; //!< Physics attr; 'camera' = frame of reference; origin node
    int               ar_camera_node_dir[MAX_CAMERAS]; //!< Physics attr; 'camera' = frame of reference; back node
    int               ar_camera_node_roll[MAX_CAMERAS]; //!< Physics attr; 'camera' = frame of reference; left node
    bool              ar_camera_node_roll_inv[MAX_CAMERAS]; //!< Physics attr; 'camera' = frame of reference; indicates roll node is right instead of left
    int               ar_lowest_node;             //!< Physics attr, filled at spawn, limited use for boats
    int               ar_lowest_contacting_node;  //!< Physics attr, filled at spawn, used for positioning on (re)spawn
    float             ar_posnode_spawn_height;
    VehicleAI*        ar_vehicle_ai;
    float             ar_scale;               //!< Physics state; scale of the actor (nominal = 1.0)
    Ogre::Real        ar_brake;               //!< Physics state; braking intensity
    float             ar_wheel_speed;         //!< Physics state; wheel speed in m/s
    float             ar_hydro_dir_command;
    float             ar_hydro_dir_state;
    Ogre::Real        ar_hydro_dir_wheel_display;
    float             ar_hydro_aileron_command;
    float             ar_hydro_aileron_state;
    float             ar_hydro_rudder_command;
    float             ar_hydro_rudder_state;
    float             ar_hydro_elevator_command;
    float             ar_hydro_elevator_state;
    Ogre::Real        ar_replay_precision;            //!< Sim attribute; determined at startup
    int               ar_replay_length;               //!< Sim attribute; clone of GVar 'sim_replay_length'
    int               ar_replay_pos;                  //!< Sim state
    float             ar_sleep_counter;               //!< Sim state; idle time counter
    ground_model_t*   ar_submesh_ground_model;
    int               ar_parking_brake;
    int               ar_lights;
    float             ar_left_mirror_angle;           //!< Sim state; rear view mirror angle
    float             ar_right_mirror_angle;          //!< Sim state; rear view mirror angle
    float             ar_elevator;                    //!< Sim state; aerial controller
    float             ar_rudder;                      //!< Sim state; aerial/marine controller
    float             ar_aileron;                     //!< Sim state; aerial controller
    int               ar_aerial_flap;                 //!< Sim state; state of aircraft flaps (values: 0-5)
    Ogre::Vector3     ar_fusedrag;                    //!< Physics state
    int               ar_current_cinecam;             //!< Sim state; index of current CineCam (-1 if using external camera)
    int               ar_custom_camera_node;          //!< Sim state; custom tracking node for 3rd-person camera
    std::string       ar_filename;                    //!< Attribute; filled at spawn
    int               ar_airbrake_intensity;          //!< Physics state; values 0-5
    int               ar_net_source_id;
    int               ar_net_stream_id;
    std::map<int,int> ar_net_stream_results;
    Ogre::Timer       ar_net_timer;
    unsigned long     ar_net_last_update_time;
    DashBoardManager* ar_dashboard;
    int               ar_request_skeletonview_change; //!< Gfx state; Request activation(1) / deactivation(-1) of skeletonview
    SimState          ar_sim_state;                   //!< Sim state

    // Bit flags
    bool ar_left_blink_on:1;  //!< Gfx state; turn signals
    bool ar_right_blink_on:1; //!< Gfx state; turn signals
    bool ar_warn_blink_on:1;  //!< Gfx state; turn signals
    bool ar_beams_visible:1;  //!< Gfx state; Are beams visible? @see setBeamVisibility
    bool ar_meshes_visible:1; //!< Gfx state; Are meshes visible? @see setMeshVisibility
    bool ar_skeletonview_is_active:1; //!< Gfx state
    bool ar_update_physics:1; //!< Physics state; Should this actor be updated (locally) in the next physics step?
    bool ar_disable_self_collision:1; //!< Physics attribute; clone of RoR.cfg entry "DisableSelfCollisions"
    bool ar_disable_actor2actor_collision:1; //!< Physics attribute; clone of RoR.cfg entry "DisableCollisions"
    bool ar_disable_aerodyn_turbulent_drag:1; //!< Physics state
    bool ar_engine_hydraulics_ready:1; //!< Sim state; does engine have enough RPM to power hydraulics?
    bool ar_hydro_speed_coupling:1;
    bool ar_replay_mode:1;      //!< Sim state

private:

    enum ResetRequest
    {
        REQUEST_RESET_NONE,
        REQUEST_RESET_ON_INIT_POS,
        REQUEST_RESET_ON_SPOT,
        REQUEST_RESET_FINAL
    };

    void              calcBeams(int doUpdate, Ogre::Real dt, int step, int maxsteps); // !< TIGHT LOOP; Physics & sound;
    void              calcBeamsInterTruck(int doUpdate, Ogre::Real dt, int step, int maxsteps); //!< TIGHT LOOP; Physics & sound - only beams between multiple truck (noshock or ropes)
    void              calcNodes(int doUpdate, Ogre::Real dt, int step, int maxsteps); //!< TIGHT LOOP; Physics;
    void              calcHooks();                         //!< TIGHT LOOP; Physics;
    void              calcRopes();                         //!< TIGHT LOOP; Physics;
    void              calcShocks2(int beam_i, Ogre::Real difftoBeamL, Ogre::Real &k, Ogre::Real &d, Ogre::Real dt, int update);
    void              calcAnimators(const int flag_state, float &cstate, int &div, float timer, const float lower_limit, const float upper_limit, const float option3);
    void              SyncReset();                         //!< this one should be called only synchronously (without physics running in background)
    void              SetPropsCastShadows(bool do_cast_shadows);
    void              determineLinkedBeams();
    void              calc_masses2(Ogre::Real total, bool reCalc=false);
    void              calcNodeConnectivityGraph();
    void              moveOrigin(Ogre::Vector3 offset);    //!< move physics origin
    void              addInterTruckBeam(beam_t* beam, Beam* a, Beam* b);
    void              removeInterTruckBeam(beam_t* beam);
    void              disjoinInterTruckBeams();            //!< Destroys all inter truck beams which are connected with this truck
    void              CreateSimpleSkeletonMaterial();
    void              cabFade(float amount);
    void              setMeshWireframe(Ogre::SceneNode *node, bool value);
    void              fadeMesh(Ogre::SceneNode *node, float amount);
    float             getAlphaRejection(Ogre::SceneNode *node);
    void              setAlphaRejection(Ogre::SceneNode *node, float amount);
    void              initSimpleSkeleton();                //!< Builds the rig-skeleton mesh.
    void              autoBlinkReset();                    //!< Resets the turn signal when the steering wheel is turned back.
    void              sendStreamSetup();
    void              updateSlideNodeForces(const Ogre::Real delta_time_sec); //!< calculate and apply Corrective forces    
    void              resetSlideNodePositions();           //!< Recalculate SlideNode positions    
    void              resetSlideNodes();                   //!< Reset all the SlideNodes    
    void              updateSlideNodePositions();          //!< incrementally update the position of all SlideNodes
    /// @param truck which truck to retrieve the closest Rail from
    /// @param node which SlideNode is being checked against
    /// @return a pair containing the rail, and the distant to the SlideNode
    std::pair<RailGroup*, Ogre::Real> getClosestRailOnTruck( Beam* truck, const SlideNode& node);

    // -------------------- data -------------------- //

    std::vector<std::pair<Ogre::String, bool> > m_dashboard_layouts; // TODO: Spawn context only, remove!

    std::vector<std::shared_ptr<Task>> m_flexbody_tasks;   //!< Gfx state
    std::shared_ptr<RigDef::File>      m_definition;
    std::unique_ptr<RoR::GfxActor>     m_gfx_actor;
    RoR::PerVehicleCameraContext       m_camera_context;
    std::bitset<MAX_WHEELS>            m_flexmesh_prepare; //!< Gfx state
    std::bitset<MAX_FLEXBODIES>        m_flexbody_prepare; //!< Gfx state
    std::vector<Ogre::String>          m_truck_config;
    std::vector<SlideNode>             m_slidenodes;       //!< all the SlideNodes available on this truck
    std::vector<RailGroup*>            m_railgroups;       //!< all the available RailGroups for this actor
    int               m_proped_wheel_pairs[MAX_WHEELS];    //!< Physics attr; For inter-differential locking
    int               m_num_braked_wheels;          //!< Physics attr, filled at spawn - Number of braked wheels.
    int               m_num_proped_wheels;          //!< Physics attr, filled at spawn - Number of propelled wheels.
    float             m_avionic_chatter_timer;      //!< Sound fx state
    PointColDetector* m_inter_point_col_detector;   //!< Physics
    PointColDetector* m_intra_point_col_detector;   //!< Physics
    std::list<Beam*>  m_linked_actors;              //!< Sim state; other actors linked using 'hooks'
    Ogre::Vector3     m_avg_node_position;          //!< average node position
    Ogre::Real        m_min_camera_radius;
    Ogre::Vector3     m_avg_node_position_prev;
    Ogre::Vector3     m_avg_node_velocity;          //!< average node velocity (compared to the previous frame step)
    Ogre::Real        m_replay_timer;               //!< Sim state
    ground_model_t*   m_last_fuzzy_ground_model;    //!< GUI state
    blinktype         m_blink_type;                 //!< Sim state; Blinker = turn signal
    float             m_stabilizer_shock_sleep;     //!< Sim state
    Replay*           m_replay_handler;
    PositionStorage*  m_position_storage;
    int               m_gfx_detail_level;      //!< Gfx state
    float             m_total_mass;            //!< Physics state; total mass in Kg
    int               m_mouse_grab_node;       //!< Sim state; node currently being dragged by user
    Ogre::Vector3     m_mouse_grab_pos;
    float             m_mouse_grab_move_force;
    float             m_spawn_rotation;
    ResetRequest      m_reset_request;
    RoRnet::TruckState* oob1;                  //!< Network; Triple buffer for incoming data (truck properties)
    RoRnet::TruckState* oob2;                  //!< Network; Triple buffer for incoming data (truck properties)
    RoRnet::TruckState* oob3;                  //!< Network; Triple buffer for incoming data (truck properties)
    char*             netb1;                   //!< Network; Triple buffer for incoming data
    char*             netb2;                   //!< Network; Triple buffer for incoming data
    char*             netb3;                   //!< Network; Triple buffer for incoming data
    int               m_net_time_offset;
    int               m_net_update_counter;
    Ogre::MovableText* m_net_label_mt;
    Ogre::SceneNode*  m_net_label_node;
    Ogre::String      m_net_username;
    float             m_custom_light_toggle_countdown; //!< Input system helper status
    float             m_cab_fade_timer;
    float             m_cab_fade_time;
    int               m_cab_fade_mode;            //<! Cab fading effect; values { -1, 0, 1, 2 }
    FlexObj*          m_cab_mesh;
    Ogre::SceneNode*  m_cab_scene_node;
    Ogre::Entity*     m_cab_entity;
    Ogre::ManualObject* m_skeletonview_manual_mesh;
    Ogre::SceneNode*  m_skeletonview_scenenode;
    Ogre::Vector3     m_camera_gforces_accu;      //!< Accumulator for 'camera' G-forces
    int               m_camera_gforces_count;     //!< Counter for 'camera' G-forces
    float             m_ref_tyre_pressure;        //!< Physics state
    float             m_stabilizer_shock_ratio;   //!< Physics state
    int               m_stabilizer_shock_request; //!< Physics state; values: { -1, 0, 1 }
    DustPool*         m_particles_dust;
    DustPool*         m_particles_drip;
    DustPool*         m_particles_sparks;
    DustPool*         m_particles_clump;
    DustPool*         m_particles_splash;
    DustPool*         m_particles_ripple;
    int               m_net_first_wheel_node;  //!< Network attr; Determines data buffer layout
    int               m_net_node_buf_size;     //!< Network attr; buffer size
    int               m_net_buffer_size;       //!< Network attr; buffer size
    int               m_wheel_node_count;      //!< Static attr; filled at spawn
    int               m_replay_pos_prev;       //!< Sim state
    int               m_previous_gear;         //!< Sim state; land vehicle shifting
    CmdKeyInertia*    m_rotator_inertia;       //!< Physics
    CmdKeyInertia*    m_hydro_inertia;         //!< Physics
    CmdKeyInertia*    m_command_inertia;       //!< Physics
    float             m_handbrake_force;       //!< Physics attr; defined in truckfile
    Airfoil*          m_fusealge_airfoil;      //!< Physics attr; defined in truckfile
    node_t*           m_fusealge_front;        //!< Physics attr; defined in truckfile
    node_t*           m_fusealge_back;         //!< Physics attr; defined in truckfile
    float             m_fusealge_width;        //!< Physics attr; defined in truckfile
    float             m_odometer_total;        //!< GUI state
    float             m_odometer_user;         //!< GUI state

    bool m_hud_features_ok:1;      //!< Gfx state; Are HUD features matching actor's capabilities?
    bool m_slidenodes_locked:1;    //!< Physics state; Are SlideNodes locked?
    bool m_blinker_autoreset:1;    //!< Gfx state; We're steering - when we finish, the blinker should turn off
    bool m_net_brake_light:1;
    bool m_net_reverse_light:1;
    bool m_hide_own_net_label:1;
    bool m_reverse_light_active:1; //!< Gfx state
    bool m_water_contact:1;        //!< Scripting state
    bool m_water_contact_old:1;    //!< Scripting state
    bool m_increased_accuracy:1;   //!< Physics state; temporarily bypass collision test cooldown timers
    bool m_beacon_light_is_active:1;        //!< Gfx state
    bool m_custom_particles_enabled:1;      //!< Gfx state
    bool m_skeletonview_mesh_initialized:1; //!< Gfx state; Was the rig-skeleton mesh built?
    bool m_slidenodes_connect_on_spawn:1;   //<! spawn context: try to connect slide-nodes directly after spawning (TODO: remove!)
    bool m_is_cinecam_rotation_center:1;    //<! Attribute; filled at spawn
    bool m_preloaded_with_terrain:1;        //!< Spawn context (TODO: remove!)
    bool m_high_res_wheelnode_collisions:1; //!< Physics attr; set at spawn
    bool m_spawn_free_positioned:1;         //!< Spawn context (TODO: remove!)
    bool m_gfx_reduce_shadows:1;       //!< Gfx switch; alias of RoR.cfg entry "Shadow optimizations"

#ifdef FEAT_TIMING
    BeamThreadStats *statistics, *statistics_gfx;
#endif

    struct VehicleForceSensors
    {
        inline void Reset()
        {
            accu_body_forces    = 0;
            accu_hydros_forces  = 0;
            out_body_forces     = 0;
            out_hydros_forces   = 0;
        };

        Ogre::Vector3 accu_body_forces;
        float         accu_hydros_forces;
        Ogre::Vector3 out_body_forces;
        float         out_hydros_forces;
    } m_force_sensors; //!< Data for ForceFeedback devices
};
