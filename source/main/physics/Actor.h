/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2024 Petr Ohlidal

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
#include "CmdKeyInertia.h"
#include "Differentials.h"
#include "Engine.h"
#include "GfxActor.h"
#include "PerVehicleCameraContext.h"
#include "RigDef_Prerequisites.h"
#include "RoRnet.h"
#include "RefCountingObject.h"
#include "RefCountingObjectPtr.h"
#include "SimData.h"
#include "SoundScriptManager.h"
#include "TyrePressure.h"
#include "VehicleAI.h"

#include <Ogre.h>

namespace RoR {

/// @addtogroup Physics
/// @{

/// Softbody object; can be anything from soda can to a space shuttle
/// Constructed from a truck definition file, see https://docs.rigsofrods.org/vehicle-creation/fileformat-truck/
/// To spawn in-game, use `MSG_SIM_SPAWN_ACTOR_REQUESTED`, see `GameContext::PushMessage()`, in AngelScript use `game.pushMessage();`
/// Gameplay states are described by `enum ActorState`. For additional state vars see "Gameplay state" section below.
class Actor : public RefCountingObject<Actor>
{
    friend class ActorSpawner;
    friend class ActorManager;
    friend class GfxActor; // Temporary until all visuals are moved there. ~ only_a_ptr, 2018
    friend class OutGauge;
public:

    Actor(
          ActorInstanceID_t actor_id
        , unsigned int vector_index
        , RigDef::DocumentPtr def
        , ActorSpawnRequest rq
        );

    virtual ~Actor() override;

    void              dispose(); //!< Effectively destroys the object but keeps it in memory to satisfy shared pointers.

    /// @name Networking
    /// @{
    void              sendStreamSetup();
    void              sendStreamData();                    //!< Send outgoing data
    void              pushNetwork(char* data, int size);   //!< Process incoming data; fills actor's data buffers and flips them. Called by the network thread.//! 
    void              calcNetwork();
    /// @}

    /// @name Physics state
    /// @{
    // PLEASE maintain the same order as in 'scripting/bindings/ActorAngelscript.cpp' and 'doc/angelscript/.../BeamClass.h'
    ActorState        getTruckState() { return ar_state; }
    Ogre::Vector3     getPosition();
    // AngelScript alias: `getVehiclePosition()`
    float             getRotation();
    // AngelScript alias: `getHeadingDirectionAngle()`
    Ogre::Quaternion  getOrientation();
    float             getSpeed() { return m_avg_node_velocity.length(); };
    Ogre::Vector3     getGForces() { return m_camera_local_gforces_cur; };
    float             getTotalMass(bool withLocked=true);
    int               getNodeCount() { return ar_num_nodes; }
    Ogre::Vector3     getNodePosition(int nodeNumber);     //!< Returns world position of node
    bool              isNodeWheelRim(int nodeNumber);      //!< Is node marked as wheel rim? Note some wheel models use only tire nodes. See https://docs.rigsofrods.org/vehicle-creation/fileformat-truck/#wheels
    bool              isNodeWheelTire(int nodeNumber);     //!< Is node marked as wheel tire? Note some wheel models use only tire nodes. See https://docs.rigsofrods.org/vehicle-creation/fileformat-truck/#wheels
    int               getWheelNodeCount() const;
    float             getWheelSpeed() const { return ar_wheel_speed; }
    void              reset(bool keep_position = false);   //!< call this one to reset a truck from any context
    float             getShockSpringRate(int shock_number);
    float             getShockDamping(int shock_number);
    float             getShockVelocity(int shock_number);
    int               getShockNode1(int shock_number);
    int               getShockNode2(int shock_number);
    // not exported to scripting:
    void              resetPosition(Ogre::Vector3 translation, bool setInitPosition); //!< Moves the actor to given world coords (pivot point is node 0).
    void              resetPosition(float px, float pz, bool setInitPosition, float miny); //!< Moves the actor to given world coords (pivot point is node 0).
    void              softRespawn(Ogre::Vector3 spawnpos, Ogre::Quaternion spawnrot); //!< Use `MSG_SIM_MODIFY_ACTOR_REQUESTED` with type `SOFT_RESPAWN`; Resets the actor to given position as if spawned there (pivot point is `spawnpos`).
    void              requestRotation(float rotation, Ogre::Vector3 center) { m_rotation_request += rotation; m_rotation_request_center = center; };
    void              requestAngleSnap(int division) { m_anglesnap_request = division; };
    void              requestTranslation(Ogre::Vector3 translation) { m_translation_request += translation; };
    int               getShockCount() { return ar_num_shocks; }
    Ogre::Vector3     getVelocity() const { return m_avg_node_velocity; }; //!< average actor velocity, calculated using the actor positions of the last two frames
    Ogre::Vector3     getDirection();
    Ogre::Vector3     getRotationCenter();
    float             getMinHeight(bool skip_virtual_nodes=true);
    float             getMaxHeight(bool skip_virtual_nodes=true);
    float             getHeightAboveGround(bool skip_virtual_nodes=true);
    float             getHeightAboveGroundBelow(float height, bool skip_virtual_nodes=true);
    Ogre::Vector3     getMaxGForces() { return m_camera_local_gforces_max; };
    bool              hasSlidenodes() { return !m_slidenodes.empty(); };
    void              updateSlideNodePositions();          //!< incrementally update the position of all SlideNodes
    void              updateSlideNodeForces(const Ogre::Real delta_time_sec); //!< calculate and apply Corrective forces
    void              resetSlideNodePositions();           //!< Recalculate SlideNode positions
    void              resetSlideNodes();                   //!< Reset all the SlideNodes
    /// @}

    /// @name Physics editing
    /// @{
    // PLEASE maintain the same order as in 'scripting/bindings/ActorAngelscript.cpp' and 'doc/angelscript/.../BeamClass.h'
    void              scaleTruck(float value);
    void              setMass(float m);
    void              setLoadedMass(float m);
    void              setSimAttribute(ActorSimAttr attr, float val); //!< HAZARDOUS - values may not be checked; Pay attention to 'safe values' at each attribute description.
    float             getSimAttribute(ActorSimAttr attr);
    // not exported to scripting:
    void              applyNodeBeamScales();               //!< For GUI::NodeBeamUtils
    void              searchBeamDefaults();                //!< Searches for more stable beam defaults
    void              updateInitPosition();
    void              propagateNodeBeamChangesToDef(); //!< Back-propagates changes done by N/B-utils UI to the def-document.
    void              recalculateNodeMasses();
    /// @}

    /// @name User interaction
    /// @{
    // PLEASE maintain the same order as in 'scripting/bindings/ActorAngelscript.cpp' and 'doc/angelscript/.../BeamClass.h'
    void              parkingbrakeToggle();
    void              tractioncontrolToggle();
    void              antilockbrakeToggle();
    void              toggleCustomParticles();
    bool              getCustomParticleMode();
    bool              isLocked();                          //!< Are hooks locked?
    void              setForcedCinecam(CineCameraID_t cinecam_id, BitMask_t flags);
    void              clearForcedCinecam();
    bool              getForcedCinecam(CineCameraID_t& cinecam_id, BitMask_t& flags);
    int               getNumCinecams() { return ar_num_cinecams; }
    // not exported to scripting:
    void              mouseMove(NodeNum_t node, Ogre::Vector3 pos, float force);
    void              tieToggle(int group=-1, ActorLinkingRequestType mode=ActorLinkingRequestType::TIE_TOGGLE, ActorInstanceID_t forceunlock_filter=ACTORINSTANCEID_INVALID);
    bool              isTied();
    void              hookToggle(int group=-1, ActorLinkingRequestType mode=ActorLinkingRequestType::HOOK_TOGGLE,NodeNum_t mousenode=NODENUM_INVALID, ActorInstanceID_t forceunlock_filter=ACTORINSTANCEID_INVALID);
    void              ropeToggle(int group=-1, ActorLinkingRequestType mode=ActorLinkingRequestType::ROPE_TOGGLE, ActorInstanceID_t forceunlock_filter=ACTORINSTANCEID_INVALID);
    void              engineTriggerHelper(int engineNumber, EngineTriggerType type, float triggerValue);
    void              toggleSlideNodeLock();
    bool              getParkingBrake() { return ar_parking_brake; }
    void              cruisecontrolToggle();               //!< Defined in 'gameplay/CruiseControl.cpp'
    void              toggleAxleDiffMode();                //! Cycles through the available inter axle diff modes
    void              displayAxleDiffMode();               //! Writes info to console/notify box
    int               getAxleDiffMode() { return m_num_axle_diffs; }
    void              toggleWheelDiffMode();               //! Cycles through the available inter wheel diff modes
    void              displayWheelDiffMode();              //! Writes info to console/notify box
    int               getWheelDiffMode() { return m_num_wheel_diffs; }
    void              toggleTransferCaseMode();            //! Toggles between 2WD and 4WD mode
    TransferCase*     getTransferCaseMode() { return m_transfer_case; }
    void              toggleTransferCaseGearRatio();       //! Toggles between Hi and Lo mode
    Ogre::String      getTransferCaseName();               //! Gets the current transfer case mode name (4WD Hi, ...)
    void              displayTransferCaseMode();           //! Writes info to console/notify area
    void              setSmokeEnabled(bool enabled) { m_disable_smoke = !enabled; }
    bool              getSmokeEnabled() const { return !m_disable_smoke; }
    //! @}

    /// @name Vehicle lights
    /// @{
    // PLEASE maintain the same order as in 'scripting/bindings/ActorAngelscript.cpp' and 'doc/angelscript/.../BeamClass.h'
    BlinkType         getBlinkType();
    void              setBlinkType(BlinkType blink);
    void              toggleBlinkType(BlinkType blink);
    bool              getCustomLightVisible(int number);
    void              setCustomLightVisible(int number, bool visible);
    bool              getBeaconMode() const { return m_lightmask & RoRnet::LIGHTMASK_BEACONS; }
    void              beaconsToggle();
    bool              getBrakeLightVisible() const { return m_lightmask & RoRnet::LIGHTMASK_BRAKES; }
    bool              getReverseLightVisible() const { return m_lightmask & RoRnet::LIGHTMASK_REVERSE; }
    int               countCustomLights(int control_number);
    int               countFlaresByType(FlareType type);
    // not exported to scripting:
    void              toggleHeadlights();
    BitMask_t         getLightStateMask() const { return m_lightmask; }
    void              setLightStateMask(BitMask_t lightmask); //!< Does all the necessary toggling.
    void              importLightStateMask(BitMask_t lightmask); //!< Only for linked (locked/tied) actors forwarding flare states; see 'flaregroups_no_import' in .truck format.
    bool              getSideLightsVisible() const { return m_lightmask & RoRnet::LIGHTMASK_SIDELIGHTS; }
    void              setSideLightsVisible(bool val) { BITMASK_SET(m_lightmask, RoRnet::LIGHTMASK_SIDELIGHTS, val); }
    bool              getHeadlightsVisible() const { return m_lightmask & RoRnet::LIGHTMASK_HEADLIGHT; }
    void              setHeadlightsVisible(bool val) { if (val != this->getHeadlightsVisible()) { this->toggleHeadlights(); } }
    bool              getHighBeamsVisible() const { return m_lightmask & RoRnet::LIGHTMASK_HIGHBEAMS; }
    void              setHighBeamsVisible(bool val) { BITMASK_SET(m_lightmask, RoRnet::LIGHTMASK_HIGHBEAMS, val); }
    bool              getFogLightsVisible() const { return m_lightmask & RoRnet::LIGHTMASK_FOGLIGHTS; }
    void              setFogLightsVisible(bool val) { BITMASK_SET(m_lightmask, RoRnet::LIGHTMASK_FOGLIGHTS, val); }
    void              setBeaconMode(bool val) { BITMASK_SET(m_lightmask, RoRnet::LIGHTMASK_BEACONS, val); }
    //! @}

    /// @name Visual state updates
    /// @{
    void              updateSkidmarks();                   //!< Creates or updates skidmarks.
    void              prepareInside(bool inside);          //!< Prepares vehicle for in-cabin camera use.
    void              updateFlareStates(float dt);
    void              updateVisual(float dt=0);
    void              updateDashBoards(float dt);
    void              forceAllFlaresOff();
    //! @}

    /// @name Audio
    /// @{
    void              updateSoundSources();
    void              muteAllSounds();
    void              unmuteAllSounds();
    //! @}

    /// @name Subsystems
    /// @{
    VehicleAIPtr      getVehicleAI() { return ar_vehicle_ai; }
    Ogre::MaterialPtr        getManagedMaterialInstance(const std::string& orig_name);
    std::vector<std::string> getManagedMaterialNames();
    // not exported to scripting:
    Replay*           getReplay();
    TyrePressure&     getTyrePressure() { return m_tyre_pressure; }
    EnginePtr      getEngine() { return ar_engine; }
    //! @}

    /// @name Organizational
    /// @{
    // PLEASE maintain the same ordering as in 'scripting/bindings/ActorAngelscript.cpp' and 'doc/angelscript/.../BeamClass.h'
    std::string       getTruckName() { return ar_design_name; }
    std::string       getTruckFileName() { return ar_filename; }
    std::string       getTruckFileResourceGroup();
    int               getTruckType() { return ar_driveable; }
    Ogre::String      getSectionConfig() { return m_section_config; }
    int               getInstanceId() { return ar_instance_id; }
    // not exported to scripting:
    CacheEntryPtr&    getUsedActorEntry(); //!< The actor entry itself.
    CacheEntryPtr&    getUsedSkinEntry();
    TuneupDefPtr&     getWorkingTuneupDef();
    void              ensureWorkingTuneupDef(); //!< Creates a working tuneup def if it doesn't exist yet.
    void              removeWorkingTuneupDef(); //!< Deletes the working tuneup def object if it exists.
    bool              isPreloadedWithTerrain() const { return m_preloaded_with_terrain; };
    std::vector<authorinfo_t> getAuthors();
    std::vector<std::string>  getDescription();
    //! @}

    void              ForceFeedbackStep(int steps);
    void              HandleInputEvents(float dt);
    void              HandleAngelScriptEvents(float dt);
    void              UpdateCruiseControl(float dt);       //!< Defined in 'gameplay/CruiseControl.cpp'
    bool              Intersects(ActorPtr actor, Ogre::Vector3 offset = Ogre::Vector3::ZERO);  //!< Slow intersection test
    /// Moves the actor at most 'direction.length()' meters towards 'direction' to resolve any collisions
    void              resolveCollisions(Ogre::Vector3 direction);
    /// Auto detects an ideal collision avoidance direction (front, back, left, right, up)
    /// Then moves the actor at most 'max_distance' meters towards that direction to resolve any collisions
    void              resolveCollisions(float max_distance, bool consider_up);    
    float             getSteeringAngle();
    float             getMinCameraRadius() { return m_min_camera_radius; };
    int               GetNumActiveConnectedBeams(int nodeid);     //!< Returns the number of active (non bounded) beams connected to a node
    void              NotifyActorCameraChanged();                 //!< Logic: sound, display; Notify this vehicle that camera changed;
    float             getAvgPropedWheelRadius() { return m_avg_proped_wheel_radius; };
    void              setAirbrakeIntensity(float intensity);
    void              UpdateBoundingBoxes();
    void              calculateAveragePosition();
    void              UpdatePhysicsOrigin();
    void              SoftReset();
    void              SyncReset(bool reset_position);      //!< this one should be called only synchronously (without physics running in background)
    void              WriteDiagnosticDump(std::string const& filename);
    Ogre::Vector3     GetCameraDir()                    { return (ar_nodes[ar_main_camera_node_pos].RelPosition - ar_nodes[ar_main_camera_node_dir].RelPosition).normalisedCopy(); }
    Ogre::Vector3     GetCameraRoll()                   { return (ar_nodes[ar_main_camera_node_pos].RelPosition - ar_nodes[ar_main_camera_node_roll].RelPosition).normalisedCopy(); }
    Ogre::Vector3     GetFFbBodyForces() const          { return m_force_sensors.out_body_forces; }
    GfxActor*         GetGfxActor()                     { return m_gfx_actor.get(); }
    void              RequestUpdateHudFeatures()        { m_hud_features_ok = false; }
    Ogre::Real        getMinimalCameraRadius();
    float             GetFFbHydroForces() const         { return m_force_sensors.out_hydros_forces; }
    bool              isBeingReset() const              { return m_ongoing_reset; };
    void              UpdatePropAnimInputEvents();

    // -------------------- Public data -------------------- //

    /// @name Mass
    /// @{
    float             ar_dry_mass = 0.f;              //!< User-defined (editable via NBUtil); from 'globals' arg#1 - default for all nodes
    float             ar_original_dry_mass = 0.f;     //!< Un-edited value from 'globals' arg#1
    float             ar_load_mass = 0.f;             //!< User-defined (editable via NBUtil); from 'globals' arg#2 - only applies to nodes with 'l' flag
    float             ar_original_load_mass = 0.f;    //!< Un-edited value from 'globals' arg#2
    int               ar_masscount = 0;               //!< Calculated; Number of nodes loaded with l option
    float             ar_total_mass = 0.f;            //!< Calculated; total mass in Kg
    float             ar_initial_total_mass = 0.f;    //!< Calculated; total mass in Kg (snapshot at spawn)
    /// @}

    // Node data (split to layers)
    node_t*              ar_nodes = nullptr;
    int*                 ar_nodes_id = nullptr;                  //!< Number in truck file, -1 for nodes generated by wheels/cinecam
    std::string*         ar_nodes_name = nullptr;                //!< Name in truck file, only if defined with 'nodes2'
    std::vector<float>   ar_nodes_default_loadweights;           //!< 'set_node_defaults': load weight.
    std::vector<float>   ar_nodes_override_loadweights;          //!< 'nodes': 'l' flag and number.
    Ogre::Vector3*       ar_nodes_spawn_offsets = nullptr;       //!< Relative positions (incl. Tuning system tweaks) from the definition file, for spawn-like resetting (i.e. with terrain editor on preloaded actor).
    std::vector<BitMask_t> ar_nodes_options;                     //!< merged options from 'nodes' and 'set_node_defaults'
    std::vector<float>   ar_minimass;                            //!< minimum node mass in Kg - can be scaled in-game via NBUtil
    std::vector<float>   ar_orig_minimass;                       //!< minimum node mass in Kg - original unscaled values
    std::vector<float>   ar_initial_node_masses;
    std::vector<Ogre::Vector3> ar_initial_node_positions;        //!< Absolute world positions, for resetting to pristine state.

    // Node additional info
    bool                 ar_minimass_skip_loaded_nodes = false;
    int                  ar_nodes_name_top_length = 0; //!< For nicely formatted diagnostic output
    int                  ar_num_nodes = 0;

    // Beam data (split to layers)
    beam_t*              ar_beams = nullptr;
    int                  ar_num_beams = 0;
    std::vector<std::pair<float, float>> ar_initial_beam_defaults;
    std::vector<bool>    ar_beams_invisible;    //!< Used only by the exporter (for rendering, invisible beams simply get no mesh).
    std::vector<bool>    ar_beams_user_defined; //!< True for 'beams', false for wheels/cinecam/hooknode/wings/rotators etc...

    std::vector<beam_t*> ar_inter_beams;       //!< Beams connecting 2 actors
    shock_t*             ar_shocks = nullptr;            //!< Shock absorbers
    int                  ar_num_shocks = 0;        //!< Number of shock absorbers
    bool                 ar_has_active_shocks = false; //!< Are there active stabilizer shocks?
    rotator_t*           ar_rotators = nullptr;
    int                  ar_num_rotators = 0;
    wing_t*              ar_wings = nullptr;
    int                  ar_num_wings = 0;
    std::vector<authorinfo_t> authors;
    std::vector<rope_t>       ar_ropes;
    std::vector<ropable_t>    ar_ropables;
    std::vector<tie_t>        ar_ties;
    std::vector<hook_t>       ar_hooks;
    std::vector<flare_t>      ar_flares;
    std::vector<Airbrake*>    ar_airbrakes;
    CmdKeyArray               ar_command_key; //!< BEWARE: commandkeys are indexed 1-MAX_COMMANDS!
    Ogre::AxisAlignedBox      ar_bounding_box;     //!< standard bounding box (surrounds all nodes of an actor)
    Ogre::AxisAlignedBox      ar_evboxes_bounding_box; //!< bounding box around nodes eligible for eventbox triggering
    Ogre::AxisAlignedBox      ar_predicted_bounding_box;
    
    std::vector<wheeldetacher_t>   ar_wheeldetachers;
    std::vector<std::vector<int>>  ar_node_to_node_connections;
    std::vector<std::vector<int>>  ar_node_to_beam_connections;
    std::vector<Ogre::AxisAlignedBox>  ar_collision_bounding_boxes; //!< smart bounding boxes, used for determining the state of an actor (every box surrounds only a subset of nodes)
    std::vector<Ogre::AxisAlignedBox>  ar_predicted_coll_bounding_boxes;
    std::map<std::string, Ogre::MaterialPtr>  ar_managed_materials;
    std::vector<UniqueCommandKeyPair> ar_unique_commandkey_pairs; //!< UI helper for displaying command control keys to user (must be built at spawn).

    int               ar_num_contactable_nodes = 0; //!< Total number of nodes which can contact ground or cabs
    int               ar_num_contacters = 0; //!< Total number of nodes which can selfcontact cabs
    wheel_t           ar_wheels[MAX_WHEELS] = {};
    int               ar_num_wheels = 0;
    soundsource_t     ar_soundsources[MAX_SOUNDSCRIPTS_PER_TRUCK] = {};
    int               ar_num_soundsources = 0;
    AeroEngine*       ar_aeroengines[MAX_AEROENGINES] = {};
    int               ar_num_aeroengines = 0;
    Screwprop*        ar_screwprops[MAX_SCREWPROPS] = {};
    int               ar_num_screwprops = 0;
    int               ar_cabs[MAX_CABS * 3] = {};
    int               ar_num_cabs = 0;
    std::vector<hydrobeam_t> ar_hydros;
    int               ar_collcabs[MAX_CABS] = {};
    collcab_rate_t    ar_inter_collcabrate[MAX_CABS] = {};
    collcab_rate_t    ar_intra_collcabrate[MAX_CABS] = {};
    int               ar_num_collcabs = 0;
    int               ar_buoycabs[MAX_CABS] = {};
    int               ar_buoycab_types[MAX_CABS] = {};
    int               ar_num_buoycabs = 0;
    NodeNum_t         ar_camera_rail[MAX_CAMERARAIL] = {}; //!< Nodes defining camera-movement spline
    int               ar_num_camera_rails = 0;
    bool              ar_hide_in_actor_list = false;      //!< Hide in list of spawned actors (available in top menubar). Useful for fixed-place machinery, i.e. cranes.
    Ogre::String      ar_design_name;             //!< Name of the vehicle/machine/object this actor represents
    float             ar_anim_previous_crank = 0.f;     //!< For 'animator' with flag 'torque'
    float             alb_ratio = 0.f;          //!< Anti-lock brake attribute: Regulating force
    float             alb_minspeed = 0.f;       //!< Anti-lock brake attribute;
    bool              alb_mode = false;           //!< Anti-lock brake state; Enabled? {1/0}
    float             alb_pulse_time = 0.f;     //!< Anti-lock brake attribute;
    bool              alb_pulse_state = false;    //!< Anti-lock brake state;
    bool              alb_nodash = false;         //!< Anti-lock brake attribute: Hide the dashboard indicator?
    bool              alb_notoggle = false;       //!< Anti-lock brake attribute: Disable in-game toggle?
    float             alb_timer = 0.f;          //!< Anti-lock brake state;
    float             ar_anim_shift_timer = 0.f;//!< For 'animator' with flag 'shifter'
    bool              cc_mode = false;            //!< Cruise Control
    bool              cc_can_brake = false;       //!< Cruise Control
    float             cc_target_rpm = 0.f;      //!< Cruise Control
    float             cc_target_speed = 0.f;    //!< Cruise Control
    float             cc_target_speed_lower_limit = 0.f; //!< Cruise Control
    std::deque<float> cc_accs;            //!< Cruise Control
    bool              sl_enabled = false;         //!< Speed limiter;
    float             sl_speed_limit = 0.f;     //!< Speed limiter;
    ExtCameraMode     ar_extern_camera_mode = ExtCameraMode::CLASSIC;
    NodeNum_t         ar_extern_camera_node = NODENUM_INVALID;
    NodeNum_t         ar_exhaust_pos_node   = 0;   //!< Old-format exhaust (one per vehicle) emitter node
    NodeNum_t         ar_exhaust_dir_node   = 0;   //!< Old-format exhaust (one per vehicle) backwards direction node
    ActorInstanceID_t ar_instance_id = ACTORINSTANCEID_INVALID;              //!< Static attr; session-unique ID
    unsigned int      ar_vector_index = 0;             //!< Sim attr; actor element index in std::vector<m_actors>
    ActorType         ar_driveable = NOT_DRIVEABLE;                //!< Sim attr; marks vehicle type and features
    EnginePtr         ar_engine;
    NodeNum_t         ar_cinecam_node[MAX_CAMERAS] = {NODENUM_INVALID}; //!< Sim attr; Cine-camera node indexes
    int               ar_num_cinecams = 0;             //!< Sim attr;
    Autopilot*        ar_autopilot = nullptr;
    float             ar_brake_force = 0.f;              //!< Physics attr; filled at spawn
    
    Ogre::Vector3     ar_origin = Ogre::Vector3::ZERO;                   //!< Physics state; base position for softbody nodes
    int               ar_num_cameras = 0;
    Ogre::Quaternion  ar_main_camera_dir_corr = Ogre::Quaternion::IDENTITY;              //!< Sim attr;
    NodeNum_t         ar_main_camera_node_pos            = 0;    //!< Sim attr; ar_camera_node_pos[0]  >= 0 ? ar_camera_node_pos[0]  : 0
    NodeNum_t         ar_main_camera_node_dir            = 0;    //!< Sim attr; ar_camera_node_dir[0]  >= 0 ? ar_camera_node_dir[0]  : 0
    NodeNum_t         ar_main_camera_node_roll           = 0;    //!< Sim attr; ar_camera_node_roll[0] >= 0 ? ar_camera_node_roll[0] : 0
    NodeNum_t         ar_camera_node_pos[MAX_CAMERAS]    = {NODENUM_INVALID};  //!< Physics attr; 'camera' = frame of reference; origin node
    NodeNum_t         ar_camera_node_dir[MAX_CAMERAS]    = {NODENUM_INVALID};  //!< Physics attr; 'camera' = frame of reference; back node
    NodeNum_t         ar_camera_node_roll[MAX_CAMERAS]   = {NODENUM_INVALID};  //!< Physics attr; 'camera' = frame of reference; left node
    bool              ar_camera_node_roll_inv[MAX_CAMERAS] = {false};              //!< Physics attr; 'camera' = frame of reference; indicates roll node is right instead of left
    
    float             ar_posnode_spawn_height = 0.f;
    VehicleAIPtr      ar_vehicle_ai;
    float             ar_scale = 1.f;               //!< Physics state; scale of the actor (nominal = 1.0)
    Ogre::Real        ar_brake = 0.f;               //!< Physics state; braking intensity
    float             ar_wheel_speed = 0.f;         //!< Physics state; wheel speed in m/s
    float             ar_wheel_spin = 0.f;          //!< Physics state; wheel speed in radians/s
    float             ar_avg_wheel_speed = 0.f;     //!< Physics state; avg wheel speed in m/s
    float             ar_hydro_dir_command = 0.f;
    float             ar_hydro_dir_state = 0.f;
    Ogre::Real        ar_hydro_dir_wheel_display = 0.f;
    float             ar_hydro_aileron_command = 0.f;
    float             ar_hydro_aileron_state = 0.f;
    float             ar_hydro_rudder_command = 0.f;
    float             ar_hydro_rudder_state = 0.f;
    float             ar_hydro_elevator_command = 0.f;
    float             ar_hydro_elevator_state = 0.f;
    float             ar_sleep_counter = 0.f;               //!< Sim state; idle time counter
    ground_model_t*   ar_submesh_ground_model = nullptr;
    bool              ar_parking_brake = false;
    bool              ar_trailer_parking_brake = false;
    float             ar_left_mirror_angle = 0.52f;           //!< Sim state; rear view mirror angle
    float             ar_right_mirror_angle = -0.52f;          //!< Sim state; rear view mirror angle
    float             ar_elevator = 0.f;                    //!< Sim state; aerial controller
    float             ar_rudder = 0.f;                      //!< Sim state; aerial/marine controller
    float             ar_aileron = 0.f;                     //!< Sim state; aerial controller
    int               ar_aerial_flap = 0;                 //!< Sim state; state of aircraft flaps (values: 0-5)
    Ogre::Vector3     ar_fusedrag = Ogre::Vector3::ZERO;                    //!< Physics state
    std::string       ar_filename;                    //!< Attribute; filled at spawn
    std::string       ar_filehash;                    //!< Attribute; filled at spawn
    int               ar_airbrake_intensity = 0;          //!< Physics state; values 0-5
    int               ar_net_source_id = 0;               //!< Unique ID of remote player who spawned this actor
    int               ar_net_stream_id = 0;
    std::map<int,int> ar_net_stream_results;
    Ogre::Timer       ar_net_timer;
    unsigned long     ar_net_last_update_time = 0;
    DashBoardManager* ar_dashboard = nullptr;
    float             ar_collision_range = DEFAULT_COLLISION_RANGE;             //!< Physics attr
    float             ar_top_speed = 0.f;                   //!< Sim state
    ground_model_t*   ar_last_fuzzy_ground_model = nullptr;     //!< GUI state
    CollisionBoxPtrVec m_potential_eventboxes;
    std::vector<std::pair<collision_box_t*, NodeNum_t>> m_active_eventboxes;

    // Player camera 'cameras & cinecam'
    // * 'cinecam#' creates dedicated node to dictate camera position + 6 attachment beams.
    // * 'camera#' specifies a reference frame for the cinecam by referencing 3 preexisting nodes: ref, x, y.
    // NOTE camera#0 is special - serves a general orientation frame for the whole actor. Cinecam#0 isn't required to exist, but camera#0 is.
    CineCameraID_t    ar_current_cinecam = CINECAMERAID_INVALID; //!< Sim state; index of current CineCam (`CINECAMERAID_INVALID` if using 3rd-person camera)
    NodeNum_t         ar_custom_camera_node = NODENUM_INVALID; //!< Sim state; custom tracking node for 3rd-person camera
    PerVehicleCameraContext ar_camera_context;
    CineCameraID_t    ar_forced_cinecam = CINECAMERAID_INVALID; //!< Sim state; index of CineCam forced by script (`CINECAMERAID_INVALID` if not forced)
    BitMask_t         ar_forced_cinecam_flags = 0; //!< Sim state; flags for forced CineCam supplied by script

    // TractionControl
    float             tc_ratio = 0.f;                   //!< Regulating force
    bool              tc_mode = false;                  //!< Enabled?
    float             tc_pulse_time = 0.f;
    bool              tc_pulse_state = 0.f;
    bool              tc_nodash = false;                //!< Hide the dashboard indicator?
    bool              tc_notoggle = false;              //!< Disable in-game toggle?
    float             tc_timer = 0.f;
    float             tc_wheelslip_constant = 0.25f;    //!< use `ACTORSIMATTR_TC_WHEELSLIP_CONSTANT`

    // Guisettings
    bool              ar_guisettings_use_engine_max_rpm = false;
    float             ar_guisettings_speedo_max_kph = 0.f;
    float             ar_guisettings_shifter_anim_time = 0.4f;

    // Gameplay state
    ActorState        ar_state = ActorState::LOCAL_SIMULATED;
    ActorPtrVec       ar_linked_actors;                 //!< BEWARE: Includes indirect links, see `DetermineLinkedActors()`; Other actors linked using 'hooks/ties/ropes/slidenodes'; use `MSG_SIM_ACTOR_LINKING_REQUESTED`
    bool              m_ongoing_reset = false;          //!< Hack to prevent position/rotation creep during interactive truck reset (aka LiveRepair).
    bool              ar_physics_paused = false;        //!< Actor physics individually paused by user.
    bool              ar_muted_by_peeropt = false;      //!< Muted by user in multiplayer (see `RoRnet::PEEROPT_MUTE_ACTORS`).

    // Repair state
    Ogre::Vector3     m_rotation_request_center = Ogre::Vector3::ZERO;
    float             m_rotation_request = 0.f;         //!< Accumulator
    int               m_anglesnap_request = 0;        //!< Accumulator
    Ogre::Vector3     m_translation_request = Ogre::Vector3::ZERO;      //!< Accumulator

    // Realtime node/beam structure editing helpers
    bool                    ar_nb_initialized = false;
    std::vector<float>      ar_nb_optimum;            //!< Temporary storage of the optimum search result
    std::vector<float>      ar_nb_reference;          //!< Temporary storage of the reference search result
    int                     ar_nb_skip_steps = 0;         //!< Amount of physics steps to be skipped before measuring
    int                     ar_nb_measure_steps = 500;      //!< Amount of physics steps to be measured
    float                   ar_nb_minimass_scale = 1.f; //!< scale of 'set_default_minimass' (affects all nodes the same way)
    std::pair<float, float> ar_nb_beams_scale;        //!< Scales for springiness & damping of regular beams
    std::pair<float, float> ar_nb_shocks_scale;       //!< Scales for springiness & damping of shock beams
    std::pair<float, float> ar_nb_wheels_scale;       //!< Scales for springiness & damping of wheel / rim beams
    std::pair<float, float> ar_nb_beams_d_interval;   //!< Search interval for springiness & damping of regular beams
    std::pair<float, float> ar_nb_beams_k_interval;   //!< Search interval for springiness & damping of regular beams
    std::pair<float, float> ar_nb_shocks_d_interval;  //!< Search interval for springiness & damping of shock beams
    std::pair<float, float> ar_nb_shocks_k_interval;  //!< Search interval for springiness & damping of shock beams
    std::pair<float, float> ar_nb_wheels_d_interval;  //!< Search interval for springiness & damping of wheel / rim beams
    std::pair<float, float> ar_nb_wheels_k_interval;  //!< Search interval for springiness & damping of wheel / rim beams

    // Bit flags
    bool ar_update_physics:1; //!< Physics state; Should this actor be updated (locally) in the next physics step?
    bool ar_disable_aerodyn_turbulent_drag:1; //!< Physics state
    bool ar_engine_hydraulics_ready:1; //!< Sim state; does engine have enough RPM to power hydraulics?
    bool ar_hydro_speed_coupling:1;
    bool ar_collision_relevant:1;      //!< Physics state;
    bool ar_is_police:1;        //!< Gfx/sfx attr
    bool ar_rescuer_flag:1;     //!< Gameplay attr; defined in truckfile. TODO: Does anybody use this anymore?
    bool ar_forward_commands:1; //!< Sim state
    bool ar_import_commands:1;  //!< Sim state
    bool ar_toggle_ropes:1;     //!< Sim state
    bool ar_toggle_ties:1;      //!< Sim state
    bool ar_cparticles_active:1;//!< Gfx state

private:

    bool              CalcForcesEulerPrepare(bool doUpdate); 
    void              CalcAircraftForces(bool doUpdate);   
    void              CalcForcesEulerCompute(bool doUpdate, int num_steps); 
    void              CalcAnimators(hydrobeam_t const& hydrobeam, float &cstate, int &div);
    void              CalcBeams(bool trigger_hooks);       
    void              CalcBeamsInterActor();               
    void              CalcBuoyance(bool doUpdate);         
    void              CalcCommands(bool doUpdate);         
    void              CalcCabCollisions();                 
    void              CalcDifferentials();                 
    void              CalcForceFeedback(bool doUpdate);    
    void              CalcFuseDrag();                      
    void              CalcHooks();                         
    void              CalcHydros();                        
    void              CalcMouse();                         
    void              CalcNodes();
    void              CalcEventBoxes();
    void              CalcReplay();                        
    void              CalcRopes();                         
    void              CalcShocks(bool doUpdate, int num_steps); 
    void              CalcShocks2(int i, Ogre::Real difftoBeamL, Ogre::Real &k, Ogre::Real &d, Ogre::Real v);
    void              CalcShocks3(int i, Ogre::Real difftoBeamL, Ogre::Real &k, Ogre::Real &d, Ogre::Real v);
    void              CalcTriggers(int i, Ogre::Real difftoBeamL, bool update_hooks);
    void              CalcTies();                          
    void              CalcTruckEngine(bool doUpdate);      
    void              CalcWheels(bool doUpdate, int num_steps); 

    void              DetermineLinkedActors();
    void              calcNodeConnectivityGraph();
    void              AddInterActorBeam(beam_t* beam, ActorPtr other, ActorLinkingRequestType type);  //!< Do not call directly - use `MSG_SIM_ACTOR_LINKING_REQUESTED`
    void              RemoveInterActorBeam(beam_t* beam, ActorLinkingRequestType type);  //!< Do not call directly - use `MSG_SIM_ACTOR_LINKING_REQUESTED`
    void              DisjoinInterActorBeams();            //!< Helper for `MSG_` handlers, do not invoke by hand.
    void              autoBlinkReset();                    //!< Resets the turn signal when the steering wheel is turned back.
    void              ResetAngle(float rot);
    void              calculateLocalGForces();             //!< Derive the truck local g-forces from the global ones
    /// Virtually moves the actor at most 'direction.length()' meters towards 'direction' trying to resolve any collisions
    /// Returns a minimal offset by which the actor needs to be moved to resolve any collisions
    //  Both PointColDetectors need to be updated accordingly before calling this
    Ogre::Vector3     calculateCollisionOffset(Ogre::Vector3 direction);
    /// @param actor which actor to retrieve the closest Rail from
    /// @param node which SlideNode is being checked against
    /// @return a pair containing the rail, and the distant to the SlideNode
    std::pair<RailGroup*, Ogre::Real> GetClosestRailOnActor( ActorPtr actor, const SlideNode& node);

    // -------------------- data -------------------- //

    std::vector<std::shared_ptr<Task>> m_flexbody_tasks;   //!< Gfx state
    RigDef::DocumentPtr                m_definition;
    std::unique_ptr<GfxActor>          m_gfx_actor;
    Ogre::String                       m_section_config;
    std::vector<SlideNode>             m_slidenodes;       //!< all the SlideNodes available on this actor
    std::vector<RailGroup*>            m_railgroups;       //!< all the available RailGroups for this actor
    std::vector<Ogre::Entity*>         m_deletion_entities;    //!< For unloading vehicle; filled at spawn.
    std::vector<Ogre::SceneNode*>      m_deletion_scene_nodes; //!< For unloading vehicle; filled at spawn.
    int               m_proped_wheel_pairs[MAX_WHEELS] = {};    //!< Physics attr; For inter-differential locking
    int               m_num_proped_wheels = 0;          //!< Physics attr, filled at spawn - Number of propelled wheels.
    float             m_avg_proped_wheel_radius = 0.f;    //!< Physics attr, filled at spawn - Average proped wheel radius.
    float             m_avionic_chatter_timer = 11.f;      //!< Sound fx state (some pseudo random number,  doesn't matter)
    PointColDetector* m_inter_point_col_detector = nullptr;   //!< Physics
    PointColDetector* m_intra_point_col_detector = nullptr;   //!< Physics
    
    Ogre::Vector3     m_avg_node_position = Ogre::Vector3::ZERO;          //!< average node position
    Ogre::Real        m_min_camera_radius = 0.f;
    Ogre::Vector3     m_avg_node_position_prev = Ogre::Vector3::ZERO;
    Ogre::Vector3     m_avg_node_velocity = Ogre::Vector3::ZERO;          //!< average node velocity (compared to the previous frame step)
    float             m_stabilizer_shock_sleep = 0.f;     //!< Sim state
    Replay*           m_replay_handler = nullptr;
    NodeNum_t         m_mouse_grab_node = NODENUM_INVALID;  //!< Sim state; node currently being dragged by user
    Ogre::Vector3     m_mouse_grab_pos = Ogre::Vector3::ZERO;
    float             m_mouse_grab_move_force = 0.f;
    float             m_spawn_rotation = 0.f;
    Ogre::Timer       m_reset_timer;
    Ogre::Vector3     m_camera_gforces_accu = Ogre::Vector3::ZERO;      //!< Accumulator for 'camera' G-forces
    Ogre::Vector3     m_camera_gforces = Ogre::Vector3::ZERO;           //!< Physics state (global)
    Ogre::Vector3     m_camera_local_gforces_cur = Ogre::Vector3::ZERO; //!< Physics state (camera local)
    Ogre::Vector3     m_camera_local_gforces_max = Ogre::Vector3::ZERO; //!< Physics state (camera local)
    float             m_stabilizer_shock_ratio = 0.f;   //!< Physics state
    int               m_stabilizer_shock_request = 0; //!< Physics state; values: { -1, 0, 1 }
    Differential*     m_axle_diffs[1+MAX_WHEELS/2] = {};//!< Physics
    int               m_num_axle_diffs = 0;           //!< Physics attr
    Differential*     m_wheel_diffs[MAX_WHEELS/2] = {};//!< Physics
    int               m_num_wheel_diffs = 0;          //!< Physics attr
    TransferCase*     m_transfer_case = nullptr;            //!< Physics
    int               m_wheel_node_count = 0;      //!< Static attr; filled at spawn
    int               m_previous_gear = 0;         //!< Sim state; land vehicle shifting
    float             m_handbrake_force = 0.f;       //!< Physics attr; defined in truckfile
    Airfoil*          m_fusealge_airfoil = nullptr;      //!< Physics attr; defined in truckfile
    node_t*           m_fusealge_front = nullptr;        //!< Physics attr; defined in truckfile
    node_t*           m_fusealge_back = nullptr;         //!< Physics attr; defined in truckfile
    float             m_fusealge_width = 0.f;        //!< Physics attr; defined in truckfile
    float             m_odometer_total = 0.f;        //!< GUI state
    float             m_odometer_user = 0.f;         //!< GUI state
    int               m_num_command_beams = 0;     //!< TODO: Remove! Spawner context only; likely unused feature
    std::unique_ptr<Buoyance> m_buoyance;              //!< Physics
    CacheEntryPtr     m_used_actor_entry;
    CacheEntryPtr     m_used_skin_entry;               //!< Graphics
    TuneupDefPtr      m_working_tuneup_def;            //!< Each actor gets unique instance, even if loaded from .tuneup file in modcache.
    Skidmark*         m_skid_trails[MAX_WHEELS*2] = {};
    bool              m_antilockbrake = false;         //!< GUI state
    bool              m_tractioncontrol = false;       //!< GUI state
    bool              m_has_axles_section = false;     //!< Temporary (legacy parsing helper) until central diffs are implemented
    TyrePressure      m_tyre_pressure;
    std::vector<std::string>  m_description;
    std::vector<PropAnimKeyState> m_prop_anim_key_states;

    /// @name Networking
    /// @{
    size_t            m_net_node_buf_size = 0;        //!< For incoming/outgoing traffic; calculated on spawn
    size_t            m_net_wheel_buf_size = 0;       //!< For incoming/outgoing traffic; calculated on spawn
    size_t            m_net_propanimkey_buf_size = 0; //!< For incoming/outgoing traffic; calculated on spawn
    size_t            m_net_total_buffer_size = 0;    //!< For incoming/outgoing traffic; calculated on spawn
    float             m_net_node_compression = 0.f;     //!< For incoming/outgoing traffic; calculated on spawn
    int               m_net_first_wheel_node = 0;     //!< Network attr; Determines data buffer layout; calculated on spawn

    std::string       m_net_username;
    int               m_net_color_num = 0;
    /// @}

    /// @name Light states
    /// @{
    GfxFlaresMode     m_flares_mode = GfxFlaresMode::NONE;       //!< Snapshot of cvar 'gfx_flares_mode' on spawn.
    BitMask_t         m_lightmask = 0;                           //!< RoRnet::Lightmask
    BitMask_t         m_flaregroups_no_import = 0;               //!< RoRnet::Lightmask
    bool              m_blinker_autoreset = false;               //!< When true, we're steering and blinker will turn off automatically.
    bool              m_blinker_left_lit = false;                //!< Blinking state of left turn signal
    bool              m_blinker_right_lit = false;               //!< Blinking state of right turn signal
    /// @}

    bool m_hud_features_ok:1;      //!< Gfx state; Are HUD features matching actor's capabilities?
    bool m_slidenodes_locked:1;    //!< Physics state; Are SlideNodes locked?
    bool m_net_initialized:1;
    bool m_water_contact:1;        //!< Scripting state
    bool m_water_contact_old:1;    //!< Scripting state
    bool m_has_command_beams:1;    //!< Physics attr;
    bool m_preloaded_with_terrain:1;        //!< Spawn context (TODO: remove!)
    bool m_beam_break_debug_enabled:1;  //!< Logging state
    bool m_beam_deform_debug_enabled:1; //!< Logging state
    bool m_trigger_debug_enabled:1;     //!< Logging state
    bool m_disable_default_sounds:1;    //!< Spawner context; TODO: remove
    bool m_disable_smoke:1;             //!< Stops/starts smoke particles (i.e. exhausts, turbojets).

    struct VehicleForceSensors
    {
        inline void Reset()
        {
            accu_body_forces    = Ogre::Vector3::ZERO;
            accu_hydros_forces  = 0;
            out_body_forces     = Ogre::Vector3::ZERO;
            out_hydros_forces   = 0;
        };

        Ogre::Vector3 accu_body_forces;
        float         accu_hydros_forces;
        Ogre::Vector3 out_body_forces;
        float         out_hydros_forces;
    } m_force_sensors; //!< Data for ForceFeedback devices

    struct NetUpdate
    {
        std::vector<char> veh_state;   //!< Actor properties (engine, brakes, lights, ...)
        std::vector<char> node_data;   //!< Compressed node positions
        std::vector<float> wheel_data; //!< Wheel rotations
    };

    std::deque<NetUpdate> m_net_updates; //!< Incoming stream of NetUpdates
};

/// @} // addtogroup Physics

} // namespace RoR
