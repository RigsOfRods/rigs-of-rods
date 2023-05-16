/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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
/// @author Thomas Fischer
/// @date   30th of April 2010
/// @brief Core data structures for simulation; Everything affected by by either physics, network or user interaction is here.
///
/// Note that simulation state and gfx state is separated; For example,
/// light states (on/off) are here while the actual lights (renderer objects) are in 'GfxData.h' :)

#pragma once

#include "ForwardDeclarations.h"
#include "SimConstants.h"
#include "BitFlags.h"
#include "CmdKeyInertia.h"
#include "InputEngine.h"

#include <memory>
#include <Ogre.h>
#include <OgreUTFString.h>
#include <rapidjson/document.h>

namespace RoR {

enum CollisionEventFilter: short
{
    EVENT_NONE = 0,
    EVENT_ALL,
    EVENT_AVATAR,
    EVENT_TRUCK,
    EVENT_AIRPLANE,
    EVENT_BOAT,
    EVENT_DELETE
};

enum class ExtCameraMode
{
    INVALID = -1,
    CLASSIC = 0,
    CINECAM = 1,
    NODE    = 2,
};

/// @addtogroup Gameplay
/// @{

enum HookAction
{
    HOOK_LOCK=0,
    HOOK_UNLOCK,
    HOOK_TOGGLE,
    MOUSE_HOOK_TOGGLE,
};

/// @}

/// @addtogroup Physics
/// @{

enum BeamType: short
{
    BEAM_NORMAL,
    BEAM_HYDRO,
    BEAM_VIRTUAL,         //!< Excluded from mass calculations, visuals permanently disabled
};

/// @} // addtogroup Physics

/// @addtogroup Gameplay
/// @{

enum HookState
{
    UNLOCKED,       //!< lock not locked
    PREUNLOCK,      //!< preunlocking, inter-actor beam deletion in progress
    PRELOCK,        //!< prelocking, attraction forces in action
    LOCKED          //!< lock locked.
};

enum ActorType //!< Aka 'Driveable'
{
    // DO NOT MODIFY NUMBERS - serialized into cache file, see RoR::CacheEntry

    NOT_DRIVEABLE  = 0,   //!< not drivable at all
    TRUCK          = 1,   //!< its a truck (or other land vehicle)
    AIRPLANE       = 2,   //!< its an airplane
    BOAT           = 3,   //!< its a boat
    MACHINE        = 4,   //!< its a machine
    AI             = 5,   //!< machine controlled by an Artificial Intelligence
};

/// @}

/// @addtogroup Physics
/// @{

enum SpecialBeam: short
{
    NOSHOCK,        //!< not a shock
    SHOCK1,         //!< shock1
    SHOCK2,         //!< shock2
    SHOCK3,         //!< shock3
    TRIGGER,        //!< trigger
    SUPPORTBEAM,    //!<
    ROPE            //!<
};

/// @} // addtogroup Physics

/// @addtogroup Gameplay
/// @{

enum BlinkType //!< Turn signal
{
    BLINK_NONE,     //!<
    BLINK_LEFT,     //!<
    BLINK_RIGHT,    //!<
    BLINK_WARN      //!<
};

enum HydroFlags
{
    HYDRO_FLAG_SPEED        = BITMASK(1),
    HYDRO_FLAG_DIR          = BITMASK(2),
    HYDRO_FLAG_AILERON      = BITMASK(3),
    HYDRO_FLAG_RUDDER       = BITMASK(4),
    HYDRO_FLAG_ELEVATOR     = BITMASK(5),
    HYDRO_FLAG_REV_AILERON  = BITMASK(6),
    HYDRO_FLAG_REV_RUDDER   = BITMASK(7),
    HYDRO_FLAG_REV_ELEVATOR = BITMASK(8),
};

enum AnimFlags
{
    ANIM_FLAG_AIRSPEED      = BITMASK(1),
    ANIM_FLAG_VVI           = BITMASK(2),
    ANIM_FLAG_ALTIMETER     = BITMASK(3),
    ANIM_FLAG_AOA           = BITMASK(4),
    ANIM_FLAG_FLAP          = BITMASK(5),
    ANIM_FLAG_AIRBRAKE      = BITMASK(6),
    ANIM_FLAG_ROLL          = BITMASK(7),
    ANIM_FLAG_PITCH         = BITMASK(8),
    ANIM_FLAG_THROTTLE      = BITMASK(9),
    ANIM_FLAG_RPM           = BITMASK(10),
    ANIM_FLAG_ACCEL         = BITMASK(11),
    ANIM_FLAG_BRAKE         = BITMASK(12),
    ANIM_FLAG_CLUTCH        = BITMASK(13),
    ANIM_FLAG_TACHO         = BITMASK(14),
    ANIM_FLAG_SPEEDO        = BITMASK(15),
    ANIM_FLAG_PBRAKE        = BITMASK(16),
    ANIM_FLAG_TURBO         = BITMASK(17),
    ANIM_FLAG_SHIFTER       = BITMASK(18),
    ANIM_FLAG_AETORQUE      = BITMASK(19),
    ANIM_FLAG_AEPITCH       = BITMASK(20),
    ANIM_FLAG_AESTATUS      = BITMASK(21),
    ANIM_FLAG_TORQUE        = BITMASK(22),
    ANIM_FLAG_HEADING       = BITMASK(23),
    ANIM_FLAG_DIFFLOCK      = BITMASK(24),
    ANIM_FLAG_STEERING      = BITMASK(25),
    ANIM_FLAG_EVENT         = BITMASK(26),
    ANIM_FLAG_AILERONS      = BITMASK(27),
    ANIM_FLAG_ARUDDER       = BITMASK(28),
    ANIM_FLAG_BRUDDER       = BITMASK(29),
    ANIM_FLAG_BTHROTTLE     = BITMASK(30),
    ANIM_FLAG_PERMANENT     = BITMASK(31),
    ANIM_FLAG_ELEVATORS     = BITMASK(32),
};

enum AnimModes
{
    ANIM_MODE_ROTA_X        = BITMASK(1),
    ANIM_MODE_ROTA_Y        = BITMASK(2),
    ANIM_MODE_ROTA_Z        = BITMASK(3),
    ANIM_MODE_OFFSET_X      = BITMASK(4),
    ANIM_MODE_OFFSET_Y      = BITMASK(5),
    ANIM_MODE_OFFSET_Z      = BITMASK(6),
    ANIM_MODE_AUTOANIMATE   = BITMASK(7),
    ANIM_MODE_NOFLIP        = BITMASK(8),
    ANIM_MODE_BOUNCE        = BITMASK(9),
};

/// @}

/// @addtogroup Physics
/// @{

enum ShockFlags
{
    SHOCK_FLAG_NORMAL           = BITMASK(1),
    SHOCK_FLAG_LACTIVE          = BITMASK(3),
    SHOCK_FLAG_RACTIVE          = BITMASK(4),
    SHOCK_FLAG_ISSHOCK2         = BITMASK(5),
    SHOCK_FLAG_ISSHOCK3         = BITMASK(6),
    SHOCK_FLAG_SOFTBUMP         = BITMASK(7),
    SHOCK_FLAG_ISTRIGGER        = BITMASK(8),
    SHOCK_FLAG_TRG_BLOCKER      = BITMASK(9),
    SHOCK_FLAG_TRG_CMD_SWITCH   = BITMASK(10),
    SHOCK_FLAG_TRG_CMD_BLOCKER  = BITMASK(11),
    SHOCK_FLAG_TRG_BLOCKER_A    = BITMASK(12),
    SHOCK_FLAG_TRG_HOOK_UNLOCK  = BITMASK(13),
    SHOCK_FLAG_TRG_HOOK_LOCK    = BITMASK(14),
    SHOCK_FLAG_TRG_CONTINUOUS   = BITMASK(15),
    SHOCK_FLAG_TRG_ENGINE       = BITMASK(16)
};

/// @} // addtogroup Physics

/// @addtogroup Gameplay
/// @{

enum EngineTriggerType
{
    TRG_ENGINE_CLUTCH    = 0,
    TRG_ENGINE_BRAKE     = 1,
    TRG_ENGINE_ACC       = 2,
    TRG_ENGINE_RPM       = 3,
    TRG_ENGINE_SHIFTUP   = 4,
    TRG_ENGINE_SHIFTDOWN = 5 
};

enum class FlareType: char
{
    NONE           = 0,
    // Front lights
    HEADLIGHT      = 'f',
    HIGH_BEAM      = 'h',
    FOG_LIGHT      = 'g',
    // Rear lighs
    TAIL_LIGHT     = 't',
    BRAKE_LIGHT    = 'b',
    REVERSE_LIGHT  = 'R',
    // Special lights
    SIDELIGHT      = 's',
    BLINKER_LEFT   = 'l',
    BLINKER_RIGHT  = 'r',
    USER           = 'u',
    DASHBOARD      = 'd'
};

/// @addtogroup Aerial
/// @{

enum LocalizerType
{
    LOCALIZER_VERTICAL,
    LOCALIZER_HORIZONTAL,
    LOCALIZER_NDB,
    LOCALIZER_VOR
};

/// @}
/// @}

/// @addtogroup Physics
/// @{

enum class ActorState
{
    LOCAL_SIMULATED,  //!< simulated (local) actor
    NETWORKED_OK,     //!< not simulated (remote) actor
    NETWORKED_HIDDEN, //!< not simulated, not updated (remote)
    LOCAL_REPLAY,
    LOCAL_SLEEPING,   //!< sleeping (local) actor
    DISPOSED          //!< removed from simulation, still in memory to satisfy pointers.
};

enum class AeroEngineType
{
    AE_UNKNOWN,
    AE_XPROP,
    AE_TURBOJET,
};

// --------------------------------
// Soft body physics

/// Physics: A vertex in the softbody structure
struct node_t
{
    node_t(NodeNum_t n)
        : pos(n)
        , nd_cab_node(false)
        , nd_rim_node(false)
        , nd_tyre_node(false)
        , nd_contacter(false)
        , nd_contactable(false)
        , nd_has_ground_contact(false)
        , nd_has_mesh_contact(false)
        , nd_immovable(false)
        , nd_loaded_mass(false)
        , nd_no_ground_contact(false)
        , nd_override_mass(false)
        , nd_under_water(false)
        , nd_no_mouse_grab(false)
    {}

    Ogre::Vector3   RelPosition = Ogre::Vector3::ZERO; //!< relative to the local physics origin (one origin per actor) (shaky)
    Ogre::Vector3   AbsPosition = Ogre::Vector3::ZERO; //!< absolute position in the world (shaky)
    Ogre::Vector3   Velocity = Ogre::Vector3::ZERO;
    Ogre::Vector3   Forces = Ogre::Vector3::ZERO;

    float           mass = 0.f;
    float           buoyancy = 0.f;
    float           friction_coef = 0.f;
    float           surface_coef = 0.f;
    float           volume_coef = 0.f;

    NodeNum_t       pos = NODENUM_INVALID;             //!< This node's index in Actor::ar_nodes array.
    BboxID_t        nd_coll_bbox_id = BBOXID_INVALID;  //!< Optional attribute (-1 = none) - multiple collision bounding boxes defined in truckfile
    int16_t         nd_lockgroup = NODE_LOCKGROUP_DEFAULT; //!< Optional attribute (-1 = default, 9999 = deny lock) - used in the hook lock logic

    // Bit flags
    bool            nd_cab_node:1;           //!< Attr; This node is part of collision triangle
    bool            nd_rim_node:1;           //!< Attr; This node is part of a rim
    bool            nd_tyre_node:1;          //!< Attr; This node is part of a tyre
    bool            nd_contacter:1;          //!< Attr; User-defined
    bool            nd_contactable:1;        //!< Attr; This node will be treated as contacter on inter truck collisions
    bool            nd_has_ground_contact:1; //!< Physics state
    bool            nd_has_mesh_contact:1;   //!< Physics state
    bool            nd_immovable:1;          //!< Attr; User-defined
    bool            nd_loaded_mass:1;        //!< User defined attr; mass is calculated from 'globals/loaded-mass' rather than 'globals/dry-mass'
    bool            nd_no_ground_contact:1;  //!< User-defined attr; node ignores contact with ground
    bool            nd_override_mass:1;      //!< User defined attr; mass is user-specified rather than calculated (override the calculation)
    bool            nd_under_water:1;        //!< State; GFX hint
    bool            nd_no_mouse_grab:1;      //!< Attr; User-defined

    float           nd_avg_collision_slip = 0.f;                   //!< Physics state; average slip velocity across the last few physics frames
    Ogre::Vector3   nd_last_collision_slip = Ogre::Vector3::ZERO;  //!< Physics state; last collision slip vector
    Ogre::Vector3   nd_last_collision_force = Ogre::Vector3::ZERO; //!< Physics state; last collision force
    ground_model_t* nd_last_collision_gm = nullptr;                //!< Physics state; last collision 'ground model' (surface definition)
};

/// Simulation: An edge in the softbody structure
struct beam_t
{
    beam_t(BeamID_t id) : bm_pos(id) {}

    NodeNum_t       p1num = NODENUM_INVALID;
    NodeNum_t       p2num = NODENUM_INVALID;
    float           k = 0.f;                     //!< tensile spring
    float           d = 0.f;                     //!< damping factor
    float           L = 0.f;                     //!< length
    float           minmaxposnegstress = 0.f;
    float           maxposstress = 0.f;
    float           maxnegstress = 0.f;
    float           strength = 0.f;
    float           stress = 0.f;
    float           plastic_coef = 0.f;
    int             detacher_group;              //!< Attribute: detacher group number (integer)
    SpecialBeam     bounded = NOSHOCK;
    BeamType        bm_type = BEAM_NORMAL;
    bool            bm_inter_actor = false;      //!< in case p2 is on another actor
    ActorPtr        bm_locked_actor;             //!< in case p2 is on another actor
    bool            bm_disabled = false;
    bool            bm_broken = false;
    BeamID_t        bm_pos = BEAMID_INVALID;

    float           shortbound = 0.f;
    float           longbound = 0.f;
    float           refL = 0.f;                  //!< reference length

    shock_t*        shock = nullptr;

    float           initial_beam_strength = 0.f; //!< for reset
    float           default_beam_deform = 0.f;   //!< for reset

    float           debug_k = 0.f;               //!< debug shock spring_rate
    float           debug_d = 0.f;               //!< debug shock damping
    float           debug_v = 0.f;               //!< debug shock velocity
};

struct shock_t
{
    shock_t() { memset(this, 0, sizeof(shock_t)); }

    int beamid;
    int flags;

    bool trigger_enabled;       //!< general trigger,switch and blocker state
    float trigger_switch_state; //!< needed to avoid doubleswitch, bool and timer in one
    float trigger_boundary_t;   //!< optional value to tune trigger_switch_state autorelease
    int trigger_cmdlong;        //!< F-key for trigger injection longbound-check
    int trigger_cmdshort;       //!< F-key for trigger injection shortbound-check
    int last_debug_state;       //!< smart debug output

    float springin;  //!< shocks2 & shocks3
    float dampin;    //!< shocks2 & shocks3
    float springout; //!< shocks2 & shocks3
    float dampout;   //!< shocks2 & shocks3

    float sprogin;   //!< shocks2
    float dprogin;   //!< shocks2
    float sprogout;  //!< shocks2
    float dprogout;  //!< shocks2

    float splitin;   //!< shocks3
    float dslowin;   //!< shocks3
    float dfastin;   //!< shocks3
    float splitout;  //!< shocks3
    float dslowout;  //!< shocks3
    float dfastout;  //!< shocks3

    float sbd_spring;           //!< set beam default for spring
    float sbd_damp;             //!< set beam default for damping
};

struct collcab_rate_t
{
    int rate;     // remaining amount of physics cycles to be skipped
    int distance; // distance (in physics cycles) to the previous collision check
};

/// @} // addtogroup Physics

struct soundsource_t
{
    static const int TYPE_ALWAYS = -2;
    static const int TYPE_EXTERIOR = -1;

    soundsource_t(SoundScriptInstancePtr& s, NodeNum_t n, int t) : ssi(s), nodenum(n), type(t) {}

    SoundScriptInstancePtr ssi;
    NodeNum_t nodenum = NODENUM_INVALID;
    int type = TYPE_ALWAYS; //!< When active? Can be cinecam index or a `TYPE_*` constant
};

struct wheel_t
{
    enum class BrakeCombo /// Wheels are braked by three mechanisms: A footbrake, a handbrake/parkingbrake, and directional brakes used for skidsteer steering.
    {
        NONE,                 //!< - 0 = no  footbrake, no  handbrake, no  direction control -- wheel is unbraked
        FOOT_HAND,            //!< - 1 = yes footbrake, yes handbrake, no  direction control
        FOOT_HAND_SKID_LEFT,  //!< - 2 = yes footbrake, yes handbrake, yes direction control (braked when vehicle steers to the left)
        FOOT_HAND_SKID_RIGHT, //!< - 3 = yes footbrake, yes handbrake, yes direction control (braked when vehicle steers to the right)
        FOOT_ONLY             //!< - 4 = yes footbrake, no  handbrake, no  direction control -- footbrake only, such as with the front wheels of a passenger car
    };
    
    NodeNum_t     wh_arm_nodenum         = NODENUM_INVALID;
    NodeNum_t     wh_near_attach_nodenum = NODENUM_INVALID;
    NodeNum_t     wh_axis_node0num       = NODENUM_INVALID;
    NodeNum_t     wh_axis_node1num       = NODENUM_INVALID;

    std::vector<NodeNum_t> wh_tire_nodes;
    std::vector<NodeNum_t> wh_rim_nodes;

    BrakeCombo  wh_braking;
    int         wh_propulsed;             // TODO: add enum ~ only_a_ptr, 08/2017
    Ogre::Real  wh_radius;
    Ogre::Real  wh_rim_radius;
    Ogre::Real  wh_speed;             //!< Current wheel speed in m/s
    Ogre::Real  wh_avg_speed;         //!< Internal physics state; Do not read from this
    Ogre::Real  wh_alb_coef;          //!< Sim state; Current anti-lock  brake modulation ratio
    Ogre::Real  wh_tc_coef;           //!< Sim state; Current traction control modulation ratio
    Ogre::Real  wh_mass;              //!< Total rotational mass of the wheel
    Ogre::Real  wh_torque;            //!< Internal physics state; Do not read from this
    Ogre::Real  wh_last_torque;       //!< Last internal forces (engine / brakes / diffs)
    Ogre::Real  wh_last_retorque;     //!< Last external forces (friction, ...)
    float       wh_net_rp;
    float       wh_width;
    bool        wh_is_detached;

    // Debug
    float debug_rpm;
    float debug_torque;
    Ogre::Vector3 debug_vel;
    Ogre::Vector3 debug_slip;
    Ogre::Vector3 debug_force;
    Ogre::Vector3 debug_scaled_cforce;
};

struct wheeldetacher_t
{
    int wd_wheel_id;
    int wd_detacher_group;
};

/// @addtogroup Gameplay
/// @{

struct hook_t
{
    HookState hk_locked;
    int       hk_group;
    int       hk_lockgroup;
    bool      hk_selflock;
    bool      hk_autolock;
    bool      hk_nodisable;
    float     hk_maxforce;
    float     hk_lockrange;
    float     hk_lockspeed;
    float     hk_timer;
    float     hk_timer_preset;
    float     hk_min_length;                    //!< Absolute value in meters
    NodeNum_t hk_hook_node = NODENUM_INVALID;   //!< The hooking node on master actor, never invalid.
    BeamID_t  hk_beam = BEAMID_INVALID;         //!< The hooking beam on master actor.
    ActorPtr  hk_locked_actor;                  //!< The slave actor, or null if not hooked.
    NodeNum_t hk_locked_node = NODENUM_INVALID; //!< The locked node on slave actor, or NODENUM_INVALID if not hooked.
};

struct ropable_t
{
    NodeNum_t    rb_nodenum = NODENUM_INVALID;
    RopableID_t  rb_pos = ROPABLEID_INVALID;    //!< Index into ar_ropables
    int          group;
    int          attached_ties;                 //!< State
    int          attached_ropes;                //!< State
    bool         multilock;                     //!< Attribute
};

struct rope_t
{
    HookState   rp_locked = UNLOCKED;
    int         rp_group;
    BeamID_t    rp_beam = BEAMID_INVALID;
    RopableID_t rp_locked_ropable_id = ROPABLEID_INVALID;
    ActorPtr    rp_locked_actor;
};

struct tie_t
{
    ActorPtr    ti_locked_actor;
    BeamID_t    ti_beamid = BEAMID_INVALID;
    RopableID_t ti_locked_ropable_id = ROPABLEID_INVALID;
    int         ti_group;
    float       ti_contract_speed;
    float       ti_max_stress;
    float       ti_min_length;       //!< Proportional to orig; length

    bool       ti_no_self_lock:1;   //!< Attribute
    bool       ti_tied:1;           //!< State
    bool       ti_tying:1;          //!< State
};

struct wing_t
{
    FlexAirfoil *fa;
    Ogre::SceneNode *cnode;
};

struct commandbeam_state_t
{
    commandbeam_state_t() { memset(this, 0, sizeof(commandbeam_state_t)); }

    int8_t   auto_moving_mode;      //!< State

    // Bit flags
    bool     pressed_center_mode:1; //!< State
    bool     auto_move_lock:1;      //!< State
};

struct commandbeam_t
{
    uint16_t cmb_beam_index;            //!< Index to Actor::ar_beams array
    float    cmb_engine_coupling;       //!< Attr from truckfile
    float    cmb_center_length;         //!< Attr computed at spawn
    float    cmb_speed;                 //!< Attr; Rate of contraction/extension
    float    cmb_boundary_length;       //!< Attr; Maximum/minimum length proportional to orig. len.

    // Bit flags
    bool     cmb_is_contraction:1;      //!< Attribute defined at spawn
    bool     cmb_is_force_restricted:1; //!< Attribute defined in truckfile
    bool     cmb_needs_engine:1;        //!< Attribute defined in truckfile
    bool     cmb_is_autocentering:1;    //!< Attribute defined in truckfile
    bool     cmb_plays_sound:1;         //!< Attribute defined in truckfile
    bool     cmb_is_1press:1;           //!< Attribute defined in truckfile
    bool     cmb_is_1press_center:1;    //!< Attribute defined in truckfile

    std::shared_ptr<commandbeam_state_t> cmb_state;
};

struct command_t
{
    int commandValueState;
    float commandValue;
    float triggerInputValue;
    float playerInputValue;
    bool trigger_cmdkeyblock_state;  //!< identifies blocked F-commands for triggers
    std::vector<commandbeam_t> beams;
    std::vector<int> rotators;
    Ogre::String description;
    RoR::CmdKeyInertia rotator_inertia;
    RoR::CmdKeyInertia command_inertia;
};

struct hydrobeam_t
{
    uint16_t hb_beam_index; //!< Index to Actor::ar_beams array
    float    hb_ref_length; //!< Idle length in meters
    float    hb_speed;      //!< Rate of change
    int      hb_flags;
    int      hb_anim_flags; //!< Animators (beams updating length based on simulation variables)
    float    hb_anim_param; //!< Animators (beams updating length based on simulation variables)
    RoR::CmdKeyInertia  hb_inertia;
};

struct rotator_t
{
    bool needs_engine;
    NodeNum_t nodes1[4];
    NodeNum_t nodes2[4];
    NodeNum_t axis1; //!< rot axis
    NodeNum_t axis2;
    float angle;
    float rate;
    float force;
    float tolerance;
    float engine_coupling;
    float debug_rate;
    float debug_aerror;
};

struct flare_t
{
    NodeNum_t noderef = NODENUM_INVALID;
    NodeNum_t nodex   = NODENUM_INVALID;
    NodeNum_t nodey   = NODENUM_INVALID;
    float offsetx;
    float offsety;
    float offsetz;
    Ogre::SceneNode *snode;
    Ogre::BillboardSet *bbs;
    Ogre::Light *light;
    FlareType fl_type;
    int controlnumber; //!< Only 'u' type flares, valid values 0-9, maps to EV_TRUCK_LIGHTTOGGLE01 to 10.
    int dashboard_link; //!< Only 'd' type flares, valid values are DD_*
    float blinkdelay;
    float blinkdelay_curr;
    bool blinkdelay_state;
    float size;
    float intensity;
    bool uses_inertia = false; //!< Only 'flares3'
    SimpleInertia inertia; //!< Only 'flares3'
};

struct exhaust_t
{
    NodeNum_t emitterNode   = NODENUM_INVALID;
    NodeNum_t directionNode = NODENUM_INVALID;
    Ogre::SceneNode *smokeNode;
    Ogre::ParticleSystem* smoker;
};


struct cparticle_t
{
    NodeNum_t emitterNode   = NODENUM_INVALID;
    NodeNum_t directionNode = NODENUM_INVALID;
    bool active;
    Ogre::SceneNode *snode;
    Ogre::ParticleSystem* psys;
};

/// User input state for animated props with 'source:event'.
struct PropAnimKeyState
{
    bool eventlock_present = false;
    bool event_active_prev = false;
    bool anim_active = false;
    events event_id = EV_MODE_LAST; // invalid
};

/// @}

// --------------------------------
// some non-actor structs

/// @addtogroup Physics
/// @{

/// @addtogroup Collisions
/// @{


struct collision_box_t
{
    bool virt;
    bool refined;
    bool selfrotated;
    bool camforced;
    bool enabled;
    CollisionEventFilter event_filter;
    short eventsourcenum;
    Ogre::Vector3 lo;           //!< absolute collision box
    Ogre::Vector3 hi;           //!< absolute collision box
    Ogre::Vector3 center;       //!< center of rotation
    Ogre::Quaternion rot;       //!< rotation
    Ogre::Quaternion unrot;     //!< rotation
    Ogre::Vector3 selfcenter;   //!< center of self rotation
    Ogre::Quaternion selfrot;   //!< self rotation
    Ogre::Quaternion selfunrot; //!< self rotation
    Ogre::Vector3 relo;         //!< relative collision box
    Ogre::Vector3 rehi;         //!< relative collision box
    Ogre::Vector3 campos;       //!< camera position
    Ogre::Vector3 debug_verts[8];//!< box corners in absolute world position
};
typedef std::vector<collision_box_t*> CollisionBoxPtrVec;

/// Surface friction properties.
struct ground_model_t
{
    float va;                       //!< adhesion velocity
    float ms;                       //!< static friction coefficient
    float mc;                       //!< sliding friction coefficient
    float t2;                       //!< hydrodynamic friction (s/m)
    float vs;                       //!< stribeck velocity (m/s)
    float alpha;                    //!< steady-steady
    float strength;                 //!< ground strength

    float fluid_density;            //!< Density of liquid
    float flow_consistency_index;   //!< general drag coefficient

    //! if flow_behavior_index<1 then liquid is Pseudoplastic (ketchup, whipped cream, paint)
    //! if =1 then liquid is Newtoni'an fluid
    //! if >1 then liquid is Dilatant fluid (less common)
    float flow_behavior_index;

    
    float solid_ground_level;       //!< how deep the solid ground is
    float drag_anisotropy;          //!< Upwards/Downwards drag anisotropy

    int fx_type;
    Ogre::ColourValue fx_colour;
    char name[256];
    char basename[256];
    char particle_name[256];

    int fx_particle_amount;         //!< amount of particles

    float fx_particle_min_velo;     //!< minimum velocity to display sparks
    float fx_particle_max_velo;     //!< maximum velocity to display sparks
    float fx_particle_fade;         //!< fade coefficient
    float fx_particle_timedelta;    //!< delta for particle animation
    float fx_particle_velo_factor;  //!< velocity factor
    float fx_particle_ttl;
};

/// @} // addtogroup Collisions
/// @} // addtogroup Physics

struct authorinfo_t
{
    int id;
    Ogre::String type;
    Ogre::String name;
    Ogre::String email;
};

struct ActorSpawnRequest
{
    enum class Origin //!< Enables special processing
    {
        UNKNOWN,
        CONFIG_FILE,  //!< 'Preselected vehicle' in RoR.cfg or command line
        TERRN_DEF,    //!< Preloaded with terrain
        USER,         //!< Direct selection by user via GUI
        SAVEGAME,     //!< User spawned and part of a savegame
        NETWORK,      //!< Remote controlled
        AI            //!< Script controlled
    };

    CacheEntry*         asr_cache_entry = nullptr; //!< Optional, overrides 'asr_filename' and 'asr_cache_entry_num'
    std::string         asr_filename;
    Ogre::String        asr_config;
    Ogre::Vector3       asr_position = Ogre::Vector3::ZERO;
    Ogre::Quaternion    asr_rotation = Ogre::Quaternion::ZERO;
    collision_box_t*    asr_spawnbox = nullptr;
    CacheEntry*         asr_skin_entry = nullptr;
    Origin              asr_origin = Origin::UNKNOWN;
    int                 asr_debugview = 0; //(int)DebugViewType::DEBUGVIEW_NONE;
    Ogre::UTFString     asr_net_username;
    int                 asr_net_color = 0;
    int                 net_source_id = 0;
    int                 net_stream_id = 0;
    bool                asr_free_position = false;   //!< Disables the automatic spawn position adjustment
    bool                asr_enter = true;
    bool                asr_terrn_machine = false;   //!< This is a fixed machinery
    std::shared_ptr<rapidjson::Document>
                        asr_saved_state;             //!< Pushes msg MODIFY_ACTOR (type RESTORE_SAVED) after spawn.
};

struct ActorModifyRequest
{
    enum class Type
    {
        INVALID,
        RELOAD,               //!< Full reload from filesystem, requested by user
        RESET_ON_INIT_POS,
        RESET_ON_SPOT,
        SOFT_RESET,
        RESTORE_SAVED,
        WAKE_UP
    };

    ActorInstanceID_t   amr_actor = ACTORINSTANCEID_INVALID;// not ActorPtr because it's not thread-safe
    Type                amr_type;
    std::shared_ptr<rapidjson::Document>
                        amr_saved_state;
};

enum class ActorLinkingRequestType
{
    INVALID,
    HOOK_ACTION,
    TIE_ACTION,
    ROPE_ACTION,
    SLIDENODE_ACTION
};

/// Estabilishing a physics linkage between 2 actors modifies a global linkage table
/// and triggers immediate update of every actor's linkage tables,
/// so it has to be done sequentially on main thread.
struct ActorLinkingRequest
{
    ActorInstanceID_t alr_actor_instance_id = ACTORINSTANCEID_INVALID;
    ActorLinkingRequestType alr_type = ActorLinkingRequestType::INVALID;
    // hookToggle()
    int alr_hook_group = -1;
    HookAction alr_hook_action;
    NodeNum_t alr_hook_mousenode = NODENUM_INVALID;
    // tieToggle()
    int alr_tie_group = -1;
    // ropeToggle()
    int alr_rope_group = -1;
};

} // namespace RoR
