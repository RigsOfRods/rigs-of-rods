/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2015 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

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
* @file
* @brief Core data structures for simulation.
* @author created on 30th of April 2010 by Thomas Fischer 
*/

#pragma once

/*
 A word of warning:
 RoR's performance is very sensitive to the ordering of the parameters in this
 structure (due to cache reasons). You can easily destroy RoR's performance if you put
 something in the wrong place. Unless you know what you are doing (do you come armed
 with a cache usage tracker?), add what you wish to the bottom of the structure.
*/

// The RoR required includes (should be included already)
#include "RoRPrerequisites.h"
#include "rornet.h"
#include "SlideNode.h"

/* maximum limits */
static const int   MAX_TRUCKS                 = 5000;            //!< maximum number of trucks for the engine

static const int   MAX_NODES                  = 1000;            //!< maximum number of nodes per truck
static const int   MAX_BEAMS                  = 5000;            //!< maximum number of beams per truck
static const int   MAX_ROTATORS               = 20;              //!< maximum number of rotators per truck
static const int   MAX_CONTACTERS             = 2000;            //!< maximum number of contacters per truck
static const int   MAX_HYDROS                 = 1000;            //!< maximum number of hydros per truck
static const int   MAX_WHEELS                 = 64;              //!< maximum number of wheels per truck
static const int   MAX_SUBMESHES              = 500;             //!< maximum number of submeshes per truck
static const int   MAX_TEXCOORDS              = 3000;            //!< maximum number of texture coordinates per truck
static const int   MAX_CABS                   = 3000;            //!< maximum number of cabs per truck
static const int   MAX_SHOCKS                 = MAX_BEAMS;       //!< maximum number of shocks per truck
static const int   MAX_ROPES                  = 64;              //!< maximum number of ropes per truck
static const int   MAX_ROPABLES               = 64;              //!< maximum number of ropables per truck
static const int   MAX_TIES                   = 64;              //!< maximum number of ties per truck
static const int   MAX_PROPS                  = 200;             //!< maximum number of props per truck
static const int   MAX_COMMANDS               = 84;              //!< maximum number of commands per truck
static const int   MAX_CAMERAS                = 10;              //!< maximum number of cameras per truck
static const int   MAX_RIGIDIFIERS            = 100;             //!< maximum number of rigifiers per truck
static const int   MAX_FLEXBODIES             = 64;              //!< maximum number of flexbodies per truck
static const int   MAX_AEROENGINES            = 8;               //!< maximum number of aero engines per truck
static const int   MAX_SCREWPROPS             = 8;               //!< maximum number of boat screws per truck
static const int   MAX_AIRBRAKES              = 20;              //!< maximum number of airbrakes per truck
static const int   MAX_SOUNDSCRIPTS_PER_TRUCK = 128;             //!< maximum number of soundsscripts per truck
static const int   MAX_WINGS                  = 40;              //!< maximum number of wings per truck
static const int   MAX_CPARTICLES             = 10;              //!< maximum number of custom particles per truck
static const int   MAX_PRESSURE_BEAMS         = 4000;            //!< maximum number of pressure beams per truck
static const int   MAX_CAMERARAIL             = 50;              //!< maximum number of camera rail points

static const float RAD_PER_SEC_TO_RPM         = 9.5492965855137f; //!< Convert radian/second to RPM (60/2*PI)

/* other global static definitions */
static const int   TRUCKFILEFORMATVERSION     = 3;               //!< truck file format version number

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

/* physics defaults */
static const float DEFAULT_RIGIDIFIER_SPRING    = 1000000.0f;
static const float DEFAULT_RIGIDIFIER_DAMP      = 50000.0f;
static const float DEFAULT_SPRING               = 9000000.0f;
static const float DEFAULT_DAMP                 = 12000.0f;
static const float DEFAULT_GRAVITY              = -9.8f;         //!< earth gravity
static const float DEFAULT_DRAG                 = 0.05f;
static const float DEFAULT_BEAM_DIAMETER        = 0.05f;         //!< 5 centimeters default beam width
static const float DEFAULT_COLLISION_RANGE      = 0.02f;
static const float MIN_BEAM_LENGTH              = 0.1f;          //!< minimum beam lenght is 10 centimeters
static const float INVERTED_MIN_BEAM_LENGTH     = 1.0f / MIN_BEAM_LENGTH;
static const float BEAM_SKELETON_DIAMETER       = 0.01f;
static const float DEFAULT_WATERDRAG            = 10.0f;
static const float IRON_DENSITY                 = 7874.0f;
static const float BEAM_BREAK                   = 1000000.0f;
static const float BEAM_DEFORM                  = 400000.0f;
static const float BEAM_CREAK_DEFAULT           = 100000.0f;
static const float WHEEL_FRICTION_COEF          = 2.0f;
static const float CHASSIS_FRICTION_COEF        = 0.5f; //!< Chassis has 1/4 the friction of wheels.
static const float SPEED_STOP                   = 0.2f;
static const float STAB_RATE                    = 0.025f;
static const float NODE_FRICTION_COEF_DEFAULT   = 1.0f;
static const float NODE_VOLUME_COEF_DEFAULT     = 1.0f;
static const float NODE_SURFACE_COEF_DEFAULT    = 1.0f;
static const float NODE_LOADWEIGHT_DEFAULT      = -1.0f;
static const float SUPPORT_BEAM_LIMIT_DEFAULT   = 4.0f;
static const float ROTATOR_FORCE_DEFAULT        = 10000000.0f;
static const float ROTATOR_TOLERANCE_DEFAULT    = 0.0f;
static const float HOOK_FORCE_DEFAULT		    = 10000000.0f;
static const float HOOK_RANGE_DEFAULT           = 0.4f;
static const float HOOK_SPEED_DEFAULT           = 0.00025f;
static const float HOOK_LOCK_TIMER_DEFAULT      = 5.0;
static const int   NODE_LOCKGROUP_DEFAULT		= -1; // all hooks scan all nodes

#define THREAD_SINGLE false	//!< single threading mode
#define THREAD_MULTI  true  //!< multi threading mode

/* Enumerations */
enum {
	BEAM_NORMAL,
	BEAM_HYDRO,
	BEAM_VIRTUAL,
	BEAM_MARKED,
	BEAM_INVISIBLE,
	BEAM_INVISIBLE_HYDRO
};
enum {
	NODE_NORMAL,
	NODE_LOADED
};

enum {
	ACTIVATED,      //!< leading truck
	DESACTIVATED,   //!< not leading but active
	MAYSLEEP,       //!< active but wanting to sleep
	GOSLEEP,        //!< active but ordered to sleep ASAP (synchronously)
	SLEEPING,       //!< not active, sleeping
	NETWORKED,      //!< not calculated, gets remote data
	NETWORKED_INVALID,	//!< not calculated, size differs from expected
	RECYCLE,        //!< waiting for reusage
	DELETED,        //!< special used when truck pointer is 0
};

enum {
	UNLOCKED,       //!< lock not locked
	PRELOCK,        //!< prelocking, attraction forces in action
	LOCKED          //!< lock locked.
};
enum {
	NOT_DRIVEABLE,  //!< not drivable at all
	TRUCK,          //!< its a truck
	AIRPLANE,       //!< its an airplane
	BOAT,           //!< its a boat
	MACHINE,        //!< its a machine
	AI,             //!< machine controlled by an Artificial Intelligence
};
enum {
	DRY,            //!< node is dry
	DRIPPING,       //!< node is dripping
	WET             //!< node is wet
};
enum {
	NOSHOCK,        //!< not a shock
	SHOCK1,         //!< shock1
	SHOCK2,         //!< shock2
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
	SHOCK_FLAG_INVISIBLE		= BITMASK(2),
	SHOCK_FLAG_LACTIVE			= BITMASK(3),
	SHOCK_FLAG_RACTIVE			= BITMASK(4),
	SHOCK_FLAG_ISSHOCK2			= BITMASK(5),
	SHOCK_FLAG_SOFTBUMP			= BITMASK(6),
	SHOCK_FLAG_ISTRIGGER		= BITMASK(7),
	SHOCK_FLAG_TRG_BLOCKER		= BITMASK(8),
	SHOCK_FLAG_TRG_CMD_SWITCH	= BITMASK(9),
	SHOCK_FLAG_TRG_CMD_BLOCKER	= BITMASK(10),
	SHOCK_FLAG_TRG_BLOCKER_A	= BITMASK(11),
	SHOCK_FLAG_TRG_HOOK_UNLOCK 	= BITMASK(12),
	SHOCK_FLAG_TRG_HOOK_LOCK   	= BITMASK(13),
	SHOCK_FLAG_TRG_CONTINUOUS	= BITMASK(14),
	SHOCK_FLAG_TRG_ENGINE		= BITMASK(15)
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

/* some info holding arrays */
static const float flapangles[6] = {0.f, -0.07f, -0.17f, -0.33f, -0.67f, -1.f};

/* basic structures */

#include "datatypes/node.h"
#include "datatypes/shock.h"

struct collcab_rate_t
{
	int rate;
	int distance;
	bool update;
	bool calcforward;
};

#include "datatypes/beam.h"


struct soundsource_t
{
	SoundScriptInstance* ssi;
	int nodenum;
	int type;
};

struct contacter_t
{
	int nodeid;
	int contacted;
	int opticontact;
};

struct rigidifier_t
{
	node_t* a;
	node_t* b;
	node_t* c;
	float k;
	float d;
	float alpha;
	float lastalpha;
	beam_t *beama;
	beam_t *beamc;
};

#include "datatypes/wheel.h"

struct hook_t
{
	int locked;
	int group;
	int lockgroup;
	bool lockNodes;
	bool selflock;
	bool autolock;
	bool nodisable;
	bool visible;
	float maxforce;
	float lockrange;
	float lockspeed;
	float timer;
	float timer_preset;
	node_t *hookNode;
	node_t *lockNode;
	beam_t *beam;
	Beam *lockTruck;
};

struct ropable_t
{
	node_t *node;
	int group;
	bool multilock;
	int used;
};

struct rope_t
{
	int locked;
	int group;
	beam_t *beam;
	node_t *lockedto;
	ropable_t *lockedto_ropable;
	Beam *lockedtruck;
};


struct tie_t
{
	beam_t *beam;
	ropable_t *lockedto;
	int group;
	bool tied;
	bool tying;
	float commandValue;
};


struct wing_t
{
	FlexAirfoil *fa;
	Ogre::SceneNode *cnode;
};

struct command_t
{
	int commandValueState;
	float commandValue;
	float triggerInputValue;
	float playerInputValue;
	bool trigger_cmdkeyblock_state;  //!< identifies blocked F-commands for triggers
	std::vector<int> beams;
	std::vector<int> rotators;
	Ogre::String description;
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
	char type;
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
	Ogre::SceneNode *snode; //!< The pivot scene node (parented to root-node).
	Ogre::SceneNode *wheel; //!< Special prop: custom steering wheel for dashboard
	Ogre::Vector3 wheelpos; //!< Special prop: custom steering wheel for dashboard
	int mirror;             //<! Special prop: rear view mirror {0 = disabled, -1 = right, 1 = left}
	char beacontype;        //<! Special prop: beacon {0 = none, 'b' = user-specified, 'r' = red, 'p' = police lightbar, 'L'/'R'/'w' - aircraft wings}
	Ogre::BillboardSet *bbs[4];
	Ogre::SceneNode *bbsnode[4];
	Ogre::Light *light[4];
	float brate[4];
	float bpos[4];
	int pale;               //!< Is this a pale? (Boolean {0/1})
	int spinner;            //!< Is this a spinprop? (Boolean {0/1})
	bool animated;
	float anim_x_Rot;
	float anim_y_Rot;
	float anim_z_Rot;
	float anim_x_Off;
	float anim_y_Off;
	float anim_z_Off;
	float animratio[10]; //!< A coefficient for the animation, prop degree if used with mode: rotation and propoffset if used with mode: offset. 
	int animFlags[10];
	int animMode[10];
	float animOpt1[10]; //!< The lower limit for the animation
	float animOpt2[10]; //!< The upper limit for the animation
	float animOpt3[10]; //!< Various purposes
	float animOpt4[10]; 
	float animOpt5[10];
	int animKey[10];
	int animKeyState[10];
	int lastanimKS[10];
	Ogre::Real wheelrotdegree;
	int cameramode; //!< Visibility control {-2 = always, -1 = 3rdPerson only, 0+ = cinecam index}
	MeshObject *mo;
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


struct debugtext_t
{
	int id;
	Ogre::MovableText *txt;
	Ogre::SceneNode *node;
};

#include "datatypes/rig.h"


// some non-beam structs


struct collision_box_t
{
	//absolute collision box
	Ogre::Vector3 lo; //!< absolute collision box
	Ogre::Vector3 hi; //!< absolute collision box
	bool refined;
	//rotation
	Ogre::Quaternion rot; //!< rotation
	Ogre::Quaternion unrot; //!< rotation
	//center of rotation
	Ogre::Vector3 center; //!< center of rotation
	//relative collision box
	Ogre::Vector3 relo; //!< relative collision box
	Ogre::Vector3 rehi; //!< relative collision box
	//self rotation
	bool selfrotated;
	Ogre::Vector3 selfcenter;
	Ogre::Quaternion selfrot;
	Ogre::Quaternion selfunrot;
	int eventsourcenum;
	bool virt;
	bool camforced;
	Ogre::Vector3 campos;
	int event_filter;
	bool enabled;
	Ogre::Vector3 ilo, ihi;
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
