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

#pragma once

#include "RoRPrerequisites.h"
#include "OgrePrerequisites.h"
#include "RigDef_Prerequisites.h"

#include "BeamData.h"
#include "IThreadTask.h"
#include "Streamable.h"

#include <OgreTimer.h>
#include <OgreOverlayElement.h>
#include <vector>

/** 
* Represents an entire rig (any vehicle type)
* Contains logic related to physics, network, sound, threading, rendering. It's a bit of a monster class :(
*/
class Beam :
	public IThreadTask,
	public rig_t,
	public Streamable
{
	friend class RigSpawner;
	friend class RigInspector; // Debug utility class

public:
	Beam() {}; // for wrapper, DO NOT USE!
	~Beam();

#ifdef USE_ANGELSCRIPT
	// we have to add this to be able to use the class as reference inside scripts
	void addRef(){};
	void release(){};
#endif

	/**
	* Constructor
	*
	* @param tnum Vehicle number (alias Truck Number)
	* @param pos
	* @param rot
	* @param fname Rig file name.
	* @param ismachine (see BeamData.h)
	* @param truckconfig Networking related.
	* @param preloaded_with_terrain Is this rig being pre-loaded along with terrain?
    * @param cache_entry_number Needed for flexbody caching. Pass -1 if unavailable (flexbody caching will be disabled)
	*/
	Beam( 
		  int tnum
		, Ogre::Vector3 pos
		, Ogre::Quaternion rot
		, const char* fname
        , RoR::RigLoadingProfiler* rig_loading_profiler
		, bool networked = false
		, bool networking = false
		, collision_box_t *spawnbox = nullptr
		, bool ismachine = false
		, const std::vector<Ogre::String> *truckconfig = nullptr
		, Skin *skin = nullptr
		, bool freeposition = false
		, bool preloaded_with_terrain = false
        , int cache_entry_number = -1
		);

	/**
	* Parses network data; fills truck data buffers and flips them. Called by the network thread.
	*/
	void pushNetwork(char* data, int size);
	void calcNetwork();

	void updateNetworkInfo();

	
	void activate();
	void desactivate();
	void addPressure(float v);
	float getPressure();
	Ogre::Vector3 getPosition();
	void resetAngle(float rot);
	void resetPosition(float px, float pz, bool setInitPosition, float miny);
	void resetPosition(float px, float pz, bool setInitPosition);

	/**
	* Moves vehicle.
	* @param translation Offset to move vehicle.
	* @param setInitPosition Set initial positions of nodes to current position?
	*/
	void resetPosition(Ogre::Vector3 translation, bool setInitPosition);

	/**
	* Call this one to reset a truck from any context
	*/
	void reset(bool keepPosition = false); 

	/**
	* this is called by the beamfactory worker thread
	*/
	void threadentry();

	/**
	* Spawns vehicle.
	*/
	bool LoadTruck(
        RoR::RigLoadingProfiler* rig_loading_profiler,
		Ogre::String const & file_name, 
		Ogre::SceneNode *parent_scene_node, 
		Ogre::Vector3 const & spawn_position,
		Ogre::Quaternion & spawn_rotation,
		collision_box_t *spawn_box,
		bool preloaded_with_terrain = false,
        int cache_entry_number = -1
	);

	/**
	* TIGHT-LOOP; Called once by frame and is responsible for animation of all the trucks!
	* the instance called is the one of the current ACTIVATED truck
	*/
	bool frameStep(Ogre::Real dt);

	void setupDefaultSoundSources();

	void updateSoundSources();

	/**
	* Event handler
	*/
	void mouseMove(int node, Ogre::Vector3 pos, float force);

	/**
	* Event handler
	*/
	void lightsToggle();

	/**
	* Event handler
	*/
	void tieToggle(int group=-1);

	/**
	* Event handler
	*/
	void ropeToggle(int group=-1);

	/**
	* Event handler
	*/
	void hookToggle(int group=-1, hook_states mode=HOOK_TOGGLE, int node_number=-1);
	void engineTriggerHelper(int engineNumber, int type, float triggerValue);
	void toggleSlideNodeLock();
	void toggleCustomParticles();
	void toggleAxleLock();	//! diff lock on or off

	/**
	* Event handler
	*/
	void parkingbrakeToggle();

	/**
	* Event handler
	*/
	void antilockbrakeToggle();

	/**
	* Event handler
	*/
	void tractioncontrolToggle();

	/**
	* Event handler
	*/
	void cruisecontrolToggle();

	/**
	* Event handler
	*/
	void beaconsToggle();
	void forwardCommands();
	
	/**
	* Event handler; toggle replay mode.
	*/
	void setReplayMode(bool rm);
	int savePosition(int position);
	int loadPosition(int position);
	void updateTruckPosition();

	/**
	* Ground.
	*/
	ground_model_t *getLastFuzzyGroundModel();

	/**
	* Creates or updates skidmarks. No side effects.
	*/
	void updateSkidmarks();
	void updateAI(float dt);

	/**
	* Prepares vehicle for in-cabin camera use.
	*/
	void prepareInside(bool inside);
	
	/**
	* Display.
	*/
	void updateFlares(float dt, bool isCurrent=false);

	/**
	* TIGHT-LOOP; Display; updates positions of props.
	* Each prop has scene node parented to root-node (only_a_ptr 11/2013).
	*/
	void updateProps();

	/**
	* TIGHT-LOOP; Logic: display, sound, particles
	* @see updateVisualPrepare
	* @see updateVisualFinal
	*/
	void updateVisual(float dt=0);

	/**
	* TIGHT-LOOP; Logic: display (+flexbodies +overlays +particles), sound, threading
	* Does a mixture of tasks:
	* - Sound: updates sound sources; plays aircraft radio chatter; 
	* - Particles: updates particles (dust, exhausts, custom)
	* - Display: updates wings; updates props; updates rig-skeleton + cab fade effect; updates flexbodies; updates debug overlay
	*/
	void updateVisualPrepare(float dt=0);

	/**
	* TIGHT-LOOP; Logic: display (+flexbodies), threading
	*/
	void updateVisualFinal(float dt=0);
	void updateLabels(float dt=0);

	/**
	* Display
	* @param v 0 = full detail, 1 = no beams
	*/
	void setDetailLevel(int v);

	/** 
	* Display; displays "skeleton" (visual rig) mesh.
	*/
	void showSkeleton(bool meshes=true, bool newMode=false, bool linked=true);

	/** 
	* Display; hides "skeleton" (visual rig) mesh.
	*/
	void hideSkeleton(bool newMode=false, bool linked=true);

	/** 
	* Display; updates the "skeleton" (visual rig) mesh.
	*/
	void updateSimpleSkeleton();

	/** 
	* TIGHT-LOOP; Display; changes coloring of the "skeleton" (visual rig) mesh.
	*/
	void updateSkeletonColouring(int doUpdate);
	void resetAutopilot();
	void disconnectAutopilot();
	void scaleTruck(float value);
	float currentScale;

	/**
	* TIGHT-LOOP (optional); 
	*/
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

	
	//! @{ calc forces euler division
	void calcTruckEngine(bool doUpdate, Ogre::Real dt);
	void calcBeams(bool doUpdate, Ogre::Real dt, int step, int maxsteps);
	void calcAnimatedProps(bool doUpdate, Ogre::Real dt);
	void calcHooks(bool doUpdate);
	void calcForceFeedBack(bool doUpdate);
	void calcMouse();
	void calcNodes_(bool doUpdate, Ogre::Real dt, int step, int maxsteps);
	void calcSlideNodes(Ogre::Real dt);
	void calcTurboProp(bool doUpdate, Ogre::Real dt);
	void calcScrewProp(bool doUpdate);
	void calcWing();
	void calcFuseDrag();
	void calcAirBrakes();
	void calcBuoyance(bool doUpdate, Ogre::Real dt, int step, int maxsteps);
	void calcAxles(bool doUpdate, Ogre::Real dt);
	void calcWheels(bool doUpdate, Ogre::Real dt, int step, int maxsteps);
	void calcShocks(bool doUpdate, Ogre::Real dt);
	void calcHydros(bool doUpdate, Ogre::Real dt);
	void calcCommands(bool doUpdate, Ogre::Real dt);
	void calcReplay(bool doUpdate, Ogre::Real dt);
	//! @}
	
	//! @{ helper routines

	void calcBeam(beam_t& beam, bool doUpdate, Ogre::Real dt, int& increased_accuracy);
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

	int stabcommand; //!< Stabilization; values: { -1, 0, 1 }
	int skeleton; //!< Visibility of "skeleton" (visual rig) { 0 = not visible, 1 = visible, 2 = visible in "newMode" }
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

	/**
	* Returns the number of active (non bounded) beams connected to a node
	*/
	int nodeBeamConnections(int nodeid);

	/**
	* Logic: sound, display; Notify this vehicle that camera changed; 
	*/
	void changedCamera();

	void StopAllSounds();
	void UnmuteAllSounds();

	float getTotalMass(bool withLocked=true);
	void recalc_masses();
	int getWheelNodeCount();
	void setMass(float m);

	Ogre::String realtruckfilename;

	/**
	 * @return Returns a list of all connected (hooked) beams 
	 */
	std::list<Beam*> getAllLinkedBeams() { return linkedBeams; };

	/** 
	* This must be in the header as the network stuff is using it...
	*/
	bool getBrakeLightVisible();

	/**
	* Tells if the reverse-light is currently lit.
	*/
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

	/**
	* Sets visibility of all beams on this vehicle
	* @param visible Toggle
	* @param linked Apply to linked vehicles also?
	*/
	void setBeamVisibility(bool visible, bool linked=true);

	bool beamsVisible; //!< Are beams visible? @see setBeamVisibility

	/**
	* Sets visibility of all meshes on this vehicle
	* @param visible Toggle
	* @param linked Apply to linked vehicles also?
	*/
	void setMeshVisibility(bool visible, bool linked=true);

	bool meshesVisible; //!< Are meshes visible? @see setMeshVisibility

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

	/**
	* Overrides IThreadTask::run()
	*/
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

	/**
	* TIGHT LOOP; Physics; 
	*/
	void calcForcesEulerCompute(int doUpdate, Ogre::Real dt, int step = 0, int maxsteps = 1);

	/**
	* TIGHT LOOP; Physics; 
	*/
	void calcForcesEulerFinal(int doUpdate, Ogre::Real dt, int step = 0, int maxsteps = 1);
	void intraTruckCollisionsPrepare(Ogre::Real dt);
	void intraTruckCollisionsCompute(Ogre::Real dt, int chunk_index = 0, int chunk_number = 1);
	void intraTruckCollisionsFinal(Ogre::Real dt);
	void interTruckCollisionsPrepare(Ogre::Real dt);
	void interTruckCollisionsCompute(Ogre::Real dt, int chunk_index = 0, int chunk_number = 1);
	void interTruckCollisionsFinal(Ogre::Real dt);

	/**
	* TIGHT LOOP; Physics & sound; 
	* @param doUpdate Only passed to Beam::calcShocks2()
	*/
	void calcBeams(int doUpdate, Ogre::Real dt, int step, int maxsteps, int chunk_index, int chunk_number);

	/**
	* TIGHT LOOP; Physics; 
	* @param doUpdate Unused (overwritten in function)
	*/
	void calcNodes(int doUpdate, Ogre::Real dt, int step, int maxsteps, int chunk_index, int chunk_number);

	/**
	* TIGHT LOOP; Physics; 
	*/
	void calcHooks();

	/**
	* TIGHT LOOP; Physics; 
	*/
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

	std::vector<Ogre::String> m_truck_config;

	oob_t *oob1; //!< Network; Triple buffer for incoming data (truck properties)
	oob_t *oob2; //!< Network; Triple buffer for incoming data (truck properties)
	oob_t *oob3; //!< Network; Triple buffer for incoming data (truck properties)
	char *netb1; //!< Network; Triple buffer for incoming data
	char *netb2; //!< Network; Triple buffer for incoming data
	char *netb3; //!< Network; Triple buffer for incoming data
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

	void CreateSimpleSkeletonMaterial();

	// cab fading stuff - begin
	void cabFade(float amount);
	void setMeshWireframe(Ogre::SceneNode *node, bool value);
	void fadeMesh(Ogre::SceneNode *node, float amount);
	float getAlphaRejection(Ogre::SceneNode *node);
	void setAlphaRejection(Ogre::SceneNode *node, float amount);
	float cabFadeTimer;
	float cabFadeTime;
	int cabFadeMode; //<! Cab fading effect; values { -1, 0, 1, 2 }
	// cab fading stuff - end
	bool lockSkeletonchange;
	bool floating_origin_enable;

	Ogre::ManualObject *simpleSkeletonManualObject;
	Ogre::SceneNode *simpleSkeletonNode;
	bool simpleSkeletonInitiated; //!< Was the rig-skeleton mesh built?
	/** 
	* Builds the rig-skeleton mesh.
	*/
	void initSimpleSkeleton();

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
