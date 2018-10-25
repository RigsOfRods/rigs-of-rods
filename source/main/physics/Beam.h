/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2018 Petr Ohlidal & contributors

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
/// NOTE: Until 01/2018, this class was named `Beam` (and was derived from `rig_t`), you may find references to this.
class Actor : public ZeroedMemoryAllocator
{
    friend class ActorSpawner;
    friend class RoR::ActorManager;
    friend class RoR::GfxActor; // Temporary until all visuals are moved there. ~ only_a_ptr, 2018
public:

    enum class SimState
    {
        LOCAL_SIMULATED,  //!< simulated (local) actor
        NETWORKED_OK,     //!< not simulated (remote) actor
        LOCAL_SLEEPING,   //!< sleeping (local) actor
        INVALID           //!< not simulated and not updated via the network (e.g. size differs from expected)
    };

    /// @param actor_id Unique ID (previously 'truck number' or 'trucknum')
    /// @param pos
    /// @param rot
    /// @param fname Rig file name.
    /// @param actor_config Networking related.
    /// @param preloaded_with_terrain Is this rig being pre-loaded along with terrain?
    Actor(
          int actor_id
        , unsigned int vector_index
        , std::shared_ptr<RigDef::File> def
        , RoR::ActorSpawnRequest rq
        );

    ~Actor();

    void              PushNetwork(char* data, int size);   //!< Parses network data; fills actor's data buffers and flips them. Called by the network thread.
    void              CalcNetwork();
    void              UpdateNetworkInfo();
    bool              AddTyrePressure(float v);
    float             GetTyrePressure();
    float             getRotation();
    Ogre::Vector3     getDirection();
    Ogre::Vector3     getPosition();
    /// Moves the actor.
    /// @param translation Offset to move in world coordinates
    /// @param setInitPosition Set initial positions of nodes to current position?
    void              ResetPosition(Ogre::Vector3 translation, bool setInitPosition);
    void              RequestRotation(float rotation);
    void              RequestTranslation(Ogre::Vector3 translation);
    Ogre::Vector3     GetRotationCenter();                 //!< Return the rotation center of the actor
    bool              ReplayStep();
    void              ForceFeedbackStep(int steps);
    void              HandleInputEvents(float dt);
    void              HandleAngelScriptEvents(float dt);
    void              UpdateSoundSources();
    void              HandleMouseMove(int node, Ogre::Vector3 pos, float force); //!< Event handler
    void              ToggleLights();                      //!< Event handler
    void              ToggleTies(int group=-1);
    void              ToggleRopes(int group=-1);            //!< Event handler
    void              ToggleHooks(int group=-1, hook_states mode=HOOK_TOGGLE, int node_number=-1); //!< Event handler
    void              EngineTriggerHelper(int engineNumber, int type, float triggerValue);
    void              ToggleSlideNodeLock();
    void              ToggleCustomParticles();
    void              ToggleAxleLock();                    //! diff lock on or off
    void              ToggleParkingBrake();                //!< Event handler
    void              ToggleAntiLockBrake();               //!< Event handler
    void              ToggleTractionControl();             //!< Event handler
    void              ToggleCruiseControl();               //!< Event handler
    void              ToggleBeacons();                     //!< Event handler
    void              forwardCommands();
    void              setReplayMode(bool rm);              //!< Event handler; toggle replay mode.
    int               savePosition(int position);
    int               loadPosition(int position);
    /// Virtually moves the actor at most 'direction.length()' meters towards 'direction' trying to resolve any collisions
    /// Returns a minimal offset by which the actor needs to be moved to resolve any collisions
    Ogre::Vector3     calculateCollisionOffset(Ogre::Vector3 direction);
    /// Moves the actor at most 'direction.length()' meters towards 'direction' to resolve any collisions
    void              resolveCollisions(Ogre::Vector3 direction);
    /// Auto detects an ideal collision avoidance direction (front, back, left, right, up)
    /// Then moves the actor at most 'max_distance' meters towards that direction to resolve any collisions
    void              resolveCollisions(float max_distance, bool consider_up);
    void              updateSkidmarks();                   //!< Creates or updates skidmarks. No side effects.
    void              prepareInside(bool inside);          //!< Prepares vehicle for in-cabin camera use.
    void              UpdateFlareStates(float dt_sec);
    void              updateVisual(float dt=0);
    void              setDetailLevel(int v);               //!< @param v 0 = full detail, 1 = no beams
    void              resetAutopilot();
    void              disconnectAutopilot();
    void              ScaleActor(float value);
    Ogre::String      getAxleLockName();                   //!< get the name of the current differential model
    int               getAxleLockCount();
    Ogre::Vector3     getGForces();
    float             getSteeringAngle();
    std::string       GetActorDesignName();
    std::string       GetActorFileName();
    int               GetActorType();
    int               GetNumActiveConnectedBeams(int nodeid);     //!< Returns the number of active (non bounded) beams connected to a node
    void              NotifyActorCameraChanged();                 //!< Logic: sound, display; Notify this vehicle that camera changed;
    void              StopAllSounds();
    void              UnmuteAllSounds();
    float             getTotalMass(bool withLocked=true);
    float             getAvgPropedWheelRadius() { return m_avg_proped_wheel_radius; };
    void              recalc_masses();
    int               getWheelNodeCount();
    void              setMass(float m);
    bool              getBrakeLightVisible();
    bool              getReverseLightVisible();            //!< Tells if the reverse-light is currently lit.
    bool              getCustomLightVisible(int number);
    void              setCustomLightVisible(int number, bool visible);
    bool              getBeaconMode();
    void              setBlinkType(blinktype blink);    
    void              setAirbrakeIntensity(float intensity);
    bool              getCustomParticleMode();
    int               getLowestNode();
    void              receiveStreamData(unsigned int type, int source, unsigned int streamid, char *buffer, unsigned int len);
    void              sendStreamData();
    bool              isTied();
    bool              isLocked(); 
    void              updateDashBoards(float dt);
    void              UpdateBoundingBoxes();
    void              calculateAveragePosition();
    void              UpdatePhysicsOrigin();
    bool              CalcForcesEulerPrepare();            //!< A single physics step (see PHYSICS_DT)
    void              calcForcesEulerCompute(int step, int num_steps); //!< TIGHT LOOP; Physics;
    void              SyncReset(bool reset_position);      //!< this one should be called only synchronously (without physics running in background)
    blinktype         getBlinkType();
    std::vector<authorinfo_t>     getAuthors();
    std::vector<std::string>      getDescription();
    RoR::PerVehicleCameraContext* GetCameraContext()    { return &m_camera_context; }
    std::list<Actor*> GetAllLinkedActors()              { return m_linked_actors; }; //!< Returns a list of all connected (hooked) actors
    Ogre::Vector3     GetCameraDir()                    { return (ar_nodes[ar_main_camera_node_pos].RelPosition - ar_nodes[ar_main_camera_node_dir].RelPosition).normalisedCopy(); }
    Ogre::Vector3     GetCameraRoll()                   { return (ar_nodes[ar_main_camera_node_pos].RelPosition - ar_nodes[ar_main_camera_node_roll].RelPosition).normalisedCopy(); }
    Ogre::Vector3     GetFFbBodyForces() const          { return m_force_sensors.out_body_forces; }
    PointColDetector* IntraPointCD()                    { return m_intra_point_col_detector; }
    PointColDetector* InterPointCD()                    { return m_inter_point_col_detector; }
    RoR::GfxActor*    GetGfxActor()                     { return m_gfx_actor.get(); }
    void              RequestUpdateHudFeatures()        { m_hud_features_ok = false; }
    Ogre::Vector3     getNodePosition(int nodeNumber);     //!< Returns world position of node
    Ogre::Real        getMinimalCameraRadius();
    Replay*           getReplay();
    float             GetFFbHydroForces() const         { return m_force_sensors.out_hydros_forces; }
    bool              isPreloadedWithTerrain() const    { return m_preloaded_with_terrain; };
    VehicleAI*        getVehicleAI()                    { return ar_vehicle_ai; }
    float             getWheelSpeed() const             { return ar_wheel_speed; }
    int               GetNumNodes() const               { return ar_num_nodes; }
    Ogre::Vector3     getVelocity() const               { return m_avg_node_velocity; }; //!< average actor velocity, calculated using the actor positions of the last two frames
    bool              GetUseSkidmarks() const           { return m_use_skidmarks; }
    int               GetGfxDetailLevel() const         { return m_gfx_detail_level; }
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
    std::vector<rope_t>       ar_ropes;
    std::vector<ropable_t>    ar_ropables;
    std::vector<tie_t>        ar_ties;
    std::vector<hook_t>       ar_hooks;
    std::vector<flare_t>      ar_flares;
    Ogre::AxisAlignedBox      ar_bounding_box;     //!< standard bounding box (surrounds all nodes of an actor)
    Ogre::AxisAlignedBox      ar_predicted_bounding_box;
    std::vector<std::vector<int>>  ar_node_to_node_connections;
    std::vector<std::vector<int>>  ar_node_to_beam_connections;
    std::vector<Ogre::AxisAlignedBox>  ar_collision_bounding_boxes; //!< smart bounding boxes, used for determining the state of an actor (every box surrounds only a subset of nodes)
    std::vector<Ogre::AxisAlignedBox>  ar_predicted_coll_bounding_boxes;
    contacter_t       ar_contacters[MAX_CONTACTERS];
    int               ar_num_contacters;
    wheel_t           ar_wheels[MAX_WHEELS];
    int               ar_num_wheels;
    command_t         ar_command_key[MAX_COMMANDS + 10]; // 0 for safety
    cparticle_t       ar_custom_particles[MAX_CPARTICLES];
    int               ar_num_custom_particles;
    soundsource_t     ar_soundsources[MAX_SOUNDSCRIPTS_PER_TRUCK];
    int               ar_num_soundsources;
    int               ar_pressure_beams[MAX_PRESSURE_BEAMS];
    int               ar_free_pressure_beam;
    AeroEngine*       ar_aeroengines[MAX_AEROENGINES];
    int               ar_num_aeroengines;
    Screwprop*        ar_screwprops[MAX_SCREWPROPS];
    int               ar_num_screwprops;
    int               ar_cabs[MAX_CABS*3];
    int               ar_num_cabs;
    std::vector<hydrobeam_t> ar_hydros;
    int               ar_collcabs[MAX_CABS];
    collcab_rate_t    ar_inter_collcabrate[MAX_CABS];
    collcab_rate_t    ar_intra_collcabrate[MAX_CABS];
    int               ar_num_collcabs;
    int               ar_buoycabs[MAX_CABS];
    int               ar_buoycab_types[MAX_CABS];
    int               ar_num_buoycabs;
    Airbrake*         ar_airbrakes[MAX_AIRBRAKES];
    int               ar_num_airbrakes;
    int               ar_camera_rail[MAX_CAMERARAIL]; //!< Nodes defining camera-movement spline
    int               ar_num_camera_rails;
    bool              ar_hide_in_actor_list;      //!< Hide in list of spawned actors (available in top menubar). Useful for fixed-place machinery, i.e. cranes.
    Ogre::String      ar_design_name;             //!< Name of the vehicle/machine/object this actor represents
    float             ar_anim_previous_crank;     //!< For 'animator' with flag 'torque'
    float             alb_ratio;          //!< Anti-lock brake attribute: Regulating force
    float             alb_minspeed;       //!< Anti-lock brake attribute;
    bool              alb_mode;           //!< Anti-lock brake state; Enabled? {1/0}
    float             alb_pulse_time;     //!< Anti-lock brake attribute;
    bool              alb_pulse_state;    //!< Anti-lock brake state;
    bool              alb_nodash;         //!< Anti-lock brake attribute: Hide the dashboard indicator?
    bool              alb_notoggle;       //!< Anti-lock brake attribute: Disable in-game toggle?
    float             alb_timer;          //!< Anti-lock brake state;
    float             tc_ratio;           //!< Traction control attribute: Regulating force
    bool              tc_mode;            //!< Traction control state; Enabled? {1/0}
    float             tc_pulse_time;      //!< Traction control attribute;
    bool              tc_pulse_state;     //!< Traction control state;
    bool              tc_nodash;          //!< Traction control attribute; Hide the dashboard indicator?
    bool              tc_notoggle;        //!< Traction control attribute; Disable in-game toggle?
    float             tc_timer;           //!< Traction control state;
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
    int               ar_exhaust_pos_node; //!< Old-format exhaust (one per vehicle) emitter node
    int               ar_exhaust_dir_node; //!< Old-format exhaust (one per vehicle) backwards direction node
    int               ar_instance_id;              //!< Static attr; session-unique ID
    unsigned int      ar_vector_index;             //!< Sim attr; actor element index in std::vector<m_actors>
    int               ar_driveable;                //!< Sim attr; marks vehicle type and features
    EngineSim*        ar_engine;
    int               ar_cinecam_node[MAX_CAMERAS];//!< Sim attr; Cine-camera node indexes
    int               ar_num_cinecams;             //!< Sim attr;
    std::string       ar_help_panel_material;      //!< GUI attr, defined in truckfile
    Autopilot*        ar_autopilot;
    float             ar_brake_force;              //!< Physics attr; filled at spawn
    float             ar_speedo_max_kph;           //!< GUI attr
    Ogre::Vector3     ar_origin;                   //!< Physics state; base position for softbody nodes
    int               ar_num_cameras;
    Ogre::Quaternion  ar_main_camera_dir_corr;     //!< Sim attr;
    int               ar_main_camera_node_pos;     //!< Sim attr; ar_camera_node_pos[0]  >= 0 ? ar_camera_node_pos[0]  : 0
    int               ar_main_camera_node_dir;     //!< Sim attr; ar_camera_node_dir[0]  >= 0 ? ar_camera_node_dir[0]  : 0
    int               ar_main_camera_node_roll;    //!< Sim attr; ar_camera_node_roll[0] >= 0 ? ar_camera_node_roll[0] : 0
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
    float             ar_wheel_spin;          //!< Physics state; wheel speed in radians/s
    float             ar_avg_wheel_speed;     //!< Physics state; avg wheel speed in m/s
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
    bool              ar_parking_brake;
    int               ar_lights;                      //!< boolean 1/0
    float             ar_left_mirror_angle;           //!< Sim state; rear view mirror angle
    float             ar_right_mirror_angle;          //!< Sim state; rear view mirror angle
    float             ar_elevator;                    //!< Sim state; aerial controller
    float             ar_rudder;                      //!< Sim state; aerial/marine controller
    float             ar_aileron;                     //!< Sim state; aerial controller
    int               ar_aerial_flap;                 //!< Sim state; state of aircraft flaps (values: 0-5)
    Ogre::Vector3     ar_fusedrag;                    //!< Physics state
    int               ar_current_cinecam;             //!< Sim state; index of current CineCam (-1 if using 3rd-person camera)
    int               ar_custom_camera_node;          //!< Sim state; custom tracking node for 3rd-person camera
    std::string       ar_filename;                    //!< Attribute; filled at spawn
    int               ar_airbrake_intensity;          //!< Physics state; values 0-5
    int               ar_net_source_id;
    int               ar_net_stream_id;
    std::map<int,int> ar_net_stream_results;
    Ogre::Timer       ar_net_timer;
    unsigned long     ar_net_last_update_time;
    DashBoardManager* ar_dashboard;
    SimState          ar_sim_state;                   //!< Sim state
    float             ar_collision_range;             //!< Physics attr
    ground_model_t*   ar_last_fuzzy_ground_model;     //!< GUI state

    // Bit flags
    bool ar_left_blink_on:1;  //!< Gfx state; turn signals
    bool ar_right_blink_on:1; //!< Gfx state; turn signals
    bool ar_warn_blink_on:1;  //!< Gfx state; turn signals
    bool ar_update_physics:1; //!< Physics state; Should this actor be updated (locally) in the next physics step?
    bool ar_disable_self_collision:1; //!< Physics attribute; clone of RoR.cfg entry "DisableSelfCollisions"
    bool ar_disable_actor2actor_collision:1; //!< Physics attribute; clone of RoR.cfg entry "DisableCollisions"
    bool ar_disable_aerodyn_turbulent_drag:1; //!< Physics state
    bool ar_engine_hydraulics_ready:1; //!< Sim state; does engine have enough RPM to power hydraulics?
    bool ar_gui_use_engine_max_rpm:1;  //!< Gfx attr
    bool ar_hydro_speed_coupling:1;
    bool ar_collision_relevant:1;      //!< Physics state;
    bool ar_replay_mode:1;      //!< Sim state
    bool ar_is_police:1;        //!< Gfx/sfx attr
    bool ar_use_heathaze:1;     //!< Spawner context; TODO:remove
    bool ar_uses_networking:1;  //!< Networking attr; This actor is either remote or has remote counterpart
    bool ar_rescuer_flag:1;     //!< Gameplay attr; defined in truckfile. TODO: Does anybody use this anymore?
    bool ar_forward_commands:1; //!< Sim state
    bool ar_import_commands:1;  //!< Sim state

private:

    void              CalcBeams(bool trigger_hooks);       //!< Single physics step (see PHYSICS_DT); Physics & sound;
    void              CalcBeamsInterActor();               //!< Physics (1 step) & sound - only beams between multiple actors (noshock or ropes)
    void              CalcNodes();                         //!< Single physics step (see PHYSICS_DT)
    void              calcHooks();                         //!< TIGHT LOOP; Physics;
    void              calcRopes();                         //!< TIGHT LOOP; Physics;
    void              calcShocks2(int beam_i, Ogre::Real difftoBeamL, Ogre::Real &k, Ogre::Real &d, bool update_hooks);
    void              calcAnimators(const int flag_state, float &cstate, int &div, float timer, const float lower_limit, const float upper_limit, const float option3);
    void              DetermineLinkedActors();
    void              RecalculateNodeMasses(Ogre::Real total, bool reCalc=false); //!< Previously 'calc_masses2()'
    void              calcNodeConnectivityGraph();
    void              AddInterActorBeam(beam_t* beam, Actor* a, Actor* b);
    void              RemoveInterActorBeam(beam_t* beam);
    void              DisjoinInterActorBeams();            //!< Destroys all inter-actor beams which are connected with this actor
    void              autoBlinkReset();                    //!< Resets the turn signal when the steering wheel is turned back.
    void              sendStreamSetup();
    void              updateSlideNodeForces(const Ogre::Real delta_time_sec); //!< calculate and apply Corrective forces
    void              resetSlideNodePositions();           //!< Recalculate SlideNode positions
    void              resetSlideNodes();                   //!< Reset all the SlideNodes
    void              updateSlideNodePositions();          //!< incrementally update the position of all SlideNodes
    void              ResetAngle(float rot);
    void              ResetPosition(float px, float pz, bool setInitPosition, float miny);
    /// @param actor which actor to retrieve the closest Rail from
    /// @param node which SlideNode is being checked against
    /// @return a pair containing the rail, and the distant to the SlideNode
    std::pair<RailGroup*, Ogre::Real> GetClosestRailOnActor( Actor* actor, const SlideNode& node);

    // -------------------- data -------------------- //

    std::vector<std::shared_ptr<Task>> m_flexbody_tasks;   //!< Gfx state
    std::shared_ptr<RigDef::File>      m_definition;
    std::unique_ptr<RoR::GfxActor>     m_gfx_actor;
    RoR::PerVehicleCameraContext       m_camera_context;
    std::vector<Ogre::String>          m_actor_config;
    std::vector<SlideNode>             m_slidenodes;       //!< all the SlideNodes available on this actor
    std::vector<RailGroup*>            m_railgroups;       //!< all the available RailGroups for this actor
    std::vector<Ogre::Entity*>         m_deletion_entities;    //!< For unloading vehicle; filled at spawn.
    std::vector<Ogre::SceneNode*>      m_deletion_scene_nodes; //!< For unloading vehicle; filled at spawn.
    int               m_proped_wheel_pairs[MAX_WHEELS];    //!< Physics attr; For inter-differential locking
    int               m_num_braked_wheels;          //!< Physics attr, filled at spawn - Number of braked wheels.
    int               m_num_proped_wheels;          //!< Physics attr, filled at spawn - Number of propelled wheels.
    float             m_avg_proped_wheel_radius;    //!< Physics attr, filled at spawn - Average proped wheel radius.
    float             m_avionic_chatter_timer;      //!< Sound fx state
    PointColDetector* m_inter_point_col_detector;   //!< Physics
    PointColDetector* m_intra_point_col_detector;   //!< Physics
    std::list<Actor*>  m_linked_actors;              //!< Sim state; other actors linked using 'hooks'
    Ogre::Vector3     m_avg_node_position;          //!< average node position
    Ogre::Real        m_min_camera_radius;
    Ogre::Vector3     m_avg_node_position_prev;
    Ogre::Vector3     m_avg_node_velocity;          //!< average node velocity (compared to the previous frame step)
    Ogre::Real        m_replay_timer;               //!< Sim state
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
    RoRnet::VehicleState* oob1;                  //!< Network; Triple buffer for incoming data (actor properties)
    RoRnet::VehicleState* oob2;                  //!< Network; Triple buffer for incoming data (actor properties)
    RoRnet::VehicleState* oob3;                  //!< Network; Triple buffer for incoming data (actor properties)
    char*             netb1;                   //!< Network; Triple buffer for incoming data
    char*             netb2;                   //!< Network; Triple buffer for incoming data
    char*             netb3;                   //!< Network; Triple buffer for incoming data
    int               m_net_time_offset;
    int               m_net_update_counter;
    Ogre::MovableText* m_net_label_mt;
    Ogre::SceneNode*  m_net_label_node;
    Ogre::String      m_net_username;
    float             m_custom_light_toggle_countdown; //!< Input system helper status
    float             m_rotation_request;         //!< Accumulator
    Ogre::Vector3     m_translation_request;      //!< Accumulator
    Ogre::Vector3     m_camera_gforces_accu;      //!< Accumulator for 'camera' G-forces
    int               m_camera_gforces_count;     //!< Counter for 'camera' G-forces
    float             m_ref_tyre_pressure;        //!< Physics state
    float             m_stabilizer_shock_ratio;   //!< Physics state
    int               m_stabilizer_shock_request; //!< Physics state; values: { -1, 0, 1 }
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
    Axle*             m_axles[MAX_WHEELS/2];   //!< Physics
    int               m_num_axles;             //!< Physics attr
    int               m_num_command_beams;     //!< TODO: Remove! Spawner context only; likely unused feature
    float             m_minimass;              //!< Physics attr; minimum node mass in Kg
    float             m_load_mass;             //!< Physics attr; predefined load mass in Kg
    int               m_masscount;             //!< Physics attr; Number of nodes loaded with l option
    float             m_dry_mass;              //!< Physics attr;
    unsigned int      m_net_custom_lights[4];  //!< Sim state
    unsigned char     m_net_custom_light_count;//!< Sim attr
    RoR::GfxFlaresMode m_flares_mode;          //!< Gfx attr, clone of GVar -- TODO: remove
    std::unique_ptr<Buoyance> m_buoyance;      //!< Physics
    RoR::SkinDef*     m_used_skin;             //!< Spawner context (TODO: remove!)
    RoR::Skidmark*    m_skid_trails[MAX_WHEELS*2];
    bool              m_antilockbrake;         //!< GUI state
    bool              m_tractioncontrol;       //!< GUI state
    bool              m_ongoing_reset;         //!< Hack to prevent position/rotation creep during interactive truck reset
    bool              m_has_axles_section;     //!< Temporary (legacy parsing helper) until central diffs are implemented

    bool m_hud_features_ok:1;      //!< Gfx state; Are HUD features matching actor's capabilities?
    bool m_slidenodes_locked:1;    //!< Physics state; Are SlideNodes locked?
    bool m_blinker_autoreset:1;    //!< Gfx state; We're steering - when we finish, the blinker should turn off
    bool m_net_brake_light:1;
    bool m_net_reverse_light:1;
    bool m_hide_own_net_label:1;
    bool m_reverse_light_active:1; //!< Gfx state
    bool m_water_contact:1;        //!< Scripting state
    bool m_water_contact_old:1;    //!< Scripting state
    bool m_has_command_beams:1;    //!< Physics attr;
    bool m_beacon_light_is_active:1;        //!< Gfx state
    bool m_custom_particles_enabled:1;      //!< Gfx state
    bool m_cinecam_is_rotation_center:1;    //<! Attribute; filled at spawn
    bool m_preloaded_with_terrain:1;        //!< Spawn context (TODO: remove!)
    bool m_gfx_reduce_shadows:1;        //!< Gfx switch; alias of RoR.cfg entry "Shadow optimizations"
    bool m_beam_break_debug_enabled:1;  //!< Logging state
    bool m_beam_deform_debug_enabled:1; //!< Logging state
    bool m_trigger_debug_enabled:1;     //!< Logging state
    bool m_disable_default_sounds:1;    //!< Spawner context; TODO: remove
    bool m_disable_smoke:1;             //!< Gfx state
    bool m_use_skidmarks:1;             //!< Gfx attr; filled at spawn from GVar

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
