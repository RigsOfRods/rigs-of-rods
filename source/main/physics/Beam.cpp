/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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
#include "Beam.h"

#include "AirBrake.h"
#include "Airfoil.h"
#include "ApproxMath.h"
#include "AutoPilot.h"
#include "BeamData.h"
#include "BeamEngine.h"
#include "BeamFactory.h"
#include "BeamStats.h"
#include "Buoyance.h"
#include "CameraManager.h"
#include "CmdKeyInertia.h"
#include "Collisions.h"
#include "DashBoardManager.h"
#include "Differentials.h"
#include "DustManager.h"
#include "ErrorUtils.h"
#include "FlexAirfoil.h"
#include "FlexBody.h"
#include "FlexMesh.h"
#include "FlexMeshWheel.h"
#include "FlexObj.h"
#include "IHeightFinder.h"
#include "InputEngine.h"
#include "Language.h"
#include "MeshObject.h"
#include "MovableText.h"
#include "Network.h"
#include "Ogre.h"
#include "PointColDetector.h"
#include "PositionStorage.h"
#include "Replay.h"
#include "RoRFrameListener.h"
#include "ScrewProp.h"
#include "Scripting.h"
#include "Settings.h"
#include "Skidmark.h"
#include "SlideNode.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "ThreadPool.h"
#include "TurboJet.h"
#include "TurboProp.h"
#include "Water.h"

// some gcc fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif //OGRE_PLATFORM_LINUX

using namespace Ogre;

Beam::Beam(int tnum, Ogre::Vector3 pos, Ogre::Quaternion rot, const char* fname, bool networked /* = false  */, bool networking /* = false  */, collision_box_t *spawnbox /* = NULL  */, bool ismachine/* =false  */, int flareMode /* = 0  */, const std::vector<Ogre::String> *truckconfig /* = 0  */, Skin *skin /* = 0  */, bool freeposition /* = false */) :
	  deleting(false)
	, GUIFeaturesChanged(false)
	, aileron(0)
	, avichatter_timer(11.0f) // some pseudo random number,  doesn't matter
	, beacon(false)
	, beamsVisible(true)
	, blinkingtype(BLINK_NONE)
	, blinktreshpassed(false)
	, brake(0.0)
	, cabFadeMode(0)
	, cabFadeTime(0.3)
	, cabFadeTimer(0)
	, cameranodeacc(Vector3::ZERO)
	, cameranodecount(0)
	, canwork(true)
	, cparticle_mode(false)
	, currentScale(1)
	, currentcamera(-1) // -1 = external
	, dash(nullptr)
	, detailLevel(0)
	, disableDrag(false)
	, disableTruckTruckCollisions(false)
	, disableTruckTruckSelfCollisions(false)
	, elevator(0)
	, flap(0)
	, floating_origin_enable(true)
	, fusedrag(Vector3::ZERO)
	, hydroaileroncommand(0)
	, hydroaileronstate(0)
	, hydrodircommand(0)
	, hydrodirstate(0)
	, hydrodirwheeldisplay(0.0)
	, hydroelevatorcommand(0)
	, hydroelevatorstate(0)
	, hydroruddercommand(0)
	, hydrorudderstate(0)
	, iPosition(pos)
	, increased_accuracy(false)
	, interPointCD()
	, intraPointCD()
	, isInside(false)
	, last_net_time(0)
	, lastlastposition(pos)
	, lastposition(pos)
	, leftMirrorAngle(0.52)
	, lights(1)
	, lockSkeletonchange(false)
	, locked(0)
	, lockedold(0)
	, mTimeUntilNextToggle(0)
	, meshesVisible(true)
	, minCameraRadius(0)
	, mousemoveforce(0.0f)
	, mousenode(-1)
	, mousepos(Vector3::ZERO)
	, netBrakeLight(false)
	, netLabelNode(0)
	, netMT(0)
	, netReverseLight(false)
	, networkAuthlevel(0)
	, networkUsername("")
	, oldreplaypos(-1)
	, parkingbrake(0)
	, posStorage(0)
	, position(pos)
	, previousGear(0)
	, refpressure(50.0)
	, replay(0)
	, replayPrecision(0)
	, replayTimer(0)
	, replaylen(10000)
	, replaymode(false)
	, replaypos(0)
	, requires_wheel_contact(false)
	, reset_requested(0)
	, reverselight(false)
	, rightMirrorAngle(-0.52)
	, rudder(0)
	, simpleSkeletonInitiated(false)
	, simpleSkeletonManualObject(0)
	, simulated(false)
	, skeleton(0)
	, sleepcount(0)
	, smokeNode(NULL)
	, smoker(NULL)
	, stabcommand(0)
	, stabratio(0.0)
	, stabsleep(0.0)
	, tdt(0.1)
	, thread_index(0)
	, thread_number(0)
	, thread_task(THREAD_BEAMFORCESEULER)
	, totalmass(0)
	, tsteps(100)
	, ttdt(0.1)
	, watercontact(false)
	, watercontactold(false)
{
	pthread_cond_init(&flexable_task_count_cv, NULL);
	pthread_mutex_init(&flexable_task_count_mutex, NULL);
	for (int task=0; task < THREAD_MAX; task++)
	{
		task_count[task] = 0;
		pthread_cond_init(&task_count_cv[task], NULL);
		pthread_mutex_init(&task_count_mutex[task], NULL);
		pthread_mutex_init(&task_index_mutex[task], NULL);
	}
	pthread_mutex_init(&itc_node_access_mutex, NULL);

	mCamera = gEnv->mainCamera;
	airbrakeval = 0;
	alb_minspeed = 0.0f;
	alb_mode = 0;
	alb_notoggle = false;
	alb_notoggle = false;
	alb_present = false;
	alb_pulse = 1;
	alb_pulse_state = false;
	alb_ratio = 0.0f;
	animTimer = 0.0f;
	antilockbrake = 0;

	cabMesh = NULL;
	cablight = 0;
	cablightNode = 0;

	cc_mode = false;
	cc_can_brake = false;
	cc_target_rpm = 0.0f;
	cc_target_speed = 0.0f;
	cc_target_speed_lower_limit = 0.0f;

	collisionRelevant = false;

	debugVisuals = 0;

	driveable = NOT_DRIVEABLE;
	driverSeat = 0;

	enable_wheel2 = true; // since 0.38 enabled wheels2 by default
	engine = 0;

	flaresMode = flaresMode;

	freePositioned = freeposition;
	free_aeroengine = 0;
	free_airbrake = 0;
	free_axle = 0;
	free_beam = 0;
	free_buoycab = 0;
	free_cab = 0;
	free_camerarail = 0;
	free_collcab = 0;
	free_contacter = 0;
	free_cparticle = 0;
	free_flare = 0;
	free_flexbody = 0;
	free_hydro = 0;
	free_node = 0;
	free_pressure_beam = 0;
	free_prop = 0;
	free_rigidifier = 0;
	free_rotator = 0;
	free_screwprop = 0;
	free_shock = 0;
	free_soundsource = 0;
	free_sub = 0;
	free_texcoord = 0;
	free_wheel = 0;
	free_wing = 0;

	disableTruckTruckCollisions = BSETTING("DisableCollisions", false);
	disableTruckTruckSelfCollisions = BSETTING("DisableSelfCollisions", false);

	heathaze = !disable_smoke && BSETTING("HeatHaze", false);
	hideInChooser = false;

	origin = Vector3::ZERO;

	previousCrank = 0.0f;

	sl_enabled = false;
	sl_speed_limit = 0.0f;

	state = SLEEPING;

	tc_fade = 0.0f;
	tc_mode = 0;
	tc_present = false;
	tc_pulse = 1;
	tc_pulse_state = false;
	tc_ratio = 0.0f;
	tc_wheelslip = 0.0f;
	tcalb_timer = 0.0f;

	tractioncontrol = 0;
	trucknum = tnum;

	usedSkin = skin;

#ifdef USE_MYGUI
	dash = new DashBoardManager();
#endif // USE_MYGUI

#ifdef FEAT_TIMING
	// this enables beam engine timing statistics
	statistics = BES.getClient(tnum, BES_CORE);
	statistics_gfx = BES.getClient(tnum, BES_GFX);
#endif

	this->networking = networking;

	for (int i=0; i<MAX_SUBMESHES; i++)
	{
		subisback[i] = 0;
	}	

	LOG("BEAM: loading new truck: " + String(fname));

	if (ismachine)
	{
		driveable = MACHINE;
	}

	// copy truck config
	if (truckconfig && truckconfig->size())
	{
		for (std::vector<String>::const_iterator it = truckconfig->begin(); it != truckconfig->end(); ++it)
		{
			this->truckconfig.push_back(*it);
		}
	}

	materialFunctionMapper = new MaterialFunctionMapper();
	cmdInertia   = new CmdKeyInertia();
	hydroInertia = new CmdKeyInertia();
	rotaInertia  = new CmdKeyInertia();

	submesh_ground_model = gEnv->collisions->defaultgm;

	realtruckfilename = String(fname);
	sprintf(truckname, "t%i", tnum);
	
	strcpy(uniquetruckid,"-1");

	simpleSkeletonNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
	deletion_sceneNodes.push_back(simpleSkeletonNode);
	
	beamsRoot=gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
	deletion_sceneNodes.push_back(netLabelNode);

	// skidmark stuff
	useSkidmarks = BSETTING("Skidmarks", false);

	// you could disable the collision code here:
	if (!disableTruckTruckCollisions)
	{
		interPointCD.emplace_back(new PointColDetector());

		if (gEnv->threadPool)
		{
			for (int i=1; i<gEnv->threadPool->getSize(); i++)
				interPointCD.emplace_back(new PointColDetector());
		}
	}

	if (!disableTruckTruckSelfCollisions)
	{
		intraPointCD.emplace_back(new PointColDetector());

		if (gEnv->threadPool)
		{
			for (int i=1; i<gEnv->threadPool->getSize(); i++)
				intraPointCD.emplace_back(new PointColDetector());
		}
	}

	dustp   = DustManager::getSingleton().getDustPool("dust");
	dripp   = DustManager::getSingleton().getDustPool("dripp");
	sparksp = DustManager::getSingleton().getDustPool("sparks");
	clumpp  = DustManager::getSingleton().getDustPool("clump");
	splashp = DustManager::getSingleton().getDustPool("splash");
	ripplep = DustManager::getSingleton().getDustPool("ripple");
	
	if (networked || networking)
	{
		enable_wheel2 = false;
	}

	cparticle_enabled = BSETTING("Particles", true);
	if (strnlen(fname, 200) > 0)
	{
		if (loadTruck2(String(fname), beamsRoot, pos, rot, spawnbox))
		{
	   		return;
		}
	}

	// setup sounds properly
	changedCamera();

	// setup replay mode
	bool enablereplay = BSETTING("Replay mode", false);

	if (enablereplay && !networked && !networking)
	{
		replaylen = ISETTING("Replay length", 10000);
		replay = new Replay(this, replaylen);

		int steps = ISETTING("Replay Steps per second", 240);

		if (steps <= 0)
			replayPrecision = 0.0f;
		else
			replayPrecision = 1.0f / ((float)steps);
	}

	// add storage
	bool enablePosStor = BSETTING("Position Storage", false);
	if (enablePosStor)
	{
		posStorage = new PositionStorage(free_node, 10);
	}

	//search first_wheel_node
	first_wheel_node=free_node;
	for (int i=0; i<free_node; i++)
	{
		if (nodes[i].iswheel)
		{
			first_wheel_node=i;
			break;
		}
	}

	// network buffer layout (without oob_t):
	//
	//  - 3 floats (x,y,z) for the reference node 0
	//  - free_node - 1 times 3 short ints (compressed position info)
	//  - free_wheel times a float for the wheel rotation
	//
	nodebuffersize = sizeof(float) * 3 + (first_wheel_node-1) * sizeof(short int) * 3;
	netbuffersize  = nodebuffersize + free_wheel * sizeof(float);
	updateVisual();
	// stop lights
	lightsToggle();

	updateFlares(0);
	updateProps();
	if (engine)
	{
		engine->offstart();
	}
	// pressurize tires
	addPressure(0.0);

	checkBeamMaterial();

	// start network stuff
	if (networked)
	{
		state = NETWORKED;
		// malloc memory
		oob1=(oob_t*)malloc(sizeof(oob_t));
		oob2=(oob_t*)malloc(sizeof(oob_t));
		oob3=(oob_t*)malloc(sizeof(oob_t));
		netb1=(char*)malloc(netbuffersize);
		netb2=(char*)malloc(netbuffersize);
		netb3=(char*)malloc(netbuffersize);
		nettimer = new Timer();
		net_toffset = 0;
		netcounter = 0;
		// init mutex
		pthread_mutex_init(&net_mutex, NULL);
		if (engine)
		{
			engine->start();
		}
	}

	if (networking)
	{
		sendStreamSetup();
	}
}

Beam::~Beam()
{
	// TODO: IMPROVE below: delete/destroy prop entities, etc

	deleting = true;
	state = DELETED;

	// hide everything, prevents deleting stuff while drawing
	this->setBeamVisibility(false);
	this->setMeshVisibility(false);

	// delete all classes we might have constructed
#ifdef USE_MYGUI
	if (dash) delete dash; dash=0;
#endif // USE_MYGUI

	// stop all the Sounds
#ifdef USE_OPENAL
	for (int i=SS_TRIG_NONE+1; i<SS_MAX_TRIG; i++)
	{
		SoundScriptManager::getSingleton().trigStop(this->trucknum, i);
	}
#endif // USE_OPENAL

	// destruct and remove every tiny bit of stuff we created :-|
	if (nettimer) delete nettimer; nettimer=0;
	if (engine) delete engine; engine=0;
	if (buoyance) delete buoyance; buoyance=0;
	if (autopilot) delete autopilot;
	if (fuseAirfoil) delete fuseAirfoil;
	if (cabMesh) delete cabMesh;
	if (materialFunctionMapper) delete materialFunctionMapper;
	if (replay) delete replay;

	std::vector<SceneNode*> deletion_sceneNodes;
	std::vector<Entity *> deletion_Entities;

	// remove all scene nodes
	if (deletion_sceneNodes.size() > 0)
	{
		int size = (int)deletion_sceneNodes.size();
		for (int i=0;i<size; i++)
		{
			if (!deletion_sceneNodes[i]) continue;
			deletion_sceneNodes[i]->removeAndDestroyAllChildren();
			gEnv->sceneManager->destroySceneNode(deletion_sceneNodes[i]);

			deletion_sceneNodes[i]=0;
		}
	}
	// delete skidmarks as well?!

	// delete wings
	for (int i=0; i<free_wing;i++)
	{
		// flexAirfoil, airfoil
		if (wings[i].fa) delete wings[i].fa; wings[i].fa=0;
		if (wings[i].cnode)
		{
			wings[i].cnode->removeAndDestroyAllChildren();
			gEnv->sceneManager->destroySceneNode(wings[i].cnode);
		}
	}

	// delete aeroengines
	for (int i=0; i<free_aeroengine;i++)
	{
		if (aeroengines[i]) delete aeroengines[i];
	}

	// delete screwprops
	for (int i=0; i<free_screwprop;i++)
	{
		if (screwprops[i]) delete screwprops[i];
	}

	// delete airbrakes
	for (int i=0; i<free_airbrake;i++)
	{
		if (airbrakes[i]) delete airbrakes[i];
	}

	// delete flexbodies
	for (int i=0; i<free_flexbody;i++)
	{
		if (flexbodies[i]) delete flexbodies[i];
	}


	// delete meshwheels
	for (int i=0; i<free_wheel;i++)
	{

		//if (vwheels[i].fm) delete vwheels[i].fm; // CRASH!
		if (vwheels[i].cnode)
		{
			vwheels[i].cnode->removeAndDestroyAllChildren();
			gEnv->sceneManager->destroySceneNode(vwheels[i].cnode);
		}
	}

	// delete props
	for (int i=0; i<free_prop;i++)
	{
		if (props[i].bbsnode[0])
		{
			props[i].bbsnode[0]->removeAndDestroyAllChildren();
			gEnv->sceneManager->destroySceneNode(props[i].bbsnode[0]);
		}
		if (props[i].bbsnode[1])
		{
			props[i].bbsnode[1]->removeAndDestroyAllChildren();
			gEnv->sceneManager->destroySceneNode(props[i].bbsnode[1]);
		}
		if (props[i].bbsnode[2])
		{
			props[i].bbsnode[2]->removeAndDestroyAllChildren();
			gEnv->sceneManager->destroySceneNode(props[i].bbsnode[2]);
		}
		if (props[i].bbsnode[3])
		{
			props[i].bbsnode[3]->removeAndDestroyAllChildren();
			gEnv->sceneManager->destroySceneNode(props[i].bbsnode[3]);
		}
		if (props[i].snode)
		{
			props[i].snode->removeAndDestroyAllChildren();
			gEnv->sceneManager->destroySceneNode(props[i].snode);
		}
		if (props[i].wheel)
		{
			props[i].wheel->removeAndDestroyAllChildren();
			gEnv->sceneManager->destroySceneNode(props[i].wheel);
		}
		if (props[i].light[0]) gEnv->sceneManager->destroyLight(props[i].light[0]);
		if (props[i].light[1]) gEnv->sceneManager->destroyLight(props[i].light[1]);
		if (props[i].light[2]) gEnv->sceneManager->destroyLight(props[i].light[2]);
		if (props[i].light[3]) gEnv->sceneManager->destroyLight(props[i].light[3]);
	}

	// delete flares
	for (int i=0; i<free_flare; i++)
	{
		if (flares[i].snode)
		{
			flares[i].snode->removeAndDestroyAllChildren();
			gEnv->sceneManager->destroySceneNode(flares[i].snode);
		}
		if (flares[i].light) gEnv->sceneManager->destroyLight(flares[i].light);

	}

	// delete exhausts
	for (std::vector < exhaust_t >::iterator it=exhausts.begin(); it!=exhausts.end(); it++)
	{
		if (it->smokeNode)
		{
			it->smokeNode->removeAndDestroyAllChildren();
			gEnv->sceneManager->destroySceneNode(it->smokeNode);
		}
		if (it->smoker)
		{
			it->smoker->removeAllAffectors();
			it->smoker->removeAllEmitters();
			gEnv->sceneManager->destroyParticleSystem(it->smoker);
		}
	}

	// delete cparticles
	for (int i=0; i<free_cparticle; i++)
	{
		if (cparticles[free_cparticle].snode)
		{
			cparticles[free_cparticle].snode->removeAndDestroyAllChildren();
			gEnv->sceneManager->destroySceneNode(cparticles[free_cparticle].snode);
		}
		if (cparticles[free_cparticle].psys)
		{
			cparticles[free_cparticle].psys->removeAllAffectors();
			cparticles[free_cparticle].psys->removeAllEmitters();
			gEnv->sceneManager->destroyParticleSystem(cparticles[free_cparticle].psys);
		}

	}

	// delete beams
	for (int i=0; i<free_beam; i++)
	{
		if (beams[i].mEntity)    beams[i].mEntity->setVisible(false);
		if (beams[i].mSceneNode)
		{
			beams[i].mSceneNode->removeAndDestroyAllChildren();
			//gEnv->ogreSceneManager->destroySceneNode(beams[i].mSceneNode);
		}
	}

	// delete Rails
	for (std::vector< RailGroup* >::iterator it = mRailGroups.begin(); it != mRailGroups.end(); it++)
	{
		// signal to the Rail that
		(*it)->cleanUp();
		delete (*it);
	}

	if (netMT)
	{
		netMT->setVisible(false);
		delete netMT;
		netMT = 0;
	}

	if (net_mutex)
	{
		pthread_mutex_destroy(&net_mutex);
	}

	pthread_cond_destroy(&flexable_task_count_cv);
	pthread_mutex_destroy(&flexable_task_count_mutex);
	for (int task=0; task < THREAD_MAX; task++)
	{
		pthread_cond_destroy(&task_count_cv[task]);
		pthread_mutex_destroy(&task_count_mutex[task]);
		pthread_mutex_destroy(&task_index_mutex[task]);
	}
	pthread_mutex_destroy(&itc_node_access_mutex);
}

// This method scales trucks. Stresses should *NOT* be scaled, they describe
// the material type and they do not depend on length or scale.
void Beam::scaleTruck(float value)
{
	BES_GFX_START(BES_GFX_ScaleTruck);

	if (value<0) return;
	currentScale *= value;
	// scale beams
	for (int i=0;i<free_beam;i++)
	{
		//beams[i].k *= value;
		beams[i].d *= value;
		beams[i].L *= value;
		beams[i].refL *= value;
		beams[i].Lhydro *= value;
		beams[i].hydroRatio *= value;

		beams[i].diameter *= value;
		beams[i].lastforce *= value;
	}
	// scale nodes
	Vector3 refpos = nodes[0].AbsPosition;
	Vector3 relpos = nodes[0].RelPosition;
	Vector3 smopos = nodes[0].smoothpos;
	for (int i=1;i<free_node;i++)
	{
		nodes[i].iPosition = refpos + (nodes[i].iPosition-refpos) * value;
		nodes[i].AbsPosition = refpos + (nodes[i].AbsPosition-refpos) * value;
		nodes[i].RelPosition = relpos + (nodes[i].RelPosition-relpos) * value;
		nodes[i].smoothpos = smopos + (nodes[i].smoothpos-smopos) * value;
		nodes[i].Velocity *= value;
		nodes[i].Forces *= value;
		nodes[i].lockedPosition *= value;
		nodes[i].lockedVelocity *= value;
		nodes[i].lockedForces *= value;
		nodes[i].mass *= value;
	}
	updateSlideNodePositions();

	// props and stuff
	// TOFIX: care about prop positions as well!
	for (int i=0;i<free_prop;i++)
	{
		if (props[i].snode) props[i].snode->scale(value, value, value);
		if (props[i].wheel) props[i].wheel->scale(value, value, value);
		if (props[i].wheel) props[i].wheelpos = relpos + (props[i].wheelpos-relpos) * value;
		if (props[i].bbsnode[0]) props[i].bbsnode[0]->scale(value, value, value);
		if (props[i].bbsnode[1]) props[i].bbsnode[1]->scale(value, value, value);
		if (props[i].bbsnode[2]) props[i].bbsnode[2]->scale(value, value, value);
		if (props[i].bbsnode[3]) props[i].bbsnode[3]->scale(value, value, value);
	}
	// tell the cabmesh that resizing is ok, and they dont need to break ;)
	if (cabMesh) cabMesh->scale(value);
	// update engine values
	if (engine)
	{
		//engine->maxRPM *= value;
		//engine->iddleRPM *= value;
		engine->engineTorque *= value;
		//engine->stallRPM *= value;
		//engine->brakingTorque *= value;
	}
	// todo: scale flexbody
	for (int i=0;i<free_flexbody;i++)
	{
		flexbodies[i]->getSceneNode()->scale(value, value, value);
	}
	// todo: fix meshwheels
	//for (int i=0;i<free_wheel;i++)
	//{
		//if (vwheels[i].cnode) vwheels[i].cnode->scale(value, value, value);
		//if (vwheels[i].fm && vwheels[i].cnode) vwheels[i].cnode->scale(value, value, value);
	//}
	BES_GFX_STOP(BES_GFX_ScaleTruck);

}

void Beam::initSimpleSkeleton()
{
	// create
	simpleSkeletonManualObject =  gEnv->sceneManager->createManualObject();

	simpleSkeletonManualObject->estimateIndexCount(free_beam*2);
	simpleSkeletonManualObject->setCastShadows(false);
	simpleSkeletonManualObject->setDynamic(true);
	simpleSkeletonManualObject->setRenderingDistance(300);
	for (int i=0; i < free_beam; i++)
	{
		simpleSkeletonManualObject->begin("mat-beam-0", RenderOperation::OT_LINE_LIST);
		simpleSkeletonManualObject->position(beams[i].p1->smoothpos);
		simpleSkeletonManualObject->position(beams[i].p2->smoothpos);
		simpleSkeletonManualObject->end();
	}
	simpleSkeletonNode->attachObject(simpleSkeletonManualObject);
	simpleSkeletonNode->setVisible(false);
	simpleSkeletonInitiated=true;
}

void Beam::updateSimpleSkeleton()
{
	BES_GFX_START(BES_GFX_UpdateSkeleton);

	if (!simpleSkeletonInitiated)
		initSimpleSkeleton();
	// just update
	for (int i=0; i < (int)simpleSkeletonManualObject->getNumSections(); i++)
	{
		if (i >= free_beam)
			break;

		int scale=(int)(beams[i].scale * 100);
		if (scale>100) scale=100;
		if (scale<-100) scale=-100;
		char bname[256];
		sprintf(bname, "mat-beam-%d", scale);

		simpleSkeletonManualObject->setMaterialName(i, bname);
		simpleSkeletonManualObject->beginUpdate(i);
		simpleSkeletonManualObject->position(beams[i].p1->smoothpos);
		// remove broken beams
		if (beams[i].broken || beams[i].disabled)
			simpleSkeletonManualObject->position(beams[i].p1->smoothpos);
		else
			simpleSkeletonManualObject->position(beams[i].p2->smoothpos);
		simpleSkeletonManualObject->end();
	}

	BES_GFX_STOP(BES_GFX_UpdateSkeleton);
}

void Beam::moveOrigin(Vector3 offset)
{
	origin += offset;
	for (int i=0; i<free_node; i++)
	{
		nodes[i].RelPosition -= offset;
	}
}

void Beam::changeOrigin(Vector3 newOrigin)
{
	moveOrigin(newOrigin - origin);
}

Vector3 Beam::getPosition()
{
	return position; //the position is already in absolute position
}

node_t *Beam::addNode(Vector3 pos)
{
	init_node(free_node, pos.x, pos.y, pos.z, NODE_NORMAL, 100, 0, 0, free_node);
	node_t *n = &nodes[free_node];

	// we must map the actual poitition back to init position
	n->iPosition = nodes[0].iPosition + (pos - nodes[0].AbsPosition);;
	free_node++;
	return n;
}

beam_t *Beam::addBeam(int id1, int id2)
{
	int type = BEAM_NORMAL;

	if (id1 < 0 || id1 >= free_node || id2 < 0 || id2 >= free_node)
	{
		LOG("Error: unknown node number in beams section ("
			+TOSTRING(id1)+","+TOSTRING(id2)+")");
		exit(3);
	}
	//skip if a beam already exists
	LOG(TOSTRING(nodes[id1].AbsPosition)+" -> "+TOSTRING(nodes[id2].AbsPosition));

	for (int i=0; i<free_beam; i++)
	{
		if ((beams[i].p1==&nodes[id1] && beams[i].p2==&nodes[id2]) || (beams[i].p1==&nodes[id2] && beams[i].p2==&nodes[id1]))
		{
			LOG("Skipping duplicate beams: from node "+TOSTRING(id1)+" to node "+TOSTRING(id2));
			return NULL;
		}
	}

	int pos=add_beam(beamsRoot, &nodes[id1], &nodes[id2], \
			type, default_break * default_break_scale, default_spring * default_spring_scale, \
			default_damp * default_damp_scale, detacher_group_state,-1, -1, -1, 1, \
			default_beam_diameter);

	beams[pos].type=BEAM_NORMAL;
	return &beams[pos];
}

void Beam::checkBeamMaterial()
{
	BES_GFX_START(BES_GFX_checkBeamMaterial);
	if (MaterialManager::getSingleton().resourceExists("mat-beam-0"))
		return;
	int i = 0;
	char bname[256];
	for (i=-100;i<=100;i++)
	{
		//register a material for skeleton view
		sprintf(bname, "mat-beam-%d", i);
		MaterialPtr mat=(MaterialPtr)(MaterialManager::getSingleton().create(bname, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME));
		float f = fabs(((float)i)/100);
		if (i<=0)
			mat->getTechnique(0)->getPass(0)->createTextureUnitState()->setColourOperationEx(LBX_MODULATE, LBS_MANUAL, LBS_CURRENT, ColourValue(0.2f, 2.0f*(1.0f-f), f*2.0f, 0.8f));
		else
			mat->getTechnique(0)->getPass(0)->createTextureUnitState()->setColourOperationEx(LBX_MODULATE, LBS_MANUAL, LBS_CURRENT, ColourValue(f*2.0f, 2.0f*(1.0f-f), 0.2f, 0.8f));
		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(TFO_ANISOTROPIC);
		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
		mat->setLightingEnabled(false);
		mat->setReceiveShadows(false);
	}
	BES_GFX_STOP(BES_GFX_checkBeamMaterial);
}

void Beam::activate()
{
	if (state < NETWORKED)
	{
		state = ACTIVATED;
	}
}

void Beam::desactivate()
{
	if (state < NETWORKED)
	{
		state = DESACTIVATED;
		sleepcount = 0;
	}
}

//called by the network thread
void Beam::pushNetwork(char* data, int size)
{
	BES_GFX_START(BES_GFX_pushNetwork);
	if (!oob3) return;

	// check if the size of the data matches to what we expected
	if ((unsigned int)size == (netbuffersize + sizeof(oob_t)))
	{
		// we walk through the incoming data and separate it a bit
		char *ptr = data;

		// put the oob_t in front, describes truck basics, engine state, flares, etc
		memcpy((char*)oob3,  ptr, sizeof(oob_t));
		ptr += sizeof(oob_t);

		// then copy the node data
		memcpy((char*)netb3, ptr, nodebuffersize);
		ptr += nodebuffersize;

		// then take care of the wheel speeds
		for (int i = 0; i < free_wheel; i++)
		{
			float wspeed = *(float*)(ptr);
			wheels[i].rp3 = wspeed;

			ptr += sizeof(float);
		}
	} else
	{
		// TODO: show the user the problem in the GUI
		LOG("WRONG network size: we expected " + TOSTRING(netbuffersize+sizeof(oob_t)) + " but got " + TOSTRING(size) + " for vehicle " + String(truckname));
		state = NETWORKED_INVALID;
		return;
	}
	//okay, the big switch
	MUTEX_LOCK(&net_mutex);

	// and the buffer switching to have linear smoothing
	oob_t *ot;
	ot   = oob1;
	oob1 = oob2;
	oob2 = oob3;
	oob3 = ot;

	char *ft;
	ft    = netb1;
	netb1 = netb2;
	netb2 = netb3;
	netb3 = ft;

	for (int i =0 ; i < free_wheel; i++)
	{
		float rp;
		rp            = wheels[i].rp1;
		wheels[i].rp1 = wheels[i].rp2;
		wheels[i].rp2 = wheels[i].rp3;
		wheels[i].rp3 = rp;
	}
	netcounter++;
	MUTEX_UNLOCK(&net_mutex);
	BES_GFX_STOP(BES_GFX_pushNetwork);
}

void Beam::calcNetwork()
{
	BES_GFX_START(BES_GFX_calcNetwork);
	Vector3 apos=Vector3::ZERO;
	if (netcounter<4) return;
	//we must update Nodes positions from available network informations
	//we must lock as long as we use oob1, oob2, netb1, netb2
	MUTEX_LOCK(&net_mutex);
	int tnow=nettimer->getMilliseconds();
	//adjust offset to match remote time
	int rnow=tnow+net_toffset;
	//if we receive older data from the future, we must correct the offset
	if (oob1->time>rnow) {net_toffset=oob1->time-tnow; rnow=tnow+net_toffset;}
	//if we receive last data from the past, we must correct the offset
	if (oob2->time<rnow) {net_toffset=oob2->time-tnow; rnow=tnow+net_toffset;}
	float tratio=(float)(rnow-oob1->time)/(float)(oob2->time-oob1->time);
	//LOG(" network time diff: "+ TOSTRING(net_toffset));
	Vector3 p1ref = Vector3::ZERO;
	Vector3 p2ref = Vector3::ZERO;
	short *sp1 = (short*)(netb1 + sizeof(float) * 3);
	short *sp2 = (short*)(netb2 + sizeof(float) * 3);

	Vector3 p1 = Vector3::ZERO;
	Vector3 p2 = Vector3::ZERO;
	for (int i = 0; i < first_wheel_node; i++)
	{
		//linear interpolation
		if (i == 0)
		{
			// first node is uncompressed
			p1.x  = ((float*)netb1)[0];
			p1.y  = ((float*)netb1)[1];
			p1.z  = ((float*)netb1)[2];
			p1ref = p1;
			p2.x  = ((float*)netb2)[0];
			p2.y  = ((float*)netb2)[1];
			p2.z  = ((float*)netb2)[2];
			p2ref = p2;
		}
		else
		{
			// all other nodes are compressed:
			// short int compared to previous node
			p1.x = (float)(sp1[(i - 1) *3 + 0]) / 300.0f;
			p1.y = (float)(sp1[(i - 1) *3 + 1]) / 300.0f;
			p1.z = (float)(sp1[(i - 1) *3 + 2]) / 300.0f;
			p1   = p1 + p1ref;

			p2.x = (float)(sp2[(i - 1) *3 + 0]) / 300.0f;
			p2.y = (float)(sp2[(i - 1) *3 + 1]) / 300.0f;
			p2.z = (float)(sp2[(i - 1) *3 + 2]) / 300.0f;
			p2   = p2 + p2ref;
		}
		nodes[i].AbsPosition  = p1 + tratio * (p2 - p1);
		nodes[i].smoothpos    = nodes[i].AbsPosition;
		nodes[i].RelPosition  = nodes[i].AbsPosition - origin;

		// calculate the average beside ...
		apos += nodes[i].AbsPosition;
	}
	// set average position
	position = apos / first_wheel_node;

	// take care of the wheels
	for (int i=0; i<free_wheel; i++)
	{
		float rp=wheels[i].rp1+tratio*(wheels[i].rp2-wheels[i].rp1);
		//compute ideal positions
		Vector3 axis=wheels[i].refnode1->RelPosition-wheels[i].refnode0->RelPosition;
		axis.normalise();
		Plane pplan=Plane(axis, wheels[i].refnode0->AbsPosition);
		Vector3 ortho=-pplan.projectVector(wheels[i].near_attach->AbsPosition)-wheels[i].refnode0->AbsPosition;
		Vector3 ray=ortho.crossProduct(axis);
		ray.normalise();
		ray=ray*wheels[i].radius;
		float drp=2.0*3.14159/(wheels[i].nbnodes/2);
		for (int j=0; j<wheels[i].nbnodes/2; j++)
		{
			Vector3 uray=Quaternion(Radian(rp-drp*j), axis)*ray;
			wheels[i].nodes[j*2]->AbsPosition=wheels[i].refnode0->AbsPosition+uray;
			wheels[i].nodes[j*2]->smoothpos=wheels[i].nodes[j*2]->AbsPosition;
			wheels[i].nodes[j*2]->RelPosition=wheels[i].nodes[j*2]->AbsPosition-origin;

			wheels[i].nodes[j*2+1]->AbsPosition=wheels[i].refnode1->AbsPosition+uray;
			wheels[i].nodes[j*2+1]->smoothpos=wheels[i].nodes[j*2+1]->AbsPosition;
			wheels[i].nodes[j*2+1]->RelPosition=wheels[i].nodes[j*2+1]->AbsPosition-origin;
		}
	}
	//give some slack to the mutex
	float engspeed  = oob1->engine_speed+tratio*(oob2->engine_speed-oob1->engine_speed);
	float engforce  = oob1->engine_force+tratio*(oob2->engine_force-oob1->engine_force);
	float engclutch = oob1->engine_clutch+tratio*(oob2->engine_clutch-oob1->engine_clutch);
	float netwspeed = oob1->wheelspeed+tratio*(oob2->wheelspeed-oob1->wheelspeed);
	float netbrake  = oob1->brake+tratio*(oob2->brake-oob1->brake);

	hydrodirwheeldisplay = oob1->hydrodirstate;
	WheelSpeed           = netwspeed;

	int gear = oob1->engine_gear;
	unsigned int flagmask = oob1->flagmask;

	MUTEX_UNLOCK(&net_mutex);
#ifdef USE_OPENAL
	if (engine)
	{
		SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_ENGINE, engspeed);
	}
	if (free_aeroengine > 0)
	{
		SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_AEROENGINE1, engspeed);
		SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_AEROENGINE2, engspeed);
		SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_AEROENGINE3, engspeed);
		SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_AEROENGINE4, engspeed);
	}
#endif //OPENAL

	brake = netbrake;

	if (engine)
	{
		int automode = -1;
		if ((flagmask&NETMASK_ENGINE_MODE_AUTOMATIC)!=0)          automode = BeamEngine::AUTOMATIC;
		else if ((flagmask&NETMASK_ENGINE_MODE_SEMIAUTO)!=0)      automode = BeamEngine::SEMIAUTO;
		else if ((flagmask&NETMASK_ENGINE_MODE_MANUAL)!=0)        automode = BeamEngine::MANUAL;
		else if ((flagmask&NETMASK_ENGINE_MODE_MANUAL_STICK)!=0)  automode = BeamEngine::MANUAL_STICK;
		else if ((flagmask&NETMASK_ENGINE_MODE_MANUAL_RANGES)!=0) automode = BeamEngine::MANUAL_RANGES;

		bool contact = ((flagmask&NETMASK_ENGINE_CONT) != 0);
		bool running = ((flagmask&NETMASK_ENGINE_RUN)  != 0);

		engine->netForceSettings(engspeed, engforce, engclutch, gear, running, contact, automode);
	}


	// set particle cannon
	if (((flagmask&NETMASK_PARTICLE)!=0) != cparticle_mode)
		toggleCustomParticles();

	// set lights
	if (((flagmask&NETMASK_LIGHTS)!=0) != lights)
		lightsToggle();
	if (((flagmask&NETMASK_BEACONS)!=0) != beacon)
		beaconsToggle();

	antilockbrake   = flagmask & NETMASK_ALB_ACTIVE;
	tractioncontrol = flagmask & NETMASK_TC_ACTIVE;
	parkingbrake    = flagmask & NETMASK_PBRAKE;


	blinktype btype = BLINK_NONE;
	if ((flagmask&NETMASK_BLINK_LEFT)!=0)
		btype = BLINK_LEFT;
	else if ((flagmask&NETMASK_BLINK_RIGHT)!=0)
		btype = BLINK_RIGHT;
	else if ((flagmask&NETMASK_BLINK_WARN)!=0)
		btype = BLINK_WARN;
	setBlinkType(btype);

	setCustomLightVisible(0, ((flagmask&NETMASK_CLIGHT1)>0));
	setCustomLightVisible(1, ((flagmask&NETMASK_CLIGHT2)>0));
	setCustomLightVisible(2, ((flagmask&NETMASK_CLIGHT3)>0));
	setCustomLightVisible(3, ((flagmask&NETMASK_CLIGHT4)>0));

#ifdef USE_OPENAL
	if ((flagmask & NETMASK_HORN))
		SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_HORN);
	else
		SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_HORN);
#endif //OPENAL
	netBrakeLight   = ((flagmask&NETMASK_BRAKES)!=0);
	netReverseLight = ((flagmask&NETMASK_REVERSE)!=0);

#ifdef USE_OPENAL
	if (netReverseLight)
		SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_REVERSE_GEAR);
	else
		SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_REVERSE_GEAR);
#endif //OPENAL

#ifdef USE_MYGUI
	updateDashBoards(tratio);
#endif // USE_MYGUI

	BES_GFX_STOP(BES_GFX_calcNetwork);
}

void Beam::addPressure(float v)
{
	refpressure+=v;
	if (refpressure<0) refpressure=0;
	if (refpressure>100) refpressure=100;
	for (int i=0; i<free_pressure_beam; i++)
	{
		beams[pressure_beams[i]].k=10000+refpressure*10000;
	}
}

float Beam::getPressure()
{
	if (free_pressure_beam) return refpressure;
	return 0;
}

void Beam::calc_masses2(Real total, bool reCalc)
{
	BES_GFX_START(BES_GFX_calc_masses2);

	bool debugMass = BSETTING("Debug Truck Mass", false);

	//reset
	for (int i=0; i<free_node; i++)
	{
		if (!nodes[i].iswheel)
		{
			if (!nodes[i].masstype == NODE_LOADED)
			{
				nodes[i].mass = 0;
			} else if (!nodes[i].overrideMass)
			{
				nodes[i].mass = loadmass / (float)masscount;
			}
		}
	}
	//average linear density
	Real len = 0.0f;
	for (int i=0; i<free_beam; i++)
	{
		if (beams[i].type!=BEAM_VIRTUAL)
		{
			Real half_newlen = beams[i].L / 2.0;
			if (!(beams[i].p1->iswheel)) len += half_newlen;
			if (!(beams[i].p2->iswheel)) len += half_newlen;
		}
	}
	if (!reCalc)
	{
		for (int i=0; i<free_beam; i++)
		{
			if (beams[i].type!=BEAM_VIRTUAL)
			{
				Real half_mass = beams[i].L * total / len / 2.0f;
				if (!(beams[i].p1->iswheel)) beams[i].p1->mass += half_mass;
				if (!(beams[i].p2->iswheel)) beams[i].p2->mass += half_mass;
			}
		}
	}
	//fix rope masses
	for (std::vector <rope_t>::iterator it = ropes.begin(); it!=ropes.end(); it++)
	{
		it->beam->p2->mass = 100.0f;
	}
	//fix camera mass
	for (int i=0; i<freecinecamera; i++)
		nodes[cinecameranodepos[i]].mass = 20.0f;

	//hooks must be heavy
	//for (std::vector<hook_t>::iterator it=hooks.begin(); it!=hooks.end(); it++)
	//	if (!it->hookNode->overrideMass)
	//		it->hookNode->mass = 500.0f;

	//update gravimass
	for (int i=0; i<free_node; i++)
	{
		//LOG("Nodemass "+TOSTRING(i)+"-"+TOSTRING(nodes[i].mass));
		//for stability
		if (!nodes[i].iswheel && nodes[i].mass < minimass)
		{
			if (debugMass)
				LOG("Node " + TOSTRING(i) +" mass ("+TOSTRING(nodes[i].mass)+"kg) too light. Resetting to minimass ("+ TOSTRING(minimass) +"kg).");
			nodes[i].mass = minimass;
		}
		nodes[i].gravimass = Vector3(0.0f, gEnv->terrainManager->getGravity() * nodes[i].mass, 0.0f);
	}

    // update inverted mass cache
	for (int i=0; i<free_node; i++)
	{
		nodes[i].inverted_mass=1.0f/nodes[i].mass;
    }

	//update minendmass
	for (int i=0; i<free_beam; i++)
	{
		beams[i].minendmass = beams[i].p1->mass;
		if (beams[i].p2->mass < beams[i].minendmass)
			beams[i].minendmass = beams[i].p2->mass;
	}
	totalmass = 0;
	for (int i=0; i<free_node; i++)
	{
		if (debugMass)
		{
			String msg = "Node " + TOSTRING(i) +" : "+ TOSTRING((int)nodes[i].mass) +" kg";
			if (nodes[i].masstype == NODE_LOADED)
			{
				if (nodes[i].overrideMass)
					msg += " (overriden by node mass)";
				else
					msg += " (normal load node: "+TOSTRING(loadmass)+" kg / "+TOSTRING(masscount)+" nodes)";
			}
			LOG(msg);
		}
		totalmass += nodes[i].mass;
	}
	LOG("TOTAL VEHICLE MASS: " + TOSTRING((int)totalmass) +" kg");
	//now a special stuff
	int unst = 0;
	int st = 0;
	int wunst = 0;
	for (int i=0; i<free_beam; i++)
	{
		float mass = beams[i].p1->mass;
		if (beams[i].p2->mass<mass) mass = beams[i].p2->mass;
	}
	LOG("Beams status: unstable:"+TOSTRING(unst)+" wheel:"+TOSTRING(wunst)+" normal:"+TOSTRING(free_beam-unst-wunst-st)+" superstable:"+TOSTRING(st));

	BES_GFX_STOP(BES_GFX_calc_masses2);
}

// this recalculates the masses (useful when the gravity was changed...)
void Beam::recalc_masses()
{
	this->calc_masses2(totalmass, true);
}

float Beam::getTotalMass(bool withLocked)
{
	if (!withLocked) return totalmass; // already computed in calc_masses2

	float mass = totalmass;
	
	for (std::list<Beam*>::iterator it = linkedBeams.begin(); it != linkedBeams.end(); ++it)
	{
		mass += (*it)->totalmass;
	}

	return mass;
}

void Beam::determineLinkedBeams()
{
	linkedBeams.clear();

	bool found = true;
	std::map< Beam*, bool> lookup_table;
	std::pair<std::map< Beam*, bool>::iterator, bool> ret;

	lookup_table.insert(std::pair< Beam*, bool>(this, false));

	while (found)
	{
		found = false;

		for (std::map< Beam*, bool>::iterator it_beam=lookup_table.begin(); it_beam != lookup_table.end(); ++it_beam)
		{
			if (!it_beam->second)
			{
				for (std::vector<hook_t>::iterator it_hook=it_beam->first->hooks.begin(); it_hook != it_beam->first->hooks.end(); ++it_hook)
				{
					if (it_hook->lockTruck)
					{
						ret = lookup_table.insert(std::pair< Beam*, bool>(it_hook->lockTruck, false));
						if (ret.second)
						{
							linkedBeams.push_back(it_hook->lockTruck);
							found = true;
						}
					}
				}
				it_beam->second = true;
			}
		}
	}
}

int Beam::getWheelNodeCount()
{
	return free_node-first_wheel_node;
}

void Beam::setupDefaultSoundSources()
{
#ifdef USE_OPENAL
	if (SoundScriptManager::getSingleton().isDisabled()) return;
	//engine
	if (engine)
	{
		if (engine->type=='t')
		{
			addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_diesel", trucknum, NULL), smokeId);
			addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_force", trucknum, NULL), smokeId);
			addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_brakes", trucknum, NULL), 0);
			addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_parkbrakes", trucknum, NULL), 0);
			addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_reverse_beep", trucknum, NULL), 0);
		}
		if (engine->type=='c')
			addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_car", trucknum, NULL), smokeId);
		if (engine->hasturbo)
			addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_turbo", trucknum, NULL), smokeId);
		if (engine->hasair)
			addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_air_purge", trucknum, NULL), 0);
		//starter
		addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_starter", trucknum, NULL), 0);
		// turn signals
		addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_turn_signal", trucknum, NULL), 0);
	}
	if (driveable==TRUCK)
	{
		//horn
		if (ispolice)
			addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_police", trucknum, NULL), 0);
		else
			addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_horn", trucknum, NULL), 0);
		//shift
		addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_shift", trucknum, NULL), 0);
	}
	//pump
	if (hascommands)
		addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_pump", trucknum, NULL), 0);
	//antilock brake
	if (alb_present)
		addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_antilock", trucknum, NULL), 0);
	//tractioncontrol
	if (tc_present)
		addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_tractioncontrol", trucknum, NULL), 0);
	//screetch
	if ((driveable==TRUCK || driveable==AIRPLANE) && free_wheel)
		addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_screetch", trucknum, NULL), 0);
	//break & creak
	addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_break", trucknum, NULL), 0);
	addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_creak", trucknum, NULL), 0);
	//boat engine
	if (driveable==BOAT)
	{
		if (totalmass>50000.0)
			addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_marine_large", trucknum, NULL), smokeId);
		else
			addSoundSource(SoundScriptManager::getSingleton().createInstance("tracks/default_marine_small", trucknum, NULL), smokeId);
		//no start/stop engine for boats, so set sound always on!
		SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_ENGINE);
		SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_ENGINE, 0.5);
	}
	//airplane warnings
	if (driveable==AIRPLANE)
	{
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_gpws_10"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_gpws_20"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_gpws_30"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_gpws_40"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_gpws_50"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_gpws_100"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_gpws_pullup"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_gpws_minimums"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_gpws_apdisconnect"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_aoa_warning"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_aivionic_chat01"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_aivionic_chat02"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_aivionic_chat03"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_aivionic_chat04"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_aivionic_chat05"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_aivionic_chat06"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_aivionic_chat07"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_aivionic_chat08"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_aivionic_chat09"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_aivionic_chat10"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_aivionic_chat11"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_aivionic_chat12"), trucknum, NULL), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_aivionic_chat13"), trucknum, NULL), 0);
	}
	//airplane engines
	for (int i=0; i<free_aeroengine && i<8; i++)
	{
		if (aeroengines[i]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOJET)
		{
			addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_turbojet_start")+TOSTRING(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
			addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_turbojet_lopower")+TOSTRING(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
			addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_turbojet_hipower")+TOSTRING(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
			if (((Turbojet*)(aeroengines[i]))->afterburnable)
				addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_turbojet_afterburner")+TOSTRING(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
		}
		else if (aeroengines[i]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOPROP)
		{
			if (((Turboprop*)aeroengines[i])->is_piston)
			{
				addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_pistonprop_start")+TOSTRING(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
				addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_pistonprop_lopower")+TOSTRING(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
				addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_pistonprop_hipower")+TOSTRING(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
			}
			else
			{
				addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_turboprop_start")+TOSTRING(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
				addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_turboprop_lopower")+TOSTRING(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
				addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/default_turboprop_hipower")+TOSTRING(i+1), trucknum, NULL), aeroengines[i]->getNoderef());
			}
		}
	}


	// linked sounds
	for (int i=0; i<free_commands; i++)
	{
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/linked/default_command/extend"), trucknum, NULL, SL_COMMAND, i), 0);
		addSoundSource(SoundScriptManager::getSingleton().createInstance(String("tracks/linked/default_command/retract"), trucknum, NULL, SL_COMMAND, -i), 0);
	}

#endif //OPENAL
}

void Beam::calcNodeConnectivityGraph()
{
	BES_GFX_START(BES_GFX_calcNodeConnectivityGraph);
	int i;

	nodetonodeconnections.resize(free_node, std::vector< int >());
	nodebeamconnections.resize(free_node, std::vector< int >());

	for (i=0; i<free_beam; i++)
	{
		if (beams[i].p1!=NULL && beams[i].p2!=NULL && beams[i].p1->pos>=0 && beams[i].p2->pos>=0)
		{
			nodetonodeconnections[beams[i].p1->pos].push_back(beams[i].p2->pos);
			nodebeamconnections[beams[i].p1->pos].push_back(i);
			nodetonodeconnections[beams[i].p2->pos].push_back(beams[i].p1->pos);
			nodebeamconnections[beams[i].p2->pos].push_back(i);
		}
	}
	BES_GFX_STOP(BES_GFX_calcNodeConnectivityGraph);
}

void Beam::updateContacterNodes()
{
	for (int i=0; i<free_collcab; i++)
	{
		int tmpv = collcabs[i] * 3;
		nodes[cabs[tmpv]].contacter = true;
		nodes[cabs[tmpv+1]].contacter = true;
		nodes[cabs[tmpv+2]].contacter = true;
	}
}

int Beam::savePosition(int indexPosition)
{
	if (!posStorage) return -1;
	Vector3* nbuff = posStorage->getStorage(indexPosition);
	if (!nbuff) return -3;
	for (int i=0; i<free_node; i++)
		nbuff[i] = nodes[i].AbsPosition;
	posStorage->setUsage(indexPosition, true);
	return 0;
}

int Beam::loadPosition(int indexPosition)
{
	if (!posStorage) return -1;
	if (!posStorage->getUsage(indexPosition)) return -2;

	Vector3* nbuff = posStorage->getStorage(indexPosition);
	if (!nbuff) return -3;
	Vector3 pos = Vector3(0,0,0);
	for (int i=0; i<free_node; i++)
	{
		nodes[i].AbsPosition   = nbuff[i];
		nodes[i].RelPosition   = nbuff[i] - origin;
		nodes[i].smoothpos     = nbuff[i];

		// reset forces
		nodes[i].Velocity      = Vector3::ZERO;
		nodes[i].Forces        = Vector3::ZERO;
		nodes[i].lastdrag      = Vector3::ZERO;
		nodes[i].buoyanceForce = Vector3::ZERO;
		nodes[i].lastdrag      = Vector3::ZERO;
		nodes[i].lockednode    = 0;
		nodes[i].isSkin        = nodes[i].iIsSkin;

		pos = pos + nbuff[i];
	}
	position = pos / (float)(free_node);

	resetSlideNodes();

	return 0;
}

void Beam::updateTruckPosition()
{
	// calculate average position (and smooth)
	if (externalcameramode == 0)
	{
		// the classic approach: average over all nodes and beams
		Vector3 aposition = Vector3::ZERO;
		for (int n=0; n < free_node; n++)
		{
			nodes[n].smoothpos = nodes[n].AbsPosition;
			aposition += nodes[n].smoothpos;
		}
		position = aposition / free_node;
	} else if (externalcameramode == 1 && freecinecamera > 0)
	{
		// the new (strange) approach: reuse the cinecam node
		for (int n=0; n < free_node; n++)
		{
			nodes[n].smoothpos = nodes[n].AbsPosition;
		}
		position = nodes[cinecameranodepos[0]].AbsPosition;
	} else if (externalcameramode == 2 && externalcameranode >= 0)
	{
		// the new (strange) approach #2: reuse a specified node
		for (int n=0; n < free_node; n++)
		{
			nodes[n].smoothpos = nodes[n].AbsPosition;
		}
		position = nodes[externalcameranode].AbsPosition;
	} else
	{
		Vector3 aposition = Vector3::ZERO;
		for (int n=0; n < free_node; n++)
		{
			nodes[n].smoothpos = nodes[n].AbsPosition;
			aposition += nodes[n].smoothpos;
		}
		position = aposition / free_node;
	}
}

void Beam::resetAngle(float rot)
{
	// Set origin of rotation to camera node
	Vector3 origin = nodes[0].AbsPosition;
	
	if (cameranodepos[0] >= 0 && cameranodepos[0] < MAX_NODES)
	{
		origin = nodes[cameranodepos[0]].AbsPosition;
	}

	// Set up matrix for yaw rotation
	Matrix3 matrix;
	matrix.FromEulerAnglesXYZ(Radian(0), Radian(-rot - Math::HALF_PI), Radian(0));

	for (int i = 0; i < free_node; i++)
	{
		// Move node back to origin, apply rotation matrix, and move node back
		nodes[i].AbsPosition -= origin;
		nodes[i].AbsPosition  = matrix * nodes[i].AbsPosition;
		nodes[i].AbsPosition += origin;
		// Update related values
		nodes[i].RelPosition  = nodes[i].AbsPosition;
		nodes[i].smoothpos    = nodes[i].AbsPosition;
	}

	resetSlideNodePositions();
}

void Beam::resetPosition(float px, float pz, bool setInitPosition)
{
	// horizontal displacement
	Vector3 offset = Vector3(px, -iPosition.y, pz) - nodes[0].AbsPosition;
	for (int i=0; i < free_node; i++)
	{
		nodes[i].AbsPosition += offset;
	}

	// vertical displacement
	float minoffset = nodes[0].AbsPosition.y - gEnv->terrainManager->getHeightFinder()->getHeightAt(nodes[0].AbsPosition.x, nodes[0].AbsPosition.z);

	for (int i=1; i < free_node; i++)
	{
		Vector3 pos = Vector3(nodes[i].AbsPosition.x, gEnv->terrainManager->getHeightFinder()->getHeightAt(nodes[i].AbsPosition.x, nodes[i].AbsPosition.z), nodes[i].AbsPosition.z);
		gEnv->collisions->collisionCorrect(&pos);
		minoffset = std::min(nodes[i].AbsPosition.y - pos.y, minoffset);
	}

	if (gEnv->terrainManager->getWater())
	{
		minoffset = std::min(-gEnv->terrainManager->getWater()->getHeight(), minoffset);
	}

	for (int i=0; i<free_node; i++)
	{
		nodes[i].AbsPosition.y -= minoffset;
	}

	resetPosition(Vector3::ZERO, setInitPosition);
}

void Beam::resetPosition(float px, float pz, bool setInitPosition, float miny)
{
	// horizontal displacement
	Vector3 offset = Vector3(px, -iPosition.y, pz) - nodes[0].AbsPosition;
	for (int i=0; i < free_node; i++)
	{
		nodes[i].AbsPosition += offset;
	}

	// vertical displacement
	float minoffset = nodes[0].AbsPosition.y - miny;

	for (int i=1; i < free_node; i++)
	{
		minoffset = std::min(nodes[i].AbsPosition.y - miny, minoffset);
	}

	if (gEnv->terrainManager->getWater())
	{
		minoffset = std::min(-gEnv->terrainManager->getWater()->getHeight(), minoffset);
	}

	for (int i=0; i<free_node; i++)
	{
		nodes[i].AbsPosition.y -= minoffset;
	}

	resetPosition(Vector3::ZERO, setInitPosition);
}

void Beam::resetPosition(Vector3 translation, bool setInitPosition)
{
	// total displacement
	if (translation != Vector3::ZERO)
	{
		Vector3 offset = translation - nodes[0].AbsPosition;
		for (int i=0; i<free_node; i++)
		{
			nodes[i].AbsPosition += offset;
		}
	}

	// calculate average position
	Vector3 apos(Vector3::ZERO);
	for (int i=0; i<free_node; i++)
	{
		if (setInitPosition)
		{
			nodes[i].iPosition = nodes[i].AbsPosition;
		}
		nodes[i].smoothpos   = nodes[i].AbsPosition;
		nodes[i].RelPosition = nodes[i].AbsPosition - origin;
		apos += nodes[i].AbsPosition;
	}
	position = apos / free_node;

	// calculate min camera radius for truck
	if (minCameraRadius < 0.01f)
	{
		// recalc
		for (int i=0; i<free_node; i++)
		{
			Real dist = nodes[i].AbsPosition.distance(position);
			if (dist > minCameraRadius)
			{
				minCameraRadius = dist;
			}
		}
		minCameraRadius *= 1.2f; // ten percent buffer
	}

	//if (netLabelNode) netLabelNode->setPosition(nodes[0].Position);

	resetSlideNodePositions();
}

void Beam::mouseMove(int node, Vector3 pos, float force)
{
	mousenode = node;
	mousemoveforce = force;
	mousepos = pos;
}

bool Beam::hasDriverSeat()
{
	return driverSeat != 0;
}

int Beam::calculateDriverPos(Vector3 &pos, Quaternion &rot)
{
	BES_GFX_START(BES_GFX_calculateDriverPos);
	if (!hasDriverSeat()) return 1;

	Vector3 diffY = nodes[driverSeat->nodey].smoothpos - nodes[driverSeat->noderef].smoothpos;
	Vector3 diffX = nodes[driverSeat->nodex].smoothpos - nodes[driverSeat->noderef].smoothpos;

	Vector3 normal = (diffY.crossProduct(diffX)).normalisedCopy();

	// position
	Vector3 position = nodes[driverSeat->noderef].smoothpos + driverSeat->offsetx * diffX + driverSeat->offsety * diffY;

	pos = position + normal * driverSeat->offsetz;

	// orientation
	Vector3 refx = diffX.normalisedCopy();
	Vector3 refy = refx.crossProduct(normal);

	rot = Quaternion(refx, normal, refy) * driverSeat->rot * Quaternion(Degree(180), Vector3::UNIT_Y); // rotate towards the driving direction
	BES_GFX_STOP(BES_GFX_calculateDriverPos);
	return 0;
}

void Beam::resetAutopilot()
{
	autopilot->disconnect();
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_but")->setMaterialName("tracks/hdg-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_wlv_but")->setMaterialName("tracks/wlv-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_nav_but")->setMaterialName("tracks/nav-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_but")->setMaterialName("tracks/hold-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_but")->setMaterialName("tracks/vs-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_but")->setMaterialName("tracks/athr-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_gpws_but")->setMaterialName("tracks/gpws-on");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_brks_but")->setMaterialName("tracks/brks-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_val")->setCaption("000");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_val")->setCaption("1000");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_val")->setCaption("000");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_val")->setCaption("150");
}
void Beam::disconnectAutopilot()
{
	autopilot->disconnect();
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_but")->setMaterialName("tracks/hdg-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_wlv_but")->setMaterialName("tracks/wlv-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_nav_but")->setMaterialName("tracks/nav-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_but")->setMaterialName("tracks/hold-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_but")->setMaterialName("tracks/vs-off");
	OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_but")->setMaterialName("tracks/athr-off");
}


void Beam::toggleAxleLock()
{
	for (int i = 0; i < free_axle; ++i)
	{
		if (!axles[i]) continue;
		axles[i]->toggleDiff();
	}
}

int Beam::getAxleLockCount()
{
	return free_axle;
}

String Beam::getAxleLockName()
{
	if (!axles[0]) return String();
	return axles[0]->getDiffTypeName();
}

void Beam::reset(bool keepPosition)
{
	reset_requested = keepPosition ? 2 : 1;
}

void Beam::SyncReset()
{
	hydrodirstate=0.0;
	hydroaileronstate=0.0;
	hydrorudderstate=0.0;
	hydroelevatorstate=0.0;
	hydrodirwheeldisplay=0.0;
	if (hydroInertia) hydroInertia->resetCmdKeyDelay();
	parkingbrake=0;
	cc_mode = false;
	fusedrag=Vector3::ZERO;
	origin=Vector3::ZERO;
	for (unsigned int i=0; i<interPointCD.size(); i++)
	{
		interPointCD[i]->reset();
	}
	for (unsigned int i=0; i<intraPointCD.size(); i++)
	{
		intraPointCD[i]->reset();
	}
	float yPos = nodes[lowestnode].AbsPosition.y;

	Vector3 cur_position = nodes[0].AbsPosition;
	Vector3 cur_dir = nodes[0].AbsPosition;
	if (cameranodepos[0] >= 0 && cameranodepos[0] < MAX_NODES)
	{
		cur_dir = nodes[cameranodepos[0]].RelPosition - nodes[cameranodedir[0]].RelPosition;
	}
	float cur_rot = atan2(cur_dir.dotProduct(Vector3::UNIT_X), cur_dir.dotProduct(-Vector3::UNIT_Z));
	if (engine) engine->start();
	for (int i=0; i<free_node; i++)
	{
		nodes[i].AbsPosition=nodes[i].iPosition;
		nodes[i].RelPosition=nodes[i].iPosition-origin;
		nodes[i].smoothpos=nodes[i].iPosition;
		nodes[i].Velocity=Vector3::ZERO;
		nodes[i].Forces=Vector3::ZERO;
		nodes[i].lastdrag=Vector3::ZERO;
		nodes[i].buoyanceForce=Vector3::ZERO;
		nodes[i].lastdrag=Vector3::ZERO;
		//this is problematic, we should also find what is locked to this, and unlock it
		nodes[i].lockednode=0;
		nodes[i].isSkin=nodes[i].iIsSkin;
	}

	for (int i=0; i<free_beam; i++)
	{
		beams[i].broken=0;
		beams[i].maxposstress=beams[i].default_deform;
		beams[i].maxnegstress=-beams[i].default_deform;
		beams[i].minmaxposnegstress=beams[i].default_deform;
		beams[i].strength=beams[i].iStrength;
		beams[i].plastic_coef=beams[i].default_plastic_coef;
		beams[i].L=beams[i].refL;
		beams[i].lastforce=Vector3::ZERO;
		beams[i].stress=0.0;
		beams[i].disabled=false;
		if (beams[i].mSceneNode && beams[i].type!=BEAM_VIRTUAL && beams[i].type!=BEAM_INVISIBLE && beams[i].type!=BEAM_INVISIBLE_HYDRO)
		{
			//reattach possibly detached nodes
			//beams[i].mSceneNode->setVisible(true);
			if (beams[i].mSceneNode->numAttachedObjects()==0) beams[i].mSceneNode->attachObject(beams[i].mEntity);
		}
	}

	//this is a hook assistance beam and needs to be disabled after reset
	for (std::vector <hook_t>::iterator it = hooks.begin(); it!=hooks.end(); it++)
	{
		if (it->lockTruck)
		{
			it->lockTruck->determineLinkedBeams();
			it->lockTruck->hideSkeleton(it->lockTruck->skeleton==2, false);

			for (std::list<Beam*>::iterator it_truck = it->lockTruck->linkedBeams.begin(); it_truck != it->lockTruck->linkedBeams.end(); ++it)
			{
				(*it_truck)->determineLinkedBeams();
				(*it_truck)->hideSkeleton((*it_truck)->skeleton==2, false);
			}
		}
		it->beam->mSceneNode->detachAllObjects();
		it->beam->disabled = true;
		it->locked        = UNLOCKED;
		it->lockNodes     = true;
		it->lockNode      = 0;
		it->lockTruck     = 0;
		it->beam->p2      = &nodes[0];
		it->beam->p2truck = 0;
		it->beam->L       = (nodes[0].AbsPosition - it->hookNode->AbsPosition).length();
	}

	for (int i=0; i<free_contacter; i++) contacters[i].contacted = 0;
	for (std::vector <rope_t>::iterator it = ropes.begin(); it != ropes.end(); it++) it->lockedto=0;
	for (std::vector <tie_t>::iterator it = ties.begin(); it != ties.end(); it++)
	{
		it->beam->disabled = true;
		it->beam->p2       = &nodes[0];
		it->beam->mSceneNode->detachAllObjects();
	}
	for (int i=0; i<free_aeroengine; i++) aeroengines[i]->reset();
	for (int i=0; i<free_screwprop; i++) screwprops[i]->reset();
	for (int i=0; i<free_rotator; i++) rotators[i].angle = 0.0;
	for (int i=0; i<free_wing; i++) wings[i].fa->broken = false;
	for (int i=0; i<free_wheel; i++) wheels[i].speed = 0.0;
	if (buoyance) buoyance->setsink(0);
	refpressure = 50.0;
	addPressure(0.0);
	if (autopilot) resetAutopilot();
	for (int i=0; i<free_flexbody; i++) flexbodies[i]->reset();

	// reset on spot with backspace
	if (reset_requested == 2)
	{
		resetAngle(cur_rot);

		if (yPos != 0)
			resetPosition(cur_position.x, cur_position.z, false, yPos + 0.02f);
		else
			resetPosition(cur_position.x, cur_position.z, false);
	}

	// reset commands (self centering && push once/twice forced to terminate moving commands)
	for (int i=0; i<MAX_COMMANDS; i++)
	{
		commandkey[i].commandValue = 0.0;
		commandkey[i].triggerInputValue = 0.0f;
		commandkey[i].playerInputValue = 0.0f;
	}

	resetSlideNodes();
	reset_requested = 0;
}

// TODO: Move this to beamfactory
// this is called by the beamfactory worker thread
void Beam::threadentry()
{
	Beam **trucks = ttrucks;
	dtperstep = tdt / (Real)tsteps;

	for (curtstep=0; curtstep<tsteps; curtstep++)
	{
		num_simulated_trucks = 0;
		gEnv->mrTime += dtperstep;

		for (int t=0; t<tnumtrucks; t++)
		{
			if (trucks[t] && (trucks[t]->simulated = trucks[t]->calcForcesEulerPrepare(curtstep==0, dtperstep, curtstep, tsteps)))
			{
				num_simulated_trucks++;
				trucks[t]->calledby    = this;
				trucks[t]->curtstep    = this->curtstep;
				trucks[t]->tsteps      = this->tsteps;
				trucks[t]->dtperstep   = this->dtperstep;
				trucks[t]->thread_task = THREAD_BEAMFORCESEULER;
			}
		}
		for (int t=0; t<tnumtrucks; t++)
		{
			if (trucks[t])
				trucks[t]->num_simulated_trucks = this->num_simulated_trucks;
		}
		if (num_simulated_trucks < 2 || !BeamFactory::getSingleton().beamThreadPool)
		{
			for (int t=0; t<tnumtrucks; t++)
			{
				if (trucks[t] && trucks[t]->simulated)
				{
					trucks[t]->calcForcesEulerCompute(curtstep==0, dtperstep, curtstep, tsteps);
					if (!disableTruckTruckSelfCollisions)
					{
						trucks[t]->intraTruckCollisionsPrepare(dtperstep);
						runThreadTask(trucks[t], THREAD_INTRA_TRUCK_COLLISIONS);
						trucks[t]->intraTruckCollisionsFinal(dtperstep);
					}
					break;
				}
			}
		} else
		{
			task_count[THREAD_BEAMFORCESEULER] = num_simulated_trucks;

			std::list<IThreadTask*> tasks;

			// Push tasks into thread pool
			for (int t=0; t<tnumtrucks; t++)
			{
				if (trucks[t] && trucks[t]->simulated)
				{
					tasks.emplace_back(trucks[t]);
				}
			}

			BeamFactory::getSingleton().beamThreadPool->enqueue(tasks);

			// Wait for all tasks to complete
			MUTEX_LOCK(&task_count_mutex[THREAD_BEAMFORCESEULER]);
			while (task_count[THREAD_BEAMFORCESEULER] > 0)
			{
				pthread_cond_wait(&task_count_cv[THREAD_BEAMFORCESEULER], &task_count_mutex[THREAD_BEAMFORCESEULER]);
			}
			MUTEX_UNLOCK(&task_count_mutex[THREAD_BEAMFORCESEULER]);
		}
		for (int t=0; t<tnumtrucks; t++)
		{
			if (trucks[t] && trucks[t]->simulated)
				trucks[t]->calcForcesEulerFinal(curtstep==0, dtperstep, curtstep, tsteps);
		}

		if (!disableTruckTruckCollisions && num_simulated_trucks > 1)
		{
			BES_START(BES_CORE_Contacters);
			interTruckCollisionsPrepare(dtperstep);
			runThreadTask(this, THREAD_INTER_TRUCK_COLLISIONS);
			interTruckCollisionsFinal(dtperstep);
			BES_STOP(BES_CORE_Contacters);
		}
	}
}

//integration loop
//bool frameStarted(const FrameEvent& evt)
//this will be called once by frame and is responsible for animation of all the trucks!
//the instance called is the one of the current ACTIVATED truck
bool Beam::frameStep(Real dt)
{
	BES_GFX_START(BES_GFX_framestep);

	if (dt==0) return true;
	if (!loading_finished) return true;
	if (state >= SLEEPING) return true;
	if (mTimeUntilNextToggle > -1)
		mTimeUntilNextToggle -= dt;
	
	int steps = 2000.0 * dt;

	// TODO: move this to the correct spot
	// update all dashboards
#ifdef USE_MYGUI
	updateDashBoards(dt);
#endif // USE_MYGUI

	// some scripting stuff:
#ifdef USE_ANGELSCRIPT
	// TODO: Update locked
	if (locked != lockedold)
	{
		if (locked == LOCKED) ScriptEngine::getSingleton().triggerEvent(SE_TRUCK_LOCKED, trucknum);
		if (locked == UNLOCKED) ScriptEngine::getSingleton().triggerEvent(SE_TRUCK_UNLOCKED, trucknum);
		lockedold = locked;
	}
	if (watercontact && !watercontactold)
	{
		ScriptEngine::getSingleton().triggerEvent(SE_TRUCK_TOUCHED_WATER, trucknum);
	}
	watercontactold = watercontact;
#endif

	BES_GFX_STOP(BES_GFX_framestep);

	Beam **trucks = BeamFactory::getSingleton().getTrucks();
	int numtrucks = BeamFactory::getSingleton().getTruckCount();

	stabsleep -= dt;
	if (replaymode && replay && replay->isValid())
	{
		// no replay update needed if position was not changed
		if (replaypos != oldreplaypos)
		{
			unsigned long time=0;
			//replay update
			node_simple_t *nbuff = (node_simple_t *)replay->getReadBuffer(replaypos, 0, time);
			if (nbuff)
			{
				Vector3 pos = Vector3::ZERO;
				for (int i=0; i<free_node; i++)
				{
					nodes[i].AbsPosition = nbuff[i].pos;
					nodes[i].RelPosition = nbuff[i].pos - origin;
					nodes[i].smoothpos = nbuff[i].pos;
					pos = pos + nbuff[i].pos;
				}
				updateSlideNodePositions();

				position=pos/(float)(free_node);
				// now beams
				beam_simple_t *bbuff = (beam_simple_t *)replay->getReadBuffer(replaypos, 1, time);
				for (int i=0; i<free_beam; i++)
				{
					beams[i].scale = bbuff[i].scale;
					beams[i].broken = bbuff[i].broken;
					beams[i].disabled = bbuff[i].disabled;
				}
				//LOG("replay: " + TOSTRING(time));
				oldreplaypos = replaypos;
			}
		}
	} else
	{
		// simulation update
		if (BeamFactory::getSingleton().getThreadingMode() == THREAD_SINGLE)
		{
			dtperstep = dt / (Real)steps;
			
			for (int i=0; i<steps; i++)
			{
				int num_simulated_trucks = 0;
				gEnv->mrTime += dtperstep;

				for (int t=0; t<numtrucks; t++)
				{
					if (trucks[t] && (trucks[t]->simulated = trucks[t]->calcForcesEulerPrepare(i==0, dtperstep, i, steps)))
					{
						num_simulated_trucks++;
						trucks[t]->calcForcesEulerCompute(i==0, dtperstep, i, steps);
						trucks[t]->calcForcesEulerFinal(i==0, dtperstep, i, steps);
						if (!disableTruckTruckSelfCollisions)
						{
							trucks[t]->intraTruckCollisionsPrepare(dtperstep);
							trucks[t]->intraTruckCollisionsCompute(dtperstep);
							trucks[t]->intraTruckCollisionsFinal(dtperstep);
						}
					}
				}
				if (!disableTruckTruckCollisions && num_simulated_trucks > 1)
				{
					BES_START(BES_CORE_Contacters);
					interTruckCollisionsPrepare(dtperstep);
					interTruckCollisionsCompute(dtperstep);
					interTruckCollisionsFinal(dtperstep);
					BES_STOP(BES_CORE_Contacters);
				}
			}
		} else if (!BeamFactory::getSingleton().asynchronousPhysics())
		{
			BeamFactory::getSingleton()._WorkerWaitForSync();
		}
		
		ttdt = tdt;
		tdt = dt;

		ffforce = affforce / steps;
		ffhydro = affhydro / steps;
		if (free_hydro) ffhydro = ffhydro / free_hydro;

		for (int t=0; t<numtrucks; t++)
		{
			if (!trucks[t]) continue;

			if (trucks[t]->reset_requested)
			{
				trucks[t]->SyncReset();
			}
			if (trucks[t]->simulated)
			{
				trucks[t]->lastlastposition = trucks[t]->lastposition;
				trucks[t]->lastposition = trucks[t]->position;
				trucks[t]->updateTruckPosition();
			}
			if (floating_origin_enable && trucks[t]->nodes[0].RelPosition.squaredLength() > 10000.0)
			{
				trucks[t]->moveOrigin(trucks[t]->nodes[0].RelPosition);
			}
		}

#ifdef FEAT_TIMING
		if (statistics)     statistics->frameStep(dt);
		if (statistics_gfx) statistics_gfx->frameStep(dt);
#endif // FEAT_TIMING
		
		// we must take care of this
		for (int t=0; t<numtrucks; t++)
		{
			if (!trucks[t]) continue;

			// synchronous sleep
			if (trucks[t]->state == GOSLEEP) trucks[t]->state = SLEEPING;

			if (!BeamFactory::getSingleton().allTrucksForcedActive() && trucks[t]->state == DESACTIVATED)
			{
				trucks[t]->sleepcount++;
				if ((trucks[t]->lastposition - trucks[t]->lastlastposition).length() / dt > 0.1f)
				{
					trucks[t]->sleepcount = 7;
				}
				if (trucks[t]->sleepcount > 10)
				{
					trucks[t]->state = MAYSLEEP;
					trucks[t]->sleepcount = 0;
				}
			}
		}

		if (BeamFactory::getSingleton().getThreadingMode() == THREAD_MULTI)
		{
			tsteps = steps;
			ttrucks = trucks;
			tnumtrucks = numtrucks;
			BeamFactory::getSingleton()._WorkerPrepareStart();
			BeamFactory::getSingleton()._WorkerSignalStart();
		}
	}

	return true;
}

void Beam::sendStreamSetup()
{
	if (!gEnv->network || state == NETWORKED ) return;
	// only init stream if its local.
	// the stream is local when networking=true and networked=false

	// register the local stream
	stream_register_trucks_t reg;
	memset(&reg, 0, sizeof(stream_register_trucks_t));
	reg.status = 0;
	reg.type   = 0; // 0 = truck
	reg.bufferSize = netbuffersize;
	strncpy(reg.name, realtruckfilename.c_str(), 128);
	if (!truckconfig.empty())
	{
		// insert section config
		for (int i = 0; i < std::min<int>((int)truckconfig.size(), 10); i++)
			strncpy(reg.truckconfig[i], truckconfig[i].c_str(), 60);
	}

	NetworkStreamManager::getSingleton().addLocalStream(this, (stream_register_t *)&reg, sizeof(reg));
}

void Beam::sendStreamData()
{
	BES_GFX_START(BES_GFX_sendStreamData);
#ifdef USE_SOCKETW
	int t = netTimer.getMilliseconds();
	if (t-last_net_time < 100)
		return;

	last_net_time = t;

	//look if the packet is too big first
	int final_packet_size = sizeof(oob_t) + sizeof(float) * 3 + first_wheel_node * sizeof(float) * 3 + free_wheel * sizeof(float);
	if (final_packet_size > (int)maxPacketLen)
	{
		showError(_L("Truck is too big to be send over the net."), _L("Network error!"));
		exit(126);
	}

	char send_buffer[maxPacketLen];
	memset(send_buffer, 0, maxPacketLen);

	unsigned int packet_len = 0;

	// oob_t is at the beginning of the buffer
	{
		oob_t *send_oob = (oob_t *)send_buffer;
		packet_len += sizeof(oob_t);

		send_oob->flagmask = 0;

		send_oob->time = Network::getNetTime();
		if (engine)
		{
			send_oob->engine_speed   = engine->getRPM();
			send_oob->engine_force   = engine->getAcc();
			send_oob->engine_clutch  = engine->getClutch();
			send_oob->engine_gear    = engine->getGear();

			if (engine->contact) send_oob->flagmask += NETMASK_ENGINE_CONT;
			if (engine->running) send_oob->flagmask += NETMASK_ENGINE_RUN;

			if      (engine->getAutoMode() == BeamEngine::AUTOMATIC)     send_oob->flagmask += NETMASK_ENGINE_MODE_AUTOMATIC;
			else if (engine->getAutoMode() == BeamEngine::SEMIAUTO)      send_oob->flagmask += NETMASK_ENGINE_MODE_SEMIAUTO;
			else if (engine->getAutoMode() == BeamEngine::MANUAL)        send_oob->flagmask += NETMASK_ENGINE_MODE_MANUAL;
			else if (engine->getAutoMode() == BeamEngine::MANUAL_STICK)  send_oob->flagmask += NETMASK_ENGINE_MODE_MANUAL_STICK;
			else if (engine->getAutoMode() == BeamEngine::MANUAL_RANGES) send_oob->flagmask += NETMASK_ENGINE_MODE_MANUAL_RANGES;

		}
		if (free_aeroengine>0)
		{
			float rpm =aeroengines[0]->getRPM();
			send_oob->engine_speed = rpm;
		}

		send_oob->hydrodirstate = hydrodirstate;
		send_oob->brake         = brake;
		send_oob->wheelspeed    = WheelSpeed;

		blinktype b = getBlinkType();
		if      (b == BLINK_LEFT)       send_oob->flagmask += NETMASK_BLINK_LEFT;
		else if (b == BLINK_RIGHT)      send_oob->flagmask += NETMASK_BLINK_RIGHT;
		else if (b == BLINK_WARN)       send_oob->flagmask += NETMASK_BLINK_WARN;

		if (lights)                     send_oob->flagmask += NETMASK_LIGHTS;
		if (getCustomLightVisible(0))   send_oob->flagmask += NETMASK_CLIGHT1;
		if (getCustomLightVisible(1))   send_oob->flagmask += NETMASK_CLIGHT2;
		if (getCustomLightVisible(2))   send_oob->flagmask += NETMASK_CLIGHT3;
		if (getCustomLightVisible(3))   send_oob->flagmask += NETMASK_CLIGHT4;

		if (getBrakeLightVisible())		send_oob->flagmask += NETMASK_BRAKES;
		if (getReverseLightVisible())	send_oob->flagmask += NETMASK_REVERSE;
		if (getBeaconMode())			send_oob->flagmask += NETMASK_BEACONS;
		if (getCustomParticleMode())    send_oob->flagmask += NETMASK_PARTICLE;

		if (parkingbrake)                send_oob->flagmask += NETMASK_PBRAKE;
		if (tractioncontrol)             send_oob->flagmask += NETMASK_TC_ACTIVE;
		if (antilockbrake)               send_oob->flagmask += NETMASK_ALB_ACTIVE;

#ifdef USE_OPENAL
		if (SoundScriptManager::getSingleton().getTrigState(trucknum, SS_TRIG_HORN))
			send_oob->flagmask += NETMASK_HORN;
#endif //OPENAL
	}


	// then process the contents
	{
		char *ptr = send_buffer + sizeof(oob_t);
		float *send_nodes = (float *)ptr;
		packet_len += netbuffersize;

		// copy data into the buffer
		int i;

		// reference node first
		Vector3 &refpos = nodes[0].AbsPosition;
		send_nodes[0]   = refpos.x;
		send_nodes[1]   = refpos.y;
		send_nodes[2]   = refpos.z;

		ptr += sizeof(float) * 3;// plus 3 floats from above

		// then copy the other nodes into a compressed short format
		short *sbuf = (short*)ptr;
		for (i = 1; i < first_wheel_node; i++)
		{
			Vector3 relpos =nodes[i].AbsPosition - refpos;
			sbuf[(i - 1) * 3 + 0] = (short int)(relpos.x * 300.0f);
			sbuf[(i - 1) * 3 + 1] = (short int)(relpos.y * 300.0f);
			sbuf[(i - 1) * 3 + 2] = (short int)(relpos.z * 300.0f);

			ptr += sizeof(short int) * 3; // increase pointer
		}

		// then to the wheels
		float *wfbuf = (float*)ptr;
		for (i = 0; i < free_wheel; i++)
		{
			wfbuf[i] = wheels[i].rp;
		}
	}

	this->addPacket(MSG2_STREAM_DATA, packet_len, send_buffer);
#endif //SOCKETW
	BES_GFX_STOP(BES_GFX_sendStreamData);
}

void Beam::receiveStreamData(unsigned int &type, int &source, unsigned int &_streamid, char *buffer, unsigned int &len)
{
	BES_GFX_START(BES_GFX_receiveStreamData);
	if (state != NETWORKED) return; // this should not happen
	// TODO: FIX
	//if (this->source != source || this->streamid != streamid) return; // data not for us

	if (type == MSG2_STREAM_DATA
	   && source == (int)this->sourceid
	   && _streamid == this->streamid
	  )
	{
		pushNetwork(buffer, len);
	}
	BES_GFX_STOP(BES_GFX_receiveStreamData);
}

void Beam::calcAnimators(int flagstate, float &cstate, int &div, Real timer, float opt1, float opt2, float opt3)
{
	BES_GFX_START(BES_GFX_calcAnimators);
	int flag_state=flagstate;
	Real dt = timer;
	float option1 = opt1;
	float option2 = opt2;
	float option3 = opt3;

	//boat rudder
	if (flag_state & ANIM_FLAG_BRUDDER)
	{
		int spi;
		float ctmp = 0.0f;
		for (spi=0; spi<free_screwprop; spi++)
			if (screwprops[spi]) ctmp += screwprops[spi]->getRudder();

		if (spi > 0) ctmp = ctmp / spi;
		cstate = ctmp;
		div++;
	}

	//boat throttle
	if (flag_state & ANIM_FLAG_BTHROTTLE)
	{
		int spi;
		float ctmp = 0.0f;
		for (spi=0; spi<free_screwprop; spi++)
			if (screwprops[spi]) ctmp += screwprops[spi]->getThrottle();

		if (spi > 0) ctmp = ctmp / spi;
		cstate = ctmp;
		div++;
	}

	//differential lock status
	if (flag_state & ANIM_FLAG_DIFFLOCK)
	{
		if (free_axle)
		{
			if (getAxleLockName() == "Open") cstate = 0.0f;
			if (getAxleLockName() == "Split") cstate = 0.5f;
			if (getAxleLockName() == "Locked") cstate = 1.0f;
		} else  // no axles/diffs avail, mode is split by default
			cstate=0.5f;

		div++;
	}

	//heading
	if (flag_state & ANIM_FLAG_HEADING)
	{
		float heading = getHeadingDirectionAngle();
		// rad2deg limitedrange  -1 to +1
		cstate = (heading * 57.29578f) / 360.0f;
		div++;
	}

	//torque
	if (flag_state & ANIM_FLAG_TORQUE)
	{
		float torque=engine->getCrankFactor();
		if (torque <= 0.0f) torque = 0.0f;
		if (torque >= previousCrank)
			cstate -= torque / 10.0f;
		else
			cstate = 0.0f;

		if (cstate <= -1.0f) cstate = -1.0f;
		previousCrank = torque;
		div++;
	}

	//shifterseq, to amimate sequentiell shifting
	if ((flag_state & ANIM_FLAG_SHIFTER) && option3 == 3.0f)
	{
	// opt1 &opt2 = 0   this is a shifter
		if (!option1 &&  !option2)
		{
			int shifter=engine->getGear();
			if (shifter > previousGear)
			{
				cstate = 1.0f;
				animTimer = 0.2f;
			}
			if (shifter < previousGear)
			{
				cstate = -1.0f;
				animTimer = -0.2f;
			}
			previousGear = shifter;

			if (animTimer > 0.0f)
			{
				cstate = 1.0f;
				animTimer -= dt;
				if (animTimer < 0.0f)
					animTimer = 0.0f;
			}
			if (animTimer < 0.0f)
			{
				cstate = -1.0f;
				animTimer += dt;
				if (animTimer > 0.0f)
					animTimer = 0.0f;
			}
		} else
		{
			// check if option1 is a valid to get commandvalue, then get commandvalue
			if (option1 >= 1.0f && option1 <= 48.0)
				if (commandkey[int(option1)].commandValue > 0) cstate += 1.0f;
			// check if option2 is a valid to get commandvalue, then get commandvalue
			if (option2 >= 1.0f && option2 <= 48.0)
				if (commandkey[int(option2)].commandValue > 0) cstate -= 1.0f;
		}

		div++;
	}

	//shifterman1, left/right
	if ((flag_state & ANIM_FLAG_SHIFTER) && option3 == 1.0f)
	{
		int shifter=engine->getGear();
		if (!shifter)
		{
			cstate = -0.5f;
		} else
		if (shifter < 0)
		{
			cstate = 1.0f;
		} else
		{
			cstate -= int((shifter - 1.0) / 2.0);
		}
		div++;
	}

	//shifterman2, up/down
	if ((flag_state & ANIM_FLAG_SHIFTER) && option3 == 2.0f)
	{
		int shifter=engine->getGear();
		cstate = 0.5f;
		if (shifter < 0)
		{
			cstate = 1.0f;
		}
		if (shifter > 0)
		{
			cstate = shifter % 2;
		}
		div++;
	}

	//shifterlinear, to amimate cockpit gearselect gauge and autotransmission stick
	if ((flag_state & ANIM_FLAG_SHIFTER) && option3 == 4.0f)
	{
		int shifter=engine->getGear();
		int numgears=engine->getNumGears();
		cstate -= (shifter + 2.0) / (numgears + 2.0);
		div++;
	}

	//parking brake
	if (flag_state & ANIM_FLAG_PBRAKE)
	{
		float pbrake=parkingbrake;
		cstate -= pbrake;
		div++;
	}

	//speedo ( scales with speedomax )
	if (flag_state & ANIM_FLAG_SPEEDO)
	{
		float speedo=WheelSpeed / speedoMax;
		cstate -= speedo * 3.0f;
		div++;
	}

	//engine tacho ( scales with maxrpm, default is 3500 )
	if (flag_state & ANIM_FLAG_TACHO)
	{
		float tacho=engine->getRPM()/engine->getMaxRPM();
		cstate -= tacho;
		div++;
	}

	//turbo
	if (flag_state & ANIM_FLAG_TURBO)
	{
		float turbo=engine->getTurboPSI()*3.34;
		cstate -= turbo / 67.0f ;
		div++;
	}

	//brake
	if (flag_state & ANIM_FLAG_BRAKE)
	{
		float brakes=brake/brakeforce;
		cstate -= brakes;
		div++;
	}

	//accelerator
	if (flag_state & ANIM_FLAG_ACCEL)
	{
		float accel=engine->getAcc();
		cstate -= accel + 0.06f;
		//( small correction, get acc is nver smaller then 0.06.
		div++;
	}

		//clutch
	if (flag_state & ANIM_FLAG_CLUTCH)
	{
		float clutch=engine->getClutch();
		cstate -= abs(1.0f - clutch);
		div++;
	}

	//aeroengines rpm + throttle + torque ( turboprop ) + pitch ( turboprop ) + status +  fire
	int ftp=free_aeroengine;

	if (ftp > option3 - 1.0f)
	{
		int aenum = int(option3 - 1.0f);
		if (flag_state & ANIM_FLAG_RPM)
		{
			float angle;
			float pcent=aeroengines[aenum]->getRPMpc();
			if (pcent<60.0) angle=-5.0+pcent*1.9167;
			else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
			else angle=314.0;
			cstate -= angle / 314.0f;
			div++;
		}
		if (flag_state & ANIM_FLAG_THROTTLE)
		{
			float throttle=aeroengines[aenum]->getThrottle();
			cstate -= throttle;
			div++;
		}

		if (flag_state & ANIM_FLAG_AETORQUE)
			if (aeroengines[aenum]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOPROP)
			{
				Turboprop *tp=(Turboprop*)aeroengines[aenum];
				cstate=(100.0*tp->indicated_torque/tp->max_torque) / 120.0f;
				div++;
			}

		if (flag_state & ANIM_FLAG_AEPITCH)
			if (aeroengines[aenum]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOPROP)
			{
				Turboprop *tp=(Turboprop*)aeroengines[aenum];
				cstate=tp->pitch / 120.0f;
				div++;
			}

		if (flag_state & ANIM_FLAG_AESTATUS)
		{
 			if (!aeroengines[aenum]->getIgnition())
				cstate = 0.0f;
			else
				cstate = 0.5f;
			if (aeroengines[aenum]->isFailed()) cstate = 1.0f;
			div++;
		}
	}

	//airspeed indicator
	if (flag_state & ANIM_FLAG_AIRSPEED)
	{

		// TODO Unused Varaible
		//float angle=0.0;
		float ground_speed_kt= nodes[0].Velocity.length()*1.9438;
		float altitude=nodes[0].AbsPosition.y;

		// TODO Unused Varaible
		//float sea_level_temperature=273.15+15.0; //in Kelvin
		float sea_level_pressure=101325; //in Pa

		// TODO Unused Varaible
		//float airtemperature=sea_level_temperature-altitude*0.0065; //in Kelvin
		float airpressure=sea_level_pressure*pow(1.0-0.0065*altitude/288.15, 5.24947); //in Pa
		float airdensity=airpressure*0.0000120896;//1.225 at sea level
		float kt=ground_speed_kt*sqrt(airdensity/1.225);
		cstate -= kt / 100.0f;
		div++;
	}

	//vvi indicator
	if (flag_state & ANIM_FLAG_VVI)
	{
		float vvi=nodes[0].Velocity.y*196.85;
		// limit vvi scale to +/- 6m/s
		cstate -=vvi / 6000.0f;
		if (cstate >= 1.0f) cstate = 1.0f;
		if (cstate <= -1.0f) cstate = -1.0f;
		div++;
	}

	//altimeter
	if (flag_state & ANIM_FLAG_ALTIMETER)
	{
		//altimeter indicator 1k oscillating
		if (option3 == 3.0f)
		{
			float altimeter = (nodes[0].AbsPosition.y*1.1811) / 360.0f;
			int alti_int = int(altimeter);
			float alti_mod= (altimeter - alti_int);
			cstate -= alti_mod;
		}

		//altimeter indicator 10k oscillating
		if (option3 == 2.0f)
		{
			float alti=nodes[0].AbsPosition.y*1.1811 / 3600.0f;
			int alti_int = int(alti);
			float alti_mod= (alti - alti_int);
			cstate -= alti_mod;
			if (cstate <= -1.0f) cstate = -1.0f;
		}

		//altimeter indicator 100k limited
		if (option3 == 1.0f)
		{
			float alti=nodes[0].AbsPosition.y*1.1811  / 36000.0f;
			cstate -= alti;
			if (cstate <= -1.0f) cstate = -1.0f;
		}
		div++;
	}

	//AOA
	if (flag_state & ANIM_FLAG_AOA)
	{
		float aoa=0;
		if (free_wing>4) aoa=(wings[4].fa->aoa) / 25.0f;
		if ((nodes[0].Velocity.length()*1.9438) < 10.0f) aoa=0;
		cstate -= aoa;
		if (cstate <= -1.0f) cstate = -1.0f;
		if (cstate >= 1.0f) cstate = 1.0f;
		div++;
	}

	Vector3 cam_pos  = nodes[0].RelPosition;
	Vector3 cam_roll = nodes[0].RelPosition;
	Vector3 cam_dir  = nodes[0].RelPosition;

	if (cameranodepos[0] >= 0 && cameranodepos[0] < MAX_NODES)
	{
		cam_pos  = nodes[cameranodepos[0]].RelPosition;
		cam_roll = nodes[cameranoderoll[0]].RelPosition;
		cam_dir  = nodes[cameranodedir[0]].RelPosition;
	}

	// roll
	if (flag_state & ANIM_FLAG_ROLL)
	{
		Vector3 rollv = (cam_pos - cam_roll).normalisedCopy();
		Vector3 dirv  = (cam_pos - cam_dir ).normalisedCopy();
		Vector3 upv   = dirv.crossProduct(-rollv);
		float rollangle = asin(rollv.dotProduct(Vector3::UNIT_Y));
		// rad to deg
		rollangle = Math::RadiansToDegrees(rollangle);
		// flip to other side when upside down
		if (upv.y < 0) rollangle = 180.0f - rollangle;
		cstate = rollangle / 180.0f;
		// data output is -0.5 to 1.5, normalize to -1 to +1 without changing the zero position.
		// this is vital for the animator beams and does not effect the animated props
		if (cstate >= 1.0f) cstate = cstate - 2.0f;
		div++;
	}

	// pitch
	if (flag_state & ANIM_FLAG_PITCH)
	{
		Vector3 dirv  = (cam_pos - cam_dir ).normalisedCopy();
		float pitchangle = asin(dirv.dotProduct(Vector3::UNIT_Y));
		// radian to degrees with a max cstate of +/- 1.0
		cstate = (Math::RadiansToDegrees(pitchangle) / 90.0f);
		div++;
	}

	// airbrake
	if (flag_state & ANIM_FLAG_AIRBRAKE)
	{
		float airbrake = airbrakeval;
		// cstate limited to -1.0f
		cstate -= airbrake / 5.0f;
		div++;
	}

	//flaps
	if (flag_state & ANIM_FLAG_FLAP)
	{
		float flaps=flapangles[flap];
		// cstate limited to -1.0f
		cstate = flaps;
		div++;
	}

	BES_GFX_STOP(BES_GFX_calcAnimators);
}


void Beam::calcShocks2(int beam_i, Real difftoBeamL, Real &k, Real &d, Real dt, int update)
{
	if (!beams[beam_i].shock) return;

	int i=beam_i;
	float beamsLep=beams[i].L*0.8f;
	float longboundprelimit=beams[i].longbound*beamsLep;
	float shortboundprelimit=-beams[i].shortbound*beamsLep;
	// this is a shock2
	float logafactor;
	//shock extending since last cycle
	if (beams[i].shock->lastpos < difftoBeamL)
	{
		//get outbound values
		k=beams[i].shock->springout;
		d=beams[i].shock->dampout;
		// add progression
		if (beams[i].longbound != 0.0f)
		{
			logafactor=difftoBeamL/(beams[i].longbound*beams[i].L);
			logafactor=logafactor*logafactor;
		} else
		{
			logafactor = 1.0f;
		}
		if (logafactor > 1.0f) logafactor = 1.0f;
		k=k+(beams[i].shock->sprogout*k*logafactor);
		d=d+(beams[i].shock->dprogout*d*logafactor);
	} else
	{
		//shock compresssing since last cycle
		//get inbound values
		k=beams[i].shock->springin;
		d=beams[i].shock->dampin;
		// add progression
		if (beams[i].shortbound != 0.0f)
		{
			logafactor=difftoBeamL/(beams[i].shortbound*beams[i].L);
			logafactor=logafactor*logafactor;
		} else
		{
			logafactor = 1.0f;
		}
		if (logafactor > 1.0f) logafactor = 1.0f;
		k=k+(beams[i].shock->sprogin*k*logafactor);
		d=d+(beams[i].shock->dprogin*d*logafactor);
	}
	if (beams[i].shock->flags & SHOCK_FLAG_SOFTBUMP)
	{
		// soft bump shocks
		if (difftoBeamL > longboundprelimit)
		{
			//reset to longbound progressive values (oscillating beam workaround)
			k=beams[i].shock->springout;
			d=beams[i].shock->dampout;
			// add progression
			if (beams[i].longbound != 0.0f)
			{
				logafactor=difftoBeamL/(beams[i].longbound*beams[i].L);
				logafactor=logafactor*logafactor;
			} else
			{
				logafactor = 1.0f;
			}
			if (logafactor > 1.0f) logafactor = 1.0f;
			k=k+(beams[i].shock->sprogout*k*logafactor);
			d=d+(beams[i].shock->dprogout*d*logafactor);
			//add shortbump progression
			if (beams[i].longbound != 0.0f)
			{
				logafactor=((difftoBeamL-longboundprelimit)*5.0f)/(beams[i].longbound*beams[i].L);
				logafactor=logafactor*logafactor;
			} else
			{
				logafactor = 1.0f;
			}
			if (logafactor > 1.0f) logafactor = 1.0f;
			k=k+(k+ 100.0f)* beams[i].shock->sprogout *logafactor;
			d=d+(d+ 100.0f)* beams[i].shock->dprogout * logafactor;
			if (beams[i].shock->lastpos > difftoBeamL)
			// rebound mode..get new values
			{
				k=beams[i].shock->springin;
				d=beams[i].shock->dampin;
			}
		} else if (difftoBeamL < shortboundprelimit)
		{
			//reset to shortbound progressive values (oscillating beam workaround)
			k=beams[i].shock->springin;
			d=beams[i].shock->dampin;
			if (beams[i].shortbound != 0.0f)
			{
				logafactor=difftoBeamL/(beams[i].shortbound*beams[i].L);
				logafactor=logafactor*logafactor;
			} else
			{
				logafactor = 1.0f;
			}
			if (logafactor > 1.0f) logafactor = 1.0f;
			k=k+(beams[i].shock->sprogin*k*logafactor);
			d=d+(beams[i].shock->dprogin*d*logafactor);
			//add shortbump progression
			if (beams[i].shortbound != 0.0f)
			{
				logafactor=((difftoBeamL-shortboundprelimit)*5.0f)/(beams[i].shortbound*beams[i].L);
				logafactor=logafactor*logafactor;
			} else
			{
				logafactor = 1.0f;
			}
			if (logafactor > 1.0f) logafactor = 1.0f;
			k=k+(k+ 100.0f)* beams[i].shock->sprogout *logafactor;
			d=d+(d+ 100.0f)* beams[i].shock->dprogout * logafactor;
			if (beams[i].shock->lastpos < difftoBeamL)
			// rebound mode..get new values
			{
				k=beams[i].shock->springout;
				d=beams[i].shock->dampout;
			}
		}
		if (difftoBeamL > beams[i].longbound*beams[i].L || difftoBeamL < -beams[i].shortbound*beams[i].L)
		{
			// block reached...hard bump in soft mode with 4x default damping
			if (k < beams[i].shock->sbd_spring) k = beams[i].shock->sbd_spring;
			if (d < beams[i].shock->sbd_damp) d = beams[i].shock->sbd_damp;
		}
	}

	if (beams[i].shock->flags & SHOCK_FLAG_NORMAL)
	{
		if (difftoBeamL > beams[i].longbound*beams[i].L || difftoBeamL < -beams[i].shortbound*beams[i].L)
		{
			if (beams[i].shock && !(beams[i].shock->flags & SHOCK_FLAG_ISTRIGGER)) // this is NOT a trigger beam
			{
				// hard (normal) shock bump
				k = beams[i].shock->sbd_spring;
				d = beams[i].shock->sbd_damp;
			}
		}

		if (beams[i].shock && (beams[i].shock->flags & SHOCK_FLAG_ISTRIGGER) && beams[i].shock->trigger_enabled)  // this is a trigger and its enabled
		{
			if (difftoBeamL > beams[i].longbound*beams[i].L || difftoBeamL < -beams[i].shortbound*beams[i].L) // that has hit boundary
			{
				beams[i].shock->trigger_switch_state -= dt;
				if (beams[i].shock->trigger_switch_state <= 0.0f) // emergency release for dead-switched trigger
					beams[i].shock->trigger_switch_state = 0.0f;
				if (beams[i].shock->flags & SHOCK_FLAG_TRG_BLOCKER) // this is an enabled blocker and past boundary
				{
					for (int scount = i + 1; scount <= i + beams[i].shock->trigger_cmdshort; scount++)   // (cycle blockerbeamID +1) to (blockerbeamID + beams to lock)
					{
						if (beams[scount].shock && (beams[scount].shock->flags & SHOCK_FLAG_ISTRIGGER))  // don't mess anything up if the user set the number too big
						{
							if (triggerdebug && !beams[scount].shock->trigger_enabled && beams[i].shock->last_debug_state != 1)
							{
								LOG(" Trigger disabled. Blocker BeamID " + TOSTRING(i) + " enabled trigger " + TOSTRING(scount));
								beams[i].shock->last_debug_state = 1;
							}
							beams[scount].shock->trigger_enabled = false;	// disable the trigger
						}
					}
				} else if (beams[i].shock->flags & SHOCK_FLAG_TRG_BLOCKER_A) // this is an enabled inverted blocker and inside boundary
				{
					for (int scount = i + 1; scount <= i + beams[i].shock->trigger_cmdlong; scount++)   // (cycle blockerbeamID + 1) to (blockerbeamID + beams to release)
					{
						if (beams[scount].shock && (beams[scount].shock->flags & SHOCK_FLAG_ISTRIGGER))  // don't mess anything up if the user set the number too big
						{
							if (triggerdebug && beams[scount].shock->trigger_enabled && beams[i].shock->last_debug_state != 9)
							{
								LOG(" Trigger enabled. Inverted Blocker BeamID " + TOSTRING(i) + " disabled trigger " + TOSTRING(scount));
								beams[i].shock->last_debug_state = 9;
							}
							beams[scount].shock->trigger_enabled = true;	// enable the triggers
						}
					}
				} else if (beams[i].shock->flags & SHOCK_FLAG_TRG_CMD_BLOCKER) // this an enabled cmd-key-blocker and past a boundary
				{
					commandkey[beams[i].shock->trigger_cmdshort].trigger_cmdkeyblock_state = false; // Release the cmdKey
					if (triggerdebug && beams[i].shock->last_debug_state != 2)
					{
						LOG(" F-key trigger block released. Blocker BeamID " + TOSTRING(i) + " Released F" + TOSTRING(beams[i].shock->trigger_cmdshort));
						beams[i].shock->last_debug_state = 2;
					}
				} else if (beams[i].shock->flags & SHOCK_FLAG_TRG_CMD_SWITCH) // this is an enabled cmdkey switch and past a boundary
				{
					if (!beams[i].shock->trigger_switch_state)// this switch is triggered first time in this boundary
					{
						for (int scount=0; scount<free_shock; scount++)
						{
							int short1 = beams[shocks[scount].beamid].shock->trigger_cmdshort;  // cmdshort of checked trigger beam
							int short2 = beams[i].shock->trigger_cmdshort;						// cmdshort of switch beam
							int long1 = beams[shocks[scount].beamid].shock->trigger_cmdlong;	// cmdlong of checked trigger beam
							int long2 = beams[i].shock->trigger_cmdlong;						// cmdlong of switch beam
							int tmpi = beams[shocks[scount].beamid].shock->beamid;				// beamID global of checked trigger beam
							if (((short1 == short2 && long1 == long2) || (short1 == long2 && long1 == short2)) && i != tmpi)  // found both command triggers then swap if its not the switching trigger
							{
								int tmpcmdkey = beams[shocks[scount].beamid].shock->trigger_cmdlong;
								beams[shocks[scount]. beamid].shock->trigger_cmdlong = beams[shocks[scount].beamid].shock->trigger_cmdshort;
								beams[shocks[scount].beamid].shock->trigger_cmdshort = tmpcmdkey;
								beams[i].shock->trigger_switch_state = beams[i].shock->trigger_boundary_t ;  //prevent trigger switching again before leaving boundaries or timeout
								if (triggerdebug && beams[i].shock->last_debug_state != 3)
								{
									LOG(" Trigger F-key commands switched. Switch BeamID "  + TOSTRING(i)+ " switched commands of Trigger BeamID " + TOSTRING(beams[shocks[scount].beamid].shock->beamid) + " to cmdShort: F" + TOSTRING(beams[shocks[scount].beamid].shock->trigger_cmdshort) + ", cmdlong: F" + TOSTRING(beams[shocks[scount].beamid].shock->trigger_cmdlong));
									beams[i].shock->last_debug_state = 3;
								}
							}
						}
					}
				} else
				{ // just a trigger, check high/low boundary and set action
					if (difftoBeamL > beams[i].longbound*beams[i].L) // trigger past longbound
					{
						if (beams[i].shock->flags & SHOCK_FLAG_TRG_HOOK_UNLOCK)
						{
							if (update)
							{
								//autolock hooktoggle unlock
								hookToggle(beams[i].shock->trigger_cmdlong, HOOK_UNLOCK, -1);
							}
						} else if (beams[i].shock->flags & SHOCK_FLAG_TRG_HOOK_LOCK)
						{
							if (update)
							{
								//autolock hooktoggle lock
								hookToggle(beams[i].shock->trigger_cmdlong, HOOK_LOCK, -1);
							}
						} else if(beams[i].shock->flags & SHOCK_FLAG_TRG_ENGINE)
						{
							engineTriggerHelper(beams[i].shock->trigger_cmdshort, beams[i].shock->trigger_cmdlong, 1.0f);
						} else
						{
							//just a trigger
							if (!commandkey[beams[i].shock->trigger_cmdlong].trigger_cmdkeyblock_state)	// related cmdkey is not blocked
							{
								if (beams[i].shock->flags & SHOCK_FLAG_TRG_CONTINUOUS)
									commandkey[beams[i].shock->trigger_cmdshort].triggerInputValue = 1; // continuous trigger only operates on trigger_cmdshort
								else
									commandkey[beams[i].shock->trigger_cmdlong].triggerInputValue = 1;
								if (triggerdebug && beams[i].shock->last_debug_state != 4)
								{
									LOG(" Trigger Longbound activated. Trigger BeamID " + TOSTRING(i) + " Triggered F" + TOSTRING(beams[i].shock->trigger_cmdlong));
									beams[i].shock->last_debug_state = 4;
								}
							}
						}
					} else // trigger past short bound
					{
						if (beams[i].shock->flags & SHOCK_FLAG_TRG_HOOK_UNLOCK)
						{
							if (update)
							{
								//autolock hooktoggle unlock
								hookToggle(beams[i].shock->trigger_cmdshort, HOOK_UNLOCK, -1);
							}
						} else if (beams[i].shock->flags & SHOCK_FLAG_TRG_HOOK_LOCK)
						{
							if (update)
							{
								//autolock hooktoggle lock
								hookToggle(beams[i].shock->trigger_cmdshort, HOOK_LOCK, -1);
							}
						} else if(beams[i].shock->flags & SHOCK_FLAG_TRG_ENGINE)
						{
							bool triggerValue = !(beams[i].shock->flags & SHOCK_FLAG_TRG_CONTINUOUS); // 0 if trigger is continuous, 1 otherwise

							engineTriggerHelper(beams[i].shock->trigger_cmdshort, beams[i].shock->trigger_cmdlong, triggerValue);
						} else
						{
							//just a trigger
							if (!commandkey[beams[i].shock->trigger_cmdshort].trigger_cmdkeyblock_state) // related cmdkey is not blocked
							{
								if (beams[i].shock->flags & SHOCK_FLAG_TRG_CONTINUOUS)
									commandkey[beams[i].shock->trigger_cmdshort].triggerInputValue = 0; // continuous trigger only operates on trigger_cmdshort
								else
									commandkey[beams[i].shock->trigger_cmdshort].triggerInputValue = 1;

								if (triggerdebug  && beams[i].shock->last_debug_state != 5)
								{
									LOG(" Trigger Shortbound activated. Trigger BeamID " + TOSTRING(i) + " Triggered F" + TOSTRING(beams[i].shock->trigger_cmdshort));
									beams[i].shock->last_debug_state = 5;
								}
							}
						}
					}
				}
			} else // this is a trigger inside boundaries and its enabled
			{
				if (beams[i].shock->flags & SHOCK_FLAG_TRG_CONTINUOUS) // this is an enabled continuous trigger
				{					
					if (beams[i].longbound - beams[i].shortbound > 0.0f)
					{
						float diffPercentage = difftoBeamL / beams[i].L;
						float triggerValue = (diffPercentage - beams[i].shortbound) / (beams[i].longbound - beams[i].shortbound);

						triggerValue = std::max(0.0f, triggerValue);
						triggerValue = std::min(triggerValue, 1.0f);

						if (beams[i].shock->flags & SHOCK_FLAG_TRG_ENGINE) // this trigger controls an engine
						{
							engineTriggerHelper(beams[i].shock->trigger_cmdshort, beams[i].shock->trigger_cmdlong, triggerValue);
						} else
						{
							// normal trigger
							commandkey[beams[i].shock->trigger_cmdshort].triggerInputValue = triggerValue;
							commandkey[beams[i].shock->trigger_cmdlong].triggerInputValue = triggerValue;
						}
					}
				} else if (beams[i].shock->flags & SHOCK_FLAG_TRG_BLOCKER) // this is an enabled blocker and inside boundary
				{
					for (int scount = i + 1; scount <= i + beams[i].shock->trigger_cmdlong; scount++)   // (cycle blockerbeamID + 1) to (blockerbeamID + beams to release)
					{
						if (beams[scount].shock && (beams[scount].shock->flags & SHOCK_FLAG_ISTRIGGER))  // don't mess anything up if the user set the number too big
						{
							if (triggerdebug && beams[scount].shock->trigger_enabled && beams[i].shock->last_debug_state != 6)
							{
								LOG(" Trigger enabled. Blocker BeamID " + TOSTRING(i) + " disabled trigger " + TOSTRING(scount));
								beams[i].shock->last_debug_state = 6;
							}
							beams[scount].shock->trigger_enabled = true;	// enable the triggers
						}
					}
				} else if (beams[i].shock->flags & SHOCK_FLAG_TRG_BLOCKER_A) // this is an enabled reverse blocker and past boundary
				{
					for (int scount = i + 1; scount <= i + beams[i].shock->trigger_cmdshort; scount++)   // (cylce blockerbeamID +1) to (blockerbeamID + beams tob lock)
					{
						if (beams[scount].shock && (beams[scount].shock->flags & SHOCK_FLAG_ISTRIGGER))  // dont mess anything up if the user set the number too big
						{
							if (triggerdebug && !beams[scount].shock->trigger_enabled && beams[i].shock->last_debug_state != 10)
							{
								LOG(" Trigger disabled. Inverted Blocker BeamID " + TOSTRING(i) + " enabled trigger " + TOSTRING(scount));
								beams[i].shock->last_debug_state = 10;
							}
							beams[scount].shock->trigger_enabled = false;	// disable the trigger
						}
					}
				} else if ((beams[i].shock->flags & SHOCK_FLAG_TRG_CMD_SWITCH) && beams[i].shock->trigger_switch_state) // this is a switch that was activated and is back inside boundaries again
				{
					beams[i].shock->trigger_switch_state = 0.0f;  //trigger_switch reset
					if (triggerdebug && beams[i].shock->last_debug_state != 7)
					{
						LOG(" Trigger switch reset. Switch BeamID " + TOSTRING(i));
						beams[i].shock->last_debug_state = 7;
					}
				} else if ((beams[i].shock->flags & SHOCK_FLAG_TRG_CMD_BLOCKER) && !commandkey[beams[i].shock->trigger_cmdshort].trigger_cmdkeyblock_state) // this cmdkeyblocker is inside boundaries and cmdkeystate is diabled
				{
					commandkey[beams[i].shock->trigger_cmdshort].trigger_cmdkeyblock_state = true; // activate trigger blocking
					if (triggerdebug && beams[i].shock->last_debug_state != 8)
					{
						LOG(" F-key trigger blocked. Blocker BeamID " + TOSTRING(i) + " Blocked F" + TOSTRING(beams[i].shock->trigger_cmdshort));
						beams[i].shock->last_debug_state = 8;
					}
				}
			}
		}
	}
	// save beam position for next simulation cycle
	beams[i].shock->lastpos = difftoBeamL;
}

// truck a - truck b collisions
void Beam::interTruckCollisionsPrepare(Real dt)
{
	Beam** trucks = BeamFactory::getSingleton().getTrucks();
	int numtrucks = BeamFactory::getSingleton().getTruckCount();

	for (unsigned int i=0; i<interPointCD.size(); i++)
	{
		interPointCD[i]->update(trucks, numtrucks);
	}
}

void Beam::interTruckCollisionsCompute(Real dt, int chunk_index /*= 0*/, int chunk_number /*= 1*/)
{
	Beam** trucks = BeamFactory::getSingleton().getTrucks();
	int numtrucks = BeamFactory::getSingleton().getTruckCount();

	float inverted_dt = 1.0f / dt;

	Beam* hittruck;
	Matrix3 forward;
	Vector3 bx;
	Vector3 by;
	Vector3 bz;
	Vector3 forcevec;
	Vector3 plnormal;
	Vector3 point;
	Vector3 vecrelVel;
	float trwidth;
	int hitnodeid;
	int hittruckid;
	int tmpv;
	node_t* hitnode;
	node_t* na;
	node_t* nb;
	node_t* no;

	for (int t=0; t<numtrucks; t++)
	{
		//If you change any of the below "ifs" concerning trucks then you should
		//also consider changing the parallel "ifs" inside PointColDetector
		//see "pointCD" above.
		//Performance some times forces ugly architectural designs....
		if (!trucks[t] || !trucks[t]->collisionRelevant || trucks[t]->state >= SLEEPING) continue;

		trwidth=trucks[t]->collrange;

		int chunk_size = trucks[t]->free_collcab / chunk_number;
		int end_index = (chunk_index+1)*chunk_size;

		if (chunk_index+1 == chunk_number)
		{
			end_index = trucks[t]->free_collcab;
		}

		for (int i=chunk_index*chunk_size; i<end_index; i++)
		{
			trucks[t]->inter_collcabrate[i].update=true;
			if (trucks[t]->inter_collcabrate[i].rate>0)
			{
				trucks[t]->inter_collcabrate[i].rate--;
				trucks[t]->inter_collcabrate[i].update=false;
				continue;
			}

			tmpv=trucks[t]->collcabs[i]*3;
			no=&trucks[t]->nodes[trucks[t]->cabs[tmpv]];
			na=&trucks[t]->nodes[trucks[t]->cabs[tmpv+1]];
			nb=&trucks[t]->nodes[trucks[t]->cabs[tmpv+2]];

			int distance = trucks[t]->inter_collcabrate[i].distance + std::min(12.0f * no->Velocity.length() / 55.5f, 12.0f);
			distance = std::max(1, distance);

			interPointCD[chunk_index]->query(no->AbsPosition
				, na->AbsPosition
				, nb->AbsPosition, trwidth*distance);

			trucks[t]->inter_collcabrate[i].calcforward=true;
			for (int h=0; h<interPointCD[chunk_index]->hit_count; h++)
			{
				hitnodeid=interPointCD[chunk_index]->hit_list[h]->nodeid;
				hittruckid=interPointCD[chunk_index]->hit_list[h]->truckid;
				hitnode=&trucks[hittruckid]->nodes[hitnodeid];

				//ignore self-contact here
				if (hittruckid==t) continue;

				hittruck=trucks[hittruckid];

				//calculate transform matrices
				if (trucks[t]->inter_collcabrate[i].calcforward)
				{
					trucks[t]->inter_collcabrate[i].calcforward=false;
					bx=na->RelPosition;
					by=nb->RelPosition;
					bx-=no->RelPosition;
					by-=no->RelPosition;
					bz=bx.crossProduct(by);
					bz=fast_normalise(bz);
					//coordinates change matrix
					forward.SetColumn(0, bx);
					forward.SetColumn(1, by);
					forward.SetColumn(2, bz);
					forward=forward.Inverse();
				}

				//change coordinates
				point=forward*(hitnode->AbsPosition-no->AbsPosition);

				//test
				if (point.x>=0 && point.y>=0 && (point.x+point.y)<=1.0 && point.z<=trwidth && point.z>=-trwidth)
				{
					//collision
					plnormal=bz;

					//some more accuracy for the normal
					plnormal.normalise();

					float penetration=0.0f;

					//Find which side most of the connected nodes (through beams) are
					if (hittruck->nodetonodeconnections[hitnodeid].size()>3)
					{
						//float sumofdistances=0.0f;
						int posside=0;
						int negside=0;
						float tmppz=point.z;
						float distance;

						for (unsigned int ni=0;ni<hittruck->nodetonodeconnections[hitnodeid].size();ni++)
						{
							distance=plnormal.dotProduct(hittruck->nodes[hittruck->nodetonodeconnections[hitnodeid][ni]].AbsPosition-no->AbsPosition);
							if (distance>=0) posside++; else negside++;
						}

						//Current hitpoint's position has triple the weight
						if (point.z>=0) posside+=3;
						else negside+=3;

						if (negside>posside)
						{
							plnormal=-plnormal;
							tmppz=-tmppz;
						}

						penetration=(trwidth-tmppz);
					} else
					{
						//If we are on the other side of the triangle invert the triangle's normal
						if (point.z<0) plnormal=-plnormal;
						penetration=(trwidth-fabs(point.z));
					}

					//Find the point's velocity relative to the triangle
					vecrelVel=(hitnode->Velocity-
						(no->Velocity*(-point.x-point.y+1.0f)+na->Velocity*point.x
						+nb->Velocity*point.y));

					//Find the velocity perpendicular to the triangle
					float velForce=vecrelVel.dotProduct(plnormal);
					//if it points away from the triangle the ignore it (set it to 0)
					if (velForce<0.0f) velForce=-velForce;
					else velForce=0.0f;

					//Velocity impulse
					float vi=hitnode->mass*inverted_dt*(velForce+inverted_dt*penetration)*0.5f;

					//MUTEX_LOCK(&itc_node_access_mutex);
					//The force that the triangle puts on the point
					float trfnormal=(no->Forces*(-point.x-point.y+1.0f)+na->Forces*point.x
						+nb->Forces*point.y).dotProduct(plnormal);
					//(applied only when it is towards the point)
					if (trfnormal<0.0f) trfnormal=0.0f;

					//The force that the point puts on the triangle
					
					float pfnormal=hitnode->Forces.dotProduct(plnormal);
					//(applied only when it is towards the triangle)
					if (pfnormal>0.0f) pfnormal=0.0f;

					float fl=(vi+trfnormal-pfnormal)*0.5f;

					forcevec=Vector3::ZERO;
					float nso;

					//Calculate the collision forces
					gEnv->collisions->primitiveCollision(hitnode, forcevec, vecrelVel, plnormal, ((float) dt), trucks[t]->submesh_ground_model, &nso, penetration, fl);

					hitnode->Forces+=forcevec;
					// no network special case for now
					//if (trucks[t]->state==NETWORKED)

					no->Forces-=(-point.x-point.y+1.0f)*forcevec;
					na->Forces-=(point.x)*forcevec;
					nb->Forces-=(point.y)*forcevec;
					//MUTEX_UNLOCK(&itc_node_access_mutex);
				}
			}
		}
	}
}

void Beam::interTruckCollisionsFinal(Real dt)
{
	Beam** trucks = BeamFactory::getSingleton().getTrucks();
	int numtrucks = BeamFactory::getSingleton().getTruckCount();

	for (int t=0; t<numtrucks; t++)
	{
		if (!trucks[t] || !trucks[t]->collisionRelevant || trucks[t]->state >= SLEEPING) continue;

		for (int i=0; i<trucks[t]->free_collcab; i++)
		{
			if (!trucks[t]->inter_collcabrate[i].update) continue;
			if (trucks[t]->inter_collcabrate[i].calcforward)
			{
				trucks[t]->inter_collcabrate[i].rate = trucks[t]->inter_collcabrate[i].distance - 1;
				if (trucks[t]->inter_collcabrate[i].distance < 13)
				{
					trucks[t]->inter_collcabrate[i].distance++;
				}
			} else
			{
				trucks[t]->inter_collcabrate[i].distance /= 2;
				trucks[t]->inter_collcabrate[i].rate = 0;
			}
		}
	}
}

// truck a - truck a collisions
void Beam::intraTruckCollisionsPrepare(Real dt)
{
	for (unsigned int i=0; i<intraPointCD.size(); i++)
	{
		intraPointCD[i]->update(this);
	}
}

void Beam::intraTruckCollisionsCompute(Real dt, int chunk_index /*= 0*/, int chunk_number /*= 1*/)
{
	float inverted_dt = 1.0f / dt;

	Matrix3 forward;
	Vector3 bx;
	Vector3 by;
	Vector3 bz;
	Vector3 forcevec;
	Vector3 plnormal;
	Vector3 point;
	Vector3 vecrelVel;
	float trwidth;
	int hitnodeid;
	int tmpv;
	node_t* hitnode;
	node_t* na;
	node_t* nb;
	node_t* no;

	trwidth=collrange;

	int chunk_size = free_collcab / chunk_number;
	int end_index = (chunk_index+1)*chunk_size;

	if (chunk_index+1 == chunk_number)
	{
		end_index = free_collcab;
	}

	for (int i=chunk_index*chunk_size; i<end_index; i++)
	{
		intra_collcabrate[i].update=true;
		if (intra_collcabrate[i].rate>0)
		{
			intra_collcabrate[i].rate--;
			intra_collcabrate[i].update=false;
			continue;
		}

		tmpv=collcabs[i]*3;
		no=&nodes[cabs[tmpv]];
		na=&nodes[cabs[tmpv+1]];
		nb=&nodes[cabs[tmpv+2]];

		intraPointCD[chunk_index]->query(no->AbsPosition
			, na->AbsPosition
			, nb->AbsPosition, trwidth);

		bool calcforward=true;
		intra_collcabrate[i].calcforward=true;
		for (int h=0; h<intraPointCD[chunk_index]->hit_count;h++)
		{
			hitnodeid=intraPointCD[chunk_index]->hit_list[h]->nodeid;
			hitnode=&nodes[hitnodeid];

			//ignore wheel/chassis self contact
			//if (hitnode->iswheel && !(trucks[t]->requires_wheel_contact)) continue;
			if (hitnode->iswheel) continue;
			if (no==hitnode || na==hitnode || nb==hitnode) continue;

			//calculate transform matrices
			if (calcforward)
			{
				calcforward=false;
				bx=na->RelPosition;
				by=nb->RelPosition;
				bx-=no->RelPosition;
				by-=no->RelPosition;
				bz=bx.crossProduct(by);
				bz=fast_normalise(bz);
				//coordinates change matrix
				forward.SetColumn(0, bx);
				forward.SetColumn(1, by);
				forward.SetColumn(2, bz);
				forward=forward.Inverse();
			}

			//change coordinates
			point = forward * (hitnode->AbsPosition - no->AbsPosition);

			//test
			if (point.x>=0 && point.y>=0 && (point.x+point.y)<=1.0 && point.z<=trwidth && point.z>=-trwidth)
			{
				//collision
				intra_collcabrate[i].calcforward=false;
				plnormal=bz;

				//some more accuracy for the normal
				plnormal.normalise();

				float penetration=0.0f;

				if (point.z<0) plnormal=-plnormal;
				penetration=(trwidth-fabs(point.z));

				//Find the point's velocity relative to the triangle
				vecrelVel=(hitnode->Velocity-
					(no->Velocity*(-point.x-point.y+1.0f)+na->Velocity*point.x
					+nb->Velocity*point.y));

				//Find the velocity perpendicular to the triangle
				float velForce=vecrelVel.dotProduct(plnormal);
				//if it points away from the triangle the ignore it (set it to 0)
				if (velForce<0.0f) velForce=-velForce;
				else velForce=0.0f;

				//Velocity impulse
				float vi=hitnode->mass*inverted_dt*(velForce+inverted_dt*penetration)*0.5f;

				//MUTEX_LOCK(&itc_node_access_mutex);
				//The force that the triangle puts on the point
				float trfnormal=(no->Forces*(-point.x-point.y+1.0f)+na->Forces*point.x
					+nb->Forces*point.y).dotProduct(plnormal);
				//(applied only when it is towards the point)
				if (trfnormal<0.0f) trfnormal=0.0f;

				//The force that the point puts on the triangle
				float pfnormal=hitnode->Forces.dotProduct(plnormal);
				//(applied only when it is towards the triangle)
				if (pfnormal>0.0f) pfnormal=0.0f;

				float fl=(vi+trfnormal-pfnormal)*0.5f;

				forcevec=Vector3::ZERO;
				float nso;

				//Calculate the collision forces
				gEnv->collisions->primitiveCollision(hitnode, forcevec, vecrelVel, plnormal, ((float) dt), submesh_ground_model, &nso, penetration, fl);

				hitnode->Forces+=forcevec;

				// no network special case for now
				//if (trucks[t]->state==NETWORKED)

				no->Forces-=(-point.x-point.y+1.0f)*forcevec;
				na->Forces-=(point.x)*forcevec;
				nb->Forces-=(point.y)*forcevec;
				//MUTEX_UNLOCK(&itc_node_access_mutex);
			}
		}
	}
}

void Beam::intraTruckCollisionsFinal(Real dt)
{
	for (int i=0; i<free_collcab; i++)
	{
		if (!intra_collcabrate[i].update) continue;
		if (intra_collcabrate[i].calcforward)
		{
			intra_collcabrate[i].rate = intra_collcabrate[i].distance - 1;
			if (intra_collcabrate[i].distance < 13)
			{
				intra_collcabrate[i].distance++;
			}
		} else
		{
			intra_collcabrate[i].distance /= 2;
			intra_collcabrate[i].rate = 0;
		}
	}
}

// call this once per frame in order to update the skidmarks
void Beam::updateSkidmarks()
{
	if (!useSkidmarks) return;

	BES_START(BES_CORE_Skidmarks);

	for (int i=0; i<free_wheel; i++)
	{
		// ignore wheels without data
		if (wheels[i].lastContactInner == Vector3::ZERO && wheels[i].lastContactOuter == Vector3::ZERO) continue;
		// create skidmark object for wheels with data if not existing
		if (!skidtrails[i])
		{
			skidtrails[i] = new Skidmark(&wheels[i], beamsRoot, 300, 200);
		}

		skidtrails[i]->updatePoint();
	}

	//LOG("updating skidmark visuals");
	for (int i=0; i<free_wheel; i++)
	{
		if (skidtrails[i]) skidtrails[i]->update();
	}

	BES_STOP(BES_CORE_Skidmarks);
}


Quaternion Beam::specialGetRotationTo(const Vector3& src, const Vector3& dest) const
{
	// Based on Stan Melax's article in Game Programming Gems
	Quaternion q;
	// Copy, since cannot modify local
	Vector3 v0 = src;
	Vector3 v1 = dest;
	v0.normalise();
	v1.normalise();


	// NB if the crossProduct approaches zero, we get unstable because ANY axis will do
	// when v0 == -v1
	Real d = v0.dotProduct(v1);
	// If dot == 1, vectors are the same
	if (d >= 1.0f)
	{
		return Quaternion::IDENTITY;
	}
	if (d < (1e-6f - 1.0f))
	{
		// Generate an axis
		Vector3 axis = Vector3::UNIT_X.crossProduct(src);
		if (axis.isZeroLength()) // pick another if colinear
			axis = Vector3::UNIT_Y.crossProduct(src);
		axis.normalise();
		q.FromAngleAxis(Radian(Math::PI), axis);
	}
	else
	{
		Real s = fast_sqrt( (1+d)*2 );
		if (s==0) return Quaternion::IDENTITY;

		Vector3 c = v0.crossProduct(v1);
		Real invs = 1 / s;


		q.x = c.x * invs;
		q.y = c.y * invs;
		q.z = c.z * invs;
		q.w = s * 0.5;
	}
	return q;
}



void Beam::prepareInside(bool inside)
{
	isInside=inside;
	if (inside)
	{
		//going inside

		// activate cabin lights if lights are turned on
		if (lights && cablightNode && cablight)
		{
			cablightNode->setVisible(true);
			cablight->setVisible(true);
		}

		if (shadowOptimizations)
		{
			//disabling shadow
			if (cabNode && cabNode->numAttachedObjects() && cabNode->getAttachedObject(0)) ((Entity*)(cabNode->getAttachedObject(0)))->setCastShadows(false);
			int i;
			for (i=0; i<free_prop; i++)
			{
				if (props[i].snode && props[i].snode->numAttachedObjects()) props[i].snode->getAttachedObject(0)->setCastShadows(false);
				if (props[i].wheel && props[i].wheel->numAttachedObjects()) props[i].wheel->getAttachedObject(0)->setCastShadows(false);
			}
			for (i=0; i<free_wheel; i++) if (vwheels[i].cnode->numAttachedObjects()) vwheels[i].cnode->getAttachedObject(0)->setCastShadows(false);
			for (i=0; i<free_beam; i++) if (beams[i].mEntity) beams[i].mEntity->setCastShadows(false);

		}
		if (cabNode)
		{
			char transmatname[256];
			sprintf(transmatname, "%s-trans", texname);
			MaterialPtr transmat=(MaterialPtr)(MaterialManager::getSingleton().getByName(transmatname));
			transmat->setReceiveShadows(false);
		}
		//setting camera
		mCamera->setNearClipDistance( 0.1 );
		//activate mirror
		//if (mirror) mirror->setActive(true);
		//enable transparent seat
		MaterialPtr seatmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("driversseat"));
		seatmat->setDepthWriteEnabled(false);
		seatmat->setSceneBlending(SBT_TRANSPARENT_ALPHA);
	}
	else
	{
		//going outside
#ifdef USE_MYGUI
		if (dash)
			dash->setVisible(false);
#endif // USE_MYGUI

		// disable cabin light before going out
		if (cablightNode && cablight)
		{
			cablightNode->setVisible(false);
			cablight->setVisible(false);
		}

		if (shadowOptimizations)
		{
			//enabling shadow
			if (cabNode && cabNode->numAttachedObjects() && cabNode->getAttachedObject(0)) ((Entity*)(cabNode->getAttachedObject(0)))->setCastShadows(true);
			int i;
			for (i=0; i<free_prop; i++)
			{
				if (props[i].snode && props[i].snode->numAttachedObjects()) props[i].snode->getAttachedObject(0)->setCastShadows(true);
				if (props[i].wheel && props[i].wheel->numAttachedObjects()) props[i].wheel->getAttachedObject(0)->setCastShadows(true);
			}
			for (i=0; i<free_wheel; i++) if (vwheels[i].cnode->numAttachedObjects()) vwheels[i].cnode->getAttachedObject(0)->setCastShadows(true);
			for (i=0; i<free_beam; i++) if (beams[i].mEntity) beams[i].mEntity->setCastShadows(true);
		}

		if (cabNode)
		{
			char transmatname[256];
			sprintf(transmatname, "%s-trans", texname);
			MaterialPtr transmat=(MaterialPtr)(MaterialManager::getSingleton().getByName(transmatname));
			transmat->setReceiveShadows(true);
		}
		//setting camera
		mCamera->setNearClipDistance( 0.5 );
		//desactivate mirror
		//if (mirror) mirror->setActive(false);
		//disable transparent seat
		MaterialPtr seatmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("driversseat"));
		seatmat->setDepthWriteEnabled(true);
		seatmat->setSceneBlending(SBT_REPLACE);
	}
}


void Beam::lightsToggle()
{
	// no lights toggling in skeleton mode because of possible bug with emissive texture
	if (skeleton)
		return;

	Beam **trucks = BeamFactory::getSingleton().getTrucks();
	int trucksnum = BeamFactory::getSingleton().getTruckCount();

	//export light command
	if (trucks!=0 && state==ACTIVATED && forwardcommands)
	{
		for (int i=0; i<trucksnum; i++)
		{
			if (!trucks[i]) continue;
			if (trucks[i]->state==DESACTIVATED && trucks[i]->importcommands) trucks[i]->lightsToggle();
		}
	}
	lights = !lights;
	if (cablight && cablightNode && isInside)
		cablightNode->setVisible((lights!=0));
	if (!lights)
	{
		for (int i=0; i<free_flare; i++)
		{
			if (flares[i].type == 'f')
			{
				flares[i].snode->setVisible(false);
				if (flares[i].bbs) flares[i].snode->detachAllObjects();
				if (flares[i].light) flares[i].light->setVisible(false);
				flares[i].isVisible=false;
			}
		}
		if (hasEmissivePass)
		{
			char clomatname[256] = {};
			sprintf(clomatname, "%s-noem", texname);
			if (cabNode->numAttachedObjects())
			{
				Entity* ent=((Entity*)(cabNode->getAttachedObject(0)));
				int numsubent=ent->getNumSubEntities();
				for (int i=0; i<numsubent; i++)
				{
					SubEntity *subent=ent->getSubEntity(i);
					if (!strcmp((subent->getMaterialName()).c_str(), texname)) subent->setMaterialName(clomatname);
				}
				//			((Entity*)(cabNode->getAttachedObject(0)))->setMaterialName(clomatname);
			}
		}
	}
	else
	{
		for (int i=0; i<free_flare; i++)
		{
			if (flares[i].type == 'f')
			{
				if (flares[i].light) flares[i].light->setVisible(true);
				flares[i].isVisible=true;
				if (flares[i].bbs) flares[i].snode->attachObject(flares[i].bbs);
			}
		}
		if (hasEmissivePass)
		{
			char clomatname[256] = {};
			sprintf(clomatname, "%s-noem", texname);
			if (cabNode->numAttachedObjects())
			{
				Entity* ent=((Entity*)(cabNode->getAttachedObject(0)));
				int numsubent=ent->getNumSubEntities();
				for (int i=0; i<numsubent; i++)
				{
					SubEntity *subent=ent->getSubEntity(i);
					if (!strcmp((subent->getMaterialName()).c_str(), clomatname)) subent->setMaterialName(texname);
				}
				//			((Entity*)(cabNode->getAttachedObject(0)))->setMaterialName(texname);
			}
		}
	}

	TRIGGER_EVENT(SE_TRUCK_LIGHT_TOGGLE, trucknum);
}

void Beam::updateFlares(float dt, bool isCurrent)
{
	bool enableAll = true;
	if (flaresMode==0)
		return;
	if (flaresMode==2 && !isCurrent)
		enableAll=false;
	BES_GFX_START(BES_GFX_updateFlares);
	int i;
	//okay, this is just ugly, we have flares in props!
	//we have to update them here because they run
	if (beacon)
	{
		for (i=0; i<free_prop; i++)
		{
			if (props[i].beacontype=='b')
			{
				//update light
				Quaternion orientation=props[i].snode->getOrientation();
				props[i].light[0]->setPosition(props[i].snode->getPosition()+orientation*Vector3(0,0,0.12));
				props[i].bpos[0]+=dt*props[i].brate[0];//rotate baby!
				props[i].light[0]->setDirection(orientation*Vector3(cos(props[i].bpos[0]),sin(props[i].bpos[0]),0));
				//billboard
				Vector3 vdir=props[i].light[0]->getPosition()-mCamera->getPosition();
				float vlen=vdir.length();
				if (vlen>100.0) {props[i].bbsnode[0]->setVisible(false);continue;}
				//normalize
				vdir=vdir/vlen;
				props[i].bbsnode[0]->setPosition(props[i].light[0]->getPosition()-vdir*0.1);
				float amplitude=props[i].light[0]->getDirection().dotProduct(vdir);
				if (amplitude>0)
				{
					props[i].bbsnode[0]->setVisible(true);
					props[i].bbs[0]->setDefaultDimensions(amplitude*amplitude*amplitude, amplitude*amplitude*amplitude);
				}
				else
				{
					props[i].bbsnode[0]->setVisible(false);
				}
				props[i].light[0]->setVisible(enableAll);
			}
			if (props[i].beacontype=='p')
			{
				int k;
				for (k=0; k<4; k++)
				{
					//update light
					Quaternion orientation=props[i].snode->getOrientation();
					switch (k)
					{
					case 0: props[i].light[k]->setPosition(props[i].snode->getPosition()+orientation*Vector3(-0.64,0,0.14));break;
					case 1: props[i].light[k]->setPosition(props[i].snode->getPosition()+orientation*Vector3(-0.32,0,0.14));break;
					case 2: props[i].light[k]->setPosition(props[i].snode->getPosition()+orientation*Vector3(+0.32,0,0.14));break;
					case 3: props[i].light[k]->setPosition(props[i].snode->getPosition()+orientation*Vector3(+0.64,0,0.14));break;
					}
					props[i].bpos[k]+=dt*props[i].brate[k];//rotate baby!
					props[i].light[k]->setDirection(orientation*Vector3(cos(props[i].bpos[k]),sin(props[i].bpos[k]),0));
					//billboard
					Vector3 vdir=props[i].light[k]->getPosition()-mCamera->getPosition();
					float vlen=vdir.length();
					if (vlen>100.0) {props[i].bbsnode[k]->setVisible(false);continue;}
					//normalize
					vdir=vdir/vlen;
					props[i].bbsnode[k]->setPosition(props[i].light[k]->getPosition()-vdir*0.2);
					float amplitude=props[i].light[k]->getDirection().dotProduct(vdir);
					if (amplitude>0)
					{
						props[i].bbsnode[k]->setVisible(true);
						props[i].bbs[k]->setDefaultDimensions(amplitude*amplitude*amplitude, amplitude*amplitude*amplitude);
					}
					else
					{
						props[i].bbsnode[k]->setVisible(false);
					}
					props[i].light[k]->setVisible(enableAll);
				}
			}
			if (props[i].beacontype=='r')
			{
				//update light
				Quaternion orientation=props[i].snode->getOrientation();
				props[i].light[0]->setPosition(props[i].snode->getPosition()+orientation*Vector3(0,0,0.06));
				props[i].bpos[0]+=dt*props[i].brate[0];//rotate baby!
				//billboard
				Vector3 vdir=props[i].light[0]->getPosition()-mCamera->getPosition();
				float vlen=vdir.length();
				if (vlen>100.0) {props[i].bbsnode[0]->setVisible(false);continue;}
				//normalize
				vdir=vdir/vlen;
				props[i].bbsnode[0]->setPosition(props[i].light[0]->getPosition()-vdir*0.1);
				bool visible=false;
				if (props[i].bpos[0]>1.0)
				{
					props[i].bpos[0]=0.0;
					visible=true;
				}
				visible = visible && enableAll;
				props[i].light[0]->setVisible(visible);
				props[i].bbsnode[0]->setVisible(visible);

			}
			if (props[i].beacontype=='R' || props[i].beacontype=='L')
			{
				Vector3 mposition=nodes[props[i].noderef].smoothpos+props[i].offsetx*(nodes[props[i].nodex].smoothpos-nodes[props[i].noderef].smoothpos)+props[i].offsety*(nodes[props[i].nodey].smoothpos-nodes[props[i].noderef].smoothpos);
				//billboard
				Vector3 vdir=mposition-mCamera->getPosition();
				float vlen=vdir.length();
				if (vlen>100.0) {props[i].bbsnode[0]->setVisible(false);continue;}
				//normalize
				vdir=vdir/vlen;
				props[i].bbsnode[0]->setPosition(mposition-vdir*0.1);
			}
			if (props[i].beacontype=='w')
			{
				Vector3 mposition=nodes[props[i].noderef].smoothpos+props[i].offsetx*(nodes[props[i].nodex].smoothpos-nodes[props[i].noderef].smoothpos)+props[i].offsety*(nodes[props[i].nodey].smoothpos-nodes[props[i].noderef].smoothpos);
				props[i].light[0]->setPosition(mposition);
				props[i].bpos[0]+=dt*props[i].brate[0];//rotate baby!
				//billboard
				Vector3 vdir=mposition-mCamera->getPosition();
				float vlen=vdir.length();
				if (vlen>100.0) {props[i].bbsnode[0]->setVisible(false);continue;}
				//normalize
				vdir=vdir/vlen;
				props[i].bbsnode[0]->setPosition(mposition-vdir*0.1);
				bool visible=false;
				if (props[i].bpos[0]>1.0)
				{
					props[i].bpos[0]=0.0;
					visible=true;
				}
				visible = visible && enableAll;
				props[i].light[0]->setVisible(visible);
				props[i].bbsnode[0]->setVisible(visible);
			}
		}
	}
	//the flares
	bool keysleep=false;
	for (i=0; i<free_flare; i++)
	{
		// let the light blink
		if (flares[i].blinkdelay != 0)
		{
			flares[i].blinkdelay_curr -= dt;
			if (flares[i].blinkdelay_curr <= 0)
			{
				flares[i].blinkdelay_curr = flares[i].blinkdelay;
				flares[i].blinkdelay_state = !flares[i].blinkdelay_state;
			}
		}
		else
		{
			flares[i].blinkdelay_state = true;
		}
		//LOG(TOSTRING(flares[i].blinkdelay_curr));
		// manage light states
		bool isvisible = true; //this must be true to be able to switch on the frontlight
		if (flares[i].type == 'f') {
			materialFunctionMapper->toggleFunction(i, (lights==1));
			if (!lights)
				continue;
		} else if (flares[i].type == 'b') {
			isvisible = getBrakeLightVisible();
		} else if (flares[i].type == 'R') {
			if (engine || reverselight)
				isvisible = getReverseLightVisible();
			else
				isvisible=false;
		} else if (flares[i].type == 'u' && flares[i].controlnumber != -1) {
			if (state==ACTIVATED) // no network!!
			{
				// networked customs are set directly, so skip this
				if (INPUTENGINE.getEventBoolValue(EV_TRUCK_LIGHTTOGGLE01 + (flares[i].controlnumber - 1)) && mTimeUntilNextToggle <= 0)
				{
					flares[i].controltoggle_status = ! flares[i].controltoggle_status;
					keysleep = true;
				}
			}
			isvisible = flares[i].controltoggle_status;

		} else if (flares[i].type == 'l') {
			isvisible = (blinkingtype == BLINK_LEFT || blinkingtype == BLINK_WARN);
		} else if (flares[i].type == 'r') {
			isvisible = (blinkingtype == BLINK_RIGHT || blinkingtype == BLINK_WARN);
		}
		// apply blinking
		isvisible = isvisible && flares[i].blinkdelay_state;

		if (flares[i].type == 'l' && blinkingtype == BLINK_LEFT)
		{
			left_blink_on  = isvisible;
#ifdef USE_OPENAL
			if (left_blink_on)
				SoundScriptManager::getSingleton().trigOnce(trucknum, SS_TRIG_TURN_SIGNAL_TICK);
#endif //USE_OPENAL
		} else if (flares[i].type == 'r' && blinkingtype == BLINK_RIGHT)
		{
			right_blink_on = isvisible;
#ifdef USE_OPENAL
			if (right_blink_on)
				SoundScriptManager::getSingleton().trigOnce(trucknum, SS_TRIG_TURN_SIGNAL_TICK);
#endif //USE_OPENAL
		} else if (flares[i].type == 'l' && blinkingtype == BLINK_WARN)
		{
			warn_blink_on  = isvisible;
#ifdef USE_OPENAL
			if (warn_blink_on)
				SoundScriptManager::getSingleton().trigOnce(trucknum, SS_TRIG_TURN_SIGNAL_WARN_TICK);
#endif //USE_OPENAL
		}


		//left_blink_on, right_blink_on, warn_blink_on;
		// update material Bindings
		materialFunctionMapper->toggleFunction(i, isvisible);

		flares[i].snode->setVisible(isvisible);
		if (flares[i].light)
			flares[i].light->setVisible(isvisible && enableAll);
		flares[i].isVisible=isvisible;

		Vector3 normal=(nodes[flares[i].nodey].smoothpos-nodes[flares[i].noderef].smoothpos).crossProduct(nodes[flares[i].nodex].smoothpos-nodes[flares[i].noderef].smoothpos);
		normal.normalise();
		Vector3 mposition=nodes[flares[i].noderef].smoothpos+flares[i].offsetx*(nodes[flares[i].nodex].smoothpos-nodes[flares[i].noderef].smoothpos)+flares[i].offsety*(nodes[flares[i].nodey].smoothpos-nodes[flares[i].noderef].smoothpos);
		Vector3 vdir=mposition-mCamera->getPosition();
		float vlen=vdir.length();
		// not visible from 500m distance
		if (vlen > 500.0)
		{
			flares[i].snode->setVisible(false);
			continue;
		}
		//normalize
		vdir=vdir/vlen;
		float amplitude=normal.dotProduct(vdir);
		flares[i].snode->setPosition(mposition-0.1*amplitude*normal*flares[i].offsetz);
		flares[i].snode->setDirection(normal);
		float fsize = flares[i].size;
		if (fsize < 0)
		{
			amplitude=1;
			fsize*=-1;
		}
		if (flares[i].light)
		{
			flares[i].light->setPosition(mposition-0.2*amplitude*normal);
			// point the real light towards the ground a bit
			flares[i].light->setDirection(-normal - Vector3(0, 0.2, 0));
		}
		if (flares[i].isVisible)
		{
			if (amplitude>0)
			{
				flares[i].bbs->setDefaultDimensions(amplitude * fsize, amplitude * fsize);
				flares[i].snode->setVisible(true);
			}
			else
			{
				flares[i].snode->setVisible(false);
			}
		}
		//flares[i].bbs->_updateBounds();
	}
	if (keysleep)
		mTimeUntilNextToggle = 0.2;
	BES_GFX_STOP(BES_GFX_updateFlares);

}

void Beam::setBlinkType(blinktype blink)
{
	blinkingtype = blink;

	left_blink_on = false;
	right_blink_on = false;
	warn_blink_on = false;

#ifdef USE_OPENAL
	if (blink == BLINK_NONE)
	{
		SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_TURN_SIGNAL);
	} else
	{
		SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_TURN_SIGNAL);
	}
#endif //OPENAL
}

void Beam::autoBlinkReset()
{
	blinktype blink = getBlinkType();

	// TODO: make this set-able per truck
	float blink_lock_range = 0.1f;

	if (blink == BLINK_LEFT && hydrodirstate < -blink_lock_range)
		// passed the threshold: the turn signal gets locked
		blinktreshpassed = true;

	if (blink == BLINK_LEFT && blinktreshpassed && hydrodirstate > -blink_lock_range)
	{
		// steering wheel turned back: turn signal gets automatically unlocked
		setBlinkType(BLINK_NONE);
		blinktreshpassed = false;
	}

	// same for the right turn signal
	if (blink == BLINK_RIGHT && hydrodirstate > blink_lock_range)
		blinktreshpassed = true;

	if (blink == BLINK_RIGHT && blinktreshpassed && hydrodirstate < blink_lock_range)
	{
		setBlinkType(BLINK_NONE);
		blinktreshpassed = false;
	}
}

void Beam::updateProps()
{
	BES_GFX_START(BES_GFX_updateProps);
	int i;
	//the props
	for (i=0; i<free_prop; i++)
	{
		if (!props[i].snode) continue;
		Vector3 normal=(nodes[props[i].nodey].smoothpos-nodes[props[i].noderef].smoothpos).crossProduct(nodes[props[i].nodex].smoothpos-nodes[props[i].noderef].smoothpos);
		normal.normalise();
		//position
		Vector3 mposition=nodes[props[i].noderef].smoothpos+props[i].offsetx*(nodes[props[i].nodex].smoothpos-nodes[props[i].noderef].smoothpos)+props[i].offsety*(nodes[props[i].nodey].smoothpos-nodes[props[i].noderef].smoothpos);
		props[i].snode->setPosition(mposition+normal*props[i].offsetz);
		//orientation
		Vector3 refx=nodes[props[i].nodex].smoothpos-nodes[props[i].noderef].smoothpos;
		refx.normalise();
		Vector3 refy=refx.crossProduct(normal);
		Quaternion orientation=Quaternion(refx, normal, refy)*props[i].rot;
		props[i].snode->setOrientation(orientation);
		if (props[i].wheel)
		{
			//display wheel
			Quaternion brot=Quaternion(Degree(-59.0), Vector3::UNIT_X);
			brot=brot*Quaternion(Degree(hydrodirwheeldisplay*props[i].wheelrotdegree), Vector3::UNIT_Y);
			props[i].wheel->setPosition(mposition+normal*props[i].offsetz+orientation*props[i].wheelpos);
			props[i].wheel->setOrientation(orientation*brot);
		}
	}
	//we also consider airbrakes as props
	for (i=0; i<free_airbrake; i++) airbrakes[i]->updatePosition((float)airbrakeval/5.0);
	BES_GFX_STOP(BES_GFX_updateProps);
}

void Beam::toggleCustomParticles()
{
	cparticle_mode = !cparticle_mode;
	for (int i=0; i<free_cparticle; i++)
	{
		cparticles[i].active = !cparticles[i].active;
		for (int j=0; j<cparticles[i].psys->getNumEmitters(); j++)
		{
			cparticles[i].psys->getEmitter(j)->setEnabled(cparticles[i].active);
		}
	}

	//ScriptEvent - Particle Toggle
	TRIGGER_EVENT(SE_TRUCK_CPARTICLES_TOGGLE, trucknum);
}

void Beam::updateSoundSources()
{
	BES_GFX_START(BES_GFX_updateSoundSources);
#ifdef USE_OPENAL
	if (SoundScriptManager::getSingleton().isDisabled()) return;
	for (int i=0; i<free_soundsource; i++)
	{
		soundsources[i].ssi->setPosition(nodes[soundsources[i].nodenum].AbsPosition, nodes[soundsources[i].nodenum].Velocity);
	}
	//also this, so it is updated always, and for any vehicle
	SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_AIRSPEED, nodes[0].Velocity.length()*1.9438);
	SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_WHEELSPEED, WheelSpeed*3.6);
#endif //OPENAL
	BES_GFX_STOP(BES_GFX_updateSoundSources);
}

void Beam::updateLabels(float dt)
{
	if (netLabelNode && netMT && netMT->isVisible())
	{
		// this ensures that the nickname is always in a readable size
		netLabelNode->setPosition(position + Vector3(0.0f, (boundingBox.getMaximum().y - boundingBox.getMinimum().y), 0.0f));
		Vector3 vdir = position - mCamera->getPosition();
		float vlen = vdir.length();
		float h = std::max(0.6, vlen / 30.0);

		netMT->setCharacterHeight(h);
		if (vlen > 1000) // 1000 ... vlen
			netMT->setCaption(networkUsername + "  (" + TOSTRING((float)(ceil(vlen / 100) / 10.0) ) + " km)");
		else if (vlen > 20) // 20 ... vlen ... 1000
			netMT->setCaption(networkUsername + "  (" + TOSTRING((int)vlen) + " m)");
		else // 0 ... vlen ... 20
			netMT->setCaption(networkUsername);

		//netMT->setAdditionalHeight((boundingBox.getMaximum().y - boundingBox.getMinimum().y) + h + 0.1);
		netMT->setVisible(true);
	}
}

void Beam::updateVisualPrepare(float dt)
{
	BES_GFX_START(BES_GFX_updateVisual);

	Vector3 ref(Vector3::UNIT_Y);
	autoBlinkReset();
	//sounds too
	updateSoundSources();

	if (deleting) return;
	if (debugVisuals) updateDebugOverlay();

	//dust
	// UH - design problem, it updates every truck dust D:
	DustManager::getSingleton().update(WheelSpeed);

#ifdef USE_OPENAL
	//airplane radio chatter
	if (driveable == AIRPLANE && state != SLEEPING)
	{
		// play random chatter at random time
		avichatter_timer -= dt;
		if (avichatter_timer < 0)
		{
			SoundScriptManager::getSingleton().trigOnce(trucknum, SS_TRIG_AVICHAT01 + Math::RangeRandom(0, 12));
			avichatter_timer = Math::RangeRandom(11,30);
		}
	}
#endif //openAL

	//update custom particle systems
	for (int i=0; i<free_cparticle; i++)
	{
		Vector3 pos=nodes[cparticles[i].emitterNode].smoothpos;
		Vector3 dir=pos-nodes[cparticles[i].directionNode].smoothpos;
		//dir.normalise();
		dir=fast_normalise(dir);
		cparticles[i].snode->setPosition(pos);
		for (int j=0; j<cparticles[i].psys->getNumEmitters(); j++)
		{
			cparticles[i].psys->getEmitter(j)->setDirection(dir);
		}
	}
	// update exhausts
	if (!disable_smoke && engine && exhausts.size() > 0)
	{
		std::vector < exhaust_t >::iterator it;
		for (it=exhausts.begin(); it!=exhausts.end(); it++)
		{
			if (!it->smoker)
				continue;
			Vector3 dir=nodes[it->emitterNode].smoothpos-nodes[it->directionNode].smoothpos;
			//			dir.normalise();
			ParticleEmitter *emit = it->smoker->getEmitter(0);
			it->smokeNode->setPosition(nodes[it->emitterNode].smoothpos);
			emit->setDirection(dir);
			if (engine->getSmoke()!=-1.0)
			{
				emit->setEnabled(true);
				emit->setColour(ColourValue(0.0,0.0,0.0,0.02+engine->getSmoke()*0.06));
				emit->setTimeToLive((0.02+engine->getSmoke()*0.06)/0.04);
			} else
			{
				emit->setEnabled(false);
			}
			emit->setParticleVelocity(1.0+engine->getSmoke()*2.0, 2.0+engine->getSmoke()*3.0);
		}
	}

	updateProps();

	for (int i=0; i<free_aeroengine; i++) aeroengines[i]->updateVisuals();

	//wings
	float autoaileron=0;
	float autorudder=0;
	float autoelevator=0;
	if (autopilot)
	{
		autoaileron=autopilot->getAilerons();
		autorudder=autopilot->getRudder();
		autoelevator=autopilot->getElevator();
		autopilot->gpws_update(posnode_spawn_height);
	}
	autoaileron+=aileron;
	autorudder+=rudder;
	autoelevator+=elevator;
	if (autoaileron<-1.0) autoaileron=-1.0;
	if (autoaileron>1.0) autoaileron=1.0;
	if (autorudder<-1.0) autorudder=-1.0;
	if (autorudder>1.0) autorudder=1.0;
	if (autoelevator<-1.0) autoelevator=-1.0;
	if (autoelevator>1.0) autoelevator=1.0;
	for (int i=0; i<free_wing; i++)
	{
		if (wings[i].fa->type=='a') wings[i].fa->setControlDeflection(autoaileron);
		if (wings[i].fa->type=='b') wings[i].fa->setControlDeflection(-autoaileron);
		if (wings[i].fa->type=='r') wings[i].fa->setControlDeflection(autorudder);
		if (wings[i].fa->type=='e' || wings[i].fa->type=='S' || wings[i].fa->type=='T') wings[i].fa->setControlDeflection(autoelevator);
		if (wings[i].fa->type=='f') wings[i].fa->setControlDeflection(flapangles[flap]);
		if (wings[i].fa->type=='c' || wings[i].fa->type=='V') wings[i].fa->setControlDeflection((autoaileron+autoelevator)/2.0);
		if (wings[i].fa->type=='d' || wings[i].fa->type=='U') wings[i].fa->setControlDeflection((-autoaileron+autoelevator)/2.0);
		if (wings[i].fa->type=='g') wings[i].fa->setControlDeflection((autoaileron+flapangles[flap])/2.0);
		if (wings[i].fa->type=='h') wings[i].fa->setControlDeflection((-autoaileron+flapangles[flap])/2.0);
		if (wings[i].fa->type=='i') wings[i].fa->setControlDeflection((-autoelevator+autorudder)/2.0);
		if (wings[i].fa->type=='j') wings[i].fa->setControlDeflection((autoelevator+autorudder)/2.0);
		wings[i].cnode->setPosition(wings[i].fa->flexit());
	}
	//setup commands for hydros
	hydroaileroncommand=autoaileron;
	hydroruddercommand=autorudder;
	hydroelevatorcommand=autoelevator;

	if (cabFadeMode>0 && dt > 0)
	{
		if (cabFadeTimer > 0)
			cabFadeTimer-=dt;
		if (cabFadeTimer < 0.1 && cabFadeMode == 1)
		{
			cabFadeMode=0;
			cabFade(0.4);
		} else if (cabFadeTimer < 0.1 && cabFadeMode == 2)
		{
			cabFadeMode=0;
			cabFade(1);
		}

		if (cabFadeMode == 1)
			cabFade(0.4 + 0.6 * cabFadeTimer/cabFadeTime);
		else if (cabFadeMode == 2)
			cabFade(1 - 0.6 * cabFadeTimer/cabFadeTime);
	}

	for (int i=0; i<free_beam; i++)
	{
		if (!skeleton)
		{
			if (beams[i].broken==1 && beams[i].mSceneNode)
			{
				beams[i].mSceneNode->detachAllObjects();
				beams[i].broken = 2;
			}

			if (beams[i].mSceneNode!=0 && !beams[i].disabled && beams[i].type!=BEAM_INVISIBLE && beams[i].type!=BEAM_INVISIBLE_HYDRO && beams[i].type!=BEAM_VIRTUAL)
			{
				beams[i].mSceneNode->setPosition(beams[i].p1->smoothpos.midPoint(beams[i].p2->smoothpos));
				beams[i].mSceneNode->setOrientation(specialGetRotationTo(ref, beams[i].p1->smoothpos-beams[i].p2->smoothpos));
				//beams[i].mSceneNode->setScale(default_beam_diameter/100.0,(beams[i].p1->smoothpos-beams[i].p2->smoothpos).length()/100.0,default_beam_diameter/100.0);
				beams[i].mSceneNode->setScale(beams[i].diameter, (beams[i].p1->smoothpos-beams[i].p2->smoothpos).length(), beams[i].diameter);
			}
		} else if (beams[i].mSceneNode!=0 && !beams[i].disabled)
		{
			beams[i].mSceneNode->setPosition(beams[i].p1->smoothpos.midPoint(beams[i].p2->smoothpos));
			beams[i].mSceneNode->setOrientation(specialGetRotationTo(ref, beams[i].p1->smoothpos-beams[i].p2->smoothpos));
			//beams[i].mSceneNode->setScale(default_beam_diameter/100.0,(beams[i].p1->smoothpos-beams[i].p2->smoothpos).length()/100.0,default_beam_diameter/100.0);
			beams[i].mSceneNode->setScale(skeleton_beam_diameter, (beams[i].p1->smoothpos-beams[i].p2->smoothpos).length(), skeleton_beam_diameter);
		}

	}

	if (skeleton == 2)
		updateSimpleSkeleton();

	BES_GFX_START(BES_GFX_updateFlexBodies);
	if (cabMesh) cabNode->setPosition(cabMesh->flexit());

	if (gEnv->threadPool)
	{
		flexmesh_prepare.reset();
		for (int i=0; i<free_wheel; i++)
		{
			flexmesh_prepare.set(i, vwheels[i].cnode && vwheels[i].fm->flexitPrepare(this));
		}

		flexbody_prepare.reset();
		for (int i=0; i<free_flexbody; i++)
		{
			flexbody_prepare.set(i, flexbodies[i]->flexitPrepare(this));
		}

		flexable_task_count = flexmesh_prepare.count() + flexbody_prepare.count();

		std::list<IThreadTask*> tasks;

		// Push tasks into thread pool
		for (int i=0; i<free_wheel; i++)
		{
			if (flexmesh_prepare[i])
				tasks.emplace_back(vwheels[i].fm);
		}
		for (int i=0; i<free_flexbody; i++)
		{
			if (flexbody_prepare[i])
				tasks.emplace_back(flexbodies[i]);
		}

		gEnv->threadPool->enqueue(tasks);
	} else
	{
		for (int i=0; i<free_wheel; i++)
		{
			if (vwheels[i].cnode && vwheels[i].fm->flexitPrepare(this))
			{
				vwheels[i].fm->flexitCompute();
				vwheels[i].cnode->setPosition(vwheels[i].fm->flexitFinal());
			}
		}
		for (int i=0; i<free_flexbody; i++)
		{
			if (flexbodies[i]->flexitPrepare(this))
			{
				flexbodies[i]->flexitCompute();
				flexbodies[i]->flexitFinal();
			}
		}
	}
	BES_GFX_STOP(BES_GFX_updateFlexBodies);

	BES_GFX_STOP(BES_GFX_updateVisual);
}

void Beam::updateVisualFinal(float dt)
{
	if (gEnv->threadPool)
	{
		// Wait for all tasks to complete
		MUTEX_LOCK(&flexable_task_count_mutex);
		while (flexable_task_count > 0)
		{
			pthread_cond_wait(&flexable_task_count_cv, &flexable_task_count_mutex);
		}
		MUTEX_UNLOCK(&flexable_task_count_mutex);

		for (int i=0; i<free_wheel; i++)
		{
			if (flexmesh_prepare[i])
				vwheels[i].cnode->setPosition(vwheels[i].fm->flexitFinal());
		}
		for (int i=0; i<free_flexbody; i++)
		{
			if (flexbody_prepare[i])
				flexbodies[i]->flexitFinal();
		}
	} 

	BES_GFX_STOP(BES_GFX_updateFlexBodies);

	BES_GFX_STOP(BES_GFX_updateVisual);
}

void Beam::updateVisual(float dt)
{
	updateVisualPrepare(dt);
	updateVisualFinal(dt);
}

//v=0: full detail
//v=1: no beams
void Beam::setDetailLevel(int v)
{
	if (v != detailLevel)
	{
		if (detailLevel == 0 && v == 1)
		{
			// detach
			gEnv->sceneManager->getRootSceneNode()->removeChild(beamsRoot);
		}
		if (detailLevel == 1 && v == 0)
		{
			// attach
			gEnv->sceneManager->getRootSceneNode()->addChild(beamsRoot);
		}
		detailLevel = v;
	}
}

void Beam::preMapLabelRenderUpdate(bool mode, float charheight)
{
	static float orgcharheight=0;
	if (mode && netLabelNode)
	{
		netMT->showOnTop(true);
		orgcharheight = netMT->getCharacterHeight();
		netMT->setCharacterHeight(charheight);
		//netMT->setAdditionalHeight(0);
		netMT->setVisible(false);
	} else if (!mode && netLabelNode)
	{
		netMT->showOnTop(false);
		netMT->setCharacterHeight(orgcharheight);
		netMT->setVisible(true);
	}
}

void Beam::showSkeleton(bool meshes, bool newMode, bool linked)
{
	if (lockSkeletonchange)
		return;
	lockSkeletonchange=true;
	int i;

	skeleton=1;

	if (newMode)
		skeleton=2;

	if (meshes)
	{
		cabFadeMode=1;
		cabFadeTimer=cabFadeTime;
	} else
	{
		cabFadeMode=-1;
		// directly hide meshes, no fading
		cabFade(0);
	}
	for (i=0; i<free_wheel; i++)
	{
		if (vwheels[i].cnode) vwheels[i].cnode->setVisible(false);
		if (vwheels[i].fm) vwheels[i].fm->setVisible(false);
	}
	for (i=0; i<free_prop; i++)
	{
		if (props[i].snode)
			setMeshWireframe(props[i].snode, true);
		if (props[i].wheel)
			setMeshWireframe(props[i].wheel, true);
	}

	if (!newMode)
	{
		for (i=0; i<free_beam; i++)
		{
			if (beams[i].mSceneNode && beams[i].mEntity)
			{
				if (!beams[i].broken && beams[i].mSceneNode->numAttachedObjects()==0)
					beams[i].mSceneNode->attachObject(beams[i].mEntity);
				//material
				beams[i].mEntity->setMaterialName("mat-beam-0");
				beams[i].mEntity->setCastShadows(false);
			}
		}
	}else
	{
		if (simpleSkeletonNode)
		{
			updateSimpleSkeleton();
			simpleSkeletonNode->setVisible(true);
		}
	}

	// hide mesh wheels
	for (i=0; i<free_wheel; i++)
	{
		if (vwheels[i].fm && vwheels[i].meshwheel)
		{
			Entity *e = ((FlexMeshWheel*)(vwheels[i].fm))->getRimEntity();
			if (e)
				e->setVisible(false);
		}
	}

	// wireframe drawning for flexbody
	for (i=0; i<free_flexbody; i++)
	{
		SceneNode *s = flexbodies[i]->getSceneNode();
		if (!s)
			continue;
		setMeshWireframe(s, true);
	}

	for (std::vector<tie_t>::iterator it=ties.begin(); it!=ties.end(); it++)
		if (it->beam->disabled)
			it->beam->mSceneNode->detachAllObjects();
	
	if (linked)
	{
		// apply to the locked truck
		for (std::list<Beam*>::iterator it = linkedBeams.begin(); it != linkedBeams.end(); ++it)
		{
			(*it)->showSkeleton(meshes, newMode, false);
		}
	}

	lockSkeletonchange=false;

	TRIGGER_EVENT(SE_TRUCK_SKELETON_TOGGLE, trucknum);
}

void Beam::hideSkeleton(bool newMode, bool linked)
{
	if (lockSkeletonchange)
		return;
	lockSkeletonchange=true;
	int i;
	skeleton=0;

	if (cabFadeMode>=0)
	{
		cabFadeMode=2;
		cabFadeTimer=cabFadeTime;
	} else
	{
		cabFadeMode=-1;
		// directly show meshes, no fading
		cabFade(1);
	}


	for (i=0; i<free_wheel; i++)
	{
		if (vwheels[i].cnode) vwheels[i].cnode->setVisible(true);
		if (vwheels[i].fm) vwheels[i].fm->setVisible(true);
	}
	for (i=0; i<free_prop; i++)
	{
		if (props[i].snode)
			setMeshWireframe(props[i].snode, false);
		if (props[i].wheel)
			setMeshWireframe(props[i].wheel, false);
	}

	if (!newMode)
	{
		for (i=0; i<free_beam; i++)
		{
			if (beams[i].mSceneNode)
			{
				if (beams[i].type==BEAM_VIRTUAL || beams[i].type==BEAM_INVISIBLE || beams[i].type==BEAM_INVISIBLE_HYDRO) beams[i].mSceneNode->detachAllObjects();
				//material
				if (beams[i].type==BEAM_HYDRO || beams[i].type==BEAM_MARKED) beams[i].mEntity->setMaterialName("tracks/Chrome");
				else beams[i].mEntity->setMaterialName(default_beam_material);
			}
		}
	}else
	{
		if (simpleSkeletonNode)
			simpleSkeletonNode->setVisible(false);
	}

	// show mesh wheels
	for (i=0; i<free_wheel; i++)
	{
		if (vwheels[i].fm && vwheels[i].meshwheel)
		{
			Entity *e = ((FlexMeshWheel *)(vwheels[i].fm))->getRimEntity();
			if (e)
				e->setVisible(true);
		}
	}

	// normal drawning for flexbody
	for (i=0; i<free_flexbody; i++)
	{
		SceneNode *s = flexbodies[i]->getSceneNode();
		if (!s)
			continue;
		setMeshWireframe(s, false);
	}

	for (std::vector<tie_t>::iterator it=ties.begin(); it!=ties.end(); it++)
		if (it->beam->disabled)
			it->beam->mSceneNode->detachAllObjects();

	if (linked)
	{
		// apply to the locked truck
		for (std::list<Beam*>::iterator it = linkedBeams.begin(); it != linkedBeams.end(); ++it)
		{
			(*it)->hideSkeleton(newMode, false);
		}
	}

	lockSkeletonchange=false;
}

void Beam::fadeMesh(SceneNode *node, float amount)
{
	for (int a=0;a<node->numAttachedObjects();a++)
	{
		Entity *e = (Entity *)node->getAttachedObject(a);
		MaterialPtr m = e->getSubEntity(0)->getMaterial();
		if (m.getPointer() == 0)
			continue;
		for (int x=0;x<m->getNumTechniques();x++)
		{
			for (int y=0;y<m->getTechnique(x)->getNumPasses();y++)
			{
				// TODO: fix this
				//m->getTechnique(x)->getPass(y)->setAlphaRejectValue(0);
				if (m->getTechnique(x)->getPass(y)->getNumTextureUnitStates() > 0)
					m->getTechnique(x)->getPass(y)->getTextureUnitState(0)->setAlphaOperation(LBX_MODULATE, LBS_TEXTURE, LBS_MANUAL, 1.0, amount);
			}
		}
	}
}

float Beam::getAlphaRejection(SceneNode *node)
{
	for (int a=0;a<node->numAttachedObjects();a++)
	{
		Entity *e = (Entity *)node->getAttachedObject(a);
		MaterialPtr m = e->getSubEntity(0)->getMaterial();
		if (m.getPointer() == 0)
			continue;
		for (int x=0;x<m->getNumTechniques();x++)
		{
			for (int y=0;y<m->getTechnique(x)->getNumPasses();y++)
			{
				return m->getTechnique(x)->getPass(y)->getAlphaRejectValue();
			}
		}
	}
	return 0;
}

void Beam::setAlphaRejection(SceneNode *node, float amount)
{
	for (int a=0;a<node->numAttachedObjects();a++)
	{
		Entity *e = (Entity *)node->getAttachedObject(a);
		MaterialPtr m = e->getSubEntity(0)->getMaterial();
		if (m.getPointer() == 0)
			continue;
		for (int x=0;x<m->getNumTechniques();x++)
		{
			for (int y=0;y<m->getTechnique(x)->getNumPasses();y++)
			{
				m->getTechnique(x)->getPass(y)->setAlphaRejectValue((unsigned char)amount);
				return;
			}
		}
	}
}
void Beam::setMeshWireframe(SceneNode *node, bool value)
{
	for (int a=0;a<node->numAttachedObjects();a++)
	{
		Entity *e = (Entity *)node->getAttachedObject(a);
		for (int se=0;se<(int)e->getNumSubEntities();se++)
		{
			MaterialPtr m = e->getSubEntity(se)->getMaterial();
			if (m.getPointer() == 0)
				continue;
			for (int x=0;x<m->getNumTechniques();x++)
				for (int y=0;y<m->getTechnique(x)->getNumPasses();y++)
					if (value)
						m->getTechnique(x)->getPass(y)->setPolygonMode(PM_WIREFRAME);
					else
						m->getTechnique(x)->getPass(y)->setPolygonMode(PM_SOLID);
		}
	}
}

void Beam::setBeamVisibility(bool visible, bool linked)
{
	for (int i=0; i < free_beam; i++)
	{
		if (beams[i].mSceneNode)
		{
			beams[i].mSceneNode->setVisible(visible);
		}
	}

	beamsVisible = visible;

	if (linked)
	{
		// apply to the locked truck
		for (std::list<Beam*>::iterator it = linkedBeams.begin(); it != linkedBeams.end(); ++it)
		{
			(*it)->setBeamVisibility(visible, false);
		}
	}
}

void Beam::setMeshVisibility(bool visible, bool linked)
{
	for (int i=0; i < free_prop; i++)
	{
		if (props[i].mo)			props[i].mo->setVisible(visible);
		if (props[i].wheel)		props[i].wheel->setVisible(visible);
		if (props[i].bbsnode[0]) props[i].bbsnode[0]->setVisible(visible);
		if (props[i].bbsnode[1]) props[i].bbsnode[1]->setVisible(visible);
		if (props[i].bbsnode[2]) props[i].bbsnode[2]->setVisible(visible);
		if (props[i].bbsnode[3]) props[i].bbsnode[3]->setVisible(visible);
	}
	for (int i=0; i < free_flexbody; i++)
	{
		flexbodies[i]->setVisible(visible);
	}
	for (int i=0; i < free_wheel; i++)
	{
		if (vwheels[i].cnode)
		{
			vwheels[i].cnode->setVisible(visible);
		}
		if (vwheels[i].fm)
		{
			vwheels[i].fm->setVisible(visible);
		}
	}
	if (cabMesh)
	{
		cabNode->setVisible(visible);
	}

	meshesVisible = visible;

	if (linked)
	{
		// apply to the locked truck
		for (std::list<Beam*>::iterator it = linkedBeams.begin(); it != linkedBeams.end(); ++it)
		{
			(*it)->setMeshVisibility(visible, false);
		}
	}
}

void Beam::cabFade(float amount)
{
	static float savedCabAlphaRejection = 0;

	// truck cab
	if (cabMesh)
	{
		if (amount == 0)
		{
			cabNode->setVisible(false);
		} else
		{
			if (amount == 1)
				cabNode->setVisible(true);
			if (savedCabAlphaRejection == 0)
				savedCabAlphaRejection = getAlphaRejection(cabNode);
			if (amount == 1)
				setAlphaRejection(cabNode, savedCabAlphaRejection);
			else if (amount < 1)
				setAlphaRejection(cabNode, 0);
			fadeMesh(cabNode, amount);
		}
	}

	// wings
	for (int i=0; i<free_wing; i++)
	{
		if (amount == 0)
		{
			wings[i].cnode->setVisible(false);
		} else
		{
			if (amount == 1)
				wings[i].cnode->setVisible(true);
			fadeMesh(wings[i].cnode, amount);
		}
	}
}

void Beam::tieToggle(int group)
{
	//export tie commands
	Beam **trucks = BeamFactory::getSingleton().getTrucks();
	int trucksnum = BeamFactory::getSingleton().getTruckCount();

	if (state==ACTIVATED && forwardcommands)
	{
		int i;
		for (i=0; i<trucksnum; i++)
		{
			if (!trucks[i]) continue;
			if (trucks[i]->state==DESACTIVATED && trucks[i]->importcommands)
				trucks[i]->tieToggle(group);
		}
	}

	//untie all ties if one is tied
	bool istied = false;

	for (std::vector<tie_t>::iterator it=ties.begin(); it!=ties.end(); it++)
	{
		// only handle ties with correct group
		if (group != -1 && (it->group != -1 && it->group != group))
			continue;

		// if tied, untie it. And the other way round
		if (it->tied)
		{
			// tie is locked and should get unlocked and stop tying
			it->tied  = false;
			it->tying = false;
			if (it->lockedto) it->lockedto->used--;
			// disable the ties beam
			it->beam->p2 = &nodes[0];
			it->beam->p2truck = 0;
			it->beam->disabled = true;
			it->beam->mSceneNode->detachAllObjects();
			istied = true;
		}
	}

	// iterate over all ties
	if (!istied)
	{
		for (std::vector<tie_t>::iterator it=ties.begin(); it!=ties.end(); it++)
		{
			// only handle ties with correct group
			if (group != -1 && (it->group != -1 && it->group != group))
			continue;

			if (!it->tied)
			{
				// tie is unlocked and should get locked, search new remote ropable to lock to
				float mindist = it->beam->refL;
				node_t *shorter=0;
				Beam *shtruck=0;
				ropable_t *locktedto=0;
				// iterate over all trucks
				for (int t=0; t<trucksnum; t++)
				{
					if (!trucks[t]) continue;
					if (trucks[t]->state==SLEEPING) continue;
					// and their ropables
					for (std::vector <ropable_t>::iterator itr = trucks[t]->ropables.begin(); itr!=trucks[t]->ropables.end(); itr++)
					{
						// if the ropable is not multilock and used, then discard this ropable
						if (!itr->multilock && itr->used)
							continue;

						//skip if tienode is ropable too (no selflock)
						if (itr->node->id == it->beam->p1->id)
							continue;

						// calculate the distance and record the nearest ropable
						float dist = (it->beam->p1->AbsPosition - itr->node->AbsPosition).length();
						if (dist < mindist)
						{
							mindist = dist;
							shorter = itr->node;
							shtruck = trucks[t];
							locktedto = &(*itr);
						}
					}
				}
				// if we found a ropable, then tie towards it
				if (shorter)
				{
					//okay, we have found a rope to tie

					// enable the beam and visually display the beam
					it->beam->disabled = false;
					if (it->beam->mSceneNode->numAttachedObjects() == 0)
						it->beam->mSceneNode->attachObject(it->beam->mEntity);

					// now trigger the tying action
					it->beam->p2 = shorter;
					it->beam->p2truck = shtruck;
					it->beam->stress = 0;
					it->beam->L = it->beam->refL;
					it->tied  = true;
					it->tying = true;
					it->lockedto = locktedto;
					it->lockedto->used++;
				}
			}
		}
	}

	//ScriptEvent - Tie toggle
	TRIGGER_EVENT(SE_TRUCK_TIE_TOGGLE, trucknum);
}

void Beam::ropeToggle(int group)
{
	Beam **trucks = BeamFactory::getSingleton().getTrucks();
	int trucksnum = BeamFactory::getSingleton().getTruckCount();

	// iterate over all ropes
	for (std::vector <rope_t>::iterator it = ropes.begin(); it!=ropes.end(); it++)
	{
		// only handle ropes with correct group
		if (group != -1 && (it->group != -1 && it->group != group))
			continue;

		if (it->locked == LOCKED || it->locked == PRELOCK)
		{
			// we unlock ropes
			it->locked = UNLOCKED;
			// remove node locking
			if (it->lockedto)         it->lockedto->lockednode=0;
			if (it->lockedto_ropable) it->lockedto_ropable->used--;
			it->lockedto = &nodes[0];
			it->lockedtruck = 0;
		} else
		{
			//we lock ropes
			// search new remote ropable to lock to
			float mindist = it->beam->L;
			node_t *shorter=0;
			Beam *shtruck=0;
			ropable_t *rop=0;
			// iterate over all trucks
			for (int t=0; t<trucksnum; t++)
			{
				if (!trucks[t]) continue;
				if (trucks[t]->state==SLEEPING) continue;
				// and their ropables
				for (std::vector <ropable_t>::iterator itr = trucks[t]->ropables.begin(); itr!=trucks[t]->ropables.end(); itr++)
				{
					// if the ropable is not multilock and used, then discard this ropable
					if (!itr->multilock && itr->used)
						continue;

					// calculate the distance and record the nearest ropable
					float dist = (it->beam->p1->AbsPosition - itr->node->AbsPosition).length();
					if (dist < mindist)
					{
						mindist = dist;
						shorter = itr->node;
						shtruck = trucks[t];
						rop     = &(*itr);
					}
				}
			}
			// if we found a ropable, then lock it
			if (shorter)
			{
				//okay, we have found a rope to tie
				it->lockedto    = shorter;
				it->lockedtruck = shtruck;
				it->locked      = PRELOCK;
				it->lockedto_ropable = rop;
				it->lockedto_ropable->used++;
			}
		}
	}
}

void Beam::hookToggle(int group, int mode, int node_number)
{
	Beam **trucks = BeamFactory::getSingleton().getTrucks();
	int trucksnum = BeamFactory::getSingleton().getTruckCount();

	// iterate over all hooks
	for (std::vector <hook_t>::iterator it = hooks.begin(); it!=hooks.end(); it++)
	{
		if (mode == MOUSE_HOOK_TOGGLE && it->hookNode->id != node_number)
			//skip all other nodes except the one manually toggled by mouse
			continue;

		if (mode == HOOK_TOGGLE && group == -1)
		{
			//manually triggerd (EV_COMMON_LOCK). Toggle all hooks groups with group#: -1, 0, 1 ++
			if (it->group <= -2)
					continue;
		}

		if (mode == HOOK_LOCK && group == -2)
		{
			//automatic lock attempt (cyclic with doupdate). Toggle all hooks groups with group#: -2, -3, -4 --, skip the ones wiwhich are not autlock ( triggered only )
			if (it->group >= -1 || !it->autolock)
				continue;
		}
		if (mode == HOOK_UNLOCK && group == -2)
		{
			//manual unlock ALL autolock and triggerlock, do not unlock standard hooks (EV_COMMON_AUTOLOCK)
			if (it->group >= -1 || !it->autolock)
				continue;
		}
		if ((mode == HOOK_LOCK || mode == HOOK_UNLOCK) && group <= -3)
		{
			//trigger beam lock or unlock. Toggle one hook group with group#: group
			if (it->group != group)
				continue;
		}
		if ((mode == HOOK_LOCK || mode == HOOK_UNLOCK) && group >= -1)
			continue;
		if (mode == HOOK_LOCK && it->timer > 0.0f)
		{
			//check relock delay timer for autolock nodes and skip if not 0
			continue;
		}

		Beam* lastLockTruck = it->lockTruck; // memorize current value

		//this is a locked or prelocked hook and its not a locking attempt
		if ((it->locked == LOCKED || it->locked == PRELOCK) && mode != HOOK_LOCK)
		{
			// we unlock ropes
			it->locked = UNLOCKED;
			// remove node locking
			if (it->lockNode)
				it->lockNode->lockednode=0;
			it->lockNode  = 0;
			it->lockTruck = 0;
			if (it->group <= -2)
				it->timer = it->timer_preset;	//timer reset for autolock nodes

			//disable hook-assistance beam
			it->beam->mSceneNode->detachAllObjects();
			it->beam->p2       = &nodes[0];
			it->beam->p2truck  = 0;
			it->beam->L        = (nodes[0].AbsPosition - it->hookNode->AbsPosition).length();
			it->beam->disabled = true;
		}
		// do this only for toggle or lock attempts, skip prelocked or locked nodes for performance
		else if (mode != HOOK_UNLOCK && it->locked == UNLOCKED)
		{
			// we lock hooks
			// search new remote ropable to lock to
			float mindist = it->lockrange;
			node_t *shorter=0;
			Beam *shtruck=0;
			float distance = 100000000.0f;
			// iterate over all trucks
			for (int t=0; t<trucksnum; t++)
			{
				if (t == this->trucknum && !it->selflock) continue; // dont lock to self
				if (!trucks[t]) continue;
				if (trucks[t]->state==SLEEPING) continue;

				// do we lock against all nodes or just against ropables?
				bool found = false;
				if (it->lockNodes)
				{
					int last_node=0; // nodenumber storage
					// all nodes, so walk them
					for (int i=0; i<trucks[t]->free_node; i++)
					{
						//skip all nodes with lockgroup 9999 (denylock)
						if (trucks[t]->nodes[i].lockgroup == 9999)
							continue;

						// exclude this truck and its current hooknode from the locking search
						if (this == trucks[t] && i == it->hookNode->id)
							continue;

						// a lockgroup for this hooknode is set -> skip all nodes that do not have the same lockgroup (-1 = default(all nodes))
						if (it->lockgroup != -1 && it->lockgroup != trucks[t]->nodes[i].lockgroup)
							continue;

						// measure distance
						float n2n_distance = (it->hookNode->AbsPosition - trucks[t]->nodes[i].AbsPosition).length();
						if (n2n_distance < mindist)
						{
							if (distance >= n2n_distance)
							{
								//lockated a node that is closer
								distance = n2n_distance;
								last_node=i;
								found = true;
							}
						}
					}
					if (found)
					{
						// we found a node, lock to it
						it->lockNode  = &(trucks[t]->nodes[last_node]);
						it->lockTruck = trucks[t];
						it->locked    = PRELOCK;
						found     = true; // dont check the other trucks
					}
				} else
				{
					// we lock against ropables

					// and their ropables
					for (std::vector <ropable_t>::iterator itr = trucks[t]->ropables.begin(); itr!=trucks[t]->ropables.end(); itr++)
					{
						// if the ropable is not multilock and used, then discard this ropable
						if (!itr->multilock && itr->used)
							continue;

						// calculate the distance and record the nearest ropable
						float dist = (it->hookNode->AbsPosition - itr->node->AbsPosition).length();
						if (dist < mindist)
						{
							mindist = dist;
							shorter = itr->node;
							shtruck = trucks[t];
						}
					}
				}
				// if we found a ropable, then lock it
				if (shorter)
				{
					// we found a ropable, lock to it
					it->lockNode  = shorter;
					it->lockTruck = shtruck;
					it->locked    = PRELOCK;
					found         = true; // dont check the other trucks
				}

				//removed since we want to lock to the nearest node in distance
				//if (found)
					// if we found some lock, we wont check all other trucks
					//break;
			}
		}

		if (it->lockTruck != lastLockTruck)
		{
			std::list<Beam*> linkedBeams (linkedBeams);

			linkedBeams.push_back(this);
			linkedBeams.push_back(std::max(lastLockTruck, it->lockTruck));
			linkedBeams.splice(linkedBeams.end(), linkedBeams.back()->linkedBeams);
			linkedBeams.sort();
			linkedBeams.unique();

			for (std::list<Beam*>::iterator it = linkedBeams.begin(); it != linkedBeams.end(); ++it)
			{
				(*it)->determineLinkedBeams();

				if (skeleton && this != (*it) && !(*it)->skeleton)
				{
					(*it)->showSkeleton(true, skeleton==2, false);
				} else if (skeleton && this != (*it) && (*it)->skeleton)
				{
					(*it)->hideSkeleton(skeleton==2, false);
				}
			}
		}
	}
}

void Beam::parkingbrakeToggle()
{
	parkingbrake = !parkingbrake;

#ifdef USE_OPENAL
	if (parkingbrake)
		SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_PARK);
	else
		SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_PARK);
#endif // USE_OPENAL

	//ScriptEvent - Parking Brake toggle
	TRIGGER_EVENT(SE_TRUCK_PARKINGBREAK_TOGGLE, trucknum);
}

void Beam::antilockbrakeToggle()
{
	alb_mode = !alb_mode;
}

void Beam::tractioncontrolToggle()
{
	tc_mode = !tc_mode;
}

void Beam::cruisecontrolToggle()
{
	cc_mode = !cc_mode;

	if (cc_mode)
	{
		cc_target_speed = WheelSpeed;
		cc_target_rpm   = engine->getRPM();
	} else
	{
		cc_target_speed = 0;
		cc_target_rpm   = 0;
	}
}

void Beam::beaconsToggle()
{
	bool enableLight = true;
	if (flaresMode==0)
		return;
	if (flaresMode==1)
		enableLight=false;
	int i;
	beacon = !beacon;
	for (i=0; i<free_prop; i++)
	{
		if (props[i].beacontype=='b')
		{
			props[i].light[0]->setVisible(beacon && enableLight);
			props[i].bbsnode[0]->setVisible(beacon);
			if (props[i].bbs[0] && beacon && !props[i].bbsnode[0]->numAttachedObjects())
			{
				props[i].bbs[0]->setVisible(true);
				props[i].bbsnode[0]->attachObject(props[i].bbs[0]);
			} else if (props[i].bbs[0] && !beacon)
			{
				props[i].bbsnode[0]->detachAllObjects();
				props[i].bbs[0]->setVisible(false);
			}
		}
		else if (props[i].beacontype=='R' || props[i].beacontype=='L')
		{
			props[i].bbsnode[0]->setVisible(beacon);
			if (props[i].bbs[0] && beacon && !props[i].bbsnode[0]->numAttachedObjects())
				props[i].bbsnode[0]->attachObject(props[i].bbs[0]);
			else if (props[i].bbs[0] && !beacon)
				props[i].bbsnode[0]->detachAllObjects();
		}
		else if (props[i].beacontype=='p')
		{
			for (int k=0; k<4; k++)
			{
				props[i].light[k]->setVisible(beacon && enableLight);
				props[i].bbsnode[k]->setVisible(beacon);
				if (props[i].bbs[k] && beacon && !props[i].bbsnode[k]->numAttachedObjects())
					props[i].bbsnode[k]->attachObject(props[i].bbs[k]);
				else if (props[i].bbs[k] && !beacon)
					props[i].bbsnode[k]->detachAllObjects();
			}
		} else
		{
			for (int k=0; k<4; k++)
			{
				if (props[i].light[k])
				{
					props[i].light[k]->setVisible(beacon && enableLight);
				}
				if (props[i].bbsnode[k])
				{
					props[i].bbsnode[k]->setVisible(beacon);

					if (props[i].bbs[k] && beacon && !props[i].bbsnode[k]->numAttachedObjects())
					{
						props[i].bbsnode[k]->attachObject(props[i].bbs[k]);
					}
					else if (props[i].bbs[k] && !beacon)
					{
						props[i].bbsnode[k]->detachAllObjects();
					}
				}
			}
		}
	}

	//ScriptEvent - Beacon toggle
	TRIGGER_EVENT(SE_TRUCK_BEACONS_TOGGLE, trucknum);
}

void Beam::setReplayMode(bool rm)
{
	if (!replay || !replay->isValid()) return;

	if (replaymode && !rm)
	{
		replaypos=0;
		oldreplaypos=-1;
	}

	replaymode = rm;
	replay->setVisible(replaymode);
}

void Beam::setDebugOverlayState(int mode)
{
	// enable disable debug visuals
	debugVisuals = mode;

	if (nodes_debug.empty())
	{
		LOG("initializing debugVisuals");
		// add node labels
		for (int i=0; i<free_node; i++)
		{
			debugtext_t t;
			char nodeName[256]="", entName[256]="";
			sprintf(nodeName, "%s-nodesDebug-%d", truckname, i);
			sprintf(entName, "%s-nodesDebug-%d-Ent", truckname, i);
			t.id=i;
			t.txt = new MovableText(nodeName, "n"+TOSTRING(i));
			t.txt->setFontName("highcontrast_black");
			t.txt->setTextAlignment(MovableText::H_LEFT, MovableText::V_BELOW);
			//t.txt->setAdditionalHeight(0);
			t.txt->showOnTop(true);
			t.txt->setCharacterHeight(0.5f);
			t.txt->setColor(ColourValue::White);
			t.txt->setRenderingDistance(2);

			t.node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
			t.node->attachObject(t.txt);
			t.node->setPosition(nodes[i].smoothpos);
			t.node->setScale(Vector3(0.5,0.5,0.5));

			// collision nodes debug, also mimics as node visual
			SceneNode *s = t.node->createChildSceneNode();
			Entity *b = gEnv->sceneManager->createEntity(entName, "sphere.mesh");
			b->setMaterialName("tracks/transgreen");
			s->attachObject(b);
			float f = 0.005f;
			s->setScale(f,f,f);

			/*
			// collision nodes
			if (nodes[i].collRadius > 0.00001)
			{
				b->setMaterialName("tracks/transred");
				f = nodes[i].collRadius;
			} else
			{
				b->setMaterialName("tracks/transgreen");
			}
			*/

			nodes_debug.push_back(t);
		}

		// add beam labels
		for (int i=0; i<free_beam; i++)
		{
			debugtext_t t;
			char nodeName[256]="";
			sprintf(nodeName, "%s-beamsDebug-%d", truckname, i);
			t.id=i;
			t.txt = new MovableText(nodeName, "b"+TOSTRING(i));
			t.txt->setFontName("highcontrast_black");
			t.txt->setTextAlignment(MovableText::H_LEFT, MovableText::V_BELOW);
			//t.txt->setAdditionalHeight(0);
			t.txt->showOnTop(true);
			t.txt->setCharacterHeight(1);
			t.txt->setColor(ColourValue::Black);
			t.txt->setRenderingDistance(2);

			t.node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
			t.node->attachObject(t.txt);

			Vector3 pos = beams[i].p1->smoothpos - (beams[i].p1->smoothpos - beams[i].p2->smoothpos)/2;
			t.node->setPosition(pos);
			t.node->setVisible(false);
			t.node->setScale(Vector3(0.1,0.1,0.1));
			beams_debug.push_back(t);
		}

	}

	// then hide them according to the state:
	bool nodesVisible = debugVisuals == 1 || (debugVisuals >= 3 && debugVisuals <= 5);
	bool beamsVisible = debugVisuals == 2 || debugVisuals == 3 || (debugVisuals >= 6 && debugVisuals <= 11);

	for (std::vector<debugtext_t>::iterator it=nodes_debug.begin(); it!=nodes_debug.end();it++)
		it->node->setVisible(nodesVisible);
	for (std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
		it->node->setVisible(beamsVisible);

	updateDebugOverlay();
}

void Beam::updateDebugOverlay()
{
	if (!debugVisuals) return;

	switch(debugVisuals)
	{
	case 0: // off
		return;
	case 1: // node-numbers
		// not written dynamically
		for (std::vector<debugtext_t>::iterator it=nodes_debug.begin(); it!=nodes_debug.end();it++)
			it->node->setPosition(nodes[it->id].smoothpos);
		break;
	case 2: // beam-numbers
		// not written dynamically
		for (std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
			it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
		break;
	case 3: // node-and-beam-numbers
		// not written dynamically
		for (std::vector<debugtext_t>::iterator it=nodes_debug.begin(); it!=nodes_debug.end();it++)
			it->node->setPosition(nodes[it->id].smoothpos);
		for (std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
			it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
		break;
	case 4: // node-mass
		for (std::vector<debugtext_t>::iterator it=nodes_debug.begin(); it!=nodes_debug.end();it++)
		{
			it->node->setPosition(nodes[it->id].smoothpos);
			it->txt->setCaption(TOSTRING(nodes[it->id].mass));
		}
		break;
	case 5: // node-locked
		for (std::vector<debugtext_t>::iterator it=nodes_debug.begin(); it!=nodes_debug.end();it++)
		{
			it->txt->setCaption((nodes[it->id].locked)?"locked":"unlocked");
			it->node->setPosition(nodes[it->id].smoothpos);
		}
		break;
	case 6: // beam-compression
		for (std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
		{
			it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
			int scale=(int)(beams[it->id].scale * 100);
			it->txt->setCaption(TOSTRING(scale));
		}
		break;
	case 7: // beam-broken
		for (std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
		{
			it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
			if (beams[it->id].broken)
			{
				it->node->setVisible(true);
				it->txt->setCaption("BROKEN");
			} else
			{
				it->node->setVisible(false);
			}
		}
		break;
	case 8: // beam-stress
		for (std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
		{
			it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
			it->txt->setCaption(TOSTRING((float) fabs(beams[it->id].stress)));
		}
		break;
	case 9: // beam-strength
		for (std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
		{
			it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
			it->txt->setCaption(TOSTRING(beams[it->id].strength));
		}
		break;
	case 10: // beam-hydros
		for (std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
		{
			if (beams[it->id].type == BEAM_HYDRO || beams[it->id].type == BEAM_INVISIBLE_HYDRO)
			{
				it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
				int v = (beams[it->id].L / beams[it->id].Lhydro) * 100;
				it->txt->setCaption(TOSTRING(v));
				it->node->setVisible(true);
			} else
			{
				it->node->setVisible(false);
			}
		}
		break;
	case 11: // beam-commands
		for (std::vector<debugtext_t>::iterator it=beams_debug.begin(); it!=beams_debug.end();it++)
		{
			it->node->setPosition(beams[it->id].p1->smoothpos - (beams[it->id].p1->smoothpos - beams[it->id].p2->smoothpos)/2);
			int v = (beams[it->id].L / beams[it->id].commandLong) * 100;
			it->txt->setCaption(TOSTRING(v));
		}
		break;
	}
}

void Beam::updateNetworkInfo()
{
	BES_GFX_START(BES_GFX_updateNetworkInfo);
#ifdef USE_SOCKETW
	if (!gEnv->network) return;
	bool remote = (state == NETWORKED);
	if (remote)
	{
		client_t *c = gEnv->network->getClientInfo(sourceid);
		if (!c) return;
		networkUsername = UTFString(c->user.username);
		networkAuthlevel = c->user.authstatus;
	} else
	{
		user_info_t *info = gEnv->network->getLocalUserData();
		if (!info) return;
		if (UTFString(info->username).empty()) return;
		networkUsername = UTFString(info->username);
		networkAuthlevel = info->authstatus;
	}

	if (netLabelNode && netMT)
	{
		// ha, this caused the empty caption bug, but fixed now since we change the caption if its empty:
		netMT->setCaption(networkUsername);
		/*
		if (networkAuthlevel & AUTH_ADMIN)
		{
			netMT->setFontName("highcontrast_red");
		} else if (networkAuthlevel & AUTH_RANKED)
		{
			netMT->setFontName("highcontrast_green");
		} else
		{
			netMT->setFontName("highcontrast_black");
		}
		*/
		netLabelNode->setVisible(true);
	}
	else
	{
		char wname[256];
		sprintf(wname, "netlabel-%s", truckname);
		netMT = new MovableText(wname, networkUsername);
		netMT->setFontName("CyberbitEnglish");
		netMT->setTextAlignment(MovableText::H_CENTER, MovableText::V_ABOVE);
		//netMT->setAdditionalHeight(2);
		netMT->showOnTop(false);
		netMT->setCharacterHeight(2);
		netMT->setColor(ColourValue::Black);

		/*
		if (networkAuthlevel & AUTH_ADMIN)
		{
			netMT->setFontName("highcontrast_red");
		} else if (networkAuthlevel & AUTH_RANKED)
		{
			netMT->setFontName("highcontrast_green");
		} else
		{
			netMT->setFontName("highcontrast_black");
		}
		*/

		netLabelNode=gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
		netLabelNode->attachObject(netMT);
		netLabelNode->setPosition(position);
		netLabelNode->setVisible(true);
		deletion_sceneNodes.push_back(netLabelNode);
		deletion_Objects.push_back(netMT);
	}
#endif //SOCKETW
	BES_GFX_STOP(BES_GFX_updateNetworkInfo);
}

void Beam::deleteNetTruck()
{
	// TODO: properly delete things ...
	//park and recycle vehicle
	state = RECYCLE;
	netMT->setVisible(false);
	resetPosition(100000, 100000, false, 100000);
	netLabelNode->setVisible(false);
	updateVisual();
}

float Beam::getHeadingDirectionAngle()
{
	if (cameranodepos[0] >= 0 && cameranodedir[0] >= 0)
	{
		Vector3 idir = nodes[cameranodepos[0]].RelPosition - nodes[cameranodedir[0]].RelPosition;
		return atan2(idir.dotProduct(Vector3::UNIT_X), (idir).dotProduct(-Vector3::UNIT_Z));
	}

	return 0.0f;
}

bool Beam::getReverseLightVisible()
{
	if (state == NETWORKED)
		return netReverseLight;

	if (engine)
		return (engine->getGear() < 0);

	return reverselight;
}

void Beam::changedCamera()
{
	// change sound setup
#ifdef USE_OPENAL
	for (int i=0; i < free_soundsource; i++)
	{
		bool enabled = (soundsources[i].type == -2 || soundsources[i].type == currentcamera);
		soundsources[i].ssi->setEnabled(enabled);
	}
#endif // USE_OPENAL

	// change video camera mode needs for-loop through all video(mirror)cams, check camera mode against currentcamera and then send the right bool
	// bool state = true;
	// VideoCamera *v = VideoCamera::setActive(state);

	// look for props
	for (int i=0; i < free_prop; i++)
	{
		bool enabled = (props[i].cameramode == -2 || props[i].cameramode == currentcamera);
		if (props[i].mo) props[i].mo->setMeshEnabled(enabled);
	}

	// look for flexbodies
	for (int i=0; i < free_flexbody; i++)
	{
		bool enabled = (flexbodies[i]->getCameraMode() == -2 || flexbodies[i]->getCameraMode() == currentcamera);
		flexbodies[i]->setEnabled(enabled);
	}
}

//Returns the number of active (non bounded) beams connected to a node
int Beam::nodeBeamConnections(int nodeid)
{
	int totallivebeams=0;
	for (unsigned int ni=0; ni<nodebeamconnections[nodeid].size(); ++ni)
	{
		if (!beams[nodebeamconnections[nodeid][ni]].disabled && !beams[nodebeamconnections[nodeid][ni]].bounded) totallivebeams++;
	}
	return totallivebeams;
}

bool Beam::isTied()
{
	for (std::vector <tie_t>::iterator it=ties.begin(); it!=ties.end();it++)
		if (it->tied)
			return true;
	return false;
}

bool Beam::isLocked()
{
	for (std::vector <hook_t>::iterator it=hooks.begin(); it!=hooks.end();it++)
		if (it->locked==LOCKED)
			return true;
	return false;
}

int Beam::loadTruck2(String filename, SceneNode *parent, Vector3 pos, Quaternion rot, collision_box_t *spawnbox)
{
	int res = loadTruck(filename, parent, pos, rot, spawnbox);
	if (res) return res;

	// place correctly
	if (!hasfixes)
	{
		// check if over-sized
		calcBoundingBoxes();
		//px = px - (boundingBox.getMaximum().x + boundingBox.getMinimum().x) / 2.0;
		pos.x -= (boundingBox.getMaximum().x + boundingBox.getMinimum().x) / 2.0 - pos.x;
		//pz = pz - (boundingBox.getMaximum().z + boundingBox.getMinimum().z) / 2.0;
		pos.z -= (boundingBox.getMaximum().z + boundingBox.getMinimum().z)/2.0 - pos.z;
		
		if (freePositioned)
			resetPosition(pos, true);
		else
			resetPosition(pos.x, pos.z, true);

		if (spawnbox)
		{
			bool inside = true;

			for (int i=0; i < free_node; i++)
				inside = (inside && gEnv->collisions->isInside(nodes[i].AbsPosition, spawnbox, 0.2f));

			if (!inside)
			{
				float miny = spawnbox->relo.y + spawnbox->center.y + 0.01f;
				Vector3 gpos = Vector3(pos.x, 0.0f, pos.z);

				gpos -= rot * Vector3((spawnbox->hi.x - spawnbox->lo.x + boundingBox.getMaximum().x - boundingBox.getMinimum().x) * 0.6f, 0.0f, 0.0f);
				
				resetPosition(gpos.x, gpos.z, true, miny);
			}
		}
	} else
	{
		resetPosition(pos, true);
	}

	//compute final mass
	calc_masses2(truckmass);
	//setup default sounds
	if (!disable_default_sounds) setupDefaultSoundSources();

	//compute collision box
	calcBoundingBoxes();

	//compute lowest node
	calcLowestNode();

	//compute node connectivity graph
	calcNodeConnectivityGraph();

	//update contacter nodes
	updateContacterNodes();

	// fix up submesh collision model
	if (!subMeshGroundModelName.empty())
	{
		submesh_ground_model = gEnv->collisions->getGroundModelByString(subMeshGroundModelName);
		if (!submesh_ground_model) gEnv->collisions->defaultgm;
	}

	// print some truck memory stats
	int mem = 0, memr = 0, tmpmem = 0;
	LOG("BEAM: memory stats following");

	tmpmem = free_beam * sizeof(beam_t); mem += tmpmem;
	memr += MAX_BEAMS * sizeof(beam_t);
	LOG("BEAM: beam memory: " + TOSTRING(tmpmem) + " B (" + TOSTRING(free_beam) + " x " + TOSTRING(sizeof(beam_t)) + " B) / " + TOSTRING(MAX_BEAMS * sizeof(beam_t)));

	tmpmem = free_node * sizeof(node_t); mem += tmpmem;
	memr += MAX_NODES * sizeof(beam_t);
	LOG("BEAM: node memory: " + TOSTRING(tmpmem) + " B (" + TOSTRING(free_node) + " x " + TOSTRING(sizeof(node_t)) + " B) / " + TOSTRING(MAX_NODES * sizeof(node_t)));

	tmpmem = free_shock * sizeof(shock_t); mem += tmpmem;
	memr += MAX_SHOCKS * sizeof(beam_t);
	LOG("BEAM: shock memory: " + TOSTRING(tmpmem) + " B (" + TOSTRING(free_shock) + " x " + TOSTRING(sizeof(shock_t)) + " B) / " + TOSTRING(MAX_SHOCKS * sizeof(shock_t)));

	tmpmem = free_prop * sizeof(prop_t); mem += tmpmem;
	memr += MAX_PROPS * sizeof(beam_t);
	LOG("BEAM: prop memory: " + TOSTRING(tmpmem) + " B (" + TOSTRING(free_prop) + " x " + TOSTRING(sizeof(prop_t)) + " B) / " + TOSTRING(MAX_PROPS * sizeof(prop_t)));

	tmpmem = free_wheel * sizeof(wheel_t); mem += tmpmem;
	memr += MAX_WHEELS * sizeof(beam_t);
	LOG("BEAM: wheel memory: " + TOSTRING(tmpmem) + " B (" + TOSTRING(free_wheel) + " x " + TOSTRING(sizeof(wheel_t)) + " B) / " + TOSTRING(MAX_WHEELS * sizeof(wheel_t)));

	tmpmem = free_rigidifier * sizeof(rigidifier_t); mem += tmpmem;
	memr += MAX_RIGIDIFIERS * sizeof(beam_t);
	LOG("BEAM: rigidifier memory: " + TOSTRING(tmpmem) + " B (" + TOSTRING(free_rigidifier) + " x " + TOSTRING(sizeof(rigidifier_t)) + " B) / " + TOSTRING(MAX_RIGIDIFIERS * sizeof(rigidifier_t)));

	tmpmem = free_flare * sizeof(flare_t); mem += tmpmem;
	memr += free_flare * sizeof(beam_t);
	LOG("BEAM: flare memory: " + TOSTRING(tmpmem) + " B (" + TOSTRING(free_flare) + " x " + TOSTRING(sizeof(flare_t)) + " B)");

	LOG("BEAM: truck memory used: " + TOSTRING(mem)  + " B (" + TOSTRING(mem/1024)  + " kB)");
	LOG("BEAM: truck memory allocated: " + TOSTRING(memr)  + " B (" + TOSTRING(memr/1024)  + " kB)");


#ifdef USE_MYGUI
	// now load any dashboards
	if (dash)
	{
		if (dashBoardLayouts.empty())
		{
			// load default for a truck
			if (driveable == TRUCK)
			{
				dash->loadDashBoard("default_dashboard.layout", false);
				// TODO: load texture dashboard by default as well
				dash->loadDashBoard("default_dashboard.layout", true);
			}
		} else
		{
			// load all dashes
			for (unsigned int i=0; i < dashBoardLayouts.size(); i++)
				dash->loadDashBoard(dashBoardLayouts[i].first, dashBoardLayouts[i].second);
		}
		dash->setVisible(false);
	}
#endif // USE_MYGUI

	return res;
}

void Beam::updateAI(float dt)
{
	if (driveable != TRUCK ||
		!gEnv->cameraManager ||
		!gEnv->cameraManager->hasActiveBehavior() ||
		!gEnv->cameraManager->gameControlsLocked())
	{
		return;
	}

	// start engine if not running
	if (engine && !engine->running)
		engine->start();

	Vector3 TargetPosition = mCamera->getPosition();
	TargetPosition.y=0;
	Quaternion TargetOrientation = Quaternion::ZERO;

	Vector3 mAgentPosition        = position;
	Quaternion mAgentOrientation  = Quaternion(Radian(getHeadingDirectionAngle()), Vector3::NEGATIVE_UNIT_Y);
	mAgentOrientation.normalise();

	/*
	// this is for debugging purposes
	static SceneNode *n = 0;
	if (!n)
	{
		Entity *e = gEnv->ogreSceneManager->createEntity("axes.mesh");
		n = gEnv->ogreSceneManager->getRootSceneNode()->createChildSceneNode();
		n->attachObject(e);
	}
	n->setPosition(mAgentPosition);
	n->setOrientation(mAgentOrientation);
	*/

	Vector3 mVectorToTarget       = TargetPosition - mAgentPosition; // A-B = B->A
	mAgentPosition.normalise();
	mAgentPosition.y=0;

	Vector3 mAgentHeading         = mAgentOrientation * mAgentPosition;
	Vector3 mTargetHeading        = TargetOrientation * TargetPosition;
	mAgentHeading.normalise();
	mTargetHeading.normalise();

	// Orientation control - Vector3::UNIT_Y is common up vector.
	Vector3 mAgentVO        = mAgentOrientation.Inverse() * Vector3::UNIT_Y;
	Vector3 mTargetVO       = TargetOrientation * Vector3::UNIT_Y;

	// Compute new torque scalar (-1.0 to 1.0) based on heading vector to target.
	Vector3 mSteeringForce = mAgentOrientation.Inverse() * mVectorToTarget;
	mSteeringForce.normalise();

	float mYaw    = mSteeringForce.x;
	//float mPitch  = mSteeringForce.y;
	//float mRoll   = mTargetVO.getRotationTo( mAgentVO ).getRoll().valueRadians();

	/*
	String txt = "AI:"+TOSTRING(mSteeringForce);
	gEnv->frameListener->getOverlayWrapper()->flashMessage(txt, 1, -1);
	*/

	// actually steer
	hydrodircommand = mYaw;

	// accelerate / brake
	float maxvelo = 1;

	maxvelo = std::max<float>(0.2f, 1-fabs(mYaw)) * 50;


	if (engine)
	{
		if (mVectorToTarget.length() > minCameraRadius * 2.0f)
		{
			if (WheelSpeed < maxvelo)
				engine->autoSetAcc(0.8f);
			else
				engine->autoSetAcc(0);
			brake = 0;
		} else
		{
			engine->autoSetAcc(0);
			brake = brakeforce;

			if (!(int(gEnv->mrTime * 4.0f) % 2)) 
			{
				lightsToggle();
			}
		}
	}
}

void Beam::updateDashBoards(float &dt)
{
#ifdef USE_MYGUI
	if (!dash) return;
	// some temp vars
	Vector3 dir;

	//special case for the editor
	if (editorId >= 0)
	{
		String str = "Position: X=" + TOSTRING(nodes[editorId].AbsPosition.x) + "  Y="+TOSTRING(nodes[editorId].AbsPosition.y) + "  Z="+TOSTRING(nodes[editorId].AbsPosition.z);
		// TODO: FIX THIS?
		//str += "Angles: 0.0 " + TOSTRING(editor->pturn)+ "  "+TOSTRING(editor->ppitch);
		//str += "Object: " + String(editor->curtype);
		dash->setChar(DD_EDITOR_NODE_INFO, str.c_str());
	}

	// engine and gears
	if (engine)
	{
		// gears first
		int gear = engine->getGear();
		dash->setInt(DD_ENGINE_GEAR, gear);

		int numGears = (int)engine->getNumGears();
		dash->setInt(DD_ENGINE_NUM_GEAR, numGears);

		String str = String();

		// now construct that classic gear string
		if (gear > 0)
			str = TOSTRING(gear) + "/" + TOSTRING(numGears);
		else if (gear==0)
			str = String("N");
		else
			str = String("R");

		dash->setChar(DD_ENGINE_GEAR_STRING, str.c_str());
		
		// R N D 2 1 String
		int cg = engine->getAutoShift();
		if (cg != BeamEngine::MANUALMODE)
		{
			str  = ((cg == BeamEngine::REAR)   ?"#ffffff":"#868686") + String("R\n");
			str += ((cg == BeamEngine::NEUTRAL)?"#ff0012":"#8a000a") + String("N\n");
			str += ((cg == BeamEngine::DRIVE)  ?"#12ff00":"#248c00") + String("D\n");
			str += ((cg == BeamEngine::TWO)    ?"#ffffff":"#868686") + String("2\n");
			str += ((cg == BeamEngine::ONE)    ?"#ffffff":"#868686") + String("1");
		} else
		{
			//str = "#b8b8b8M\na\nn\nu\na\nl";
			str = "#b8b8b8M\na\nn\nu";
		}
		dash->setChar(DD_ENGINE_AUTOGEAR_STRING, str.c_str());

		// autogears
		int autoGear = engine->getAutoShift();
		dash->setInt(DD_ENGINE_AUTO_GEAR, autoGear);

		// clutch
		float clutch = engine->getClutch();
		dash->setFloat(DD_ENGINE_CLUTCH, clutch);

		// accelerator
		float acc = engine->getAcc();
		dash->setFloat(DD_ACCELERATOR, acc);

		// RPM
		float rpm = engine->getRPM();
		dash->setFloat(DD_ENGINE_RPM, rpm);

		// turbo
		float turbo = engine->getTurboPSI() * 3.34f; // MAGIC :/
		dash->setFloat(DD_ENGINE_TURBO, turbo);

		// ignition
		bool ign = engine->contact;
		dash->setBool(DD_ENGINE_IGNITION, ign);

		// battery
		bool batt = (engine->contact && !engine->running);
		dash->setBool(DD_ENGINE_BATTERY, batt);

		// clutch warning
		bool cw = (fabs(engine->getTorque()) >= engine->getClutchForce() * 10.0f);
		dash->setBool(DD_ENGINE_CLUTCH_WARNING, cw);

	}

	// brake
	float dash_brake = brake / brakeforce;
	dash->setFloat(DD_BRAKE, dash_brake);

	// speedo
	float velocity = nodes[0].Velocity.length();

	if (cameranodepos[0] >= 0 && cameranodedir[0] >=0)
	{
		Vector3 hdir = (nodes[cameranodepos[0]].RelPosition - nodes[cameranodedir[0]].RelPosition).normalisedCopy();
		velocity = hdir.dotProduct(nodes[0].Velocity);
	}
	float speed_kph = velocity * 3.6f;
	dash->setFloat(DD_ENGINE_SPEEDO_KPH, speed_kph);
	float speed_mph = velocity * 2.23693629f;
	dash->setFloat(DD_ENGINE_SPEEDO_MPH, speed_mph);

	// roll
	if (cameranodepos[0] >= 0 && cameranodepos[0] < MAX_NODES)
	{
		dir = nodes[cameranodepos[0]].RelPosition - nodes[cameranoderoll[0]].RelPosition;
		dir.normalise();
		float angle = asin(dir.dotProduct(Vector3::UNIT_Y));
		if (angle < -1) angle = -1;
		if (angle >  1) angle =  1;

		float f = Radian(angle).valueDegrees();
		dash->setFloat(DD_ROLL, f);
	}

	// active shocks / roll correction
	if (free_active_shock)
	{
		// TOFIX: certainly not working:
		float roll_corr = - stabratio * 10.0f;
		dash->setFloat(DD_ROLL_CORR, roll_corr);

		bool corr_active = (stabcommand > 0);
		dash->setBool(DD_ROLL_CORR_ACTIVE, corr_active);
	}

	// pitch
	if (cameranodepos[0] >= 0 && cameranodepos[0] < MAX_NODES)
	{
		dir = nodes[cameranodepos[0]].RelPosition - nodes[cameranodedir[0]].RelPosition;
		dir.normalise();
		float angle = asin(dir.dotProduct(Vector3::UNIT_Y));
		if (angle < -1) angle = -1;
		if (angle >  1) angle =  1;

		float f = Radian(angle).valueDegrees();
		dash->setFloat(DD_PITCH, f);
	}

	// parking brake
	bool pbrake = (parkingbrake > 0);
	dash->setBool(DD_PARKINGBRAKE, pbrake);

	// locked lamp
	bool locked = isLocked();
	dash->setBool(DD_LOCKED, locked);

	// low pressure lamp
	bool low_pres = !canwork;
	dash->setBool(DD_LOW_PRESSURE, low_pres);

	// lights
	bool lightsOn = (lights > 0);
	dash->setBool(DD_LIGHTS, lightsOn);

	// Traction Control
	if (tc_present)
	{
		int dash_tc_mode = 1; // 0 = not present, 1 = off, 2 = on, 3 = active
		if (tc_mode)
		{
			if (tractioncontrol)
				dash_tc_mode = 3;
			else
				dash_tc_mode = 2;
		}
		dash->setInt(DD_TRACTIONCONTROL_MODE, dash_tc_mode);
	}

	// Anti Lock Brake
	if (alb_present)
	{
		int dash_alb_mode = 1; // 0 = not present, 1 = off, 2 = on, 3 = active
		if (alb_mode)
		{
			if (antilockbrake)
				dash_alb_mode = 3;
			else
				dash_alb_mode = 2;
		}
		dash->setInt(DD_ANTILOCKBRAKE_MODE, dash_alb_mode);
	}

	// load secured lamp
	if (isTied())
	{
		int ties_mode = 0; // 0 = not locked, 1 = prelock, 2 = lock
		if (fabs(commandkey[0].commandValue) > 0.000001f)
			ties_mode = 1;
		else
			ties_mode = 2;
		dash->setInt(DD_TIES_MODE, ties_mode);
	}

	// Boat things now: screwprops and alike
	if (free_screwprop)
	{
		// the throttle and rudder
		for (int i = 0; i < free_screwprop && i < DD_MAX_SCREWPROP; i++)
		{
			float throttle = screwprops[i]->getThrottle();
			dash->setFloat(DD_SCREW_THROTTLE_0 + i, throttle);

			float steering = screwprops[i]->getRudder();
			dash->setFloat(DD_SCREW_STEER_0 + i, steering);
		}

		// water depth display, only if we have a screw prop at least
		if (cameranodepos[0] >= 0 && cameranodepos[0] < MAX_NODES)
		{
			// position
			Vector3 dir = nodes[cameranodepos[0]].RelPosition - nodes[cameranodedir[0]].RelPosition;
			dir.normalise();

			int low_node = getLowestNode();
			if (low_node != -1)
			{
				Vector3 pos = nodes[low_node].AbsPosition;
				float depth =  pos.y - gEnv->terrainManager->getHeightFinder()->getHeightAt(pos.x, pos.z);
				dash->setFloat(DD_WATER_DEPTH, depth);
			}
		}

		// water speed
		if (cameranodepos[0] >= 0 && cameranodepos[0] < MAX_NODES)
		{
			Vector3 hdir = nodes[cameranodepos[0]].RelPosition - nodes[cameranodedir[0]].RelPosition;
			hdir.normalise();
			float knots = hdir.dotProduct(nodes[cameranodepos[0]].Velocity) * 1.9438f; // 1.943 = m/s in knots/s
			dash->setFloat(DD_WATER_SPEED, knots);
		}
	}

	// now airplane things, aeroengines, etc.
	if (free_aeroengine)
	{
		for (int i = 0; i < free_aeroengine && i < DD_MAX_AEROENGINE; i++)
		{
			float throttle = aeroengines[i]->getThrottle();
			dash->setFloat(DD_AEROENGINE_THROTTLE_0 + i, throttle);

			bool failed = aeroengines[i]->isFailed();
			dash->setBool(DD_AEROENGINE_FAILED_0 + i, failed);

			float pcent = aeroengines[i]->getRPMpc();
			dash->setFloat(DD_AEROENGINE_RPM_0 + i, pcent);
		}
	}

	// wings stuff, you dont need an aeroengine
	if (free_wing)
	{
		for (int i = 0; i < free_wing && i < DD_MAX_WING; i++)
		{
			// Angle of Attack (AOA)
			float aoa = wings[i].fa->aoa;
			dash->setFloat(DD_WING_AOA_0 + i, aoa);
		}
	}

	// some things only activate when a wing or an aeroengine is present
	if (free_wing || free_aeroengine)
	{
		//airspeed
		{
			float ground_speed_kt = nodes[0].Velocity.length() * 1.9438f; // 1.943 = m/s in knots/s

			//tropospheric model valid up to 11.000m (33.000ft)
			float altitude              = nodes[0].AbsPosition.y;
			//float sea_level_temperature = 273.15 + 15.0; //in Kelvin // MAGICs D:
			float sea_level_pressure    = 101325; //in Pa
			//float airtemperature        = sea_level_temperature - altitude * 0.0065f; //in Kelvin
			float airpressure           = sea_level_pressure * pow(1.0f - 0.0065f * altitude / 288.15f, 5.24947f); //in Pa
			float airdensity            = airpressure * 0.0000120896f; //1.225 at sea level

			float knots = ground_speed_kt * sqrt(airdensity / 1.225f); //KIAS
			dash->setFloat(DD_AIRSPEED, knots);
		}

		// altimeter (height above ground)
		{
			float alt = nodes[0].AbsPosition.y * 1.1811f; // MAGIC
			dash->setFloat(DD_ALTITUDE, alt);

			char altc[10];
			sprintf(altc, "%03u", (int)(nodes[0].AbsPosition.y / 30.48f)); // MAGIC
			dash->setChar(DD_ALTITUDE_STRING, altc);
		}
	}

	dash->setFloat(DD_ODOMETER_TOTAL, odometerTotal);
	dash->setFloat(DD_ODOMETER_USER, odometerUser);

	// set the features of this vehicle once
	if (!GUIFeaturesChanged)
	{
		bool hasEngine = (engine != 0);
		bool hasturbo = false;
		bool autogearVisible = false;

		if (hasEngine)
		{
			hasturbo = engine->hasturbo;
			autogearVisible = (engine->getAutoShift() != BeamEngine::MANUALMODE);
		}

		dash->setEnabled(DD_ENGINE_TURBO, hasturbo);
		dash->setEnabled(DD_ENGINE_GEAR, hasEngine);
		dash->setEnabled(DD_ENGINE_NUM_GEAR, hasEngine);
		dash->setEnabled(DD_ENGINE_GEAR_STRING, hasEngine);
		dash->setEnabled(DD_ENGINE_AUTOGEAR_STRING, hasEngine);
		dash->setEnabled(DD_ENGINE_AUTO_GEAR, hasEngine);
		dash->setEnabled(DD_ENGINE_CLUTCH, hasEngine);
		dash->setEnabled(DD_ENGINE_RPM, hasEngine);
		dash->setEnabled(DD_ENGINE_IGNITION, hasEngine);
		dash->setEnabled(DD_ENGINE_BATTERY, hasEngine);
		dash->setEnabled(DD_ENGINE_CLUTCH_WARNING, hasEngine);

		dash->setEnabled(DD_TRACTIONCONTROL_MODE, tc_present);
		dash->setEnabled(DD_ANTILOCKBRAKE_MODE, alb_present);
		dash->setEnabled(DD_TIES_MODE, !ties.empty());
		dash->setEnabled(DD_LOCKED, !hooks.empty());

		dash->setEnabled(DD_ENGINE_AUTOGEAR_STRING, autogearVisible);

		dash->updateFeatures();
		GUIFeaturesChanged = true;
	}

	// TODO: compass value

#if 0
	// ADI - attitude director indicator
	//roll
	Vector3 rollv=curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition-curr_truck->nodes[curr_truck->cameranoderoll[0]].RelPosition;
	rollv.normalise();
	float rollangle=asin(rollv.dotProduct(Vector3::UNIT_Y));

	//pitch
	Vector3 dirv=curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition-curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition;
	dirv.normalise();
	float pitchangle=asin(dirv.dotProduct(Vector3::UNIT_Y));
	Vector3 upv=dirv.crossProduct(-rollv);
	if (upv.y<0) rollangle=3.14159-rollangle;
	ow->adibugstexture->setTextureRotate(Radian(-rollangle));
	ow->aditapetexture->setTextureVScroll(-pitchangle*0.25);
	ow->aditapetexture->setTextureRotate(Radian(-rollangle));

	// HSI - Horizontal Situation Indicator
	Vector3 idir=curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition-curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition;
	//			idir.normalise();
	float dirangle=atan2(idir.dotProduct(Vector3::UNIT_X), idir.dotProduct(-Vector3::UNIT_Z));
	ow->hsirosetexture->setTextureRotate(Radian(dirangle));
	if (curr_truck->autopilot)
	{
		ow->hsibugtexture->setTextureRotate(Radian(dirangle)-Degree(curr_truck->autopilot->heading));
		float vdev=0;
		float hdev=0;
		curr_truck->autopilot->getRadioFix(localizers, free_localizer, &vdev, &hdev);
		if (hdev>15) hdev=15;
		if (hdev<-15) hdev=-15;
		ow->hsivtexture->setTextureUScroll(-hdev*0.02);
		if (vdev>15) vdev=15;
		if (vdev<-15) vdev=-15;
		ow->hsihtexture->setTextureVScroll(-vdev*0.02);
	}

	// VVI - Vertical Velocity Indicator
	float vvi=curr_truck->nodes[0].Velocity.y*196.85;
	if (vvi<1000.0 && vvi>-1000.0) angle=vvi*0.047;
	if (vvi>1000.0 && vvi<6000.0) angle=47.0+(vvi-1000.0)*0.01175;
	if (vvi>6000.0) angle=105.75;
	if (vvi<-1000.0 && vvi>-6000.0) angle=-47.0+(vvi+1000.0)*0.01175;
	if (vvi<-6000.0) angle=-105.75;
	ow->vvitexture->setTextureRotate(Degree(-angle+90.0));


	if (curr_truck->aeroengines[0]->getType() == AeroEngine::AEROENGINE_TYPE_TURBOPROP)
	{
		Turboprop *tp=(Turboprop*)curr_truck->aeroengines[0];
		//pitch
		ow->airpitch1texture->setTextureRotate(Degree(-tp->pitch*2.0));
		//torque
		pcent=100.0*tp->indicated_torque/tp->max_torque;
		if (pcent<60.0) angle=-5.0+pcent*1.9167;
		else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
		else angle=314.0;
		ow->airtorque1texture->setTextureRotate(Degree(-angle));
	}

	if (ftp>1 && curr_truck->aeroengines[1]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOPROP)
	{
		Turboprop *tp=(Turboprop*)curr_truck->aeroengines[1];
		//pitch
		ow->airpitch2texture->setTextureRotate(Degree(-tp->pitch*2.0));
		//torque
		pcent=100.0*tp->indicated_torque/tp->max_torque;
		if (pcent<60.0) angle=-5.0+pcent*1.9167;
		else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
		else angle=314.0;
		ow->airtorque2texture->setTextureRotate(Degree(-angle));
	}

	if (ftp>2 && curr_truck->aeroengines[2]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOPROP)
	{
		Turboprop *tp=(Turboprop*)curr_truck->aeroengines[2];
		//pitch
		ow->airpitch3texture->setTextureRotate(Degree(-tp->pitch*2.0));
		//torque
		pcent=100.0*tp->indicated_torque/tp->max_torque;
		if (pcent<60.0) angle=-5.0+pcent*1.9167;
		else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
		else angle=314.0;
		ow->airtorque3texture->setTextureRotate(Degree(-angle));
	}

	if (ftp>3 && curr_truck->aeroengines[3]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOPROP)
	{
		Turboprop *tp=(Turboprop*)curr_truck->aeroengines[3];
		//pitch
		ow->airpitch4texture->setTextureRotate(Degree(-tp->pitch*2.0));
		//torque
		pcent=100.0*tp->indicated_torque/tp->max_torque;
		if (pcent<60.0) angle=-5.0+pcent*1.9167;
		else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
		else angle=314.0;
		ow->airtorque4texture->setTextureRotate(Degree(-angle));
	}

	//starters
	if (curr_truck->aeroengines[0]->getIgnition()) ow->engstarto1->setMaterialName("tracks/engstart-on"); else ow->engstarto1->setMaterialName("tracks/engstart-off");
	if (ftp>1 && curr_truck->aeroengines[1]->getIgnition()) ow->engstarto2->setMaterialName("tracks/engstart-on"); else ow->engstarto2->setMaterialName("tracks/engstart-off");
	if (ftp>2 && curr_truck->aeroengines[2]->getIgnition()) ow->engstarto3->setMaterialName("tracks/engstart-on"); else ow->engstarto3->setMaterialName("tracks/engstart-off");
	if (ftp>3 && curr_truck->aeroengines[3]->getIgnition()) ow->engstarto4->setMaterialName("tracks/engstart-on"); else ow->engstarto4->setMaterialName("tracks/engstart-off");
}

#endif //0
	dash->update(dt);
#endif // USE_MYGUI
}

Vector3 Beam::getGForces()
{
	if (cameranodepos[0] >= 0 && cameranodepos[0] < MAX_NODES && cameranodedir[0] >= 0 && cameranodedir[0] < MAX_NODES && cameranoderoll[0] >= 0 && cameranoderoll[0] < MAX_NODES)
	{
		Vector3 acc      = cameranodeacc / cameranodecount;
		cameranodeacc    = Vector3::ZERO;
		cameranodecount  = 0;

		float longacc     = acc.dotProduct((nodes[cameranodepos[0]].RelPosition - nodes[cameranodedir[0]].RelPosition).normalisedCopy());
		float latacc      = acc.dotProduct((nodes[cameranodepos[0]].RelPosition - nodes[cameranoderoll[0]].RelPosition).normalisedCopy());
		
		Vector3 diffdir  = nodes[cameranodepos[0]].RelPosition - nodes[cameranodedir[0]].RelPosition;
		Vector3 diffroll = nodes[cameranodepos[0]].RelPosition - nodes[cameranoderoll[0]].RelPosition;
		
		Vector3 upv = diffdir.crossProduct(-diffroll);
		upv.normalise();

		float gravity = DEFAULT_GRAVITY;

		if (gEnv->terrainManager)
		{
			gravity = gEnv->terrainManager->getGravity();
		}

		float vertacc = std::abs(gravity) - acc.dotProduct(-upv);

		return Vector3(vertacc / std::abs(gravity), longacc / std::abs(gravity), latacc / std::abs(gravity));
	}

	return Vector3::ZERO;
}

void Beam::triggerGUIFeaturesChanged()
{
	GUIFeaturesChanged = true;
}

void Beam::engineTriggerHelper(int engineNumber, int type, float triggerValue)
{
	// engineNumber tells us which engine
	BeamEngine* e = engine; // placeholder: trucks do not have multiple engines yet

	switch (type)
	{
	case TRG_ENGINE_CLUTCH:
		if (e) e->setClutch(triggerValue);
		break;
	case TRG_ENGINE_BRAKE:
		brake = triggerValue * brakeforce;
		break;
	case TRG_ENGINE_ACC:
		if (e) e->setAcc(triggerValue);
		break;
	case TRG_ENGINE_RPM:
		// TODO: Implement setTargetRPM in the BeamEngine.cpp
		break;
	case TRG_ENGINE_SHIFTUP:
		if (e) e->shift(1);
		break;
	case TRG_ENGINE_SHIFTDOWN:
		if (e) e->shift(-1);
		break;
	default:
		break;
	}
}

void Beam::runThreadTask(Beam* truck, ThreadTask task, bool shared /*= false*/)
{
	ThreadTask old_thread_task = truck->thread_task;

	truck->thread_index = 0;
	truck->thread_number = 1;
	truck->thread_task = task;

	if (gEnv->threadPool && (!shared || gEnv->threadPool->getSize() / truck->num_simulated_trucks > 1))
	{
		truck->thread_number = gEnv->threadPool->getSize();
		if (shared)
		{
			truck->thread_number /= truck->num_simulated_trucks;
		}
		truck->task_count[task] = truck->thread_number;

		std::list<IThreadTask*> tasks;

		// Push tasks into thread pool
		for (int i=0; i<truck->thread_number; i++)
		{
			tasks.emplace_back(truck);
		}

		gEnv->threadPool->enqueue(tasks);

		// Wait for all tasks to complete
		MUTEX_LOCK(&truck->task_count_mutex[task]);
		while (truck->task_count[task] > 0)
		{
			pthread_cond_wait(&truck->task_count_cv[task], &truck->task_count_mutex[task]);
		}
		MUTEX_UNLOCK(&truck->task_count_mutex[task]);
	} else
	{
		truck->run();
	}

	truck->thread_task = old_thread_task;
}

void Beam::run()
{
	if (thread_task == THREAD_BEAMFORCESEULER)
	{
		calcForcesEulerCompute(curtstep==0, dtperstep, curtstep, tsteps);
		if (!disableTruckTruckSelfCollisions)
		{
			intraTruckCollisionsPrepare(dtperstep);
			intraTruckCollisionsCompute(dtperstep);
			intraTruckCollisionsFinal(dtperstep);
		}
	} else
	{
		int index = 0;
		if (thread_number > 1)
		{
			MUTEX_LOCK(&task_index_mutex[thread_task]);
			index = thread_index;
			thread_index++;
			MUTEX_UNLOCK(&task_index_mutex[thread_task]);
		}

		switch (thread_task)
		{
		case THREAD_BEAMS:
			calcBeams(curtstep==0, dtperstep, curtstep, tsteps, index, thread_number);
			break;
		case THREAD_INTRA_TRUCK_COLLISIONS:
			intraTruckCollisionsCompute(dtperstep, index, thread_number);
			break;
		case THREAD_INTER_TRUCK_COLLISIONS:
			interTruckCollisionsCompute(dtperstep, index, thread_number);
			break;
		case THREAD_NODES:
			calcNodes(curtstep==0, dtperstep, curtstep, tsteps, index, thread_number);
			break;
		}
	}
}

void Beam::onComplete()
{
	if (thread_task == THREAD_BEAMFORCESEULER)
	{
		MUTEX_LOCK(&calledby->task_count_mutex[thread_task]);
		calledby->task_count[thread_task]--;
		MUTEX_UNLOCK(&calledby->task_count_mutex[thread_task]);
		if (!calledby->task_count[thread_task])
		{
			pthread_cond_signal(&calledby->task_count_cv[thread_task]);
		}
	} else
	{
		MUTEX_LOCK(&task_count_mutex[thread_task]);
		task_count[thread_task]--;
		MUTEX_UNLOCK(&task_count_mutex[thread_task]);
		if (!task_count[thread_task])
		{
			pthread_cond_signal(&task_count_cv[thread_task]);
		}
	}
}
