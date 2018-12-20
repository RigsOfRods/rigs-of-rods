/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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
/// @brief Core data structures for simulation.

#pragma once

/*
 A word of warning:
 RoR's performance is very sensitive to the ordering of the parameters in this
 structure (due to cache reasons). You can easily destroy RoR's performance if you put
 something in the wrong place. Unless you know what you are doing (do you come armed
 with a cache usage tracker?), add what you wish to the bottom of the structure.
*/

// The RoR required includes (should be included already)
#include "ForwardDeclarations.h"
#include "BeamConstants.h"
#include "BitFlags.h"

enum event_types {
    EVENT_NONE=0,
    EVENT_ALL,
    EVENT_AVATAR,
    EVENT_TRUCK,
    EVENT_AIRPLANE,
    EVENT_BOAT,
    EVENT_DELETE
};

enum hook_states {
    HOOK_LOCK=0,
    HOOK_UNLOCK,
    HOOK_TOGGLE,
    MOUSE_HOOK_TOGGLE,
};

/* Enumerations */
enum {
    BEAM_NORMAL,
    BEAM_HYDRO,
    BEAM_VIRTUAL,         //!< Excluded from mass calculations, visuals permanently disabled
};

enum {
    UNLOCKED,       //!< lock not locked
    PREUNLOCK,      //!< preunlocking, inter-actor beam deletion in progress
    PRELOCK,        //!< prelocking, attraction forces in action
    LOCKED          //!< lock locked.
};
enum {
    NOT_DRIVEABLE,  //!< not drivable at all
    TRUCK,          //!< its a truck (or other land vehicle)
    AIRPLANE,       //!< its an airplane
    BOAT,           //!< its a boat
    MACHINE,        //!< its a machine
    AI,             //!< machine controlled by an Artificial Intelligence
};

enum {
    NOSHOCK,        //!< not a shock
    SHOCK1,         //!< shock1
    SHOCK2,         //!< shock2
    SHOCK3,         //!< shock3
    TRIGGER,        //!< trigger
    SUPPORTBEAM,    //!<
    ROPE            //!<
};
enum blinktype {
    BLINK_NONE,     //!<
    BLINK_LEFT,     //!<
    BLINK_RIGHT,    //!<
    BLINK_WARN      //!<
};

enum {
    HYDRO_FLAG_SPEED        = BITMASK(1),
    HYDRO_FLAG_DIR          = BITMASK(2),
    HYDRO_FLAG_AILERON      = BITMASK(3),
    HYDRO_FLAG_RUDDER       = BITMASK(4),
    HYDRO_FLAG_ELEVATOR     = BITMASK(5),
    HYDRO_FLAG_REV_AILERON  = BITMASK(6),
    HYDRO_FLAG_REV_RUDDER   = BITMASK(7),
    HYDRO_FLAG_REV_ELEVATOR = BITMASK(8),
};

enum {
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

enum {
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

enum {
    SHOCK_FLAG_NORMAL			= BITMASK(1),
    SHOCK_FLAG_LACTIVE			= BITMASK(3),
    SHOCK_FLAG_RACTIVE			= BITMASK(4),
    SHOCK_FLAG_ISSHOCK2			= BITMASK(5),
    SHOCK_FLAG_ISSHOCK3			= BITMASK(6),
    SHOCK_FLAG_SOFTBUMP			= BITMASK(7),
    SHOCK_FLAG_ISTRIGGER		= BITMASK(8),
    SHOCK_FLAG_TRG_BLOCKER		= BITMASK(9),
    SHOCK_FLAG_TRG_CMD_SWITCH	= BITMASK(10),
    SHOCK_FLAG_TRG_CMD_BLOCKER	= BITMASK(11),
    SHOCK_FLAG_TRG_BLOCKER_A	= BITMASK(12),
    SHOCK_FLAG_TRG_HOOK_UNLOCK 	= BITMASK(13),
    SHOCK_FLAG_TRG_HOOK_LOCK   	= BITMASK(14),
    SHOCK_FLAG_TRG_CONTINUOUS	= BITMASK(15),
    SHOCK_FLAG_TRG_ENGINE		= BITMASK(16)
};

enum {
    TRG_ENGINE_CLUTCH    = 0,
    TRG_ENGINE_BRAKE     = 1,
    TRG_ENGINE_ACC       = 2,
    TRG_ENGINE_RPM       = 3,
    TRG_ENGINE_SHIFTUP   = 4,
    TRG_ENGINE_SHIFTDOWN = 5 
};

enum {
    DEFAULT_DETACHER_GROUP  = 0, // default for detaching beam group
};

enum {
    NOWHEEL,
    WHEEL_DEFAULT,
    WHEEL_2,
    WHEEL_FLEXBODY
};

enum class FlareType: char
{
    NONE           = 0,
    HEADLIGHT      = 'f',
    BRAKE_LIGHT    = 'b',
    REVERSE_LIGHT  = 'R',
    BLINKER_LEFT   = 'l',
    BLINKER_RIGHT  = 'r',
    USER           = 'u'
};

/* some info holding arrays */
static const float flapangles[6] = {0.f, -0.07f, -0.17f, -0.33f, -0.67f, -1.f};

/* basic structures */

#include "datatypes/node_t.h"
#include "datatypes/shock_t.h"

struct collcab_rate_t
{
    int rate;     // remaining amount of physics cycles to be skipped
    int distance; // distance (in physics cycles) to the previous collision check
};

#include "datatypes/beam_t.h"

struct soundsource_t
{
    SoundScriptInstance* ssi;
    int nodenum;
    int type;
};

#include "datatypes/wheel_t.h"

struct hook_t
{
    int     hk_locked;
    int     hk_group;
    int     hk_lockgroup;
    bool    hk_lock_nodes;
    bool    hk_selflock;
    bool    hk_autolock;
    bool    hk_nodisable;
    float   hk_maxforce;
    float   hk_lockrange;
    float   hk_lockspeed;
    float   hk_timer;
    float   hk_timer_preset;
    float   hk_min_length; //!< Absolute value in meters
    node_t* hk_hook_node;
    node_t* hk_lock_node;
    beam_t* hk_beam;
    Actor*  hk_locked_actor;
};

struct ropable_t
{
    node_t *node;
    int group;
    bool multilock;
    bool in_use;
};

struct rope_t
{
    int        rp_locked;
    int        rp_group;
    beam_t*    rp_beam;
    node_t*    rp_locked_node;
    ropable_t* rp_locked_ropable;
    Actor*     rp_locked_actor;
};

struct tie_t
{
    Actor*     ti_locked_actor;
    beam_t*    ti_beam;
    ropable_t* ti_locked_ropable;
    int        ti_group;
    float      ti_command_value;
    float      ti_contract_speed;
    float      ti_max_stress;
    float      ti_min_length;       //!< Proportional to orig; length

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
};

struct hydrobeam_t
{
    uint16_t hb_beam_index; //!< Index to Actor::ar_beams array
    float    hb_ref_length; //!< Idle length in meters
    float    hb_speed;      //!< Rate of change
    int      hb_flags;
    int      hb_anim_flags; //!< Animators (beams updating length based on simulation variables)
    float    hb_anim_param; //!< Animators (beams updating length based on simulation variables)
};

struct rotator_t
{
    int nodes1[4];
    int nodes2[4];
    int axis1; //!< rot axis
    int axis2;
    float angle;
    float rate;
    float force;
    float tolerance;
    float rotatorEngineCoupling;
    bool rotatorNeedsEngine;
};

struct flare_t
{
    int noderef;
    int nodex;
    int nodey;
    float offsetx;
    float offsety;
    float offsetz;
    Ogre::SceneNode *snode;
    Ogre::BillboardSet *bbs;
    Ogre::Light *light;
    FlareType fl_type;
    int controlnumber;
    bool controltoggle_status;
    float blinkdelay;
    float blinkdelay_curr;
    bool blinkdelay_state;
    float size;
    bool isVisible;
};

/**
* SIM-CORE; Prop = an object mounted on vehicle chassis.
*/
struct prop_t
{
    prop_t() { memset(this, 0, sizeof(*this)); }

    int noderef;
    int nodex;
    int nodey;
    float offsetx;
    float offsety;
    float offsetz;
    float rotaX;
    float rotaY;
    float rotaZ;
    float orgoffsetX;
    float orgoffsetY;
    float orgoffsetZ;
    Ogre::Quaternion rot;
    Ogre::SceneNode *scene_node; //!< The pivot scene node (parented to root-node).
    Ogre::SceneNode *wheel; //!< Special prop: custom steering wheel for dashboard
    Ogre::Vector3 wheelpos; //!< Special prop: custom steering wheel for dashboard
    int mirror;             //<! Special prop: rear view mirror {0 = disabled, -1 = right, 1 = left}
    char beacontype;        //<! Special prop: beacon {0 = none, 'b' = user-specified, 'r' = red, 'p' = police lightbar, 'L'/'R'/'w' - aircraft wings}

    // formerly named "bbs"
    Ogre::BillboardSet *beacon_flares_billboard_system[4];

    // formerly named bbsnode
    Ogre::SceneNode *beacon_flare_billboard_scene_node[4];

    // formerly named "light"
    Ogre::Light *beacon_light[4];

    // formerly named "brate"
    float beacon_light_rotation_rate[4]; //<! Radians per second
    
    // formerly named "bpos"
    float beacon_light_rotation_angle[4]; //<! Radians
    
    float animratio[10]; //!< A coefficient for the animation, prop degree if used with mode: rotation and propoffset if used with mode: offset.
    int animFlags[10];
    int animMode[10];
    float animOpt3[10]; //!< Various purposes
    float animOpt5[10];
    int animKey[10];
    int animKeyState[10];
    int lastanimKS[10];
    Ogre::Real wheelrotdegree;
    int cameramode; //!< Visibility control {-2 = always, -1 = 3rdPerson only, 0+ = cinecam index}
    MeshObject *mo;
    MeshObject *wheelmo;

    struct {
        float lower_limit;  //!< The lower limit for the animation
        float upper_limit;  //!< The upper limit for the animation
    } constraints[10];

    int  pp_aero_engine_idx;          //!< Special - a turboprop/pistonprop reference
    bool pp_aero_propeller_blade:1;   //!< Special - single blade mesh
    bool pp_aero_propeller_spin:1;    //!< Special - blurred spinning propeller effect
};

struct exhaust_t
{
    int emitterNode;
    int directionNode;
    char material[256];
    float factor;
    bool isOldFormat; //!< False if defined in 'exhausts' section, true if defined in 'nodes' by 'x'/'y' flag.
    Ogre::SceneNode *smokeNode;
    Ogre::ParticleSystem* smoker;
};


struct cparticle_t
{
    int emitterNode;
    int directionNode;
    bool active;
    Ogre::SceneNode *snode;
    Ogre::ParticleSystem* psys;
};



// some non-beam structs


struct collision_box_t
{
    bool virt;
    bool refined;
    bool selfrotated;
    bool camforced;
    bool enabled;
    short event_filter;
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
};

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

struct authorinfo_t
{
    int id;
    Ogre::String type;
    Ogre::String name;
    Ogre::String email;
};

namespace RoR {

struct ActorSpawnRequest
{
    enum class Origin //!< Enables special processing
    {
        UNKNOWN,
        CONFIG_FILE,  //!< 'Preselected vehicle' in RoR.cfg
        TERRN_DEF,    //!< Preloaded with terrain
        USER,         //!< Direct selection by user via GUI
        NETWORK       //!< Remote controlled
    };

    ActorSpawnRequest();

    CacheEntry*       asr_cache_entry; //!< Optional, overrides 'asr_filename' and 'asr_cache_entry_num'
    std::string       asr_filename;
    std::vector<Ogre::String> asr_config;
    Ogre::Vector3     asr_position;
    Ogre::Quaternion  asr_rotation;
    int               asr_cache_entry_num;
    collision_box_t*  asr_spawnbox;
    RoR::SkinDef*     asr_skin;
    Origin            asr_origin;
    bool              asr_free_position:1;   //!< Disables the automatic spawn position adjustment
    bool              asr_terrn_machine:1;   //!< This is a fixed machinery
};

struct ActorModifyRequest
{
    enum class Type
    {
        INVALID,
        RELOAD,               //!< Full reload from filesystem, requested by user
        RESET_ON_INIT_POS,
        RESET_ON_SPOT
    };

    Actor* amr_actor;
    Type   amr_type;
};

} // namespace RoR
