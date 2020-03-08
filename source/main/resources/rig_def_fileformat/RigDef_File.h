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
    @brief Structures which represent a rig-definition file (1:1 match) 

    Values are initialised to their defaults, or 0 if there's no default.

    NOTE ON ARCHITECTURE:
    These structs were designed for a single purpose - to bring data 
    from rig def. file.	Do not use them for any other purpose, don't 
    modify them beyond their original purpose. Avoid modifying their
    contents, only Parser should do that; use them only for reading;

    NOTES: 
    * Since these are open structs, the m_ prefix for member variables is not used.
    * Members prefixed by _ are helper flags which mark special values or missing values.
*/

#pragma once

#include "BitFlags.h"

#include "RigDef_Node.h"
#include "BeamConstants.h"

#include <list>
#include <memory>
#include <vector>
#include <string>
#include <OgreString.h>
#include <OgreVector3.h>
#include <OgreStringConverter.h>

namespace RigDef {

extern const char* ROOT_MODULE_NAME;

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
    unsigned int options; ///< Bit flags
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
    bool _enable_advanced_deformation; ///< Informs whether "enable_advanced_deformation" directive preceded these defaults.
    bool _is_plastic_deform_coef_user_defined;
    bool _is_user_defined; ///< Informs whether these data were read from "set_beam_defaults" directive or filled in by the parser on startup.
    BeamDefaultsScale scale;
};

/* -------------------------------------------------------------------------- */
/* Hybrid section MINIMASS                                                    */
/* -------------------------------------------------------------------------- */

struct MinimassPreset
{
    enum Option
    {
        OPTION_n_FILLER  = 'n',     //!< Updates the global minimass
        OPTION_l_SKIP_LOADED = 'l'  //!< Only apply minimum mass to nodes without "L" option.
    };

    MinimassPreset(): min_mass(DEFAULT_MINIMASS)
    {}

    explicit MinimassPreset(float m): min_mass(m)
    {}

    float min_mass; //!< minimum node mass in Kg
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
    GuiSettings():
        speedo_highest_kph(DEFAULT_SPEEDO_MAX),
        use_max_rpm(false), /* This is default */
        interactive_overview_map_mode(MAP_MODE_OFF)
    {}

    enum MapMode
    {
        MAP_MODE_OFF = 1,
        MAP_MODE_SIMPLE,
        MAP_MODE_ZOOM,

        MAP_MODE_INVALID = 0xFFFFFFFF,
    };

    const static unsigned int DEFAULT_SPEEDO_MAX = 140;

    std::string tacho_material;
    std::string speedo_material;
    unsigned int speedo_highest_kph;
    bool use_max_rpm;
    Ogre::String help_material;
    MapMode interactive_overview_map_mode;
    std::list<std::string> dashboard_layouts;
    std::list<std::string> rtt_dashboard_layouts;
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
        MotorSource():
            source(0),
            motor(0)
        {}

        static const unsigned int SOURCE_AERO_THROTTLE = BITMASK(1);
        static const unsigned int SOURCE_AERO_RPM      = BITMASK(2);
        static const unsigned int SOURCE_AERO_TORQUE   = BITMASK(3);
        static const unsigned int SOURCE_AERO_PITCH    = BITMASK(4);
        static const unsigned int SOURCE_AERO_STATUS   = BITMASK(5);

        unsigned int source;
        unsigned int motor;
    };

    Animation():
        ratio(0),
        lower_limit(-1.f),
        upper_limit(-1.f),
        source(0),
        mode(0)
    {}

    BITMASK_PROPERTY( source,  1, SOURCE_AIRSPEED          , HasSource_AirSpeed            , SetHasSource_AirSpeed )
    BITMASK_PROPERTY( source,  2, SOURCE_VERTICAL_VELOCITY , HasSource_VerticalVelocity    , SetHasSource_VerticalVelocity )
    BITMASK_PROPERTY( source,  3, SOURCE_ALTIMETER_100K    , HasSource_AltiMeter100k       , SetHasSource_AltiMeter100k )
    BITMASK_PROPERTY( source,  4, SOURCE_ALTIMETER_10K     , HasSource_AltiMeter10k        , SetHasSource_AltiMeter10k )
    BITMASK_PROPERTY( source,  5, SOURCE_ALTIMETER_1K      , HasSource_AltiMeter1k         , SetHasSource_AltiMeter1k )
    BITMASK_PROPERTY( source,  6, SOURCE_ANGLE_OF_ATTACK   , HasSource_AOA                 , SetHasSource_AOA )
    BITMASK_PROPERTY( source,  7, SOURCE_FLAP              , HasSource_Flap                , SetHasSource_Flap )
    BITMASK_PROPERTY( source,  8, SOURCE_AIR_BRAKE         , HasSource_AirBrake            , SetHasSource_AirBrake )
    BITMASK_PROPERTY( source,  9, SOURCE_ROLL              , HasSource_Roll                , SetHasSource_Roll )
    BITMASK_PROPERTY( source, 10, SOURCE_PITCH             , HasSource_Pitch               , SetHasSource_Pitch )
    BITMASK_PROPERTY( source, 11, SOURCE_BRAKES            , HasSource_Brakes              , SetHasSource_Brakes )
    BITMASK_PROPERTY( source, 12, SOURCE_ACCEL             , HasSource_Accel               , SetHasSource_Accel )
    BITMASK_PROPERTY( source, 13, SOURCE_CLUTCH            , HasSource_Clutch              , SetHasSource_Clutch )
    BITMASK_PROPERTY( source, 14, SOURCE_SPEEDO            , HasSource_Speedo              , SetHasSource_Speedo )
    BITMASK_PROPERTY( source, 15, SOURCE_TACHO             , HasSource_Tacho               , SetHasSource_Tacho )
    BITMASK_PROPERTY( source, 16, SOURCE_TURBO             , HasSource_Turbo               , SetHasSource_Turbo )
    BITMASK_PROPERTY( source, 17, SOURCE_PARKING           , HasSource_ParkingBrake        , SetHasSource_ParkingBrake )
    BITMASK_PROPERTY( source, 18, SOURCE_SHIFT_LEFT_RIGHT  , HasSource_ManuShiftLeftRight  , SetHasSource_ManuShiftLeftRight )
    BITMASK_PROPERTY( source, 19, SOURCE_SHIFT_BACK_FORTH  , HasSource_ManuShiftBackForth  , SetHasSource_ManuShiftBackForth )
    BITMASK_PROPERTY( source, 20, SOURCE_SEQUENTIAL_SHIFT  , HasSource_SeqentialShift      , SetHasSource_SeqentialShift )
    BITMASK_PROPERTY( source, 21, SOURCE_SHIFTERLIN        , HasSource_ShifterLin          , SetHasSource_ShifterLin )
    BITMASK_PROPERTY( source, 22, SOURCE_TORQUE            , HasSource_Torque              , SetHasSource_Torque )
    BITMASK_PROPERTY( source, 23, SOURCE_HEADING           , HasSource_Heading             , SetHasSource_Heading )
    BITMASK_PROPERTY( source, 24, SOURCE_DIFFLOCK          , HasSource_DiffLock            , SetHasSource_DiffLock )
    BITMASK_PROPERTY( source, 25, SOURCE_BOAT_RUDDER       , HasSource_BoatRudder          , SetHasSource_BoatRudder )
    BITMASK_PROPERTY( source, 26, SOURCE_BOAT_THROTTLE     , HasSource_BoatThrottle        , SetHasSource_BoatThrottle )
    BITMASK_PROPERTY( source, 27, SOURCE_STEERING_WHEEL    , HasSource_SteeringWheel       , SetHasSource_SteeringWheel )
    BITMASK_PROPERTY( source, 28, SOURCE_AILERON           , HasSource_Aileron             , SetHasSource_Aileron )
    BITMASK_PROPERTY( source, 29, SOURCE_ELEVATOR          , HasSource_Elevator            , SetHasSource_Elevator )
    BITMASK_PROPERTY( source, 30, SOURCE_AIR_RUDDER        , HasSource_AerialRudder        , SetHasSource_AerialRudder )
    BITMASK_PROPERTY( source, 31, SOURCE_PERMANENT         , HasSource_Permanent           , SetHasSource_Permanent )
    BITMASK_PROPERTY( source, 32, SOURCE_EVENT             , HasSource_Event               , SetHasSource_Event ) // Full house32

    static const unsigned int MODE_ROTATION_X          = BITMASK(1);
    static const unsigned int MODE_ROTATION_Y          = BITMASK(2);
    static const unsigned int MODE_ROTATION_Z          = BITMASK(3);
    static const unsigned int MODE_OFFSET_X            = BITMASK(4);
    static const unsigned int MODE_OFFSET_Y            = BITMASK(5);
    static const unsigned int MODE_OFFSET_Z            = BITMASK(6);
    static const unsigned int MODE_AUTO_ANIMATE        = BITMASK(7);
    static const unsigned int MODE_NO_FLIP             = BITMASK(8);
    static const unsigned int MODE_BOUNCE              = BITMASK(9);
    static const unsigned int MODE_EVENT_LOCK          = BITMASK(10);

    float ratio;
    float lower_limit;
    float upper_limit;
    unsigned int source;
    std::list<MotorSource> motor_sources;
    unsigned int mode;

    // NOTE: MSVC highlights 'event' as keyword: http://msdn.microsoft.com/en-us/library/4b612y2s%28v=vs.100%29.aspx
    // But it's ok to use as identifier in this context: http://msdn.microsoft.com/en-us/library/8d7y7wz6%28v=vs.100%29.aspx
    Ogre::String event;

    void AddMotorSource(unsigned int source, unsigned int motor);
};

/* -------------------------------------------------------------------------- */
/* Section AXLES                                                              */
/* -------------------------------------------------------------------------- */

struct Axle
{
    Axle():
        options(0)
    {}

    static const char OPTION_o_OPEN    = 'o';
    static const char OPTION_l_LOCKED  = 'l';
    static const char OPTION_s_SPLIT   = 's';
    static const char OPTION_s_VISCOUS = 'v';

    Node::Ref wheels[2][2];
    std::vector<char> options; //!< Order matters!
};

/* -------------------------------------------------------------------------- */
/* Section INTERAXLES                                                         */
/* -------------------------------------------------------------------------- */

struct InterAxle
{
    InterAxle():
        a1(0),
        a2(0),
        options(0)
    {}

    static const char OPTION_o_OPEN    = 'o';
    static const char OPTION_l_LOCKED  = 'l';
    static const char OPTION_s_SPLIT   = 's';
    static const char OPTION_s_VISCOUS = 'v';

    int a1;
    int a2;
    std::vector<char> options; //!< Order matters!
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
    Beam():
        options(0),
        extension_break_limit(0), /* This is default */
        _has_extension_break_limit(false),
        detacher_group(0), /* 0 = Default detacher group */
        editor_group_id(-1)
    {}

    BITMASK_PROPERTY(options, 1, OPTION_i_INVISIBLE, HasFlag_i_Invisible, SetFlag_i_Invisible);
    BITMASK_PROPERTY(options, 2, OPTION_r_ROPE     , HasFlag_r_Rope     , SetFlag_r_Rope     );
    BITMASK_PROPERTY(options, 3, OPTION_s_SUPPORT  , HasFlag_s_Support  , SetFlag_s_Support  );

    Node::Ref nodes[2];
    unsigned int options; ///< Bit flags
    float extension_break_limit;
    bool _has_extension_break_limit;
    int detacher_group;
    std::shared_ptr<BeamDefaults> defaults;
    int editor_group_id;
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
    Ogre::String type;
    int forum_account_id = -1; // -1 = invalid
    Ogre::String name;
    Ogre::String email;
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
    float shift_time;       ///< Seconds
    float clutch_time;      ///< Seconds
    float post_shift_time;  ///< Seconds
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
/* Inline-section SLOPE_BRAKE                                                 */
/* -------------------------------------------------------------------------- */

struct SlopeBrake
{
    SlopeBrake():
        regulating_force(6.f),
        attach_angle(5.f),
        release_angle(10.f)
    {}

    static float GetDefaultRegulatingForce()
    {
        return 6.f;
    }

    static float GetDefaultAttachAngle()
    {
        return 5.f;
    }

    static float GetDefaultReleaseAngle()
    {
        return 10.f;
    }

    float regulating_force;
    float attach_angle;
    float release_angle;
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

/** Syntax-sugar struct to hold enums */
struct Wheels
{
    enum Braking
    {
        BRAKING_NO                = 0,
        BRAKING_YES               = 1,
        BRAKING_DIRECTIONAL_LEFT  = 2,
        BRAKING_DIRECTIONAL_RIGHT = 3,
        BRAKING_ONLY_FOOT         = 4,

        BRAKING_INVALID           = 0xFFFFFFFF
    };

    enum Propulsion
    {
        PROPULSION_NONE     = 0,
        PROPULSION_FORWARD  = 1,
        PROPULSION_BACKWARD = 2,

        PROPULSION_INVALID  = 0xFFFFFFFF
    };
};

/** Attributes common to all wheel definitions */
struct BaseWheel
{
    BaseWheel():
        width(0),
        num_rays(0),
        braking(Wheels::BRAKING_NO),
        mass(0)
    {}

    virtual ~BaseWheel()
    {}

    float width;
    unsigned int num_rays;
    Node::Ref nodes[2];
    Node::Ref rigidity_node;
    Wheels::Braking braking;
    Wheels::Propulsion propulsion;
    Node::Ref reference_arm_node;
    float mass;
    std::shared_ptr<NodeDefaults> node_defaults;
    std::shared_ptr<BeamDefaults> beam_defaults;
};

/** The actual wheel */
struct Wheel: BaseWheel
{
    Wheel():
        BaseWheel(),
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
        BaseWheel(),
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
        BaseWheel(),
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

/** Used for both 'flares' and 'flares_2' sections
*/
struct Flare2
{
    Flare2():
        offset(0, 0, 1), /* Section 'flares(1)' has offset.z hardcoded to 1 */
        type(TYPE_f_HEADLIGHT),
        control_number(-1),
        blink_delay_milis(-2),
        size(-1)
    {}

    enum Type: char
    {
        TYPE_f_HEADLIGHT     = 'f',
        TYPE_b_BRAKELIGHT    = 'b',
        TYPE_l_LEFT_BLINKER  = 'l',
        TYPE_r_RIGHT_BLINKER = 'r',
        TYPE_R_REVERSE_LIGHT = 'R',
        TYPE_u_USER          = 'u',

        TYPE_INVALID         = 'n'
    };

    Node::Ref reference_node;
    Node::Ref node_axis_x;
    Node::Ref node_axis_y;
    Ogre::Vector3 offset;
    Type type;
    int control_number;
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
    std::vector<Node::Range> node_list_to_import; //< Node ranges are disallowed in fileformatversion >=450
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
    float spring_rate;         ///< The 'stiffness' of the shock. The higher the value, the less the shock will move for a given bump. 
    float damping;             ///< The 'resistance to motion' of the shock. The best value is given by this equation:  2 * sqrt(suspended mass * springness)
    float short_bound;         ///< Maximum contraction. The shortest length the shock can be, as a proportion of its original length. "0" means the shock will not be able to contract at all, "1" will let it contract all the way to zero length. If the shock tries to shorten more than this value allows, it will become as rigid as a normal beam. 
    float long_bound;          ///< Maximum extension. The longest length a shock can be, as a proportion of its original length. "0" means the shock will not be able to extend at all. "1" means the shock will be able to double its length. Higher values allow for longer extension.
    float precompression;      ///< Changes compression or extension of the suspension when the truck spawns. This can be used to "level" the suspension of a truck if it sags in game. The default value is 1.0. 
    unsigned int options;      ///< Bit flags.
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
    float spring_in;                  ///< Spring value applied when the shock is compressing.
    float damp_in;                    ///< Damping value applied when the shock is compressing. 
    float progress_factor_spring_in;  ///< Progression factor for springin. A value of 0 disables this option. 1...x as multipliers, example:maximum springrate == springrate + (factor*springrate)
    float progress_factor_damp_in;    ///< Progression factor for dampin. 0 = disabled, 1...x as multipliers, example:maximum dampingrate == springrate + (factor*dampingrate)
    float spring_out;                 ///< spring value applied when shock extending
    float damp_out;                   ///< damping value applied when shock extending
    float progress_factor_spring_out; ///< Progression factor springout, 0 = disabled, 1...x as multipliers, example:maximum springrate == springrate + (factor*springrate)
    float progress_factor_damp_out;   ///< Progression factor dampout, 0 = disabled, 1...x as multipliers, example:maximum dampingrate == springrate + (factor*dampingrate)
    float short_bound;                ///< Maximum contraction limit, in percentage ( 1.00 = 100% )
    float long_bound;                 ///< Maximum extension limit, in percentage ( 1.00 = 100% )
    float precompression;             ///< Changes compression or extension of the suspension when the truck spawns. This can be used to "level" the suspension of a truck if it sags in game. The default value is 1.0.  
    unsigned int options;             ///< Bit flags.
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
    float spring_in;                  ///< Spring value applied when the shock is compressing.
    float damp_in;                    ///< Damping value applied when the shock is compressing. 
    float spring_out;                 ///< Spring value applied when shock extending
    float damp_out;                   ///< Damping value applied when shock extending
    float damp_in_slow;               ///< Damping value applied when shock is commpressing slower than split in velocity
    float split_vel_in;               ///< Split velocity in (m/s) - threshold for slow / fast damping during compression
    float damp_in_fast;               ///< Damping value applied when shock is commpressing faster than split in velocity
    float damp_out_slow;              ///< Damping value applied when shock is commpressing slower than split out velocity
    float split_vel_out;              ///< Split velocity in (m/s) - threshold for slow / fast damping during extension
    float damp_out_fast;              ///< Damping value applied when shock is commpressing faster than split out velocity
    float short_bound;                ///< Maximum contraction limit, in percentage ( 1.00 = 100% )
    float long_bound;                 ///< Maximum extension limit, in percentage ( 1.00 = 100% )
    float precompression;             ///< Changes compression or extension of the suspension when the truck spawns. This can be used to "level" the suspension of a truck if it sags in game. The default value is 1.0.  
    unsigned int options;             ///< Bit flags.
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
    AeroAnimator():
        flags(0),
        motor(0)
    {}

    static const unsigned int OPTION_THROTTLE = BITMASK(1);
    static const unsigned int OPTION_RPM      = BITMASK(2);
    static const unsigned int OPTION_TORQUE   = BITMASK(3);
    static const unsigned int OPTION_PITCH    = BITMASK(4);
    static const unsigned int OPTION_STATUS   = BITMASK(5);

    unsigned int flags;
    unsigned int motor;
};

struct Animator
{
    Animator():
        lenghtening_factor(0),
        flags(0),
        short_limit(0),
        long_limit(0),
        detacher_group(0)
    {}

    static const unsigned int OPTION_VISIBLE           = BITMASK(1);
    static const unsigned int OPTION_INVISIBLE         = BITMASK(2);
    static const unsigned int OPTION_AIRSPEED          = BITMASK(3);
    static const unsigned int OPTION_VERTICAL_VELOCITY = BITMASK(4);
    static const unsigned int OPTION_ALTIMETER_100K    = BITMASK(5);
    static const unsigned int OPTION_ALTIMETER_10K     = BITMASK(6);
    static const unsigned int OPTION_ALTIMETER_1K      = BITMASK(7);
    static const unsigned int OPTION_ANGLE_OF_ATTACK   = BITMASK(8);
    static const unsigned int OPTION_FLAP              = BITMASK(9);
    static const unsigned int OPTION_AIR_BRAKE         = BITMASK(10);
    static const unsigned int OPTION_ROLL              = BITMASK(11);
    static const unsigned int OPTION_PITCH             = BITMASK(12);
    static const unsigned int OPTION_BRAKES            = BITMASK(13);
    static const unsigned int OPTION_ACCEL             = BITMASK(14);
    static const unsigned int OPTION_CLUTCH            = BITMASK(15);
    static const unsigned int OPTION_SPEEDO            = BITMASK(16);
    static const unsigned int OPTION_TACHO             = BITMASK(17);
    static const unsigned int OPTION_TURBO             = BITMASK(18);
    static const unsigned int OPTION_PARKING           = BITMASK(19);
    static const unsigned int OPTION_SHIFT_LEFT_RIGHT  = BITMASK(20);
    static const unsigned int OPTION_SHIFT_BACK_FORTH  = BITMASK(21);
    static const unsigned int OPTION_SEQUENTIAL_SHIFT  = BITMASK(22);
    static const unsigned int OPTION_GEAR_SELECT       = BITMASK(23);
    static const unsigned int OPTION_TORQUE            = BITMASK(24);
    static const unsigned int OPTION_DIFFLOCK          = BITMASK(25);
    static const unsigned int OPTION_BOAT_RUDDER       = BITMASK(26);
    static const unsigned int OPTION_BOAT_THROTTLE     = BITMASK(27);
    static const unsigned int OPTION_SHORT_LIMIT       = BITMASK(28);
    static const unsigned int OPTION_LONG_LIMIT        = BITMASK(29);

    Node::Ref nodes[2];
    float lenghtening_factor;
    unsigned int flags;
    float short_limit;
    float long_limit;
    AeroAnimator aero_animator;
    std::shared_ptr<Inertia> inertia_defaults;
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group;
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
    struct EngineTrigger
    {
        enum Function
        {
            ENGINE_TRIGGER_FUNCTION_CLUTCH      = 0,
            ENGINE_TRIGGER_FUNCTION_BRAKE       = 1,
            ENGINE_TRIGGER_FUNCTION_ACCELERATOR = 2,
            ENGINE_TRIGGER_FUNCTION_RPM_CONTROL = 3,
            ENGINE_TRIGGER_FUNCTION_SHIFT_UP    = 4, ///< Do not mix with OPTION_t_CONTINUOUS
            ENGINE_TRIGGER_FUNCTION_SHIFT_DOWN  = 5, ///< Do not mix with OPTION_t_CONTINUOUS

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

    Trigger();

    BITMASK_PROPERTY(options,  1, OPTION_i_INVISIBLE             , HasFlag_i_Invisible,         SetFlag_i_Invisible         )
    BITMASK_PROPERTY(options,  2, OPTION_c_COMMAND_STYLE         , HasFlag_c_CommandStyle,      SetFlag_c_CommandStyle      )
    BITMASK_PROPERTY(options,  3, OPTION_x_START_OFF             , HasFlag_x_StartDisabled,     SetFlag_x_StartDisabled     )
    BITMASK_PROPERTY(options,  4, OPTION_b_BLOCK_KEYS            , HasFlag_b_KeyBlocker,        SetFlag_b_KeyBlocker        )
    BITMASK_PROPERTY(options,  5, OPTION_B_BLOCK_TRIGGERS        , HasFlag_B_TriggerBlocker,    SetFlag_B_TriggerBlocker    )
    BITMASK_PROPERTY(options,  6, OPTION_A_INV_BLOCK_TRIGGERS    , HasFlag_A_InvTriggerBlocker, SetFlag_A_InvTriggerBlocker )
    BITMASK_PROPERTY(options,  7, OPTION_s_SWITCH_CMD_NUM        , HasFlag_s_CmdNumSwitch,      SetFlag_s_CmdNumSwitch      )
    BITMASK_PROPERTY(options,  8, OPTION_h_UNLOCK_HOOKGROUPS_KEY , HasFlag_h_UnlocksHookGroup,  SetFlag_h_UnlocksHookGroup  )
    BITMASK_PROPERTY(options,  9, OPTION_H_LOCK_HOOKGROUPS_KEY   , HasFlag_H_LocksHookGroup,    SetFlag_H_LocksHookGroup    )
    BITMASK_PROPERTY(options, 10, OPTION_t_CONTINUOUS            , HasFlag_t_Continuous,        SetFlag_t_Continuous        )
    BITMASK_PROPERTY(options, 11, OPTION_E_ENGINE_TRIGGER        , HasFlag_E_EngineTrigger,     SetFlag_E_EngineTrigger     )

    inline bool IsHookToggleTrigger() { return HasFlag_H_LocksHookGroup() || HasFlag_h_UnlocksHookGroup(); }

    inline bool IsTriggerBlockerAnyType() { return HasFlag_B_TriggerBlocker() || HasFlag_A_InvTriggerBlocker(); }

    inline void SetEngineTrigger(Trigger::EngineTrigger const & trig)
    {
        shortbound_trigger_action = (int) trig.function;
        longbound_trigger_action = (int) trig.motor_index;
    }

    inline Trigger::EngineTrigger GetEngineTrigger() const
    {
        assert(HasFlag_E_EngineTrigger());
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
        assert(BITMASK_IS_0(options, OPTION_B_BLOCK_TRIGGERS | OPTION_A_INV_BLOCK_TRIGGERS 
            | OPTION_h_UNLOCK_HOOKGROUPS_KEY | OPTION_H_LOCK_HOOKGROUPS_KEY | OPTION_E_ENGINE_TRIGGER));
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
        assert(HasFlag_h_UnlocksHookGroup() || HasFlag_H_LocksHookGroup());
        HookToggleTrigger trig;
        trig.contraction_trigger_hookgroup_id = shortbound_trigger_action;
        trig.extension_trigger_hookgroup_id = longbound_trigger_action;
        return trig;
    }

    Node::Ref nodes[2];
    float contraction_trigger_limit;
    float expansion_trigger_limit;
    unsigned int options;
    float boundary_timer;
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group;
    int shortbound_trigger_action;
    int longbound_trigger_action;
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

    static const char* TypeToStr(Type type);
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
/* Section NODECOLLISION                                                      */
/* -------------------------------------------------------------------------- */

struct NodeCollision
{
    NodeCollision():
        radius(0)
    {}

    Node::Ref node;
    float radius;
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
        special(SPECIAL_NONE)
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

        SPECIAL_NONE = 0xFFFFFFFF
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
/* Section SCRIPTS                                                            */
/* -------------------------------------------------------------------------- */

struct Script
{
    enum Type
    {
        TYPE_INVALID,
        TYPE_FRAMESTEP,
        TYPE_SIMSTEP
    };

    Script()
    {}

    Script(Type t, std::string const& fname, std::string const& arg)
        :type(t), filename(fname), arguments(arg)
    {}

    Type        type = TYPE_INVALID;
    std::string filename;
    std::string arguments;
};

/* -------------------------------------------------------------------------- */
/* Section SLIDENODES                                                         */
/* -------------------------------------------------------------------------- */

struct SlideNode
{
    SlideNode();

    BITMASK_PROPERTY( constraint_flags, 1, CONSTRAINT_ATTACH_ALL     , HasConstraint_a_AttachAll     , SetConstraint_a_AttachAll     )   
    BITMASK_PROPERTY( constraint_flags, 2, CONSTRAINT_ATTACH_FOREIGN , HasConstraint_f_AttachForeign , SetConstraint_f_AttachForeign )
    BITMASK_PROPERTY( constraint_flags, 3, CONSTRAINT_ATTACH_SELF    , HasConstraint_s_AttachSelf 	 , SetConstraint_s_AttachSelf	 )
    BITMASK_PROPERTY( constraint_flags, 4, CONSTRAINT_ATTACH_NONE    , HasConstraint_n_AttachNone 	 , SetConstraint_n_AttachNone	 )

    Node::Ref slide_node;
    std::vector<Node::Range> rail_node_ranges;
    float spring_rate;
    float break_force;
    float tolerance;
    int railgroup_id;
    bool _railgroup_id_set;
    float attachment_rate;
    float max_attachment_distance;
    bool _break_force_set;
    unsigned int constraint_flags;
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
    Cab():
        options(0)
    {}

    bool GetOption_D_ContactBuoyant()
    {
        return BITMASK_IS_1(options, OPTION_b_BUOYANT) && BITMASK_IS_1(options, OPTION_c_CONTACT);
    }

    bool GetOption_F_10xTougherBuoyant()
    {
        return BITMASK_IS_1(options, OPTION_b_BUOYANT) && BITMASK_IS_1(options, OPTION_p_10xTOUGHER);
    }

    bool GetOption_S_UnpenetrableBuoyant()
    {
        return BITMASK_IS_1(options, OPTION_b_BUOYANT) && BITMASK_IS_1(options, OPTION_u_INVULNERABLE);
    }

    static const unsigned int OPTION_c_CONTACT           = BITMASK(1);
    static const unsigned int OPTION_b_BUOYANT           = BITMASK(2);
    static const unsigned int OPTION_p_10xTOUGHER        = BITMASK(3);
    static const unsigned int OPTION_u_INVULNERABLE      = BITMASK(4);
    static const unsigned int OPTION_s_BUOYANT_NO_DRAG   = BITMASK(5);
    static const unsigned int OPTION_r_BUOYANT_ONLY_DRAG = BITMASK(6);

    Node::Ref nodes[3];
    unsigned int options;
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
    Tie();

    static const char OPTION_n_FILLER = 'n';
    static const char OPTION_v_FILLER = 'v';
    static const char OPTION_i_INVISIBLE = 'i';
    static const char OPTION_s_NO_SELF_LOCK = 's';

    Node::Ref root_node;
    float max_reach_length;
    float auto_shorten_rate;
    float min_length;
    float max_length;
    bool is_invisible;
    bool disable_self_lock;
    float max_stress;
    std::shared_ptr<BeamDefaults> beam_defaults;
    int detacher_group;
    int group;
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
    static const std::string CONTROL_LEGAL_FLAGS;

    Wing();

    enum Control
    {
        CONTROL_n_NONE                  = 'n',
        CONTROL_a_RIGHT_AILERON         = 'a',
        CONTROL_b_LEFT_AILERON          = 'b',
        CONTROL_f_FLAP                  = 'f',
        CONTROL_e_ELEVATOR              = 'e',
        CONTROL_r_RUDDER                = 'r',
        CONTROL_S_RIGHT_HAND_STABILATOR = 'S',
        CONTROL_T_LEFT_HAND_STABILATOR  = 'T',
        CONTROL_c_RIGHT_ELEVON          = 'c',
        CONTROL_d_LEFT_ELEVON           = 'd',
        CONTROL_g_RIGHT_FLAPERON        = 'g',
        CONTROL_h_LEFT_FLAPERON         = 'h',
        CONTROL_U_RIGHT_HAND_TAILERON   = 'U',
        CONTROL_V_LEFT_HAND_TAILERON    = 'V',
        CONTROL_i_RIGHT_RUDDERVATOR     = 'i',
        CONTROL_j_LEFT_RUDDERVATOR      = 'j',

        CONTROL_INVALID                 = 0xFFFFFFFF
    };

    Node::Ref nodes[8];
    float tex_coords[8];
    Control control_surface;
    float chord_point;
    float min_deflection;
    float max_deflection;
    Ogre::String airfoil;
    float efficacy_coef;
};

/* -------------------------------------------------------------------------- */
/* Root document                                                              */
/* -------------------------------------------------------------------------- */

struct File
{
    /// Group of elements of same type, formed by i.e. ';grp:NAME' comments in nodews/beams
    struct EditorGroup
    {
        EditorGroup(const char* _name): name(_name) {}
        std::string name;
    }; // more attributes may be needed...

    /** Modular part of vehicle (part of file wrapped in 'section ~ end_section' tags)
    */
    struct Module
    {
        Module(Ogre::String const & name);

        Ogre::String name;

        Ogre::String                       help_panel_material_name;
        std::vector<unsigned int>          contacter_nodes;

        /* Sections*/
        std::vector<Airbrake>              airbrakes;
        std::vector<Animator>              animators;
        std::shared_ptr<AntiLockBrakes>    anti_lock_brakes;
        std::vector<Axle>                  axles;
        std::vector<Beam>                  beams;
        std::vector<EditorGroup>           beam_editor_groups; // Originally ';grp:NAME' comments from Editorizer tool
        std::shared_ptr<Brakes>            brakes;
        std::vector<Camera>                cameras;
        std::vector<CameraRail>            camera_rails;
        std::vector<CollisionBox>          collision_boxes;
        std::vector<Cinecam>               cinecam;
        std::vector<Command2>              commands_2; /* sections 'commands' & 'commands2' are unified */
        std::shared_ptr<CruiseControl>     cruise_control;
        std::vector<Node::Ref>             contacters;
        std::shared_ptr<Engine>            engine;
        std::shared_ptr<Engoption>         engoption;
        std::shared_ptr<Engturbo>          engturbo;
        std::vector<Exhaust>               exhausts;
        std::shared_ptr<ExtCamera>         ext_camera;
        std::vector<Node::Ref>              fixes;
        std::vector<Flare2>                flares_2;
        std::vector<std::shared_ptr<Flexbody>>	flexbodies;
        std::vector<FlexBodyWheel>         flex_body_wheels;
        std::vector<Fusedrag>              fusedrag;
        std::shared_ptr<Globals>           globals;
        std::shared_ptr<GuiSettings>       gui_settings;
        std::vector<Hook>                  hooks;
        std::vector<Hydro>                 hydros;
        std::vector<InterAxle>             interaxles;
        std::vector<Lockgroup>             lockgroups;
        std::vector<ManagedMaterial>       managed_materials;
        std::vector<MaterialFlareBinding>  material_flare_bindings;
        std::vector<MeshWheel>             mesh_wheels;
        std::vector<Node>                  nodes; /* Nodes and Nodes2 are unified in this parser */
        std::vector<EditorGroup>           node_editor_groups; // Originally ';grp:NAME' comments from Editorizer tool
        std::vector<NodeCollision>         node_collisions;
        std::vector<Particle>              particles;
        std::vector<Pistonprop>            pistonprops;
        std::vector<Prop>                  props;
        std::vector<RailGroup>             railgroups; 
        std::vector<Ropable>               ropables;
        std::vector<Rope>                  ropes;
        std::vector<Rotator>               rotators;
        std::vector<Rotator2>              rotators_2;
        std::vector<Screwprop>             screwprops;
        std::vector<Script>                scripts;
        std::vector<Shock>                 shocks;
        std::vector<Shock2>                shocks_2;
        std::vector<Shock3>                shocks_3;
        SkeletonSettings                   skeleton_settings;
        std::vector<SlideNode>             slidenodes;
        std::shared_ptr<SlopeBrake>        slope_brake;
        std::vector<SoundSource>           soundsources;
        std::vector<SoundSource2>          soundsources2;
        SpeedLimiter                       speed_limiter;
        Ogre::String                       submeshes_ground_model_name;
        std::vector<Submesh>               submeshes;
        std::vector<Tie>                   ties;
        std::shared_ptr<TorqueCurve>       torque_curve;
        std::shared_ptr<TractionControl>   traction_control;
        std::shared_ptr<TransferCase>      transfer_case;
        std::vector<Trigger>               triggers;
        std::vector<Turbojet>              turbojets;
        std::vector<Turboprop2>            turboprops_2;
        std::vector<VideoCamera>           videocameras;
        std::vector<WheelDetacher>         wheeldetachers;
        std::vector<Wheel>                 wheels;
        std::vector<Wheel2>                wheels_2;
        std::vector<Wing>                  wings;
    };

    File();

    /** IMPORTANT! If you add a value here, you must also modify Regexes::IDENTIFY_KEYWORD, it relies on numeric values of this enum. */
    enum Keyword
    {
        KEYWORD_ADD_ANIMATION = 1,
        //KEYWORD_ADVDRAG, // not supported yet
        KEYWORD_AIRBRAKES,
        KEYWORD_ANIMATORS,
        KEYWORD_ANTI_LOCK_BRAKES,
        KEYWORD_AXLES,
        KEYWORD_AUTHOR,
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
        KEYWORD_CONTACTERS,
        KEYWORD_CRUISECONTROL,
        KEYWORD_DESCRIPTION,
        KEYWORD_DETACHER_GROUP,
        KEYWORD_DISABLEDEFAULTSOUNDS,
        KEYWORD_ENABLE_ADVANCED_DEFORM,
        KEYWORD_END,
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
        KEYWORD_FORWARDCOMMANDS,
        KEYWORD_FUSEDRAG,
        KEYWORD_GLOBALS,
        KEYWORD_GUID,
        KEYWORD_GUISETTINGS,
        KEYWORD_HELP,
        KEYWORD_HIDE_IN_CHOOSER,
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
        KEYWORD_NODECOLLISION,
        KEYWORD_NODES,
        KEYWORD_NODES2,
        KEYWORD_PARTICLES,
        KEYWORD_PISTONPROPS,
        KEYWORD_PROP_CAMERA_MODE,
        KEYWORD_PROPS,
        KEYWORD_RAILGROUPS,
        KEYWORD_RESCUER,
        KEYWORD_RIGIDIFIERS,
        KEYWORD_ROLLON,
        KEYWORD_ROPABLES,
        KEYWORD_ROPES,
        KEYWORD_ROTATORS,
        KEYWORD_ROTATORS2,
        KEYWORD_SCREWPROPS,
        KEYWORD_SCRIPTS,
        KEYWORD_SECTION,
        KEYWORD_SECTIONCONFIG,
        KEYWORD_SET_BEAM_DEFAULTS,
        KEYWORD_SET_BEAM_DEFAULTS_SCALE,
        KEYWORD_SET_COLLISION_RANGE,
        KEYWORD_SET_DEFAULT_MINIMASS,
        KEYWORD_SET_INERTIA_DEFAULTS,
        KEYWORD_SET_MANAGEDMATS_OPTIONS,
        KEYWORD_SET_NODE_DEFAULTS,
        KEYWORD_SET_SHADOWS,
        KEYWORD_SET_SKELETON_SETTINGS,
        KEYWORD_SHOCKS,
        KEYWORD_SHOCKS2,
        KEYWORD_SHOCKS3,
        KEYWORD_SLIDENODE_CONNECT_INSTANT,
        KEYWORD_SLIDENODES,
        KEYWORD_SLOPE_BRAKE,
        KEYWORD_SOUNDSOURCES,
        KEYWORD_SOUNDSOURCES2,
        //KEYWORD_SOUNDSOURCES3, // not supported yet
        KEYWORD_SPEEDLIMITER,
        KEYWORD_SUBMESH,
        KEYWORD_SUBMESH_GROUNDMODEL,
        KEYWORD_TEXCOORDS,
        KEYWORD_TIES,
        KEYWORD_TORQUECURVE,
        KEYWORD_TRACTION_CONTROL,
        KEYWORD_TRANSFER_CASE,
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

    enum Section
    {
        SECTION_AIRBRAKES,
        SECTION_AUTHOR,
        SECTION_ANIMATORS,
        SECTION_ANTI_LOCK_BRAKES,
        SECTION_AXLES,
        SECTION_BEAMS,
        SECTION_BRAKES,
        SECTION_CAMERAS,
        SECTION_CAMERA_RAIL,
        SECTION_CINECAM,
        SECTION_COLLISION_BOXES,
        SECTION_COMMANDS,
        SECTION_COMMANDS_2,
        SECTION_CONTACTERS,
        SECTION_ENGINE,
        SECTION_ENGOPTION,
        SECTION_ENGTURBO,
        SECTION_EXHAUSTS,
        SECTION_FIXES,
        SECTION_FLARES,
        SECTION_FLARES_2,
        SECTION_FLEXBODIES,
        SECTION_FLEX_BODY_WHEELS,	
        SECTION_FUSEDRAG,
        SECTION_GLOBALS,
        SECTION_GUI_SETTINGS,
        SECTION_HELP,
        SECTION_HOOKS,
        SECTION_HYDROS,
        SECTION_INTERAXLES,
        SECTION_LOCKGROUPS,
        SECTION_MANAGED_MATERIALS,
        SECTION_MAT_FLARE_BINDINGS,
        SECTION_MESH_WHEELS,
        SECTION_MESH_WHEELS_2,
        SECTION_MINIMASS,
        SECTION_NODES,
        SECTION_NODES_2,
        SECTION_NODE_COLLISION,
        SECTION_PARTICLES,
        SECTION_PISTONPROPS,
        SECTION_PROPS,
        SECTION_RAILGROUPS,
        SECTION_ROPABLES,
        SECTION_ROPES,
        SECTION_ROTATORS,
        SECTION_ROTATORS_2,
        SECTION_SCREWPROPS,
        SECTION_SCRIPTS,
        SECTION_SHOCKS,
        SECTION_SHOCKS_2,
        SECTION_SHOCKS_3,
        SECTION_SLIDENODES,
        SECTION_SOUNDSOURCES,
        SECTION_SOUNDSOURCES2,
        SECTION_SUBMESH,
        SECTION_SLOPE_BRAKE,
        SECTION_TIES,
        SECTION_TORQUE_CURVE,
        SECTION_TRACTION_CONTROL,
        SECTION_TRANSFER_CASE,
        SECTION_TRIGGERS,
        SECTION_TRUCK_NAME, ///< The very start of file	
        SECTION_TURBOJETS,
        SECTION_TURBOPROPS,
        SECTION_TURBOPROPS_2,
        SECTION_VIDEO_CAMERA,
        SECTION_WHEELDETACHERS,
        SECTION_WHEELS,
        SECTION_WHEELS_2,
        SECTION_WINGS,

        SECTION_NONE,       ///< Right after rig name, for example.

        SECTION_INVALID = 0xFFFFFFFF
    };

    enum Subsection
    {
        SUBSECTION_NONE = 0,

        SUBSECTION__FLEXBODIES__PROPLIKE_LINE,
        SUBSECTION__FLEXBODIES__FORSET_LINE,

        SUBSECTION__SUBMESH__TEXCOORDS,
        SUBSECTION__SUBMESH__CAB,

        SUBSECTION_INVALID = 0xFFFFFFFF
    };

    static const char * SubsectionToString(Subsection subsection);

    static const char * SectionToString(Section section);

    static const char * KeywordToString(Keyword keyword);

    unsigned int file_format_version;
    Ogre::String guid;
    std::vector<Ogre::String> description;
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
    float collision_range;

    // File hash
    std::string hash;

    // Vehicle modules (caled 'sections' in truckfile doc)
    std::shared_ptr<Module> root_module; ///< Required to exist. `shared_ptr` is used for unified handling with other modules.
    std::map< Ogre::String, std::shared_ptr<Module> > user_modules;

    // File sections
    std::vector<Author> authors;
    std::shared_ptr<Fileinfo> file_info;
    std::shared_ptr<MinimassPreset> global_minimass;
    bool minimass_skip_loaded_nodes; //!< Only apply minimum mass to nodes without "L" option. Global effect.
};

} // namespace RigDef
