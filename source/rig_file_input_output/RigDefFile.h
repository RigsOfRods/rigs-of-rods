/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file RigDefFile.h
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

#include <list>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <OgreString.h>
#include <OgreVector3.h>
#include <OgreStringConverter.h>

namespace RigDef 
{

/* -------------------------------------------------------------------------- */
/* Utility
/* -------------------------------------------------------------------------- */

struct CameraSettings
{
	CameraSettings():
		mode(MODE_ALWAYS),
		cinecam_index(0)
	{}

	enum Mode
	{
		MODE_BEGIN    = -9999,
		MODE_ALWAYS   = -2,
		MODE_EXTERNAL = -1,
		MODE_CINECAM  = 1,
		MODE_END      = 9999,
		MODE_INVALID  = 0xFFFFFFFF
	};

	Mode mode;
	unsigned int cinecam_index;
};

/* -------------------------------------------------------------------------- */
/* Directive SET_NODE_DEFAULTS
/* -------------------------------------------------------------------------- */

struct NodeDefaults
{
	NodeDefaults():
		load_weight(-1.f),
		friction(1),
		volume(1),
		surface(1),
		options(0)
	{}

	float load_weight;
	float friction;
	float volume;
	float surface;
	unsigned int options; ///< Bit flags
};

/* -------------------------------------------------------------------------- */
/* Directive SET_BEAM_DEFAULTS_SCALE
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
/* Directive SET_BEAM_DEFAULTS
/* -------------------------------------------------------------------------- */

struct BeamDefaults
{
	BeamDefaults():
		springiness(9000000.f),
		damping_constant(12000.f),
		deformation_threshold_constant(400000.f),
		visual_beam_diameter(0.05f),
		beam_material_name("tracks/beam"),
		plastic_deformation_coefficient(0.f), /* This is a default */
		breaking_threshold_constant(1000000.f),
		_user_specified_fields(0),
		_enable_advanced_deformation(false),
		_is_user_defined(false)
	{}

	static const unsigned int PARAM_SPRINGINESS                = BITMASK(1);
	static const unsigned int PARAM_DAMPING_CONSTANT           = BITMASK(2);
	static const unsigned int PARAM_DEFORM_THRESHOLD_CONSTANT  = BITMASK(3);
	static const unsigned int PARAM_BREAK_THRESHOLD_CONSTANT   = BITMASK(4);
	static const unsigned int PARAM_BEAM_DIAMETER              = BITMASK(5);
	static const unsigned int PARAM_BEAM_MATERIAL              = BITMASK(6);
	static const unsigned int PARAM_PLASTIC_DEFORM_COEFFICIENT = BITMASK(7);

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
		return breaking_threshold_constant * scale.breaking_threshold_constant;
	}

	float springiness;
	float damping_constant;
	float deformation_threshold_constant;
	float breaking_threshold_constant;
	float visual_beam_diameter;
	Ogre::String beam_material_name;
	float plastic_deformation_coefficient;
	unsigned int _user_specified_fields; ///< Bit flags
	bool _enable_advanced_deformation; ///< Informs whether "enable_advanced_deformation" directive preceded these defaults.
	bool _is_user_defined; ///< Informs whether these data were read from "set_beam_defaults" directive or filled in by the parser on startup.
	BeamDefaultsScale scale;
};

/* -------------------------------------------------------------------------- */
/* Sections NODES, NODES_2
/* -------------------------------------------------------------------------- */

/** Nodes and Nodes2 are unified in this parser.
*/
struct Node
{
	/** Abstract node ID: can be numeric or string identifier.
	*/
	class Id
	{

	public:

		Id():
			m_id_num(INVALID_ID_VALUE)
		{}

		Id(unsigned int id_num):
			m_id_num(id_num)
		{}

		Id(Ogre::String const & id_str):
			m_id_num(0),
			m_id_str(id_str)
		{}

		static const unsigned int INVALID_ID_VALUE = 0xFFFFFFFF;

		bool IsValid()
		{
			return (m_id_num != INVALID_ID_VALUE);
		}

		void Invalidate()
		{
			SetNum(INVALID_ID_VALUE);
		}

		void SetNum(unsigned int id_num)
		{
			m_id_num = id_num;
			m_id_str.clear();
		}

		void SetStr(Ogre::String const & id_str)
		{
			m_id_str = id_str;
			m_id_num = 0;
		}

		Ogre::String const & Str()
		{
			return m_id_str;
		}

		unsigned int Num()
		{
			return m_id_num;
		}

		bool Compare(Id const & rhs)
		{
			if (m_id_str.empty() && rhs.m_id_str.empty())
			{
				return m_id_num == rhs.m_id_num;
			}
			else
			{
				return m_id_str == rhs.m_id_str;
			}
		}

		bool operator!=(Id const & rhs)
		{
			return ! Compare(rhs);
		}

		bool operator==(Id const & rhs)
		{
			return Compare(rhs);
		}

		Ogre::String ToString()
		{
			if (! m_id_str.empty())
			{
				return m_id_str;
			}
			else
			{
				return Ogre::StringConverter::toString(m_id_num);
			}
		}

	private:

		unsigned int m_id_num;
		Ogre::String m_id_str;
	};

	struct Range
	{
		Range(Node::Id start, Node::Id end):
			start(start),
			end(end)
		{}

		Range(Node::Id single):
			start(single),
			end(single)
		{}

		Range(unsigned int single_number):
			start(Node::Id(single_number)),
			end(Node::Id(single_number))
		{}

		bool IsRange()
		{
			return start != end;
		}

		void SetSingle(Node::Id const & node)
		{
			start = node;
			end = node;
		}

		Node::Id start;
		Node::Id end;
	};

	Node():
		position(Ogre::Vector3::ZERO),
		options(0),
		load_weight_override(0),
		_has_load_weight_override(false),
		detacher_group(0) /* Global detacher group */
	{}

	static const unsigned int OPTION_n_MOUSE_GRAB         = BITMASK(1);
	static const unsigned int OPTION_m_NO_MOUSE_GRAB      = BITMASK(2);
	static const unsigned int OPTION_f_NO_SPARKS          = BITMASK(3);
	static const unsigned int OPTION_x_EXHAUST_POINT      = BITMASK(4);
	static const unsigned int OPTION_y_EXHAUST_DIRECTION  = BITMASK(5);
	static const unsigned int OPTION_c_NO_GROUND_CONTACT  = BITMASK(6);
	static const unsigned int OPTION_h_HOOK_POINT         = BITMASK(7);
	static const unsigned int OPTION_e_TERRAIN_EDIT_POINT = BITMASK(8);
	static const unsigned int OPTION_b_EXTRA_BUOYANCY     = BITMASK(9);
	static const unsigned int OPTION_p_NO_PARTICLES       = BITMASK(10);
	static const unsigned int OPTION_L_LOG                = BITMASK(11);
	static const unsigned int OPTION_l_LOAD_WEIGHT        = BITMASK(12);

	Id id;
	Ogre::Vector3 position;
	unsigned int options; ///< Bit flags
	float load_weight_override;
	bool _has_load_weight_override;
	boost::shared_ptr<NodeDefaults> node_defaults;
	boost::shared_ptr<BeamDefaults> beam_defaults; /* Needed for hook */
	int detacher_group;
};

/* -------------------------------------------------------------------------- */
/* Directive SET_DEFAULT_INERTIA
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

struct DefaultInertia: Inertia
{
};

struct OptionalInertia: public Inertia
{
	OptionalInertia():
		Inertia(),
		_start_delay_factor_set(false),
		_stop_delay_factor_set(false)
	{}

	bool _start_delay_factor_set;
	bool _stop_delay_factor_set;
};

/* -------------------------------------------------------------------------- */
/* Directive SET_NODE_DEFAULTS
/* -------------------------------------------------------------------------- */

struct ManagedMaterialsOptions
{
	ManagedMaterialsOptions():
		double_sided(false)
	{}

	bool double_sided;
};

/* -------------------------------------------------------------------------- */
/* Directive SET_SHADOWS
/* -------------------------------------------------------------------------- */

struct ShadowOptions
{
	ShadowOptions():
		shadow_mode(0)
	{}

	int shadow_mode;
};

/* -------------------------------------------------------------------------- */
/* Section GLOBALS
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
/* Section GUISETTINGS
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
		MAP_MODE_BEGIN = 0,
		MAP_MODE_OFF = 1,
		MAP_MODE_SIMPLE,
		MAP_MODE_ZOOM,
		MAP_MODE_END,
		MAP_MODE_INVALID = 0xFFFFFFFF,
	};

	const static unsigned int DEFAULT_SPEEDO_MAX = 140;

	Ogre::String tacho_material;
	Ogre::String speedo_material;
	unsigned int speedo_highest_kph;
	bool use_max_rpm;
	Ogre::String help_material;
	MapMode interactive_overview_map_mode;
	std::list<Ogre::String> dashboard_layouts;
	std::list<Ogre::String> rtt_dashboard_layouts;
};

/* -------------------------------------------------------------------------- */
/* Section AIRBRAKES
/* -------------------------------------------------------------------------- */

struct Airbrake
{
	Airbrake():
		offset(Ogre::Vector3::ZERO),
		width(0),
		height(0),
		max_inclination_angle(0),
		texcoord_x1(0),
		texcoord_x2(0),
		texcoord_y1(0),
		texcoord_y2(0),
		lift_coefficient(-1.f)
	{}

	Node::Id reference_node;
	Node::Id x_axis_node;
	Node::Id y_axis_node;
	Node::Id aditional_node;
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
/* Directive ADD_ANIMATION
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

	static const unsigned int SOURCE_AIRSPEED          = BITMASK(1);
	static const unsigned int SOURCE_VERTICAL_VELOCITY = BITMASK(2);
	static const unsigned int SOURCE_ALTIMETER_100K    = BITMASK(3);
	static const unsigned int SOURCE_ALTIMETER_10K     = BITMASK(4);
	static const unsigned int SOURCE_ALTIMETER_1K      = BITMASK(5);
	static const unsigned int SOURCE_ANGLE_OF_ATTACK   = BITMASK(6);
	static const unsigned int SOURCE_FLAP              = BITMASK(7);
	static const unsigned int SOURCE_AIR_BRAKE         = BITMASK(8);
	static const unsigned int SOURCE_ROLL              = BITMASK(9);
	static const unsigned int SOURCE_PITCH             = BITMASK(10);
	static const unsigned int SOURCE_BRAKES            = BITMASK(11);
	static const unsigned int SOURCE_ACCEL             = BITMASK(12);
	static const unsigned int SOURCE_CLUTCH            = BITMASK(13);
	static const unsigned int SOURCE_SPEEDO            = BITMASK(14);
	static const unsigned int SOURCE_TACHO             = BITMASK(15);
	static const unsigned int SOURCE_TURBO             = BITMASK(16);
	static const unsigned int SOURCE_PARKING           = BITMASK(17);
	static const unsigned int SOURCE_SHIFT_LEFT_RIGHT  = BITMASK(18);
	static const unsigned int SOURCE_SHIFT_BACK_FORTH  = BITMASK(19);
	static const unsigned int SOURCE_SEQUENTIAL_SHIFT  = BITMASK(20);
	static const unsigned int SOURCE_SHIFTERLIN        = BITMASK(21);
	static const unsigned int SOURCE_TORQUE            = BITMASK(22);
	static const unsigned int SOURCE_HEADING           = BITMASK(23);
	static const unsigned int SOURCE_DIFFLOCK          = BITMASK(24);
	static const unsigned int SOURCE_BOAT_RUDDER       = BITMASK(25);
	static const unsigned int SOURCE_BOAT_THROTTLE     = BITMASK(26);
	static const unsigned int SOURCE_STEERING_WHEEL    = BITMASK(27);
	static const unsigned int SOURCE_AILERON           = BITMASK(28);
	static const unsigned int SOURCE_ELEVATOR          = BITMASK(29);
	static const unsigned int SOURCE_AIR_RUDDER        = BITMASK(30);
	static const unsigned int SOURCE_PERMANENT         = BITMASK(31);
	static const unsigned int SOURCE_EVENT             = BITMASK(32); /* Full house32 :) */

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
};

/* -------------------------------------------------------------------------- */
/* Section AXLES
/* -------------------------------------------------------------------------- */

struct Axle
{
	Axle():
		options(0)
	{}

	static const unsigned int OPTION_o_OPEN   = BITMASK(1);
	static const unsigned int OPTION_l_LOCKED = BITMASK(2);
	static const unsigned int OPTION_s_SPLIT  = BITMASK(3);

	Node::Id wheels[2][2];
	unsigned int options;
};

/* -------------------------------------------------------------------------- */
/* Section BEAMS
/* -------------------------------------------------------------------------- */

struct Beam
{
	Beam():
		options(0),
		extension_break_limit(0), /* This is default */
		_has_extension_break_limit(false),
		detacher_group(0) /* 0 = Default detacher group */
	{}

	static const unsigned int OPTION_i_INVISIBLE = BITMASK(1);
	static const unsigned int OPTION_r_ROPE      = BITMASK(2);
	static const unsigned int OPTION_s_SUPPORT   = BITMASK(3);

	Node::Id nodes[2];
	unsigned int options; ///< Bit flags
	float extension_break_limit;
	bool _has_extension_break_limit;
	int detacher_group;
	boost::shared_ptr<BeamDefaults> defaults;
};

/* -------------------------------------------------------------------------- */
/* Section CAMERAS
/* -------------------------------------------------------------------------- */

struct Camera
{
	Node::Id center_node;
	Node::Id back_node;
	Node::Id left_node;
};

/* -------------------------------------------------------------------------- */
/* Section CAMERARAIL
/* -------------------------------------------------------------------------- */

struct CameraRail
{
	CameraRail()
	{
		nodes.reserve(25);
	}

	std::vector<Node::Id> nodes;
};

/* -------------------------------------------------------------------------- */
/* Section CINECAM
/* -------------------------------------------------------------------------- */

struct Cinecam
{
	Cinecam():
		position(Ogre::Vector3::ZERO),
		spring(8000),
		damping(800)
	{}

	Ogre::Vector3 position;
	Node::Id nodes[8];
	float spring;
	float damping;
	boost::shared_ptr<BeamDefaults> beam_defaults;
	boost::shared_ptr<NodeDefaults> node_defaults;
};

/* -------------------------------------------------------------------------- */
/* Section COLLISIONBOXES
/* -------------------------------------------------------------------------- */

struct CollisionBox
{
	CollisionBox()
	{
		nodes.reserve(25);
	}

	std::vector<Node::Id> nodes;
};

/* -------------------------------------------------------------------------- */
/* Section CRUISECONTROL
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
/* Section AUTHOR
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
/* Inline - section FILEINFO
/* -------------------------------------------------------------------------- */

struct Fileinfo
{
	Fileinfo():
		category_id(0),
		file_version(0), /* Default */
		_has_unique_id(false),
		_has_category_id(false),
		_has_file_version_set(false)
	{}

	Ogre::String unique_id;
	unsigned int category_id;
	unsigned int file_version;
	bool _has_unique_id;
	bool _has_category_id;
	bool _has_file_version_set;
};

/* -------------------------------------------------------------------------- */
/* Section ENGINE
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
/* Section ENGOPTION
/* -------------------------------------------------------------------------- */

struct Engoption
{
	Engoption();

	enum EngineType
	{
		ENGINE_TYPE_BEGIN   =   0,
		ENGINE_TYPE_c_CAR   = 'c',
		ENGINE_TYPE_t_TRUCK = 't',
		ENGINE_TYPE_END     = 999,

		ENGINE_TYPE_INVALID = 0xFFFFFFFF
	};

	float inertia;
	EngineType type;
	float clutch_force;
	bool _clutch_force_use_default;
	float shift_time;       ///< Seconds
	float clutch_time;      ///< Seconds
	float post_shift_time;  ///< Seconds
	float idle_rpm;
	bool _idle_rpm_use_default;
	float stall_rpm;
	float max_idle_mixture;
	float min_idle_mixture;
};

/* -------------------------------------------------------------------------- */
/* Section EXHAUSTS
/* -------------------------------------------------------------------------- */

struct Exhaust
{
	Node::Id reference_node;
	Node::Id direction_node;
	Ogre::String material_name;
};

/* -------------------------------------------------------------------------- */
/* Section EXTCAMERA
/* -------------------------------------------------------------------------- */

struct ExtCamera
{
	ExtCamera():
		mode(MODE_CLASSIC)
	{}

	enum Mode
	{
		MODE_BEGIN,

		MODE_CLASSIC,
		MODE_CINECAM,
		MODE_NODE,

		MODE_END,
		MODE_INVALID = 0xFFFFFFFF
	};

	Mode mode;
	Node::Id node;
};

/* -------------------------------------------------------------------------- */
/* Section BRAKES
/* -------------------------------------------------------------------------- */

struct Brakes
{
	Brakes():
		default_braking_force(30000),
		parking_brake_force(0),
		_parking_brake_force_set(false)
	{}

	float default_braking_force;
	float parking_brake_force;
	bool _parking_brake_force_set;
};

/* -------------------------------------------------------------------------- */
/* Section ANTI_LOCK_BRAKES
/* -------------------------------------------------------------------------- */

struct AntiLockBrakes
{
	AntiLockBrakes();

	static const unsigned int MODE_ON           = BITMASK(1);
	static const unsigned int MODE_OFF          = BITMASK(2);
	static const unsigned int MODE_NO_DASHBOARD = BITMASK(3);
	static const unsigned int MODE_NO_TOGGLE    = BITMASK(4);

	float regulation_force;
	unsigned int min_speed;
	float pulse_per_sec;
	bool _pulse_per_sec_set;
	unsigned int mode; ///< Bit flags
};

/* -------------------------------------------------------------------------- */
/* Section TRACTION_CONTROL
/* -------------------------------------------------------------------------- */

struct TractionControl
{
	TractionControl();

	static const unsigned int MODE_ON           = BITMASK(1);
	static const unsigned int MODE_OFF          = BITMASK(2);
	static const unsigned int MODE_NO_DASHBOARD = BITMASK(3);
	static const unsigned int MODE_NO_TOGGLE    = BITMASK(4);

	float regulation_force;
	float wheel_slip;
	float fade_speed;
	float pulse_per_sec;
	unsigned int mode; ///< Bit flags
};

/* -------------------------------------------------------------------------- */
/* Inline-section SLOPE_BRAKE
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
/* Section WHEELS
/* -------------------------------------------------------------------------- */

/** Syntax-sugar struct to hold enums */
struct Wheels
{
	enum Braking
	{
		BRAKING_BEGIN             = -9999,
		BRAKING_NO                = 0,
		BRAKING_YES               = 1,
		BRAKING_DIRECTIONAL_LEFT  = 2,
		BRAKING_DIRECTIONAL_RIGHT = 3,
		BRAKING_ONLY_FOOT         = 4,
		BRAKING_END               = 9999,

		BRAKING_INVALID           = 0xFFFFFFFF
	};

	enum Propulsion
	{
		PROPULSION_BEGIN    = -9999,
		PROPULSION_NONE     = 0,
		PROPULSION_FORWARD  = 1,
		PROPULSION_BACKWARD = 2,
		PROPULSION_END      = 9999,

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
	Node::Id nodes[2];
	Node::Id rigidity_node;
	Wheels::Braking braking;
	Wheels::Propulsion propulsion;
	Node::Id reference_arm_node;
	float mass;
	boost::shared_ptr<NodeDefaults> node_defaults;
	boost::shared_ptr<BeamDefaults> beam_defaults;
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
/* Section WHEELS_2
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
/* Section MESHWHEELS
/* -------------------------------------------------------------------------- */

struct MeshWheel: BaseWheel
{
	MeshWheel():
		BaseWheel(),
		side(SIDE_INVALID),
		rim_radius(0),
		tyre_radius(0)
	{}

	enum Side
	{
		SIDE_BEGIN = 0,
		SIDE_RIGHT = 'r',
		SIDE_LEFT = 'l',
		SIDE_END = 9999,
		
		SIDE_INVALID = 0xFFFFFFFF
	};

	Side side;
	Ogre::String mesh_name;
	Ogre::String material_name;
	float rim_radius;
	float tyre_radius;
	float spring;
	float damping;
};

/* -------------------------------------------------------------------------- */
/* Section MESHWHEELS_2
/* -------------------------------------------------------------------------- */

struct MeshWheel2: BaseWheel2
{
	MeshWheel2():
		side(MeshWheel::SIDE_INVALID)
	{}

	/* Rim is set-up by `beam_defaults`, params in section are for tire. */
	MeshWheel::Side side;
	Ogre::String mesh_name;
	Ogre::String material_name;
};

/* -------------------------------------------------------------------------- */
/* Section FLARES FLARES2
/* -------------------------------------------------------------------------- */

/** Used for both 'flares' and 'flares_2' sections
*/
struct Flare2
{
	Flare2():
		x(0),
		y(0),
		offset(0, 0, 1), /* Section 'flares(1)' has offset.z hardcoded to 1 */
		type(TYPE_f_HEADLIGHT),
		control_number(-1),
		blink_delay_milis(500),
		size(-1)
	{}

	enum Type
	{
		TYPE_BEGIN           = 0,
		TYPE_f_HEADLIGHT     = 'f',
		TYPE_b_BRAKELIGHT    = 'b',
		TYPE_l_LEFT_BLINKER  = 'l',
		TYPE_r_RIGHT_BLINKER = 'r',
		TYPE_R_REVERSE_LIGHT = 'R',
		TYPE_u_USER          = 'u',
		TYPE_END             = 9999,

		TYPE_INVALID         = 0xFFFFFFFF
	};

	Node::Id reference_node;
	float x;
	float y;
	Ogre::Vector3 offset;
	Type type;
	int control_number;
	int blink_delay_milis;
	float size;
	Ogre::String material_name;
};

/* -------------------------------------------------------------------------- */
/* Section FLEXBODIES
/* -------------------------------------------------------------------------- */

struct Flexbody
{
	Flexbody():
		offset(Ogre::Vector3::ZERO),
		rotation(Ogre::Vector3::ZERO)
	{
		forset.reserve(10);
	}

	Node::Id reference_node;
	Node::Id x_axis_node;
	Node::Id y_axis_node;
	Ogre::Vector3 offset;
	Ogre::Vector3 rotation;
	Ogre::String mesh_name;
	std::list<Animation> animations;
	std::vector<Node::Range> forset;
	CameraSettings camera_settings;
};

/* -------------------------------------------------------------------------- */
/* Section FLEX_BODY_WHEELS
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
/* Section FUSEDRAG
/* -------------------------------------------------------------------------- */

struct Fusedrag
{
	Fusedrag();

	bool autocalc;
	Node::Id front_node;
	Node::Id rear_node;
	float approximate_width;
	Ogre::String airfoil_name;
	float area_coefficient;
	bool _area_coefficient_set;
};

/* -------------------------------------------------------------------------- */
/* Section HOOKS
/* -------------------------------------------------------------------------- */

struct Hook
{
	Hook();

	static const unsigned int FLAG_SELF_LOCK           = BITMASK(1);
	static const unsigned int FLAG_AUTO_LOCK           = BITMASK(2);
	static const unsigned int FLAG_NO_DISABLE          = BITMASK(3);
	static const unsigned int FLAG_NO_ROPE             = BITMASK(4);
	static const unsigned int FLAG_VISIBLE             = BITMASK(5);

	Node::Id node;
	unsigned int flags;
	float option_hook_range;
	float option_speed_coef;
	float option_max_force;
	int option_hookgroup;
	int option_lockgroup;
	float option_timer;
	float option_minimum_range_meters;
};

/* -------------------------------------------------------------------------- */
/* Section SHOCKS
/* -------------------------------------------------------------------------- */

struct Shock
{
	Shock();

	static const unsigned int OPTION_i_INVISIBLE    = BITMASK(1);
	static const unsigned int OPTION_L_ACTIVE_LEFT  = BITMASK(2); //< Stability active suspension can be made with "L" for suspension on the truck's left and "R" for suspension on the truck's right. 
	static const unsigned int OPTION_R_ACTIVE_RIGHT = BITMASK(3); //< Stability active suspension can be made with "L" for suspension on the truck's left and "R" for suspension on the truck's right. 
	static const unsigned int OPTION_m_METRIC       = BITMASK(4);

	Node::Id nodes[2];
	float spring_rate;         ///< The 'stiffness' of the shock. The higher the value, the less the shock will move for a given bump. 
	float damping;             ///< The 'resistance to motion' of the shock. The best value is given by this equation:  2 * sqrt(suspended mass * springness)
	float short_bound;         ///< Maximum contraction. The shortest length the shock can be, as a proportion of its original length. "0" means the shock will not be able to contract at all, "1" will let it contract all the way to zero length. If the shock tries to shorten more than this value allows, it will become as rigid as a normal beam. 
	float long_bound;          ///< Maximum extension. The longest length a shock can be, as a proportion of its original length. "0" means the shock will not be able to extend at all. "1" means the shock will be able to double its length. Higher values allow for longer extension.
	float precompression;      ///< Changes compression or extension of the suspension when the truck spawns. This can be used to "level" the suspension of a truck if it sags in game. The default value is 1.0. 
	unsigned int options;      ///< Bit flags.
	boost::shared_ptr<BeamDefaults> beam_defaults;
	int detacher_group;
};

/* -------------------------------------------------------------------------- */
/* Section SHOCKS_2
/* -------------------------------------------------------------------------- */

struct Shock2
{
	Shock2();

	static const unsigned int OPTION_i_INVISIBLE        = BITMASK(1);
	static const unsigned int OPTION_s_SOFT_BUMP_BOUNDS = BITMASK(2);  ///< soft bump boundaries, use when shocks reach limiters too often and "jumprebound" (default is hard bump boundaries)
	static const unsigned int OPTION_m_METRIC           = BITMASK(3);  ///< metric values for shortbound/longbound applying to the length of the beam.
	static const unsigned int OPTION_M_ABSOLUTE_METRIC  = BITMASK(4);  ///< Absolute metric values for shortbound/longbound, settings apply without regarding to the original length of the beam.(Use with caution, check ror.log for errors)

	Node::Id nodes[2];
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
	boost::shared_ptr<BeamDefaults> beam_defaults;
	int detacher_group;
};

/* -------------------------------------------------------------------------- */
/* Inline-section SET_SKELETON_DISPLAY
/* -------------------------------------------------------------------------- */

struct SkeletonSettings
{
	SkeletonSettings():
		visibility_range_meters(150.f),
		beam_thickness_meters(0.01f)
	{}

	float visibility_range_meters;
	float beam_thickness_meters;
};

/* -------------------------------------------------------------------------- */
/* Section HYDROS
/* -------------------------------------------------------------------------- */

struct Hydro
{
	Hydro():
		lenghtening_factor(0),
		options(0),
		detacher_group(0)
	{}

	static const unsigned int OPTION_i_INVISIBLE                 = BITMASK(1);

	/* Useful for trucks */

	static const unsigned int OPTION_s_DISABLE_ON_HIGH_SPEED     = BITMASK(2);

	/* Useful for planes: These can be used to control flight surfaces, or to create a thrust vectoring system.  */

	static const unsigned int OPTION_a_INPUT_AILERON             = BITMASK(3);
	static const unsigned int OPTION_r_INPUT_RUDDER              = BITMASK(4);
	static const unsigned int OPTION_e_INPUT_ELEVATOR            = BITMASK(5);
	static const unsigned int OPTION_u_INPUT_AILERON_ELEVATOR    = BITMASK(6);
	static const unsigned int OPTION_v_INPUT_InvAILERON_ELEVATOR = BITMASK(7);
	static const unsigned int OPTION_x_INPUT_AILERON_RUDDER      = BITMASK(8);
	static const unsigned int OPTION_y_INPUT_InvAILERON_RUDDER   = BITMASK(9);
	static const unsigned int OPTION_g_INPUT_ELEVATOR_RUDDER     = BITMASK(10);
	static const unsigned int OPTION_h_INPUT_InvELEVATOR_RUDDER  = BITMASK(11);

	Node::Id nodes[2];
	float lenghtening_factor;
	unsigned int options;
	OptionalInertia inertia;
	boost::shared_ptr<DefaultInertia> inertia_defaults;
	boost::shared_ptr<BeamDefaults> beam_defaults;
	int detacher_group;
};

/* -------------------------------------------------------------------------- */
/* Section ANIMATORS
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

	Node::Id nodes[2];
	float lenghtening_factor;
	unsigned int flags;
	float short_limit;
	float long_limit;
	AeroAnimator aero_animator;
	boost::shared_ptr<DefaultInertia> inertia_defaults;
	boost::shared_ptr<BeamDefaults> beam_defaults;
	int detacher_group;
};

/* -------------------------------------------------------------------------- */
/* Section COMMANDS & COMMANDS_2 (unified)
/* -------------------------------------------------------------------------- */

struct Command2
{
	Command2();

	static const unsigned int OPTION_i_INVISIBLE         = BITMASK(1);
	static const unsigned int OPTION_r_ROPE              = BITMASK(2);
	static const unsigned int OPTION_c_AUTO_CENTER       = BITMASK(3);
	static const unsigned int OPTION_f_NOT_FASTER        = BITMASK(4);
	static const unsigned int OPTION_p_PRESS_ONCE        = BITMASK(5);
	static const unsigned int OPTION_o_PRESS_ONCE_CENTER = BITMASK(6);

	unsigned int _format_version;
	Node::Id nodes[2];
	float shorten_rate;
	float lengthen_rate;
	float max_contraction;
	float max_extension;
	unsigned int contract_key;
	unsigned int extend_key;
	unsigned int options;
	Ogre::String description;
	OptionalInertia inertia;
	float affect_engine;
	bool needs_engine;
	boost::shared_ptr<BeamDefaults> beam_defaults;
	boost::shared_ptr<DefaultInertia> inertia_defaults;
	int detacher_group;

	bool HasOption(unsigned int option)
	{
		return BITMASK_IS_1(options, option);
	}
};

/* -------------------------------------------------------------------------- */
/* Section ROTATORS
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

	Node::Id axis_nodes[2];
	Node::Id base_plate_nodes[4];
	Node::Id rotating_plate_nodes[4];

	float rate;
	unsigned int spin_left_key;
	unsigned int spin_right_key;
	OptionalInertia inertia;
	boost::shared_ptr<DefaultInertia> inertia_defaults;
	float engine_coupling;
	bool needs_engine;
};

/* -------------------------------------------------------------------------- */
/* Section ROTATORS_2
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
/* Section TRIGGERS
/* -------------------------------------------------------------------------- */

struct Trigger
{
	Trigger();

	static const unsigned int OPTION_i_INVISIBLE             = BITMASK(1);
	static const unsigned int OPTION_c_COMMAND_STYLE         = BITMASK(2);
	static const unsigned int OPTION_x_START_OFF             = BITMASK(3);
	static const unsigned int OPTION_b_BLOCK_KEYS            = BITMASK(4);
	static const unsigned int OPTION_B_BLOCK_TRIGGERS        = BITMASK(5);
	static const unsigned int OPTION_A_INV_BLOCK_TRIGGERS    = BITMASK(6);
	static const unsigned int OPTION_s_SWITCH_CMD_NUM        = BITMASK(7);
	static const unsigned int OPTION_h_UNLOCK_HOOKGROUPS_KEY = BITMASK(8);
	static const unsigned int OPTION_H_LOCK_HOOKGROUPS_KEY   = BITMASK(9);
	static const unsigned int OPTION_t_CONTINUOUS            = BITMASK(10);
	static const unsigned int OPTION_E_ENGINE_TRIGGER        = BITMASK(11);

	enum EngineTriggerFunction
	{
		ENGINE_TRIGGER_FUNCTION_BEGIN,
		ENGINE_TRIGGER_FUNCTION_CLUTCH      = 0,
		ENGINE_TRIGGER_FUNCTION_BRAKE       = 1,
		ENGINE_TRIGGER_FUNCTION_ACCELERATOR = 2,
		ENGINE_TRIGGER_FUNCTION_RPM_CONTROL = 3,
		ENGINE_TRIGGER_FUNCTION_SHIFT_UP    = 4, ///< Do not mix with OPTION_t_CONTINUOUS
		ENGINE_TRIGGER_FUNCTION_SHIFT_DOWN  = 5, ///< Do not mix with OPTION_t_CONTINUOUS
		ENGINE_TRIGGER_FUNCTION_END,

		ENGINE_TRIGGER_FUNCTION_INVALID     = 0xFFFFFFFF
	};

	Node::Id nodes[2];
	float contraction_trigger_limit;
	float expansion_trigger_limit;
	unsigned int shortbound_trigger_key;
	unsigned int longbound_trigger_key;
	unsigned int options;
	float boundary_timer;
	unsigned int _engine_trigger_motor_index;
	EngineTriggerFunction _engine_trigger_function;
	boost::shared_ptr<BeamDefaults> beam_defaults;
	int detacher_group;
};

/* -------------------------------------------------------------------------- */
/* Section LOCKGROUPS
/* -------------------------------------------------------------------------- */

struct Lockgroup
{
	Lockgroup()
		:	number(0)
	{
		nodes.reserve(20);
	}

	static const unsigned int LOCKGROUP_DEFAULT        = -1;
	static const unsigned int LOCKGROUP_NOLOCK         = 9999;

	unsigned int number;
	std::vector<Node::Id> nodes;
};

/* -------------------------------------------------------------------------- */
/* Section MANAGEDMATERIALS
/* -------------------------------------------------------------------------- */

struct ManagedMaterial
{
	/* IMPORTANT! Order of these values must match Regexes::IDENTIFY_MANAGED_MATERIAL_TYPE enum from Regexes.h */
	enum Type
	{
		TYPE_BEGIN,

		TYPE_FLEXMESH_STANDARD = 1,
		TYPE_FLEXMESH_TRANSPARENT,
		TYPE_MESH_STANDARD,
		TYPE_MESH_TRANSPARENT,

		TYPE_END,
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
		return damaged_diffuse_map.length() != 0;
	}

	bool HasSpecularMap()
	{
		return specular_map.length() != 0;
	}
};

/* -------------------------------------------------------------------------- */
/* Section MATERIALFLAREBINDINGS
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
/* Section NODECOLLISION
/* -------------------------------------------------------------------------- */

struct NodeCollision
{
	NodeCollision():
		radius(0)
	{}

	Node::Id node;
	float radius;
};

/* -------------------------------------------------------------------------- */
/* Section PARTICLES
/* -------------------------------------------------------------------------- */

struct Particle
{
	Node::Id emitter_node;
	Node::Id reference_node;
	Ogre::String particle_system_name;
};

/* -------------------------------------------------------------------------- */
/* Section PISTONPROPS
/* -------------------------------------------------------------------------- */

struct Pistonprop
{
	Pistonprop():
		turbine_power_kW(0),
		_couple_node_set(false),
		pitch(0)
	{}

	Node::Id reference_node;
	Node::Id axis_node;
	Node::Id blade_tip_nodes[4];
	Node::Id couple_node;
	bool _couple_node_set;
	float turbine_power_kW;
	float pitch;
	Ogre::String airfoil;
};

/* -------------------------------------------------------------------------- */
/* Section PROPS
/* -------------------------------------------------------------------------- */

struct Prop
{
	struct SteeringWheelSpecial
	{
		SteeringWheelSpecial():
			offset(Ogre::Vector3::ZERO), /* This is default */
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
		SPECIAL_BEGIN = 0,
		
		SPECIAL_LEFT_REAR_VIEW_MIRROR = 1,
		SPECIAL_RIGHT_REAR_VIEW_MIRROR,
		SPECIAL_STEERING_WHEEL_LEFT_HANDED,
		SPECIAL_STEERING_WHEEL_RIGHT_HANDED,
		SPECIAL_SPINPROP,
		SPECIAL_PALE,
		SPECIAL_DRIVER_SEAT,
		SPECIAL_DRIVER_SEAT_2,
		SPECIAL_BEACON,
		SPECIAL_REDBEACON,
		SPECIAL_LIGHTBAR,

		SPECIAL_END = 9999,
		SPECIAL_INVALID = 0xFFFFFFFF
	};

	Node::Id reference_node;
	Node::Id x_axis_node;
	Node::Id y_axis_node;
	Ogre::Vector3 offset;
	Ogre::Vector3 rotation;
	Ogre::String mesh_name;
	std::list<Animation> animations;
	CameraSettings camera_settings;
	Special special;
	BeaconSpecial special_prop_beacon;
	SteeringWheelSpecial special_prop_steering_wheel;
};

/* -------------------------------------------------------------------------- */
/* Section RAILGROUPS
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
/* Section Ropables
/* -------------------------------------------------------------------------- */

struct Ropable
{
	Ropable():
		group(-1), /* Default */
		_has_group_set(false),
		multilock(0),
		_has_multilock_set(false)
	{}

	bool use_default_group()
	{
		return group == -1;
	}

	Node::Id node;
	int group;
	bool _has_group_set;
	bool multilock;
	bool _has_multilock_set;
};

/* -------------------------------------------------------------------------- */
/* Section ROPES
/* -------------------------------------------------------------------------- */

struct Rope
{
	Rope():
		invisible(false),
		_has_invisible_set(false),
		detacher_group(0) /* Global detacher group */
	{}

	Node::Id root_node;
	Node::Id end_node;
	bool invisible;
	bool _has_invisible_set;
	boost::shared_ptr<BeamDefaults> beam_defaults;
	int detacher_group;
};

/* -------------------------------------------------------------------------- */
/* Section SCREWPROPS
/* -------------------------------------------------------------------------- */

struct Screwprop
{
	Screwprop():
		power(0)
	{}

	Node::Id prop_node;
	Node::Id back_node;
	Node::Id top_node;
	float power;
};

/* -------------------------------------------------------------------------- */
/* Section SLIDENODES
/* -------------------------------------------------------------------------- */

struct SlideNode
{
	SlideNode();

	static const unsigned int CONSTRAINT_ATTACH_ALL     = BITMASK(1);
	static const unsigned int CONSTRAINT_ATTACH_FOREIGN = BITMASK(2);
	static const unsigned int CONSTRAINT_ATTACH_SELF    = BITMASK(3);
	static const unsigned int CONSTRAINT_ATTACH_NONE    = BITMASK(4);

	Node::Id slide_node;
	std::vector<Node::Range> rail_node_ranges;
	float spring_rate;
	float break_force;
	float tolerance;
	unsigned int railgroup_id;
	bool _railgroup_id_set;
	float attachment_rate;
	float max_attachment_distance;
	bool _break_force_set;
	unsigned int constraint_flags;
};

/* -------------------------------------------------------------------------- */
/* Section SOUNDSOURCES
/* -------------------------------------------------------------------------- */

struct SoundSource
{
	Node::Id node;
	Ogre::String sound_script_name;
};

/* -------------------------------------------------------------------------- */
/* Section SOUNDSOURCES2
/* -------------------------------------------------------------------------- */

struct SoundSource2: SoundSource
{
	enum Mode
	{
		MODE_BEGIN   = 0,
		MODE_ALWAYS  = -2,
		MODE_OUTSIDE = -1,
		MODE_CINECAM = 1,
		MODE_END     = 0xFFFFFFFE,
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
/* Section SPEEDLIMITER
/* -------------------------------------------------------------------------- */

struct SpeedLimiter
{
	float max_speed;
};

/* -------------------------------------------------------------------------- */
/* Section SUBMESH
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

	Node::Id nodes[3];
	unsigned int options;
};

struct Texcoord
{
	Texcoord():
		u(0),
		v(0)
	{}

	Node::Id node;
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
/* Section TIES
/* -------------------------------------------------------------------------- */

struct Tie
{
	Tie();

	enum Options
	{
		OPTIONS_BEGIN = 0,
		OPTIONS_VISIBLE = 'n',
		OPTIONS_INVISIBLE = 'i',
		OPTIONS_END = 99,
		OPTIONS_INVALID = 0xFFFFFFFF
	};

	Node::Id root_node;
	float max_reach_length;
	float auto_shorten_rate;
	float min_length;
	float max_length;
	Options options;
	float max_stress;
	boost::shared_ptr<BeamDefaults> beam_defaults;
	int detacher_group;
	unsigned int group;
	bool _group_set;
};

/* -------------------------------------------------------------------------- */
/* Section TORQUECURVE
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
/* Section TURBOJETS
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

	Node::Id front_node;
	Node::Id back_node;
	Node::Id side_node;
	int is_reversable;
	float dry_thrust;
	float wet_thrust;
	float front_diameter;
	float back_diameter;
	float nozzle_length;
};

/* -------------------------------------------------------------------------- */
/* Section TURBOPROPS, TURBOPROPS2
/* -------------------------------------------------------------------------- */

struct Turboprop2
{
	Turboprop2():
		turbine_power_kW(0),
		_format_version(2)
	{}

	Node::Id reference_node;
	Node::Id axis_node;
	Node::Id blade_tip_nodes[4];
	float turbine_power_kW;
	Ogre::String airfoil;
	Node::Id couple_node;
	unsigned int _format_version;
};

/* -------------------------------------------------------------------------- */
/* Section VIDEOCAMERA
/* -------------------------------------------------------------------------- */

struct VideoCamera
{
	VideoCamera();

	Node::Id reference_node;
	Node::Id left_node;
	Node::Id bottom_node;
	Node::Id alt_reference_node;
	bool _alt_reference_node_set;
	Node::Id alt_orientation_node;
	bool _alt_orientation_node_set;
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
/* Section WINGS
/* -------------------------------------------------------------------------- */

struct Wing
{
	Wing():
		control_surface(CONTROL_n_NONE)
	{
		tex_coords[0] = 0.f;
		tex_coords[1] = 0.f;
		tex_coords[2] = 0.f;
		tex_coords[3] = 0.f;
		tex_coords[4] = 0.f;
		tex_coords[5] = 0.f;
		tex_coords[6] = 0.f;
		tex_coords[7] = 0.f;
	}

	enum Control
	{
		CONTROL_BEGIN                   = 0,

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

		CONTROL_END                     = 0xFFFFFFFE,

		CONTROL_INVALID                 = 0xFFFFFFFF
	};

	Node::Id nodes[8];
	float tex_coords[8];
	Control control_surface;
	float chord_point;
	float min_deflection;
	float max_deflection;
	Ogre::String airfoil;
	float efficancy_coef;
	
};

/* -------------------------------------------------------------------------- */
/* Root document
/* -------------------------------------------------------------------------- */

struct File
{
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
		boost::shared_ptr<AntiLockBrakes>  anti_lock_brakes;
		std::vector<Axle>                  axles;
		std::vector<Beam>                  beams;
		boost::shared_ptr<Brakes>          brakes;
		std::vector<Camera>                cameras;
		std::vector<CameraRail>            camera_rails;
		std::vector<CollisionBox>          collision_boxes;
		std::vector<Cinecam>               cinecam;
		std::vector<Command2>              commands_2; /* sections 'commands' & 'commands2' are unified */
		boost::shared_ptr<CruiseControl>   cruise_control;
		std::vector<Node::Id>              contacters;
		boost::shared_ptr<Engine>          engine;
		boost::shared_ptr<Engoption>       engoption;
		std::vector<Exhaust>               exhausts;
		boost::shared_ptr<ExtCamera>       ext_camera;
		std::vector<Node::Id>              fixes;
		std::vector<Flare2>                flares_2;
		std::vector<
			boost::shared_ptr<Flexbody>
		>                                  flexbodies;
		std::vector<FlexBodyWheel>         flex_body_wheels;
		boost::shared_ptr<Fusedrag>        fusedrag;
		boost::shared_ptr<Globals>         globals;
		boost::shared_ptr<GuiSettings>     gui_settings;
		std::vector<Hook>                  hooks;
		std::vector<Hydro>                 hydros;
		std::vector<Lockgroup>             lockgroups;
		std::vector<ManagedMaterial>       managed_materials;
		std::vector<MaterialFlareBinding>  material_flare_bindings;
		std::vector<MeshWheel>             mesh_wheels;
		std::vector<MeshWheel2>            mesh_wheels_2;
		std::vector<Node>                  nodes; /* Nodes and Nodes2 are unified in this parser */
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
		std::vector<Shock>                 shocks;
		std::vector<Shock2>                shocks_2;
		boost::shared_ptr<
			SkeletonSettings
		>                                  skeleton_settings;
		std::vector<SlideNode>             slidenodes;
		boost::shared_ptr<SlopeBrake>      slope_brake;
		std::vector<SoundSource>           soundsources;
		std::vector<SoundSource2>          soundsources2;
		boost::shared_ptr<SpeedLimiter>    speed_limiter;
		Ogre::String                       submeshes_ground_model_name;
		std::vector<Submesh>               submeshes;
		std::vector<Tie>                   ties;
		boost::shared_ptr<TorqueCurve>     torque_curve;
		boost::shared_ptr<TractionControl> traction_control;
		std::vector<Trigger>               triggers;
		std::vector<Turbojet>              turbojets;
		std::vector<Turboprop2>            turboprops_2;
		std::vector<VideoCamera>           videocameras;
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
		KEYWORD_ENABLE_ADVANCED_DEFORMATION,
		KEYWORD_END,
		KEYWORD_END_SECTION,
		KEYWORD_ENGINE,
		KEYWORD_ENGOPTION,
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
		KEYWORD_HOOKGROUP,
		KEYWORD_HOOKS,
		KEYWORD_HYDROS,
		KEYWORD_IMPORTCOMMANDS,
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
		KEYWORD_SECTION,
		KEYWORD_SECTIONCONFIG,
		KEYWORD_SET_BEAM_DEFAULTS,
		KEYWORD_SET_BEAM_DEFAULTS_SCALE,
		KEYWORD_SET_COLLISION_RANGE,
		KEYWORD_SET_INERTIA_DEFAULTS,
		KEYWORD_SET_MANAGEDMATERIALS_OPTIONS,
		KEYWORD_SET_NODE_DEFAULTS,
		KEYWORD_SET_SHADOWS,
		KEYWORD_SET_SKELETON_SETTINGS,
		KEYWORD_SHOCKS,
		KEYWORD_SHOCKS2,
		KEYWORD_SLIDENODE_CONNECT_INSTANTLY,
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
		KEYWORD_TRIGGERS,
		KEYWORD_TURBOJETS,
		KEYWORD_TURBOPROPS,
		KEYWORD_TURBOPROPS2,
		KEYWORD_VIDEOCAMERA,
		KEYWORD_WHEELS,
		KEYWORD_WHEELS2,
		KEYWORD_WINGS,

		KEYWORD_INVALID = 0xFFFFFFFF
	};

	enum Section
	{
		SECTION_BEGIN,

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
		SECTION_LOCKGROUPS,
		SECTION_MANAGED_MATERIALS,
		SECTION_MATERIAL_FLARE_BINDINGS,
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
		SECTION_SHOCKS,
		SECTION_SHOCKS_2,
		SECTION_SLIDENODES,
		SECTION_SOUNDSOURCES,
		SECTION_SOUNDSOURCES2,
		SECTION_SUBMESH,
		SECTION_SLOPE_BRAKE,
		SECTION_TIES,
		SECTION_TORQUE_CURVE,
		SECTION_TRACTION_CONTROL,
		SECTION_TRIGGERS,
		SECTION_TRUCK_NAME, ///< The very start of file	
		SECTION_TURBOJETS,
		SECTION_TURBOPROPS,
		SECTION_TURBOPROPS_2,
		SECTION_VIDEO_CAMERA,
		SECTION_WHEELS,
		SECTION_WHEELS_2,
		SECTION_WINGS,

		SECTION_NONE,       ///< Right after rig name, for example.

		SECTION_END,

		SECTION_INVALID = 0xFFFFFFFF
	};

	enum Subsection
	{
		SUBSECTION_BEGIN = -1,
		SUBSECTION_NONE = 0,

		SUBSECTION__FLEXBODIES__PROPLIKE_LINE,
		SUBSECTION__FLEXBODIES__FORSET_LINE,

		SUBSECTION__SUBMESH__TEXCOORDS,
		SUBSECTION__SUBMESH__CAB,

		SUBSECTION_END = 99999,

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
	bool _collision_range_set;
	float minimum_mass;
	bool _minimum_mass_set;

	/* Vehicle sections */
	boost::shared_ptr<Module> root_module;
	std::map< Ogre::String, boost::shared_ptr<Module> > modules;

	/* File sections */
	std::vector<Author> authors;
	boost::shared_ptr<Fileinfo> file_info;
};

} // namespace RigParser