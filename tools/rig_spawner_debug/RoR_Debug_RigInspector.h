/*

RIG INSPECTOR

Written 10/2014 by Petr "only_a_ptr" Ohlidal

This code logs all data contained within class Beam and associated data structs.

The purpose is to debug class RigSpawner, introduced in TruckParser2013 project 
	and replacing class SerializedRig.

*/

#pragma once

#include "RoRPrerequisites.h"
#include "AirBrake.h"
#include "BeamData.h"
#include "Beam.h"
#include "BeamEngine.h"
#include <iostream>

// ----------------------------------------------------------------------------
// Helper macros, to be used with std::fstream and << operator
// ----------------------------------------------------------------------------

#define ECHO_V3(V3) "["<<V3.x<<","<<V3.y<<","<<V3.z<<"]"

#define ECHO_QUAT(Q) "[" <<Q.x<<", "<<Q.y <<", "<<Q.z<<", " <<Q.w  <<"]"

#define ECHO_AABB(BOX) 	"@AABB{{" <<ECHO_V3(BOX.getMinimum())<<","	<<ECHO_V3(BOX.getMaximum())<<",finite="<<BOX.isFinite() <<"}}AABB@"

#define ECHO_PTR(PTR) ((PTR) != nullptr ? "PTR_OK" : "nullptr")

#define ECHO_NODE(N) "[" << ECHO_PTR(N) <<"," <<(((N)!= nullptr) ? N->pos : -1) << "]"

#define ECHO_BEAM(B) "{" << ECHO_PTR(B) <<", n1:" << ECHO_NODE(B->p1) <<", n2:" << ECHO_NODE(B->p2) << "}"

#define LOG_ARRAY(VARNAME, ARRAY, SIZE) \
{ \
	f << (VARNAME) << "(size=" << (SIZE) <<")" << ":"; \
	for (int i=0; i<(SIZE); ++i){ f <<((ARRAY)[i]); f << ","; }     \
}

#define LOG_ARRAY_PTR(VARNAME, ARRAY, SIZE)  \
{ \
	f << (VARNAME) << "(size=" << (SIZE) <<")" << ":" \
	; for (int i=0; i<(SIZE); ++i) { \
		f<<"[" <<i <<":" <<(ECHO_PTR((ARRAY[i]))) <<"]" ; \
	 } \
} 

// ----------------------------------------------------------------------------
// Inspector code.
// ----------------------------------------------------------------------------

class RigInspector
{
public:

	static void InspectFlexAirfoil(std::ofstream & f, FlexAirfoil & fa);

	static void InspectWings(std::ofstream & f, Beam* rig);

static void InspectRig(Beam* rig, std::string const & out_path)
{
	std::ofstream f;
	f.open(out_path);
	if (! f.is_open())
	{
		LOG("InspectRig(): failed to open file "+out_path);
		return;
	}
	//f.precision(6);

	// TRUCKPARSER DEBUG - NODES
	
	f << "\n***********************\nNodes double check (free node:" << rig->free_node<<")";
	for (int i = 0; i < rig->free_node; i++)
	{
		node_t & node = rig->nodes[i];
		f<<"\nNode"
			<<" index="<<i
			// all rig_t members, in order
			// <<" FOOOOOOO="<<node.BARRRRRRR
			<<" RelPosition="<< ECHO_V3(node.RelPosition)
			<<" AbsPosition="<< ECHO_V3(node.AbsPosition)
			<<" Velocity="<< ECHO_V3(node.Velocity)
			<<" Forces="<< ECHO_V3(node.Forces)
			<<" inverted_mass="<<node.inverted_mass
			<<" mass="<<node.mass
			<<" lastNormal="<< ECHO_V3(node.lastNormal)
			<<" locked="<<node.locked
			<<" iswheel="<<node.iswheel
			<<" wheelid="<<node.wheelid
			<<" masstype="<<node.masstype
			<<" wetstate="<<node.wetstate
			<<" contactless="<<node.contactless
			<<" lockednode="<<node.lockednode
			<<" lockedPosition="<< ECHO_V3(node.lockedPosition)
			<<" lockedForces="<< ECHO_V3(node.lockedForces)
			<<" lockedVelocity="<< ECHO_V3(node.lockedVelocity)
			<<" contacted="<<node.contacted
			<<" lockgroup="<<node.lockgroup
			<<" friction_coef="<<node.friction_coef
			<<" buoyancy="<<node.buoyancy
			<<" volume_coef="<<node.volume_coef
			<<" surface_coef="<<node.surface_coef
			<<" lastdrag="<<node.lastdrag
			<<" gravimass="<<node.gravimass
			<<" wettime="<<node.wettime
			<<" isHot="<<node.isHot
			<<" overrideMass="<<node.overrideMass
			<<" disable_particles="<<node.disable_particles
			<<" disable_sparks="<<node.disable_sparks
			<<" buoyanceForce="<< ECHO_V3(node.buoyanceForce)
			<<" id="<<node.id
			<<" collisionBoundingBoxID="<<node.collisionBoundingBoxID
			<<" collRadius="<<node.collRadius
			<<" collTestTimer="<<node.collTestTimer
			<<" iPosition="<<node.iPosition
			<<" iDistance="<<node.iDistance
			<<" smoothpos="<<node.smoothpos
			<<" iIsSkin="<<node.iIsSkin
			<<" isSkin="<<node.isSkin
			<<" contacter="<<node.contacter
			<<" mouseGrabMode="<<node.mouseGrabMode
			<<" pos="<<node.pos
			<<" mSceneNode="<<ECHO_PTR(node.mSceneNode)
			;
	}

	// BEAMS
	f << "\n\n***********************\nBeams double check (free beam:" << rig->free_beam<<")";
	for (int i = 0; i < rig->free_beam; i++)
	{
		beam_t & beam = rig->beams[i];
		f<<"\n"<<i<<": ["<<beam.p1->pos<<","<<beam.p2->pos<<"]"

			// all beam_t members, in order
			<<" p2truck="<<ECHO_PTR(beam.p2truck)
			<<" disabled:"<<beam.disabled
			<<" k:"<<beam.k
			<<" d:"<<beam.d
			<<" L:"<<beam.L
			<<" minmaxposnegstress="<<beam.minmaxposnegstress
			<<" type:"<<beam.type
			<<" maxposstress="<<beam.maxposstress
			<<" maxnegstress="<<beam.maxnegstress
			<<" shortbound="<<beam.shortbound
			<<" longbound="<<beam.longbound
			<<" strength="<<beam.strength
			<<" stress="<<beam.stress
			<<" bounded="<<beam.bounded
			<<" broken:"<<beam.broken
			<<" plastic_coef="<<beam.plastic_coef
			<<" refL="<<beam.refL
			<<" Lhydro="<<beam.Lhydro
			<<" hydroRatio="<<beam.hydroRatio
			<<" hydroFlags="<<beam.hydroFlags
			<<" animFlags="<<beam.animFlags
			<<" animOption="<<beam.animOption
			<<" commandRatioLong="<<beam.commandRatioLong
			<<" commandRatioShort="<<beam.commandRatioShort
			<<" commandShort="<<beam.commandShort
			<<" commandLong="<<beam.commandLong
			<<" commandEngineCoupling="<<beam.commandEngineCoupling
			<<" maxtiestress="<<beam.maxtiestress
			<<" diameter="<<beam.diameter
			<<" commandNeedsEngine="<<beam.commandNeedsEngine
			<<" detacher_group="<<beam.detacher_group
			<<" lastforce="<< ECHO_V3(beam.lastforce)
			<<" isCentering="<<beam.isCentering
			<<" isOnePressMode="<<beam.isOnePressMode
			<<" isForceRestricted="<<beam.isForceRestricted
			<<" iStrength="<<beam.iStrength
			<<" default_deform="<<beam.default_deform
			<<" default_plastic_coef="<<beam.default_plastic_coef
			<<" autoMovingMode="<<beam.autoMovingMode
			<<" autoMoveLock="<<beam.autoMoveLock
			// <<" FOOOOOOO="<<beam.BARRRRRRR
			// <<" FOOOOOOO="<<ECHO_PTR(node.PTRVARNAME)
			<<" pressedCenterMode="<<beam.pressedCenterMode
			<<" centerLength="<<beam.centerLength
			<<" minendmass="<<beam.minendmass
			<<" scale="<<beam.scale
			<<" shock="<<ECHO_PTR(beam.shock)
			<<" mSceneNode="<<ECHO_PTR(beam.mSceneNode)
			<<" mEntity="<<ECHO_PTR(beam.mEntity)
			<<" *EXTRA*"
			<<" iswheel:["<<beam.p1->iswheel<<","<<beam.p2->iswheel<<"]"
			<<" wheelid:["<<beam.p1->wheelid<<","<<beam.p2->wheelid<<"]"
			;
	}

	// SHOCKS

	f << "\n\n***********************\n Shocks double check (free shock:" << rig->free_shock<<")";
	for (int i = 0; i < rig->free_shock; i++)
	{
		shock_t & shock = rig->shocks[i];
		f<<"\nShock "<<i<<":"
			// <<" FOOOOOOO="<<shock.BARRRRRRR
			// <<" FOOOOOOO="<<ECHO_PTR(shock.PTRVARNAME)
			<<" beamid="<<shock.beamid
			<<" flags="<<shock.flags
			<<" lastpos="<<shock.lastpos
			<<" springin="<<shock.springin
			<<" dampin="<<shock.dampin
			<<" sprogin="<<shock.sprogin
			<<" dprogin="<<shock.dprogin
			<<" springout="<<shock.springout
			<<" dampout="<<shock.dampout
			<<" sprogout="<<shock.sprogout
			<<" dprogout="<<shock.dprogout
			<<" sbd_spring="<<shock.sbd_spring
			<<" sbd_damp="<<shock.sbd_damp
			<<" trigger_cmdlong="<<shock.trigger_cmdlong
			<<" trigger_cmdshort="<<shock.trigger_cmdshort
			<<" trigger_enabled="<<shock.trigger_enabled
			<<" trigger_switch_state="<<shock.trigger_switch_state
			<<" trigger_boundary_t="<<shock.trigger_boundary_t
			<<" last_debug_state="<<shock.last_debug_state
			;
	}

	// struct collcab_rate_t ?

	// struct soundsource_t ?

	// TODO struct contacter_t

	// struct rigidifier_t ?

	f << "\n\n***********************\n wheel double check (free wheel:" << rig->free_wheel<<")";
	for (int i = 0; i < rig->free_wheel; i++)
	{
		wheel_t & data = rig->wheels[i];
		f<<"\n\n wheel "<<i<<":"
			// <<" FOOOOOOO="<<data.BARRRRRRR
			// <<" FOOOOOOO="<<ECHO_PTR(data.PTRVARNAME)
			// <<" FOOOOOOO="<<ECHO_NODE(data.NODEPTR)
			// <<" FOOOOOOO="<<ECHO_V3(data.OGREVECTOR3)
			<<" nbnodes="<<data.nbnodes
			<<" braked="<<data.braked
			<<" arm="<<ECHO_NODE(data.arm)
			<<" near_attach="<<ECHO_NODE(data.near_attach)
			<<" refnode0="<<ECHO_NODE(data.refnode0)
			<<" refnode1="<<ECHO_NODE(data.refnode1)
			<<" propulsed="<<data.propulsed
			<<" radius="<<data.radius
			<<" speed="<<data.speed
			<<" delta_rotation="<<data.delta_rotation
			<<" rp="<<data.rp
			<<" rp1="<<data.rp1
			<<" rp2="<<data.rp2
			<<" rp3="<<data.rp3
			<<" width="<<data.width
			<<" lastContactInner="<<ECHO_V3(data.lastContactInner)
			<<" lastContactOuter="<<ECHO_V3(data.lastContactOuter)
			<<" lastRotationVec="<<ECHO_V3(data.lastRotationVec)
			<<" firstLock="<<data.firstLock
			<<" lastSlip="<<data.lastSlip
			<<" lastContactType="<<data.lastContactType
			<<" lastGroundModel="<<ECHO_PTR(data.lastGroundModel)
			<<" lastEventHandler="<<data.lastEventHandler
			;

		f<<"\n\tNodes:";
		for (int i=0; i<data.nbnodes; ++i)
		{
			f << " "<< ECHO_NODE(data.nodes[i]);
		}
	}

	// TODO struct vwheel_t

	f << "\n\n***********************\n hook double check (hooks.size():" << rig->hooks.size()<<")";
	for (unsigned int i = 0; i < rig->hooks.size(); i++)
	{
		hook_t & data = rig->hooks[i];
		f<<"\n hook "<<i<<":"

			<<" locked="<<data.locked
			<<" group="<<data.group
			<<" lockgroup="<<data.lockgroup
			<<" lockNodes="<<data.lockNodes
			<<" selflock="<<data.selflock
			<<" autolock="<<data.autolock
			<<" nodisable="<<data.nodisable
			<<" visible="<<data.visible
			<<" maxforce="<<data.maxforce
			<<" lockrange="<<data.lockrange
			<<" lockspeed="<<data.lockspeed
			<<" timer="<<data.timer
			<<" timer_preset="<<data.timer_preset
			<<" hookNode="<<ECHO_NODE(data.hookNode)
			<<" lockNode="<<ECHO_NODE(data.lockNode)
			<<" beam="<<ECHO_BEAM(data.beam)
			<<" lockTruck="<<ECHO_PTR(data.lockTruck)
			;
	}

	// struct ropable_t

	// struct rope_t

	// struct tie_t

	// struct wing_t


	// ???? command_t commandkey[MAX_COMMANDS + 10]; // 0 for safety
	f << "\n\n***********************\n command double check (free command:" << rig->free_commands<<")";
	for (int i = 0; i < rig->free_commands; i++)
	{
		command_t & data = rig->commandkey[i];
		f<<"\n\n command "<<i<<":"
			<<" commandValueState="<<data.commandValueState
			<<" commandValue="<<data.commandValue
			<<" triggerInputValue="<<data.triggerInputValue
			<<" playerInputValue="<<data.playerInputValue
			<<" trigger_cmdkeyblock_state="<<data.trigger_cmdkeyblock_state
			<<" description="<<data.description
			;

		f<<"\n\tbeams<int>:";
		for (auto itor = data.beams.begin(); itor != data.beams.end(); ++itor)
		{
			f << " " << *itor;
		}
		f<<"\n\rotators<int>:";
		for (auto itor = data.rotators.begin(); itor != data.rotators.end(); ++itor)
		{
			f << " " << *itor;
		}
			
	}

	f << "\n\n***********************\n rotator double check (free rotator:" << rig->free_rotator<<")";
	for (int i = 0; i < rig->free_rotator; i++)
	{
		rotator_t & data = rig->rotators[i];
		f<<"\n rotator "<<i<<":"
			<<" nodes1="<<data.nodes1[0] << " "<<data.nodes1[1] 
				<< " "<<data.nodes1[2] << " "<<data.nodes1[3]
			<<" nodes2="<<data.nodes2[0] << " "<<data.nodes2[1] 
				<< " "<<data.nodes2[2] << " "<<data.nodes2[3]
			<<" axis1="<<data.axis1
			<<" axis2="<<data.axis2
			<<" angle="<<data.angle
			<<" rate="<<data.rate
			<<" force="<<data.force
			<<" tolerance="<<data.tolerance
			<<" rotatorEngineCoupling="<<data.rotatorEngineCoupling
			<<" rotatorNeedsEngine="<<data.rotatorNeedsEngine
			;
	}

	// struct flare_t

	f << "\n\n***********************\n prop double check (free prop:" << rig->free_prop<<")";
	for (int i = 0; i < rig->free_prop; i++)
	{
		prop_t & data = rig->props[i];
		f<<"\n prop "<<i<<":"
			<<" noderef="<<data.noderef
			<<" nodex="<<data.nodex
			<<" nodey="<<data.nodey
			<<" offsetx="<<data.offsetx
			<<" offsety="<<data.offsety
			<<" offsetz="<<data.offsetz
			<<" rotaX="<<data.rotaX
			<<" rotaY="<<data.rotaY
			<<" rotaZ="<<data.rotaZ
			<<" orgoffsetX="<<data.orgoffsetX
			<<" orgoffsetY="<<data.orgoffsetY
			<<" orgoffsetZ="<<data.orgoffsetZ

			<<" rot="<<ECHO_QUAT(data.rot)
			<<" snode="<<ECHO_PTR(data.snode)
			<<" wheel="<<ECHO_PTR(data.wheel)
			<<" wheelpos="<<ECHO_V3(data.wheelpos)

			<<" mirror="<<data.mirror
			<<" beacontype="<<data.beacontype
			<<" bbs="
				<<ECHO_PTR(data.bbs[0])
				<<ECHO_PTR(data.bbs[1])
				<<ECHO_PTR(data.bbs[2])
				<<ECHO_PTR(data.bbs[3])
			<<" bbsnode="
				<<ECHO_PTR(data.bbsnode[0])
				<<ECHO_PTR(data.bbsnode[1])
				<<ECHO_PTR(data.bbsnode[2])
				<<ECHO_PTR(data.bbsnode[3])
			<<" light="
				<<ECHO_PTR(data.light[0])
				<<ECHO_PTR(data.light[1])
				<<ECHO_PTR(data.light[2])
				<<ECHO_PTR(data.light[3])
			<<" brate="
				<<data.brate[0]<<" "
				<<data.brate[1]<<" "
				<<data.brate[2]<<" "
				<<data.brate[3]<<" "
			<<" bpos="
				<<data.bpos[0]<<" "
				<<data.bpos[1]<<" "
				<<data.bpos[2]<<" "
				<<data.bpos[3]<<" "
			<<" pale="<<data.pale
			<<" spinner="<<data.spinner
			<<" animated="<<data.animated
			<<" anim_x_Rot="<<data.anim_x_Rot
			<<" anim_y_Rot="<<data.anim_y_Rot
			<<" anim_z_Rot="<<data.anim_z_Rot
			<<" anim_x_Off="<<data.anim_x_Off
			<<" anim_y_Off="<<data.anim_y_Off
			<<" anim_z_Off="<<data.anim_z_Off
			;
			LOG_ARRAY("animratio", data.animratio,10);
			LOG_ARRAY("animFlags", data.animFlags,10);
			LOG_ARRAY("animMode", data.animMode,10);
			LOG_ARRAY("animOpt1", data.animOpt1,10);
			LOG_ARRAY("animOpt2", data.animOpt2,10);
			LOG_ARRAY("animOpt3", data.animOpt3,10);
			LOG_ARRAY("animOpt4", data.animOpt4,10);
			LOG_ARRAY("animOpt5", data.animOpt5,10);
			LOG_ARRAY("animKey", data.animKey, 10);
			LOG_ARRAY("animKeyState", data.animKeyState, 10);
			LOG_ARRAY("lastanimKS", data.lastanimKS, 10);
			
		f	<<" wheelrotdegree="<<data.wheelrotdegree
			<<" cameramode="<<data.cameramode
			<<" mo="<<ECHO_PTR(data.mo)
			;	
	}

	// struct exhaust_t

	// struct cparticle_t

	// struct debugtext_t

	f << "\n\n***********************\n Airbrake double check (free ELEMENT:" << rig->free_airbrake<<")";
	//NOTE: This code needs 'friend' declaration in class Airbrake
	for (int i = 0; i < rig->free_airbrake; i++)
	{
		Airbrake* data = rig->airbrakes[i];
		f<<"\n Airbrake "<<i<<":"
			<<" msh(not null?)="<<!data->msh.isNull()
			<<" snode="<<ECHO_PTR(data->snode)
			<<" noderef="<<ECHO_NODE(data->noderef)
			<<" nodex="<<ECHO_NODE(data->nodex)
			<<" nodey="<<ECHO_NODE(data->nodey)
			<<" nodea="<<ECHO_NODE(data->nodea)
			<<" offset="<<ECHO_V3(data->offset)
			<<" ratio="<<data->ratio
			<<" maxangle="<<data->maxangle
			<<" area="<<data->area
			;
	}

#if 0 // TEMPLATE
	f << "\n\n***********************\n ELEMENT double check (free ELEMENT:" << rig->free_ELEMENT<<")";
	for (int i = 0; i < rig->free_ELEMENT; i++)
	{
		ELEMENT & data = rig->ELEMENT[i];
		f<<"\n ELEMENT "<<i<<":"
			// <<" FOOOOOOO="<<data.BARRRRRRR
			// <<" FOOOOOOO="<<ECHO_PTR(data.PTRVARNAME)
			// <<" FOOOOOOO="<<ECHO_NODE(data.NODEPTR)
			// <<" FOOOOOOO="<<ECHO_BEAM(data.BEAMPTR)
			// <<" FOOOOOOO="<<ECHO_V3(data.OGREVECTOR3)
			// <<" FOOOOOOO="<<ECHO_QUAT(data.OGREQUATERNION)
			//
			// LOG_ARRAY("", , 10);
			;
	}
#endif
		
	f<<"\n\n\n @@@@@@@@@@@@@@@@@@@@@@@@@@ rig_t inspection @@@@@@@@@@@@@@@@@@@@@@@@@@";

	/*
	*** TODO: ***
		contacter_t contacters[MAX_CONTACTERS];
	int free_contacter;

	rigidifier_t rigidifiers[MAX_RIGIDIFIERS];
	int free_rigidifier;

	vwheel_t vwheels[MAX_WHEELS];
	int free_wheel;

		std::vector <rope_t> ropes;
	std::vector <ropable_t> ropables;
	std::vector <tie_t> ties;
	*/
	InspectWings(f, rig);
	

	/*
	std::vector<flare_t> flares;
	int free_flare;

	prop_t *driverSeat;

	std::vector < exhaust_t > exhausts;

	cparticle_t cparticles[MAX_CPARTICLES];
	int free_cparticle;

	std::vector<debugtext_t>nodes_debug, beams_debug;

	soundsource_t soundsources[MAX_SOUNDSCRIPTS_PER_TRUCK];
	int free_soundsource;

	
	*/

	;f<<"\n\t";
	LOG_ARRAY("pressure_beams", rig->pressure_beams, rig->free_pressure_beam);

	f<<"\n\t";
	LOG_ARRAY_PTR("aeroengines", rig->aeroengines, rig->free_aeroengine);

	f<<"\n\t";
	LOG_ARRAY_PTR("screwprops", rig->screwprops, rig->free_screwprop);

	/*TODO:
	
	int cabs[MAX_CABS*3];
	int subisback[MAX_SUBMESHES]; //!< Submesh; {0, 1, 2}
	int free_cab;
	*/

	f<<"\n\t";
	LOG_ARRAY("hydro", rig->hydro, rig->free_hydro);

	f<<"\n\tfree_texcoord:"<<rig->free_texcoord; // TODO;Ogre::Vector3 texcoords[MAX_TEXCOORDS];

	/*TODO:
	int subtexcoords[MAX_SUBMESHES];
	int subcabs[MAX_SUBMESHES];
	int free_sub;*/

	/*TODO:
	
		int collcabs[MAX_CABS];
	int collcabstype[MAX_CABS];
	collcab_rate_t inter_collcabrate[MAX_CABS];
	collcab_rate_t intra_collcabrate[MAX_CABS];
	int free_collcab;
	
		int buoycabs[MAX_CABS];
	int buoycabtypes[MAX_CABS];
	int free_buoycab;

	Skidmark *skidtrails[MAX_WHEELS*2];
	bool useSkidmarks;

	FlexBody *flexbodies[MAX_FLEXBODIES];
	int free_flexbody;

	std::vector <VideoCamera *> vidcams;

	std::vector<std::string> description;

	int cameraRail[MAX_CAMERARAIL];
	int free_camerarail;
	*/

	f<<"\n\thideInChooser:"<<rig->hideInChooser;
	f<<"\n\tguid:"<<rig->guid;
	f<<"\n\thasfixes:"<<rig->hasfixes;
	f<<"\n\twingstart:"<<rig->wingstart;
	f<<"\n\tloading_finished:"<<rig->loading_finished;

	f<<"\n\t forwardcommands:"<<rig->forwardcommands;
	f<<"\n\t importcommands:"<<rig->importcommands;
	f<<"\n\t wheel_contact_requested:"<<rig->wheel_contact_requested; // rollon?
	f<<"\n\t rescuer:"<<rig->rescuer;
	f<<"\n\t disable_default_sounds:"<<rig->disable_default_sounds;
	f<<"\n\t detacher_group_state:"<<rig->detacher_group_state;
	f<<"\n\t slopeBrake:"<<rig->slopeBrake;
	f<<"\n\t slopeBrakeFactor:"<<rig->slopeBrakeFactor;
	f<<"\n\t slopeBrakeAttAngle:"<<rig->slopeBrakeAttAngle;
	f<<"\n\t slopeBrakeRelAngle:"<<rig->slopeBrakeRelAngle;
	f<<"\n\t alb_ratio:"<<rig->alb_ratio;
	f<<"\n\t alb_minspeed:"<<rig->alb_minspeed;
	f<<"\n\t alb_mode:"<<rig->alb_mode;
	f<<"\n\t alb_pulse:"<<rig->alb_pulse;
	f<<"\n\t alb_pulse_state:"<<rig->alb_pulse_state;
	f<<"\n\t alb_present:"<<rig->hideInChooser;
	f<<"\n\t alb_notoggle:"<<rig->alb_notoggle;

	f<<"\n\t tc_ratio:"<<rig->tc_ratio;
	f<<"\n\t tc_wheelslip:"<<rig->tc_wheelslip;
	f<<"\n\t tc_fade:"<<rig->tc_fade;
	f<<"\n\t tc_mode:"<<rig->tc_mode;
	f<<"\n\t tc_pulse:"<<rig->tc_pulse;
	f<<"\n\t tc_pulse_state:"<<rig->tc_pulse_state;
	f<<"\n\t tc_present:"<<rig->tc_present;
	f<<"\n\t tc_notoggle:"<<rig->tc_notoggle;
	f<<"\n\t tcalb_timer:"<<rig->tcalb_timer;
	f<<"\n\t antilockbrake:"<<rig->antilockbrake;
	f<<"\n\t tractioncontrol:"<<rig->tractioncontrol;
	f<<"\n\t animTimer:"<<rig->animTimer;

	// cruise control
	f<<"\n\t cc_mode:"<<rig->cc_mode;
	f<<"\n\t cc_can_brake:"<<rig->cc_can_brake;
	f<<"\n\t cc_target_rpm:"<<rig->cc_target_rpm;
	f<<"\n\t cc_target_speed:"<<rig->cc_target_speed;
	f<<"\n\t cc_target_speed_lower_limit:"<<rig->cc_target_speed_lower_limit;

	f<<"\n\t sl_enabled:"<<rig->sl_enabled;
	f<<"\n\t sl_speed_limit:"<<rig->sl_speed_limit;

	f<<"\n\t uniquetruckid:"<<rig->uniquetruckid;
	f<<"\n\t categoryid:"<<rig->categoryid;
	f<<"\n\t truckversion:"<<rig->truckversion;
	f<<"\n\t externalcameramode:"<<rig->externalcameramode;
	f<<"\n\t externalcameranode:"<<rig->externalcameranode;
	// authors
	f<<"\n\t fadeDist:"<<rig->fadeDist;
	f<<"\n\t collrange:"<<rig->collrange;
	f<<"\n\t masscount:"<<rig->masscount;
	f<<"\n\t disable_smoke:"<<rig->disable_smoke;
	f<<"\n\t smokeId:"<<rig->smokeId;
	f<<"\n\t smokeRef:"<<rig->smokeRef;
	f<<"\n\t truckname:"<<rig->truckname;
	f<<"\n\t networking:"<<rig->networking;
	f<<"\n\t editorId:"<<rig->editorId;
	f<<"\n\t beambreakdebug:"<<rig->beambreakdebug;
	f<<"\n\t beamdeformdebug:"<<rig->beamdeformdebug;
	f<<"\n\t triggerdebug:"<<rig->triggerdebug;

	f<<"\n\tCmdKeyInertia rotaInertia:"<<ECHO_PTR(rig->rotaInertia);
	f<<"\n\tCmdKeyInertia hydroInertia:"<<ECHO_PTR(rig->hydroInertia);
	f<<"\n\tCmdKeyInertia cmdInertia:"<<ECHO_PTR(rig->cmdInertia);

	f<<"\n\t enable_wheel2:"<<rig->enable_wheel2;
	f<<"\n\t truckmass:"<<rig->truckmass;
	f<<"\n\t loadmass:"<<rig->loadmass;
	f<<"\n\t texname:"<<rig->texname;
	f<<"\n\t trucknum:"<<rig->trucknum;
	f<<"\n\t usedSkin:"<<ECHO_PTR(rig->usedSkin);
	f<<"\n\tBuoyance buoyance:"<<ECHO_PTR(rig->buoyance);

	if (rig->engine != nullptr)
	{
		BeamEngine & data = *rig->engine;
		f<<"\n\t *** ENGINE *** "
			<<" refWheelRevolutions="<<data.refWheelRevolutions
			<<" curWheelRevolutions="<<data.curWheelRevolutions
			<<" curGear="<<data.curGear
			<<" curGearRange="<<data.curGearRange
			<<" numGears="<<data.numGears
			// TODO: std::vector<float> gearsRatio; //!< Gears
			<<" absVelocity="<<data.absVelocity
			<<" relVelocity="<<data.relVelocity
			<<" clutchForce="<<data.clutchForce
			<<" clutchTime="<<data.clutchTime
			<<" curClutch="<<data.curClutch
			<<" curClutchTorque="<<data.curClutchTorque
			<<" contact="<<data.contact
			<<" hasair="<<data.hasair
			<<" hasturbo="<<data.hasturbo
			<<" running="<<data.running
			<<" type="<<data.type
			<<" brakingTorque="<<data.brakingTorque
			<<" curAcc="<<data.curAcc
			<<" curEngineRPM="<<data.curEngineRPM
			<<" diffRatio="<<data.diffRatio
			<<" engineTorque="<<data.engineTorque
			<<" hydropump="<<data.hydropump
			<<" idleRPM="<<data.idleRPM
			<<" minIdleMixture="<<data.minIdleMixture
			<<" maxIdleMixture="<<data.maxIdleMixture
			<<" inertia="<<data.inertia
			<<" maxRPM="<<data.maxRPM
			<<" minRPM="<<data.minRPM
			<<" stallRPM="<<data.stallRPM
			<<" prime="<<data.prime

			// shifting
			<<" post_shift_time="<<data.post_shift_time
			<<" postshiftclock="<<data.postshiftclock
			<<" shift_time="<<data.shift_time
			<<" shiftclock="<<data.shiftclock
			<<" postshifting="<<data.postshifting
			<<" shifting="<<data.shifting
			<<" shiftval="<<data.shiftval

			// auto
			<<" autoselect="<<data.autoselect
			<<" autocurAcc="<<data.autocurAcc
			<<" starter="<<data.starter

			// auto transmission
			<<" fullRPMRange="<<data.fullRPMRange
			<<" oneThirdRPMRange="<<data.oneThirdRPMRange
			<<" halfRPMRange="<<data.halfRPMRange
			<<" shiftBehaviour="<<data.shiftBehaviour
			<<" upShiftDelayCounter="<<data.upShiftDelayCounter
			/*
			std::deque<float> rpms;
			std::deque<float> accs;
			std::deque<float> brakes;*/
			<<" curTurboRPM="<<data.curTurboRPM
			<<" torqueCurve="<<(data.torqueCurve != nullptr)
			<<" apressure="<<data.apressure
			<<" automode="<<data.automode
			<<" trucknum="<<data.trucknum
			;
	}

	f<<"\n\t driveable:"<<rig->driveable;
	f<<"\n\t hascommands:"<<rig->hascommands;
	f<<"\n\t hashelp:"<<rig->hashelp;
	f<<"\n\t helpmat:"<<rig->helpmat;
	/*TODO
	
	int cinecameranodepos[MAX_CAMERAS]; //!< Cine-camera node indexes
	int freecinecamera; //!< Number of cine-cameras (lowest free index)
	*/
	f<<"\n\t flaresMode:"<<rig->flaresMode;
	f<<"\n\t cablight:"<<ECHO_PTR(rig->cablight);
	f<<"\n\t cablightNode:"<<ECHO_PTR(rig->cablightNode);
	/*TODO:
		std::vector<Ogre::Entity*> deletion_Entities; //!< For unloading vehicle; filled at spawn.
	std::vector<Ogre::MovableObject *> deletion_Objects; //!< For unloading vehicle; filled at spawn.
	std::vector<Ogre::SceneNode*> deletion_sceneNodes; //!< For unloading vehicle; filled at spawn.
		int netCustomLightArray[4];
	unsigned char netCustomLightArray_counter;
	MaterialFunctionMapper *materialFunctionMapper;
	*/
	f<<"\n\t driversseatfound:"<<rig->driversseatfound;
	f<<"\n\t ispolice:"<<rig->ispolice;
	f<<"\n\t state:"<<rig->state;
	f<<"\n\t collisionRelevant:"<<rig->collisionRelevant;
	f<<"\n\t heathaze:"<<rig->heathaze;

	f<<"\n\tAutopilot autopilot:"<<ECHO_PTR(rig->autopilot);
	f<<"\n\tHeightFinder hfinder:"<<ECHO_PTR(rig->hfinder);
	f<<"\n\tAirfoil fuseAirfoil:"<<ECHO_PTR(rig->fuseAirfoil);
	

	f<<"\n\t fuseFront:"<<ECHO_NODE(rig->fuseFront);
	f<<"\n\t fuseBack:"<<ECHO_NODE(rig->fuseBack);

	f<<"\n\t fuseWidth:"<<rig->fuseWidth;
	f<<"\n\t brakeforce:"<<rig->brakeforce;
	f<<"\n\t hbrakeforce:"<<rig->hbrakeforce;
	f<<"\n\t debugVisuals:"<<rig->debugVisuals;
	f<<"\n\t speedomat:"<<rig->speedomat;
	f<<"\n\t tachomat:"<<rig->tachomat;
	f<<"\n\t speedoMax:"<<rig->speedoMax;
	f<<"\n\t useMaxRPMforGUI:"<<rig->useMaxRPMforGUI;
	f<<"\n\t minimass:"<<rig->minimass;
	f<<"\n\t cparticle_enabled:"<<rig->cparticle_enabled;
	f<<"\n\t advanced_drag:"<<rig->advanced_drag;
	f<<"\n\t advanced_node_drag:"<<rig->advanced_node_drag;
	f<<"\n\t advanced_total_drag:"<<rig->advanced_total_drag;
	// TODO Axle *axles[MAX_WHEELS/2];
	f<<"\n\t!!TODO AXLES free_axle:"<<rig->free_axle;
	f<<"\n\t free_fixes:"<<rig->free_fixes;
	f<<"\n\t propwheelcount:"<<rig->propwheelcount;
	f<<"\n\t free_commands:"<<rig->free_commands;
	f<<"\n\t fileformatversion:"<<rig->fileformatversion;
	// TODO std::vector<Ogre::String> sectionconfigs;
	f<<"\n\t origin:"<<ECHO_V3(rig->origin);
	f<<"\n\t beamsRoot:"<<ECHO_PTR(rig->beamsRoot);

	// TODO std::vector< SlideNode > mSlideNodes;
	f<<"\n\t proped_wheels:"<<rig->proped_wheels;
	f<<"\n\t braked_wheels:"<<rig->braked_wheels;
	// TODO int proppairs[MAX_WHEELS]; //!< For inter-differential locking
	f<<"\n\t slideNodesConnectInstantly:"<<rig->slideNodesConnectInstantly;

	// TODOstd::vector< RailGroup* > mRailGroups;
	f<<"\n\t mCamera:"<<ECHO_PTR(rig->mCamera);
	f<<"\n\t freecamera:"<<rig->freecamera;
	/*	int cameranodepos[MAX_CAMERAS];
	int cameranodedir[MAX_CAMERAS];
	int cameranoderoll[MAX_CAMERAS];
		bool revroll[MAX_CAMERAS];
	*/
	f<<"\n\t shadowOptimizations:"<<rig->shadowOptimizations;
	f<<"\n\t hasEmissivePass:"<<rig->hasEmissivePass;
	f<<"\n\t cabMesh:"<<ECHO_PTR(rig->cabMesh);
	f<<"\n\t cabNode:"<<ECHO_PTR(rig->cabNode);
	f<<"\n\t boundingBox:"<<ECHO_AABB(rig->boundingBox);
	f<<"\n\t predictedBoundingBox:"<<ECHO_AABB(rig->predictedBoundingBox);
	// TODO std::vector<Ogre::AxisAlignedBox> collisionBoundingBoxes; //!< smart bounding 
	// TODO std::vector<Ogre::AxisAlignedBox> predictedCollisionBoundingBoxes;
	f<<"\n\t freePositioned:"<<rig->freePositioned;
	f<<"\n\t lowestnode:"<<rig->lowestnode;

	// SNIP: Defaults

	f<<"\n\t posnode_spawn_height:"<<rig->posnode_spawn_height;
	f<<"\n\t materialReplacer:"<<ECHO_PTR(rig->materialReplacer);
	f<<"\n\t subMeshGroundModelName:"<<rig->subMeshGroundModelName;
	f<<"\n\t odometerTotal:"<<rig->odometerTotal;
	f<<"\n\t odometerUser:"<<rig->odometerUser;
	/*TODO
	
	std::vector<std::pair<Ogre::String, bool> > dashBoardLayouts;
	Ogre::String beamHash; //!< Unused
	
	*/

	f<<"\n\n\n @@@@@@@@@@@@@@@@@@@@@@@@@@ CLASS BEAM inspection @@@@@@@@@@@@@@@@@@@@@@@@@@";
		//! @{ dynamic physical properties
	f<<"\n\t brake:"<<rig->brake;
	f<<"\n\t affforce:"<<ECHO_V3(rig->affforce);
	f<<"\n\t ffforce:"<<ECHO_V3(rig->ffforce);
	f<<"\n\t affhydro:"<<rig->affhydro;
	f<<"\n\t ffhydro:"<<rig->ffhydro;
	f<<"\n\t left_blink_on:"<<rig->left_blink_on;
	f<<"\n\t right_blink_on:"<<rig->right_blink_on;
	f<<"\n\t warn_blink_on:"<<rig->warn_blink_on;

	// Vector of vectors
	f<<"\n\tnodetonodeconnections (vector of vectors):";
	auto itor_end = rig->nodetonodeconnections.end();
	for (auto itor = rig->nodetonodeconnections.begin(); itor != itor_end; ++itor)
	{
		f<<"\n\t";
		std::vector<int> & vec = *itor;
		int size = static_cast<int>(vec.size());
		LOG_ARRAY("~",vec, size);
	}

	// Vector of vectors
	f<<"\n\nodebeamconnections (vector of vectors):";
	auto nb_itor_end = rig->nodebeamconnections.end();
	for (auto nb_itor = rig->nodebeamconnections.begin(); nb_itor != nb_itor_end; ++nb_itor)
	{
		f<<"\n\t";
		std::vector<int> & vec = *nb_itor;
		int size = static_cast<int>(vec.size());
		LOG_ARRAY("~",vec, size);
	}
	
	f<<"\n\t WheelSpeed:"<<rig->WheelSpeed;
	f<<"\n\t stabcommand:"<<rig->stabcommand;
	f<<"\n\t skeleton:"<<rig->skeleton;
	f<<"\n\t stabratio:"<<rig->stabratio;
	f<<"\n\t hydrodircommand:"<<rig->hydrodircommand;
	f<<"\n\t hydroSpeedCoupling:"<<rig->hydroSpeedCoupling;
	f<<"\n\t hydrodirstate:"<<rig->hydrodirstate;
	f<<"\n\t hydrodirwheeldisplay:"<<rig->hydrodirwheeldisplay;
	f<<"\n\t hydroaileroncommand:"<<rig->hydroaileroncommand;
	f<<"\n\t hydroaileronstate:"<<rig->hydroaileronstate;
	f<<"\n\t hydroruddercommand:"<<rig->hydroruddercommand;
	f<<"\n\t hydrorudderstate:"<<rig->hydrorudderstate;
	f<<"\n\t hydroelevatorcommand:"<<rig->hydroelevatorcommand;
	f<<"\n\t hydroelevatorstate:"<<rig->hydroelevatorstate;

	f<<"\n\t canwork:"<<rig->canwork;
	f<<"\n\t replaymode:"<<rig->replaymode;
	f<<"\n\t watercontact:"<<rig->watercontact;
	f<<"\n\t watercontactold:"<<rig->watercontactold;
	f<<"\n\t locked:"<<rig->locked;
	f<<"\n\t lockedold:"<<rig->lockedold;
	f<<"\n\t oldreplaypos:"<<rig->oldreplaypos;
	f<<"\n\t replaylen:"<<rig->replaylen;
	f<<"\n\t replaypos:"<<rig->replaypos;
	f<<"\n\t sleepcount:"<<rig->sleepcount;
	f<<"\n\t previousGear:"<<rig->previousGear;
	f<<"\n\t submesh_ground_model:"<< ECHO_PTR(rig->submesh_ground_model);
	f<<"\n\t requires_wheel_contact:"<<rig->requires_wheel_contact;
	f<<"\n\t parkingbrake:"<<rig->parkingbrake;
	f<<"\n\t lights:"<<rig->lights;
	f<<"\n\t reverselight:"<<rig->reverselight;
	f<<"\n\t leftMirrorAngle:"<<rig->leftMirrorAngle;
	f<<"\n\t rightMirrorAngle:"<<rig->rightMirrorAngle;
	f<<"\n\t refpressure:"<<rig->refpressure;

	f<<"\n\t elevator:"<<rig->elevator;
	f<<"\n\t rudder:"<<rig->rudder;
	f<<"\n\t aileron:"<<rig->aileron;
	f<<"\n\t flap:"<<rig->flap;

	f<<"\n\t fusedrag:"<<ECHO_V3(rig->fusedrag);
	f<<"\n\t disableDrag:"<<rig->disableDrag;
	f<<"\n\t disableTruckTruckCollisions:"<<rig->disableTruckTruckCollisions;
	f<<"\n\t disableTruckTruckSelfCollisions:"<<rig->disableTruckTruckSelfCollisions;
	f<<"\n\t currentcamera:"<<rig->currentcamera;
	f<<"\n\t first_wheel_node:"<<rig->first_wheel_node;
	f<<"\n\t netbuffersize:"<<rig->netbuffersize;
	f<<"\n\t nodebuffersize:"<<rig->nodebuffersize;
	f<<"\n\t netLabelNode:"<<ECHO_PTR(rig->netLabelNode);

	f<<"\n\t realtruckfilename:"<<rig->realtruckfilename;
	f<<"\n\t tdt:"<<rig->tdt;
	f<<"\n\t ttdt:"<<rig->ttdt;
	f<<"\n\t simulated:"<<rig->simulated;
	f<<"\n\t airbrakeval:"<<rig->airbrakeval;
	f<<"\n\t cameranodeacc:"<<ECHO_V3(rig->cameranodeacc);
	f<<"\n\t cameranodecount:"<<rig->cameranodecount;

	f<<"\n\t beamsVisible:"<<rig->beamsVisible;
	f<<"\n\tDashBoardManager dash:"<<ECHO_PTR(rig->dash);
	f<<"\n\t calledby:"<<ECHO_PTR(rig->calledby);
	f<<"\n\t flexable_task_count:"<<rig->flexable_task_count;

	f<<"\n\t dtperstep:"<<rig->dtperstep;
	f<<"\n\t curtstep:"<<rig->curtstep;
	f<<"\n\t tsteps:"<<rig->tsteps;
	f<<"\n\t num_simulated_trucks:"<<rig->num_simulated_trucks;
	f<<"\n\t avichatter_timer:"<<rig->avichatter_timer;
	/*
		// pthread stuff
	int task_count[THREAD_MAX];
	pthread_cond_t task_count_cv[THREAD_MAX];
	pthread_mutex_t task_count_mutex[THREAD_MAX];
	pthread_mutex_t task_index_mutex[THREAD_MAX];

	ThreadTask thread_task;
	*/
	f<<"\n\t thread_index:"<<rig->thread_index;
	f<<"\n\t thread_number:"<<rig->thread_number;
	/*
		std::vector<PointColDetector*> interPointCD;
	std::vector<PointColDetector*> intraPointCD;
	*/
		/*
		// flexable stuff
	std::bitset<MAX_WHEELS> flexmesh_prepare;
	std::bitset<MAX_FLEXBODIES> flexbody_prepare;

	// linked beams (hooks)
	std::list<Beam*> linkedBeams;
	void determineLinkedBeams();
	*/

	f<<"\n\t position:"<<ECHO_V3(rig->position);
	f<<"\n\t iPosition:"<<ECHO_V3(rig->iPosition);
	f<<"\n\t lastposition:"<<ECHO_V3(rig->lastposition);
	f<<"\n\t lastlastposition:"<<ECHO_V3(rig->lastlastposition);
	f<<"\n\t minCameraRadius:"<<rig->minCameraRadius;

	f<<"\n\t replayTimer:"<<rig->replayTimer;
	f<<"\n\t replayPrecision:"<<rig->replayPrecision;
	f<<"\n\t lastFuzzyGroundModel:"<<ECHO_PTR(rig->lastFuzzyGroundModel);
	f<<"\n\t blinkingtype:"<<rig->blinkingtype;
	f<<"\n\t deleting:"<<rig->deleting;
	f<<"\n\t hydrolen:"<<rig->hydrolen;

	f<<"\n\t smokeNode:"<<ECHO_PTR(rig->smokeNode);
	f<<"\n\t smoker:"<<ECHO_PTR(rig->smoker);
	f<<"\n\t stabsleep:"<<rig->stabsleep;
	f<<"\n\t replay:"<<ECHO_PTR(rig->replay);
	f<<"\n\t posStorage:"<<ECHO_PTR(rig->posStorage);
	f<<"\n\t cparticle_mode:"<<rig->cparticle_mode;
	f<<"\n\t ttrucks:"<<ECHO_PTR(rig->ttrucks);
	f<<"\n\t tnumtrucks:"<<rig->tnumtrucks;
	f<<"\n\t detailLevel:"<<rig->detailLevel;
	f<<"\n\t increased_accuracy:"<<rig->increased_accuracy;
	f<<"\n\t isInside:"<<rig->isInside;
	f<<"\n\t beacon:"<<rig->beacon;
	f<<"\n\t totalmass:"<<rig->totalmass;

	f<<"\n\t mousenode:"<<rig->mousenode;
	f<<"\n\t mousepos:"<<ECHO_V3(rig->mousepos);
	f<<"\n\t mousemoveforce:"<<rig->mousemoveforce;
	f<<"\n\t reset_requested:"<<rig->reset_requested;
	// std::vector<Ogre::String> m_truck_config;
	f<<"\n\t oob1:"<<ECHO_PTR(rig->oob1);
	f<<"\n\t oob2:"<<ECHO_PTR(rig->oob2);
	f<<"\n\t oob3:"<<ECHO_PTR(rig->oob3);
	f<<"\n\t netb1:"<<ECHO_PTR(rig->netb1);
	f<<"\n\t netb2:"<<ECHO_PTR(rig->netb2);
	f<<"\n\t netb3:"<<ECHO_PTR(rig->netb3);
	// pthread_mutex_t net_mutex;
	f<<"\n\t nettimer:"<<ECHO_PTR(rig->nettimer);
	
	
	f<<"\n\t net_toffset:"<<rig->net_toffset;
	f<<"\n\t netcounter:"<<rig->netcounter;
	f<<"\n\t netMT:"<<ECHO_PTR(rig->netMT);
	f<<"\n\t networkUsername:"<<rig->networkUsername;
	f<<"\n\t networkAuthlevel:"<<rig->networkAuthlevel;
	f<<"\n\t netBrakeLight:"<<rig->netBrakeLight;
	f<<"\n\t netReverseLight:"<<rig->netReverseLight;
	f<<"\n\t mTimeUntilNextToggle:"<<rig->mTimeUntilNextToggle;
	f<<"\n\t cabFadeTimer:"<<rig->cabFadeTimer;
	f<<"\n\t cabFadeTime:"<<rig->cabFadeTime;
	f<<"\n\t cabFadeMode:"<<rig->cabFadeMode;
	f<<"\n\t lockSkeletonchange:"<<rig->lockSkeletonchange;
	f<<"\n\t floating_origin_enable:"<<rig->floating_origin_enable;

	f<<"\n\t simpleSkeletonManualObject:"<<ECHO_PTR(rig->simpleSkeletonManualObject);
	f<<"\n\t simpleSkeletonNode:"<<ECHO_PTR(rig->simpleSkeletonNode);
	f<<"\n\t simpleSkeletonInitiated:"<<rig->simpleSkeletonInitiated;
	f<<"\n\t blinktreshpassed:"<<rig->blinktreshpassed;

	// SNIP
	/*
		// overloaded from Streamable:
	Ogre::Timer netTimer;
	int last_net_time;
	void sendStreamSetup();
	void receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len);
	*/

	// f<<"\n\t RRRRR:"<<ECHO_PTR(rig->RRRRRR);
	f<<"\n\t dustp:"<<ECHO_PTR(rig->dustp);
	f<<"\n\t dripp:"<<ECHO_PTR(rig->dripp);
	f<<"\n\t sparksp:"<<ECHO_PTR(rig->sparksp);
	f<<"\n\t clumpp:"<<ECHO_PTR(rig->clumpp);
	f<<"\n\t splashp:"<<ECHO_PTR(rig->splashp);
	f<<"\n\t ripplep:"<<ECHO_PTR(rig->ripplep);
	f<<"\n\t SlideNodesLocked:"<<rig->SlideNodesLocked;
	f<<"\n\t GUIFeaturesChanged:"<<rig->GUIFeaturesChanged;

	f<<"\n\n\nEND!" << std::endl;

	f.close();

	// THAT's ALL FOLKS !!

}

}; //class RigInspector


