/*

RIG INSPECTOR

Written 10/2014 by Petr "only_a_ptr" Ohlidal

This code logs all data contained within class Beam and associated data structs.

The purpose is to debug class RigSpawner, introduced in TruckParser2013 project 
	and replacing class SerializedRig.

Neither the code nor the output is meant to be elegant. The output is not meant
	to be read by a human, just compared using a DIFF tool.

*/

#include "RoR_RigInspector.h"


#include "AirBrake.h"
#include "BeamData.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "Differentials.h"
#include "FlexAirfoil.h"
#include "FlexBody.h"
#include "ScrewProp.h"
#include "SoundScriptManager.h"
#include "TurboJet.h"
#include "TurboProp.h"
#include "VideoCamera.h"

// ----------------------------------------------------------------------------
// Helper macros, to be used with std::fstream and << operator
// ----------------------------------------------------------------------------

#define ECHO_V3(V3) "["<<V3.x<<","<<V3.y<<","<<V3.z<<"]"

#define ECHO_V2(V2) "["<<V2.x<<","<<V2.y<<"]"

#define ECHO_QUAT(Q) "[" <<Q.x<<", "<<Q.y <<", "<<Q.z<<", " <<Q.w  <<"]"

#define ECHO_AABB(BOX) 	"@AABB{{" <<ECHO_V3(BOX.getMinimum())<<","	<<ECHO_V3(BOX.getMaximum())<<",finite="<<BOX.isFinite() <<"}}AABB@"

#define ECHO_PTR(PTR) ((PTR) != nullptr ? "PTR_OK" : "nullptr")

#define ECHO_NODE(N) "[" << ECHO_PTR(N) <<"," <<(((N)!= nullptr) ? N->pos : -1) << "]"

#define ECHO_BEAM(B) "{" << ECHO_PTR(B) <<", n1:" << ECHO_NODE(B->p1) <<", n2:" << ECHO_NODE(B->p2) << "}"

#define LOG_ARRAY(VARNAME, ARRAY, SIZE) \
{ \
	int size = static_cast<int>((SIZE)); \
	f << (VARNAME) << "(size=" << (SIZE) <<")" << ":"; \
	for (int i=0; i<size; ++i){ f <<((ARRAY)[i]); f << ","; }     \
}

#define LOG_ARRAY_PTR(VARNAME, ARRAY, SIZE)  \
{ \
	int size = static_cast<int>((SIZE)); \
	f << (VARNAME) << "(size=" << (SIZE) <<")" << ":" \
	; for (int i=0; i<(size); ++i) { \
		f<<"[" <<i <<":" <<(ECHO_PTR((ARRAY[i]))) <<"]" ; \
	 } \
} 

#define LOG_ARRAY_AABB(VARNAME, ARRAY, SIZE)  \
{ \
	int size = static_cast<int>((SIZE)); \
	f << (VARNAME) << "(size=" << (SIZE) <<")" << ":" \
	; for (int i=0; i<(size); ++i) { \
		f<<"[" <<i <<":" <<(ECHO_AABB((ARRAY[i]))) <<"]" ; \
	 } \
}

#define LOG_ARRAY_HardwareVertexBufferSharedPtr(VARNAME, ARRAY, SIZE)  \
{ \
	int size = static_cast<int>((SIZE)); \
	f << (VARNAME) << "(size=" << (SIZE) <<")" << ":"; \
	for (int i=0; i<(size); ++i){ f <<(((ARRAY)[i]).isNull()); f << ","; }     \
}

// ----------------------------------------------------------------------------
// Inspector code.
// ----------------------------------------------------------------------------

void RigInspector::InspectRig(Beam* rig, std::string const & out_path)
{
	std::ofstream f;
	f.open(out_path);
	if (! f.is_open())
	{
		LOG("InspectRig(): failed to open file "+out_path);
		return;
	}

	// ================= Inspection! ================= //

	// nodes
	InspectNodes(f, rig);

	// beams
	InspectBeams(f, rig);
	
	// shocks
	InspectShocks(f, rig);

	// collcab_rate_t
	InspectCollcabRate(f, rig);

	// soundsources
	InspectSoundsources(f, rig);

	// struct contacter_t
	InspectContacters(f, rig);

	// struct rigidifier_t
	InspectRigidifiers(f, rig);

	// wheels + vwheel_t
	InspectWheels(f, rig);

	// hooks
	InspectHooks(f, rig);

	// struct ropable_t
	InspectRopables(f, rig);

	// struct rope_t
	InspectRopes(f, rig);

	// struct tie_t
	InspectTies(f, rig);

	// commandkey
	InspectCommandkey(f, rig);

	// rotator
	InspectRotator(f, rig);

	// struct flare_t
	InspectFlares(f, rig);

	// props
	InspectProps(f, rig);
	
	// struct exhaust_t
	InspectExhausts(f, rig);

	// struct debugtext_t

	// airbrakes
	InspectAirbrakes(f, rig);
	
	// rig_t itself
	InspectStructRig(f, rig);

	// Wings
	InspectWings(f, rig);
	
	/* TODO
	cparticle_t cparticles[MAX_CPARTICLES];
	int free_cparticle;

	std::vector<debugtext_t>nodes_debug, beams_debug;
	*/

	// aeroengines
	InspectAeroengines(f, rig);

	// screwprops
	InspectScrewprops(f, rig);

	/*TODO:
	Skidmark *skidtrails[MAX_WHEELS*2];
	bool useSkidmarks;
	*/

	// flexbodies
	InspectFlexbodies(f, rig);

	// vidcams
	InspectVideocameras(f, rig);

	// engine
	InspectEngine(f, rig);

	// Axle
	InspectAxles(f, rig);



	/*TODO:
		std::vector<Ogre::Entity*> deletion_Entities; //!< For unloading vehicle; filled at spawn.
	std::vector<Ogre::MovableObject *> deletion_Objects; //!< For unloading vehicle; filled at spawn.
	std::vector<Ogre::SceneNode*> deletion_sceneNodes; //!< For unloading vehicle; filled at spawn.
		int netCustomLightArray[4];
	unsigned char netCustomLightArray_counter;
	MaterialFunctionMapper *materialFunctionMapper;
	*/
	
	
	// dashBoardLayouts
	InspectDashboardLayouts(f, rig);

	// finally, class Beam
	InspectClassBeam(f, rig);

	f<<"\n\n\nEND!" << std::endl;

	f.close();

	// THAT's ALL FOLKS !!
}

void RigInspector::InspectWings(std::ofstream & f, Beam* rig)
{

	f << "\n\n***********************\n wings double check (free_wing:" << rig->free_wing<<")";
	for (int i = 0; i < rig->free_wing; i++)
	{
		wing_t & data = rig->wings[i];
		f<<"\n wing "<<i<<":"
			<<"(FlexAirfoil*) fa="<<ECHO_PTR(data.fa)
			<<"(Ogre::SceneNode*) cnode="<<ECHO_PTR(data.cnode);

		FlexAirfoil & fa = *data.fa;
		RigInspector::InspectFlexAirfoil(f, fa);
	}
}

void RigInspector::InspectFlexAirfoil(std::ofstream & f, FlexAirfoil & fa)
{
	f<<"\n FlexAirfoil:\n"
		<<" aoa="<<fa.aoa
		<<" type="<<fa.type
		<<" nfld="<<fa.nfld
		<<" nfrd="<<fa.nfrd
		<<" nflu="<<fa.nflu
		<<" nfru="<<fa.nfru
		<<" nbld="<<fa.nbld
		<<" nbrd="<<fa.nbrd
		<<" nblu="<<fa.nblu
		<<" nbru="<<fa.nbru
		<<" broken="<<fa.broken
		<<" breakable="<<fa.breakable
		<<" liftcoef="<<fa.liftcoef

		<<" (Ogre::MeshPtr)msh="<<!fa.msh.isNull()
		<<" subface="<<ECHO_PTR(fa.subface)
		<<" subband="<<ECHO_PTR(fa.subband)
		<<" subcup="<<ECHO_PTR(fa.subcup)
		<<" subcdn="<<ECHO_PTR(fa.subcdn)
		<<" decl="<<ECHO_PTR(fa.decl)
		<<" vbuf="<<fa.vbuf.isNull()
		<<" nVertices="<<fa.nVertices
		<<" vbufCount="<<fa.vbufCount

		/* SHADOW STUFF
			union
			{
				float *shadowposvertices;
				posVertice_t *coshadowposvertices;
			};
			union
			{
				float *shadownorvertices;
				norVertice_t *coshadownorvertices;
			};
			union
			{
				float *vertices;
				CoVertice_t *covertices;
			};
		*/

		<<" faceibufCount="<<fa.faceibufCount
		<<" bandibufCount="<<fa.bandibufCount
		<<" cupibufCount="<<fa.cupibufCount
		<<" cdnibufCount="<<fa.cdnibufCount

		<<" facefaces="<<ECHO_PTR(fa.facefaces)
		<<" bandfaces="<<ECHO_PTR(fa.bandfaces)
		<<" cupfaces="<<ECHO_PTR(fa.cupfaces)
		<<" cdnfaces="<<ECHO_PTR(fa.cdnfaces)

		<<" nodes(TODO:C_ARRAY)="<<ECHO_NODE(fa.nodes)

		<<" sref="<<fa.sref
		<<" deflection="<<fa.deflection
		<<" chordratio="<<fa.chordratio
		<<" hascontrol="<<fa.hascontrol
		<<" isstabilator="<<fa.isstabilator
		<<" stabilleft="<<fa.stabilleft
		<<" lratio="<<fa.lratio
		<<" rratio="<<fa.rratio
		<<" mindef="<<fa.mindef
		<<" maxdef="<<fa.maxdef
		<<" thickness="<<fa.thickness
		<<" useInducedDrag="<<fa.useInducedDrag
		<<" idSpan="<<fa.idSpan
		<<" idArea="<<fa.idArea
		<<" idLeft="<<fa.idLeft
		<<" airfoil="<<ECHO_PTR(fa.airfoil)
		<<" aeroengines="<<ECHO_PTR(fa.aeroengines)

		<<" free_wash="<<fa.free_wash;
		
		LOG_ARRAY(" waspropnum", fa.washpropnum, MAX_AEROENGINES);
		LOG_ARRAY(" washpropratio", fa.washpropratio, MAX_AEROENGINES);
}

void RigInspector::InspectNodes(std::ofstream & f, Beam* rig)
{
	f << "\n***********************\nNodes double check (free node:" << rig->free_node<<")";
	for (int i = 0; i < rig->free_node; i++)
	{
		node_t & node = rig->nodes[i];
		f<<"\nNode"
			<<" index="<<i
			// all node_t members, in order
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
}

void RigInspector::InspectBeams(std::ofstream & f, Beam* rig)
{
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
}

void RigInspector::InspectShocks(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n Shocks double check (free shock:" << rig->free_shock<<")";
	for (int i = 0; i < rig->free_shock; i++)
	{
		shock_t & shock = rig->shocks[i];
		f<<"\nShock "<<i<<":"
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
}

void RigInspector::InspectWheels(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n wheel double check (free wheel:" << rig->free_wheel<<")";
	for (int i = 0; i < rig->free_wheel; i++)
	{
		wheel_t & data = rig->wheels[i];
		f<<"\n\n wheel "<<i<<":"
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
		for (int j=0; j<data.nbnodes; ++j)
		{
			f << " "<< ECHO_NODE(data.nodes[j]);
		}

		f<<"\t\tVisual wheel (vwheel): ";
		vwheel_t & visuals = rig->vwheels[i];
		f
			<<" p1="<<ECHO_NODE(visuals.p1)
			<<" p2="<<ECHO_NODE(visuals.p2)
			<<" fm(Flexable *)="<<ECHO_PTR(visuals.fm) // TODO: Flexable * inspection
			<<" cnode="<<ECHO_PTR(visuals.cnode)
			<<" meshwheel="<<visuals.meshwheel
			;
	}
}

void RigInspector::InspectHooks(std::ofstream & f, Beam* rig)
{
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
}

void RigInspector::InspectClassBeam(std::ofstream & f, Beam* rig)
{

	f<<"\n\n\n @@@@@@@@@@@@@@@@@@@@@@@@@@ CLASS BEAM inspection @@@@@@@@@@@@@@@@@@@@@@@@@@";

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
		LOG_ARRAY("~",vec, vec.size());
	}

	// Vector of vectors
	f<<"\n\nodebeamconnections (vector of vectors):";
	auto nb_itor_end = rig->nodebeamconnections.end();
	for (auto nb_itor = rig->nodebeamconnections.begin(); nb_itor != nb_itor_end; ++nb_itor)
	{
		f<<"\n\t";
		std::vector<int> & vec = *nb_itor;
		LOG_ARRAY("~",vec, vec.size());
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
	*/

	f<<"\n\t dustp:"<<ECHO_PTR(rig->dustp);
	f<<"\n\t dripp:"<<ECHO_PTR(rig->dripp);
	f<<"\n\t sparksp:"<<ECHO_PTR(rig->sparksp);
	f<<"\n\t clumpp:"<<ECHO_PTR(rig->clumpp);
	f<<"\n\t splashp:"<<ECHO_PTR(rig->splashp);
	f<<"\n\t ripplep:"<<ECHO_PTR(rig->ripplep);
	f<<"\n\t SlideNodesLocked:"<<rig->SlideNodesLocked;
	f<<"\n\t GUIFeaturesChanged:"<<rig->GUIFeaturesChanged;

}

void RigInspector::InspectCommandkey(std::ofstream & f, Beam* rig)
{
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
}

void RigInspector::InspectRotator(std::ofstream & f, Beam* rig)
{
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
}

void RigInspector::InspectProps(std::ofstream & f, Beam* rig)
{
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
}

void RigInspector::InspectAirbrakes(std::ofstream & f, Beam* rig)
{
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
}

void RigInspector::InspectStructRig(std::ofstream & f, Beam* rig)
{
	f<<"\n\n\n @@@@@@@@@@@@@@@@@@@@@@@@@@ rig_t inspection @@@@@@@@@@@@@@@@@@@@@@@@@@";

	f<<"\n\tdriverSeat:"<<ECHO_PTR(rig->driverSeat);

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

	f<<"\n\t driveable:"<<rig->driveable;
	f<<"\n\t hascommands:"<<rig->hascommands;
	f<<"\n\t hashelp:"<<rig->hashelp;
	f<<"\n\t helpmat:"<<rig->helpmat;

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

	// cameranodepos
	f<<"\n\t";
	LOG_ARRAY("cameranodepos", rig->cameranodepos, MAX_CAMERAS);

	// cameranodedir
	f<<"\n\t";
	LOG_ARRAY("cameranodedir", rig->cameranodedir, MAX_CAMERAS);

	// cameranoderoll
	f<<"\n\t";
	LOG_ARRAY("cameranoderoll", rig->cameranoderoll, MAX_CAMERAS);

	// revroll
	f<<"\n\t";
	LOG_ARRAY("revroll", rig->revroll, MAX_CAMERAS);

	f<<"\n\t shadowOptimizations:"<<rig->shadowOptimizations;
	f<<"\n\t hasEmissivePass:"<<rig->hasEmissivePass;
	f<<"\n\t cabMesh:"<<ECHO_PTR(rig->cabMesh);
	f<<"\n\t cabNode:"<<ECHO_PTR(rig->cabNode);
	f<<"\n\t boundingBox:"<<ECHO_AABB(rig->boundingBox);
	f<<"\n\t predictedBoundingBox:"<<ECHO_AABB(rig->predictedBoundingBox);

	// collisionBoundingBoxes
	f<<"\n\t";
	LOG_ARRAY("collisionBoundingBoxes", rig->collisionBoundingBoxes, rig->collisionBoundingBoxes.size());

	// predictedCollisionBoundingBoxes
	f<<"\n\t";
	LOG_ARRAY("predictedCollisionBoundingBoxes", rig->predictedCollisionBoundingBoxes, rig->predictedCollisionBoundingBoxes.size());

	f<<"\n\t freePositioned:"<<rig->freePositioned;
	f<<"\n\t lowestnode:"<<rig->lowestnode;

	// SNIP: Defaults

	f<<"\n\t posnode_spawn_height:"<<rig->posnode_spawn_height;
	f<<"\n\t materialReplacer:"<<ECHO_PTR(rig->materialReplacer);
	f<<"\n\t subMeshGroundModelName:"<<rig->subMeshGroundModelName;
	f<<"\n\t odometerTotal:"<<rig->odometerTotal;
	f<<"\n\t odometerUser:"<<rig->odometerUser;

	f<<"\n\t flaresMode:"<<rig->flaresMode;
	f<<"\n\t cablight:"<<ECHO_PTR(rig->cablight);
	f<<"\n\t cablightNode:"<<ECHO_PTR(rig->cablightNode);

	f<<"\n\t";
	LOG_ARRAY("hydro", rig->hydro, rig->free_hydro);

	f<<"\n\t texcoords";
	for (int i=0; i<rig->free_texcoord; ++i)
	{
		f<<ECHO_V3(rig->texcoords[i]);
	}
	f<<"\n\tfree_texcoord:"<<rig->free_texcoord;

	f<<"\n\t";
	LOG_ARRAY("pressure_beams", rig->pressure_beams, rig->free_pressure_beam);

	f<<"\n\t";
	LOG_ARRAY("collcabs", rig->collcabs, MAX_CABS);

	f<<"\n\t";
	LOG_ARRAY("collcabstype", rig->collcabstype, MAX_CABS);

	f<<"\n\t free_collcab:"<<rig->free_collcab;

	f<<"\n\t";
	LOG_ARRAY("buoycabs", rig->buoycabs, MAX_CABS);

	f<<"\n\t";
	LOG_ARRAY("buoycabtypes", rig->buoycabtypes, MAX_CABS);

	f<<"\n\t free_buoycab:"<<rig->free_buoycab;

	f<<"\n\t";
	LOG_ARRAY("cabs", rig->cabs, MAX_CABS*3);

	f<<"\n\t";
	LOG_ARRAY("subisback", rig->subisback, MAX_SUBMESHES);

	f<<"\n\t free_cab:"<<rig->free_cab;

	f<<"\n\t";
	LOG_ARRAY("subtexcoords", rig->subtexcoords, MAX_SUBMESHES);

	f<<"\n\t";
	LOG_ARRAY("subcabs", rig->subcabs, MAX_SUBMESHES);

	f<<"\n\t free_sub:"<<rig->free_sub;

	// cinecams
	f<<"\n\t";
	LOG_ARRAY("cinecameranodepos", rig->cinecameranodepos, rig->freecinecamera);
	f<<"\n\t freecinecamera:"<<rig->freecinecamera;

	// description
	f<<"\n\t";
	LOG_ARRAY("description", rig->description, rig->description.size());

	// cameraRail
	f<<"\n\t";
	LOG_ARRAY("cameraRail", rig->cameraRail, rig->free_camerarail);
	f<<"\n\t free_camerarail:"<<rig->free_camerarail;

}

void RigInspector::InspectEngine(std::ofstream & f, Beam* rig)
{
	if (rig->engine != nullptr)
	{
		BeamEngine & data = *rig->engine;
		f<<"\n\t *** ENGINE *** "
			<<" refWheelRevolutions="<<data.refWheelRevolutions
			<<" curWheelRevolutions="<<data.curWheelRevolutions
			<<" curGear="<<data.curGear
			<<" curGearRange="<<data.curGearRange
			<<" numGears="<<data.numGears
			
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

			<<" curTurboRPM="<<data.curTurboRPM
			<<" torqueCurve="<<(data.torqueCurve != nullptr)
			<<" apressure="<<data.apressure
			<<" automode="<<data.automode
			<<" trucknum="<<data.trucknum
			;
		
		LOG_ARRAY("gearsRatio", data.gearsRatio, data.gearsRatio.size());

		LOG_ARRAY("rpms", data.rpms, data.rpms.size());
		LOG_ARRAY("accs", data.accs, data.accs.size());
		LOG_ARRAY("brakes", data.brakes, data.brakes.size());
		
	}
	else
	{
		f << "\n **ENGINE: nullptr\n";
	}
}

void RigInspector::PrintCollcabRate(std::ofstream & f, collcab_rate_t & data)
{
	f	<<" rate="<<data.rate
		<<" distance="<<data.distance
		<<" update="<<data.update
		<<" calcforward="<<data.calcforward
	;	
}

void RigInspector::InspectCollcabRate(std::ofstream & f, Beam* rig)
{
	f << "\n\n============ collcab_rate_t ============\n";
	f << "inter:\n";
	for (int i = 0; i < MAX_CABS; i++)
	{
		f << i << ":";
		PrintCollcabRate(f, rig->inter_collcabrate[i]);
	}
	f << "intra:\n";
	for (int i = 0; i < MAX_CABS; i++)
	{
		f << i << ":";
		PrintCollcabRate(f, rig->intra_collcabrate[i]);
	}
}

void RigInspector::InspectSoundsources(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n soundsources double check (free_soundsource:" << rig->free_soundsource<<")";
	for (int i = 0; i < rig->free_soundsource; i++)
	{
		soundsource_t & data = rig->soundsources[i];
		f<<"\n ELEMENT "<<i<<":"
			<<" ssi="<<ECHO_PTR(data.ssi)
			<<" nodenum="<<data.nodenum
			<<" type="<<data.type
			;

		if (data.ssi != nullptr)
		{
			SoundScriptInstance & ssi = *data.ssi;
			f<<" [[SoundScriptInstance]] "
				<<" templ="<<ECHO_PTR(ssi.templ)
				<<" sound_manager="<<ECHO_PTR(ssi.sound_manager)
				<<" start_sound="<<ECHO_PTR(ssi.start_sound)
				<<" stop_sound="<<ECHO_PTR(ssi.stop_sound)

				<<" start_sound_pitchgain="<<ssi.start_sound_pitchgain
				<<" stop_sound_pitchgain="<<ssi.stop_sound_pitchgain
				<<" lastgain="<<ssi.lastgain
				<<" truck="<<ssi.truck
				<<" sound_link_type="<<ssi.sound_link_type
				<<" sound_link_item_id="<<ssi.sound_link_item_id;

			LOG_ARRAY_PTR("sounds", ssi.sounds, MAX_SOUNDS_PER_SCRIPT);
			LOG_ARRAY("sounds_pitchgain", ssi.sounds_pitchgain, MAX_SOUNDS_PER_SCRIPT);
		}
	}
}

void RigInspector::InspectRopables(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n ropable_t double check (size:" << rig->ropables.size()<<")";
	auto end = rig->ropables.end();
	int i = 0;
	for(auto itor = rig->ropables.begin(); itor != end; ++itor)
	{
		ropable_t & data = *itor;
		f<<"\n ropable_t "<<i<<":";
		PrintRopable(f, &data);
		++i;
	}
}

void RigInspector::PrintRopable(std::ofstream & f, ropable_t * ptr)
{
	if (ptr == nullptr)
	{
		f << "Ropable: nullptr";
	}
	else
	{
		ropable_t & data = *ptr;
		f << "Ropable: "
			<<" node="<<ECHO_NODE(data.node)
			<<" group="<<data.group
			<<" multilock="<<data.multilock
			<<" used="<<data.used;
	}
}

void RigInspector::InspectRopes(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n rope_t double check (size:" << rig->ropes.size()<<")";
	auto end = rig->ropes.end();
	int i = 0;
	for(auto itor = rig->ropes.begin(); itor != end; ++itor)
	{
		rope_t & data = *itor;
		f<<"\n rope_t "<<i<<":"
			<<" locked="<<data.locked
			<<" group="<<data.group
			<<" beam="<<ECHO_BEAM(data.beam)
			<<" lockedtruck="<<ECHO_PTR(data.lockedtruck);
		f << " lockedto_ropable=";
		PrintRopable(f, data.lockedto_ropable);

		++i;
	}
}

void RigInspector::InspectTies(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n tie_t double check (size:" << rig->ties.size()<<")";
	auto end = rig->ties.end();
	int i = 0;
	for(auto itor = rig->ties.begin(); itor != end; ++itor)
	{
		tie_t & data = *itor;
		f<<"\n tie_t "<<i<<":"
			<<" beam="<<ECHO_BEAM(data.beam)
			<<" group="<<data.group
			<<" tied="<<data.tied
			<<" tying="<<data.tying
			<<" commandValue="<<data.commandValue
			<<" lockedto="<<ECHO_PTR(data.lockedto)
			;

		// TODO: Fix uninitialized pointer.
		//PrintRopable(f, data.lockedto);

		++i;
	}
}

void RigInspector::InspectContacters(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n contacter_t double check (free ELEMENT:" << rig->free_contacter<<")";
	for (int i = 0; i < rig->free_contacter; i++)
	{
		contacter_t & data = rig->contacters[i];
		f<<"\n ELEMENT "<<i<<":"
			<<" nodeid="<<data.nodeid
			<<" contacted="<<data.contacted
			<<" opticontact="<<data.opticontact
			;
	}
}

void RigInspector::InspectRigidifiers(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n"
		<<"rigidifier_t double check (free element:" << rig->free_rigidifier<<")";
	for (int i = 0; i < rig->free_rigidifier; i++)
	{
		rigidifier_t & data = rig->rigidifiers[i];
		f<<"\n rigidifier_t "<<i<<":"
			<<" a="<<ECHO_NODE(data.a)
			<<" b="<<ECHO_NODE(data.b)
			<<" c="<<ECHO_NODE(data.c)
			<<" k="<<data.k
			<<" d="<<data.d
			<<" alpha="<<data.alpha
			<<" lastalpha="<<data.lastalpha
			<<" beama="<<ECHO_BEAM(data.beama)
			<<" beamc="<<ECHO_BEAM(data.beamc)
			;
	}
}

void RigInspector::InspectFlares(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n flare_t double check"
		<< "(free ELEMENT:" << rig->free_flare<<")";
	for (int i = 0; i < rig->free_flare; i++)
	{
		flare_t & data = rig->flares[i];
		f<<"\n flare_t "<<i<<":"
			<<" noderef="<<data.noderef
			<<" nodex="<<data.nodex
			<<" nodey="<<data.nodey
			<<" offsetx="<<data.offsetx
			<<" offsety="<<data.offsety
			<<" offsetz="<<data.offsetz

			<<" snode="<<ECHO_PTR(data.snode)
			<<" bbs="<<ECHO_PTR(data.bbs)
			<<" light="<<ECHO_PTR(data.light)

			<<" type="<<data.type
			<<" controlnumber="<<data.controlnumber
			<<" controltoggle_status="<<data.controltoggle_status
			<<" blinkdelay="<<data.blinkdelay
			<<" blinkdelay_curr="<<data.blinkdelay_curr
			<<" blinkdelay_state="<<data.blinkdelay_state
			<<" size="<<data.size
			<<" isVisible="<<data.isVisible
			;
	}
}

void RigInspector::InspectExhausts(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n exhaust_t double check"
		<<"(size:" << rig->exhausts.size()<<")";
	auto end = rig->exhausts.end();
	int i = 0;
	for(auto itor = rig->exhausts.begin(); itor != end; ++itor)
	{
		exhaust_t & data = *itor;
		f<<"\n TYPE "<<i<<":"
			<<" emitterNode="<<data.emitterNode
			<<" directionNode="<<data.directionNode
			<<" material="<<data.material
			<<" factor="<<data.factor
			<<" isOldFormat="<<data.isOldFormat

			<<" smokeNode="<<ECHO_PTR(data.smokeNode)
			<<" smoker="<<ECHO_PTR(data.smoker)
			;

		++i;
	}
}

void RigInspector::PrintTurboprop(std::ofstream & f, Turboprop * ptr)
{
	f << "\n Turboprop: ";
	if (ptr == nullptr)
	{
		f << "nullptr";
		return;
	}
	Turboprop & data = *ptr;
	LOG_ARRAY("nodep", data.nodep, 4);
	LOG_ARRAY_PTR("vpales(Ogre::SceneNode*)", data.vpales, 4);
	LOG_ARRAY("twistmap", data.twistmap, 5);

	f
		<<" is_piston="<<data.is_piston
		<<" pitch="<<data.pitch
		<<" indicated_torque="<<data.indicated_torque
		<<" max_torque="<<data.max_torque
		<<" nodes="<<ECHO_NODE(data.nodes)
		<<" nodeback="<<data.nodeback
		<<" torquenode="<<data.torquenode
		<<" torquedist="<<data.torquedist

		<<" airfoil="<<ECHO_PTR(data.airfoil) // TODO: inspect airfoil!

		<<" fullpower="<<data.fullpower
		<<" proparea="<<data.proparea
		<<" airdensity="<<data.airdensity
		<<" timer="<<data.timer
		<<" lastflip="<<data.lastflip
		<<" warmupstart="<<data.warmupstart
		<<" warmuptime="<<data.warmuptime
		<<" number="<<data.number
		<<" numblades="<<data.numblades
		<<" bladewidth="<<data.bladewidth
		<<" pitchspeed="<<data.pitchspeed
		<<" maxrevpitch="<<data.maxrevpitch
		<<" regspeed="<<data.regspeed

		<<" vspinner="<<ECHO_PTR(data.vspinner)
		<<" free_vpale="<<data.free_vpale
		<<" smokePS="<<ECHO_PTR(data.smokePS)
		<<" heathazePS="<<ECHO_PTR(data.heathazePS)
		<<" smokeNode="<<ECHO_PTR(data.smokeNode)
		<<" rotenergy="<<data.rotenergy
		<<" fixed_pitch="<<data.fixed_pitch

		<<" reverse="<<data.reverse
		<<" warmup="<<data.warmup
		<<" ignition="<<data.ignition
		<<" radius="<<data.radius
		<<" failed="<<data.failed
		<<" failedold="<<data.failedold
		<<" rpm="<<data.rpm
		<<" throtle="<<data.throtle
		<<" noderef="<<data.noderef
		<<" debug="<<data.debug
		<<" propwash="<<data.propwash
		<<" axis="<<ECHO_V3(data.axis)
		<<" heathaze="<<data.heathaze
		<<" trucknum="<<data.trucknum
		<<" mod_id="<<data.mod_id
		<<" src_id="<<data.src_id
		<<" thr_id="<<data.thr_id
		;
}

void RigInspector::PrintTurbojet(std::ofstream & f, Turbojet * ptr)
{
	f << "\nTurbojet: ";
	if (ptr == nullptr)
	{
		f << "nullptr";
		return;
	}
	Turbojet & data = *ptr;
	f
		<<" mr="<<ECHO_PTR(data.mr)
		<<" heathazePS="<<ECHO_PTR(data.heathazePS)
		<<" smokePS="<<ECHO_PTR(data.smokePS)
		<<" absnode="<<ECHO_PTR(data.absnode)
		<<" nzsnode="<<ECHO_PTR(data.nzsnode)

		<<" axis="<<ECHO_V3(data.axis)

		<<" afterburner="<<data.afterburner
		<<" failed="<<data.failed
		<<" heathaze="<<data.heathaze
		<<" ignition="<<data.ignition
		<<" reversable="<<data.reversable
		<<" reverse="<<data.reverse
		<<" warmup="<<data.warmup
		<<" afterburnthrust="<<data.afterburnthrust
		<<" area="<<data.area
		<<" exhaust_velocity="<<data.exhaust_velocity
		<<" lastflip="<<data.lastflip
		<<" maxdrythrust="<<data.maxdrythrust
		<<" propwash="<<data.propwash
		<<" radius="<<data.radius
		<<" reflen="<<data.reflen
		<<" rpm="<<data.rpm
		<<" throtle="<<data.throtle
		<<" timer="<<data.timer
		<<" warmupstart="<<data.warmupstart
		<<" warmuptime="<<data.warmuptime
		<<" ab_id="<<data.ab_id
		<<" mod_id="<<data.mod_id
		<<" nodeback="<<data.nodeback
		<<" nodefront="<<data.nodefront
		<<" noderef="<<data.noderef
		<<" number="<<data.number
		<<" src_id="<<data.src_id

		<<" thr_id="<<data.thr_id
		<<" trucknum="<<data.trucknum
		<<" nodes="<<ECHO_NODE(data.nodes)
		<<" smokeNode="<<ECHO_PTR(data.smokeNode)
		;
}

void RigInspector::InspectAeroengines(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n AeroEngine double check"
		<< "(free AeroEngine:" << rig->free_aeroengine<<")";
	for (int i = 0; i < rig->free_aeroengine; i++)
	{
		f<<"\n AeroEngine "<<i<<":";
		AeroEngine* ae = rig->aeroengines[i];
		if (rig->aeroengines[i] == nullptr)
		{
			f<<"nullptr";
		}
		else
		{
			if (typeid(ae) == typeid(Turbojet))
			{
				PrintTurbojet(f, dynamic_cast<Turbojet*>(ae));
			}
			else if (typeid(ae) == typeid(Turbojet))
			{
				PrintTurbojet(f, dynamic_cast<Turbojet*>(ae));
			}
			else
			{
				f<<" unknown type";
			}
		}
	}
}

void RigInspector::InspectScrewprops(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n ELEMENT double check"
		<< "(free ELEMENT:" << rig->free_screwprop<<")";
	for (int i = 0; i < rig->free_screwprop; i++)
	{
		f<<"\n Screwprop "<<i<<":";
		Screwprop & data = * rig->screwprops[i];
		f
			<<" splashp="<<ECHO_PTR(data.splashp)
			<<" ripplep="<<ECHO_PTR(data.ripplep)
			<<" reverse="<<data.reverse
			<<" fullpower="<<data.fullpower
			<<" rudder="<<data.rudder
			<<" throtle="<<data.throtle
			<<" nodeback="<<data.nodeback
			<<" noderef="<<data.noderef
			<<" nodeup="<<data.nodeup
			<<" trucknum="<<data.trucknum
			<<" nodes="<<ECHO_NODE(data.nodes)
			;
	}
}

void RigInspector::PrintFlexbody(std::ofstream & f, FlexBody* flexbody)
{
	f<<"\nFlexbody:";
	if (flexbody == nullptr)
	{
		f<<"nullptr";
		return;
	}
	FlexBody & data = *flexbody;

	// Nodes
	f << "\n Nodes: ";
	for (int i = 0; i < data.numnodes; ++i)
	{	
		node_t* node = (data.nodes + i);
		f << ECHO_NODE(node);
	}

	// Vertices
	f << "\n Vertices:";
	int vertex_count = static_cast<int>(data.vertex_count);
	for (int i = 0; i < vertex_count; ++i)
	{
		Ogre::Vector3 & vertex = * (data.vertices + i);
		Ogre::Vector3 & dstpos = * (data.dstpos + i);
		Ogre::Vector3 & srcnormal = * (data.srcnormals + i);
		Ogre::Vector3 & dstnormal = * (data.dstnormals + i);
		Ogre::ARGB & srccolor = * (data.srccolors + i);
		FlexBody::Locator_t & locator = * (data.locs + i);

		f
			<<" vertex="<<ECHO_V3(vertex)
			<<" dstpos="<<ECHO_V3(dstpos)
			<<" srcnormal="<<ECHO_V3(srcnormal)
			<<" dstnormal="<<ECHO_V3(dstnormal)
			<<" srccolor="<<srccolor

			<<" locator.ref="<<locator.ref
			<<" locator.nx="<<locator.nx
			<<" locator.ny="<<locator.ny
			<<" locator.nz="<<locator.nz
			<<" locator.coords="<<ECHO_V3(locator.coords)
			;
	}

	// Nodeset
	f<<"\n Nodeset: ";
	for (int i = 0; i < data.freenodeset; ++i)
	{
		FlexBody::interval_t & item = data.nodeset[i];
		f<<" from="<<item.from<<" to="<<item.to;
	}

	// Data
	f
		<<" nodes="<<ECHO_NODE(data.nodes)
		<<" numnodes="<<data.numnodes
		<<" vertex_count="<<data.vertex_count
		<<" cref="<<data.cref
		<<" cx="<<data.cx
		<<" cy="<<data.cy
		<<" coffset="<<ECHO_V3(data.coffset)
		<<" snode="<<ECHO_PTR(data.snode)

		<<" freenodeset="<<data.freenodeset

		<<" sharedcount="<<data.sharedcount
		<<" sharedpbuf="<<data.sharedpbuf.isNull()
		<<" sharednbuf="<<data.sharednbuf.isNull()
		<<" sharedcbuf="<<data.sharedcbuf.isNull()

		<<" numsubmeshbuf="<<data.numsubmeshbuf
		<<" submeshnums="<<ECHO_PTR(data.submeshnums) // TODO: traverse C array
		<<" subnodecounts="<<ECHO_PTR(data.subnodecounts) // TODO: traverse C array
		;
		LOG_ARRAY_HardwareVertexBufferSharedPtr("subpbufs", data.subpbufs, 16);
		LOG_ARRAY_HardwareVertexBufferSharedPtr("subnbufs", data.subnbufs, 16);
		LOG_ARRAY_HardwareVertexBufferSharedPtr("subcbufs", data.subcbufs, 16);

		f
		<<" enabled="<<data.enabled
		<<" cameramode="<<data.cameramode
		<<" hasshared="<<data.hasshared
		<<" hasshadows="<<data.hasshadows
		<<" hastangents="<<data.hastangents
		<<" hastexture="<<data.hastexture
		<<" hasblend="<<data.hasblend
		<<" faulty="<<data.faulty
		// TODO: msh
		;
}

void RigInspector::InspectFlexbodies(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n FlexBody double check"
		<< "(free FlexBody:" << rig->free_flexbody<<")";
	for (int i = 0; i < rig->free_flexbody; i++)
	{
		PrintFlexbody(f, rig->flexbodies[i]);
	}
}

void RigInspector::InspectDashboardLayouts(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n dashBoardLayouts double check"
		<<"(size:" << rig->dashBoardLayouts.size()<<")";
	auto end = rig->dashBoardLayouts.end();
	int i = 0;
	for(auto itor = rig->dashBoardLayouts.begin(); itor != end; ++itor)
	{

		f<<"\n dashBoardLayouts "<<i<<":"
			<<" first="<<itor->first
			<<" second="<<itor->second
			;

		++i;
	}
}

void RigInspector::InspectVideocameras(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n VideoCamera double check"
		<<"(size:" << rig->vidcams.size()<<")";
	auto end = rig->vidcams.end();
	int i = 0;
	for(auto itor = rig->vidcams.begin(); itor != end; ++itor)
	{
		VideoCamera & data = *(*itor);
		f<<"\n VideoCamera "<<i<<":"
			// public stuff
			<<" camNode="<<data.camNode
			<<" lookat="<<data.lookat
			<<" switchoff="<<data.switchoff
			<<" fov="<<data.fov
			<<" minclip="<<data.minclip
			<<" maxclip="<<data.maxclip
			<<" offset="<<ECHO_V3(data.offset)
			<<" mirrorSize="<<ECHO_V2(data.mirrorSize)
			<<" rotation="<<data.rotation // Quaternion

			<<" nz="<<data.nz
			<<" ny="<<data.ny
			<<" nref="<<data.nref
			<<" camRole="<<data.camRole
			<<" materialName="<<data.materialName
			<<" disabledTexture="<<data.disabledTexture
			<<" vidCamName="<<data.vidCamName

			// protected stuff
			<<" truck(rig_t *)="<<ECHO_PTR(data.truck)
			<<" counter="<<data.counter
			<<" mVidCam="<<ECHO_PTR(data.mVidCam)
			<<" rttTex="<<ECHO_PTR(data.rttTex)
			<<" mat="<<data.mat.isNull()
			<<" debugMode="<<data.debugMode
			<<" debugNode="<<ECHO_PTR(data.debugNode)
			<<" mr="<<ECHO_PTR(data.mr)
			<<" rwMirror="<<ECHO_PTR(data.rwMirror)
			;

		++i;
	}
}

void RigInspector::InspectAxles(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n Axle double check"
		<< "(free Axle:" << rig->free_axle<<")";
	for (int i = 0; i < rig->free_axle; i++)
	{
		Axle & data = *rig->axles[i];
		f<<"\n Axle "<<i<<":"
			<<" wheel_1="<<data.wheel_1
			<<" wheel_2="<<data.wheel_2
			<<" delta_rotation="<<data.delta_rotation
			<<" torsion_rate="<<data.torsion_rate
			<<" torsion_damp="<<data.torsion_damp
			<<" avg_speed="<<data.avg_speed
			<<" gear_ratio="<<data.gear_ratio
			<<" axle_group="<<data.axle_group

			<<" which_diff="<<data.which_diff
			<<" free_diff="<<data.free_diff
			<<" current_callback="<<data.current_callback
			;
		LOG_ARRAY("available_diff_method", data.available_diff_method, data.available_diff_method.size());
	}
}

#if 0 // TEMPLATE

void RigInspector::InspectARRAY(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n ELEMENT double check"
		<< "(free ELEMENT:" << rig->free_ELEMENT<<")";
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
}

void RigInspector::InspectVECTOR(std::ofstream & f, Beam* rig)
{
	f << "\n\n***********************\n TYPE double check"
		<<"(size:" << rig->VECTOR.size()<<")";
	auto end = rig->VECTOR.end();
	int i = 0;
	for(auto itor = rig->VECTOR.begin(); itor != end; ++itor)
	{
		TYPE & data = *itor;
		f<<"\n TYPE "<<i<<":"
			// <<" FOOOOOOO="<<data.BARRRRRRR
			// <<" FOOOOOOO="<<ECHO_PTR(data.PTRVARNAME)
			// <<" FOOOOOOO="<<ECHO_NODE(data.NODEPTR)
			// <<" FOOOOOOO="<<ECHO_BEAM(data.BEAMPTR)
			// <<" FOOOOOOO="<<ECHO_V3(data.OGREVECTOR3)
			// <<" FOOOOOOO="<<ECHO_QUAT(data.OGREQUATERNION)
			//
			// LOG_ARRAY("", , 10);
			;

		++i;
	}
}

#endif