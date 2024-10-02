/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2021 Petr Ohlidal

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

    CODING GUIDELINES:
    * each element (line) should have dedicated `struct` for data. Auto-importing to higher version (i.e. 'flares' -> 'flares2') is OK.
    * all data should be stored in `std::vector<>`s.
    * the data vectors should be named exactly as their file format keywords.
    * each option-string should have dedicated `enum class` with fields named '{letter}_{MEANING}'.
    * all bitmasks should use `BitMask_t` data type.
    * all bitmask constants should have format `static const BitMask_t MEANING = BITMASK({number})`.
    * all option strings should have parsing function `GetArgWhatever(int index)`.
    * option-strings should be stored as bitmasks (unless order matters). If only single option is acceptable, use the enum directly.
    * all data structs should contain only arguments, in order. Helper data (where needed) must be prefixed with `_`.
*/

#pragma once

#include "Application.h"
#include "BitFlags.h"
#include "GfxData.h"
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

// --------------------------------
// Enums which only carry value

// IMPORTANT! If you add a value here, you must also modify Regexes::IDENTIFY_KEYWORD, it relies on numeric values of this enum.
enum class Keyword
{
    INVALID = 0,

    ADD_ANIMATION = 1,
    AIRBRAKES,
    ANIMATORS,
    ANTILOCKBRAKES,
    ASSETPACKS,
    AUTHOR,
    AXLES,
    BACKMESH,
    BEAMS,
    BRAKES,
    CAB,
    CAMERARAIL,
    CAMERAS,
    CINECAM,
    COLLISIONBOXES,
    COMMANDS,
    COMMANDS2,
    COMMENT,
    CONTACTERS,
    CRUISECONTROL,
    DEFAULT_SKIN,
    DESCRIPTION,
    DETACHER_GROUP,
    DISABLEDEFAULTSOUNDS,
    ENABLE_ADVANCED_DEFORMATION,
    END,
    END_COMMENT,
    END_DESCRIPTION,
    END_SECTION,
    ENGINE,
    ENGOPTION,
    ENGTURBO,
    ENVMAP,
    EXHAUSTS,
    EXTCAMERA,
    FILEFORMATVERSION,
    FILEINFO,
    FIXES,
    FLARES,
    FLARES2,
    FLARES3,
    FLEXBODIES,
    FLEXBODY_CAMERA_MODE,
    FLEXBODYWHEELS,
    FORSET,
    FORWARDCOMMANDS,
    FUSEDRAG,
    GLOBALS,
    GUID,
    GUISETTINGS,
    HELP,
    HIDEINCHOOSER,
    HOOKGROUP, // obsolete, ignored
    HOOKS,
    HYDROS,
    IMPORTCOMMANDS,
    INTERAXLES,
    LOCKGROUPS,
    LOCKGROUP_DEFAULT_NOLOCK,
    MANAGEDMATERIALS,
    MATERIALFLAREBINDINGS,
    MESHWHEELS,
    MESHWHEELS2,
    MINIMASS,
    NODECOLLISION, // obsolete
    NODES,
    NODES2,
    PARTICLES,
    PISTONPROPS,
    PROP_CAMERA_MODE,
    PROPS,
    RAILGROUPS,
    RESCUER,
    RIGIDIFIERS, // obsolete
    ROLLON,
    ROPABLES,
    ROPES,
    ROTATORS,
    ROTATORS2,
    SCREWPROPS,
    SCRIPTS,
    SECTION,
    SECTIONCONFIG,
    SET_BEAM_DEFAULTS,
    SET_BEAM_DEFAULTS_SCALE,
    SET_COLLISION_RANGE,
    SET_DEFAULT_MINIMASS,
    SET_INERTIA_DEFAULTS,
    SET_MANAGEDMATERIALS_OPTIONS,
    SET_NODE_DEFAULTS,
    SET_SHADOWS,
    SET_SKELETON_SETTINGS,
    SHOCKS,
    SHOCKS2,
    SHOCKS3,
    SLIDENODE_CONNECT_INSTANTLY,
    SLIDENODES,
    SLOPE_BRAKE,
    SOUNDSOURCES,
    SOUNDSOURCES2,
    SPEEDLIMITER,
    SUBMESH,
    SUBMESH_GROUNDMODEL,
    TEXCOORDS,
    TIES,
    TORQUECURVE,
    TRACTIONCONTROL,
    TRANSFERCASE,
    TRIGGERS,
    TURBOJETS,
    TURBOPROPS,
    TURBOPROPS2,
    VIDEOCAMERA,
    WHEELDETACHERS,
    WHEELS,
    WHEELS2,
    WINGS
};

const char* KeywordToString(Keyword keyword);

enum class SpecialProp
{
    NONE,
    MIRROR_LEFT,
    MIRROR_RIGHT,
    DASHBOARD_LEFT,
    DASHBOARD_RIGHT,
    AERO_PROP_SPIN,
    AERO_PROP_BLADE,
    DRIVER_SEAT,
    DRIVER_SEAT_2,
    BEACON,
    REDBEACON,
    LIGHTBAR,
};

enum class ManagedMaterialType
{
    INVALID,
    FLEXMESH_STANDARD,
    FLEXMESH_TRANSPARENT,
    MESH_STANDARD,
    MESH_TRANSPARENT,
};

// --------------------------------
// Enums which specify option letters/numbers and also carry value

enum class EngineType
{
    c_CAR                    = 'c',
    e_ECAR                   = 'e',
    t_TRUCK                  = 't',
};

enum class DifferentialType: char
{
    o_OPEN                   = 'o',
    l_LOCKED                 = 'l',
    s_SPLIT                  = 's',
    v_VISCOUS                = 'v'
};

typedef std::vector<DifferentialType> DifferentialTypeVec;

enum class MinimassOption: char
{
    n_DUMMY                  = 'n',
    l_SKIP_LOADED            = 'l'  //!< Only apply minimum mass to nodes without "L" option.
};

enum class WheelBraking: int
{
    NONE                     = 0,
    FOOT_HAND                = 1,
    FOOT_HAND_SKID_LEFT      = 2,
    FOOT_HAND_SKID_RIGHT     = 3,
    FOOT_ONLY                = 4,
};

enum class WheelPropulsion: int
{
    NONE                     = 0,
    FORWARD                  = 1,
    BACKWARD                 = 2,
};

enum class WingControlSurface: char
{
    n_NONE                   = 'n',
    a_RIGHT_AILERON          = 'a',
    b_LEFT_AILERON           = 'b',
    f_FLAP                   = 'f',
    e_ELEVATOR               = 'e',
    r_RUDDER                 = 'r',
    S_RIGHT_HAND_STABILATOR  = 'S',
    T_LEFT_HAND_STABILATOR   = 'T',
    c_RIGHT_ELEVON           = 'c',
    d_LEFT_ELEVON            = 'd',
    g_RIGHT_FLAPERON         = 'g',
    h_LEFT_FLAPERON          = 'h',
    U_RIGHT_HAND_TAILERON    = 'U',
    V_LEFT_HAND_TAILERON     = 'V',
    i_RIGHT_RUDDERVATOR      = 'i',
    j_LEFT_RUDDERVATOR       = 'j',
};

// --------------------------------
// Enums which only specify flag letters, values are carried by bit masks

enum class TieOption: char
{
    n_DUMMY                  = 'n',
    v_DUMMY                  = 'v',
    i_INVISIBLE              = 'i',
    s_NO_SELF_LOCK           = 's',
};

enum class CabOption: char
{
    c_CONTACT                = 'c',
    b_BUOYANT                = 'b',
    p_10xTOUGHER             = 'p',
    u_INVULNERABLE           = 'u',
    s_BUOYANT_NO_DRAG        = 's',
    r_BUOYANT_ONLY_DRAG      = 'r',
    D_CONTACT_BUOYANT        = 'D',
    F_10xTOUGHER_BUOYANT     = 'F',
    S_INVULNERABLE_BUOYANT   = 'S',
};

enum class TriggerOption: char
{
    i_INVISIBLE              = 'i', //!< invisible
    c_COMMAND_STYLE          = 'c', //!< trigger is set with commandstyle boundaries instead of shocksytle
    x_START_DISABLED         = 'x', //!< this trigger is disabled on startup, default is enabled
    b_KEY_BLOCKER            = 'b', //!< Set the CommandKeys that are set in a commandkeyblocker or trigger to blocked on startup, default is released
    B_TRIGGER_BLOCKER        = 'B', //!< Blocker that enable/disable other triggers
    A_INV_TRIGGER_BLOCKER    = 'A', //!< Blocker that enable/disable other triggers, reversed activation method (inverted Blocker style, auto-ON)
    s_CMD_NUM_SWITCH         = 's', //!< switch that exchanges cmdshort/cmdshort for all triggers with the same commandnumbers, default false
    h_UNLOCKS_HOOK_GROUP     = 'h',
    H_LOCKS_HOOK_GROUP       = 'H',
    t_CONTINUOUS             = 't', //!< this trigger sends values between 0 and 1
    E_ENGINE_TRIGGER         = 'E', //!< this trigger is used to control an engine
};

enum class BeamOption: char
{
    v_DUMMY                  = 'v',
    i_INVISIBLE              = 'i',
    r_ROPE                   = 'r',
    s_SUPPORT                = 's',
};

enum class HydroOption: char
{
    n_INPUT_NORMAL           = 'n',
    j_INVISIBLE              = 'j',
    i_INVISIBLE_INPUT_NORMAL = 'i', //!< For backwards compatibility; combines flags 'j' and 'n'.
    // Useful for trucks
    s_DISABLE_ON_HIGH_SPEED  = 's',
    // Useful for planes: These can be used to control flight surfaces, or to create a thrust vectoring system.
    a_INPUT_AILERON          = 'a',
    r_INPUT_RUDDER           = 'r',
    e_INPUT_ELEVATOR         = 'e',
    u_INPUT_AILERON_ELEVATOR = 'u',
    v_INPUT_InvAILERON_ELEVATOR = 'v',
    x_INPUT_AILERON_RUDDER      = 'x',
    y_INPUT_InvAILERON_RUDDER   = 'y',
    g_INPUT_ELEVATOR_RUDDER     = 'g',
    h_INPUT_InvELEVATOR_RUDDER  = 'h',
};

enum class ShockOption: char
{
    n_DUMMY                  = 'n',
    v_DUMMY                  = 'v',
    i_INVISIBLE              = 'i',
    L_ACTIVE_LEFT            = 'L',
    R_ACTIVE_RIGHT           = 'R',
    m_METRIC                 = 'm',
};

enum class Shock2Option: char
{
    n_DUMMY                  = 'n',
    v_DUMMY                  = 'v',
    i_INVISIBLE              = 'i',
    s_SOFT_BUMP_BOUNDS       = 's', //!< soft bump boundaries, use when shocks reach limiters too often and "jumprebound" (default is hard bump boundaries)
    m_METRIC                 = 'm', //!< metric values for shortbound/longbound applying to the length of the beam.
    M_ABSOLUTE_METRIC        = 'M', //!< Absolute metric values for shortbound/longbound, settings apply without regarding to the original length of the beam.(Use with caution, check ror.log for errors)
};

enum class Shock3Option: char
{
    n_DUMMY                  = 'n',
    v_DUMMY                  = 'v',
    i_INVISIBLE              = 'i',
    m_METRIC                 = 'm', //!< metric values for shortbound/longbound applying to the length of the beam.
    M_ABSOLUTE_METRIC        = 'M', //!< Absolute metric values for shortbound/longbound, settings apply without regarding to the original length of the beam.(Use with caution, check ror.log for errors)
};

enum class NodeOption: char
{
    n_DUMMY                  = 'n',
    m_NO_MOUSE_GRAB          = 'm',
    f_NO_SPARKS              = 'f',
    x_EXHAUST_POINT          = 'x',
    y_EXHAUST_DIRECTION      = 'y',
    c_NO_GROUND_CONTACT      = 'c',
    h_HOOK_POINT             = 'h',
    e_TERRAIN_EDIT_POINT     = 'e',
    b_EXTRA_BUOYANCY         = 'b',
    p_NO_PARTICLES           = 'p',
    L_LOG                    = 'L',
    l_LOAD_WEIGHT            = 'l',
};

// --------------------------------
// Shared/helper rig definition data

struct AeroAnimator // used by Animator
{
    static const BitMask_t OPTION_THROTTLE = BITMASK(1);
    static const BitMask_t OPTION_RPM      = BITMASK(2);
    static const BitMask_t OPTION_TORQUE   = BITMASK(3);
    static const BitMask_t OPTION_PITCH    = BITMASK(4);
    static const BitMask_t OPTION_STATUS   = BITMASK(5);

    BitMask_t flags      = 0u;
    unsigned int engine_idx = 0u;
};

struct Assetpack
{
    std::string filename;
};

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

struct BaseMeshWheel: public BaseWheel // common to 'meshwheels' and 'meshwheels2'
{
    RoR::WheelSide side = RoR::WheelSide::INVALID;
    Ogre::String mesh_name;
    Ogre::String material_name;
    float rim_radius = 0.f;
    float tyre_radius = 0.f;
    float spring;
    float damping;
};

struct BaseWheel2: public BaseWheel // common to 'wheels2' and 'flexbodywheels'
{
    float rim_radius = 0.f;
    float tyre_radius = 0.f;
    float tyre_springiness = 0.f;
    float tyre_damping = 0.f;
};

struct Inertia // Common base for DefaultInertia and Command2Inertia
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

// --------------------------------
// Rig definition data for individual elements

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

struct Animation
{
    struct MotorSource
    {
        // > aeroengines
        static const BitMask_t SOURCE_AERO_THROTTLE = BITMASK(1);
        static const BitMask_t SOURCE_AERO_RPM      = BITMASK(2);
        static const BitMask_t SOURCE_AERO_TORQUE   = BITMASK(3);
        static const BitMask_t SOURCE_AERO_PITCH    = BITMASK(4);
        static const BitMask_t SOURCE_AERO_STATUS   = BITMASK(5);
        // > forward gears (hack...)
        static const BitMask_t SOURCE_GEAR_FORWARD  = BITMASK(6);

        BitMask_t source = 0;
        unsigned int motor = 0;
    };

    // Source flags
    static const BitMask64_t SOURCE_AIRSPEED          = BITMASK64( 1);
    static const BitMask64_t SOURCE_VERTICAL_VELOCITY = BITMASK64( 2);
    static const BitMask64_t SOURCE_ALTIMETER_100K    = BITMASK64( 3);
    static const BitMask64_t SOURCE_ALTIMETER_10K     = BITMASK64( 4);
    static const BitMask64_t SOURCE_ALTIMETER_1K      = BITMASK64( 5);
    static const BitMask64_t SOURCE_ANGLE_OF_ATTACK   = BITMASK64( 6);
    static const BitMask64_t SOURCE_FLAP              = BITMASK64( 7);
    static const BitMask64_t SOURCE_AIR_BRAKE         = BITMASK64( 8);
    static const BitMask64_t SOURCE_ROLL              = BITMASK64( 9);
    static const BitMask64_t SOURCE_PITCH             = BITMASK64(10);
    static const BitMask64_t SOURCE_BRAKES            = BITMASK64(11);
    static const BitMask64_t SOURCE_ACCEL             = BITMASK64(12);
    static const BitMask64_t SOURCE_CLUTCH            = BITMASK64(13);
    static const BitMask64_t SOURCE_SPEEDO            = BITMASK64(14);
    static const BitMask64_t SOURCE_TACHO             = BITMASK64(15);
    static const BitMask64_t SOURCE_TURBO             = BITMASK64(16);
    static const BitMask64_t SOURCE_PARKING           = BITMASK64(17);
    static const BitMask64_t SOURCE_SHIFT_LEFT_RIGHT  = BITMASK64(18);
    static const BitMask64_t SOURCE_SHIFT_BACK_FORTH  = BITMASK64(19);
    static const BitMask64_t SOURCE_SEQUENTIAL_SHIFT  = BITMASK64(20);
    static const BitMask64_t SOURCE_SHIFTERLIN        = BITMASK64(21);
    static const BitMask64_t SOURCE_TORQUE            = BITMASK64(22);
    static const BitMask64_t SOURCE_HEADING           = BITMASK64(23);
    static const BitMask64_t SOURCE_DIFFLOCK          = BITMASK64(24);
    static const BitMask64_t SOURCE_BOAT_RUDDER       = BITMASK64(25);
    static const BitMask64_t SOURCE_BOAT_THROTTLE     = BITMASK64(26);
    static const BitMask64_t SOURCE_STEERING_WHEEL    = BITMASK64(27);
    static const BitMask64_t SOURCE_AILERON           = BITMASK64(28);
    static const BitMask64_t SOURCE_ELEVATOR          = BITMASK64(29);
    static const BitMask64_t SOURCE_AIR_RUDDER        = BITMASK64(30);
    static const BitMask64_t SOURCE_PERMANENT         = BITMASK64(31);
    static const BitMask64_t SOURCE_EVENT             = BITMASK64(32);
    static const BitMask64_t SOURCE_DASHBOARD         = BITMASK64(33);
    static const BitMask64_t SOURCE_SIGNALSTALK       = BITMASK64(34);
    static const BitMask64_t SOURCE_AUTOSHIFTERLIN    = BITMASK64(35);
    static const BitMask64_t SOURCE_GEAR_NEUTRAL      = BITMASK64(36);
    static const BitMask64_t SOURCE_GEAR_REVERSE      = BITMASK64(37);

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
    BitMask64_t source = 0;
    std::list<MotorSource> motor_sources;
    BitMask_t mode = 0;
    Ogre::String event_name;
    Ogre::String dash_link_name;

    void AddMotorSource(BitMask_t source, unsigned int motor);
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

struct Author
{
    Ogre::String type;
    unsigned int forum_account_id = 0;
    Ogre::String name;
    Ogre::String email;
    bool _has_forum_account = false;
};

struct Axle
{
    Node::Ref wheels[2][2];
    DifferentialTypeVec options; //!< Order matters!
};

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

struct Brakes
{
    float default_braking_force = 30000.f;
    float parking_brake_force = -1.f;
};

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

struct Camera
{
    Node::Ref center_node;
    Node::Ref back_node;
    Node::Ref left_node;
};

struct CameraRail
{
    std::vector<Node::Ref> nodes;
};

struct CameraSettings
{
    int mode = RoR::CAMERA_MODE_ALWAYS_VISIBLE; //!< 0 and higher = cinecam index
};

struct Cinecam
{
    Ogre::Vector3 position = Ogre::Vector3::ZERO;
    Node::Ref nodes[8];
    float spring = 8000.f;
    float damping = 800.f;
    float node_mass = 20.f;
    std::shared_ptr<BeamDefaults> beam_defaults;
    std::shared_ptr<NodeDefaults> node_defaults;
};

struct CollisionBox
{
    std::vector<Node::Ref> nodes;
};

struct CollisionRange
{
    float node_collision_range = -1;
};

struct Command2 // 'commands' are auto-imported as 'commands2' (only 1 extra argument)
{
    Node::Ref nodes[2];
    float shorten_rate = 0.f;
    float lengthen_rate = 0.f;
    float max_contraction = 0.f;
    float max_extension = 0.f;
    unsigned int contract_key = 0;
    unsigned int extend_key = 0;
    Ogre::String description;
    Inertia inertia;
    float affect_engine = 1.f;
    bool needs_engine = true;
    bool plays_sound = true;
    std::shared_ptr<BeamDefaults> beam_defaults;
    std::shared_ptr<Inertia> inertia_defaults;
    int detacher_group = 0;

    bool option_i_invisible = false;
    bool option_r_rope = false;
    bool option_c_auto_center = false;
    bool option_f_not_faster = false;
    bool option_p_1press = false;
    bool option_o_1press_center = false;
};

struct CruiseControl
{
    float min_speed = 0.f;
    int autobrake = 0;
};

struct DefaultMinimass
{
    float min_mass_Kg = DEFAULT_MINIMASS; //!< minimum node mass in Kg
};

struct DefaultSkin
{
    std::string skin_name;
};

struct Engine
{
    float shift_down_rpm = 0.f;
    float shift_up_rpm = 0.f;
    float torque = 0.f;
    float global_gear_ratio = 0.f;
    float reverse_gear_ratio = 0.f;
    float neutral_gear_ratio = 0.f;
    std::vector<float> gear_ratios;
};

struct Engoption
{
    float inertia = 10.f;
    EngineType type;
    float clutch_force = -1.f;
    float shift_time = -1.f;       //!< Seconds
    float clutch_time = -1.f;      //!< Seconds
    float post_shift_time = -1.f;  //!< Seconds
    float idle_rpm = -1.f;
    float stall_rpm = -1.f;
    float max_idle_mixture = -1.f;
    float min_idle_mixture = -1.f;
    float braking_torque = -1.f;
};
 
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

struct Exhaust
{
    Node::Ref reference_node;
    Node::Ref direction_node;
    Ogre::String particle_name;
};

struct ExtCamera
{
    RoR::ExtCameraMode mode = RoR::ExtCameraMode::CLASSIC;
    Node::Ref node;
};

struct FileFormatVersion
{
    int version = -1;
};

struct Fileinfo
{
    Ogre::String unique_id;
    int category_id = -1;
    int file_version = 0;
};

struct Flare2 // Used for both 'flares' and 'flares2' sections
{
    Node::Ref reference_node;
    Node::Ref node_axis_x;
    Node::Ref node_axis_y;
    Ogre::Vector3 offset = Ogre::Vector3(0, 0, 1); // Section 'flares(1)' has offset.z hardcoded to 1
    RoR::FlareType type = RoR::FlareType::HEADLIGHT;
    int control_number = -1; //!< Only 'u' type flares.
    std::string dashboard_link; //!< Only 'd' type flares.
    int blink_delay_milis = -2;
    float size = -1.f;
    Ogre::String material_name;
};

struct Flare3: public Flare2
{
    std::shared_ptr<Inertia> inertia_defaults;
};

struct Flexbody
{
    Node::Ref reference_node;
    Node::Ref x_axis_node;
    Node::Ref y_axis_node;
    Ogre::Vector3 offset = Ogre::Vector3::ZERO;
    Ogre::Vector3 rotation = Ogre::Vector3::ZERO;
    Ogre::String mesh_name;
    std::list<Animation> animations;
    std::vector<Node::Range> node_list_to_import;
    std::vector<Node::Ref> node_list;
    CameraSettings camera_settings;
};

struct FlexBodyWheel: public BaseWheel2
{
    RoR::WheelSide side = RoR::WheelSide::INVALID;
    float rim_springiness = 0.f;
    float rim_damping = 0.f;
    Ogre::String rim_mesh_name;
    Ogre::String tyre_mesh_name;
};

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

struct Globals
{
    float dry_mass = 0;
    float cargo_mass = 0;
    Ogre::String material_name;
};

struct Guid
{
    std::string guid;
};

struct GuiSettings
{
    std::string key;
    std::string value;
};

struct Help
{
    std::string material;
};

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

struct Hydro
{
    static const BitMask_t OPTION_j_INVISIBLE                 = BITMASK(1);
    // Useful for trucks:
    static const BitMask_t OPTION_s_DISABLE_ON_HIGH_SPEED     = BITMASK(2);
    // Useful for planes: These can be used to control flight surfaces, or to create a thrust vectoring system.
    static const BitMask_t OPTION_a_INPUT_AILERON             = BITMASK(3);
    static const BitMask_t OPTION_r_INPUT_RUDDER              = BITMASK(4);
    static const BitMask_t OPTION_e_INPUT_ELEVATOR            = BITMASK(5);
    static const BitMask_t OPTION_u_INPUT_AILERON_ELEVATOR    = BITMASK(6);
    static const BitMask_t OPTION_v_INPUT_InvAILERON_ELEVATOR = BITMASK(7);
    static const BitMask_t OPTION_x_INPUT_AILERON_RUDDER      = BITMASK(8);
    static const BitMask_t OPTION_y_INPUT_InvAILERON_RUDDER   = BITMASK(9);
    static const BitMask_t OPTION_g_INPUT_ELEVATOR_RUDDER     = BITMASK(10);
    static const BitMask_t OPTION_h_INPUT_InvELEVATOR_RUDDER  = BITMASK(11);
    // Generic steering input
    static const BitMask_t OPTION_n_INPUT_NORMAL              = BITMASK(12);

    Node::Ref nodes[2];
    float lenghtening_factor = 0.f;
    BitMask_t options = 0;
    Inertia inertia;
    std::shared_ptr<Inertia> inertia_defaults;
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group = 0;
};

struct InterAxle
{
    int a1 = 0;
    int a2 = 0;
    DifferentialTypeVec options; //!< Order matters!
};

struct Lockgroup
{
    // TODO remove these, use SimConstants.h
    static const int LOCKGROUP_DEFAULT        = -1;
    static const int LOCKGROUP_NOLOCK         = 9999;

    int number = 0;
    std::vector<Node::Ref> nodes;
};

struct ManagedMaterialsOptions
{
    bool double_sided = false;
};

struct ManagedMaterial
{
    Ogre::String name;
    ManagedMaterialType type = ManagedMaterialType::INVALID;
    ManagedMaterialsOptions options;
    Ogre::String diffuse_map;
    Ogre::String damaged_diffuse_map;
    Ogre::String specular_map;
};

struct MaterialFlareBinding
{
    unsigned int flare_number = 0;
    Ogre::String material_name;
};

struct Minimass
{
    float global_min_mass_Kg; //!< minimum node mass in Kg - only effective where DefaultMinimass was not set.
    MinimassOption option = MinimassOption::n_DUMMY;
};

struct MeshWheel: public BaseMeshWheel
{};

struct MeshWheel2: public BaseMeshWheel
{};

struct NodeDefaults
{
    NodeDefaults();

    float load_weight;
    float friction;
    float volume;
    float surface;
    unsigned int options; //!< Bit flags
};

struct Particle
{
    Node::Ref emitter_node;
    Node::Ref reference_node;
    Ogre::String particle_system_name;
};

struct Pistonprop
{
    Node::Ref reference_node;
    Node::Ref axis_node;
    Node::Ref blade_tip_nodes[4];
    Node::Ref couple_node;
    float turbine_power_kW = 0.f;
    float pitch = 0;
    Ogre::String airfoil;
};

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

    Node::Ref reference_node;
    Node::Ref x_axis_node;
    Node::Ref y_axis_node;
    Ogre::Vector3 offset = Ogre::Vector3::ZERO;
    Ogre::Vector3 rotation = Ogre::Vector3::ZERO;
    Ogre::String mesh_name;
    std::list<Animation> animations;
    CameraSettings camera_settings;
    SpecialProp special = SpecialProp::NONE;
    BeaconSpecial special_prop_beacon;
    DashboardSpecial special_prop_dashboard;
};

struct RailGroup
{
    unsigned int id = 0;
    std::vector<Node::Range> node_list;
};

struct Ropable
{
    Node::Ref node;
    int group = -1; // = value not set
    bool has_multilock = false;
};

struct Rope
{
    Node::Ref root_node;
    Node::Ref end_node;
    bool invisible = false;
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group = 0;
};

struct Rotator
{
    Node::Ref axis_nodes[2];
    Node::Ref base_plate_nodes[4];
    Node::Ref rotating_plate_nodes[4];
    float rate = 0.f;
    unsigned int spin_left_key = 0;
    unsigned int spin_right_key = 0;
    Inertia inertia;
    std::shared_ptr<Inertia> inertia_defaults;
    float engine_coupling = 1.f;
    bool needs_engine = false;
};

struct Rotator2: public Rotator
{
    float rotating_force = 10000000.f;
    float tolerance = 0.f;
    Ogre::String description;
};

struct Screwprop
{
    Node::Ref prop_node;
    Node::Ref back_node;
    Node::Ref top_node;
    float power = 0.f;
};

struct Script
{
    std::string filename;
};

struct ShadowOptions
{
    int shadow_mode = 0;
};

struct Shock
{
    static const BitMask_t OPTION_i_INVISIBLE    = BITMASK(1);
    static const BitMask_t OPTION_L_ACTIVE_LEFT  = BITMASK(2);
    static const BitMask_t OPTION_R_ACTIVE_RIGHT = BITMASK(3);
    static const BitMask_t OPTION_m_METRIC       = BITMASK(4);

    Node::Ref nodes[2];
    float spring_rate = 0.f;         //!< The 'stiffness' of the shock. The higher the value, the less the shock will move for a given bump. 
    float damping = 0.f;             //!< The 'resistance to motion' of the shock. The best value is given by this equation:  2 * sqrt(suspended mass * springness)
    float short_bound = 0.f;         //!< Maximum contraction. The shortest length the shock can be, as a proportion of its original length. "0" means the shock will not be able to contract at all, "1" will let it contract all the way to zero length. If the shock tries to shorten more than this value allows, it will become as rigid as a normal beam. 
    float long_bound = 0.f;          //!< Maximum extension. The longest length a shock can be, as a proportion of its original length. "0" means the shock will not be able to extend at all. "1" means the shock will be able to double its length. Higher values allow for longer extension.
    float precompression = 1.f;      //!< Changes compression or extension of the suspension when the truck spawns. This can be used to "level" the suspension of a truck if it sags in game. The default value is 1.0. 
    BitMask_t options = 0;
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group = 0;
};

struct Shock2
{
    Shock2();

    static const BitMask_t OPTION_i_INVISIBLE        = BITMASK(1);
    static const BitMask_t OPTION_s_SOFT_BUMP_BOUNDS = BITMASK(2); // soft bump boundaries, use when shocks reach limiters too often and "jumprebound" (default is hard bump boundaries)
    static const BitMask_t OPTION_m_METRIC           = BITMASK(3); // metric values for shortbound/longbound applying to the length of the beam.
    static const BitMask_t OPTION_M_ABSOLUTE_METRIC  = BITMASK(4); // Absolute metric values for shortbound/longbound, settings apply without regarding to the original length of the beam.(Use with caution, check ror.log for errors)

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
    BitMask_t options;
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group;
};

struct Shock3
{
    Shock3();

    static const BitMask_t OPTION_i_INVISIBLE        = BITMASK(1);
    static const BitMask_t OPTION_m_METRIC           = BITMASK(2); // metric values for shortbound/longbound applying to the length of the beam.
    static const BitMask_t OPTION_M_ABSOLUTE_METRIC  = BITMASK(3); // Absolute metric values for shortbound/longbound, settings apply without regarding to the original length of the beam.(Use with caution, check ror.log for errors)

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
    BitMask_t options;
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group;
};

struct SkeletonSettings
{
    float visibility_range_meters = 150.f;
    float beam_thickness_meters = BEAM_SKELETON_DIAMETER;
};

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

struct SoundSource
{
    Node::Ref node;
    Ogre::String sound_script_name;
};

struct SoundSource2: public SoundSource
{
    static const int MODE_ALWAYS = -2;
    static const int MODE_EXTERIOR = -1;

    int mode = MODE_ALWAYS; //!< A special constant or cinecam index.
};

struct SpeedLimiter
{
    float max_speed = 0.f;
    bool is_enabled = false;
};

struct Texcoord; // needed by Submesh

struct Submesh
{
    bool backmesh = false;
    std::vector<Texcoord> texcoords;
    std::vector<Cab> cab_triangles;
};

struct Texcoord
{
    Node::Ref node;
    float u = 0.f;
    float v = 0.f;
};

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

struct TorqueCurve
{
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

struct TransferCase
{
    int a1 = 0;
    int a2 = -1;
    bool has_2wd = true;
    bool has_2wd_lo = false;
    std::vector<float> gear_ratios = {1.0f};
};

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

struct Turbojet
{
    Node::Ref front_node;
    Node::Ref back_node;
    Node::Ref side_node;
    int is_reversable = 0;
    float dry_thrust = 0.f;
    float wet_thrust = 0.f;
    float front_diameter = 0.f;
    float back_diameter = 0.f;
    float nozzle_length = 0.f;
};

struct Turboprop2 // Section TURBOPROPS, TURBOPROPS2 
{
    Node::Ref reference_node;
    Node::Ref axis_node;
    Node::Ref blade_tip_nodes[4];
    float turbine_power_kW = 0.f;
    Ogre::String airfoil;
    Node::Ref couple_node;
};

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

struct Wheel: public BaseWheel
{
    float radius = 0.f;
    float springiness = 0.f;
    float damping = 0.f;
    Ogre::String face_material_name = "tracks/wheelface";
    Ogre::String band_material_name = "tracks/wheelband1";
};

struct Wheel2: public BaseWheel2
{
    float rim_springiness = 0.f;
    float rim_damping = 0.f;
    Ogre::String face_material_name = "tracks/wheelface";
    Ogre::String band_material_name = "tracks/wheelband1";
};

struct WheelDetacher
{
    int wheel_id = 0;
    int detacher_group = 0;
};

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

// --------------------------------
// The document

struct Document
{
    struct Module // represents 'section/end_section'. Also use by 'addonparts' system.
    {
        Module(Ogre::String const & name);

        Ogre::String name;
        RoR::CacheEntryPtr origin_addonpart; //!< Addon parts are spawned as fake Modules, resources must be loaded from this group.

        std::vector<Airbrake>              airbrakes;
        std::vector<Animator>              animators;
        std::vector<AntiLockBrakes>        antilockbrakes;
        std::vector<Assetpack>             assetpacks;
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
        std::vector<DefaultSkin>           default_skin;
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
        std::vector<Flare3>                flares3;
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
        std::vector<MeshWheel>             meshwheels;
        std::vector<MeshWheel2>            meshwheels2;
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
        std::vector<Script>                scripts;
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
        std::vector<Wheel2>                wheels2;
        std::vector<Wing>                  wings;

        // Metadata
        int _hint_nodes12_start_linenumber = -1;
        int _hint_nodes12_end_linenumber = -1;
        int _hint_beams_start_linenumber = -1;
        int _hint_beams_end_linenumber = -1;
    };

    Document();

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
