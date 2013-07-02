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
#ifndef __BEAM_H_
#define __BEAM_H_

#include "RoRPrerequisites.h"
#include "OgrePrerequisites.h"

#include "BeamData.h"
#include "CacheSystem.h"
#include "IThreadTask.h"
#include "SerializedRig.h"
#include "Streamable.h"

class Beam :
	public IThreadTask,
	public SerializedRig,
	public Streamable
{
public:
	Beam() {}; // for wrapper, DO NOT USE!
	~Beam();

#ifdef USE_ANGELSCRIPT
	// we have to add this to be able to use the class as reference inside scripts
	void addRef(){};
	void release(){};
#endif

	//constructor
	Beam( int tnum
		, Ogre::Vector3 pos
		, Ogre::Quaternion rot
		, const char* fname
		, bool networked = false
		, bool networking = false
		, collision_box_t *spawnbox = NULL
		, bool ismachine=false
		, int flareMode = 0
		, const std::vector<Ogre::String> *truckconfig = 0
		, Skin *skin = 0
		, bool freeposition = false);

	//! @{ network related functions
	void pushNetwork(char* data, int size);
	void calcNetwork();
	void updateNetworkInfo();
	//! @}

	//! @{ physic related functions
	void activate();
	void desactivate();
	void addPressure(float v);
	float getPressure();
	Ogre::Vector3 getPosition();
	void resetAngle(float rot);
	void resetPosition(float px, float pz, bool setInitPosition, float miny);
	void resetPosition(float px, float pz, bool setInitPosition);
	void resetPosition(Ogre::Vector3 translation, bool setInitPosition);
	void reset(bool keepPosition = false); //call this one to reset a truck from any context
	//this is called by the beamfactory worker thread
	void threadentry();
	//this will be called once by frame and is responsible for animation of all the trucks!
	//the instance called is the one of the current ACTIVATED truck
	bool frameStep(Ogre::Real dt);
	//! @}

	//! @{ audio related functions
	void setupDefaultSoundSources();
	void updateSoundSources();
	//! @}

	//! @{ user interaction functions
	void mouseMove(int node, Ogre::Vector3 pos, float force);
	void lightsToggle();
	void tieToggle(int group=-1);
	void ropeToggle(int group=-1);
	void hookToggle(int group=-1, int mode=HOOK_TOGGLE, int node_number=-1);
	void engineTriggerHelper(int engineNumber, int type, float triggerValue);
	void toggleSlideNodeLock();
	void toggleCustomParticles();
	void toggleAxleLock();	//! diff lock on or off
	void parkingbrakeToggle();
	void antilockbrakeToggle();
	void tractioncontrolToggle();
	void cruisecontrolToggle();
	void beaconsToggle();
	void forwardCommands();
	void setReplayMode(bool rm);
	int savePosition(int position);
	int loadPosition(int position);
	void updateTruckPosition();
	//! @}

	//! @{ ground
	ground_model_t *getLastFuzzyGroundModel();
	//! @}

	//! @{ graphical display things */
	void updateSkidmarks();
	void updateAI(float dt);
	void prepareInside(bool inside);
	void updateFlares(float dt, bool isCurrent=false);
	void updateProps();
	void updateVisual(float dt=0);
	void updateVisualPrepare(float dt=0);
	void updateVisualFinal(float dt=0);
	void updateLabels(float dt=0);
	//v=0: full detail
	//v=1: no beams
	void setDetailLevel(int v);
	void showSkeleton(bool meshes=true, bool newMode=false, bool linked=true);
	void hideSkeleton(bool newMode=false, bool linked=true);
	void updateSimpleSkeleton();
	void updateSkeletonColouring(int doUpdate);
	void resetAutopilot();
	void disconnectAutopilot();
	void scaleTruck(float value);
	float currentScale;
	void updateDebugOverlay();
	void setDebugOverlayState(int mode);

	//! @{ dynamic physical properties
	Ogre::Real brake;
	Ogre::Vector3 affforce;
	Ogre::Vector3 ffforce;
	Ogre::Real affhydro;
	Ogre::Real ffhydro;

	bool left_blink_on, right_blink_on, warn_blink_on;
	//! @}

	/* functions to be sorted */
	Ogre::Quaternion specialGetRotationTo(const Ogre::Vector3& src, const Ogre::Vector3& dest) const;
	Ogre::String getAxleLockName();	//! get the name of the current differential model
	int getAxleLockCount();
	std::vector< std::vector< int > > nodetonodeconnections;
	std::vector< std::vector< int > > nodebeamconnections;

	// wheel speed in m/s
	float WheelSpeed;
	float getWheelSpeed() { return WheelSpeed; }
	Ogre::Vector3 getGForces();

	int stabcommand;
	int skeleton;
	float stabratio;
	//direction
	float hydrodircommand;
	bool hydroSpeedCoupling;
	float hydrodirstate;
	Ogre::Real hydrodirwheeldisplay;
	//extra airplane axes
	float hydroaileroncommand;
	float hydroaileronstate;
	float hydroruddercommand;
	float hydrorudderstate;
	float hydroelevatorcommand;
	float hydroelevatorstate;

	bool canwork;
	bool replaymode;
	bool watercontact;
	bool watercontactold;
	int locked;
	int lockedold;
	int oldreplaypos;
	int replaylen;
	int replaypos;
	int sleepcount;
	//can this be driven?
	int previousGear;
	ground_model_t *submesh_ground_model;
	bool requires_wheel_contact;
	int parkingbrake;
	int lights;
	bool reverselight;
	float leftMirrorAngle;
	float rightMirrorAngle;
	float refpressure;

	bool hasDriverSeat();
	int calculateDriverPos(Ogre::Vector3 &pos, Ogre::Quaternion &rot);
	float getSteeringAngle();
	void triggerGUIFeaturesChanged();

	float elevator;
	float rudder;
	float aileron;
	int flap;

	Ogre::Vector3 fusedrag;
	
	bool disableDrag;
	bool disableTruckTruckCollisions;
	bool disableTruckTruckSelfCollisions;
	int currentcamera;
	
	int first_wheel_node;
	int netbuffersize;
	int nodebuffersize;
	Ogre::SceneNode *netLabelNode;

	std::string getTruckName();
	std::string getTruckFileName();
	std::string getTruckHash();
	int getTruckType();
	
	std::vector<authorinfo_t> getAuthors();
	std::vector<std::string> getDescription();

	int getBeamCount();
	beam_t *getBeams();
	float getDefaultDeformation();

	int getNodeCount();
	node_t *getNodes();
	int nodeBeamConnections(int nodeid);

	void changedCamera();

	float getTotalMass(bool withLocked=true);
	void recalc_masses();
	int getWheelNodeCount();
	void setMass(float m);

	beam_t *addBeam(int id1, int id2);
	node_t *addNode(Ogre::Vector3 pos);

	Ogre::String realtruckfilename;

	/*
	 *@return Returns a list of all connected (hooked) beams 
	 */
	std::list<Beam*> getAllLinkedBeams() { return linkedBeams; };

	// this must be in the header as the network stuff is using it...
	bool getBrakeLightVisible();

	bool getReverseLightVisible();

	bool getCustomLightVisible(int number);

	void setCustomLightVisible(int number, bool visible);

	bool getBeaconMode();
	void setBlinkType(blinktype blink);
	blinktype getBlinkType();
	void deleteNetTruck();
	
	float getHeadingDirectionAngle();
	bool getCustomParticleMode();
	int getLowestNode();
	void preMapLabelRenderUpdate(bool mode, float cheight=0);
	
	float tdt;
	float ttdt;
	bool simulated;
	int airbrakeval;
	Ogre::Vector3 cameranodeacc;
	int cameranodecount;

	void setBeamVisibility(bool visible, bool linked=true);
	bool beamsVisible;
	void setMeshVisibility(bool visible, bool linked=true);
	bool meshesVisible;
	bool inRange(float num, float min, float max);

	int getTruckTime();
	int getNetTruckTimeOffset();
	Ogre::Real getMinimalCameraRadius();

	Replay *getReplay();

	bool getSlideNodesLockInstant();
	void sendStreamData();
	bool isTied();
	bool isLocked();

	Ogre::SceneNode *getSceneNode() { return beamsRoot; }

#ifdef USE_MYGUI
	DashBoardManager *dash;
#endif // USE_MYGUI

	Beam* calledby;

	// IThreadTask
	void run();
	void onComplete();

	// flexable pthread stuff
	int flexable_task_count;
	pthread_cond_t flexable_task_count_cv;
	pthread_mutex_t flexable_task_count_mutex;

protected:

	enum ThreadTask {
		THREAD_BEAMFORCESEULER,
		THREAD_BEAMS,
		THREAD_INTER_TRUCK_COLLISIONS,
		THREAD_INTRA_TRUCK_COLLISIONS,
		THREAD_NODES,
		THREAD_MAX
	};

	//! @{ physic related functions
	bool calcForcesEulerPrepare(int doUpdate, Ogre::Real dt, int step = 0, int maxsteps = 1);
	void calcForcesEulerCompute(int doUpdate, Ogre::Real dt, int step = 0, int maxsteps = 1);
	void calcForcesEulerFinal(int doUpdate, Ogre::Real dt, int step = 0, int maxsteps = 1);
	void intraTruckCollisionsPrepare(Ogre::Real dt);
	void intraTruckCollisionsCompute(Ogre::Real dt, int chunk_index = 0, int chunk_number = 1);
	void intraTruckCollisionsFinal(Ogre::Real dt);
	void interTruckCollisionsPrepare(Ogre::Real dt);
	void interTruckCollisionsCompute(Ogre::Real dt, int chunk_index = 0, int chunk_number = 1);
	void interTruckCollisionsFinal(Ogre::Real dt);
	void calcBeams(int doUpdate, Ogre::Real dt, int step, int maxsteps, int chunk_index = 0, int chunk_number = 1);
	void calcNodes(int doUpdate, Ogre::Real dt, int step, int maxsteps, int chunk_index = 0, int chunk_number = 1);
	void calcHooks();
	void calcRopes();
	void calcShocks2(int beam_i, Ogre::Real difftoBeamL, Ogre::Real &k, Ogre::Real &d, Ogre::Real dt, int update);
	void calcAnimators(int flagstate, float &cstate, int &div, float timer, float opt1, float opt2, float opt3);

	void SyncReset(); //this one should be called only synchronously (without physics running in background)

	float dtperstep;
	int curtstep;
	int tsteps;
	int num_simulated_trucks;
	float avichatter_timer;

	// pthread stuff
	int task_count[THREAD_MAX];
	pthread_cond_t task_count_cv[THREAD_MAX];
	pthread_mutex_t task_count_mutex[THREAD_MAX];
	pthread_mutex_t task_index_mutex[THREAD_MAX];

	ThreadTask thread_task;
	int thread_index;
	int thread_number;

	void runThreadTask(Beam* truck, ThreadTask task, bool shared = false);
	
	// inter-/intra truck collision stuff
	pthread_mutex_t itc_node_access_mutex;

	std::vector<PointColDetector*> interPointCD;
	std::vector<PointColDetector*> intraPointCD;

	// flexable stuff
	std::bitset<MAX_WHEELS> flexmesh_prepare;
	std::bitset<MAX_FLEXBODIES> flexbody_prepare;

	// linked beams (hooks)
	std::list<Beam*> linkedBeams;
	void determineLinkedBeams();

	void calc_masses2(Ogre::Real total, bool reCalc=false);
	void calcNodeConnectivityGraph();
	void updateContacterNodes();
	void moveOrigin(Ogre::Vector3 offset); //move physics origin
	void changeOrigin(Ogre::Vector3 newOrigin); //change physics origin

	void updateDashBoards(float &dt);
	
	Ogre::Vector3 position;
	Ogre::Vector3 iPosition; // initial position
	Ogre::Vector3 lastposition;
	Ogre::Vector3 lastlastposition;
	Ogre::Real minCameraRadius;

	Ogre::Real replayTimer;
	Ogre::Real replayPrecision;

	ground_model_t *lastFuzzyGroundModel;

	// this is for managing the blinkers on the truck:
	blinktype blinkingtype;

	bool deleting;
	
	Ogre::Real hydrolen;
	
	Ogre::SceneNode *smokeNode;
	Ogre::ParticleSystem* smoker;
	float stabsleep;
	Replay *replay;
	PositionStorage *posStorage;

	bool cparticle_mode;
	Beam** ttrucks;
	int tnumtrucks;
	int detailLevel;
	bool increased_accuracy;
	bool isInside;
	bool beacon;
	float totalmass;

	int mousenode;
	Ogre::Vector3 mousepos;
	float mousemoveforce;
	int reset_requested;

	std::vector<Ogre::String> truckconfig;

	oob_t *oob1;
	oob_t *oob2;
	oob_t *oob3;
	char *netb1;
	char *netb2;
	char *netb3;
	pthread_mutex_t net_mutex;
	Ogre::Timer *nettimer;
	int net_toffset;
	int netcounter;
	Ogre::MovableText *netMT; //, *netDist;

	// network properties
	Ogre::String networkUsername;
	int networkAuthlevel;

	bool netBrakeLight, netReverseLight;
	Ogre::Real mTimeUntilNextToggle;

	void checkBeamMaterial();

	// cab fading stuff - begin
	void cabFade(float amount);
	void setMeshWireframe(Ogre::SceneNode *node, bool value);
	void fadeMesh(Ogre::SceneNode *node, float amount);
	float getAlphaRejection(Ogre::SceneNode *node);
	void setAlphaRejection(Ogre::SceneNode *node, float amount);
	float cabFadeTimer;
	float cabFadeTime;
	int cabFadeMode;
	// cab fading stuff - end
	bool lockSkeletonchange;
	bool floating_origin_enable;

	Ogre::ManualObject *simpleSkeletonManualObject;
	Ogre::SceneNode *simpleSkeletonNode;
	bool simpleSkeletonInitiated;
	void initSimpleSkeleton();

	int loadTruck2(Ogre::String filename, Ogre::SceneNode *parent, Ogre::Vector3 pos, Ogre::Quaternion rot, collision_box_t *spawnbox);

	/**
	 * Resets the turn signal when the steering wheel is turned back.
	 */
	void autoBlinkReset();
	bool blinktreshpassed;

#ifdef FEAT_TIMING
	BeamThreadStats *statistics, *statistics_gfx;
#endif


	// overloaded from Streamable:
	Ogre::Timer netTimer;
	int last_net_time;
	void sendStreamSetup();
	void receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len);

	// dustpools
	DustPool *dustp;
    DustPool *dripp;
    DustPool *sparksp;
    DustPool *clumpp;
    DustPool *splashp;
    DustPool *ripplep;

	// SLIDE NODES /////////////////////////////////////////////////////////////
	//! true if SlideNodes are locked, false if not
	bool SlideNodesLocked;
	bool GUIFeaturesChanged;


	/**
	 * calculate and apply Corrective forces
	 * @param dt delta time in seconds
	 */
	void updateSlideNodeForces(const Ogre::Real dt);
	//! Recalculate SlideNode positions
	void resetSlideNodePositions();
	//! Reset all the SlideNodes
	void resetSlideNodes();
	//! incrementally update the position of all SlideNodes
	void updateSlideNodePositions();

	/**
	 *
	 * @param truck which truck to retrieve the closest Rail from
	 * @param node which SlideNode is being checked against
	 * @return a pair containing the rail, and the distant to the SlideNode
	 */
	std::pair<RailGroup*, Ogre::Real> getClosestRailOnTruck( Beam* truck, const SlideNode& node);
};


// BEAM Inlined methods ////////////////////////////////////////////////////////

inline ground_model_t* Beam::getLastFuzzyGroundModel()
{
	return lastFuzzyGroundModel;
}

inline float Beam::getSteeringAngle()
{
	return hydrodircommand;
}

inline std::string Beam::getTruckName()
{
	return realtruckname;
}

inline std::string Beam::getTruckFileName()
{
	return realtruckfilename;
}

inline int Beam::getTruckType()
{
	return driveable;
}


inline std::string Beam::getTruckHash()
{
	return beamHash;
}


inline std::vector<authorinfo_t> Beam::getAuthors()
{
	return authors;
}

inline std::vector<std::string> Beam::getDescription()
{
	return description;
}

inline int Beam::getBeamCount()
{
	return free_beam;
}

inline beam_t* Beam::getBeams()
{
	return beams;
}

inline float Beam::getDefaultDeformation()
{
	return default_deform;
}

inline int Beam::getNodeCount()
{
	return free_node;
}

inline node_t* Beam::getNodes()
{
	return nodes;
}

inline void Beam::setMass(float m)
{
	truckmass = m;
}

inline bool Beam::getBrakeLightVisible()
{
	if (state==NETWORKED)
		return netBrakeLight;

//		return (brake > 0.15 && !parkingbrake);
	return (brake > 0.15);
}

inline bool Beam::getCustomLightVisible(int number)
{
	return netCustomLightArray[number] != -1
			&& flares[netCustomLightArray[number]].controltoggle_status;
}

inline void Beam::setCustomLightVisible(int number, bool visible)
{
	if (netCustomLightArray[number] == -1)
		return;
	flares[netCustomLightArray[number]].controltoggle_status = visible;
}


inline bool Beam::getBeaconMode()
{
	return beacon;
}

inline blinktype Beam::getBlinkType()
{
	return blinkingtype;
}

inline bool Beam::getCustomParticleMode()
{
	return cparticle_mode;
}

inline int Beam::getLowestNode()
{
	return lowestnode;
}

inline int Beam::getTruckTime()
{
	return nettimer->getMilliseconds();
}

inline int Beam::getNetTruckTimeOffset()
{
	return net_toffset;
}

inline Ogre::Real Beam::getMinimalCameraRadius()
{
	return minCameraRadius;
}

inline Replay* Beam::getReplay()
{
	return replay;
}

inline bool Beam::getSlideNodesLockInstant()
{
	return slideNodesConnectInstantly;
}

inline bool Beam::inRange(float num, float min, float max)
{
	return (num <= max && num >= min);
}

#endif // __BEAM_H_
