/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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

#include "Application.h"
#include "ForwardDeclarations.h"
#include "SimConstants.h"
#include "BitFlags.h"
#include "CmdKeyInertia.h"
#include "InputEngine.h"

#include <memory>
#include <Ogre.h>
#include <rapidjson/document.h>

namespace RoR {

/// Specified in terrain object (.ODEF) file, syntax: 'event <type> <filter>'
enum CollisionEventFilter: short
{
    EVENT_NONE = 0,          //!< Invalid value.
    EVENT_ALL,               //!< (default) ~ Triggered by any node on any vehicle
    EVENT_AVATAR,            //!< 'avatar' ~ Triggered by the character only
    EVENT_TRUCK,             //!< 'truck' ~ Triggered by any node of land vehicle (`ActorType::TRUCK`)
    EVENT_TRUCK_WHEELS,      //!< 'truck_wheels' ~ Triggered only by wheel nodes of land vehicle (`ActorType::TRUCK`)
    EVENT_AIRPLANE,          //!< 'airplane' ~ Triggered by any node of airplane (`ActorType::AIRPLANE`)
    EVENT_BOAT,              //!< 'boat' ~ Triggered by any node of boats (`ActorType::BOAT`)
};


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

enum SpecialBeam: short //!< aka 'bounded'
{
    NOSHOCK,        //!< not a shock
    SHOCK1,         //!< either 'shock1' (with flag `BEAM_HYDRO`) or a wheel beam
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
    static const int8_t    INVALID_BBOX = -1;

    node_t()               { memset(this, 0, sizeof(node_t)); nd_coll_bbox_id = INVALID_BBOX; }
    node_t(size_t _pos)    { memset(this, 0, sizeof(node_t)); nd_coll_bbox_id = INVALID_BBOX; pos = static_cast<short>(_pos); }

    Ogre::Vector3   RelPosition;             //!< relative to the local physics origin (one origin per actor) (shaky)
    Ogre::Vector3   AbsPosition;             //!< absolute position in the world (shaky)
    Ogre::Vector3   Velocity;
    Ogre::Vector3   Forces;

    Ogre::Real      mass;
    Ogre::Real      buoyancy;
    Ogre::Real      friction_coef;
    Ogre::Real      surface_coef;
    Ogre::Real      volume_coef;

    NodeNum_t       pos;                     //!< This node's index in Actor::ar_nodes array.
    int16_t         nd_coll_bbox_id;         //!< Optional attribute (-1 = none) - multiple collision bounding boxes defined in truckfile
    int16_t         nd_lockgroup;            //!< Optional attribute (-1 = default, 9999 = deny lock) - used in the hook lock logic

    // Bit flags
    bool            nd_cab_node:1;           //!< Attr; This node is part of collision triangle
    bool            nd_rim_node:1;           //!< Attr; This node is part of a rim (only wheel types with separate rim nodes)
    bool            nd_tyre_node:1;          //!< Attr; This node is part of a tyre (note some wheel types don't use rim nodes at all)
    bool            nd_contacter:1;          //!< Attr; User-defined
    bool            nd_contactable:1;        //!< Attr; This node will be treated as contacter on inter truck collisions
    bool            nd_has_ground_contact:1; //!< Physics state
    bool            nd_has_mesh_contact:1;   //!< Physics state
    bool            nd_immovable:1;          //!< Attr; User-defined
    bool            nd_loaded_mass:1;        //!< User-defined attr; mass is calculated from 'globals/loaded-mass' rather than 'globals/dry-mass' - set by etiher 'set_node_defaults' (loadweight >=0) or 'nodes' (option 'l')
    bool            nd_no_ground_contact:1;  //!< User-defined attr; node ignores contact with ground
    bool            nd_override_mass:1;      //!< User defined attr; mass is user-specified rather than calculated (override the calculation)
    bool            nd_under_water:1;        //!< State; GFX hint
    bool            nd_no_mouse_grab:1;      //!< Attr; User-defined
    bool            nd_cinecam_node:1;       //!< Attr; User-defined

    Ogre::Real      nd_avg_collision_slip;   //!< Physics state; average slip velocity across the last few physics frames
    Ogre::Vector3   nd_last_collision_slip;  //!< Physics state; last collision slip vector
    Ogre::Vector3   nd_last_collision_force; //!< Physics state; last collision force
    ground_model_t* nd_last_collision_gm;    //!< Physics state; last collision 'ground model' (surface definition)
};

/// Simulation: An edge in the softbody structure
struct beam_t
{
    beam_t();
    ~beam_t();

    node_t*         p1 = nullptr;
    node_t*         p2 = nullptr;
    float           k = 0.f;                     //!< tensile spring
    float           d = 0.f;                     //!< damping factor
    float           L = 0.f;                     //!< length
    float           minmaxposnegstress = 0.f;
    float           maxposstress = 0.f;
    float           maxnegstress = 0.f;
    float           strength = 0.f;
    float           stress = 0.f;
    float           plastic_coef = 0.f;
    int             detacher_group = DEFAULT_DETACHER_GROUP; //!< Attribute: detacher group number (integer)
    SpecialBeam     bounded = SpecialBeam::NOSHOCK;
    BeamType        bm_type = BeamType::BEAM_NORMAL;
    bool            bm_inter_actor = false;      //!< in case p2 is on another actor
    ActorPtr        bm_locked_actor;             //!< in case p2 is on another actor
    bool            bm_disabled = false;
    bool            bm_broken = false;

    float           shortbound = 0.f;
    float           longbound = 0.f;
    float           refL = 0.f;                  //!< reference length

    shock_t*        shock = nullptr;

    float           initial_beam_strength = 0.f; //!< for reset
    float           default_beam_deform = 0.f;   //!< for reset
    float           default_beam_diameter = 0.f; //!< for export only

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
    float sbd_break;            //!< set beam default for breaking threshold

    float shock_precompression; //!< Only for export
};

struct collcab_rate_t
{
    int rate;     // remaining amount of physics cycles to be skipped
    int distance; // distance (in physics cycles) to the previous collision check
};

/// @} // addtogroup Physics

struct soundsource_t
{
    soundsource_t();
    ~soundsource_t();

    SoundScriptInstancePtr ssi;
    NodeNum_t nodenum;
    int type;
};

struct wheel_t
{
    int         wh_num_nodes;
    node_t*     wh_nodes[50];             // TODO: remove limit, make this dyn-allocated ~ only_a_ptr, 08/2017
    int         wh_num_rim_nodes;
    node_t*     wh_rim_nodes[50];         // TODO: remove limit, make this dyn-allocated ~ only_a_ptr, 08/2017
    WheelBraking wh_braking;
    node_t*     wh_arm_node;
    node_t*     wh_near_attach_node;
    node_t*     wh_axis_node_0;
    node_t*     wh_axis_node_1;
    WheelPropulsion wh_propulsed;
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

    // Editing & Export (not used in simulation)
    RigDef::Keyword wh_arg_keyword;
    int             wh_arg_num_rays;
    NodeNum_t       wh_arg_rigidity_node;
    float           wh_arg_rim_spring;      //!< Not used by 'wheels' (1) and 'meshwheels' (1).
    float           wh_arg_rim_damping;     //!< Not used by 'wheels' (1) and 'meshwheels' (1).
    float           wh_arg_simple_spring;   //!< Whole wheel or just tire, depending on type.
    float           wh_arg_simple_damping;  //!< Whole wheel or just tire, depending on type.
    WheelSide       wh_arg_side;            //!< Only for 'meshwheels*' and 'flexbodywheels'
    std::string     wh_arg_media1;
    std::string     wh_arg_media2;
    int             wh_beam_start;          //!< BeamID to export 'set_beam_defaults' parameters from.

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
    hook_t();
    ~hook_t();

    HookState  hk_locked = HookState::UNLOCKED;
    int        hk_group = 0;
    int        hk_lockgroup = 0;
    bool       hk_selflock = false;
    bool       hk_autolock = false;
    bool       hk_nodisable = false;
    float      hk_maxforce = 0.f;
    float      hk_lockrange = 0.f;
    float      hk_lockspeed = 0.f;
    float      hk_timer = 0.f;
    float      hk_timer_preset = 0.f;
    float      hk_min_length = 0.f; //!< Absolute value in meters
    node_t*    hk_hook_node = nullptr;
    node_t*    hk_lock_node = nullptr;
    beam_t*    hk_beam = nullptr;
    ActorPtr   hk_locked_actor;
};

struct ropable_t
{
    node_t *node;
    int pos;            //!< Index into ar_ropables
    int group;
    int attached_ties;  //!< State
    int attached_ropes; //!< State
    bool multilock;     //!< Attribute
};

struct rope_t
{
    rope_t();
    ~rope_t();

    int        rp_locked = 0;
    int        rp_group = 0;
    beam_t*    rp_beam = nullptr;
    ropable_t* rp_locked_ropable = nullptr;
    ActorPtr   rp_locked_actor;
};

struct tie_t
{
    tie_t();
    ~tie_t();

    ActorPtr   ti_locked_actor;
    beam_t*    ti_beam = nullptr;
    ropable_t* ti_locked_ropable = nullptr;
    int        ti_group = 0;
    float      ti_contract_speed = 0.f;
    float      ti_max_stress = 0.f;
    float      ti_min_length = 0.f;       //!< Proportional to orig; length

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
    int commandValueState = 0;
    float commandValue = 0;
    float triggerInputValue = 0.f;
    float playerInputValue = 0.f;
    bool trigger_cmdkeyblock_state = false;  //!< identifies blocked F-commands for triggers
    std::vector<commandbeam_t> beams;
    std::vector<int> rotators;
    Ogre::String description;
    RoR::CmdKeyInertia rotator_inertia;
    RoR::CmdKeyInertia command_inertia;
};

struct hydrobeam_t //!< beams updating length based on simulation variables, generally known as actuators.
{
    uint16_t  hb_beam_index{0}; //!< Index to Actor::ar_beams array
    float     hb_ref_length{0}; //!< Idle length in meters
    float     hb_speed{0};      //!< Rate of change
    BitMask_t hb_flags{0};      //!< Only for 'hydros'
    BitMask_t hb_anim_flags{0}; //!< Only for 'animators' 
    float     hb_anim_param{0}; //!< Only for 'animators'
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
    Ogre::SceneNode *snode = nullptr;
    Ogre::BillboardSet *bbs = nullptr; //!< This remains `nullptr` if removed via `addonpart_unwanted_flare` or Tuning UI.
    Ogre::Light *light = nullptr;
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

/// User input state for animated props with 'source:event'.
struct PropAnimKeyState
{
    bool eventlock_present = false;
    bool event_active_prev = false;
    bool anim_active = false;
    events event_id = EV_MODE_LAST; // invalid
};

/// For backwards compatibility of the 'triggers' feature, the commandkey array must support negative indices of any size without breaking memory.
/// This class redirects the negative-indexed "virtual" commandkeys to auxiliary hashmap.
class CmdKeyArray
{
public:
    command_t& operator[](int index)
    {
        if (index >= 0 && index <= MAX_COMMANDS) // for backwards compatibility, we accept 0 as index too.
        {
            return m_commandkey[index]; // valid commandkey (indexed 1-MAX_COMMANDS!)
        }
        else if (index < 0)
        {
            return m_virtualkey[index]; // 'virtual' commandkey - if hashmap value doesn't exist, it's inserted automatically.
        }
        else
        {
            assert(false);
            return m_dummykey; // Whatever!
        }
    }
private:
    std::array<command_t, MAX_COMMANDS + 1> m_commandkey; //!< BEWARE: commandkeys are indexed 1-MAX_COMMANDS!
    std::unordered_map<int, command_t> m_virtualkey; //!< Negative-indexed commandkeys.
    command_t m_dummykey;
};

/// UI helper for displaying command control keys to user.
/// Reconstructing such list on runtime isn't possible, we must build it on spawn.
struct UniqueCommandKeyPair
{
    std::string uckp_description;
    CommandkeyID_t uckp_key1 = COMMANDKEYID_INVALID;
    CommandkeyID_t uckp_key2 = COMMANDKEYID_INVALID;
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
    std::string reverb_preset_name; //!< name of the reverb preset that applies to the inside of the collision box
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

enum class FreeForceType
{
    DUMMY,               //!< No force
    CONSTANT,            //!< Constant force given by direction and magnitude
    TOWARDS_COORDS,      //!< Constant force directed towards `ffc_target_coords`
    TOWARDS_NODE,        //!< Constant force directed towards `ffc_target_node`
    HALFBEAM_GENERIC,    //!< Like `TOWARDS_NODE`, but parametrized like a beam in truck fileformat.
    HALFBEAM_ROPE,       //!< Like `TOWARDS_NODE`, but parametrized like a rope-beam in truck fileformat.
};

/// Global force affecting particular (base) node of particular (base) actor; added ad-hoc by scripts.
struct FreeForce
{
    // Common params:
    FreeForceID_t        ffc_id = FREEFORCEID_INVALID;
    FreeForceType        ffc_type = FreeForceType::DUMMY;
    float                ffc_force_magnitude = 0.f;
    ActorPtr             ffc_base_actor;
    NodeNum_t            ffc_base_node = NODENUM_INVALID;
    // Direction-specific params:
    Ogre::Vector3        ffc_force_const_direction = Ogre::Vector3::ZERO; //!< Expected to be normalized; only effective with `FreeForceType::CONSTANT`
    Ogre::Vector3        ffc_target_coords = Ogre::Vector3::ZERO;
    ActorPtr             ffc_target_actor;
    NodeNum_t            ffc_target_node = NODENUM_INVALID;
    // Half-beam specific params:
    float                ffc_halfb_spring = DEFAULT_SPRING;
    float                ffc_halfb_damp = DEFAULT_DAMP;
    float                ffc_halfb_deform = BEAM_DEFORM;
    float                ffc_halfb_strength = BEAM_BREAK; //!< Breaking threshold
    float                ffc_halfb_diameter = DEFAULT_BEAM_DIAMETER;
    float                ffc_halfb_plastic_coef = BEAM_PLASTIC_COEF_DEFAULT;
    // Half-beam simulation state:
    float                ffc_halfb_L = 0.f; //!< Length at rest, including permanent deformations.
    float                ffc_halfb_stress = 0.f;
    float                ffc_halfb_minmaxposnegstress = 0.f;
    float                ffc_halfb_maxposstress = 0.f;
    float                ffc_halfb_maxnegstress = 0.f;
};

/// Common for ADD and MODIFY requests; tailored for use with AngelScript thru `GameScript::pushMessage()`.
struct FreeForceRequest
{
    // AngelScript `dictionary` converts all primitives to `double` or `int64`, see 'scriptdictionary.cpp', function `Set()`
    
    // common params:
    int64_t ffr_id = FREEFORCEID_INVALID;
    int64_t ffr_type = (int64_t)FreeForceType::DUMMY;
    double ffr_force_magnitude = 0.0;
    int64_t ffr_base_actor = ACTORINSTANCEID_INVALID;
    int64_t ffr_base_node = NODENUM_INVALID;
    // direction-specific params:
    Ogre::Vector3 ffr_force_const_direction = Ogre::Vector3::ZERO;
    Ogre::Vector3 ffr_target_coords = Ogre::Vector3::ZERO;
    int64_t ffr_target_actor = ACTORINSTANCEID_INVALID;
    int64_t ffr_target_node = NODENUM_INVALID;
    // Half-beam specific params:
    double ffr_halfb_spring = DEFAULT_SPRING;
    double ffr_halfb_damp = DEFAULT_DAMP;
    double ffr_halfb_deform = BEAM_DEFORM;
    double ffr_halfb_strength = BEAM_DEFORM;
    double ffr_halfb_diameter = DEFAULT_BEAM_DIAMETER;
    double ffr_halfb_plastic_coef = BEAM_PLASTIC_COEF_DEFAULT;
};

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
    ActorSpawnRequest();
    ~ActorSpawnRequest();

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

    ActorInstanceID_t   asr_instance_id = ACTORINSTANCEID_INVALID; //!< Optional; see `ActorManager::GetActorNextInstanceID()`;
    CacheEntryPtr       asr_cache_entry; //!< Optional, overrides 'asr_filename' and 'asr_cache_entry_num'
    std::string         asr_filename;    //!< Can be in "Bundle-qualified" format, i.e. "mybundle.zip:myactor.truck"
    Ogre::String        asr_config;
    Ogre::Vector3       asr_position = Ogre::Vector3::ZERO;
    Ogre::Quaternion    asr_rotation = Ogre::Quaternion::ZERO;
    collision_box_t*    asr_spawnbox = nullptr;
    CacheEntryPtr       asr_skin_entry;
    CacheEntryPtr       asr_tuneup_entry; //!< Only filled when user selected a saved/downloaded .tuneup mod in SelectorUI.
    TuneupDefPtr        asr_working_tuneup; //!< Only filled when editing tuneup via Tuning menu.
    Origin              asr_origin = Origin::UNKNOWN;
    int                 asr_debugview = 0; //(int)DebugViewType::DEBUGVIEW_NONE;
    std::string     asr_net_username;
    int                 asr_net_color = 0;
    BitMask_t           asr_net_peeropts = BitMask_t(0); //!< `RoRnet::PeerOptions` to be applied after spawn.
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
    ActorModifyRequest();
    ~ActorModifyRequest();

    enum class Type
    {
        INVALID,
        RELOAD,               //!< Full reload from filesystem, requested by user
        RESET_ON_INIT_POS,
        RESET_ON_SPOT,
        SOFT_RESPAWN, //!< Like hard reset, but positions the actor like spawn process does - using the relative positions from rig-def file (respecting Tuning system tweaks).
        SOFT_RESET,
        RESTORE_SAVED,
        WAKE_UP,
        REFRESH_VISUALS //!< Forces a synchronous update of visuals from any context - i.e. from terrain editor mode or with sleeping/physicspaused actor.
    };

    ActorInstanceID_t   amr_actor = ACTORINSTANCEID_INVALID;// not ActorPtr because it's not thread-safe
    Type                amr_type;
    std::shared_ptr<rapidjson::Document>
                        amr_saved_state;
    CacheEntryPtr       amr_addonpart; //!< Primary method of specifying cache entry.
    std::string         amr_addonpart_fname; //!< Fallback method in case CacheEntry doesn't exist anymore - that means mod was uninstalled in the meantime. Used by REMOVE_ADDONPART_AND_RELOAD.
    Ogre::Vector3       amr_softrespawn_position; //!< Position to use with `SOFT_RESPAWN`.
    Ogre::Quaternion    amr_softrespawn_rotation; //!< Rotation to use with `SOFT_RESPAWN`; use `TObjParser::CalcRotation()` to calculate quaternion from XYZ like in TOBJ file.
};

enum class ActorLinkingRequestType
{
    INVALID,
    LOAD_SAVEGAME,
    // hookToggle()
    HOOK_LOCK,
    HOOK_UNLOCK,
    HOOK_TOGGLE,
    HOOK_MOUSE_TOGGLE,
    HOOK_RESET,
    // tieToggle()
    TIE_TOGGLE,
    TIE_RESET,
    // ropeToggle()
    ROPE_TOGGLE,
    ROPE_RESET,
    // toggleSlideNodeLock()
    SLIDENODE_TOGGLE
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
    NodeNum_t alr_hook_mousenode = NODENUM_INVALID;
    // tieToggle()
    int alr_tie_group = -1;
    // ropeToggle()
    int alr_rope_group = -1;
};

///  Parameter to `Actor::setSimAttribute()` and `Actor::getSimAttribute()`; allows advanced users to tweak physics internals via script.
///  Each value represents a variable, either directly in `Actor` or a subsystem, i.e. `Engine`.
///  PAY ATTENTION to the 'safe value' limits below - those may not be checked when setting attribute values!
enum ActorSimAttr
{
    ACTORSIMATTR_NONE,

    // TractionControl
    ACTORSIMATTR_TC_RATIO, //!< Regulating force, safe values: <1 - 20>
    ACTORSIMATTR_TC_PULSE_TIME, //!< Pulse duration in seconds, safe values <0.00005 - 1>
    ACTORSIMATTR_TC_WHEELSLIP_CONSTANT, //!< Minimum wheel slip threshold, safe value = 0.25

    // Engine
    ACTORSIMATTR_ENGINE_SHIFTDOWN_RPM, //!< Automatic transmission - Param #1 of 'engine'
    ACTORSIMATTR_ENGINE_SHIFTUP_RPM, //!< Automatic transmission - Param #2 of 'engine'
    ACTORSIMATTR_ENGINE_TORQUE, //!< Engine torque in newton-meters (N/m) - Param #3 of 'engine'
    ACTORSIMATTR_ENGINE_DIFF_RATIO, //!< Differential ratio (aka global gear ratio) - Param #4 of 'engine'
    ACTORSIMATTR_ENGINE_GEAR_RATIOS_ARRAY, //!< Gearbox - Format: "<reverse_gear> <neutral_gear> <forward_gear 1> [<forward gear 2>]..."; Param #5 and onwards of 'engine'.

    // Engoption
    ACTORSIMATTR_ENGOPTION_ENGINE_INERTIA, //!< - Param #1 of 'engoption'
    ACTORSIMATTR_ENGOPTION_ENGINE_TYPE, //!< - Param #2 of 'engoption'
    ACTORSIMATTR_ENGOPTION_CLUTCH_FORCE, //!< - Param #3 of 'engoption'
    ACTORSIMATTR_ENGOPTION_SHIFT_TIME, //!< - Param #4 of 'engoption'
    ACTORSIMATTR_ENGOPTION_CLUTCH_TIME, //!< - Param #5 of 'engoption'
    ACTORSIMATTR_ENGOPTION_POST_SHIFT_TIME, //!< Time (in seconds) until full torque is transferred - Param #6 of 'engoption'
    ACTORSIMATTR_ENGOPTION_STALL_RPM, //!< RPM where engine stalls - Param #7 of 'engoption'
    ACTORSIMATTR_ENGOPTION_IDLE_RPM, //!< Target idle RPM - Param #8 of 'engoption'
    ACTORSIMATTR_ENGOPTION_MAX_IDLE_MIXTURE, //!< Max throttle to maintain idle RPM - Param #9 of 'engoption'
    ACTORSIMATTR_ENGOPTION_MIN_IDLE_MIXTURE, //!< Min throttle to maintain idle RPM - Param #10 of 'engoption'
    ACTORSIMATTR_ENGOPTION_BRAKING_TORQUE, //!< How much engine brakes on zero throttle - Param #11 of 'engoption'

    // Engturbo2 (actually 'engturbo' with Param #1 [type] set to "2" - the recommended variant)
    ACTORSIMATTR_ENGTURBO2_INERTIA_FACTOR, //!< Time to spool up - Param #2 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_NUM_TURBOS, //!< Number of turbos - Param #3 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_MAX_RPM, //!< MaxPSI * 10000 ~ calculated from Param #4 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_ENGINE_RPM_OP, //!< Engine RPM threshold for turbo to operate - Param #5 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_BOV_ENABLED, //!< Blow-off valve - Param #6 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_BOV_MIN_PSI, //!< Blow-off valve PSI threshold - Param #7 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_WASTEGATE_ENABLED, //!<  - Param #8 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_WASTEGATE_MAX_PSI, //!<  - Param #9 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_WASTEGATE_THRESHOLD_N, //!< 1 - WgThreshold ~ calculated from  Param #10 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_WASTEGATE_THRESHOLD_P, //!< 1 + WgThreshold ~ calculated from  Param #10 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_ANTILAG_ENABLED, //!<  - Param #11 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_ANTILAG_CHANCE, //!<  - Param #12 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_ANTILAG_MIN_RPM, //!<  - Param #13 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_ANTILAG_POWER, //!<  - Param #14 of 'engturbo2'

};
const char* ActorSimAttrToString(ActorSimAttr attr);

} // namespace RoR
