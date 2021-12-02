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

/** 
    @file   RigDef_File.h
    @author Petr Ohlidal
    @date   12/2013
    @brief  Data structures representing 'truck' file format, see https://docs.rigsofrods.org/vehicle-creation/fileformat-truck/ for reference.

    CODING GUIDELINES (CLEANUP IN PROGRESS):
    * each element (line) should have dedicated `struct` for data. Auto-importing to higher version (i.e. 'flares' -> 'flares2') is OK. All other "sharing" mechanisms must go away.
    * all data should be stored in `std::vector<>`s, all other random mechanisms must go away.
    * the data vectors should be named exactly as their file format keywords.
    * each option-string should have dedicated `enum class` with fields named '{letter}_{MEANING}'. All other constants must go away.
    * all bitmasks should use `BitMask_t` data type.
    * all bitmask constants should have format `static const BitMask_t MEANING = BITMASK({number})`.
    * all option strings should have parsing function `GetArgWhatever(int index)`.
    * option-strings should be stored as bitmasks (unless order matters). If only single option is acceptable, use the enum directly. All other mechanisms must go away.
    * all data structs should contain only arguments in order defined by the fileformat. Helper data must be prefixed with `_`, for example argument count (where it matters).
*/

#pragma once

#include "Application.h"
#include "BitFlags.h"
#include "RigDef_Node.h"
#include "SimConstants.h"
#include "SimData.h"

#include <list>
#include <memory>
#include <vector>
#include <string>
#include <OgreString.h>
#include <OgreVector3.h>
#include <OgreStringConverter.h>

namespace RigDef {

extern const char* ROOT_MODULE_NAME;

// IMPORTANT! If you add a value here, you must also modify Regexes::IDENTIFY_KEYWORD, it relies on numeric values of this enum.
enum Keyword
{
    KEYWORD_ADD_ANIMATION = 1,
    KEYWORD_AIRBRAKES,
    KEYWORD_ANIMATORS,
    KEYWORD_ANTILOCKBRAKES,
    KEYWORD_AUTHOR,
    KEYWORD_AXLES,
    KEYWORD_BACKMESH,
    KEYWORD_BEAMS,
    KEYWORD_BRAKES,
    KEYWORD_CAB,
    KEYWORD_CAMERARAIL,
    KEYWORD_CAMERAS,
    KEYWORD_CINECAM,
    KEYWORD_COLLISIONBOXES,
    KEYWORD_COMMANDS,
    KEYWORD_COMMANDS2,
    KEYWORD_COMMENT,
    KEYWORD_CONTACTERS,
    KEYWORD_CRUISECONTROL,
    KEYWORD_DESCRIPTION,
    KEYWORD_DETACHER_GROUP,
    KEYWORD_DISABLEDEFAULTSOUNDS,
    KEYWORD_ENABLE_ADVANCED_DEFORMATION,
    KEYWORD_END,
    KEYWORD_END_COMMENT,
    KEYWORD_END_DESCRIPTION,
    KEYWORD_END_SECTION,
    KEYWORD_ENGINE,
    KEYWORD_ENGOPTION,
    KEYWORD_ENGTURBO,
    KEYWORD_ENVMAP,
    KEYWORD_EXHAUSTS,
    KEYWORD_EXTCAMERA,
    KEYWORD_FILEFORMATVERSION,
    KEYWORD_FILEINFO,
    KEYWORD_FIXES,
    KEYWORD_FLARES,
    KEYWORD_FLARES2,
    KEYWORD_FLEXBODIES,
    KEYWORD_FLEXBODY_CAMERA_MODE,
    KEYWORD_FLEXBODYWHEELS,
    KEYWORD_FORSET,
    KEYWORD_FORWARDCOMMANDS,
    KEYWORD_FUSEDRAG,
    KEYWORD_GLOBALS,
    KEYWORD_GUID,
    KEYWORD_GUISETTINGS,
    KEYWORD_HELP,
    KEYWORD_HIDEINCHOOSER,
    KEYWORD_HOOKGROUP, // obsolete, ignored
    KEYWORD_HOOKS,
    KEYWORD_HYDROS,
    KEYWORD_IMPORTCOMMANDS,
    KEYWORD_INTERAXLES,
    KEYWORD_LOCKGROUPS,
    KEYWORD_LOCKGROUP_DEFAULT_NOLOCK,
    KEYWORD_MANAGEDMATERIALS,
    KEYWORD_MATERIALFLAREBINDINGS,
    KEYWORD_MESHWHEELS,
    KEYWORD_MESHWHEELS2,
    KEYWORD_MINIMASS,
    KEYWORD_NODECOLLISION, // obsolete
    KEYWORD_NODES,
    KEYWORD_NODES2,
    KEYWORD_PARTICLES,
    KEYWORD_PISTONPROPS,
    KEYWORD_PROP_CAMERA_MODE,
    KEYWORD_PROPS,
    KEYWORD_RAILGROUPS,
    KEYWORD_RESCUER,
    KEYWORD_RIGIDIFIERS, // obsolete
    KEYWORD_ROLLON,
    KEYWORD_ROPABLES,
    KEYWORD_ROPES,
    KEYWORD_ROTATORS,
    KEYWORD_ROTATORS2,
    KEYWORD_SCREWPROPS,
    KEYWORD_SECTION,
    KEYWORD_SECTIONCONFIG,
    KEYWORD_SET_BEAM_DEFAULTS,
    KEYWORD_SET_BEAM_DEFAULTS_SCALE,
    KEYWORD_SET_COLLISION_RANGE,
    KEYWORD_SET_DEFAULT_MINIMASS,
    KEYWORD_SET_INERTIA_DEFAULTS,
    KEYWORD_SET_MANAGEDMATERIALS_OPTIONS,
    KEYWORD_SET_NODE_DEFAULTS,
    KEYWORD_SET_SHADOWS,
    KEYWORD_SET_SKELETON_SETTINGS,
    KEYWORD_SHOCKS,
    KEYWORD_SHOCKS2,
    KEYWORD_SHOCKS3,
    KEYWORD_SLIDENODE_CONNECT_INSTANTLY,
    KEYWORD_SLIDENODES,
    KEYWORD_SLOPE_BRAKE,
    KEYWORD_SOUNDSOURCES,
    KEYWORD_SOUNDSOURCES2,
    KEYWORD_SPEEDLIMITER,
    KEYWORD_SUBMESH,
    KEYWORD_SUBMESH_GROUNDMODEL,
    KEYWORD_TEXCOORDS,
    KEYWORD_TIES,
    KEYWORD_TORQUECURVE,
    KEYWORD_TRACTIONCONTROL,
    KEYWORD_TRANSFERCASE,
    KEYWORD_TRIGGERS,
    KEYWORD_TURBOJETS,
    KEYWORD_TURBOPROPS,
    KEYWORD_TURBOPROPS2,
    KEYWORD_VIDEOCAMERA,
    KEYWORD_WHEELDETACHERS,
    KEYWORD_WHEELS,
    KEYWORD_WHEELS2,
    KEYWORD_WINGS,

    KEYWORD_INVALID = 0xFFFFFFFF
};

const char* KeywordToString(Keyword keyword);

enum class DifferentialType: char
{
    o_OPEN    = 'o',
    l_LOCKED  = 'l',
    s_SPLIT   = 's',
    v_VISCOUS = 'v'
};

typedef std::vector<DifferentialType> DifferentialTypeVec;

enum class MinimassOption: char
{
    n_DUMMY = 'n',
    l_SKIP_LOADED = 'l'  //!< Only apply minimum mass to nodes without "L" option.
};

enum class WheelBraking: int
{
    NONE                 = 0,
    FOOT_HAND            = 1,
    FOOT_HAND_SKID_LEFT  = 2,
    FOOT_HAND_SKID_RIGHT = 3,
    FOOT_ONLY            = 4,
};

enum class WheelPropulsion: int
{
    NONE     = 0,
    FORWARD  = 1,
    BACKWARD = 2,
};

enum class WingControlSurface: char
{
    n_NONE                  = 'n',
    a_RIGHT_AILERON         = 'a',
    b_LEFT_AILERON          = 'b',
    f_FLAP                  = 'f',
    e_ELEVATOR              = 'e',
    r_RUDDER                = 'r',
    S_RIGHT_HAND_STABILATOR = 'S',
    T_LEFT_HAND_STABILATOR  = 'T',
    c_RIGHT_ELEVON          = 'c',
    d_LEFT_ELEVON           = 'd',
    g_RIGHT_FLAPERON        = 'g',
    h_LEFT_FLAPERON         = 'h',
    U_RIGHT_HAND_TAILERON   = 'U',
    V_LEFT_HAND_TAILERON    = 'V',
    i_RIGHT_RUDDERVATOR     = 'i',
    j_LEFT_RUDDERVATOR      = 'j',
};

enum class TieOption: char
{
    n_DUMMY        = 'n',
    v_DUMMY        = 'v',
    i_INVISIBLE    = 'i',
    s_NO_SELF_LOCK = 's',
};

enum class CabOption: char
{
    c_CONTACT              = 'c',
    b_BUOYANT              = 'b',
    p_10xTOUGHER           = 'p',
    u_INVULNERABLE         = 'u',
    s_BUOYANT_NO_DRAG      = 's',
    r_BUOYANT_ONLY_DRAG    = 'r',
    D_CONTACT_BUOYANT      = 'D',
    F_10xTOUGHER_BUOYANT   = 'F',
    S_INVULNERABLE_BUOYANT = 'S',
};

enum class TriggerOption: char
{
    i_INVISIBLE             = 'i', //!< invisible
    c_COMMAND_STYLE         = 'c', //!< trigger is set with commandstyle boundaries instead of shocksytle
    x_START_DISABLED        = 'x', //!< this trigger is disabled on startup, default is enabled
    b_KEY_BLOCKER           = 'b', //!< Set the CommandKeys that are set in a commandkeyblocker or trigger to blocked on startup, default is released
    B_TRIGGER_BLOCKER       = 'B', //!< Blocker that enable/disable other triggers
    A_INV_TRIGGER_BLOCKER   = 'A', //!< Blocker that enable/disable other triggers, reversed activation method (inverted Blocker style, auto-ON)
    s_CMD_NUM_SWITCH        = 's', //!< switch that exchanges cmdshort/cmdshort for all triggers with the same commandnumbers, default false
    h_UNLOCKS_HOOK_GROUP    = 'h',
    H_LOCKS_HOOK_GROUP      = 'H',
    t_CONTINUOUS            = 't', //!< this trigger sends values between 0 and 1
    E_ENGINE_TRIGGER        = 'E', //!< this trigger is used to control an engine
};

enum class BeamOption: char
{
    i_INVISIBLE = 'i',
    r_ROPE      = 'r',
    s_SUPPORT   = 's',
};

/* -------------------------------------------------------------------------- */
/* Utility                                                                    */
/* -------------------------------------------------------------------------- */

struct CameraSettings
{
    CameraSettings():
        mode(MODE_ALWAYS),
        cinecam_index(0)
    {}

    enum Mode
    {
        MODE_ALWAYS   = -2,
        MODE_EXTERNAL = -1,
        MODE_CINECAM  = 1,

        MODE_INVALID  = 0xFFFFFFFF
    };

    Mode mode;
    unsigned int cinecam_index;
};

/* -------------------------------------------------------------------------- */
/* Directive SET_NODE_DEFAULTS                                                */
/* -------------------------------------------------------------------------- */

struct NodeDefaults
{
    NodeDefaults();

    float load_weight;
    float friction;
    float volume;
    float surface;
    unsigned int options; //!< Bit flags
};

/* -------------------------------------------------------------------------- */
/* Directive SET_BEAM_DEFAULTS_SCALE                                          */
/* -------------------------------------------------------------------------- */

struct BeamDefaultsScale
{
    BeamDefaultsScale():
        springiness(1),
        damping_constant(1),
        deformation_threshold_constant(1),
        breaking_threshold_constant(1)
    {}

    float springiness;
    float damping_constant;
    float deformation_threshold_constant;
    float breaking_threshold_constant;
};

/* -------------------------------------------------------------------------- */
/* Directive SET_BEAM_DEFAULTS                                                */
/* -------------------------------------------------------------------------- */

struct BeamDefaults
{
    BeamDefaults(): // NOTE: -1.f is 'empty value'; replaced by constant in parser.
        springiness(-1.f),
        damping_constant(-1.f),
        deformation_threshold(-1.f),
        visual_beam_diameter(-1.f),
        beam_material_name("tracks/beam"),
        plastic_deform_coef(0.f), // This is a default
        breaking_threshold(-1.f),
        _enable_advanced_deformation(false),
        _is_plastic_deform_coef_user_defined(false),
        _is_user_defined(false)
    {}

    float GetScaledSpringiness()
    {
        return springiness * scale.springiness;
    }

    float GetScaledDamping()
    {
        return damping_constant * scale.damping_constant;
    }

    float GetScaledBreakingThreshold()
    {
        return breaking_threshold * scale.breaking_threshold_constant;
    }

    inline float GetScaledDeformThreshold() const
    {
        return deformation_threshold * scale.deformation_threshold_constant;
    }

    float springiness;
    float damping_constant;
    float deformation_threshold;
    float breaking_threshold;
    float visual_beam_diameter;
    Ogre::String beam_material_name;
    float plastic_deform_coef;
    bool _enable_advanced_deformation; //!< Informs whether "enable_advanced_deformation" directive preceded these defaults.
    bool _is_plastic_deform_coef_user_defined;
    bool _is_user_defined; //!< Informs whether these data were read from "set_beam_defaults" directive or filled in by the parser on startup.
    BeamDefaultsScale scale;
};

/* -------------------------------------------------------------------------- */
/* Directive SET_COLLLISION_RANGE                                             */
/* -------------------------------------------------------------------------- */

struct CollisionRange
{
    float node_collision_range = -1;
};

/* -------------------------------------------------------------------------- */
/* Directive SET_DEFAULT_MINIMASS                                             */
/* -------------------------------------------------------------------------- */

struct DefaultMinimass
{
    float min_mass_Kg = DEFAULT_MINIMASS; //!< minimum node mass in Kg
};

/* -------------------------------------------------------------------------- */
/* Directive SET_DEFAULT_INERTIA                                              */
/* -------------------------------------------------------------------------- */

/** Common base for DefaultInertia and Command2Inertia */
struct Inertia 
{
    Inertia():
        start_delay_factor(0),
        stop_delay_factor(0)
    {}

    float start_delay_factor;
    float stop_delay_factor;
    Ogre::String start_function;
    Ogre::String stop_function;
};

/* -------------------------------------------------------------------------- */
/* Directive SET_MANAGEDMATERIALS_OPTIONS                                     */
/* -------------------------------------------------------------------------- */

struct ManagedMaterialsOptions
{
    ManagedMaterialsOptions():
        double_sided(false)
    {}

    bool double_sided;
};

/* -------------------------------------------------------------------------- */
/* Directive SET_SHADOWS                                                      */
/* -------------------------------------------------------------------------- */

struct ShadowOptions
{
    ShadowOptions():
        shadow_mode(0)
    {}

    int shadow_mode;
};

/* -------------------------------------------------------------------------- */
/* Section GLOBALS                                                            */
/* -------------------------------------------------------------------------- */

struct Globals
{
    Globals():
        dry_mass(0), /* The default */
        cargo_mass(0) /* The default */
    {}

    float dry_mass;
    float cargo_mass;
    Ogre::String material_name;
};

/* -------------------------------------------------------------------------- */
/* Section GUISETTINGS                                                        */
/* -------------------------------------------------------------------------- */

struct GuiSettings
{
    std::string key;
    std::string value;
};

/* -------------------------------------------------------------------------- */
/* Section AIRBRAKES                                                          */
/* -------------------------------------------------------------------------- */

struct Airbrake
{
    Airbrake();

    Node::Ref reference_node;
    Node::Ref x_axis_node;
    Node::Ref y_axis_node;
    Node::Ref aditional_node;
    Ogre::Vector3 offset;
    float width;
    float height;
    float max_inclination_angle;
    float texcoord_x1;
    float texcoord_x2;
    float texcoord_y1;
    float texcoord_y2;
    float lift_coefficient;
};

/* -------------------------------------------------------------------------- */
/* Directive ADD_ANIMATION                                                    */
/* -------------------------------------------------------------------------- */

struct Animation
{
    struct MotorSource
    {
        static const BitMask_t SOURCE_AERO_THROTTLE = BITMASK(1);
        static const BitMask_t SOURCE_AERO_RPM      = BITMASK(2);
        static const BitMask_t SOURCE_AERO_TORQUE   = BITMASK(3);
        static const BitMask_t SOURCE_AERO_PITCH    = BITMASK(4);
        static const BitMask_t SOURCE_AERO_STATUS   = BITMASK(5);

        BitMask_t source = 0;
        unsigned int motor = 0;
    };

    // Source flags
    static const BitMask_t SOURCE_AIRSPEED          = BITMASK( 1);
    static const BitMask_t SOURCE_VERTICAL_VELOCITY = BITMASK( 2);
    static const BitMask_t SOURCE_ALTIMETER_100K    = BITMASK( 3);
    static const BitMask_t SOURCE_ALTIMETER_10K     = BITMASK( 4);
    static const BitMask_t SOURCE_ALTIMETER_1K      = BITMASK( 5);
    static const BitMask_t SOURCE_ANGLE_OF_ATTACK   = BITMASK( 6);
    static const BitMask_t SOURCE_FLAP              = BITMASK( 7);
    static const BitMask_t SOURCE_AIR_BRAKE         = BITMASK( 8);
    static const BitMask_t SOURCE_ROLL              = BITMASK( 9);
    static const BitMask_t SOURCE_PITCH             = BITMASK(10);
    static const BitMask_t SOURCE_BRAKES            = BITMASK(11);
    static const BitMask_t SOURCE_ACCEL             = BITMASK(12);
    static const BitMask_t SOURCE_CLUTCH            = BITMASK(13);
    static const BitMask_t SOURCE_SPEEDO            = BITMASK(14);
    static const BitMask_t SOURCE_TACHO             = BITMASK(15);
    static const BitMask_t SOURCE_TURBO             = BITMASK(16);
    static const BitMask_t SOURCE_PARKING           = BITMASK(17);
    static const BitMask_t SOURCE_SHIFT_LEFT_RIGHT  = BITMASK(18);
    static const BitMask_t SOURCE_SHIFT_BACK_FORTH  = BITMASK(19);
    static const BitMask_t SOURCE_SEQUENTIAL_SHIFT  = BITMASK(20);
    static const BitMask_t SOURCE_SHIFTERLIN        = BITMASK(21);
    static const BitMask_t SOURCE_TORQUE            = BITMASK(22);
    static const BitMask_t SOURCE_HEADING           = BITMASK(23);
    static const BitMask_t SOURCE_DIFFLOCK          = BITMASK(24);
    static const BitMask_t SOURCE_BOAT_RUDDER       = BITMASK(25);
    static const BitMask_t SOURCE_BOAT_THROTTLE     = BITMASK(26);
    static const BitMask_t SOURCE_STEERING_WHEEL    = BITMASK(27);
    static const BitMask_t SOURCE_AILERON           = BITMASK(28);
    static const BitMask_t SOURCE_ELEVATOR          = BITMASK(29);
    static const BitMask_t SOURCE_AIR_RUDDER        = BITMASK(30);
    static const BitMask_t SOURCE_PERMANENT         = BITMASK(31);
    static const BitMask_t SOURCE_EVENT             = BITMASK(32);

    // Mode flags
    static const BitMask_t MODE_ROTATION_X          = BITMASK(1);
    static const BitMask_t MODE_ROTATION_Y          = BITMASK(2);
    static const BitMask_t MODE_ROTATION_Z          = BITMASK(3);
    static const BitMask_t MODE_OFFSET_X            = BITMASK(4);
    static const BitMask_t MODE_OFFSET_Y            = BITMASK(5);
    static const BitMask_t MODE_OFFSET_Z            = BITMASK(6);
    static const BitMask_t MODE_AUTO_ANIMATE        = BITMASK(7);
    static const BitMask_t MODE_NO_FLIP             = BITMASK(8);
    static const BitMask_t MODE_BOUNCE              = BITMASK(9);
    static const BitMask_t MODE_EVENT_LOCK          = BITMASK(10);

    float ratio = 0.f;
    float lower_limit = -1.f;
    float upper_limit = -1.f;
    BitMask_t source = 0;
    std::list<MotorSource> motor_sources;
    BitMask_t mode = 0;

    // NOTE: MSVC highlights 'event' as keyword: http://msdn.microsoft.com/en-us/library/4b612y2s%28v=vs.100%29.aspx
    // But it's ok to use as identifier in this context: http://msdn.microsoft.com/en-us/library/8d7y7wz6%28v=vs.100%29.aspx
    Ogre::String event;

    void AddMotorSource(BitMask_t source, unsigned int motor);
};

/* -------------------------------------------------------------------------- */
/* Section AXLES                                                              */
/* -------------------------------------------------------------------------- */

struct Axle
{
    Node::Ref wheels[2][2];
    DifferentialTypeVec options; //!< Order matters!
};

/* -------------------------------------------------------------------------- */
/* Section INTERAXLES                                                         */
/* -------------------------------------------------------------------------- */

struct InterAxle
{
    int a1 = 0;
    int a2 = 0;
    DifferentialTypeVec options; //!< Order matters!
};

/* -------------------------------------------------------------------------- */
/* Section TRANSFER_CASE                                                      */
/* -------------------------------------------------------------------------- */

struct TransferCase
{
    TransferCase():
        a1(0),
        a2(-1),
        has_2wd(true),
        has_2wd_lo(false),
        gear_ratios({1.0f})
    {}

    int a1;
    int a2;
    bool has_2wd;
    bool has_2wd_lo;
    std::vector<float> gear_ratios;
};

/* -------------------------------------------------------------------------- */
/* Section BEAMS                                                              */
/* -------------------------------------------------------------------------- */

struct Beam
{
    static const BitMask_t OPTION_i_INVISIBLE = BITMASK(1);
    static const BitMask_t OPTION_r_ROPE      = BITMASK(2);
    static const BitMask_t OPTION_s_SUPPORT   = BITMASK(3);

    Node::Ref nodes[2];
    BitMask_t options = 0;
    float extension_break_limit = 0.f;
    bool _has_extension_break_limit = false;
    int detacher_group = 0;
    std::shared_ptr<BeamDefaults> defaults;
};

/* -------------------------------------------------------------------------- */
/* Section CAMERAS                                                            */
/* -------------------------------------------------------------------------- */

struct Camera
{
    Node::Ref center_node;
    Node::Ref back_node;
    Node::Ref left_node;
};

/* -------------------------------------------------------------------------- */
/* Section CAMERARAIL                                                         */
/* -------------------------------------------------------------------------- */

struct CameraRail
{
    CameraRail()
    {
        nodes.reserve(25);
    }

    std::vector<Node::Ref> nodes;
};

/* -------------------------------------------------------------------------- */
/* Section CINECAM                                                            */
/* -------------------------------------------------------------------------- */

struct Cinecam
{
    Cinecam():
        position(Ogre::Vector3::ZERO),
        spring(8000.f),
        damping(800.f),
        node_mass(20.f)
    {}

    Ogre::Vector3 position;
    Node::Ref nodes[8];
    float spring;
    float damping;
    float node_mass;
    std::shared_ptr<BeamDefaults> beam_defaults;
    std::shared_ptr<NodeDefaults> node_defaults;
};

/* -------------------------------------------------------------------------- */
/* Section COLLISIONBOXES                                                     */
/* -------------------------------------------------------------------------- */

struct CollisionBox
{
    CollisionBox()
    {
        nodes.reserve(25);
    }

    std::vector<Node::Ref> nodes;
};

/* -------------------------------------------------------------------------- */
/* Section CRUISECONTROL                                                      */
/* -------------------------------------------------------------------------- */

struct CruiseControl
{
    CruiseControl():
        min_speed(0),
        autobrake(0)
    {}

    float min_speed;
    int autobrake;
};

/* -------------------------------------------------------------------------- */
/* Section AUTHOR                                                             */
/* -------------------------------------------------------------------------- */

struct Author
{
    Author():
        forum_account_id(0),
        _has_forum_account(false)
    {}

    Ogre::String type;
    unsigned int forum_account_id;
    Ogre::String name;
    Ogre::String email;
    bool _has_forum_account;
};

/* -------------------------------------------------------------------------- */
/* Inline - section FILEINFO                                                  */
/* -------------------------------------------------------------------------- */

struct Fileinfo
{
    Fileinfo():
        category_id(-1),
        file_version(0) /* Default */
    {}

    Ogre::String unique_id;
    int category_id;
    int file_version;
};

/* -------------------------------------------------------------------------- */
/* Section ENGINE                                                             */
/* -------------------------------------------------------------------------- */

struct Engine
{
    Engine():
        shift_down_rpm(0),
        shift_up_rpm(0),
        torque(0),
        global_gear_ratio(0),
        reverse_gear_ratio(0),
        neutral_gear_ratio(0)
    {
        gear_ratios.reserve(5);	
    }

    float shift_down_rpm;
    float shift_up_rpm;
    float torque;
    float global_gear_ratio;
    float reverse_gear_ratio;
    float neutral_gear_ratio;
    std::vector<float> gear_ratios;
};

/* -------------------------------------------------------------------------- */
/* Section ENGOPTION                                                          */
/* -------------------------------------------------------------------------- */

struct Engoption
{
    Engoption();

    enum EngineType
    {
        ENGINE_TYPE_c_CAR   = 'c',
        ENGINE_TYPE_e_ECAR  = 'e',
        ENGINE_TYPE_t_TRUCK = 't',

        ENGINE_TYPE_INVALID = 0xFFFFFFFF
    };

    float inertia;
    EngineType type;
    float clutch_force;
    float shift_time;       //!< Seconds
    float clutch_time;      //!< Seconds
    float post_shift_time;  //!< Seconds
    float idle_rpm;
    float stall_rpm;
    float max_idle_mixture;
    float min_idle_mixture;
    float braking_torque;
};

/* -------------------------------------------------------------------------- */
/* Section ENGTURBO                                                           */
/* -------------------------------------------------------------------------- */
 
struct Engturbo
{
    Engturbo();
    
    int version;
    float tinertiaFactor;
    int nturbos;
    float param1;
    float param2;
    float param3;
    float param4;
    float param5;
    float param6;
    float param7;
    float param8;
    float param9;
    float param10;
    float param11;
};

/* -------------------------------------------------------------------------- */
/* Section EXHAUSTS                                                           */
/* -------------------------------------------------------------------------- */

struct Exhaust
{
    Node::Ref reference_node;
    Node::Ref direction_node;
    Ogre::String particle_name;
};

/* -------------------------------------------------------------------------- */
/* Section EXTCAMERA                                                          */
/* -------------------------------------------------------------------------- */

struct ExtCamera
{
    ExtCamera():
        mode(MODE_CLASSIC)
    {}

    enum Mode
    {
        MODE_CLASSIC = 0, // Hardcoded in simulation code, do not change
        MODE_CINECAM = 1, // Hardcoded in simulation code, do not change
        MODE_NODE    = 2, // Hardcoded in simulation code, do not change

        MODE_INVALID = 0xFFFFFFFF
    };

    Mode mode;
    Node::Ref node;
};

/* -------------------------------------------------------------------------- */
/* Section FILEFORMATVERSION                                                  */
/* -------------------------------------------------------------------------- */

struct FileFormatVersion
{
    int version = -1;
};

/* -------------------------------------------------------------------------- */
/* Section GUID                                                               */
/* -------------------------------------------------------------------------- */

struct Guid
{
    std::string guid;
};

/* -------------------------------------------------------------------------- */
/* Section HELP                                                               */
/* -------------------------------------------------------------------------- */

struct Help
{
    std::string material;
};

/* -------------------------------------------------------------------------- */
/* Section BRAKES                                                             */
/* -------------------------------------------------------------------------- */

struct Brakes
{
    Brakes():
        default_braking_force(30000), // Default
        parking_brake_force(-1.f) // Empty value
    {}

    float default_braking_force;
    float parking_brake_force;
};

/* -------------------------------------------------------------------------- */
/* Section ANTI_LOCK_BRAKES                                                   */
/* -------------------------------------------------------------------------- */

struct AntiLockBrakes
{
    AntiLockBrakes();

    float regulation_force;
    unsigned int min_speed;
    float pulse_per_sec;
    bool attr_is_on;
    bool attr_no_dashboard;
    bool attr_no_toggle;
};

/* -------------------------------------------------------------------------- */
/* Section TRACTION_CONTROL                                                   */
/* -------------------------------------------------------------------------- */

struct TractionControl
{
    TractionControl();

    float regulation_force;
    float wheel_slip;
    float fade_speed;
    float pulse_per_sec;
    bool attr_is_on;
    bool attr_no_dashboard;
    bool attr_no_toggle;
};

/* -------------------------------------------------------------------------- */
/* Section WHEELDETACHERS                                                     */
/* -------------------------------------------------------------------------- */

struct WheelDetacher
{
    WheelDetacher():
        wheel_id(0),
        detacher_group(0)
    {}

    int wheel_id;
    int detacher_group;
};

/* -------------------------------------------------------------------------- */
/* Section WHEELS                                                             */
/* -------------------------------------------------------------------------- */

/** Attributes common to all wheel definitions */
struct BaseWheel
{
    float width = 0.f;
    unsigned int num_rays = 0u;
    Node::Ref nodes[2];
    Node::Ref rigidity_node;
    WheelBraking braking = WheelBraking::NONE;
    WheelPropulsion propulsion = WheelPropulsion::NONE;
    Node::Ref reference_arm_node;
    float mass = 0.f;
    std::shared_ptr<NodeDefaults> node_defaults;
    std::shared_ptr<BeamDefaults> beam_defaults;
};

/** The actual wheel */
struct Wheel: BaseWheel
{
    Wheel():
        radius(0),
        face_material_name("tracks/wheelface"),
        band_material_name("tracks/wheelband1")
    {}

    float radius;
    float springiness;
    float damping;
    Ogre::String face_material_name;
    Ogre::String band_material_name;
};

/* -------------------------------------------------------------------------- */
/* Section WHEELS_2                                                           */
/* -------------------------------------------------------------------------- */

/** Attributes common to WHEELS_2 and newer definitions */
struct BaseWheel2: BaseWheel
{
    BaseWheel2():
        rim_radius(0),
        tyre_radius(0),
        tyre_springiness(0),
        tyre_damping(0)
    {}

    float rim_radius;
    float tyre_radius;
    float tyre_springiness;
    float tyre_damping;
};

struct Wheel2: BaseWheel2
{
    Wheel2():
        BaseWheel2(),
        rim_springiness(0),
        rim_damping(0),
        face_material_name("tracks/wheelface"),
        band_material_name("tracks/wheelband1")
    {}

    Ogre::String face_material_name;
    Ogre::String band_material_name;
    float rim_springiness;
    float rim_damping;
};

/* -------------------------------------------------------------------------- */
/* Section MESHWHEELS                                                         */
/* -------------------------------------------------------------------------- */

struct MeshWheel: BaseWheel
{
    MeshWheel():
        side(SIDE_INVALID),
        rim_radius(0),
        tyre_radius(0),
        _is_meshwheel2(false)
    {}

    enum Side
    {
        SIDE_INVALID   = 0,
        SIDE_RIGHT     = 'r',
        SIDE_LEFT      = 'l'
    };

    Side side;
    Ogre::String mesh_name;
    Ogre::String material_name;
    float rim_radius;
    float tyre_radius;
    float spring;
    float damping;
    bool _is_meshwheel2;
};

/* -------------------------------------------------------------------------- */
/* Section FLARES FLARES2                                                     */
/* -------------------------------------------------------------------------- */

/** Used for both 'flares' and 'flares2' sections
*/
struct Flare2
{
    Flare2():
        offset(0, 0, 1), /* Section 'flares(1)' has offset.z hardcoded to 1 */
        type(RoR::FlareType::HEADLIGHT),
        control_number(-1),
        blink_delay_milis(-2),
        size(-1)
    {}

    Node::Ref reference_node;
    Node::Ref node_axis_x;
    Node::Ref node_axis_y;
    Ogre::Vector3 offset;
    RoR::FlareType type;
    int control_number; //!< Only 'u' type flares.
    std::string dashboard_link; //!< Only 'd' type flares.
    int blink_delay_milis;
    float size;
    Ogre::String material_name;
};

/* -------------------------------------------------------------------------- */
/* Section FLEXBODIES                                                         */
/* -------------------------------------------------------------------------- */

struct Flexbody
{
    Flexbody():
        offset(Ogre::Vector3::ZERO),
        rotation(Ogre::Vector3::ZERO)
    {
    }

    Node::Ref reference_node;
    Node::Ref x_axis_node;
    Node::Ref y_axis_node;
    Ogre::Vector3 offset;
    Ogre::Vector3 rotation;
    Ogre::String mesh_name;
    std::list<Animation> animations;
    std::vector<Node::Range> node_list_to_import; //!< Node ranges are disallowed in fileformatversion >=450
    std::vector<Node::Ref> node_list;
    CameraSettings camera_settings;
};

/* -------------------------------------------------------------------------- */
/* Section FLEX_BODY_WHEELS                                                   */
/* -------------------------------------------------------------------------- */

struct FlexBodyWheel: BaseWheel2
{
    FlexBodyWheel():
        side(MeshWheel::SIDE_INVALID),
        rim_springiness(0),
        rim_damping(0)
    {}

    MeshWheel::Side side;

    float rim_springiness;
    float rim_damping;
    Ogre::String rim_mesh_name;
    Ogre::String tyre_mesh_name;
};

/* -------------------------------------------------------------------------- */
/* Section FUSEDRAG                                                           */
/* -------------------------------------------------------------------------- */

struct Fusedrag
{
    Fusedrag();

    bool autocalc;
    Node::Ref front_node;
    Node::Ref rear_node;
    float approximate_width;
    Ogre::String airfoil_name;
    float area_coefficient;
};

/* -------------------------------------------------------------------------- */
/* Section HOOKS                                                              */
/* -------------------------------------------------------------------------- */

struct Hook
{
    Hook();

    Node::Ref node;
    float option_hook_range;
    float option_speed_coef;
    float option_max_force;
    int option_hookgroup;
    int option_lockgroup;
    float option_timer;
    float option_min_range_meters;
    bool flag_self_lock :1;
    bool flag_auto_lock :1;
    bool flag_no_disable:1;
    bool flag_no_rope   :1;
    bool flag_visible   :1;
};

/* -------------------------------------------------------------------------- */
/* Section SHOCKS                                                             */
/* -------------------------------------------------------------------------- */

struct Shock
{
    Shock();

    BITMASK_PROPERTY(options, 1, OPTION_i_INVISIBLE    , HasOption_i_Invisible,   SetOption_i_Invisible) 
    // Stability active suspension can be made with "L" for suspension on the truck's left and "R" for suspension on the truck's right. 
    BITMASK_PROPERTY(options, 2, OPTION_L_ACTIVE_LEFT  , HasOption_L_ActiveLeft,  SetOption_L_ActiveLeft) 
    // Stability active suspension can be made with "L" for suspension on the truck's left and "R" for suspension on the truck's right. 
    BITMASK_PROPERTY(options, 3, OPTION_R_ACTIVE_RIGHT , HasOption_R_ActiveRight, SetOption_R_ActiveRight)
    BITMASK_PROPERTY(options, 4, OPTION_m_METRIC       , HasOption_m_Metric,      SetOption_m_Metric) 

    Node::Ref nodes[2];
    float spring_rate;         //!< The 'stiffness' of the shock. The higher the value, the less the shock will move for a given bump. 
    float damping;             //!< The 'resistance to motion' of the shock. The best value is given by this equation:  2 * sqrt(suspended mass * springness)
    float short_bound;         //!< Maximum contraction. The shortest length the shock can be, as a proportion of its original length. "0" means the shock will not be able to contract at all, "1" will let it contract all the way to zero length. If the shock tries to shorten more than this value allows, it will become as rigid as a normal beam. 
    float long_bound;          //!< Maximum extension. The longest length a shock can be, as a proportion of its original length. "0" means the shock will not be able to extend at all. "1" means the shock will be able to double its length. Higher values allow for longer extension.
    float precompression;      //!< Changes compression or extension of the suspension when the truck spawns. This can be used to "level" the suspension of a truck if it sags in game. The default value is 1.0. 
    unsigned int options;      //!< Bit flags.
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group;
};

/* -------------------------------------------------------------------------- */
/* Section SHOCKS_2                                                           */
/* -------------------------------------------------------------------------- */

struct Shock2
{
    Shock2();

    BITMASK_PROPERTY(options, 1, OPTION_i_INVISIBLE       , HasOption_i_Invisible,      SetOption_i_Invisible) 
    // soft bump boundaries, use when shocks reach limiters too often and "jumprebound" (default is hard bump boundaries)
    BITMASK_PROPERTY(options, 2, OPTION_s_SOFT_BUMP_BOUNDS, HasOption_s_SoftBumpBounds, SetOption_s_SoftBumpBounds)
    // metric values for shortbound/longbound applying to the length of the beam.
    BITMASK_PROPERTY(options, 3, OPTION_m_METRIC          , HasOption_m_Metric,         SetOption_m_Metric)
    // Absolute metric values for shortbound/longbound, settings apply without regarding to the original length of the beam.(Use with caution, check ror.log for errors)
    BITMASK_PROPERTY(options, 4, OPTION_M_ABSOLUTE_METRIC , HasOption_M_AbsoluteMetric, SetOption_M_AbsoluteMetric)  

    Node::Ref nodes[2];
    float spring_in;                  //!< Spring value applied when the shock is compressing.
    float damp_in;                    //!< Damping value applied when the shock is compressing. 
    float progress_factor_spring_in;  //!< Progression factor for springin. A value of 0 disables this option. 1...x as multipliers, example:maximum springrate == springrate + (factor*springrate)
    float progress_factor_damp_in;    //!< Progression factor for dampin. 0 = disabled, 1...x as multipliers, example:maximum dampingrate == springrate + (factor*dampingrate)
    float spring_out;                 //!< spring value applied when shock extending
    float damp_out;                   //!< damping value applied when shock extending
    float progress_factor_spring_out; //!< Progression factor springout, 0 = disabled, 1...x as multipliers, example:maximum springrate == springrate + (factor*springrate)
    float progress_factor_damp_out;   //!< Progression factor dampout, 0 = disabled, 1...x as multipliers, example:maximum dampingrate == springrate + (factor*dampingrate)
    float short_bound;                //!< Maximum contraction limit, in percentage ( 1.00 = 100% )
    float long_bound;                 //!< Maximum extension limit, in percentage ( 1.00 = 100% )
    float precompression;             //!< Changes compression or extension of the suspension when the truck spawns. This can be used to "level" the suspension of a truck if it sags in game. The default value is 1.0.  
    unsigned int options;             //!< Bit flags.
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group;
};

/* -------------------------------------------------------------------------- */
/* Section SHOCKS_2                                                           */
/* -------------------------------------------------------------------------- */

struct Shock3
{
    Shock3();

    BITMASK_PROPERTY(options, 1, OPTION_i_INVISIBLE       , HasOption_i_Invisible,      SetOption_i_Invisible) 
    // metric values for shortbound/longbound applying to the length of the beam.
    BITMASK_PROPERTY(options, 2, OPTION_m_METRIC          , HasOption_m_Metric,         SetOption_m_Metric)
    // Absolute metric values for shortbound/longbound, settings apply without regarding to the original length of the beam.(Use with caution, check ror.log for errors)
    BITMASK_PROPERTY(options, 3, OPTION_M_ABSOLUTE_METRIC , HasOption_M_AbsoluteMetric, SetOption_M_AbsoluteMetric)  

    Node::Ref nodes[2];
    float spring_in;                  //!< Spring value applied when the shock is compressing.
    float damp_in;                    //!< Damping value applied when the shock is compressing. 
    float spring_out;                 //!< Spring value applied when shock extending
    float damp_out;                   //!< Damping value applied when shock extending
    float damp_in_slow;               //!< Damping value applied when shock is commpressing slower than split in velocity
    float split_vel_in;               //!< Split velocity in (m/s) - threshold for slow / fast damping during compression
    float damp_in_fast;               //!< Damping value applied when shock is commpressing faster than split in velocity
    float damp_out_slow;              //!< Damping value applied when shock is commpressing slower than split out velocity
    float split_vel_out;              //!< Split velocity in (m/s) - threshold for slow / fast damping during extension
    float damp_out_fast;              //!< Damping value applied when shock is commpressing faster than split out velocity
    float short_bound;                //!< Maximum contraction limit, in percentage ( 1.00 = 100% )
    float long_bound;                 //!< Maximum extension limit, in percentage ( 1.00 = 100% )
    float precompression;             //!< Changes compression or extension of the suspension when the truck spawns. This can be used to "level" the suspension of a truck if it sags in game. The default value is 1.0.  
    unsigned int options;             //!< Bit flags.
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group;
};

/* -------------------------------------------------------------------------- */
/* Inline-section SET_SKELETON_DISPLAY                                        */
/* -------------------------------------------------------------------------- */

struct SkeletonSettings
{
    SkeletonSettings():
        visibility_range_meters(150.f),
        beam_thickness_meters(BEAM_SKELETON_DIAMETER)
    {}

    float visibility_range_meters;
    float beam_thickness_meters;
};

/* -------------------------------------------------------------------------- */
/* Section HYDROS                                                             */
/* -------------------------------------------------------------------------- */

struct Hydro
{
    Hydro():
        lenghtening_factor(0),
        detacher_group(0)
    {}

    static const char OPTION_n_NORMAL                    = 'n';
    static const char OPTION_i_INVISIBLE                 = 'i';

    /* Useful for trucks */

    static const char OPTION_s_DISABLE_ON_HIGH_SPEED     = 's';

    /* Useful for planes: These can be used to control flight surfaces, or to create a thrust vectoring system.  */

    static const char OPTION_a_INPUT_AILERON             = 'a';
    static const char OPTION_r_INPUT_RUDDER              = 'r';
    static const char OPTION_e_INPUT_ELEVATOR            = 'e';
    static const char OPTION_u_INPUT_AILERON_ELEVATOR    = 'u';
    static const char OPTION_v_INPUT_InvAILERON_ELEVATOR = 'v';
    static const char OPTION_x_INPUT_AILERON_RUDDER      = 'x';
    static const char OPTION_y_INPUT_InvAILERON_RUDDER   = 'y';
    static const char OPTION_g_INPUT_ELEVATOR_RUDDER     = 'g';
    static const char OPTION_h_INPUT_InvELEVATOR_RUDDER  = 'h';

    inline bool HasFlag_a() { return options.find(RigDef::Hydro::OPTION_a_INPUT_AILERON)             != std::string::npos; }
    inline bool HasFlag_e() { return options.find(RigDef::Hydro::OPTION_e_INPUT_ELEVATOR)            != std::string::npos; }
    inline bool HasFlag_g() { return options.find(RigDef::Hydro::OPTION_g_INPUT_ELEVATOR_RUDDER)     != std::string::npos; }
    inline bool HasFlag_h() { return options.find(RigDef::Hydro::OPTION_h_INPUT_InvELEVATOR_RUDDER)  != std::string::npos; }
    inline bool HasFlag_i() { return options.find(RigDef::Hydro::OPTION_i_INVISIBLE)                 != std::string::npos; }
    inline bool HasFlag_r() { return options.find(RigDef::Hydro::OPTION_r_INPUT_RUDDER)              != std::string::npos; }
    inline bool HasFlag_s() { return options.find(RigDef::Hydro::OPTION_s_DISABLE_ON_HIGH_SPEED)     != std::string::npos; }
    inline bool HasFlag_u() { return options.find(RigDef::Hydro::OPTION_u_INPUT_AILERON_ELEVATOR)    != std::string::npos; }
    inline bool HasFlag_v() { return options.find(RigDef::Hydro::OPTION_v_INPUT_InvAILERON_ELEVATOR) != std::string::npos; }
    inline bool HasFlag_x() { return options.find(RigDef::Hydro::OPTION_x_INPUT_AILERON_RUDDER)      != std::string::npos; }
    inline bool HasFlag_y() { return options.find(RigDef::Hydro::OPTION_y_INPUT_InvAILERON_RUDDER)   != std::string::npos; }

    inline void AddFlag(char flag) { options += flag; }

    Node::Ref nodes[2];
    float lenghtening_factor;
    std::string options;
    Inertia inertia;
    std::shared_ptr<Inertia> inertia_defaults;
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group;
};

/* -------------------------------------------------------------------------- */
/* Section ANIMATORS                                                          */
/* -------------------------------------------------------------------------- */

struct AeroAnimator
{
    static const BitMask_t OPTION_THROTTLE = BITMASK(1);
    static const BitMask_t OPTION_RPM      = BITMASK(2);
    static const BitMask_t OPTION_TORQUE   = BITMASK(3);
    static const BitMask_t OPTION_PITCH    = BITMASK(4);
    static const BitMask_t OPTION_STATUS   = BITMASK(5);

    BitMask_t flags      = 0u;
    unsigned int engine_idx = 0u;
};

struct Animator
{
    static const BitMask_t OPTION_VISIBLE           = BITMASK( 1);
    static const BitMask_t OPTION_INVISIBLE         = BITMASK( 2);
    static const BitMask_t OPTION_AIRSPEED          = BITMASK( 3);
    static const BitMask_t OPTION_VERTICAL_VELOCITY = BITMASK( 4);
    static const BitMask_t OPTION_ALTIMETER_100K    = BITMASK( 5);
    static const BitMask_t OPTION_ALTIMETER_10K     = BITMASK( 6);
    static const BitMask_t OPTION_ALTIMETER_1K      = BITMASK( 7);
    static const BitMask_t OPTION_ANGLE_OF_ATTACK   = BITMASK( 8);
    static const BitMask_t OPTION_FLAP              = BITMASK( 9);
    static const BitMask_t OPTION_AIR_BRAKE         = BITMASK(10);
    static const BitMask_t OPTION_ROLL              = BITMASK(11);
    static const BitMask_t OPTION_PITCH             = BITMASK(12);
    static const BitMask_t OPTION_BRAKES            = BITMASK(13);
    static const BitMask_t OPTION_ACCEL             = BITMASK(14);
    static const BitMask_t OPTION_CLUTCH            = BITMASK(15);
    static const BitMask_t OPTION_SPEEDO            = BITMASK(16);
    static const BitMask_t OPTION_TACHO             = BITMASK(17);
    static const BitMask_t OPTION_TURBO             = BITMASK(18);
    static const BitMask_t OPTION_PARKING           = BITMASK(19);
    static const BitMask_t OPTION_SHIFT_LEFT_RIGHT  = BITMASK(20);
    static const BitMask_t OPTION_SHIFT_BACK_FORTH  = BITMASK(21);
    static const BitMask_t OPTION_SEQUENTIAL_SHIFT  = BITMASK(22);
    static const BitMask_t OPTION_GEAR_SELECT       = BITMASK(23);
    static const BitMask_t OPTION_TORQUE            = BITMASK(24);
    static const BitMask_t OPTION_DIFFLOCK          = BITMASK(25);
    static const BitMask_t OPTION_BOAT_RUDDER       = BITMASK(26);
    static const BitMask_t OPTION_BOAT_THROTTLE     = BITMASK(27);
    static const BitMask_t OPTION_SHORT_LIMIT       = BITMASK(28);
    static const BitMask_t OPTION_LONG_LIMIT        = BITMASK(29);

    Node::Ref nodes[2];
    float lenghtening_factor = 0.f;
    BitMask_t flags = 0;
    float short_limit = 0.f;
    float long_limit = 0.f;
    AeroAnimator aero_animator;
    std::shared_ptr<Inertia> inertia_defaults;
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group = 0;
};

/* -------------------------------------------------------------------------- */
/* Section COMMANDS & COMMANDS_2 (unified)                                    */
/* -------------------------------------------------------------------------- */

struct Command2
{
    Command2();

    unsigned int _format_version;
    Node::Ref nodes[2];
    float shorten_rate;
    float lengthen_rate;
    float max_contraction;
    float max_extension;
    unsigned int contract_key;
    unsigned int extend_key;
    Ogre::String description;
    Inertia inertia;
    float affect_engine;
    bool needs_engine;
    bool plays_sound;
    std::shared_ptr<BeamDefaults> beam_defaults;
    std::shared_ptr<Inertia> inertia_defaults;
    int detacher_group;

    bool option_i_invisible;
    bool option_r_rope;
    bool option_c_auto_center;
    bool option_f_not_faster;
    bool option_p_1press;
    bool option_o_1press_center;
};

/* -------------------------------------------------------------------------- */
/* Section ROTATORS                                                           */
/* -------------------------------------------------------------------------- */

struct Rotator
{
    Rotator():
        rate(0),
        spin_left_key(0),
        spin_right_key(0),
        engine_coupling(1), /* Default */
        needs_engine(false) /* Default */
    {}

    Node::Ref axis_nodes[2];
    Node::Ref base_plate_nodes[4];
    Node::Ref rotating_plate_nodes[4];

    float rate;
    unsigned int spin_left_key;
    unsigned int spin_right_key;
    Inertia inertia;
    std::shared_ptr<Inertia> inertia_defaults;
    float engine_coupling;
    bool needs_engine;
};

/* -------------------------------------------------------------------------- */
/* Section ROTATORS_2                                                         */
/* -------------------------------------------------------------------------- */

struct Rotator2: public Rotator
{
    Rotator2():
        Rotator(),
        rotating_force(10000000), /* Default */
        tolerance(0)
    {}

    float rotating_force;
    float tolerance;
    Ogre::String description;
};

/* -------------------------------------------------------------------------- */
/* Section TRIGGERS                                                           */
/* -------------------------------------------------------------------------- */

struct Trigger
{
    static const BitMask_t OPTION_i_INVISIBLE             = BITMASK( 1);
    static const BitMask_t OPTION_c_COMMAND_STYLE         = BITMASK( 2);
    static const BitMask_t OPTION_x_START_DISABLED        = BITMASK( 3);
    static const BitMask_t OPTION_b_KEY_BLOCKER           = BITMASK( 4);
    static const BitMask_t OPTION_B_TRIGGER_BLOCKER       = BITMASK( 5);
    static const BitMask_t OPTION_A_INV_TRIGGER_BLOCKER   = BITMASK( 6);
    static const BitMask_t OPTION_s_CMD_NUM_SWITCH        = BITMASK( 7);
    static const BitMask_t OPTION_h_UNLOCKS_HOOK_GROUP    = BITMASK( 8);
    static const BitMask_t OPTION_H_LOCKS_HOOK_GROUP      = BITMASK( 9);
    static const BitMask_t OPTION_t_CONTINUOUS            = BITMASK(10);
    static const BitMask_t OPTION_E_ENGINE_TRIGGER        = BITMASK(11);

    // TODO:  All the following structs+functions should really be spawner helpers

    struct EngineTrigger
    {
        enum Function
        {
            ENGINE_TRIGGER_FUNCTION_CLUTCH      = 0,
            ENGINE_TRIGGER_FUNCTION_BRAKE       = 1,
            ENGINE_TRIGGER_FUNCTION_ACCELERATOR = 2,
            ENGINE_TRIGGER_FUNCTION_RPM_CONTROL = 3,
            ENGINE_TRIGGER_FUNCTION_SHIFT_UP    = 4, //!< Do not mix with OPTION_t_CONTINUOUS
            ENGINE_TRIGGER_FUNCTION_SHIFT_DOWN  = 5, //!< Do not mix with OPTION_t_CONTINUOUS

            ENGINE_TRIGGER_FUNCTION_INVALID     = 0xFFFFFFFF
        };

        Function function;
        unsigned int motor_index;
    };

    struct CommandKeyTrigger
    {
        unsigned int contraction_trigger_key;
        unsigned int extension_trigger_key;
    };

    struct HookToggleTrigger
    {
        int contraction_trigger_hookgroup_id;
        int extension_trigger_hookgroup_id;
    };

    inline bool IsHookToggleTrigger() const
    {
        return BITMASK_IS_1(options, OPTION_H_LOCKS_HOOK_GROUP) ||
               BITMASK_IS_1(options, OPTION_h_UNLOCKS_HOOK_GROUP);
    }

    inline bool IsTriggerBlockerAnyType() const
    {
        return BITMASK_IS_1(options, OPTION_B_TRIGGER_BLOCKER) ||
               BITMASK_IS_1(options, OPTION_A_INV_TRIGGER_BLOCKER);
    }

    inline void SetEngineTrigger(Trigger::EngineTrigger const & trig)
    {
        shortbound_trigger_action = (int) trig.function;
        longbound_trigger_action = (int) trig.motor_index;
    }

    inline Trigger::EngineTrigger GetEngineTrigger() const
    {
        ROR_ASSERT(BITMASK_IS_1(options, OPTION_E_ENGINE_TRIGGER));
        EngineTrigger trig;
        trig.function = static_cast<EngineTrigger::Function>(shortbound_trigger_action);
        trig.motor_index = static_cast<unsigned int>(longbound_trigger_action);
        return trig;
    }

    inline void SetCommandKeyTrigger(CommandKeyTrigger const & trig)
    {
        shortbound_trigger_action = (int) trig.contraction_trigger_key;
        longbound_trigger_action = (int) trig.extension_trigger_key;
    }

    inline CommandKeyTrigger GetCommandKeyTrigger() const
    {
        ROR_ASSERT(BITMASK_IS_0(options, OPTION_B_TRIGGER_BLOCKER | OPTION_A_INV_TRIGGER_BLOCKER 
            | OPTION_h_UNLOCKS_HOOK_GROUP | OPTION_H_LOCKS_HOOK_GROUP | OPTION_E_ENGINE_TRIGGER));
        CommandKeyTrigger out;
        out.contraction_trigger_key = static_cast<unsigned int>(shortbound_trigger_action);
        out.extension_trigger_key = static_cast<unsigned int>(longbound_trigger_action);
        return out;
    }

    inline void SetHookToggleTrigger(HookToggleTrigger const & trig)
    {
        shortbound_trigger_action = trig.contraction_trigger_hookgroup_id;
        longbound_trigger_action = trig.extension_trigger_hookgroup_id;
    }

    inline HookToggleTrigger GetHookToggleTrigger() const
    {
        ROR_ASSERT(this->IsTriggerBlockerAnyType());
        HookToggleTrigger trig;
        trig.contraction_trigger_hookgroup_id = shortbound_trigger_action;
        trig.extension_trigger_hookgroup_id = longbound_trigger_action;
        return trig;
    }

    Node::Ref nodes[2];
    float contraction_trigger_limit = 0.f;
    float expansion_trigger_limit = 0.f;
    BitMask_t options = 0;
    float boundary_timer = 1.f;
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group = 0;
    int shortbound_trigger_action = 0;
    int longbound_trigger_action = 0;
};

/* -------------------------------------------------------------------------- */
/* Section LOCKGROUPS                                                         */
/* -------------------------------------------------------------------------- */

struct Lockgroup
{
    Lockgroup()
        :	number(0)
    {
        nodes.reserve(20);
    }

    static const int LOCKGROUP_DEFAULT        = -1;
    static const int LOCKGROUP_NOLOCK         = 9999;

    int number;
    std::vector<Node::Ref> nodes;
};

/* -------------------------------------------------------------------------- */
/* Section MANAGEDMATERIALS                                                   */
/* -------------------------------------------------------------------------- */

struct ManagedMaterial
{
    /* IMPORTANT! Order of these values must match Regexes::IDENTIFY_MANAGED_MATERIAL_TYPE enum from Regexes.h */
    enum Type
    {
        TYPE_FLEXMESH_STANDARD = 1,
        TYPE_FLEXMESH_TRANSPARENT,
        TYPE_MESH_STANDARD,
        TYPE_MESH_TRANSPARENT,

        TYPE_INVALID = 0xFFFFFFFF
    };

    Ogre::String name;
    
    /* Attributes */
    Type type;
    ManagedMaterialsOptions options;
    
    /* Textures */
    Ogre::String diffuse_map;
    Ogre::String damaged_diffuse_map;
    Ogre::String specular_map;

    bool HasDamagedDiffuseMap()
    {
        return (damaged_diffuse_map.length() != 0 && damaged_diffuse_map[0] != '-');
    }

    bool HasSpecularMap()
    {
        return (specular_map.length() != 0 && specular_map[0] != '-');
    }
};

/* -------------------------------------------------------------------------- */
/* Section MATERIALFLAREBINDINGS                                              */
/* -------------------------------------------------------------------------- */

struct MaterialFlareBinding
{
    MaterialFlareBinding():
        flare_number(0)
    {}

    unsigned int flare_number;
    Ogre::String material_name;
};

/* -------------------------------------------------------------------------- */
/* Section MINIMASS                                                           */
/* -------------------------------------------------------------------------- */

struct Minimass
{
    float global_min_mass_Kg; //!< minimum node mass in Kg - only effective where DefaultMinimass was not set.
    MinimassOption option = MinimassOption::n_DUMMY;
};

/* -------------------------------------------------------------------------- */
/* Section PARTICLES                                                          */
/* -------------------------------------------------------------------------- */

struct Particle
{
    Node::Ref emitter_node;
    Node::Ref reference_node;
    Ogre::String particle_system_name;
};

/* -------------------------------------------------------------------------- */
/* Section PISTONPROPS                                                        */
/* -------------------------------------------------------------------------- */

struct Pistonprop
{
    Pistonprop():
        turbine_power_kW(0),
        pitch(0)
    {}

    Node::Ref reference_node;
    Node::Ref axis_node;
    Node::Ref blade_tip_nodes[4];
    Node::Ref couple_node;
    float turbine_power_kW;
    float pitch;
    Ogre::String airfoil;
};

/* -------------------------------------------------------------------------- */
/* Section PROPS                                                              */
/* -------------------------------------------------------------------------- */

struct Prop
{
    struct DashboardSpecial
    {
        DashboardSpecial():
            offset(Ogre::Vector3::ZERO), // Default depends on right/left-hand dashboard placement
            rotation_angle(160.f),
            _offset_is_set(false),
            mesh_name("dirwheel.mesh")
        {}

        Ogre::Vector3 offset;
        bool _offset_is_set;
        float rotation_angle;
        Ogre::String mesh_name;
    };

    struct BeaconSpecial
    {
        BeaconSpecial():
            color(1.0, 0.5, 0.0),
            flare_material_name("tracks/beaconflare")
        {}

        Ogre::String flare_material_name;
        Ogre::ColourValue color;
    };

    Prop():
        offset(Ogre::Vector3::ZERO),
        rotation(Ogre::Vector3::ZERO),
        special(SPECIAL_INVALID)
    {}

    /* IMPORTANT! Values must match results from Regexes::SPECIAL_PROPS */
    enum Special
    {
        SPECIAL_MIRROR_LEFT = 1,
        SPECIAL_MIRROR_RIGHT,
        SPECIAL_DASHBOARD_LEFT,
        SPECIAL_DASHBOARD_RIGHT,
        SPECIAL_AERO_PROP_SPIN,
        SPECIAL_AERO_PROP_BLADE,
        SPECIAL_DRIVER_SEAT,
        SPECIAL_DRIVER_SEAT_2,
        SPECIAL_BEACON,
        SPECIAL_REDBEACON,
        SPECIAL_LIGHTBAR,

        SPECIAL_INVALID = 0xFFFFFFFF
    };

    Node::Ref reference_node;
    Node::Ref x_axis_node;
    Node::Ref y_axis_node;
    Ogre::Vector3 offset;
    Ogre::Vector3 rotation;
    Ogre::String mesh_name;
    std::list<Animation> animations;
    CameraSettings camera_settings;
    Special special;
    BeaconSpecial special_prop_beacon;
    DashboardSpecial special_prop_dashboard;
};

/* -------------------------------------------------------------------------- */
/* Section RAILGROUPS                                                         */
/* -------------------------------------------------------------------------- */

struct RailGroup
{
    RailGroup():
        id(0)
    {}

    unsigned int id;
    std::vector<Node::Range> node_list;
};

/* -------------------------------------------------------------------------- */
/* Section Ropables                                                           */
/* -------------------------------------------------------------------------- */

struct Ropable
{
    Ropable():
        group(-1), // = value not set
        has_multilock(false)
    {}

    Node::Ref node;
    int group;
    bool has_multilock;
};

/* -------------------------------------------------------------------------- */
/* Section ROPES                                                              */
/* -------------------------------------------------------------------------- */

struct Rope
{
    Rope():
        invisible(false),
        detacher_group(0) /* Global detacher group */
    {}

    Node::Ref root_node;
    Node::Ref end_node;
    bool invisible;
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group;
};

/* -------------------------------------------------------------------------- */
/* Section SCREWPROPS                                                         */
/* -------------------------------------------------------------------------- */

struct Screwprop
{
    Screwprop():
        power(0)
    {}

    Node::Ref prop_node;
    Node::Ref back_node;
    Node::Ref top_node;
    float power;
};

/* -------------------------------------------------------------------------- */
/* Section SLIDENODES                                                         */
/* -------------------------------------------------------------------------- */

struct SlideNode
{
    static const BitMask_t CONSTRAINT_ATTACH_ALL     = BITMASK(1);
    static const BitMask_t CONSTRAINT_ATTACH_FOREIGN = BITMASK(2);
    static const BitMask_t CONSTRAINT_ATTACH_SELF    = BITMASK(3);
    static const BitMask_t CONSTRAINT_ATTACH_NONE    = BITMASK(4);

    Node::Ref slide_node;
    std::vector<Node::Range> rail_node_ranges;
    BitMask_t constraint_flags = 0;

    // Optional args
    float spring_rate=0.f;      bool _spring_rate_set=false;
    float break_force=0.f;      bool _break_force_set=false;
    float tolerance=0.f;        bool _tolerance_set=false;
    float attachment_rate=0.f;  bool _attachment_rate_set=false;
    int   railgroup_id=-1;      bool _railgroup_id_set=false;
    float max_attach_dist=0.f;  bool _max_attach_dist_set=false;
};

/* -------------------------------------------------------------------------- */
/* Section SOUNDSOURCES                                                       */
/* -------------------------------------------------------------------------- */

struct SoundSource
{
    Node::Ref node;
    Ogre::String sound_script_name;
};

/* -------------------------------------------------------------------------- */
/* Section SOUNDSOURCES2                                                      */
/* -------------------------------------------------------------------------- */

struct SoundSource2: SoundSource
{
    enum Mode
    {
        MODE_ALWAYS  = -2,
        MODE_OUTSIDE = -1,
        MODE_CINECAM = 1,

        MODE_INVALID = 0xFFFFFFFF
    };

    SoundSource2():
        mode(MODE_INVALID),
        cinecam_index(0)
    {}

    Mode mode;
    unsigned int cinecam_index;
};

/* -------------------------------------------------------------------------- */
/* Section SPEEDLIMITER                                                       */
/* -------------------------------------------------------------------------- */

struct SpeedLimiter
{
    SpeedLimiter():
        max_speed(0.f),
        is_enabled(false)
    {}

    float max_speed;
    bool is_enabled;
};

/* -------------------------------------------------------------------------- */
/* Section SUBMESH                                                            */
/* -------------------------------------------------------------------------- */

struct Cab
{
    static const BitMask_t OPTION_c_CONTACT              = BITMASK(1);
    static const BitMask_t OPTION_b_BUOYANT              = BITMASK(2);
    static const BitMask_t OPTION_p_10xTOUGHER           = BITMASK(3);
    static const BitMask_t OPTION_u_INVULNERABLE         = BITMASK(4);
    static const BitMask_t OPTION_s_BUOYANT_NO_DRAG      = BITMASK(5);
    static const BitMask_t OPTION_r_BUOYANT_ONLY_DRAG    = BITMASK(6);
    static const BitMask_t OPTION_D_CONTACT_BUOYANT      = OPTION_c_CONTACT | OPTION_b_BUOYANT;
    static const BitMask_t OPTION_F_10xTOUGHER_BUOYANT   = OPTION_p_10xTOUGHER | OPTION_b_BUOYANT;
    static const BitMask_t OPTION_S_INVULNERABLE_BUOYANT = OPTION_u_INVULNERABLE | OPTION_b_BUOYANT;

    Node::Ref nodes[3];
    BitMask_t options = 0;
};

struct Texcoord
{
    Texcoord():
        u(0),
        v(0)
    {}

    Node::Ref node;
    float u;
    float v;
};

struct Submesh
{
    Submesh():
        backmesh(false)
    {}

    bool backmesh;
    std::vector<Texcoord> texcoords;
    std::vector<Cab> cab_triangles;
};

/* -------------------------------------------------------------------------- */
/* Section TIES                                                               */
/* -------------------------------------------------------------------------- */

struct Tie
{
    static const BitMask_t OPTION_i_INVISIBLE         = BITMASK(1);
    static const BitMask_t OPTION_s_DISABLE_SELF_LOCK = BITMASK(2);

    Node::Ref root_node;
    float max_reach_length = 0.f;
    float auto_shorten_rate = 0.f;
    float min_length = 0.f;
    float max_length = 0.f;
    BitMask_t options = 0;
    float max_stress = 100000.0f;
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group = 0;
    int group = -1;
};

/* -------------------------------------------------------------------------- */
/* Section TORQUECURVE                                                        */
/* -------------------------------------------------------------------------- */

struct TorqueCurve
{
    TorqueCurve()
    {}

    struct Sample
    {
        Sample():
            power(0),
            torque_percent(0)
        {}

        float power;
        float torque_percent;
    };

    std::vector<Sample> samples;
    Ogre::String predefined_func_name;
};

/* -------------------------------------------------------------------------- */
/* Section TURBOJETS                                                          */
/* -------------------------------------------------------------------------- */

struct Turbojet
{
    Turbojet():
        is_reversable(0),
        dry_thrust(0),
        wet_thrust(0),
        front_diameter(0),
        back_diameter(0),
        nozzle_length(0)
    {}

    Node::Ref front_node;
    Node::Ref back_node;
    Node::Ref side_node;
    int is_reversable;
    float dry_thrust;
    float wet_thrust;
    float front_diameter;
    float back_diameter;
    float nozzle_length;
};

/* -------------------------------------------------------------------------- */
/* Section TURBOPROPS, TURBOPROPS2                                            */
/* -------------------------------------------------------------------------- */

struct Turboprop2
{
    Turboprop2():
        turbine_power_kW(0),
        _format_version(2)
    {}

    Node::Ref reference_node;
    Node::Ref axis_node;
    Node::Ref blade_tip_nodes[4];
    float turbine_power_kW;
    Ogre::String airfoil;
    Node::Ref couple_node;
    unsigned int _format_version;
};

/* -------------------------------------------------------------------------- */
/* Section VIDEOCAMERA                                                        */
/* -------------------------------------------------------------------------- */

struct VideoCamera
{
    VideoCamera();

    Node::Ref reference_node;
    Node::Ref left_node;
    Node::Ref bottom_node;
    Node::Ref alt_reference_node;
    Node::Ref alt_orientation_node;
    Ogre::Vector3 offset;
    Ogre::Vector3 rotation;
    float field_of_view;
    unsigned int texture_width;
    unsigned int texture_height;
    float min_clip_distance;
    float max_clip_distance;
    int camera_role;
    int camera_mode;
    Ogre::String material_name;
    Ogre::String camera_name;
};

/* -------------------------------------------------------------------------- */
/* Section WINGS                                                              */
/* -------------------------------------------------------------------------- */

struct Wing
{
    Node::Ref nodes[8];
    float tex_coords[8] = {};
    WingControlSurface control_surface = WingControlSurface::n_NONE;
    float chord_point = -1.f;
    float min_deflection = -1.f;
    float max_deflection = -1.f;
    Ogre::String airfoil;
    float efficacy_coef = 1.f; // So-called 'liftcoef'.
};

/* -------------------------------------------------------------------------- */
/* Root document                                                              */
/* -------------------------------------------------------------------------- */

struct File
{
    struct Module
    {
        Module(Ogre::String const & name);

        Ogre::String name;

        std::vector<Airbrake>              airbrakes;
        std::vector<Animator>              animators;
        std::vector<AntiLockBrakes>        antilockbrakes;
        std::vector<Author>                author;
        std::vector<Axle>                  axles;
        std::vector<Beam>                  beams;
        std::vector<Brakes>                brakes;
        std::vector<Camera>                cameras;
        std::vector<CameraRail>            camerarail;
        std::vector<CollisionBox>          collisionboxes;
        std::vector<Cinecam>               cinecam;
        std::vector<Command2>              commands2; // 'commands' are auto-imported as 'commands2' (only 1 extra argument)
        std::vector<CruiseControl>         cruisecontrol;
        std::vector<Node::Ref>             contacters;
        std::vector<Ogre::String>          description;
        std::vector<Engine>                engine;
        std::vector<Engoption>             engoption;
        std::vector<Engturbo>              engturbo;
        std::vector<Exhaust>               exhausts;
        std::vector<ExtCamera>             extcamera;
        std::vector<FileFormatVersion>     fileformatversion;
        std::vector<Node::Ref>             fixes;
        std::vector<Fileinfo>              fileinfo;
        std::vector<Flare2>                flares2; // 'flares' are auto-imported as 'flares2' (only 1 extra argument)
        std::vector<Flexbody>              flexbodies;
        std::vector<FlexBodyWheel>         flexbodywheels;
        std::vector<Fusedrag>              fusedrag;
        std::vector<Globals>               globals;
        std::vector<Guid>                  guid;
        std::vector<GuiSettings>           guisettings;
        std::vector<Help>                  help;
        std::vector<Hook>                  hooks;
        std::vector<Hydro>                 hydros;
        std::vector<InterAxle>             interaxles;
        std::vector<Lockgroup>             lockgroups;
        std::vector<ManagedMaterial>       managedmaterials;
        std::vector<MaterialFlareBinding>  materialflarebindings;
        std::vector<MeshWheel>             mesh_wheels;
        std::vector<Minimass>              minimass;
        std::vector<Node>                  nodes; /* Nodes and Nodes2 are unified in this parser */
        std::vector<Particle>              particles;
        std::vector<Pistonprop>            pistonprops;
        std::vector<Prop>                  props;
        std::vector<RailGroup>             railgroups;
        std::vector<Ropable>               ropables;
        std::vector<Rope>                  ropes;
        std::vector<Rotator>               rotators;
        std::vector<Rotator2>              rotators2;
        std::vector<Screwprop>             screwprops;
        std::vector<Shock>                 shocks;
        std::vector<Shock2>                shocks2;
        std::vector<Shock3>                shocks3;
        std::vector<CollisionRange>        set_collision_range;
        std::vector<SkeletonSettings>      set_skeleton_settings;
        std::vector<SlideNode>             slidenodes;
        std::vector<SoundSource>           soundsources;
        std::vector<SoundSource2>          soundsources2;
        std::vector<SpeedLimiter>          speedlimiter;
        std::vector<Ogre::String>          submesh_groundmodel;
        std::vector<Submesh>               submeshes;
        std::vector<Tie>                   ties;
        std::vector<TorqueCurve>           torquecurve;
        std::vector<TractionControl>       tractioncontrol;
        std::vector<TransferCase>          transfercase;
        std::vector<Trigger>               triggers;
        std::vector<Turbojet>              turbojets;
        std::vector<Turboprop2>            turboprops2;
        std::vector<VideoCamera>           videocameras;
        std::vector<WheelDetacher>         wheeldetachers;
        std::vector<Wheel>                 wheels;
        std::vector<Wheel2>                wheels_2;
        std::vector<Wing>                  wings;
    };

    File();

    bool hide_in_chooser;
    bool enable_advanced_deformation;
    bool slide_nodes_connect_instantly;
    bool rollon;
    bool forward_commands;
    bool import_commands;
    bool lockgroup_default_nolock;
    bool rescuer;
    bool disable_default_sounds;
    Ogre::String name;

    // File hash
    std::string hash;

    // Vehicle modules (caled 'sections' in truckfile doc)
    std::shared_ptr<Module> root_module; //!< Required to exist. `shared_ptr` is used for unified handling with other modules.
    std::map< Ogre::String, std::shared_ptr<Module> > user_modules;

};

} // namespace RigDef
