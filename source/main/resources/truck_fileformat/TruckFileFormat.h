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
    @file
    @author Petr Ohlidal
    @date   12/2013
    @brief Truck file format, see ReadMe.txt and https://docs.rigsofrods.org/vehicle-creation/fileformat-truck/

    Values are initialised to their defaults, or 0 if there's no default.
    Members prefixed by _ are helper flags which mark special values or missing values.
*/

#pragma once

#include "Application.h"
#include "BitFlags.h"
#include "TruckCommon.h"
#include "TruckPresets.h"
#include "TruckNodesBeams.h"
#include "SimData.h"

#include <list>
#include <memory>
#include <vector>
#include <string>
#include <OgreString.h>
#include <OgreVector3.h>
#include <OgreStringConverter.h>

namespace RoR {
namespace Truck {

typedef std::shared_ptr<Document> DocumentPtr;
typedef std::shared_ptr<Module> ModulePtr;

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
    enum MapMode
    {
        MAP_MODE_OFF = 1,
        MAP_MODE_SIMPLE,
        MAP_MODE_ZOOM,

        MAP_MODE_INVALID = 0xFFFFFFFF,
    };

    std::string tacho_material = "-1";
    std::string speedo_material = "-1";
    float speedo_highest_kph = -1.f;
    int use_max_rpm = -1;
    std::string help_material = "-1";
    MapMode interactive_overview_map_mode = MAP_MODE_INVALID;
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
    Node::Ref wheels[2][2];
    std::vector<DifferentialType> options; //!< Order matters!
};

/* -------------------------------------------------------------------------- */
/* Section INTERAXLES                                                         */
/* -------------------------------------------------------------------------- */

struct InterAxle
{
    int a1 = 0;
    int a2 = 0;
    std::vector<DifferentialType> options; //!< Order matters!
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

struct FileInfo
{
    std::string unique_id;
    int category_id = -1;
    int file_version = 0;
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

    enum Type
    {
        TYPE_f_HEADLIGHT     = 'f',
        TYPE_b_BRAKELIGHT    = 'b',
        TYPE_l_LEFT_BLINKER  = 'l',
        TYPE_r_RIGHT_BLINKER = 'r',
        TYPE_R_REVERSE_LIGHT = 'R',
        TYPE_u_USER          = 'u',

        TYPE_INVALID         = 0xFFFFFFFF
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
    int cameramode = 0;
};

/* -------------------------------------------------------------------------- */
/* Section FORSET                                                             */
/* -------------------------------------------------------------------------- */

typedef std::vector<Node::Range> ForSet;

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


/// Section 'rotators'
struct Rotator
{
    Node::Ref axis_nodes[2];
    Node::Ref base_plate_nodes[4];
    Node::Ref rotating_plate_nodes[4];

    float rate = 0.f;
    int spin_left_key = 0;
    int spin_right_key = 0;
    OptionalInertia inertia;
    float engine_coupling = 1.f;
    bool needs_engine = false;
};

/// Section 'rotators2'
struct Rotator2: public Rotator
{
    float rotating_force = ROTATOR_FORCE_DEFAULT;
    float tolerance = ROTATOR_TOLERANCE_DEFAULT;
    std::string description;
};



/* -------------------------------------------------------------------------- */
/* Section LOCKGROUPS                                                         */
/* -------------------------------------------------------------------------- */

struct Lockgroup
{
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
    Type type;
    Ogre::String diffuse_map;
    Ogre::String damaged_diffuse_map;
    Ogre::String specular_map;

    // Presets
    int managed_mat_options = -1;

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
    BITMASK_PROPERTY( constraint_flags, 1, CONSTRAINT_ATTACH_ALL     , HasConstraint_a_AttachAll     , SetConstraint_a_AttachAll     )   
    BITMASK_PROPERTY( constraint_flags, 2, CONSTRAINT_ATTACH_FOREIGN , HasConstraint_f_AttachForeign , SetConstraint_f_AttachForeign )
    BITMASK_PROPERTY( constraint_flags, 3, CONSTRAINT_ATTACH_SELF    , HasConstraint_s_AttachSelf 	 , SetConstraint_s_AttachSelf	 )
    BITMASK_PROPERTY( constraint_flags, 4, CONSTRAINT_ATTACH_NONE    , HasConstraint_n_AttachNone 	 , SetConstraint_n_AttachNone	 )

    Node::Ref slide_node;
    std::vector<Node::Range> rail_node_ranges;
    unsigned int constraint_flags;
    int editor_group_id = -1;

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
    float max_speed = -1.f;
};

/* -------------------------------------------------------------------------- */
/* Section CAB                                                            */
/* -------------------------------------------------------------------------- */

struct Cab
{
    Node::Ref nodes[3];
    char type = 'n'; // 'n' = dummy type
};

/* -------------------------------------------------------------------------- */
/* Section TEXCOORDS                                                            */
/* -------------------------------------------------------------------------- */

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

/// Special comment ';grp:'
struct EditorGroup
{
    EditorGroup(const char* _name, Keyword sect): name(_name), section(sect) {}
    std::string name;
    Keyword section;
};

/// Section 'minimass' - does not affect 'set_default_minimass'!
struct Minimass: Element
{
    float min_mass = -1; //!< minimum node mass in Kg
    int option_skip_loaded_nodes = -1;
};

/// A segment of the truck file; may be created explicitly by 'section/end_section', otherwise is created implicitly.
struct Module
{
    // Metadata
    Ogre::StringVector                 sectionconfigs;             //!< Keyword 'section', values refer to 'sectionconfig'
    bool                               defined_explicitly = false; //!< Created using 'section/end_section'?

    // Informational
    std::vector<FileInfo>              fileinfo;
    std::vector<Author>                authors;
    Ogre::StringVector                 description;                //!< Keyword 'description/end_description'
    std::vector<int>                   fileformatversion;

    // Sequential ordering
    std::vector<SeqSection>            sequence;                   //!< Just references to other data arrays (most, but not all of them).

    // Presets
    std::vector<BeamDefaults>          beam_defaults;              //!< Keyword 'set_beam_defaults'
    std::vector<BeamDefaultsScale>     beam_defaults_scale;        //!< Keyword 'set_beam_defaults_scale'
    std::vector<CollisionRangePreset>  collision_range_preset;     //!< Keyword 'set_collision_range'
    std::vector<DetacherGroupPreset>   detacher_group_preset;      //!< Keyword 'detacher_group'
    std::vector<float>                 default_minimass;           //!< Keyword 'set_default_minimass' - minimum node mass in Kg
    std::vector<InertiaDefaults>       inertia_defaults;           //!< Keyword 'set_inertia_defaults'
    std::vector<ManagedMatOptions>     managed_mat_options;        //!< Keyword 'set_managedmaterials_options'
    std::vector<NodeDefaults>          node_defaults;              //!< Keyword 'set_node_defaults'
    std::vector<SkeletonSettings>      skeleton_settings;          //!< Keyword 'set_skeleton_display'
    std::vector<EditorGroup>           editor_groups;              //!< Special comment ';grp:'

    // Modifiers
    std::vector<int>                   prop_camera_mode;
    std::vector<int>                   flexbody_camera_mode;
    std::vector<bool>                  backmesh;
    std::vector<Animation>             add_animation;

    // Node/beam definitions
    std::vector<Node>                  nodes;                      //!< Keywords 'nodes*'
    std::vector<Animator>              animators;
    std::vector<Beam>                  beams;
    std::vector<Command2>              commands_2;                 //!< Keywords 'commands/commands2'
    std::vector<Hydro>                 hydros;
    std::vector<Rope>                  ropes;
    std::vector<Shock>                 shocks;
    std::vector<Shock2>                shocks_2;
    std::vector<Shock3>                shocks_3;
    std::vector<Tie>                   ties;
    std::vector<Trigger>               triggers;

    // Node/beam generators
    std::vector<Cinecam>               cinecam;
    std::vector<FlexBodyWheel>         flex_body_wheels;
    std::vector<MeshWheel>             mesh_wheels;
    std::vector<Wheel>                 wheels;
    std::vector<Wheel2>                wheels_2;

    // Special node settings
    std::vector<Node::Ref>             contacters;
    std::vector<Node::Ref>             fixes;
    std::vector<Hook>                  hooks;
    std::vector<SlideNode>             slidenodes;
    std::vector<ExtCamera>             extcamera;
    std::vector<NodeCollision>         node_collisions;
    std::vector<Ropable>               ropables;
    std::vector<Lockgroup>             lockgroups;
    std::vector<RailGroup>             railgroups;

    // Drivetrain
    std::vector<Axle>                  axles;
    std::vector<AntiLockBrakes>        anti_lock_brakes;
    std::vector<Brakes>                brakes;
    std::vector<CruiseControl>         cruise_control;
    std::vector<Engine>                engine;
    std::vector<Engoption>             engoption;
    std::vector<Engturbo>              engturbo;
    std::vector<InterAxle>             interaxles;
    std::vector<SpeedLimiter>          speed_limiter;
    std::vector<TorqueCurve>           torque_curve;           //!< Not sequential, at most 1 in module
    std::vector<TractionControl>       traction_control;
    std::vector<TransferCase>          transfer_case;

    // UI
    std::vector<std::string>           help;
    std::vector<GuiSettings>           gui_settings;           //!< Not sequential, at most 1 in module

    // Aerodynamics
    std::vector<Airbrake>              airbrakes;
    std::vector<Fusedrag>              fusedrag;
    std::vector<Pistonprop>            pistonprops;
    std::vector<Turbojet>              turbojets;
    std::vector<Turboprop2>            turboprops_2;
    std::vector<Wing>                  wings;

    // Hydrodynamics
    std::vector<Screwprop>             screwprops;

    // General physics
    std::vector<Camera>                cameras; //!< Important for physics, define reference XYZ axes for whole vehicle (and camera views).
    std::vector<CollisionBox>          collision_boxes;
    std::vector<Globals>               globals;
    std::vector<Rotator>               rotators;
    std::vector<Rotator2>              rotators_2;
    std::vector<SlopeBrake>            slope_brake;
    std::vector<std::string>           submesh_groundmodel;
    std::vector<Minimass>              minimass; //!< Does not affect 'set_default_minimas' presets, only provides fallback for nodes where 'set_default_minimass' was not used (for backwards compatibility).
    std::vector<Texcoord>              texcoords;
    std::vector<Cab>                   cab; //!< Collision cab triangles

    // Look and feel
    std::vector<CameraRail>            camera_rails;
    std::vector<Exhaust>               exhausts;
    std::vector<Flare2>                flares_2;
    std::vector<Flexbody>              flexbodies;
    std::vector<ForSet>                forset;
    std::vector<ManagedMaterial>       managed_materials;
    std::vector<MaterialFlareBinding>  material_flare_bindings;
    std::vector<Particle>              particles;
    std::vector<Prop>                  props;
    std::vector<SoundSource>           soundsources;
    std::vector<SoundSource2>          soundsources2;
    std::vector<VideoCamera>           videocameras;
    std::vector<WheelDetacher>         wheeldetachers;
};

struct Document
{
    static const char* KeywordToString(Keyword keyword);

    std::vector<Truck::ModulePtr> modules;        //!< Partitions of truck file (keyword 'section')
    Ogre::StringVector            sectionconfig;

    std::string  guid;
    std::string  name;
    std::string  hash;
};

} // namespace Truck
} // namespace RoR
