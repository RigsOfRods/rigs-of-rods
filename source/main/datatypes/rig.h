/*
 * rig.h
 *
 *  Created on: Dec 29, 2012
 *      Author: chris
 */

#ifndef RIG_H_
#define RIG_H_

#include "RoRPrerequisites.h"
#include "BeamData.h"
#include <vector>

#include <OgrePrerequisites.h>

/**
* SIM-CORE; Represents a vehicle.
*/
struct rig_t
{
	// TODO: sort these a bit more ...
	node_t nodes[MAX_NODES];
	int free_node;

	beam_t beams[MAX_BEAMS];
	int free_beam;

	contacter_t contacters[MAX_CONTACTERS];
	int free_contacter;

	rigidifier_t rigidifiers[MAX_RIGIDIFIERS];
	int free_rigidifier;

	wheel_t wheels[MAX_WHEELS];
	vwheel_t vwheels[MAX_WHEELS];
	int free_wheel;

	std::vector <rope_t> ropes;
	std::vector <ropable_t> ropables;
	std::vector <tie_t> ties;
	std::vector <hook_t> hooks;

	wing_t wings[MAX_WINGS];
	int free_wing;

	command_t commandkey[MAX_COMMANDS + 10]; // 0 for safety

	rotator_t rotators[MAX_ROTATORS];
	int free_rotator;

	std::vector<flare_t> flares;
	int free_flare;

	prop_t props[MAX_PROPS];
	prop_t *driverSeat;
	int free_prop;

	shock_t shocks[MAX_SHOCKS];
	int free_shock;
	int free_active_shock; //!< this has no array associated with it. its just to determine if there are active shocks!

	std::vector < exhaust_t > exhausts;

	cparticle_t cparticles[MAX_CPARTICLES];
	int free_cparticle;

	std::vector<debugtext_t>nodes_debug, beams_debug;

	soundsource_t soundsources[MAX_SOUNDSCRIPTS_PER_TRUCK];
	int free_soundsource;

	int pressure_beams[MAX_PRESSURE_BEAMS];
	int free_pressure_beam;

	AeroEngine *aeroengines[MAX_AEROENGINES];
	int free_aeroengine;

	Screwprop *screwprops[MAX_SCREWPROPS];
	int free_screwprop;

	int cabs[MAX_CABS*3];
	int subisback[MAX_SUBMESHES]; //!< Submesh; {0, 1, 2}
	int free_cab;

	int hydro[MAX_HYDROS];
	int free_hydro;

	Ogre::Vector3 texcoords[MAX_TEXCOORDS];
	int free_texcoord;

	int subtexcoords[MAX_SUBMESHES];
	int subcabs[MAX_SUBMESHES];
	int free_sub;

	int collcabs[MAX_CABS];
	int collcabstype[MAX_CABS];
	collcab_rate_t inter_collcabrate[MAX_CABS];
	collcab_rate_t intra_collcabrate[MAX_CABS];
	int free_collcab;

	int buoycabs[MAX_CABS];
	int buoycabtypes[MAX_CABS];
	int free_buoycab;

	Airbrake *airbrakes[MAX_AIRBRAKES];
	int free_airbrake;

	Skidmark *skidtrails[MAX_WHEELS*2];
	bool useSkidmarks;

	FlexBody *flexbodies[MAX_FLEXBODIES];
	int free_flexbody;

	std::vector <VideoCamera *> vidcams;

	std::vector<std::string> description;

	int cameraRail[MAX_CAMERARAIL];
	int free_camerarail;

	bool hideInChooser;

	char guid[128];
	int hasfixes;
	int wingstart;

	Ogre::String realtruckname;
	bool loading_finished;

	bool forwardcommands;
	bool importcommands;
	bool wheel_contact_requested;
	bool rescuer;
	bool disable_default_sounds;

	// Antilockbrake + Tractioncontrol
	bool slopeBrake;
	float slopeBrakeFactor;
	float slopeBrakeAttAngle;
	float slopeBrakeRelAngle;
	float previousCrank;
	float alb_ratio;        //!< Anti-lock brake attribute: Regulating force
	float alb_minspeed;     //!< Anti-lock brake attribute;
	int alb_mode;           //!< Anti-lock brake status; Enabled? {1/0}
	unsigned int alb_pulse; //!< Anti-lock brake attribute;
	bool alb_pulse_state;   //!< Anti-lock brake status;
	bool alb_present;       //!< Anti-lock brake attribute: Display the dashboard indicator?
	bool alb_notoggle;      //!< Anti-lock brake attribute: Disable in-game toggle?
	float tc_ratio;
	float tc_wheelslip;
	float tc_fade;
	int tc_mode;           //!< Traction control status; Enabled? {1/0}
	unsigned int tc_pulse; //!< Traction control attribute;
	bool tc_pulse_state;
	bool tc_present;       //!< Traction control attribute; Display the dashboard indicator?
	bool tc_notoggle;      //!< Traction control attribute; Disable in-game toggle?
	float tcalb_timer;
	int antilockbrake;
	int tractioncontrol;
	float animTimer;

	// Cruise Control
	bool cc_mode;
	bool cc_can_brake;
	float cc_target_rpm;
	float cc_target_speed;
	float cc_target_speed_lower_limit;

	// Speed Limiter
	bool sl_enabled; //!< Speed limiter;
	float sl_speed_limit; //!< Speed limiter;

	char uniquetruckid[256];
	int categoryid;
	int truckversion;
	int externalcameramode, externalcameranode;
	std::vector<authorinfo_t> authors;
	float fadeDist;
	float collrange;
	int masscount; //!< Number of nodes loaded with l option
	bool disable_smoke;
	int smokeId;  //!< Old-format exhaust (one per vehicle) emitter node
	int smokeRef; //!< Old-format exhaust (one per vehicle) backwards direction node
	char truckname[256];
	bool networking;
	int editorId;
	bool beambreakdebug, beamdeformdebug, triggerdebug;
	CmdKeyInertia *rotaInertia;
	CmdKeyInertia *hydroInertia;
	CmdKeyInertia *cmdInertia;
	bool enable_wheel2; //!< If false, wheels2 are downgraded to wheels1 (needed for multiplayer)
	float truckmass;
	float loadmass;
	char texname[1024]; //!< Material name
	int trucknum;
	Skin *usedSkin;
	Buoyance *buoyance;

	int driveable;
	BeamEngine *engine;
	int hascommands;
	int hashelp;
	char helpmat[256];
	int cinecameranodepos[MAX_CAMERAS]; //!< Cine-camera node indexes
	int freecinecamera; //!< Number of cine-cameras (lowest free index)
	int flaresMode;
	Ogre::Light *cablight;
	Ogre::SceneNode *cablightNode;
	std::vector<Ogre::Entity*> deletion_Entities; //!< For unloading vehicle; filled at spawn.
	std::vector<Ogre::MovableObject *> deletion_Objects; //!< For unloading vehicle; filled at spawn.
	std::vector<Ogre::SceneNode*> deletion_sceneNodes; //!< For unloading vehicle; filled at spawn.
	int netCustomLightArray[4];
	unsigned char netCustomLightArray_counter;
	MaterialFunctionMapper *materialFunctionMapper;
	bool driversseatfound;
	bool ispolice;
	int state;
	bool collisionRelevant;
	bool heathaze;
	Autopilot *autopilot;
	HeightFinder *hfinder;
	Airfoil *fuseAirfoil;
	node_t *fuseFront;
	node_t *fuseBack;
	float fuseWidth;
	float brakeforce;
	float hbrakeforce;
	//! Dbg. overlay type { NODES: 1-Numbers, 4-Mass, 5-Locked | BEAMS: 2-Numbers, 6-Compression, 7-Broken, 8-Stress, 9-Strength, 10-Hydros, 11-Commands, OTHER: 3-N&B numbers, 12-14 unknown }
	int debugVisuals;

	Ogre::String speedomat, tachomat;
	float speedoMax;
	bool useMaxRPMforGUI;
	float minimass;
	bool cparticle_enabled;
	bool advanced_drag;
	float advanced_node_drag;
	float advanced_total_drag;

	Axle *axles[MAX_WHEELS/2];
	int free_axle;

	int free_fixes;
	int propwheelcount;
	int free_commands;
	int fileformatversion;

	std::vector<Ogre::String> sectionconfigs;

	Ogre::Vector3 origin;
	Ogre::SceneNode *beamsRoot;
	//! Stores all the SlideNodes available on this truck
	std::vector< SlideNode > mSlideNodes;

	int proped_wheels; //!< Number of propelled wheels.
	int braked_wheels; //!< Number of braked wheels.

	int proppairs[MAX_WHEELS]; //!< For inter-differential locking

	//! try to connect slide-nodes directly after spawning
	bool slideNodesConnectInstantly;

	//! Stores all the available RailGroups for this truck
	std::vector< RailGroup* > mRailGroups;

	Ogre::Camera *mCamera;
	int freecamera;
	int cameranodepos[MAX_CAMERAS];
	int cameranodedir[MAX_CAMERAS];
	int cameranoderoll[MAX_CAMERAS];
	bool revroll[MAX_CAMERAS];
	bool shadowOptimizations;
	int hasEmissivePass;
	FlexObj *cabMesh;
	Ogre::SceneNode *cabNode;
	Ogre::AxisAlignedBox boundingBox; //!< standard bounding box (surrounds all nodes of a truck)
	Ogre::AxisAlignedBox predictedBoundingBox;
	std::vector<Ogre::AxisAlignedBox> collisionBoundingBoxes; //!< smart bounding boxes, used for determining the state of a truck (every box surrounds only a subset of nodes)
	std::vector<Ogre::AxisAlignedBox> predictedCollisionBoundingBoxes;
	bool freePositioned;
	int lowestnode; //!< never updated after truck init!?!

	float default_spring; //!< TODO: REMOVE! (parser context only)
	float default_spring_scale; //!< TODO: REMOVE! (parser context only)
	float default_damp;
	float default_damp_scale;
	float default_deform;
	float default_deform_scale;
	float default_break;
	float default_break_scale;

	float default_beam_diameter;
	float default_plastic_coef;
	float skeleton_beam_diameter;

	char default_beam_material[256];
	float default_node_friction; //!< TODO: REMOVE! (parser context only)
	float default_node_volume; //!< TODO: REMOVE! (parser context only)
	float default_node_surface; //!< TODO: REMOVE! (parser context only)
	float default_node_loadweight; //!< TODO: REMOVE! (parser context only)
	char default_node_options[50]; //!< TODO: REMOVE! (parser context only)

	float posnode_spawn_height;

	MaterialReplacer *materialReplacer;
	Ogre::String subMeshGroundModelName;

	float odometerTotal;
	float odometerUser;

	std::vector<std::pair<Ogre::String, bool> > dashBoardLayouts;
	Ogre::String beamHash; //!< Unused
};

#endif /* RIG_H_ */
