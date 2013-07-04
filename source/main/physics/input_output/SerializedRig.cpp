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
#include "SerializedRig.h"

#include "AirBrake.h"
#include "Airfoil.h"
#include "AutoPilot.h"
#include "BeamEngine.h"
#include "Buoyance.h"
#include "CacheSystem.h"
#include "CmdKeyInertia.h"
#include "Console.h"
#include "Differentials.h"
#include "FlexAirfoil.h"
#include "FlexBody.h"
#include "FlexMesh.h"
#include "FlexMeshWheel.h"
#include "FlexObj.h"
#include "InputEngine.h"
#include "JSON.h"
#include "MaterialFunctionMapper.h"
#include "MaterialReplacer.h"
#include "MeshObject.h"
#include "RoRFrameListener.h"
#include "RoRVersion.h"
#include "SHA1.h"
#include "ScopeLog.h"
#include "ScrewProp.h"
#include "Settings.h"
#include "Skin.h"
#include "SlideNode.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "TorqueCurve.h"
#include "TurboJet.h"
#include "TurboProp.h"
#include "VideoCamera.h"

using namespace Ogre;

const std::map<std::string, int> SerializedRig::truck_sections = init_truck_sections();

SerializedRig::SerializedRig()
{
	mCamera=0;
	// clear rig parent structure
	memset(this->nodes, 0, sizeof(node_t) * MAX_NODES); free_node = 0;
	memset(this->beams, 0, sizeof(beam_t) * MAX_BEAMS); free_beam = 0;
	memset(this->contacters, 0, sizeof(contacter_t) * MAX_CONTACTERS); free_contacter = 0;
	memset(this->rigidifiers, 0, sizeof(rigidifier_t) * MAX_RIGIDIFIERS); free_rigidifier = 0;
	memset(this->wheels, 0, sizeof(wheel_t) * MAX_WHEELS); free_wheel = 0;
	memset(this->vwheels, 0, sizeof(vwheel_t) * MAX_WHEELS);
	ropes.clear();
	ropables.clear();
	ties.clear();
	hooks.clear();
	memset(this->wings, 0, sizeof(wing_t) * MAX_WINGS); free_wing = 0;

	// commands contain complex data structures, do not memset them ...
	for (int i=0;i<MAX_COMMANDS+1;i++)
	{
		this->commandkey[i].commandValue=0;
		this->commandkey[i].beams.clear();
		this->commandkey[i].rotators.clear();
		this->commandkey[i].description="";
	}

	memset(this->rotators, 0, sizeof(rotator_t) * MAX_ROTATORS); free_rotator = 0;
	flares.clear(); free_flare = 0;
	memset(this->props, 0, sizeof(prop_t) * MAX_PROPS); free_prop = 0;
	driverSeat=0;
	memset(this->shocks, 0, sizeof(shock_t) * MAX_SHOCKS); free_shock = 0; free_active_shock = 0;
	exhausts.clear();
	memset(this->cparticles, 0, sizeof(cparticle_t) * MAX_CPARTICLES); free_cparticle = 0;
	nodes_debug.clear();
	beams_debug.clear();
	memset(this->soundsources, 0, sizeof(soundsource_t) * MAX_SOUNDSCRIPTS_PER_TRUCK); free_soundsource = 0;
	memset(this->pressure_beams, 0, sizeof(int) * MAX_PRESSURE_BEAMS); free_pressure_beam = 0;
	memset(this->aeroengines, 0, sizeof(AeroEngine *) * MAX_AEROENGINES); free_aeroengine = 0;
	memset(this->cabs, 0, sizeof(int) * (MAX_CABS*3)); free_cab = 0;
	memset(this->subisback, 0, sizeof(int) * MAX_SUBMESHES);
	memset(this->hydro, 0, sizeof(int) * MAX_HYDROS); free_hydro = 0;
	for (int i=0;i<MAX_TEXCOORDS;i++) this->texcoords[i] = Vector3::ZERO;
	free_texcoord=0;
	memset(this->subtexcoords, 0, sizeof(int) * MAX_SUBMESHES); free_sub = 0;
	memset(this->subcabs, 0, sizeof(int) * MAX_SUBMESHES);
	memset(this->collcabs, 0, sizeof(int) * MAX_CABS);
	memset(this->collcabstype, 0, sizeof(int) * MAX_CABS);
	memset(this->inter_collcabrate, 0, sizeof(collcab_rate_t) * MAX_CABS); free_collcab = 0;
	memset(this->intra_collcabrate, 0, sizeof(collcab_rate_t) * MAX_CABS);
	memset(this->buoycabs, 0, sizeof(int) * MAX_CABS); free_buoycab = 0;
	memset(this->buoycabtypes, 0, sizeof(int) * MAX_CABS);
	memset(this->airbrakes, 0, sizeof(Airbrake *) * MAX_AIRBRAKES); free_airbrake = 0;
	memset(this->skidtrails, 0, sizeof(Skidmark *) * (MAX_WHEELS*2)); useSkidmarks = false;
	memset(this->flexbodies, 0, sizeof(FlexBody *) * MAX_FLEXBODIES); free_flexbody = 0;
	vidcams.clear();
	description.clear();

	memset(this->guid, 0, 128);
	memset(this->uniquetruckid, 0, 256);
	
	
	hasfixes=0;
	wingstart=-1;
	
	this->realtruckname=String();
	loading_finished=false;
	forwardcommands=false;

	wheel_contact_requested = false;
	rescuer = false;
	disable_default_sounds=false;
	detacher_group_state=DEFAULT_DETACHER_GROUP; // initialize default(0) var for detacher_group_state
	slopeBrake=false;
	beam_creak=BEAM_CREAK_DEFAULT;
	categoryid=-1;
	truckversion=-1;
	externalcameramode=0;
	externalcameranode=-1;
	authors.clear();
	slideNodesConnectInstantly=false;

	default_spring=DEFAULT_SPRING;
	default_spring_scale=1;
	default_damp=DEFAULT_DAMP;
	default_damp_scale=1;
	default_deform=BEAM_DEFORM;
	default_deform_scale=1;
	default_break=BEAM_BREAK;
	default_break_scale=1;

	default_beam_diameter=DEFAULT_BEAM_DIAMETER;
	skeleton_beam_diameter=BEAM_SKELETON_DIAMETER;
	strcpy(default_beam_material, "tracks/beam");
	default_plastic_coef=0;
	default_node_friction=NODE_FRICTION_COEF_DEFAULT;
	default_node_volume=NODE_VOLUME_COEF_DEFAULT;
	default_node_surface=NODE_SURFACE_COEF_DEFAULT;
	default_node_loadweight=NODE_LOADWEIGHT_DEFAULT;

	odometerTotal = 0;
	odometerUser  = 0;
	dashBoardLayouts.clear();

	memset(default_node_options, 0, 49);
	memset(texname, 0, 1023);
	memset(helpmat, 0, 255);
	
	fadeDist=150.0;
	collrange=DEFAULT_COLLISION_RANGE;
	masscount=0;
	disable_smoke = !SETTINGS.getBooleanSetting("Particles", true);
	smokeId=0;
	smokeRef=0;
	networking = false;
	editorId=-1;
	beambreakdebug  = SETTINGS.getBooleanSetting("Beam Break Debug", false);
	beamdeformdebug = SETTINGS.getBooleanSetting("Beam Deform Debug", false);
	triggerdebug    = SETTINGS.getBooleanSetting("Trigger Debug", false);
	rotaInertia=0;
	hydroInertia=0;
	cmdInertia=0;
	truckmass=0;
	loadmass=0;
	usedSkin=0;
	trucknum=0;
	buoyance=0;
	driveable=NOT_DRIVEABLE;

	free_fixes=0;
	propwheelcount=0;
	free_commands=0;
	fileformatversion=0;
	sectionconfigs.clear();

	origin=Vector3::ZERO;
	mSlideNodes.clear();

	enable_wheel2=true; //BSETTING("Enhanced wheels");

	engine=0;
	hascommands=0;
	hashelp=0;
	cinecameranodepos[0]=-1;
	freecinecamera=0;
	flaresMode=3;
	cablight=0;
	cablightNode=0;
	deletion_sceneNodes.clear();
	deletion_Objects.clear();
	netCustomLightArray[0] = -1;
	netCustomLightArray[1] = -1;
	netCustomLightArray[2] = -1;
	netCustomLightArray[3] = -1;
	netCustomLightArray_counter = 0;
	materialFunctionMapper=0;

	driversseatfound=false;
	ispolice=false;
	state=SLEEPING;
	hasposlights=false;
	heathaze=false;
	autopilot=0;
	hfinder=0;
	fuseAirfoil=0;
	fuseFront=0;
	fuseBack=0;
	fuseWidth=0;
	brakeforce=30000.0;
	hbrakeforce = 2*brakeforce;
	debugVisuals = SETTINGS.getBooleanSetting("DebugBeams", false);
	shadowOptimizations = SETTINGS.getBooleanSetting("Shadow optimizations", true);

	proped_wheels=0;
	braked_wheels=0;

	speedomat=String();
	tachomat=String();
	speedoMax=140;
	useMaxRPMforGUI=false;
	minimass=50.0;
	cparticle_enabled=false;
	truckconfig.clear();
	advanced_drag=false;
	advanced_node_drag=0;
	advanced_total_drag=0;
	freecamera=0;
	hasEmissivePass=0;
	cabMesh=0;
	cabNode=0;
	cameranodepos[0]=-1;
	cameranodedir[0]=-1;
	cameranoderoll[0]=-1;
	freePositioned=false;
	lowestnode=0;
	beamsRoot=0;

	virtuallyLoaded=false;
	ignoreProblems=false;

	subMeshGroundModelName = "";

	materialReplacer = NULL;
	if (!virtuallyLoaded)
		materialReplacer = new MaterialReplacer();

	beamHash = String();

	// get lights mode
	flaresMode = 3; // on by default
	String lightMode = SSETTING("Lights", "Only current vehicle, main lights");
	if (lightMode == "None (fastest)")
		flaresMode = 0;
	else if (lightMode == "No light sources")
		flaresMode = 1;
	else if (lightMode == "Only current vehicle, main lights")
		flaresMode = 2;
	else if (lightMode == "All vehicles, main lights")
		flaresMode = 3;
	else if (lightMode == "All vehicles, all lights")
		flaresMode = 4;

}

SerializedRig::~SerializedRig()
{
	if (engine)
	{
		delete(engine);
		engine=NULL;
	}
}

const std::map<std::string, int> SerializedRig::init_truck_sections()
{
	std::map<std::string, int> truckSections;

	truckSections.emplace(std::pair<std::string, int>("advdrag", BTS_ADVANCEDDRAG));
	truckSections.emplace(std::pair<std::string, int>("airbrakes", BTS_AIRBRAKES));
	truckSections.emplace(std::pair<std::string, int>("animators", BTS_ANIMATORS));
	truckSections.emplace(std::pair<std::string, int>("axles", BTS_AXLES));
	truckSections.emplace(std::pair<std::string, int>("beams", BTS_BEAMS));
	truckSections.emplace(std::pair<std::string, int>("brakes", BTS_BRAKES));
	truckSections.emplace(std::pair<std::string, int>("cab", BTS_CAB));
	truckSections.emplace(std::pair<std::string, int>("camerarail", BTS_CAMERARAIL));
	truckSections.emplace(std::pair<std::string, int>("cameras", BTS_CAMERAS));
	truckSections.emplace(std::pair<std::string, int>("cinecam", BTS_CINECAM));
	truckSections.emplace(std::pair<std::string, int>("collisionboxes", BTS_COLLISIONBOXES));
	truckSections.emplace(std::pair<std::string, int>("commands", BTS_COMMANDS));
	truckSections.emplace(std::pair<std::string, int>("commands2", BTS_COMMANDS2));
	truckSections.emplace(std::pair<std::string, int>("contacters", BTS_CONTACTERS));
	truckSections.emplace(std::pair<std::string, int>("description", BTS_DESCRIPTION));
	truckSections.emplace(std::pair<std::string, int>("engine", BTS_ENGINE));
	truckSections.emplace(std::pair<std::string, int>("engoption", BTS_ENGOPTION));
	truckSections.emplace(std::pair<std::string, int>("envmap", BTS_ENVMAP));
	truckSections.emplace(std::pair<std::string, int>("exhausts", BTS_EXHAUSTS));
	truckSections.emplace(std::pair<std::string, int>("fixes", BTS_FIXES));
	truckSections.emplace(std::pair<std::string, int>("flares", BTS_FLARES));
	truckSections.emplace(std::pair<std::string, int>("flares2", BTS_FLARES2));
	truckSections.emplace(std::pair<std::string, int>("flexbodies", BTS_FLEXBODIES));
	truckSections.emplace(std::pair<std::string, int>("flexbodywheels", BTS_FLEXBODYWHEELS));
	truckSections.emplace(std::pair<std::string, int>("fusedrag", BTS_FUSEDRAG));
	truckSections.emplace(std::pair<std::string, int>("globals", BTS_GLOBALS));
	truckSections.emplace(std::pair<std::string, int>("globeams", BTS_GLOBEAMS));
	truckSections.emplace(std::pair<std::string, int>("guisettings", BTS_GUISETTINGS));
	truckSections.emplace(std::pair<std::string, int>("help", BTS_HELP));
	truckSections.emplace(std::pair<std::string, int>("hookgroup", BTS_HOOKGROUP));
	truckSections.emplace(std::pair<std::string, int>("hooks", BTS_HOOKS));
	truckSections.emplace(std::pair<std::string, int>("hydros", BTS_HYDROS));
	truckSections.emplace(std::pair<std::string, int>("lockgroups", BTS_LOCKGROUPS));
	truckSections.emplace(std::pair<std::string, int>("managedmaterials", BTS_MANAGEDMATERIALS));
	truckSections.emplace(std::pair<std::string, int>("materialflarebindings", BTS_MATERIALFLAREBINDINGS));
	truckSections.emplace(std::pair<std::string, int>("meshwheels", BTS_MESHWHEELS));
	truckSections.emplace(std::pair<std::string, int>("meshwheels2", BTS_MESHWHEELS2));
	truckSections.emplace(std::pair<std::string, int>("minimass", BTS_MINIMASS));
	truckSections.emplace(std::pair<std::string, int>("nodecollision", BTS_NODECOLLISION));
	truckSections.emplace(std::pair<std::string, int>("nodes", BTS_NODES));
	truckSections.emplace(std::pair<std::string, int>("nodes2", BTS_NODES2));
	truckSections.emplace(std::pair<std::string, int>("particles", BTS_PARTICLES));
	truckSections.emplace(std::pair<std::string, int>("pistonprops", BTS_PISTONPROPS));
	truckSections.emplace(std::pair<std::string, int>("props", BTS_PROPS));
	truckSections.emplace(std::pair<std::string, int>("railgroups", BTS_RAILGROUPS));
	truckSections.emplace(std::pair<std::string, int>("rigidifiers", BTS_RIGIDIFIERS));
	truckSections.emplace(std::pair<std::string, int>("ropables", BTS_ROPABLES));
	truckSections.emplace(std::pair<std::string, int>("ropes", BTS_ROPES));
	truckSections.emplace(std::pair<std::string, int>("rotators", BTS_ROTATORS));
	truckSections.emplace(std::pair<std::string, int>("rotators2", BTS_ROTATORS2));
	truckSections.emplace(std::pair<std::string, int>("screwprops", BTS_SCREWPROPS));
	truckSections.emplace(std::pair<std::string, int>("shocks", BTS_SHOCKS));
	truckSections.emplace(std::pair<std::string, int>("shocks2", BTS_SHOCKS2));
	truckSections.emplace(std::pair<std::string, int>("slidenodes", BTS_SLIDENODES));
	truckSections.emplace(std::pair<std::string, int>("soundsources", BTS_SOUNDSOURCES));
	truckSections.emplace(std::pair<std::string, int>("soundsources2", BTS_SOUNDSOURCES2));
	truckSections.emplace(std::pair<std::string, int>("soundsources3", BTS_SOUNDSOURCES3));
	truckSections.emplace(std::pair<std::string, int>("texcoords", BTS_TEXCOORDS));
	truckSections.emplace(std::pair<std::string, int>("ties", BTS_TIES));
	truckSections.emplace(std::pair<std::string, int>("torquecurve", BTS_TORQUECURVE));
	truckSections.emplace(std::pair<std::string, int>("triggers", BTS_TRIGGER));
	truckSections.emplace(std::pair<std::string, int>("turbojets", BTS_TURBOJETS));
	truckSections.emplace(std::pair<std::string, int>("turboprops", BTS_TURBOPROPS));
	truckSections.emplace(std::pair<std::string, int>("turboprops2", BTS_TURBOPROPS2));
	truckSections.emplace(std::pair<std::string, int>("videocamera", BTS_VIDCAM));
	truckSections.emplace(std::pair<std::string, int>("wheels", BTS_WHEELS));
	truckSections.emplace(std::pair<std::string, int>("wheels2", BTS_WHEELS2));
	truckSections.emplace(std::pair<std::string, int>("wings", BTS_WINGS));

	return truckSections;
}

int SerializedRig::loadTruckVirtual(String fname, bool ignorep)
{
	virtuallyLoaded = true;
	ignoreProblems = ignorep;
	return loadTruck(fname, NULL, Vector3::ZERO, Quaternion::ZERO, NULL);
}

int SerializedRig::loadTruck(Ogre::String filename, Ogre::SceneNode *parent, Ogre::Vector3 pos, Ogre::Quaternion rot, collision_box_t *spawnbox)
{
	// add custom include path now, before scopelog, hides the path ...
	if (!SSETTING("resourceIncludePath", "").empty())
	{
		ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("resourceIncludePath", ""), "FileSystem", "customInclude");
	}

	ScopeLog scope_log("beam_"+filename);

	// initialize custom include path
	if (!SSETTING("resourceIncludePath", "").empty())
	{
		ResourceBackgroundQueue::getSingleton().initialiseResourceGroup("customInclude");
	}


	// create parsing context that we use for loading the truck
	parsecontext_t c;
	c.filename     = filename;
	c.modeString   = "none";
	c.warningText  = String();
	c.linecounter  = 0;
	c.line[0]      = 0;
	c.mode         = BTS_NONE;

	warnings.clear();
	modehistory.clear();

	int savedmode = BTS_NONE;
		
	// some temp varialbes
	int leftlight=0;
	int rightlight=0;
	int managedmaterials_doublesided=0; // not by default
	float wingarea=0.0;
	Real inertia_startDelay=-1, inertia_stopDelay=-1;
	char inertia_default_startFunction[50]="", inertia_default_stopFunction[50]="";
	int shadowmode = 1;
	Ogre::StringVector args;
	args.resize(20);
	float fuse_z_min = 1000.0f;
	float fuse_z_max = -1000.0f;
	float fuse_y_min = 1000.0f;
	float fuse_y_max = -1000.0f;

	bool in_section = false; // reminder for us if we are in a section

	// load truck configuration settings
	bool enable_background_loading = BSETTING("Background Loading", false);
	bool enable_advanced_deformation = false;
	int lockgroup_default = NODE_LOCKGROUP_DEFAULT;

	parser_warning(c, "Start of truck loading: " + filename, PARSER_INFO);


	DataStreamPtr ds = DataStreamPtr();
	String group = String();
	String errorStr = String();
	try
	{
		CACHE.checkResourceLoaded(filename, group);
		// error on ds open lower
		// open the stream and start reading :)
		ds = ResourceGroupManager::getSingleton().openResource(filename, group);
	} catch(Ogre::Exception& e)
	{
		errorStr = String(e.what());
	}

	//this->cacheEntryInfo = CACHE.getResourceInfo(filename);
	if (ds.isNull() || !ds->isReadable())
	{
#ifdef USE_MYGUI
		Console *console = Console::getSingletonPtrNoCreation();
		if (console) console->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, "unable to load vehicle (unable to open file): " + filename + " : " + errorStr, "error.png", 30000, true);
#endif // USE_MYGUI
		parser_warning(c, "Can't open truck file '"+filename+"'", PARSER_FATAL_ERROR);
		return -1;
	}


	// read in truckname on first line
	c.line = ds->getLine(true);
	StringUtil::trim(c.line);
	realtruckname = c.line;
	c.linecounter++;

	// then loop through the rest of the lines
	while (!ds->eof())
	{
		c.line = ds->getLine(true);
		StringUtil::trim(c.line);
		c.linecounter++;
		
		// clear arguments, just to be sure
		args.clear();

		if (c.line.size() == 0 || c.line[0]==';' || c.line[0]=='/')
			continue;

		// try for parsing exceptions
		try
		{

			if (c.line == "end")
			{
				parser_warning(c, "End of truck loading", PARSER_INFO);
				c.modeString = "end";
				loading_finished = 1;
				break;
			}

			if (c.line == "end_description" && c.mode == BTS_DESCRIPTION)
			{
				c.mode = BTS_NONE;
				continue;
			}

			if (c.line == "end_comment" && c.mode == BTS_COMMENT)
			{
				c.mode = savedmode;
				continue;
			}
			if (c.line == "end_section" && c.mode == BTS_IN_SECTION )
			{
				c.mode = savedmode;
				in_section = false;
				continue;
			}
			if (c.line == "end_section" && in_section)
			{
				// compatibility mode: do not restore the section
				in_section = false;
				continue;
			}

			if (c.mode == BTS_DESCRIPTION)
			{
				// description
				//parse dscription, and ignore every possible keyword
				//char *tmp = strdup(c.line);
				description.push_back(std::string(c.line));
				continue;
			}

			if (c.mode == BTS_COMMENT)
			{
				// comment
				// ignore everything
				continue;
			}
			if (c.mode == BTS_IN_SECTION)
			{
				// ignored truck section
				continue;
			}

			// now check if we are in a new section
			std::map<std::string, int>::const_iterator it_ts = truck_sections.find(c.line);

			if (it_ts != truck_sections.end())
			{
				// save the current mode in the history
				modehistory.push_back(c);
				// then set the new one
				c.mode = it_ts->second;
				c.modeString = it_ts->first;
				continue;
			} else if (c.line[0] == ' ' || c.line[c.line.size()-1] == ' ')
			{
				parser_warning(c, "spaces in section declarations not allowed", PARSER_ERROR);
			}

			// check for commands (one line sections)
			if (c.line == "forwardcommands") {forwardcommands=true;continue;};
			if (c.line == "importcommands") {importcommands=true;continue;};
			if (c.line == "rollon") {wheel_contact_requested=true;continue;};
			if (c.line == "rescuer") {rescuer=true;continue;};
			if (c.line == "comment") {savedmode=c.mode; c.mode=BTS_COMMENT; continue;};
			if (c.line == "disabledefaultsounds") {disable_default_sounds=true;continue;};
			if (c.line.size() > 13 && c.line.substr(0, 13) == "sectionconfig") {savedmode=c.mode;c.mode=BTS_SECTIONCONFIG; /* NOT continue */};
			if (c.line.size() > 7 && c.line.substr(0, 7) == "section" && c.mode!=BTS_SECTIONCONFIG)
			{
				c.mode=BTS_SECTION;
				/* NOT continue */
			}
			/* BTS_IN_SECTION = reserved for ignored section */
		
			if (c.line.size() > 14 && c.line.substr(0, 14) == "detacher_group")
			{
				parse_args(c, args, 1);
				if (args[1] == "end")
				{
					detacher_group_state = 0;
					continue;
				} else
				{
					detacher_group_state = PARSEINT(args[1]);
					continue;
				}
			}
			if (c.line.size() > 8 && c.line.substr(0, 8) == "fileinfo")
			{
				int n = parse_args(c, args, 2);
				strncpy(uniquetruckid, args[1].c_str(), 254);
				if (n > 2) categoryid   = PARSEINT(args[2]);
				if (n > 3) truckversion = PARSEINT(args[3]);
				continue;
			}
			if (c.line.size() > 9 && c.line.substr(0, 9) == "extcamera")
			{
				int n = parse_args(c, args, 2);
				if (args[1] == "classic") externalcameramode = 0;
				if (args[1] == "cinecam") externalcameramode = 1;
				if (args[1] == "node")    externalcameramode = 2;
				if (n > 2 && args[1] == "node") externalcameranode = parse_node_number(c, args[2]);
				continue;
			}


			if (c.line.size() > 20 && c.line.substr(0, 19) == "submesh_groundmodel")
			{
				int n = parse_args(c, args, 2);
				subMeshGroundModelName = args[1];
			}

			if (c.line.size() > 9 && c.line.substr(0, 10) == "SlopeBrake")
			{
				slopeBrake=true;
				slopeBrakeFactor   = 6.0f;
				slopeBrakeAttAngle = 5.0f;
				slopeBrakeRelAngle = 10.0f;
				c.modeString       = "slopebrake";
				if (c.line.size() == 18)
				{
					parser_warning(c, "Slope-Brake enhancment added with default settings.", PARSER_INFO);
					continue;
				}
				int n = parse_args(c, args, 1);
				if (n > 1) slopeBrakeFactor      = PARSEINT(args[1]);
				if (n > 2) slopeBrakeAttAngle    = PARSEINT(args[2]);
				if (n > 3) slopeBrakeRelAngle    = PARSEINT(args[3]);

				if (slopeBrakeFactor   < 1.0f)  slopeBrakeFactor   = 1.0f;
				if (slopeBrakeFactor   > 20.0f) slopeBrakeFactor   = 20.0f;
				if (slopeBrakeAttAngle < 1.0f)  slopeBrakeAttAngle = 1.0f;
				if (slopeBrakeAttAngle > 45.0f) slopeBrakeAttAngle = 45.0f;
				if (slopeBrakeRelAngle < 1.0f)  slopeBrakeRelAngle = 1.0f;
				if (slopeBrakeRelAngle > 45.0f) slopeBrakeRelAngle = 45.0f;
				slopeBrakeRelAngle += slopeBrakeAttAngle;
				parser_warning(c,"Slope-Brake enhancment added. " + filename +" line " + StringConverter::toString(c.linecounter) + ". Slopebrake-Factor: " + StringConverter::toString(slopeBrakeFactor) + ". Free Rollback Offset: " + StringConverter::toString(slopeBrakeAttAngle) + "degree. Release at Offset: " + StringConverter::toString(slopeBrakeRelAngle) + "degree.", PARSER_INFO);
				continue;
			}
			if (c.line.size() > 14 && c.line.substr(0, 14) == "AntiLockBrakes")
			{
				float ratio  = -1.0f;
				c.modeString = "antilockbrake";

				Ogre::StringVector options = Ogre::StringUtil::split(String(c.line).substr(15), ","); // "AntiLockBrakes " = 15 characters
				// check for common errors
				if (options.size() < 2)
				{
					parser_warning(c, "Error parsing File (Antilockbrakes) " + filename +" line " + StringConverter::toString(c.linecounter) + ". Not enough Options parsed, trying to continue ...", PARSER_ERROR);
					continue;
				}

				for (unsigned int i=0;i<options.size();i++)
				{
					if (i == 0)
					{
						ratio = StringConverter::parseReal(options[i]);
						//set ratio
						if (ratio)
						{
							alb_ratio = ratio;
							alb_ratio = std::max(0.0f, alb_ratio);
							alb_ratio = std::min(alb_ratio, 20.0f);
						} else
						{
							parser_warning(c, "Error parsing File " + filename +" line " + StringConverter::toString(c.linecounter) + ". Mode not parsed, trying to continue....");
							continue;
						}
					} else if (i == 1)
					{
						// wheelspeed adaption: 60 sec * 60 mins / 1000(kilometer) = 3.6 to get meter per sec
						alb_minspeed = StringConverter::parseReal(options[i]) / 3.6f;
						alb_minspeed = std::max(0.5f, alb_minspeed);
					} else if (i == 2)
					{
						float pulse = StringConverter::parseReal(options[i]);
						if (pulse <= 1.0f || pulse >= 2000.0f)
						{
							alb_pulse = 1;
						} else
						{
							alb_pulse = (int)(2000.0f / fabs(pulse));
						}
					} else
					{
						// parse the rest
						Ogre::StringVector args2 = Ogre::StringUtil::split(options[i], ":");
						if (args2.size() == 0)
						{
							parser_warning(c, "Error parsing File (Antilockbrakes) " + filename +" line " + StringConverter::toString(c.linecounter) + ". Antilockbrakes disabeld.", PARSER_ERROR);
							continue;
						}
						// trim spaces from the entry
						Ogre::StringUtil::trim(args2[0]);
						if (args2.size() >= 2) Ogre::StringUtil::trim(args2[1]);

						if (args2[0] == "mode" && args2.size() == 2)
						{
							//set source identification flag
							Ogre::StringVector args3 = Ogre::StringUtil::split(args2[1], "&");
							for (unsigned int j=0;j<args3.size();j++)
							{
								String sourceStr = args3[j];
								Ogre::StringUtil::trim(sourceStr);
								if (sourceStr == "ON" || sourceStr == "on")	
								{
									alb_mode = 1;
									alb_present = true;
								} else if (sourceStr == "OFF" || sourceStr == "off")
								{
									alb_mode = 0;
									alb_present = true;
								} else if (sourceStr == "NODASH" || sourceStr == "nodash" || sourceStr == "Nodash" || sourceStr == "NoDash")
								{
									alb_present = false;
								} else if (sourceStr == "NOTOGGLE" || sourceStr == "notoggle" || sourceStr == "Notoggle" || sourceStr == "NoToggle")
								{
									alb_notoggle = true;
								}
							}
						} else
						{
							parser_warning(c, "Antilockbrakes Mode: missing " + filename +" line " + StringConverter::toString(c.linecounter) + ". Antilockbrakes Mode = ON.", PARSER_ERROR);
							alb_present = true;
							alb_mode = 1;
						}
					}
				}
				continue;
			}
			if (c.line.size() > 14 && c.line.substr(0, 15) == "TractionControl")
			{
				float ratio = 0.0f;
				c.modeString = "tractioncontrol";
				// parse the line
				Ogre::StringVector options = Ogre::StringUtil::split(String(c.line).substr(16), ","); // "TractionControl " = 16 characters
				// check for common errors
				if (options.size() < 2)
				{
					parser_warning(c,"Error parsing File (TractionControl) " + filename +" line " + StringConverter::toString(c.linecounter) + ". Not enough Options parsed, trying to continue ...", PARSER_ERROR);
					continue;
				}

				for (unsigned int i=0; i<options.size(); i++)
				{
					if (i == 0)
					{
						ratio = StringConverter::parseReal(options[i]);
						//set ratio
						if (ratio)
						{
							tc_ratio = ratio;
							tc_ratio = std::max(0.0f, tc_ratio);
							tc_ratio = std::min(tc_ratio, 20.0f);
						} else
						{
							parser_warning(c,"Error parsing File (TractionControl) " + filename +" line " + StringConverter::toString(c.linecounter) + ". TractionControl disabeld.", PARSER_ERROR);
							continue;
						}
					} else if (i == 1)
					{
						tc_wheelslip = StringConverter::parseReal(options[i]);
						tc_wheelslip = std::max(0.0f, tc_wheelslip);
					} else if (i == 2)
					{
						tc_fade = StringConverter::parseReal(options[i]);
						tc_fade = std::max(0.1f, tc_fade);
					} else if (i == 3)
					{
						float pulse = StringConverter::parseReal(options[i]);
						if (pulse <= 1.0f || pulse >= 2000.0f)
						{
							tc_pulse = 1;
						} else
						{
							tc_pulse = (int)(2000.0f / fabs(pulse));
						}
					} else
					{
						// parse the rest
						Ogre::StringVector args2 = Ogre::StringUtil::split(options[i], ":");
						if (args2.size() == 0)
						{
							parser_warning(c,"Error parsing File (TractionControl) " + filename +" line " + StringConverter::toString(c.linecounter) + ". Mode not parsed, trying to continue....", PARSER_ERROR);
							continue;
						}
						// trim spaces from the entry
						Ogre::StringUtil::trim(args2[0]);
						if (args2.size() >= 2) Ogre::StringUtil::trim(args2[1]);

						if (args2[0] == "mode" && args2.size() == 2)
						{
							//set source identification flag
							Ogre::StringVector args3 = Ogre::StringUtil::split(args2[1], "&");
							for (unsigned int j=0;j<args3.size();j++)
							{
								String sourceStr = args3[j];
								Ogre::StringUtil::trim(sourceStr);
								if (sourceStr == "ON" || sourceStr == "on" || sourceStr == "On")	
								{
									tc_mode = 1;
									tc_present = true;
								} else if (sourceStr == "OFF" || sourceStr == "off" || sourceStr == "Off")
								{
									tc_mode = 0;
									tc_present = true;
								} else if (sourceStr == "NODASH" || sourceStr == "nodash" || sourceStr == "Nodash" || sourceStr == "NoDash")
								{
									tc_present = false;
								} else if (sourceStr == "NOTOGGLE" || sourceStr == "notoggle" || sourceStr == "Notoggle" || sourceStr == "NoToggle")
								{
									tc_notoggle = true;
								}
							}
						} else
						{
							parser_warning(c, "TractionControl Mode: missing " + filename +" line " + StringConverter::toString(c.linecounter) + ". TractionControl Mode = ON.", PARSER_ERROR);
							tc_present = true;
							tc_mode = 1;
						}
					}
				}
				continue;
			}
			if (c.line.size() > 13 && c.line.substr(0, 13) == "cruisecontrol")
			{
				parse_args(c, args, 3);
				cc_target_speed_lower_limit = PARSEREAL(args[1]);
				cc_can_brake = PARSEINT(args[2]) != 0;
				if (cc_target_speed_lower_limit <= 0.0f)
				{
					parser_warning(c, "CruiseControl: First parameter must be a decimal and greater than zero (e.g. 5.6)");
					continue;
				}
				continue;
			}
			if (c.line.size() > 12 && c.line.substr(0, 12) == "speedlimiter")
			{
				parse_args(c, args, 2);
				sl_speed_limit = PARSEREAL(args[1]);
				if (sl_speed_limit <= 0.0f)
				{
					parser_warning(c, "SpeedLimiter: Parameter must be a decimal and greater than zero (e.g. 69.445)");
					continue;
				}
				sl_enabled = true;
				continue;
			}
			if (c.line.size() > 17 && c.line.substr(0, 17) == "fileformatversion")
			{
				parse_args(c, args, 2);
				fileformatversion = PARSEINT(args[1]);
				if (fileformatversion > TRUCKFILEFORMATVERSION)
				{
					parser_warning(c, "The file is for a newer RoR version", PARSER_WARNING);
					continue;
				}
				continue;
			}
			if (c.line.size() > 6 && c.line.substr(0, 6) == "author")
			{
				int n = parse_args(c, args, 2);
				authorinfo_t author;
				if (n > 1) author.type  = args[1];
				if (n > 2) author.id    = PARSEINT(args[2]);
				if (n > 3) author.name  = args[3];
				if (n > 4) author.email = args[4];
				authors.push_back(author);
				continue;
			}

			if (c.line == "slidenode_connect_instantly")
			{
				slideNodesConnectInstantly = true;
				continue;
			}

			if (c.line == "enable_advanced_deformation")
			{
				enable_advanced_deformation = true;
				continue;
			}

			if (c.line == "hideInChooser")
			{
				hideInChooser = true;
				continue;
			}


			if (c.line == "lockgroup_default_nolock")
			{
				lockgroup_default = 9999;
				continue;
			}


			if (c.line == "set_shadows")
			{
				parse_args(c, args, 2);
				shadowmode = PARSEINT(args[1]);
				continue;
			}

			if (c.line.size() > 16 && c.line.substr(0, 16) == "prop_camera_mode")
			{
				parse_args(c, args, 2);
				int pmode = PARSEINT(args[1]);

				// always use the last prop
				prop_t *prop = &props[free_prop-1];
				if (prop->mo) prop->cameramode = pmode;
				continue;
			}

			if (c.line.size() > 20 && c.line.substr(0, 20) == "flexbody_camera_mode")
			{
				parse_args(c, args, 2);
				int pmode = PARSEINT(args[1]);

				// always use the last flexbody
				FlexBody *flex = flexbodies[free_flexbody-1];
				if (flex) flex->setCameraMode(pmode);
				continue;
			}

			if (c.line.size() > 13 && c.line.substr(0, 13) == "add_animation")
			{
				c.modeString = "add_animation";
				Ogre::StringVector options = Ogre::StringUtil::split(c.line.substr(14), ","); // "add_animation " = 14 characters

				if (options.size() < 4)
				{
					parser_warning(c,"Error parsing File (add_animation) " + filename +" line " + StringConverter::toString(c.linecounter) + ". Not enough Options parsed, trying to continue ...", PARSER_ERROR);
					continue;
				}

				/*
				 * this command has several layers for splitting up the c.line:
				 * 1. ',' the top level will be split up with a comma to separate the main options
				 * 2. ':' the second level will be split up with colon, it separates the entry name and its value
				 * 3. '|' the third level is used to specify multiple values for the entry value
				 */
				int animnum = 0;
				float ratio = 0.0f, opt1 = -1.0f, opt2 = -1.0f;

				if (!free_prop)
				{
					parser_warning(c, "No prop to animate existing", PARSER_ERROR);
					continue;
				}

				// always use the last prop
				prop_t *prop = &props[free_prop-1];

				// look for a free anim slot, important: do not over the borders!
				while (prop->animFlags[animnum] && animnum < 10)
					animnum++;

				// all slots used?
				if (animnum >= 10)
				{
					parser_warning(c, "Cant animate a prop more then 10 times", PARSER_ERROR);
					continue;
				}

				// parse the arguments individually
				for (unsigned int i=0;i<options.size();i++)
				{
					if (i == 0)
					{
						ratio = StringConverter::parseReal(options[i]);
						//set ratio
						if (ratio)
							prop->animratio[animnum]=ratio;
						else
							parser_warning(c, "Animation-Ratio = 0 ?", PARSER_ERROR);
					} else if (i == 1)
					{
						opt1 = StringConverter::parseReal(options[i]);
						prop->animOpt1[animnum] = opt1;
					} else if (i == 2)
					{
						opt2 = StringConverter::parseReal(options[i]);
						prop->animOpt2[animnum] = opt2;
						prop->animOpt3[animnum] = 0.0f;
						prop->animOpt4[animnum] = 0.0f;
						prop->animOpt5[animnum] = 0.0f;
						prop->animKeyState[animnum] = -1.0f;
					} else
					{
						// parse the rest
						Ogre::StringVector args2 = Ogre::StringUtil::split(options[i], ":");
						if (args2.size() == 0)
							continue;

						// trim spaces from the entry
						Ogre::StringUtil::trim(args2[0]);
						if (args2.size() >= 2) Ogre::StringUtil::trim(args2[1]);

						if (args2[0] == "source" && args2.size() == 2)
						{
							//set source identification flag
							Ogre::StringVector args3 = Ogre::StringUtil::split(args2[1], "|");
							for (unsigned int j=0;j<args3.size();j++)
							{
								String sourceStr = args3[j];
								Ogre::StringUtil::trim(sourceStr);
								if (sourceStr == "airspeed")             { prop->animFlags[animnum] |= (ANIM_FLAG_AIRSPEED); }
								else if (sourceStr == "vvi")             { prop->animFlags[animnum] |= (ANIM_FLAG_VVI); }
								else if (sourceStr == "altimeter100k")   { prop->animFlags[animnum] |= (ANIM_FLAG_ALTIMETER); prop->animOpt3[animnum] = 1.0f; }
								else if (sourceStr == "altimeter10k")    { prop->animFlags[animnum] |= (ANIM_FLAG_ALTIMETER); prop->animOpt3[animnum] = 2.0f; }
								else if (sourceStr == "altimeter1k")     { prop->animFlags[animnum] |= (ANIM_FLAG_ALTIMETER); prop->animOpt3[animnum] = 3.0f; }
								else if (sourceStr == "aoa")             { prop->animFlags[animnum] |= (ANIM_FLAG_AOA); }
								else if (sourceStr == "flap")            { prop->animFlags[animnum] |= (ANIM_FLAG_FLAP); }
								else if (sourceStr == "airbrake")        { prop->animFlags[animnum] |= (ANIM_FLAG_AIRBRAKE); }
								else if (sourceStr == "roll")            { prop->animFlags[animnum] |= (ANIM_FLAG_ROLL); }
								else if (sourceStr == "pitch")           { prop->animFlags[animnum] |= (ANIM_FLAG_PITCH); }
								else if (sourceStr == "throttle1")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 1.0f; }
								else if (sourceStr == "throttle2")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 2.0f; }
								else if (sourceStr == "throttle3")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 3.0f; }
								else if (sourceStr == "throttle4")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 4.0f; }
								else if (sourceStr == "throttle5")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 5.0f; }
								else if (sourceStr == "throttle6")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 6.0f; }
								else if (sourceStr == "throttle7")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 7.0f; }
								else if (sourceStr == "throttle8")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 8.0f; }
								else if (sourceStr == "rpm1")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 1.0f; }
								else if (sourceStr == "rpm2")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 2.0f; }
								else if (sourceStr == "rpm3")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 3.0f; }
								else if (sourceStr == "rpm4")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 4.0f; }
								else if (sourceStr == "rpm5")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 5.0f; }
								else if (sourceStr == "rpm6")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 6.0f; }
								else if (sourceStr == "rpm7")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 7.0f; }
								else if (sourceStr == "rpm8")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 8.0f; }
								else if (sourceStr == "aerotorq1")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 1.0f; }
								else if (sourceStr == "aerotorq2")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 2.0f; }
								else if (sourceStr == "aerotorq3")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 3.0f; }
								else if (sourceStr == "aerotorq4")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 4.0f; }
								else if (sourceStr == "aerotorq5")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 5.0f; }
								else if (sourceStr == "aerotorq6")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 6.0f; }
								else if (sourceStr == "aerotorq7")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 7.0f; }
								else if (sourceStr == "aerotorq8")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 8.0f; }
								else if (sourceStr == "aeropit1")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 1.0f; }
								else if (sourceStr == "aeropit2")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 2.0f; }
								else if (sourceStr == "aeropit3")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 3.0f; }
								else if (sourceStr == "aeropit4")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 4.0f; }
								else if (sourceStr == "aeropit5")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 5.0f; }
								else if (sourceStr == "aeropit6")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 6.0f; }
								else if (sourceStr == "aeropit7")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 7.0f; }
								else if (sourceStr == "aeropit8")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 8.0f; }
								else if (sourceStr == "aerostatus1")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 1.0f; }
								else if (sourceStr == "aerostatus2")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 2.0f; }
								else if (sourceStr == "aerostatus3")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 3.0f; }
								else if (sourceStr == "aerostatus4")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 4.0f; }
								else if (sourceStr == "aerostatus5")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 5.0f; }
								else if (sourceStr == "aerostatus6")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 6.0f; }
								else if (sourceStr == "aerostatus7")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 7.0f; }
								else if (sourceStr == "aerostatus8")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 8.0f; }
								else if (sourceStr == "brakes")          { prop->animFlags[animnum] |= (ANIM_FLAG_BRAKE); }
								else if (sourceStr == "accel")           { prop->animFlags[animnum] |= (ANIM_FLAG_ACCEL); }
								else if (sourceStr == "clutch")          { prop->animFlags[animnum] |= (ANIM_FLAG_CLUTCH); }
								else if (sourceStr == "speedo")          { prop->animFlags[animnum] |= (ANIM_FLAG_SPEEDO); }
								else if (sourceStr == "tacho")           { prop->animFlags[animnum] |= (ANIM_FLAG_TACHO); }
								else if (sourceStr == "turbo")           { prop->animFlags[animnum] |= (ANIM_FLAG_TURBO); }
								else if (sourceStr == "parking")         { prop->animFlags[animnum] |= (ANIM_FLAG_PBRAKE); }
								else if (sourceStr == "shifterman1")     { prop->animFlags[animnum] |= (ANIM_FLAG_SHIFTER); prop->animOpt3[animnum] = 1.0f; }
								else if (sourceStr == "shifterman2")     { prop->animFlags[animnum] |= (ANIM_FLAG_SHIFTER); prop->animOpt3[animnum] = 2.0f; }
								else if (sourceStr == "sequential")      { prop->animFlags[animnum] |= (ANIM_FLAG_SHIFTER); prop->animOpt3[animnum] = 3.0f; }
								else if (sourceStr == "shifterlin")      { prop->animFlags[animnum] |= (ANIM_FLAG_SHIFTER); prop->animOpt3[animnum] = 4.0f; }
								else if (sourceStr == "torque")          { prop->animFlags[animnum] |= (ANIM_FLAG_TORQUE); }
								else if (sourceStr == "heading")         { prop->animFlags[animnum] |= (ANIM_FLAG_HEADING); }
								else if (sourceStr == "difflock")        { prop->animFlags[animnum] |= (ANIM_FLAG_DIFFLOCK); }
								else if (sourceStr == "steeringwheel")   { prop->animFlags[animnum] |= (ANIM_FLAG_STEERING); }
								else if (sourceStr == "aileron")         { prop->animFlags[animnum] |= (ANIM_FLAG_AILERONS); }
								else if (sourceStr == "elevator")        { prop->animFlags[animnum] |= (ANIM_FLAG_ELEVATORS); }
								else if (sourceStr == "rudderair")       { prop->animFlags[animnum] |= (ANIM_FLAG_ARUDDER); }
								else if (sourceStr == "rudderboat")      { prop->animFlags[animnum] |= (ANIM_FLAG_BRUDDER); }
								else if (sourceStr == "throttleboat")    { prop->animFlags[animnum] |= (ANIM_FLAG_BTHROTTLE); }
								else if (sourceStr == "permanent")       { prop->animFlags[animnum] |= (ANIM_FLAG_PERMANENT); }
								else if (sourceStr == "event")           { prop->animFlags[animnum] |= (ANIM_FLAG_EVENT); }
							}

							if (prop->animFlags[animnum] == 0)
								parser_warning(c, "Failed to identify source.", PARSER_ERROR);
							else
								parser_warning(c, "Animation source set to prop#: " + TOSTRING(free_prop-1) + ", flag " +TOSTRING(prop->animFlags[animnum]) + ", Animationnumber: " + TOSTRING(animnum), PARSER_INFO);
						}
						else if (args2[0] == "mode" && args2.size() == 2)
						{
							//set c.mode identification flag
							Ogre::StringVector args3 = Ogre::StringUtil::split(args2[1], "|");
							for (unsigned int j=0;j<args3.size();j++)
							{
								String modeStr = args3[j];
								Ogre::StringUtil::trim(modeStr);
								if (modeStr == "x-rotation")      prop->animMode[animnum] |= (ANIM_MODE_ROTA_X);
								else if (modeStr == "y-rotation") prop->animMode[animnum] |= (ANIM_MODE_ROTA_Y);
								else if (modeStr == "z-rotation") prop->animMode[animnum] |= (ANIM_MODE_ROTA_Z);
								else if (modeStr == "x-offset")   prop->animMode[animnum] |= (ANIM_MODE_OFFSET_X);
								else if (modeStr == "y-offset")   prop->animMode[animnum] |= (ANIM_MODE_OFFSET_Y);
								else if (modeStr == "z-offset")   prop->animMode[animnum] |= (ANIM_MODE_OFFSET_Z);
							}

							if (prop->animMode[animnum] == 0)
								parser_warning(c, "Failed to identify animation c.mode.", PARSER_ERROR);
							else
								parser_warning(c, "Animation mode set to prop#: " + TOSTRING(free_prop-1)+ ", mode " +TOSTRING(prop->animMode[animnum]) + ", Animationnumber: " + TOSTRING(animnum), PARSER_INFO);
						}
						else if (args2[0] == "autoanimate" && args2.size() == 1)
						{
							// TODO: re-add check for invalid cases
							prop->animMode[animnum] |= (ANIM_MODE_AUTOANIMATE);

							if     (prop->animMode[animnum] & ANIM_MODE_ROTA_X)   { prop->animOpt1[animnum] = opt1 + prop->rotaX; prop->animOpt2[animnum] = opt2 + prop->rotaX; prop->animOpt4[animnum] = prop->rotaX; }
							else if (prop->animMode[animnum] & ANIM_MODE_ROTA_Y)   { prop->animOpt1[animnum] = opt1 + prop->rotaY; prop->animOpt2[animnum] = opt2 + prop->rotaY; prop->animOpt4[animnum] = prop->rotaY; }
							else if (prop->animMode[animnum] & ANIM_MODE_ROTA_Z)   { prop->animOpt1[animnum] = opt1 + prop->rotaZ; prop->animOpt2[animnum] = opt2 + prop->rotaZ; prop->animOpt4[animnum] = prop->rotaZ; }
							else if (prop->animMode[animnum] & ANIM_MODE_OFFSET_X) { prop->animOpt1[animnum] = opt1 + prop->orgoffsetX; prop->animOpt2[animnum] = opt2 + prop->orgoffsetX; prop->animOpt4[animnum] = prop->orgoffsetX; }
							else if (prop->animMode[animnum] & ANIM_MODE_OFFSET_Y) { prop->animOpt1[animnum] = opt1 + prop->orgoffsetY; prop->animOpt2[animnum] = opt2 + prop->orgoffsetY; prop->animOpt4[animnum] = prop->orgoffsetY; }
							else if (prop->animMode[animnum] & ANIM_MODE_OFFSET_Z) { prop->animOpt1[animnum] = opt1 + prop->orgoffsetZ; prop->animOpt2[animnum] = opt2 + prop->orgoffsetZ; prop->animOpt4[animnum] = prop->orgoffsetZ; }
							parser_warning(c, "Animation mode Autoanimation added to prop#: " + TOSTRING(free_prop-1) + " , Animationnumber: " + TOSTRING(animnum), PARSER_INFO);
						}
						else if (args2[0] == "noflip" && args2.size() == 1)
						{
							prop->animMode[animnum] |= (ANIM_MODE_NOFLIP);
						}
						else if (args2[0] == "bounce" && args2.size() == 1)
						{
							prop->animMode[animnum] |= (ANIM_MODE_BOUNCE);
							prop->animOpt5[animnum] = 1.0f;
						}
						else if (args2[0] == "eventlock" && args2.size() == 1)
						{
							prop->animKeyState[animnum] = 0.0f;
							prop->lastanimKS[animnum] = 0.0f;
						}
						else if (args2[0] == "event" && args2.size() == 2)
						{
							// we are using keys as source
							prop->animFlags[animnum] |= ANIM_FLAG_EVENT;

							// now parse the keys
							prop->animFlags[animnum] |= ANIM_FLAG_EVENT;
							Ogre::StringVector args3 = Ogre::StringUtil::split(args2[1], "|");
							for (unsigned int j=0;j<args3.size();j++)
							{
								String eventStr = args3[j];
								Ogre::StringUtil::trim(eventStr);
								Ogre::StringUtil::toUpperCase(eventStr);
								int evtID = INPUTENGINE.resolveEventName(eventStr);
								if (evtID != -1)
									prop->animKey[animnum] = evtID;
								else
									parser_warning(c, "Animation event unknown: " + eventStr, PARSER_ERROR);
							}
						}

					}
				}
				continue;
			}
			if (c.line.size() > 28 && c.line.substr(0, 28) == "set_managedmaterials_options")
			{
				parse_args(c, args, 2);
				managedmaterials_doublesided = PARSEINT(args[1]);
				continue;
			}
			if (c.line.size() > 23 && c.line.substr(0, 23) == "set_beam_defaults_scale")
			{
				int n = parse_args(c, args, 5);
				default_spring_scale = PARSEREAL(args[1]);
				if (n > 2) default_damp_scale   = PARSEREAL(args[2]);
				if (n > 3) default_deform_scale = PARSEREAL(args[3]);
				if (n > 4) default_break_scale  = PARSEREAL(args[4]);
				continue;
			}
			if (c.line.size() > 4 && c.line.substr(0, 4) == "guid")
			{
				parse_args(c, args, 2);
				StringUtil::trim(args[1]);
				strncpy(guid, args[1].c_str(), 128);
				continue;
			}
			if (c.line.size() > 17 && c.line.substr(0, 17) == "set_beam_defaults")
			{
				String default_beam_material2;
				float tmpdefault_plastic_coef=-1.0f;
				int n = parse_args(c, args, 2);
				default_spring = PARSEREAL(args[1]);
				if (n > 2) default_damp            = PARSEREAL(args[2]);
				if (n > 3) default_deform          = PARSEREAL(args[3]);
				if (n > 4) default_break           = PARSEREAL(args[4]);
				if (n > 5) default_beam_diameter   = PARSEREAL(args[5]);
				if (n > 6) default_beam_material2  = args[6];
				if (n > 7) tmpdefault_plastic_coef = PARSEREAL(args[7]);

				if (!default_beam_material2.empty())
				{
					MaterialPtr mat = MaterialManager::getSingleton().getByName(String(default_beam_material2));
					if (!mat.isNull())
						strncpy(default_beam_material, default_beam_material2.c_str(), 256);
					else
						parser_warning(c, "beam material '" + String(default_beam_material2) + "' not found!", PARSER_ERROR);
				}
				if (default_spring<0) default_spring=DEFAULT_SPRING;
				if (default_damp<0) default_damp=DEFAULT_DAMP;
				if (default_deform<0) default_deform=BEAM_DEFORM;
				if (default_break<0) default_break=BEAM_BREAK;
				if (default_beam_diameter<0) default_beam_diameter=DEFAULT_BEAM_DIAMETER;

				// get the old 400k deformation limit for trucks with beams and set_beam_defaults based on old physics (pre beamv2)
				if (!enable_advanced_deformation && default_deform < BEAM_DEFORM)
					default_deform=BEAM_DEFORM;

				if (tmpdefault_plastic_coef>=0.0f)
				{
					beam_creak=0.0f;
					default_plastic_coef=tmpdefault_plastic_coef;
				}
				if (default_spring_scale != 1 || default_damp_scale != 1 || default_deform_scale != 1 || default_break_scale != 1)
				{
					parser_warning(c, "Due to using set_beam_defaults_scale, this set_beam_defaults was interpreted as  " + \
						TOSTRING(default_spring * default_spring_scale) + ", " + \
						TOSTRING(default_damp   * default_damp_scale) + ", " + \
						TOSTRING(default_deform * default_deform_scale) + ", " + \
						TOSTRING(default_break  * default_break_scale), PARSER_INFO);

				}
				continue;
			}
			if (c.line.size() > 20 && c.line.substr(0, 20) == "set_inertia_defaults")
			{
				int n = parse_args(c, args, 2);
				inertia_startDelay = PARSEREAL(args[1]);
				if (n > 2) inertia_stopDelay = PARSEREAL(args[2]);
				if (n > 3) strncpy(inertia_default_startFunction, args[3].c_str(), 50);
				if (n > 4) strncpy(inertia_default_stopFunction, args[4].c_str(), 50);

				if (inertia_startDelay < 0 || inertia_stopDelay < 0)
				{
					// reset it
					inertia_startDelay = -1;
					inertia_stopDelay  = -1;
					memset(inertia_default_startFunction, 0, sizeof(inertia_default_startFunction));
					memset(inertia_default_stopFunction, 0, sizeof(inertia_default_stopFunction));
				}
				continue;
			}

			if (c.line.size() > 17 && c.line.substr(0, 17) == "set_node_defaults")
			{
				int n = parse_args(c, args, 2);
				default_node_loadweight = PARSEREAL(args[1]);
				if (n > 2) default_node_friction = PARSEREAL(args[2]);
				if (n > 3) default_node_volume   = PARSEREAL(args[3]);
				if (n > 4) default_node_surface  = PARSEREAL(args[4]);
				if (n > 5) strncpy(default_node_options, args[5].c_str(), 50);
				
				if (default_node_friction < 0)   default_node_friction=NODE_FRICTION_COEF_DEFAULT;
				if (default_node_volume < 0)     default_node_volume=NODE_VOLUME_COEF_DEFAULT;
				if (default_node_surface < 0)    default_node_surface=NODE_SURFACE_COEF_DEFAULT;
				if (default_node_loadweight < 0) default_node_loadweight=NODE_LOADWEIGHT_DEFAULT;
				if (n <= 4) memset(default_node_options, 0, sizeof default_node_options);

				continue;
			}

			if (c.line.size() > 21 && c.line.substr(0, 21) == "set_skeleton_settings")
			{
				int n = parse_args(c, args, 2);
				fadeDist = PARSEREAL(args[1]);
				if (n > 2) skeleton_beam_diameter = PARSEREAL(args[2]);
				if (fadeDist < 0)
					fadeDist = 150;
				if (skeleton_beam_diameter < 0)
					skeleton_beam_diameter = BEAM_SKELETON_DIAMETER;
				continue;
			}

			if (c.line == "backmesh")
			{
				//close the current mesh
				subtexcoords[free_sub]=free_texcoord;
				subcabs[free_sub]=free_cab;
				if (free_sub >= MAX_SUBMESHES)
				{
					parser_warning(c, "submesh limit reached ("+TOSTRING(MAX_SUBMESHES)+")", PARSER_ERROR);
					continue;
				}
				//make it normal
				subisback[free_sub]=0;
				free_sub++;

				//add an extra front mesh
				int i;
				int start;
				//texcoords
				if (free_sub==1) start=0; else start=subtexcoords[free_sub-2];
				for (i=start; i<subtexcoords[free_sub-1]; i++)
				{
					texcoords[free_texcoord]=texcoords[i];;
					free_texcoord++;
				}
				//cab
				if (free_sub==1) start=0; else start=subcabs[free_sub-2];
				for (i=start; i<subcabs[free_sub-1]; i++)
				{
					cabs[free_cab*3]=cabs[i*3];
					cabs[free_cab*3+1]=cabs[i*3+1];
					cabs[free_cab*3+2]=cabs[i*3+2];
					free_cab++;
				}
				//finish it, this is a window
				subisback[free_sub]=2;
				//close the current mesh
				subtexcoords[free_sub]=free_texcoord;
				subcabs[free_sub]=free_cab;
				//make is transparent
				free_sub++;


				//add an extra back mesh
				//texcoords
				if (free_sub==1) start=0; else start=subtexcoords[free_sub-2];
				for (i=start; i<subtexcoords[free_sub-1]; i++)
				{
					texcoords[free_texcoord]=texcoords[i];;
					free_texcoord++;
				}
				//cab
				if (free_sub==1) start=0; else start=subcabs[free_sub-2];
				for (i=start; i<subcabs[free_sub-1]; i++)
				{
					cabs[free_cab*3]=cabs[i*3+1];
					cabs[free_cab*3+1]=cabs[i*3];
					cabs[free_cab*3+2]=cabs[i*3+2];
					free_cab++;
				}
				//we don't finish, there will be a submesh statement later
				subisback[free_sub]=1;
				continue;
			};
			if (c.line == "submesh")
			{
				subtexcoords[free_sub]=free_texcoord;
				subcabs[free_sub]=free_cab;
				if (free_sub >= MAX_SUBMESHES)
				{
					parser_warning(c, "submesh limit reached ("+TOSTRING(MAX_SUBMESHES)+")", PARSER_ERROR);
					continue;
				}
				free_sub++;
				//initialize the next
				subisback[free_sub]=0;
				continue;
			};

			if (c.line.size() > 21 && c.line.substr(0, 21) == "set_collision_range")
			{
				parse_args(c, args, 2);
				collrange = PARSEREAL(args[1]);
				if (collrange < 0)
					collrange = DEFAULT_COLLISION_RANGE;
				continue;
			};

			if (c.mode == BTS_NODES || c.mode == BTS_NODES2)
			{
				//parse nodes
				int id = 0;
				float x=0, y=0, z=0, mass=0;
				char options[256] = "n";
				int n = parse_args(c, args, 4);
				
				if (c.mode == BTS_NODES)
				{
					// classic approach, number needs to be in sync with free node count
					id = PARSEINT (args[0]);
				} else if (c.mode == BTS_NODES2)
				{
					// named nodes, we use the free_node counter and use the first argument as name instead
					id = free_node;
					// add it to a dictionary
					node_names[args[0]] = id;
				}
				
				x  = PARSEREAL(args[1]);
				y  = PARSEREAL(args[2]);
				z  = PARSEREAL(args[3]);
				if (n > 4) strncpy(options, args[4].c_str(), 255);
				if (n > 5) mass = PARSEREAL(args[5]);

				if (id != free_node)
				{
#ifdef USE_MYGUI
					Console *console = Console::getSingletonPtrNoCreation();
					if (console) console->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, "unable to load vehicle (lost sync in nodes numbers): " + filename, "error.png", 30000, true);
#endif // USE_MYGUI

					parser_warning(c, "lost sync in nodes numbers after node " + TOSTRING(free_node) + "(got " + TOSTRING(id) + " instead)", PARSER_FATAL_ERROR);
					return -2;
				};

				if (free_node >= MAX_NODES)
				{
					parser_warning(c, "nodes limit reached ("+TOSTRING(MAX_NODES)+")", PARSER_ERROR);
					continue;
				}
				Vector3 npos = pos + rot * Vector3(x,y,z);
				init_node(id, npos.x, npos.y, npos.z, NODE_NORMAL, 10, 0, 0, free_node, -1, default_node_friction, default_node_volume, default_node_surface, default_node_loadweight);
				nodes[id].iIsSkin = true;
				nodes[id].lockgroup = lockgroup_default;

				if 	(default_node_loadweight >= 0.0f)
				{
					nodes[id].mass = default_node_loadweight;
					nodes[id].masstype = NODE_LOADED;
					nodes[id].overrideMass = true;
				}

				// merge options and default_node_options
				strncpy(options, ((String(default_node_options) + String(options)).c_str()), 250);

				//initialize case 'h' variables
				int pos=0;
				node_t* id_hook = &nodes[0];

				// now 'parse' the options
				char *options_pointer = options;
				while (*options_pointer != 0)
				{
					switch (*options_pointer)
					{
						case 'l':	// load node
							if (mass != 0)
							{
								nodes[id].masstype=NODE_LOADED;
								nodes[id].overrideMass=true;
								nodes[id].mass=mass;
							}
							else
							{
								nodes[id].masstype=NODE_LOADED;
								masscount++;
							}
							break;
						case 'x':	//exhaust
							if (disable_smoke)
								break;
							if (smokeId == 0 && smokeRef != 0)
							{
								exhaust_t e;
								e.emitterNode = id;
								e.directionNode = smokeRef;
								e.isOldFormat = true;
								if (!virtuallyLoaded)
								{
									e.smokeNode = parent->createChildSceneNode();
									//ParticleSystemManager *pSysM=ParticleSystemManager::getSingletonPtr();
									char wname[256];
									sprintf(wname, "exhaust-%d-%s", (int)exhausts.size(), truckname);
									//if (pSysM) smoker=pSysM->createSystem(wname, "tracks/Smoke");
									e.smoker=gEnv->sceneManager->createParticleSystem(wname, "tracks/Smoke");
									if (!e.smoker) continue;
									e.smoker->setVisibilityFlags(DEPTHMAP_DISABLED); // disable particles in depthmap
									// ParticleSystem* pSys = ParticleSystemManager::getSingleton().createSystem("exhaust", "tracks/Smoke");
									e.smokeNode->attachObject(e.smoker);
									e.smokeNode->setPosition(nodes[e.emitterNode].AbsPosition);
								}
								exhausts.push_back(e);

								nodes[smokeId].isHot=true;
								nodes[id].isHot=true;
							}
							smokeId = id;
							break;
						case 'y':	//exhaust reference
							if (disable_smoke)
								break;
							if (smokeId != 0 && smokeRef == 0)
							{
								exhaust_t e;
								e.emitterNode = smokeId;
								e.directionNode = id;
								e.isOldFormat = true;
								//smokeId=id;
								if (!virtuallyLoaded)
								{
									e.smokeNode = parent->createChildSceneNode();
									//ParticleSystemManager *pSysM=ParticleSystemManager::getSingletonPtr();
									char wname[256];
									sprintf(wname, "exhaust-%d-%s", (int)exhausts.size(), truckname);
									//if (pSysM) smoker=pSysM->createSystem(wname, "tracks/Smoke");
									e.smoker=gEnv->sceneManager->createParticleSystem(wname, "tracks/Smoke");
									if (!e.smoker)  continue;
									e.smoker->setVisibilityFlags(DEPTHMAP_DISABLED); // disable particles in depthmap
									// ParticleSystem* pSys = ParticleSystemManager::getSingleton().createSystem("exhaust", "tracks/Smoke");
									e.smokeNode->attachObject(e.smoker);
									e.smokeNode->setPosition(nodes[e.emitterNode].AbsPosition);
								}
								exhausts.push_back(e);

								nodes[smokeId].isHot=true;
								nodes[id].isHot=true;
							}
							smokeRef = id;
							break;
						case 'c':	//contactless
							nodes[id].contactless = 1;
							break;
						case 'm':	// not able to mouse grab
							nodes[id].mouseGrabMode = 1;
							break;
						case 'n':	// mouse grab with hilight
							nodes[id].mouseGrabMode = 2;
							break;
						case 'h':	//hook
							// emulate the old behaviour using new fancy hookgroups
							//id check, avoid beam n0->n0
							if (id == 0)
							{
								 id_hook = &nodes[1];
							}

							pos = add_beam(parent, &nodes[id], id_hook, BEAM_HYDRO, default_break * default_break_scale * 100.0f, default_spring * default_spring_scale, default_damp * default_damp_scale * 0.1f);
							beams[pos].L                 = HOOK_RANGE_DEFAULT;
							beams[pos].refL              = beams[pos].L;
							beams[pos].Lhydro            = beams[pos].refL;
							beams[pos].bounded           = ROPE;
							beams[pos].disabled          = true;
							beams[pos].commandRatioShort = HOOK_SPEED_DEFAULT;
							beams[pos].commandRatioLong  = HOOK_SPEED_DEFAULT;
							beams[pos].commandShort      = 0.0f;
							beams[pos].commandLong       = 1.0f;
							beams[pos].maxtiestress      = HOOK_FORCE_DEFAULT;
							if (!virtuallyLoaded && beams[pos].mSceneNode)
								beams[pos].mSceneNode->detachAllObjects();

							hook_t h;
							h.hookNode     = &nodes[id];
							h.group        = -1;
							h.locked       = UNLOCKED;
							h.lockNode     = 0;
							h.lockTruck    = 0;
							h.lockNodes    = true;
							h.lockgroup    = -1;
							h.beam         = &beams[pos];
							h.maxforce     = HOOK_FORCE_DEFAULT;
							h.lockrange    = HOOK_RANGE_DEFAULT;
							h.lockspeed    = HOOK_SPEED_DEFAULT;
							h.selflock     = false;
							h.nodisable    = false;
							h.timer        = 0.0f;
							h.timer_preset = HOOK_LOCK_TIMER_DEFAULT;
							h.autolock     = false;
							hooks.push_back(h);
						break;
						case 'e':	//editor
							if (!networking)
								editorId=id;
							break;
						case 'b':	//buoy
							nodes[id].buoyancy=10000.0f;
							break;
						case 'p':	//diasble particles
							nodes[id].disable_particles=true;
						break;
						case 'f':	//diasble particles
							nodes[id].disable_sparks=true;
						break;
						case 'L':	//Log data:
							parser_warning(c, "Node " + TOSTRING(id) + "  settings. Node load mass: " + TOSTRING(nodes[id].mass) + ", friction coefficient: " + TOSTRING(default_node_friction) + " and buoyancy volume coefficient: " + TOSTRING(default_node_volume) + " Fluid drag surface coefficient: " + TOSTRING(default_node_surface)+ " Particle mode: " + TOSTRING(nodes[id].disable_particles), PARSER_INFO);
						break;
					}
					options_pointer++;
				}
				free_node++;

				// fusedrag autoclac y & z span
				if (z < fuse_z_min)
					fuse_z_min = z;
				if (z > fuse_z_max)
					fuse_z_max = z;
				if (y < fuse_y_min)
					fuse_y_min = y;
				if (y > fuse_y_max)
					fuse_y_max = y;

				continue;
			}
			else if (c.mode == BTS_CAMERARAIL)
			{
				int n = parse_args(c, args, 1);
				if (n>0 && free_camerarail < MAX_CAMERARAIL)
				{
					cameraRail[free_camerarail] = parse_node_number(c, args[0]);
					free_camerarail++;
				}
			}
			else if (c.mode == BTS_LOCKGROUPS)
			{
				//parse lockgroups
				int lockgroup = 0, id = 0, i = 0;
				int n = parse_args(c, args, 1);
				if (n>1)
				{
					lockgroup = PARSEINT(args[0]);
				} else
				{
					parser_warning(c, "Trying to parse a lockgroup without nodes defined.", PARSER_ERROR);
					continue;
				}

				// get the node#
				for (i=1; i<n;i++)
				{
					String arg = args[i];
					StringUtil::trim(arg);

					std::vector<int> special_numbers;
					special_numbers.push_back(-1);
					special_numbers.push_back(9999);

					id = parse_node_number(c, arg);
					if (id >= 0 && id <= free_node-1)
						nodes[id].lockgroup = lockgroup;
					else
						parser_warning(c, "Trying to parse a lockgroup with a node that does not exist. Node#: ("+TOSTRING(id)+")", PARSER_ERROR);
				}
				continue;
			}
			else if (c.mode == BTS_HOOKS)
			{
				//parse hooks
				float speedcoef = 1.0f, hookforce=HOOK_FORCE_DEFAULT, hookrange=HOOK_RANGE_DEFAULT, hooktimer=HOOK_LOCK_TIMER_DEFAULT, hook_shortlimit = 0.0f;
				int id = 0, lgroup = -1, hgroup = -1, i = 0;
				bool hook_selflock = false, hook_autolock = false, hook_nodisable = false, hookbeam_visble = false;

				int n = parse_args(c, args, 1);
				if (n>1) id = parse_node_number(c, args[0]);

				//check if this is really a hook
				std::vector <hook_t>::iterator itfound = hooks.end();
				for (std::vector <hook_t>::iterator it = hooks.begin(); it!=hooks.end(); it++)
				{
					if (it->hookNode == &nodes[id])
					{
						itfound = it;
						break;
					}
				}

				if (itfound == hooks.end() || id < 0 || id > free_node-1)
				{
					parser_warning(c, "Trying to parse a none existing hooknode ("+TOSTRING(id)+")", PARSER_ERROR);
					continue;
				}
		

				for (i=1; i<n;i++)
				{
					String arg = args[i];
					StringUtil::trim(arg);

					if      (arg == "hookrange" && n > i)
					{
						i++;
						String arg2 = args[i];
						hookrange = PARSEREAL(arg2);
					}
					else if (arg == "speedcoef" && n > i)
					{
						i++;
						String arg2 = args[i];
						speedcoef = PARSEREAL(arg2);
					}
					else if (arg == "maxforce" && n > i)
					{
						i++;
						String arg2 = args[i];
						hookforce = PARSEREAL(arg2);
					}
					else if ((arg == "hookgroup" || arg == "hgroup") && n > i)
					{
						i++;
						String arg2 = args[i];
						hgroup = PARSEINT(arg2);
					}
					else if ((arg == "lockgroup" || arg == "lgroup") && n > i)
					{
						i++;
						String arg2 = args[i];
						lgroup = PARSEINT(arg2);
					}

					else if (arg == "timer" && n >= i)
					{
						i++;
						String arg2 = args[i];
						hooktimer = PARSEREAL(arg2);
					}
					else if ((arg == "shortlimit" || arg == "short_limit" || arg == "short_limit") && n >= i)
					{
						i++;
						String arg2 = args[i];
						hook_shortlimit = PARSEREAL(arg2);
					}
					else if (arg == "selflock" || arg == "self-lock" || arg == "self_lock")
					{
						hook_selflock = true;
					}
					else if (arg == "autolock" || arg == "auto-lock" || arg == "auto_lock")
					{
						//only overwrite hgroup when its still default (-1) and hookgroup was not found yet in this line to parse
						if (hgroup == -1 && itfound->group ==-1)
							hgroup = -2;
						hook_autolock = true;
					}
					else if (arg == "nodisable" || arg == "no-disable" || arg == "no_disable")
					{
						hook_nodisable = true;
					}
					else if ((arg == "visible" || arg == "vis") && !virtuallyLoaded && itfound->beam->mSceneNode)
					{
						hookbeam_visble = true;
						if (itfound->beam->mSceneNode->numAttachedObjects() == 0)
							itfound->beam->mSceneNode->attachObject(itfound->beam->mEntity);
					}
					else if (arg == "norope" || arg == "no-rope" || arg == "no_rope")
					{
						itfound->beam->bounded = NOSHOCK;
					}
				}
				itfound->group              = hgroup;
				itfound->lockgroup          = lgroup;
				itfound->maxforce           = hookforce;
				itfound->lockrange          = hookrange;
				itfound->lockspeed          = speedcoef * HOOK_SPEED_DEFAULT;
				itfound->selflock           = hook_selflock;
				itfound->autolock           = hook_autolock;
				itfound->nodisable          = hook_nodisable;
				itfound->visible            = hookbeam_visble;
				itfound->timer              = 0.0f;
				itfound->timer_preset       = hooktimer;
				itfound->beam->commandShort = hook_shortlimit;
				continue;
			}
			else if (c.mode == BTS_BEAMS)
			{
				//parse beams
				int id1, id2;
				float support_break_factor = -1.0f;
				char options[50] = "v";
				int type=BEAM_NORMAL;

				int n = parse_args(c, args, 2);
				id1 = parse_node_number(c, args[0]);
				id2 = parse_node_number(c, args[1]);
				if (n > 2) strncpy(options, args[2].c_str(), 50);
				if (n > 3) support_break_factor = PARSEINT(args[3]);

				if (free_beam >= MAX_BEAMS)
				{
					parser_warning(c, "beams limit reached ("+TOSTRING(MAX_BEAMS)+")", PARSER_ERROR);
					continue;
				}

				// FIXME: separate init_beam and setup_beam to be able to set all parameters after creation
				// this is just ugly:
				char *options_pointer = options;
				while (*options_pointer != 0) {
					if (*options_pointer=='i') {
						type=BEAM_INVISIBLE;
						break;
					}
					options_pointer++;
				}

				int pos=add_beam(parent, &nodes[id1], &nodes[id2], \
				type, default_break * default_break_scale, default_spring * default_spring_scale, \
				default_damp * default_damp_scale, detacher_group_state, -1, -1, -1, 1, \
				default_beam_diameter);

				// now 'parse' the options
				options_pointer = options;
				while (*options_pointer != 0)
				{
					switch (*options_pointer)
					{
						case 'i':	// invisible
							beams[pos].type=BEAM_INVISIBLE;
							break;
						case 'v':	// visible
							beams[pos].type=BEAM_NORMAL;
							break;
						case 'r':
							beams[pos].bounded=ROPE;
							break;
						case 's':
							beams[pos].bounded=SUPPORTBEAM;
							float support_break_limit = 0.0f;
							if (support_break_factor > 0.0f)
								support_break_limit = support_break_factor;
							beams[pos].longbound = support_break_limit;
							break;
					}
					options_pointer++;
				}
				continue;
			}
			else if (c.mode == BTS_TRIGGER)
			{
				//parse triggers
				int id1, id2, triggershort, triggerlong;
				float sbound, lbound, boundarytimer = 1.0f;
				char options[50]="n";
				int n = parse_args(c, args, 6);
				id1          = parse_node_number(c, args[0]);
				id2          = parse_node_number(c, args[1]);
				sbound       = PARSEREAL(args[2]);
				lbound       = PARSEREAL(args[3]);
				triggershort = PARSEINT (args[4]);
				triggerlong  = PARSEINT (args[5]);
				if (n > 6) strncpy(options, args[6].c_str(), 50);
				if (n > 7) boundarytimer = PARSEREAL(args[7]);

				// checks ...
				if (free_beam >= MAX_BEAMS)
				{
					parser_warning(c, "Triggers, beams limit reached ("+TOSTRING(MAX_BEAMS)+")", PARSER_ERROR);
					continue;
				}
				if (free_shock >= MAX_BEAMS)
				{
					parser_warning(c, "Triggers limit reached ("+TOSTRING(MAX_SHOCKS)+")", PARSER_ERROR);
					continue;
				}
				// options
				int htype=BEAM_HYDRO;
				int shockflag = SHOCK_FLAG_NORMAL | SHOCK_FLAG_ISTRIGGER;
				shocks[free_shock].trigger_enabled = true;
				bool triggerblocker = false;
				bool triggerblocker_inverted = false;
				bool cmdkeyblock = false;
				bool hooktoggle = false;
				bool enginetrigger = false;
				commandkey[triggershort].trigger_cmdkeyblock_state = false;
				if (triggerlong != -1) commandkey[triggerlong].trigger_cmdkeyblock_state = false;

				// now 'parse' the options
				char *options_pointer = options;
				while (*options_pointer != 0)
				{
					switch (*options_pointer)
					{
						case 'i':	// invisible
							htype = BEAM_INVISIBLE_HYDRO;
							shockflag |= SHOCK_FLAG_INVISIBLE;
							break;
						case 'x':	// this trigger is disabled on startup, default is enabled
							shocks[free_shock].trigger_enabled = false;
							break;
						case 'B':	// Blocker that enable/disable other triggers
							shockflag |= SHOCK_FLAG_TRG_BLOCKER;
							triggerblocker = true;
							break;
						case 'b':	// Set the CommandKeys that are set in a commandkeyblocker or trigger to blocked on startup, default is released
							cmdkeyblock = true;
							break;
						case 's':	// switch that exchanges cmdshort/cmdshort for all triggers with the same commandnumbers, default false
							shockflag |= SHOCK_FLAG_TRG_CMD_SWITCH;
							break;
						case 'c':	// trigger is set with commandstyle boundaries instead of shocksytle
							sbound = abs(sbound-1);
							lbound = lbound-1;
							break;
						case 'A':	// Blocker that enable/disable other triggers, reversed activation method (inverted Blocker style, auto-ON)
							shockflag |= SHOCK_FLAG_TRG_BLOCKER_A;
							triggerblocker_inverted = true;
							break;
						case 'h':	// Blocker that enable/disable other triggers, reversed activation method (inverted Blocker style, auto-ON)
							shockflag |= SHOCK_FLAG_TRG_HOOK_UNLOCK;
							hooktoggle = true;
							break;
						case 'H':	// Blocker that enable/disable other triggers, reversed activation method (inverted Blocker style, auto-ON)
							shockflag |= SHOCK_FLAG_TRG_HOOK_LOCK;
							hooktoggle = true;
							break;
						case 't': // this trigger sends values between 0 and 1
							shockflag |= SHOCK_FLAG_TRG_CONTINUOUS;
							break;
						case 'E': // this trigger is used to control an engine
							shockflag |= SHOCK_FLAG_TRG_ENGINE;
							enginetrigger = true;
							break;
					}
					options_pointer++;
				}

				if (!triggerblocker && !triggerblocker_inverted && !hooktoggle && !enginetrigger)
				{
					// make the full check
					if (triggershort < 1 || triggershort > MAX_COMMANDS)
					{
						parser_warning(c, "Error: Wrong command-eventnumber (Triggers). Trigger deactivated.", PARSER_ERROR);
						continue;
					}
				} else if (!hooktoggle && !enginetrigger)
				{
					// this is a Trigger-Blocker, make special check
					if (triggershort < 0 || triggerlong < 0)
					{
						parser_warning(c, "Error: Wrong command-eventnumber (Triggers). Trigger-Blocker deactivated.", PARSER_ERROR);
						continue;
					}
				} else if (enginetrigger)
				{
					if (triggerblocker || triggerblocker_inverted || hooktoggle || (shockflag & SHOCK_FLAG_TRG_CMD_SWITCH))
					{
						parser_warning(c, "Error: Wrong command-eventnumber (Triggers). Engine trigger deactivated.", PARSER_ERROR);
						continue;
					}
				}

				int pos = add_beam(parent, &nodes[id1], &nodes[id2], htype, default_break, 0.0f, 0.0f, detacher_group_state, -1.0, sbound, lbound, 1.0f);
				beams[pos].bounded = SHOCK2;

				if (triggerdebug)
					parser_warning(c, "Trigger added. BeamID " + TOSTRING(pos), PARSER_INFO);
				beams[pos].shock = &shocks[free_shock];
				shocks[free_shock].beamid = pos;
				shocks[free_shock].trigger_switch_state = 0.0f;   // used as bool and countdowntimer, dont touch!
				if (!triggerblocker && !triggerblocker_inverted) // this is no triggerblocker (A/B)
				{
					shocks[free_shock].trigger_cmdshort = triggershort;
					if (triggerlong != -1 || (triggerlong == -1 && hooktoggle))	// this is a trigger or a hooktoggle
						shocks[free_shock].trigger_cmdlong = triggerlong;
					else // this is a commandkeyblocker
						shockflag |= SHOCK_FLAG_TRG_CMD_BLOCKER;
				} else // this is a triggerblocker
				{
					if (!triggerblocker_inverted)
					{
						//normal BLOCKER
						shockflag |= SHOCK_FLAG_TRG_BLOCKER;
						shocks[free_shock].trigger_cmdshort = triggershort;
						shocks[free_shock].trigger_cmdlong = triggerlong;
					} else
					{
						//inverted BLOCKER
						shockflag |= SHOCK_FLAG_TRG_BLOCKER_A;
						shocks[free_shock].trigger_cmdshort = triggershort;
						shocks[free_shock].trigger_cmdlong = triggerlong;
					}
				}

				if (cmdkeyblock && !triggerblocker)
				{
					commandkey[triggershort].trigger_cmdkeyblock_state = true;
					if (triggerlong != -1) commandkey[triggerlong].trigger_cmdkeyblock_state = true;
				}
				if (boundarytimer > 0)
					shocks[free_shock].trigger_boundary_t = boundarytimer;
				else
					shocks[free_shock].trigger_boundary_t = 1.0f;

				shocks[free_shock].flags = shockflag;
				shocks[free_shock].sbd_spring = default_spring;
				shocks[free_shock].sbd_damp = default_damp;
				shocks[free_shock].last_debug_state = 0;
				free_shock++;
				continue;
			}
			else if (c.mode == BTS_SHOCKS)
			{
				//parse shocks
				int id1, id2;
				float s, d, sbound,lbound,precomp;
				char options[50]="n";
				int n = parse_args(c, args, 7);
				id1          = parse_node_number(c, args[0]);
				id2          = parse_node_number(c, args[1]);
				s            = PARSEREAL(args[2]);
				d            = PARSEREAL(args[3]);
				sbound       = PARSEREAL(args[4]);
				lbound       = PARSEREAL(args[5]);
				precomp      = PARSEREAL(args[6]);
				if (n > 7) strncpy(options, args[7].c_str(), 50);

				// checks ...
				if (free_beam >= MAX_BEAMS)
				{
					parser_warning(c, "beams limit reached ("+TOSTRING(MAX_BEAMS)+")", PARSER_ERROR);
					continue;
				}
				if (free_shock >= MAX_BEAMS)
				{
					parser_warning(c, "shock limit reached ("+TOSTRING(MAX_SHOCKS)+")", PARSER_ERROR);
					continue;
				}

				// options
				int htype = BEAM_HYDRO;
				int shockflag = SHOCK_FLAG_NORMAL;

				// now 'parse' the options
				char *options_pointer = options;
				while (*options_pointer != 0)
				{
					switch (*options_pointer)
					{
						case 'i':	// invisible
							htype=BEAM_INVISIBLE_HYDRO;
							shockflag |= SHOCK_FLAG_INVISIBLE;
							break;
						case 'l':
						case 'L':
							shockflag &= ~SHOCK_FLAG_NORMAL; // not normal anymore
							shockflag |= SHOCK_FLAG_LACTIVE;
							free_active_shock++; // this has no array associated with it. its just to determine if there are active shocks!
							break;
						case 'r':
						case 'R':
							shockflag &= ~SHOCK_FLAG_NORMAL; // not normal anymore
							shockflag |= SHOCK_FLAG_RACTIVE;
							free_active_shock++; // this has no array associated with it. its just to determine if there are active shocks!
							break;
						case 'm':
							{
								// metric values: calculate sbound and lbound now
								float beam_length = nodes[id1].AbsPosition.distance(nodes[id2].AbsPosition);
								sbound = sbound / beam_length;
								lbound = lbound / beam_length;
							}
							break;
					}
					options_pointer++;
				}
				int pos = add_beam(parent, &nodes[id1], &nodes[id2], htype, default_break*4.0, s, d, detacher_group_state, -1.0, sbound, lbound, precomp);
				beams[pos].shock = &shocks[free_shock];
				shocks[free_shock].beamid = pos;
				shocks[free_shock].flags = shockflag;
				shocks[free_shock].sbd_spring = default_spring;
				shocks[free_shock].sbd_damp = default_damp;
				free_shock++;
				continue;
			}
			else if (c.mode == BTS_SHOCKS2)
			{
				//parse shocks2
				int id1, id2;
				float sin=-1.0f,din=-1.0f,psin=-1.0f,pdin=-1.0f,sout=-1.0f,dout=-1.0f,psout=-1.0f,pdout=-1.0f,sbound=-1.0f,lbound=-1.0f,precomp=-1.0f;
				char options[50]="n";
				int n = parse_args(c, args, 13);
				id1          = parse_node_number(c, args[0]);
				id2          = parse_node_number(c, args[1]);
				sin          = PARSEREAL(args[2]);
				din          = PARSEREAL(args[3]);
				psin         = PARSEREAL(args[4]);
				pdin         = PARSEREAL(args[5]);
				sout         = PARSEREAL(args[6]);
				dout         = PARSEREAL(args[7]);
				psout        = PARSEREAL(args[8]);
				pdout        = PARSEREAL(args[9]);
				sbound       = PARSEREAL(args[10]);
				lbound       = PARSEREAL(args[11]);
				precomp      = PARSEREAL(args[12]);
				if (n > 13) strncpy(options, args[13].c_str(), 50);

				// checks ...
				if (free_beam >= MAX_BEAMS)
				{
					parser_warning(c, "beams limit reached ("+TOSTRING(MAX_BEAMS)+")", PARSER_ERROR);
					continue;
				}
				if (free_shock >= MAX_BEAMS)
				{
					parser_warning(c, "shock limit reached ("+TOSTRING(MAX_SHOCKS)+")", PARSER_ERROR);
					continue;
				}
				if ( sin == -1.0f || din == -1.0f || psin == -1.0f || pdin == -1.0f || sout == -1.0f || dout == -1.0f || psout == -1.0f || pdout == -1.0f || sbound == -1.0f || lbound == -1.0f || precomp == -1.0f)
				{
#ifdef USE_MYGUI
					Console *console = Console::getSingletonPtrNoCreation();
					if (console) console->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, "unable to load vehicle (Wrong values in shocks2 section): " + filename, "error.png", 30000, true);
#endif // USE_MYGUI

					parser_warning(c, "Error: Wrong values in shocks2 section ("+TOSTRING(id1)+","+TOSTRING(id2)+")", PARSER_FATAL_ERROR);
					return -7;
				}

				// options
				int htype=BEAM_HYDRO;
				int shockflag = SHOCK_FLAG_NORMAL | SHOCK_FLAG_ISSHOCK2;

				// now 'parse' the options
				char *options_pointer = options;
				while (*options_pointer != 0)
				{
					switch (*options_pointer)
					{
						case 'i':
						{
							// invisible
							htype=BEAM_INVISIBLE_HYDRO;
							shockflag |= SHOCK_FLAG_INVISIBLE;
							break;
						}
						case 's':
						{
							// passive shock
							shockflag &= ~SHOCK_FLAG_NORMAL; // not normal anymore
							shockflag |= SHOCK_FLAG_SOFTBUMP;
							break;
						}
						case 'm':
						{
							// metric values: calculate sbound and lbound now
							float beam_length = nodes[id1].AbsPosition.distance(nodes[id2].AbsPosition);
							sbound = sbound / beam_length;
							lbound = lbound / beam_length;
							break;
						}
						case 'M':
						{
							// metric values: calculate sbound and lbound now
							float beam_length = nodes[id1].AbsPosition.distance(nodes[id2].AbsPosition);
							sbound = (beam_length - sbound) / beam_length;
							lbound = (lbound - beam_length) / beam_length;

							if (lbound < 0)
							{
								parser_warning(c, "Metric shock length calculation failed, longbound less then beams spawn length, reset to beams spawn length (longbound=0).", PARSER_ERROR);
								lbound = 0.0f;
							}

							if (sbound > 1)
							{
								parser_warning(c, "Metric shock length calculation failed, shortbound less then 0 meters, reset to 0 meters (shortbound=1).", PARSER_ERROR);
								sbound = 1.0f;
							}
						}
						break;
					}
					options_pointer++;
				}
				int pos=add_beam(parent, &nodes[id1], &nodes[id2], htype, default_break*4.0, sin, din, detacher_group_state, -1.0, sbound, lbound, precomp);
				beams[pos].bounded=SHOCK2;
				beams[pos].shock = &shocks[free_shock];
				shocks[free_shock].springin = sin;
				shocks[free_shock].dampin = din;
				shocks[free_shock].sprogin = psin;
				shocks[free_shock].dprogin = pdin;
				shocks[free_shock].springout = sout;
				shocks[free_shock].dampout = dout;
				shocks[free_shock].sprogout = psout;
				shocks[free_shock].dprogout = pdout;
				shocks[free_shock].beamid = pos;
				shocks[free_shock].flags = shockflag;
				shocks[free_shock].sbd_spring = default_spring;
				shocks[free_shock].sbd_damp = default_damp;
				free_shock++;
				continue;
			}

			else if (c.mode == BTS_FIXES)
			{
				//parse fixes
				parse_args(c, args, 1);
				int id = parse_node_number(c, args[0]);
				nodes[id].locked = 1;
				hasfixes         = 1;
				free_fixes++;
				continue;
			}
			else if (c.mode == BTS_HYDROS)
			{
				//parse hydros
				int id1, id2;
				float ratio;
				char options[50] = "n";
				Real startDelay=0;
				Real stopDelay=0;
				char startFunction[50]="";
				char stopFunction[50]="";

				int n = parse_args(c, args, 3);
				id1 = parse_node_number(c, args[0]);
				id2 = parse_node_number(c, args[1]);
				ratio = PARSEREAL(args[2]);
				if (n > 3) strncpy(options, args[3].c_str(), 50);
				if (n > 4) startDelay = PARSEREAL(args[4]);
				if (n > 5) stopDelay = PARSEREAL(args[5]);
				if (n > 6) strncpy(startFunction, args[6].c_str(), 50);
				if (n > 7) strncpy(stopFunction, args[7].c_str(), 50);

				int htype=BEAM_HYDRO;

				// FIXME: separate init_beam and setup_beam to be able to set all parameters after creation
				// this is just ugly:
				char *options_pointer = options;
				while (*options_pointer != 0)
				{
					if (*options_pointer=='i')
					{
						htype=BEAM_INVISIBLE_HYDRO;
						break;
					}
					options_pointer++;
				}

				if (free_beam >= MAX_BEAMS)
				{
					parser_warning(c, "beams limit reached ("+TOSTRING(MAX_BEAMS)+")", PARSER_ERROR);
					continue;
				}
				if (free_hydro >= MAX_HYDROS)
				{
					parser_warning(c, "hydros limit reached ("+TOSTRING(MAX_HYDROS)+")", PARSER_ERROR);
					continue;
				}

				if (hydroInertia && startDelay != 0 && stopDelay != 0)
					hydroInertia->setCmdKeyDelay(free_hydro,startDelay,stopDelay,String (startFunction), String (stopFunction));
				else if (hydroInertia && (inertia_startDelay > 0 || inertia_stopDelay > 0))
					hydroInertia->setCmdKeyDelay(free_hydro,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));


				int pos=add_beam(parent, &nodes[id1], &nodes[id2], htype, default_break * default_break_scale, default_spring * default_spring_scale, default_damp * default_damp_scale, detacher_group_state);
				hydro[free_hydro]=pos;free_hydro++;
				beams[pos].Lhydro=beams[pos].L;
				beams[pos].hydroRatio=ratio;
				beams[pos].animOption = 0.0f;


				// now 'parse' the options
				options_pointer = options;
				while (*options_pointer != 0)
				{
					switch (*options_pointer)
					{
						case 'i':	// invisible
							beams[pos].type = BEAM_INVISIBLE_HYDRO;
							break;
						case 'n':	// normal
							beams[pos].type = BEAM_HYDRO;
							beams[pos].hydroFlags |= HYDRO_FLAG_DIR;
							break;
						case 's': // speed changing hydro
							beams[pos].hydroFlags |= HYDRO_FLAG_SPEED;
							break;
						case 'a':
							beams[pos].hydroFlags |= HYDRO_FLAG_AILERON;
							break;
						case 'r':
							beams[pos].hydroFlags |= HYDRO_FLAG_RUDDER;
							break;
						case 'e':
							beams[pos].hydroFlags |= HYDRO_FLAG_ELEVATOR;
							break;
						case 'u':
							beams[pos].hydroFlags |= (HYDRO_FLAG_AILERON | HYDRO_FLAG_ELEVATOR);
							break;
						case 'v':
							beams[pos].hydroFlags |= (HYDRO_FLAG_REV_AILERON | HYDRO_FLAG_ELEVATOR);
							break;
						case 'x':
							beams[pos].hydroFlags |= (HYDRO_FLAG_AILERON | HYDRO_FLAG_RUDDER);
							break;
						case 'y':
							beams[pos].hydroFlags |= (HYDRO_FLAG_REV_AILERON | HYDRO_FLAG_RUDDER);
							break;
						case 'g':
							beams[pos].hydroFlags |= (HYDRO_FLAG_ELEVATOR | HYDRO_FLAG_RUDDER);
							break;
						case 'h':
							beams[pos].hydroFlags |= (HYDRO_FLAG_REV_ELEVATOR | HYDRO_FLAG_RUDDER);
							break;
					}
					options_pointer++;
					// if you use the i flag on its own, add the direction to it
					if (beams[pos].type == BEAM_INVISIBLE_HYDRO && !beams[pos].hydroFlags)
						beams[pos].hydroFlags |= HYDRO_FLAG_DIR;
				}
				continue;
			}
			else if (c.mode == BTS_ANIMATORS)
			{
				//parse animators
				int id1, id2;
				float ratio;
				Ogre::StringVector options = Ogre::StringUtil::split(c.line,",");
				if (options.size() < 4)
				{
					parser_warning(c, "Error parsing File (Animator) " + filename +" line " + StringConverter::toString(c.linecounter) + ". trying to continue ...", PARSER_ERROR);
					continue;
				}

				// the required values
				id1 = parse_node_number(c, options[0]);
				id2 = parse_node_number(c, options[1]);
				ratio = StringConverter::parseReal(options[2]);
				String optionStr = options[3];
				Ogre::StringVector optionArgs = Ogre::StringUtil::split(optionStr, "|:");

				int htype=BEAM_HYDRO;

				// detect invisible beams
				for (unsigned int i=0;i<optionArgs.size();i++)
				{
					String arg = optionArgs[i];
					StringUtil::trim(arg);
					if (arg == "inv")
						htype=BEAM_INVISIBLE_HYDRO;
				}

				if (id1>=free_node || id2>=free_node)
				{
#ifdef USE_MYGUI
					Console *console = Console::getSingletonPtrNoCreation();
					if (console) console->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, "unable to load vehicle (unknown node number in animators section): " + filename, "error.png", 30000, true);
#endif // USE_MYGUI

					parser_warning(c, "Error: unknown node number in animators section ("+TOSTRING(id1)+","+TOSTRING(id2)+")", PARSER_FATAL_ERROR);
					return -8;
				}

				if (free_beam >= MAX_BEAMS)
				{
					parser_warning(c, "beams limit reached ("+TOSTRING(MAX_BEAMS)+")", PARSER_ERROR);
					continue;
				}
				if (free_hydro >= MAX_HYDROS)
				{
					parser_warning(c, "hydros limit reached (via animators) ("+TOSTRING(MAX_HYDROS)+")", PARSER_ERROR);
					continue;
				}

				if (hydroInertia && (inertia_startDelay > 0 || inertia_stopDelay > 0))
					hydroInertia->setCmdKeyDelay(free_hydro,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));

				int pos=add_beam(parent, &nodes[id1], &nodes[id2], htype, default_break * default_break_scale, default_spring * default_spring_scale, default_damp * default_damp_scale, detacher_group_state);
				hydro[free_hydro]=pos;
				free_hydro++;
				beams[pos].Lhydro=beams[pos].L;
				beams[pos].hydroRatio=ratio;
				// set the limits to something with sense by default
				beams[pos].shortbound = 0.99999f;
				beams[pos].longbound = 1000000.0f;

				// parse the rest
				for (unsigned int i=0;i<optionArgs.size();i++)
				{
					String arg = optionArgs[i];
					StringUtil::trim(arg);
					beam_t *beam = &beams[pos];
					if      (arg == "vis")            { beam->type = BEAM_HYDRO; }
					else if (arg == "inv")            { htype=BEAM_INVISIBLE_HYDRO; }
					else if (arg == "airspeed")       { beam->animFlags |= (ANIM_FLAG_AIRSPEED); }
					else if (arg == "vvi")            { beam->animFlags |= (ANIM_FLAG_VVI); }
					else if (arg == "altimeter100k")  { beam->animFlags |= (ANIM_FLAG_ALTIMETER); beam->animOption = 1.0f; }
					else if (arg == "altimeter10k")   { beam->animFlags |= (ANIM_FLAG_ALTIMETER); beam->animOption = 2.0f; }
					else if (arg == "altimeter1k")    { beam->animFlags |= (ANIM_FLAG_ALTIMETER); beam->animOption = 3.0f; }
					else if (arg == "aoa")            { beam->animFlags |= (ANIM_FLAG_AOA); }
					else if (arg == "flap")           { beam->animFlags |= (ANIM_FLAG_FLAP); }
					else if (arg == "airbrake")       { beam->animFlags |= (ANIM_FLAG_AIRBRAKE); }
					else if (arg == "roll")           { beam->animFlags |= (ANIM_FLAG_ROLL); }
					else if (arg == "pitch")          { beam->animFlags |= (ANIM_FLAG_PITCH); }
					else if (arg == "throttle1")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 1.0f; }
					else if (arg == "throttle2")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 2.0f; }
					else if (arg == "throttle3")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 3.0f; }
					else if (arg == "throttle4")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 4.0f; }
					else if (arg == "throttle5")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 5.0f; }
					else if (arg == "throttle6")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 6.0f; }
					else if (arg == "throttle7")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 7.0f; }
					else if (arg == "throttle8")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 8.0f; }
					else if (arg == "rpm1")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 1.0f; }
					else if (arg == "rpm2")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 2.0f; }
					else if (arg == "rpm3")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 3.0f; }
					else if (arg == "rpm4")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 4.0f; }
					else if (arg == "rpm5")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 5.0f; }
					else if (arg == "rpm6")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 6.0f; }
					else if (arg == "rpm7")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 7.0f; }
					else if (arg == "rpm8")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 8.0f; }
					else if (arg == "aerotorq1")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 1.0f; }
					else if (arg == "aerotorq2")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 2.0f; }
					else if (arg == "aerotorq3")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 3.0f; }
					else if (arg == "aerotorq4")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 4.0f; }
					else if (arg == "aerotorq5")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 5.0f; }
					else if (arg == "aerotorq6")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 6.0f; }
					else if (arg == "aerotorq7")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 7.0f; }
					else if (arg == "aerotorq8")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 8.0f; }
					else if (arg == "aeropit1")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 1.0f; }
					else if (arg == "aeropit2")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 2.0f; }
					else if (arg == "aeropit3")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 3.0f; }
					else if (arg == "aeropit4")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 4.0f; }
					else if (arg == "aeropit5")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 5.0f; }
					else if (arg == "aeropit6")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 6.0f; }
					else if (arg == "aeropit7")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 7.0f; }
					else if (arg == "aeropit8")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 8.0f; }
					else if (arg == "aerostatus1")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 1.0f; }
					else if (arg == "aerostatus2")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 2.0f; }
					else if (arg == "aerostatus3")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 3.0f; }
					else if (arg == "aerostatus4")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 4.0f; }
					else if (arg == "aerostatus5")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 5.0f; }
					else if (arg == "aerostatus6")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 6.0f; }
					else if (arg == "aerostatus7")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 7.0f; }
					else if (arg == "aerostatus8")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 8.0f; }
					else if (arg == "brakes")         { beam->animFlags |= (ANIM_FLAG_BRAKE); }
					else if (arg == "accel")          { beam->animFlags |= (ANIM_FLAG_ACCEL); }
					else if (arg == "clutch")         { beam->animFlags |= (ANIM_FLAG_CLUTCH); }
					else if (arg == "speedo")         { beam->animFlags |= (ANIM_FLAG_SPEEDO); }
					else if (arg == "tacho")          { beam->animFlags |= (ANIM_FLAG_TACHO); }
					else if (arg == "turbo")          { beam->animFlags |= (ANIM_FLAG_TURBO); }
					else if (arg == "parking")        { beam->animFlags |= (ANIM_FLAG_PBRAKE); }
					else if (arg == "shifterman1")    { beam->animFlags |= (ANIM_FLAG_SHIFTER); beam->animOption= 1.0f; }
					else if (arg == "shifterman2")    { beam->animFlags |= (ANIM_FLAG_SHIFTER); beam->animOption= 2.0f; }
					else if (arg == "seqential")      { beam->animFlags |= (ANIM_FLAG_SHIFTER); beam->animOption= 3.0f; }
					else if (arg == "shifterlin")     { beam->animFlags |= (ANIM_FLAG_SHIFTER); beam->animOption= 4.0f; }
					else if (arg == "torque")         { beam->animFlags |= (ANIM_FLAG_TORQUE); }
					else if (arg == "throttleboat")   { beam->animFlags |= (ANIM_FLAG_BTHROTTLE); }
					else if (arg == "rudderboat")     { beam->animFlags |= (ANIM_FLAG_BRUDDER); }
					else if (arg == "shortlimit")
					{
						i++;
						String arg2 = optionArgs[i];
						StringUtil::trim(arg2);
						beams[pos].shortbound = StringConverter::parseReal(arg2);
					}
					else if (arg == "longlimit")
					{
						i++;
						String arg2 = optionArgs[i];
						StringUtil::trim(arg2);
						beams[pos].longbound = StringConverter::parseReal(arg2);
					}
					if (beam->animFlags == 0 && (arg != "shortlimit" || arg != "longlimit"))
						parser_warning(c, "Failed to identify source.", PARSER_ERROR);
					else
						parser_warning(c, "Animator source set: with flag "+TOSTRING(beams[pos].animFlags), PARSER_INFO);
				}
				continue;
			}

			else if (c.mode == BTS_WHEELS)
			{
				//parse wheels
				float radius, width, mass, spring, damp;
				char texf[256];
				char texb[256];
				int rays, node1, node2, snode, braked, propulsed, torquenode;
				int n = parse_args(c, args, 14);
				radius     = PARSEREAL(args[0]);
				width      = PARSEREAL(args[1]);
				rays       = PARSEINT (args[2]);
				node1      = parse_node_number(c, args[3]);
				node2      = parse_node_number(c, args[4]);

				std::vector<int> special_numbers;
				special_numbers.push_back(-1);
				special_numbers.push_back(9999);

				snode = parse_node_number(c, args[5], &special_numbers); // special behavior, beware

				braked     = PARSEINT (args[6]);
				propulsed  = PARSEINT (args[7]);
				torquenode = parse_node_number(c, args[8], 0, true);
				mass       = PARSEREAL(args[9]);
				spring     = PARSEREAL(args[10]);
				damp       = PARSEREAL(args[11]);
				strncpy(texf, args[12].c_str(), 255);
				strncpy(texb, args[13].c_str(), 255);
				addWheel(parent, radius,width,rays,node1,node2,snode,braked,propulsed, torquenode, mass, spring, damp, texf, texb);
				continue;
			}
			else if (c.mode == BTS_WHEELS2)
			{
				//parse wheels2
				char texf[256];
				char texb[256];
				float radius, radius2, width, mass, spring, damp, spring2, damp2;
				int rays, node1, node2, snode, braked, propulsed, torquenode;
				int n = parse_args(c, args, 17);
				radius     = PARSEREAL(args[0]);
				radius2    = PARSEREAL(args[1]);
				width      = PARSEREAL(args[2]);
				rays       = PARSEINT (args[3]);
				node1      = parse_node_number(c, args[4]);
				node2      = parse_node_number(c, args[5]);
				
				std::vector<int> special_numbers;
				special_numbers.push_back(-1);
				special_numbers.push_back(9999);

				snode = parse_node_number(c, args[6], &special_numbers); // special behavior, beware
				
				braked     = PARSEINT (args[7]);
				propulsed  = PARSEINT (args[8]);
				torquenode = parse_node_number(c, args[9]);
				mass       = PARSEREAL(args[10]);
				spring     = PARSEREAL(args[11]);
				damp       = PARSEREAL(args[12]);
				spring2    = PARSEREAL(args[13]);
				damp2      = PARSEREAL(args[14]);
				strncpy(texf, args[15].c_str(), 255);
				strncpy(texb, args[16].c_str(), 255);

				if (enable_wheel2)
					addWheel2(parent, radius,radius2,width,rays,node1,node2,snode,braked,propulsed, torquenode, mass, spring, damp, spring2, damp2, texf, texb);
				else
					addWheel(parent, radius2,width,rays,node1,node2,snode,braked,propulsed, torquenode, mass, spring2, damp2, texf, texb);
				continue;
			}
			else if (c.mode == BTS_MESHWHEELS)
			{
				//parse meshwheels
				char meshw[256];
				char texb[256];
				float radius, rimradius, width, mass, spring, damp;
				char side;
				int rays, node1, node2, snode, braked, propulsed, torquenode;

				int n = parse_args(c, args, 16);

				radius     = PARSEREAL(args[0]);
				rimradius  = PARSEREAL(args[1]);
				width      = PARSEREAL(args[2]);
				rays       = PARSEINT (args[3]);
				node1      = parse_node_number(c, args[4]);
				node2      = parse_node_number(c, args[5]);
				
				std::vector<int> special_numbers;
				special_numbers.push_back(-1);
				special_numbers.push_back(9999);

				snode = parse_node_number(c, args[6], &special_numbers); // special behavior, beware
				
				braked     = PARSEINT (args[7]);
				propulsed  = PARSEINT (args[8]);
				torquenode = parse_node_number(c, args[9]);
				mass       = PARSEREAL(args[10]);
				spring     = PARSEREAL(args[11]);
				damp       = PARSEREAL(args[12]);
				side       = args[13][0];
				strncpy(meshw, args[14].c_str(), 255);
				strncpy(texb, args[15].c_str(), 255);

				addWheel(parent, radius,width,rays,node1,node2,snode,braked,propulsed, torquenode, mass, spring, damp, meshw, texb, true, false, rimradius, side!='r');
				continue;
			}
			else if (c.mode == BTS_MESHWHEELS2)
			{
				//parse meshwheels2
				char meshw[256];
				char texb[256];
				float radius, rimradius, width, mass, spring, damp;
				char side;
				int rays, node1, node2, snode, braked, propulsed, torquenode;

				int n = parse_args(c, args, 16);

				radius     = PARSEREAL(args[0]);
				rimradius  = PARSEREAL(args[1]);
				width      = PARSEREAL(args[2]);
				rays       = PARSEINT (args[3]);
				node1      = parse_node_number(c, args[4]);
				node2      = parse_node_number(c, args[5]);
				
				std::vector<int> special_numbers;
				special_numbers.push_back(-1);
				special_numbers.push_back(9999);

				snode = parse_node_number(c, args[6], &special_numbers); // special behavior, beware

				braked     = PARSEINT (args[7]);
				propulsed  = PARSEINT (args[8]);
				torquenode = parse_node_number(c, args[9]);
				mass       = PARSEREAL(args[10]);
				spring     = PARSEREAL(args[11]);
				damp       = PARSEREAL(args[12]);
				side       = args[13][0];
				strncpy(meshw, args[14].c_str(), 255);
				strncpy(texb, args[15].c_str(), 255);

				addWheel(parent, radius,width,rays,node1,node2,snode,braked,propulsed, torquenode, mass, spring, damp, meshw, texb, true, true, rimradius, side!='r');
				continue;
			}
			else if (c.mode == BTS_FLEXBODYWHEELS)
			{
				//parse flexbodywheels, meshwheels2 with rim and flexbody tire
				char meshw[256]    = "";
				char texb[256]     = "tracks/trans";
				char flexmesh[256] = "";
				float radius = 0.0f, rimradius = 0.0f, width = 0.0f, mass = 0.0f, spring = 0.0f, damp = 0.0f, rimspring = 0.0f, rimdamp = 0.0f;
				char side;
				int rays = 0, node1 = 0, node2 = 0, snode = 0, braked = 0, propulsed = 0, torquenode = 0, node3 = 0;

				int n = parse_args(c, args, 16);

				radius     = PARSEREAL(args[0]);
				rimradius  = PARSEREAL(args[1]);
				width      = PARSEREAL(args[2]);
				rays       = PARSEINT (args[3]);
				node1      = parse_node_number(c, args[4]);
				node2      = parse_node_number(c, args[5]);
				
				std::vector<int> special_numbers;
				special_numbers.push_back(-1);
				special_numbers.push_back(9999);

				snode = parse_node_number(c, args[6], &special_numbers); // special behavior, beware

				braked     = PARSEINT (args[7]);
				propulsed  = PARSEINT (args[8]);
				torquenode = parse_node_number(c, args[9]);
				mass       = PARSEREAL(args[10]);
				spring     = PARSEREAL(args[11]);
				damp       = PARSEREAL(args[12]);
				rimspring  = PARSEREAL(args[13]);
				rimdamp    = PARSEREAL(args[14]);
				side       = args[15][0];
				strncpy(meshw, args[16].c_str(), 255);
				strncpy(flexmesh, args[17].c_str(), 255);

				//store the orientation node for rim and flexbody
				node3 = free_node;

				addWheel3(parent, radius, rimradius, width, rays, node1, node2, snode, braked, propulsed, torquenode, mass, spring, damp, rimspring, rimdamp, meshw, texb, true, true, rimradius, side!='r');


				//add flexbodies

				char uname[256];
				sprintf(uname, "flexbody-%s-%i", truckname, free_flexbody);

				if (free_flexbody >= MAX_FLEXBODIES)
				{
					parser_warning(c, "flexbodies limit reached ("+TOSTRING(MAX_FLEXBODIES)+")", PARSER_ERROR);
					continue;
				}
				
				char tmp_for_str[255];
				sprintf(tmp_for_str, "%d-%d", node3, node3 + (rays * 4) - 1);

				if (!virtuallyLoaded)
					flexbodies[free_flexbody]=new FlexBody(nodes, free_node, flexmesh, uname, node1, node2, node3, Vector3(0.5,0,0), Quaternion::ZERO, tmp_for_str, materialFunctionMapper, usedSkin, (shadowmode!=0), materialReplacer);
				free_flexbody++;
				continue;
			}
			else if (c.mode == BTS_GLOBALS)
			{
				//parse globals
				int n = parse_args(c, args, 2);
				truckmass = PARSEREAL(args[0]);
				loadmass = PARSEREAL(args[1]);
				if (n > 2)
				{
					strncpy(texname, args[2].c_str(), 1024);

					// check for skins
					if (usedSkin && usedSkin->hasReplacementForMaterial(texname))
					{
						// yay, we use a skin :D
						String newMat = usedSkin->getReplacementForMaterial(texname);
						if (!newMat.empty())
							strncpy(texname, newMat.c_str(), 1024);
					}

					if (!virtuallyLoaded)
					{
						//we clone the material
						char clonetex[256];
						sprintf(clonetex, "%s-%s",texname,truckname);
						MaterialPtr mat=(MaterialPtr)(MaterialManager::getSingleton().getByName(texname));
						if (mat.getPointer() == 0)
						{
							parser_warning(c, "Material '" + String(texname) + "' used in Section 'globals' not found! We will try to use the material 'tracks/transred' instead.", PARSER_ERROR);
							mat=(MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/transred"));
							if (mat.getPointer() == 0)
							{

#ifdef USE_MYGUI
								Console *console = Console::getSingletonPtrNoCreation();
								if (console) console->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, "unable to load vehicle (Material not found! Try to ensure that tracks/transred exists and retry): " + filename, "error.png", 30000, true);
#endif // USE_MYGUI

								parser_warning(c, "Material not found! Try to ensure that tracks/transred exists and retry.", PARSER_FATAL_ERROR);
								return -9;
							}
						}
						mat->clone(clonetex);
						strcpy(texname, clonetex);
					}
				}
				continue;
			}
			else if (c.mode == BTS_CAMERAS)
			{
				//parse cameras
				int n = parse_args(c, args, 3);
				
				std::vector<int> special_numbers;
				special_numbers.push_back(-1);

				// node usage before node section ...
				int nodepos = parse_node_number(c, args[0], &special_numbers); // special behavior, beware
				int nodedir = parse_node_number(c, args[1], &special_numbers); // special behavior, beware
				int dir     = parse_node_number(c, args[2], &special_numbers); // special behavior, beware

				addCamera(nodepos, nodedir, dir);
				continue;
			}
			else if (c.mode == BTS_ENGINE)
			{
				//parse engine
				if (driveable == MACHINE)
					continue;

				driveable=TRUCK;
				parse_args(c, args, 7);

				float minrpm = PARSEREAL(args[0]);
				args.erase(args.begin());
				float maxrpm = PARSEREAL(args[0]);
				args.erase(args.begin());
				float torque = PARSEREAL(args[0]);
				args.erase(args.begin());
				float dratio = PARSEREAL(args[0]);
				args.erase(args.begin());

				std::vector<float> gears;

				for (Ogre::StringVector::iterator it = args.begin(); it != args.end(); it++)
				{
					float tmpf = PARSEREAL(*it);
					if ( tmpf <= 0.0f ) break;
					gears.push_back(tmpf);
				}
				if (gears.size() < 3) // 2 extra gears that don't count, one for reverse and one for neutral
				{
					parser_warning(c, "Trucks with less than one forward gear are not supported!", PARSER_ERROR);
					continue;
				}
				//if (audio) audio->setupEngine();

				engine = new BeamEngine(minrpm, maxrpm, torque, gears, dratio, trucknum);

				// are there any startup shifter settings?
				Ogre::String gearboxMode = SSETTING("GearboxMode", "Automatic shift");
				if (gearboxMode == "Manual shift - Auto clutch")
					engine->setAutoMode(BeamEngine::SEMIAUTO);
				else if (gearboxMode == "Fully Manual: sequential shift")
					engine->setAutoMode(BeamEngine::MANUAL);
				else if (gearboxMode == "Fully Manual: stick shift")
					engine->setAutoMode(BeamEngine::MANUAL_STICK);
				else if (gearboxMode == "Fully Manual: stick shift with ranges")
					engine->setAutoMode(BeamEngine::MANUAL_RANGES);

				//engine->start();
				continue;
			}

			else if (c.mode == BTS_TEXCOORDS)
			{
				//parse texcoords
				parse_args(c, args, 3);
				int   id = parse_node_number(c, args[0]);
				float x  = PARSEREAL(args[1]);
				float y  = PARSEREAL(args[2]);

				if (free_texcoord >= MAX_BEAMS)
				{
					parser_warning(c, "texcoords limit reached ("+TOSTRING(MAX_TEXCOORDS)+")", PARSER_ERROR);
					continue;
				}
				texcoords[free_texcoord] = Vector3(id, x, y);
				free_texcoord++;
			}

			else if (c.mode == BTS_CAB)
			{
				//parse cab
				char type='n';
				int n = parse_args(c, args, 3);
				int id1 = parse_node_number(c, args[0]);
				int id2 = parse_node_number(c, args[1]);
				int id3 = parse_node_number(c, args[2]);
				if (n > 3) type = args[3][0];

				if (free_cab >= MAX_CABS)
				{
					parser_warning(c, "cabs limit reached ("+TOSTRING(MAX_CABS)+")", PARSER_ERROR);
					continue;
				}
				cabs[free_cab*3]=id1;
				cabs[free_cab*3+1]=id2;
				cabs[free_cab*3+2]=id3;
				if (free_collcab >= MAX_CABS)
				{
					parser_warning(c, "unable to create cabs: cabs limit reached ("+TOSTRING(MAX_CABS)+")", PARSER_ERROR);
					continue;
				}
				if (type=='c') {collcabs[free_collcab]=free_cab; collcabstype[free_collcab]=0; free_collcab++;};
				if (type=='p') {collcabs[free_collcab]=free_cab; collcabstype[free_collcab]=1; free_collcab++;};
				if (type=='u') {collcabs[free_collcab]=free_cab; collcabstype[free_collcab]=2; free_collcab++;};
				if (type=='b') {buoycabs[free_buoycab]=free_cab; collcabstype[free_collcab]=0; buoycabtypes[free_buoycab]=Buoyance::BUOY_NORMAL; free_buoycab++;   if (!buoyance && !virtuallyLoaded) buoyance=new Buoyance();};
				if (type=='r') {buoycabs[free_buoycab]=free_cab; collcabstype[free_collcab]=0; buoycabtypes[free_buoycab]=Buoyance::BUOY_DRAGONLY; free_buoycab++; if (!buoyance && !virtuallyLoaded) buoyance=new Buoyance();};
				if (type=='s') {buoycabs[free_buoycab]=free_cab; collcabstype[free_collcab]=0; buoycabtypes[free_buoycab]=Buoyance::BUOY_DRAGLESS; free_buoycab++; if (!buoyance && !virtuallyLoaded) buoyance=new Buoyance();};
				if (type=='D' || type == 'F' || type == 'S')
				{

					if (free_collcab >= MAX_CABS || free_buoycab >= MAX_CABS)
					{
						parser_warning(c, "unable to create buoycabs: cabs limit reached ("+TOSTRING(MAX_CABS)+")", PARSER_ERROR);
						continue;
					}
					collcabs[free_collcab]=free_cab;
					collcabstype[free_collcab]=0;
					if (type == 'F') collcabstype[free_collcab]=1;
					if (type == 'S') collcabstype[free_collcab]=2;
					free_collcab++;
					buoycabs[free_buoycab]=free_cab; buoycabtypes[free_buoycab]=Buoyance::BUOY_NORMAL; free_buoycab++; if (!buoyance && !virtuallyLoaded) buoyance=new Buoyance();
				}
				free_cab++;
			}

			else if (c.mode == BTS_COMMANDS || c.mode == BTS_COMMANDS2)
			{
				//parse commands
				int id1, id2,keys,keyl;
				float rateShort, rateLong, shortl, longl;
				char options[250]="";
				memset(options, 0, 249);
				String descr = String();
				hascommands=1;
				Real startDelay=0;
				Real stopDelay=0;
				char startFunction[256]="";
				char stopFunction[256]="";
				float engineCoupling = 1;
				bool needsEngine = true;
				if (c.mode == BTS_COMMANDS)
				{
					char opt='n';
					int n = parse_args(c, args, 7);
					id1        = parse_node_number(c, args[0]);
					id2        = parse_node_number(c, args[1]);
					rateShort  = PARSEREAL(args[2]);
					shortl     = PARSEREAL(args[3]);
					longl      = PARSEREAL(args[4]);
					keys       = PARSEINT (args[5]);
					keyl       = PARSEINT (args[6]);
					if (n > 7)  opt = args[7][0];
					if (n > 8)  descr = args[8];
					if (n > 9)  startDelay = PARSEREAL(args[9]);
					if (n > 10) stopDelay  = PARSEREAL(args[10]);
					if (n > 11) strncpy(startFunction, args[11].c_str(), 255);
					if (n > 12) strncpy(stopFunction,  args[12].c_str(), 255);
					if (n > 13) engineCoupling = PARSEREAL(args[13]);
					if (n > 14) needsEngine = StringConverter::parseBool(args[14]);

					options[0] = opt;
					options[1] = 0;
					rateLong = rateShort;
				}
				else if (c.mode == BTS_COMMANDS2)
				{
					int n = parse_args(c, args, 8);
					id1        = parse_node_number(c, args[0]);
					id2        = parse_node_number(c, args[1]);
					rateShort  = PARSEREAL(args[2]);
					rateLong   = PARSEREAL(args[3]);
					shortl     = PARSEREAL(args[4]);
					longl      = PARSEREAL(args[5]);
					keys       = PARSEINT (args[6]);
					keyl       = PARSEINT (args[7]);
					if (n > 8)  strncpy(options, args[8].c_str(), 250);
					if (n > 9)  descr = args[9];
					if (n > 10) startDelay = PARSEREAL(args[10]);
					if (n > 11) stopDelay  = PARSEREAL(args[11]);
					if (n > 12) strncpy(startFunction, args[12].c_str(), 255);
					if (n > 13) strncpy(stopFunction,  args[13].c_str(), 255);
					if (n > 14) engineCoupling = PARSEREAL(args[14]);
					if (n > 15) needsEngine = StringConverter::parseBool(args[15]);
				}

				//verify array limits so we don't overflow
				if (keys > MAX_COMMANDS || keyl > MAX_COMMANDS)
				{
					parser_warning(c, "Command key invalid", PARSER_ERROR);
					continue;
				}

				int htype=BEAM_HYDRO;

				char *options_pointer = options;
				while (*options_pointer != 0)
				{
					if (*options_pointer=='i')
					{
						htype=BEAM_INVISIBLE_HYDRO;
						break;
					}
					options_pointer++;
				}

				if (free_beam >= MAX_BEAMS)
				{
					parser_warning(c, "cannot create command: beams limit reached ("+TOSTRING(MAX_BEAMS)+")", PARSER_ERROR);
					continue;
				}

				int pos=add_beam(parent, &nodes[id1], &nodes[id2], htype, default_break * default_break_scale, default_spring * default_spring_scale, default_damp * default_damp_scale, detacher_group_state, -1, -1, -1, 1, default_beam_diameter);
				// now 'parse' the options
				options_pointer = options;
				while (*options_pointer != 0)
				{
					switch (*options_pointer)
					{
						case 'r':
						{
							beams[pos].bounded=ROPE;
							break;
						}
						case 'c':
						{
							if (beams[pos].isOnePressMode>0)
							{
								parser_warning(c, "Command cannot be one-pressed and self centering at the same time!", PARSER_ERROR);
								break;
							}
							beams[pos].isCentering=true;
							break;
						}
						case 'p':
						{
							if (beams[pos].isCentering)
							{
								parser_warning(c, "Command cannot be one-pressed and self centering at the same time!", PARSER_ERROR);
								break;
							}
							if (beams[pos].isOnePressMode>0)
							{
								parser_warning(c, "Command already has a one-pressed c.mode! All after the first are ignored!", PARSER_ERROR);
								break;
							}
							beams[pos].isOnePressMode=1;
							break;
						}
						case 'o':
						{
							if (beams[pos].isCentering)
							{
								parser_warning(c, "Command cannot be one-pressed and self centering at the same time!", PARSER_ERROR);
								break;
							}
							if (beams[pos].isOnePressMode>0)
							{
								parser_warning(c, "Command already has a one-pressed c.mode! All after the first are ignored!", PARSER_ERROR);
								break;
							}
							beams[pos].isOnePressMode=2;
							break;
						}
						case 'f':
						{
							beams[pos].isForceRestricted=true;
							break;
						}
					}
					options_pointer++;
				}

				beams[pos].Lhydro=beams[pos].L;
				//add short key
				commandkey[keys].beams.push_back(-pos);
				if (descr != "")
					commandkey[keys].description = descr;
					

				//add long key
				commandkey[keyl].beams.push_back(pos);
				if (descr != "")
					commandkey[keyl].description = descr;

				beams[pos].commandRatioShort=rateShort;
				beams[pos].commandRatioLong=rateLong;
				beams[pos].commandShort=shortl;
				beams[pos].commandLong=longl;
				beams[pos].commandEngineCoupling=engineCoupling;
				beams[pos].commandNeedsEngine=needsEngine;

				// set the middle of the command, so its not required to recalculate this everytime ...
				if (beams[pos].commandLong > beams[pos].commandShort)
					beams[pos].centerLength = (beams[pos].commandLong-beams[pos].commandShort)/2 + beams[pos].commandShort;
				else
					beams[pos].centerLength = (beams[pos].commandShort-beams[pos].commandLong)/2 + beams[pos].commandLong;

				// replace placeholders
				if (String(startFunction) == "/" || String(startFunction) == "-") startFunction[0]=0;
				if (String(stopFunction) == "/" || String(stopFunction) == "-") stopFunction[0]=0;

				if (cmdInertia && startDelay > 0 && stopDelay > 0)
				{
					cmdInertia->setCmdKeyDelay(keys,startDelay,stopDelay,String (startFunction),String (stopFunction));
					cmdInertia->setCmdKeyDelay(keyl,startDelay,stopDelay,String (startFunction),String (stopFunction));
				}
				else if (cmdInertia && (inertia_startDelay > 0 || inertia_stopDelay > 0))
				{
					cmdInertia->setCmdKeyDelay(keys,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));
					cmdInertia->setCmdKeyDelay(keyl,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));
				}

				free_commands++;
			}
			else if (c.mode == BTS_CONTACTERS)
			{
				//parse contacters
				int n = parse_args(c, args, 1);
				int id1 = parse_node_number(c, args[0]);

				if (free_contacter >= MAX_CONTACTERS)
				{
					parser_warning(c, "contacters limit reached ("+TOSTRING(MAX_CONTACTERS)+")", PARSER_ERROR);
					continue;
				}

				contacters[free_contacter].nodeid      = id1;
				contacters[free_contacter].contacted   = 0;
				contacters[free_contacter].opticontact = 0;
				nodes[id1].iIsSkin = true;
				free_contacter++;;
			}
			else if (c.mode == BTS_ROPES)
			{
				//parse ropes
				int group=0; //TODO: to be used
				char option = 'n';
				int n = parse_args(c, args, 2);
				int id1 = parse_node_number(c, args[0]);
				int id2 = parse_node_number(c, args[1]);
				if (n > 2) option = args[2][0];

				if (free_beam >= MAX_BEAMS)
				{
					parser_warning(c, "cannot create rope: beams limit reached ("+TOSTRING(MAX_BEAMS)+")", PARSER_ERROR);
					continue;
				}
				int htype = BEAM_NORMAL;
				if (option=='i') htype = BEAM_INVISIBLE;
				int pos=add_beam(parent, &nodes[id1], &nodes[id2],  htype, default_break * default_break_scale, default_spring * default_spring_scale, default_damp * default_damp_scale, detacher_group_state);
				beams[pos].bounded=ROPE;

				//register rope
				rope_t r;
				r.beam     = &beams[pos];
				r.lockedto = &nodes[0];
				r.group    = group;
				ropes.push_back(r);

				nodes[id1].iIsSkin = true;
				nodes[id2].iIsSkin = true;
			}
			else if (c.mode == BTS_ROPABLES)
			{
				//parse ropables
				int group = -1, multilock = 0;
				int n = parse_args(c, args, 1);
				int id1 = parse_node_number(c, args[0]);
				if (n > 1) group     = PARSEINT(args[1]);
				if (n > 2) multilock = PARSEINT(args[2]);

				ropable_t r;
				r.node      = &nodes[id1];
				r.group     = group;
				r.used      = 0;
				r.multilock = (bool)(multilock == 1);
				ropables.push_back(r);

				nodes[id1].iIsSkin = true;
			}
			else if (c.mode == BTS_TIES)
			{
				//parse ties
				int id1=0, group=-1;
				float maxl, rate, shortl, longl, maxstress=100000.0f;
				char option='n';
				hascommands=1;
				int n = parse_args(c, args, 5);
				id1     = parse_node_number(c, args[0]);
				maxl    = PARSEREAL(args[1]);
				rate    = PARSEREAL(args[2]);
				shortl  = PARSEREAL(args[3]);
				longl   = PARSEREAL(args[4]);
				if (n > 5) option = args[5][0];
				if (n > 6) maxstress = PARSEREAL(args[6]);
				if (n > 7) group = PARSEINT(args[7]);

				if (free_beam >= MAX_BEAMS)
				{
					parser_warning(c, "cannot create tie: beams limit reached ("+TOSTRING(MAX_BEAMS)+")", PARSER_ERROR);
					continue;
				}

				node_t* id_tie = &nodes[0];
				if (id1 == 0)
				{
					 id_tie = &nodes[1];
				}

				int htype=BEAM_HYDRO;
				if (option=='i') htype=BEAM_INVISIBLE_HYDRO;
				int pos=add_beam(parent, &nodes[id1], id_tie, htype, default_break * default_break_scale, default_spring * default_spring_scale, default_damp * default_damp_scale);
				beams[pos].L=maxl;
				beams[pos].refL=maxl;
				beams[pos].Lhydro=maxl;
				beams[pos].bounded=ROPE;
				beams[pos].disabled=true;
				if (!virtuallyLoaded && beams[pos].mSceneNode)
					beams[pos].mSceneNode->detachAllObjects();
				beams[pos].commandRatioShort=rate;
				beams[pos].commandRatioLong=rate;
				beams[pos].commandShort=shortl;
				beams[pos].commandLong=longl;
				beams[pos].maxtiestress=maxstress;

				//register tie
				tie_t t;
				t.group = group;
				t.tying=false;
				t.tied=false;
				t.beam=&beams[pos];
				ties.push_back(t);
			}

			else if (c.mode == BTS_HELP)
			{
				//help material
				parse_args(c, args, 1);
				strncpy(helpmat, args[0].c_str(), 255);
				hashelp = 1;
			}
			else if (c.mode == BTS_CINECAM)
			{
				//cinecam
				float x,y,z;
				int n1, n2, n3, n4, n5, n6, n7, n8;
				float spring=8000.0;
				float damp=800.0;
				int n = parse_args(c, args, 11);
				x      = PARSEREAL(args[0]);
				y      = PARSEREAL(args[1]);
				z      = PARSEREAL(args[2]);
				n1     = parse_node_number(c, args[3]);
				n2     = parse_node_number(c, args[4]);
				n3     = parse_node_number(c, args[5]);
				n4     = parse_node_number(c, args[6]);
				n5     = parse_node_number(c, args[7]);
				n6     = parse_node_number(c, args[8]);
				n7     = parse_node_number(c, args[9]);
				n8     = parse_node_number(c, args[10]);
				if (n > 11) spring = PARSEREAL(args[11]);
				if (n > 12) damp   = PARSEREAL(args[12]);

				if (free_beam >= MAX_BEAMS)
				{
					parser_warning(c, "cannot create cinecam: beams limit reached ("+TOSTRING(MAX_BEAMS)+")", PARSER_ERROR);
					continue;
				}

				if (free_node >= MAX_NODES)
				{
					parser_warning(c, "cannot create cinecam: nodes limit reached ("+TOSTRING(MAX_NODES)+")", PARSER_ERROR);
					continue;
				}

				//add node
				cinecameranodepos[freecinecamera]=free_node;
				Vector3 npos=pos+rot*Vector3(x,y,z);
				init_node(cinecameranodepos[freecinecamera], npos.x, npos.y, npos.z);
	//				init_node(cinecameranodepos[freecinecamera], px+x*cos(ry)+z*sin(ry), py+y , pz+x*cos(ry+3.14159/2.0)+z*sin(ry+3.14159/2.0));
				nodes[free_node].contactless = 1;
				free_node++;

				//add beams
				add_beam(parent, &nodes[cinecameranodepos[freecinecamera]], &nodes[n1], BEAM_INVISIBLE, default_break, spring, damp);
				add_beam(parent, &nodes[cinecameranodepos[freecinecamera]], &nodes[n2], BEAM_INVISIBLE, default_break, spring, damp);
				add_beam(parent, &nodes[cinecameranodepos[freecinecamera]], &nodes[n3], BEAM_INVISIBLE, default_break, spring, damp);
				add_beam(parent, &nodes[cinecameranodepos[freecinecamera]], &nodes[n4], BEAM_INVISIBLE, default_break, spring, damp);
				add_beam(parent, &nodes[cinecameranodepos[freecinecamera]], &nodes[n5], BEAM_INVISIBLE, default_break, spring, damp);
				add_beam(parent, &nodes[cinecameranodepos[freecinecamera]], &nodes[n6], BEAM_INVISIBLE, default_break, spring, damp);
				add_beam(parent, &nodes[cinecameranodepos[freecinecamera]], &nodes[n7], BEAM_INVISIBLE, default_break, spring, damp);
				add_beam(parent, &nodes[cinecameranodepos[freecinecamera]], &nodes[n8], BEAM_INVISIBLE, default_break, spring, damp);

				if (flaresMode>=2 && !cablight)
				{
					// create cabin light :)
					char flarename[256];
					sprintf(flarename, "cabinglight-%s", truckname);
					if (!virtuallyLoaded)
					{
						cablight = gEnv->sceneManager->createLight(flarename);
						cablight->setType(Light::LT_POINT);
						cablight->setDiffuseColour( ColourValue(0.4, 0.4, 0.3));
						cablight->setSpecularColour( ColourValue(0.4, 0.4, 0.3));
						cablight->setAttenuation(20, 1, 0, 0);
						cablight->setCastShadows(false);
						cablight->setVisible(true);
						cablightNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
						deletion_sceneNodes.emplace_back(cablightNode);
						cablightNode->attachObject(cablight);
						cablightNode->setVisible(false);
					}
				}

				freecinecamera++;
			}
			else if (c.mode == BTS_FLARES || c.mode == BTS_FLARES2)
			{
				if (flaresMode==0)
					continue;
				//parse flares
				int ref=-1, nx=0, ny=0, controlnumber=-1, blinkdelay=-2;
				float ox=0, oy=0, oz=1, size=-2;
				char type='f';
				char matname[256]="";
				if (c.mode == BTS_FLARES)
				{
					// original flares
					int n = parse_args(c, args, 5);
					ref = parse_node_number(c, args[0]);
					nx  = parse_node_number(c, args[1]);
					ny  = parse_node_number(c, args[2]);
					ox  = PARSEREAL(args[3]);
					oy  = PARSEREAL(args[4]);
					if (n > 5) type          = args[5][0];
					if (n > 6) controlnumber = PARSEINT (args[6]);
					if (n > 7) blinkdelay    = PARSEINT (args[7]);
					if (n > 8) size          = PARSEREAL (args[8]);
					if (n > 9) strncpy(matname, args[9].c_str(), 255);
				} else if (c.mode == BTS_FLARES2)
				{
					// flares 2
					int n = parse_args(c, args, 6);
					ref = parse_node_number(c, args[0]);
					nx  = parse_node_number(c, args[1]);
					ny  = parse_node_number(c, args[2]);
					ox  = PARSEREAL(args[3]);
					oy  = PARSEREAL(args[4]);
					oz  = PARSEREAL(args[5]);
					if (n > 6) type          = args[6][0];
					if (n > 7) controlnumber = PARSEINT (args[7]);
					if (n > 8) blinkdelay    = PARSEINT (args[8]);
					if (n > 9) size          = PARSEREAL (args[9]);
					if (n > 10) strncpy(matname, args[10].c_str(), 255);
				}

				// check validity
				// b = brake light
				// f = front light
				// l = left blink light
				// r = right blink light
				// R = reverse light
				// u = user controlled light (i.e. fog light) (use controlnumber later)
				if (type != 'f' && type != 'b' && type != 'l' && type != 'r' && type != 'R' && type != 'u')
					type = 'f';

				// backwards compatibility
				if (blinkdelay == -2 && (type == 'l' || type == 'r'))
					// default blink
					blinkdelay = -1;
				else if (blinkdelay == -2 && !(type == 'l' || type == 'r'))
					//default no blink
					blinkdelay = 0;
				if (size == -2 && type == 'f')
					size = 1;
				else if ((size == -2 && type != 'f') || size == -1)
					size = 0.5;

				if (controlnumber < -1 || controlnumber > 500)
				{
					parser_warning(c, "Controlnumber must be between -1 and 500!", PARSER_ERROR);
					continue;
				}
				if (blinkdelay < -1 || blinkdelay > 60000)
				{
					parser_warning(c, "Blinkdelay must be between 0 and 60000!", PARSER_ERROR);
					continue;
				}
				flare_t f;
				f.type = type;
				f.controlnumber = controlnumber;
				f.controltoggle_status = false;
				if (blinkdelay == -1)
					f.blinkdelay = 0.5f;
				else
					f.blinkdelay = (float)blinkdelay / 1000.0f;
				f.blinkdelay_curr=0.0f;
				f.blinkdelay_state=false;
				f.noderef=ref;
				f.nodex=nx;
				f.nodey=ny;
				f.offsetx=ox;
				f.offsety=oy;
				f.offsetz=oz;
				f.size=size;
				if (!virtuallyLoaded)
				{
					f.snode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
					char flarename[256];
					sprintf(flarename, "flare-%s-%i", truckname, free_flare);
					f.bbs=gEnv->sceneManager->createBillboardSet(flarename,1);
					f.bbs->createBillboard(0,0,0);
					f.bbs->setVisibilityFlags(DEPTHMAP_DISABLED);
					bool usingDefaultMaterial=true;
					if (f.bbs && (!strncmp(matname, "default", 250) || strnlen(matname, 250) == 0))
					{
						if (type == 'b')
							f.bbs->setMaterialName("tracks/brakeflare");
						else if (type == 'l' || type == 'r')
							f.bbs->setMaterialName("tracks/blinkflare");
						else // (type == 'f' || type == 'R')
							f.bbs->setMaterialName("tracks/flare");
					} else
					{
						usingDefaultMaterial=false;
						if (f.bbs)
							f.bbs->setMaterialName(String(matname));
					}
					if (f.bbs)
						f.snode->attachObject(f.bbs);
					f.isVisible=true;
					f.light=NULL;
					if (type == 'f' && usingDefaultMaterial && flaresMode >=2 && size > 0.001)
					{
						// front light
						f.light=gEnv->sceneManager->createLight(flarename);
						f.light->setType(Light::LT_SPOTLIGHT);
						f.light->setDiffuseColour( ColourValue(1, 1, 1));
						f.light->setSpecularColour( ColourValue(1, 1, 1));
						f.light->setAttenuation(400, 0.9, 0, 0);
						f.light->setSpotlightRange( Degree(35), Degree(45) );
						f.light->setCastShadows(false);
					}
					else if (type == 'f' && !usingDefaultMaterial && flaresMode >=4 && size > 0.001)
					{
						// this is a quick fix for the red backlight when frontlight is switched on
						f.light=gEnv->sceneManager->createLight(flarename);
						f.light->setType(Light::LT_SPOTLIGHT);
						f.light->setDiffuseColour( ColourValue(1.0, 0, 0));
						f.light->setSpecularColour( ColourValue(1.0, 0, 0));
						f.light->setAttenuation(10.0, 1.0, 0, 0);
						f.light->setSpotlightRange( Degree(35), Degree(45) );
						f.light->setCastShadows(false);
					}
					else if (type == 'R' && flaresMode >= 4 && size > 0.001)
					{
						// brake light
						f.light=gEnv->sceneManager->createLight(flarename);
						f.light->setType(Light::LT_SPOTLIGHT);
						f.light->setDiffuseColour(ColourValue(1, 1, 1));
						f.light->setSpecularColour(ColourValue(1, 1, 1));
						f.light->setAttenuation(20.0, 1, 0, 0);
						f.light->setSpotlightRange( Degree(35), Degree(45) );
						f.light->setCastShadows(false);
					}
					else if (type == 'b' && flaresMode >= 4 && size > 0.001)
					{
						// brake light
						f.light=gEnv->sceneManager->createLight(flarename);
						f.light->setType(Light::LT_SPOTLIGHT);
						f.light->setDiffuseColour( ColourValue(1.0, 0, 0));
						f.light->setSpecularColour( ColourValue(1.0, 0, 0));
						f.light->setAttenuation(10.0, 1.0, 0, 0);
						f.light->setSpotlightRange( Degree(35), Degree(45) );
						f.light->setCastShadows(false);
					}
					else if ((type == 'l' || type == 'r') && flaresMode >= 4 && size > 0.001)
					{
						// blink light
						f.light=gEnv->sceneManager->createLight(flarename);
						f.light->setType(Light::LT_SPOTLIGHT);
						f.light->setDiffuseColour( ColourValue(1, 1, 0));
						f.light->setSpecularColour( ColourValue(1, 1, 0));
						f.light->setAttenuation(10.0, 1, 1, 0);
						f.light->setSpotlightRange( Degree(35), Degree(45) );
						f.light->setCastShadows(false);
					}
					else if ((type == 'u') && flaresMode >= 4 && size > 0.001)
					{
						// user light always white (TODO: improve this)
						f.light=gEnv->sceneManager->createLight(flarename);
						f.light->setType(Light::LT_SPOTLIGHT);
						f.light->setDiffuseColour( ColourValue(1, 1, 1));
						f.light->setSpecularColour( ColourValue(1, 1, 1));
						f.light->setAttenuation(50.0, 1.0, 1, 0.2);
						f.light->setSpotlightRange( Degree(35), Degree(45) );
						f.light->setCastShadows(false);
					}
				}

				// update custom light array
				if (type == 'u' && netCustomLightArray_counter < 4)
				{
					netCustomLightArray[netCustomLightArray_counter] = free_flare;
					netCustomLightArray_counter++;
				}
				flares.push_back(f);
				free_flare++;
			}
			else if (c.mode == BTS_PROPS)
			{
				//parse props
				int ref, nx, ny;
				float ox, oy, oz;
				float rx, ry, rz;
				char meshname[256];

				int n = parse_args(c, args, 10);
				ref = parse_node_number(c, args[0]);
				nx  = parse_node_number(c, args[1]);
				ny  = parse_node_number(c, args[2]);
				ox  = PARSEREAL(args[3]);
				oy  = PARSEREAL(args[4]);
				oz  = PARSEREAL(args[5]);
				rx  = PARSEREAL(args[6]);
				ry  = PARSEREAL(args[7]);
				rz  = PARSEREAL(args[8]);
				strncpy(meshname, args[9].c_str(), 255);

				if (free_prop >= MAX_PROPS)
				{
					parser_warning(c, "props limit reached ("+TOSTRING(MAX_PROPS)+")", PARSER_ERROR);
					continue;
				}
				/* Initialize prop memory to avoid invalid pointers. */
				memset(&props[free_prop], 0, sizeof props[free_prop]);
				props[free_prop].noderef=ref;
				props[free_prop].nodex=nx;
				props[free_prop].nodey=ny;
				props[free_prop].offsetx=ox;
				props[free_prop].offsety=oy;
				props[free_prop].offsetz=oz;
				props[free_prop].orgoffsetX=ox;
				props[free_prop].orgoffsetY=oy;
				props[free_prop].orgoffsetZ=oz;
				props[free_prop].rotaX=rx;
				props[free_prop].rotaY=ry;
				props[free_prop].rotaZ=rz;
				props[free_prop].rot=Quaternion(Degree(rz), Vector3::UNIT_Z);
				props[free_prop].rot=props[free_prop].rot*Quaternion(Degree(ry), Vector3::UNIT_Y);
				props[free_prop].rot=props[free_prop].rot*Quaternion(Degree(rx), Vector3::UNIT_X);
				props[free_prop].wheel=0;
				props[free_prop].mirror=0;
				props[free_prop].pale=0;
				props[free_prop].spinner=0;
				props[free_prop].cameramode=-2;
				props[free_prop].wheelrotdegree=160.0;
				//set no animation by default
				props[free_prop].animFlags[0]=0;
				props[free_prop].animMode[0]=0;
				props[free_prop].animKey[0]=-1;
				props[free_prop].animKeyState[0]=-1.0f;
				String meshnameString = String(meshname);
				std::string::size_type loc = meshnameString.find("leftmirror", 0);
				if ( loc != std::string::npos ) props[free_prop].mirror=1;

				loc = meshnameString.find("rightmirror", 0);
				if ( loc != std::string::npos ) props[free_prop].mirror=-1;

				loc = meshnameString.find("dashboard", 0);
				if ( loc != std::string::npos )
				{
					char dirwheelmeshname[256];
					float dwx=0, dwy=0, dwz=0;
					Real rotdegrees=160;
					Vector3 stdpos = Vector3(-0.67, -0.61,0.24);
					if (!strncmp("dashboard-rh", meshname, 12))
						stdpos=Vector3(0.67, -0.61,0.24);
					String diwmeshname = "dirwheel.mesh";
					if (n > 10) strncpy(dirwheelmeshname, args[10].c_str(), 255);
					if (n > 11) dwx = PARSEREAL(args[11]);
					if (n > 12) dwy = PARSEREAL(args[12]);
					if (n > 13) dwz = PARSEREAL(args[13]);
					if (n > 14) rotdegrees = PARSEREAL(args[14]);
					if (n >= 14)
					{
						stdpos = Vector3(dwx, dwy, dwz);
						diwmeshname = String(dirwheelmeshname);
					}
					if (n >= 15) props[free_prop].wheelrotdegree = rotdegrees;
					props[free_prop].wheelpos=stdpos;

					// create the meshs scenenode
					if (!virtuallyLoaded)
					{
						props[free_prop].wheel = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
						// now create the mesh
						MeshObject *mo = new MeshObject(diwmeshname, "", props[free_prop].wheel, usedSkin, enable_background_loading);
						mo->setSimpleMaterialColour(ColourValue(0, 0.5, 0.5));
						mo->setMaterialFunctionMapper(materialFunctionMapper, materialReplacer);
					}
				}

				if (!virtuallyLoaded)
				{
					// create the meshs scenenode
					props[free_prop].snode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
					// now create the mesh
					props[free_prop].mo = new MeshObject(meshname, "", props[free_prop].snode, usedSkin, enable_background_loading);
					props[free_prop].mo->setSimpleMaterialColour(ColourValue(1, 1, 0));
					props[free_prop].mo->setMaterialFunctionMapper(materialFunctionMapper, materialReplacer);
					props[free_prop].mo->setCastShadows(shadowmode!=0);

					//hack for the spinprops
					if (!strncmp("spinprop", meshname, 8))
					{
						props[free_prop].spinner=1;
						props[free_prop].mo->setCastShadows(false);
						props[free_prop].snode->setVisible(false);
					}
					if (!strncmp("pale", meshname, 4))
					{
						props[free_prop].pale=1;
					}
					//detect driver seat, used to position the driver and make the seat translucent at times
					if (!strncmp("seat", meshname, 4) && !driversseatfound)
					{
						driversseatfound=true;
						props[free_prop].mo->setMaterialName("driversseat");
						driverSeat = &props[free_prop];
					}
					else if (!strncmp("seat2", meshname, 5) && !driversseatfound)
					{
						driversseatfound=true;
						driverSeat = &props[free_prop];
					}
					props[free_prop].beacontype='n';
					if (!strncmp("beacon", meshname, 6) && flaresMode>0)
					{
						ColourValue color = ColourValue(1.0, 0.5, 0.0);
						String matname = "tracks/beaconflare";
						char beaconmaterial[256];
						float br=0, bg=0, bb=0;

						if (n > 10) strncpy(beaconmaterial, args[10].c_str(), 255);
						if (n > 11) br = PARSEREAL(args[11]);
						if (n > 12) bg = PARSEREAL(args[12]);
						if (n > 13) bb = PARSEREAL(args[13]);
						if (n >= 14)
						{
							color = ColourValue(br, bg, bb);
							matname = String(beaconmaterial);
						}

						props[free_prop].bpos[0]=2.0*3.14*((Real)rand()/(Real)RAND_MAX);
						props[free_prop].brate[0]=4.0*3.14+((Real)rand()/(Real)RAND_MAX)-0.5;
						props[free_prop].beacontype='b';
						props[free_prop].bbs[0]=0;
						//the light
						props[free_prop].light[0]=gEnv->sceneManager->createLight(); //propname);
						props[free_prop].light[0]->setType(Light::LT_SPOTLIGHT);
						props[free_prop].light[0]->setDiffuseColour(color);
						props[free_prop].light[0]->setSpecularColour(color);
						props[free_prop].light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
						props[free_prop].light[0]->setSpotlightRange( Degree(35), Degree(45) );
						props[free_prop].light[0]->setCastShadows(false);
						props[free_prop].light[0]->setVisible(false);
						//the flare billboard
						props[free_prop].bbsnode[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
						props[free_prop].bbs[0]=gEnv->sceneManager->createBillboardSet(1); //(propname,1);
						props[free_prop].bbs[0]->createBillboard(0,0,0);
						if (props[free_prop].bbs[0])
						{
							props[free_prop].bbs[0]->setMaterialName(matname);
							props[free_prop].bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
						}
						//if (props[free_prop].bbs[0])
						//	props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
						props[free_prop].bbs[0]->setVisible(false);
						props[free_prop].bbsnode[0]->setVisible(false);
					}
					if (!strncmp("redbeacon", meshname, 9) && flaresMode>0)
					{
						props[free_prop].bpos[0]=0.0;
						props[free_prop].brate[0]=1.0;
						props[free_prop].beacontype='r';
						props[free_prop].bbs[0]=0;
						//the light
						props[free_prop].light[0]=gEnv->sceneManager->createLight();//propname);
						props[free_prop].light[0]->setType(Light::LT_POINT);
						props[free_prop].light[0]->setDiffuseColour( ColourValue(1.0, 0.0, 0.0));
						props[free_prop].light[0]->setSpecularColour( ColourValue(1.0, 0.0, 0.0));
						props[free_prop].light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
						props[free_prop].light[0]->setCastShadows(false);
						props[free_prop].light[0]->setVisible(false);
						//the flare billboard
						props[free_prop].bbsnode[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
						props[free_prop].bbs[0]=gEnv->sceneManager->createBillboardSet(1); //propname,1);
						props[free_prop].bbs[0]->createBillboard(0,0,0);
						if (props[free_prop].bbs[0])
						{
							props[free_prop].bbs[0]->setMaterialName("tracks/redbeaconflare");
							props[free_prop].bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
						}
						if (props[free_prop].bbs[0])
							props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
						props[free_prop].bbsnode[0]->setVisible(false);
						props[free_prop].bbs[0]->setDefaultDimensions(1.0, 1.0);
					}
					if (!strncmp("lightbar", meshname, 6) && flaresMode>0)
					{
						int k;
						ispolice=true;
						props[free_prop].beacontype='p';
						for (k=0; k<4; k++)
						{
							props[free_prop].bpos[k]=2.0*3.14*((Real)rand()/(Real)RAND_MAX);
							props[free_prop].brate[k]=4.0*3.14+((Real)rand()/(Real)RAND_MAX)-0.5;
							props[free_prop].bbs[k]=0;
							//the light
							//char rpname[256];
							//sprintf(rpname,"%s-%i", propname, k);
							props[free_prop].light[k]=gEnv->sceneManager->createLight(); //rpname);
							props[free_prop].light[k]->setType(Light::LT_SPOTLIGHT);
							if (k>1)
							{
								props[free_prop].light[k]->setDiffuseColour( ColourValue(1.0, 0.0, 0.0));
								props[free_prop].light[k]->setSpecularColour( ColourValue(1.0, 0.0, 0.0));
							}
							else
							{
								props[free_prop].light[k]->setDiffuseColour( ColourValue(0.0, 0.5, 1.0));
								props[free_prop].light[k]->setSpecularColour( ColourValue(0.0, 0.5, 1.0));
							}
							props[free_prop].light[k]->setAttenuation(50.0, 1.0, 0.3, 0.0);
							props[free_prop].light[k]->setSpotlightRange( Degree(35), Degree(45) );
							props[free_prop].light[k]->setCastShadows(false);
							props[free_prop].light[k]->setVisible(false);
							//the flare billboard
							props[free_prop].bbsnode[k] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
							props[free_prop].bbs[k]=gEnv->sceneManager->createBillboardSet(1); //rpname,1);
							props[free_prop].bbs[k]->createBillboard(0,0,0);
							if (props[free_prop].bbs[k])
							{
								if (k>1)
									props[free_prop].bbs[k]->setMaterialName("tracks/brightredflare");
								else
									props[free_prop].bbs[k]->setMaterialName("tracks/brightblueflare");
								if (props[free_prop].bbs[k])
								{
									props[free_prop].bbs[k]->setVisibilityFlags(DEPTHMAP_DISABLED);
									props[free_prop].bbsnode[k]->attachObject(props[free_prop].bbs[k]);
								}
							}
							props[free_prop].bbsnode[k]->setVisible(false);
						}
					}
				}
				//set no animation by default
				props[free_prop].animFlags[0]=0;
				props[free_prop].animMode[0]=0;

				free_prop++;
			}
			else if (c.mode == BTS_GLOBEAMS)
			{
				//parse globeams
				int n = parse_args(c, args, 1);
				default_deform = PARSEREAL(args[0]);
				if (n > 1) default_deform        = PARSEREAL(args[1]);
				if (n > 2) default_break         = PARSEREAL(args[2]);
				if (n > 3) default_beam_diameter = PARSEREAL(args[3]);
				if (n > 4) strncpy(default_beam_material, args[4].c_str(), 255);

				// hacky hack there ...
				fadeDist = 1000;
			}
			else if (c.mode == BTS_WINGS)
			{
				//parse wings
				int nds[8];
				float txes[8];
				char type;
				float cratio, mind, maxd, liftcoef = 1.0f;
				char afname[256];
				int n = parse_args(c, args, 16);
				for (int i = 0;i < 8; i++)
					nds[i]  = parse_node_number(c, args[i]);
				for (int i = 0;i < 8; i++)
					txes[i] = PARSEREAL(args[i + 8]);
				if (n > 16) type     = args[16][0];
				if (n > 17) cratio   = PARSEREAL(args[17]);
				if (n > 18) mind     = PARSEREAL(args[18]);
				if (n > 19) maxd     = PARSEREAL(args[19]);
				if (n > 20) strncpy(afname, args[20].c_str(), 255);
				if (n > 21) liftcoef = PARSEREAL(args[21]);

				if (free_wing >= MAX_WINGS)
				{
					parser_warning(c, "wings limit reached ("+TOSTRING(MAX_WINGS)+")", PARSER_ERROR);
					continue;
				}

				if (!virtuallyLoaded)
				{
					char wname[256];
					sprintf(wname, "wing-%s-%i",truckname, free_wing);
					char wnamei[256];
					sprintf(wnamei, "wingobj-%s-%i",truckname, free_wing);
					if (liftcoef != 1.0f) parser_warning(c, "Wing liftforce coefficent: " + TOSTRING(liftcoef), PARSER_INFO);
					wings[free_wing].fa=new FlexAirfoil(wname, nodes, nds[0], nds[1], nds[2], nds[3], nds[4], nds[5], nds[6], nds[7], texname, Vector2(txes[0], txes[1]), Vector2(txes[2], txes[3]), Vector2(txes[4], txes[5]), Vector2(txes[6], txes[7]), type, cratio, mind, maxd, afname, liftcoef, aeroengines, state!=NETWORKED);
					Entity *ec=0;
					try
					{
						ec = gEnv->sceneManager->createEntity(wnamei, wname);
					} catch(...)
					{
						parser_warning(c, "error loading mesh: "+String(wname), PARSER_ERROR);
						continue;
					}
					deletion_Entities.emplace_back(ec);
					MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, ColourValue(0.5, 1, 0));
					if (materialFunctionMapper) materialFunctionMapper->replaceMeshMaterials(ec);
					if (materialReplacer) materialReplacer->replaceMeshMaterials(ec);
					if (usedSkin) usedSkin->replaceMeshMaterials(ec);
					wings[free_wing].cnode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
					if (ec)
						wings[free_wing].cnode->attachObject(ec);
					//induced drag
					if (wingstart==-1) {wingstart=free_wing;wingarea=warea(nodes[wings[free_wing].fa->nfld].AbsPosition, nodes[wings[free_wing].fa->nfrd].AbsPosition, nodes[wings[free_wing].fa->nbld].AbsPosition, nodes[wings[free_wing].fa->nbrd].AbsPosition);}
					else
					{
						// disable position lights on trucks
						if (driveable==TRUCK) hasposlights=true;

						if (nds[1]!=wings[free_wing-1].fa->nfld)
						{
							//discontinuity
							//inform wing segments
							float span=(nodes[wings[wingstart].fa->nfrd].RelPosition-nodes[wings[free_wing-1].fa->nfld].RelPosition).length();
							//					float chord=(nodes[wings[wingstart].fa->nfrd].Position-nodes[wings[wingstart].fa->nbrd].Position).length();
							parser_warning(c, "Full Wing "+TOSTRING(wingstart)+"-"+TOSTRING(free_wing-1)+" SPAN="+TOSTRING(span)+" AREA="+TOSTRING(wingarea), PARSER_INFO);
							wings[wingstart].fa->enableInducedDrag(span,wingarea, false);
							wings[free_wing-1].fa->enableInducedDrag(span,wingarea, true);
							//we want also to add positional lights for first wing
							if (!hasposlights && flaresMode>0)
							{

								if (free_prop+4 >= MAX_PROPS)
								{
									parser_warning(c, "cannot create wing props: props limit reached ("+TOSTRING(MAX_PROPS)+")", PARSER_ERROR);
									continue;
								}
								//Left green
								leftlight=wings[free_wing-1].fa->nfld;
								props[free_prop].noderef=wings[free_wing-1].fa->nfld;
								props[free_prop].nodex=wings[free_wing-1].fa->nflu;
								props[free_prop].nodey=wings[free_wing-1].fa->nfld; //ignored
								props[free_prop].offsetx=0.5;
								props[free_prop].offsety=0.0;
								props[free_prop].offsetz=0.0;
								props[free_prop].rot=Quaternion::IDENTITY;
								props[free_prop].wheel=0;
								props[free_prop].wheelrotdegree=0.0;
								props[free_prop].mirror=0;
								props[free_prop].pale=0;
								props[free_prop].spinner=0;
								props[free_prop].snode=NULL; //no visible prop
								props[free_prop].bpos[0]=0.0;
								props[free_prop].brate[0]=1.0;
								props[free_prop].beacontype='L';
								//no light
								props[free_prop].light[0]=0;
								//the flare billboard
								char propname[256];
								sprintf(propname, "prop-%s-%i", truckname, free_prop);
								props[free_prop].bbsnode[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
								props[free_prop].bbs[0]=gEnv->sceneManager->createBillboardSet(propname,1);
								props[free_prop].bbs[0]->createBillboard(0,0,0);
								if (props[free_prop].bbs[0])
								{
									props[free_prop].bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
									props[free_prop].bbs[0]->setMaterialName("tracks/greenflare");
									props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
								}
								props[free_prop].bbsnode[0]->setVisible(false);
								props[free_prop].bbs[0]->setDefaultDimensions(0.5, 0.5);
								props[free_prop].animFlags[0]=0;
								props[free_prop].animMode[0]=0;
								free_prop++;
								//Left flash
								props[free_prop].noderef=wings[free_wing-1].fa->nbld;
								props[free_prop].nodex=wings[free_wing-1].fa->nblu;
								props[free_prop].nodey=wings[free_wing-1].fa->nbld; //ignored
								props[free_prop].offsetx=0.5;
								props[free_prop].offsety=0.0;
								props[free_prop].offsetz=0.0;
								props[free_prop].rot=Quaternion::IDENTITY;
								props[free_prop].wheel=0;
								props[free_prop].wheelrotdegree=0.0;
								props[free_prop].mirror=0;
								props[free_prop].pale=0;
								props[free_prop].spinner=0;
								props[free_prop].snode=NULL; //no visible prop
								props[free_prop].bpos[0]=0.5; //alt
								props[free_prop].brate[0]=1.0;
								props[free_prop].beacontype='w';
								//light
								sprintf(propname, "prop-%s-%i", truckname, free_prop);
								props[free_prop].light[0]=gEnv->sceneManager->createLight(propname);
								props[free_prop].light[0]->setType(Light::LT_POINT);
								props[free_prop].light[0]->setDiffuseColour( ColourValue(1.0, 1.0, 1.0));
								props[free_prop].light[0]->setSpecularColour( ColourValue(1.0, 1.0, 1.0));
								props[free_prop].light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
								props[free_prop].light[0]->setCastShadows(false);
								props[free_prop].light[0]->setVisible(false);
								//the flare billboard
								props[free_prop].bbsnode[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
								props[free_prop].bbs[0]=gEnv->sceneManager->createBillboardSet(propname,1);
								props[free_prop].bbs[0]->createBillboard(0,0,0);
								if (props[free_prop].bbs[0])
								{
									props[free_prop].bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
									props[free_prop].bbs[0]->setMaterialName("tracks/flare");
									props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
								}
								props[free_prop].bbsnode[0]->setVisible(false);
								props[free_prop].bbs[0]->setDefaultDimensions(1.0, 1.0);
								free_prop++;
								//Right red
								rightlight=wings[wingstart].fa->nfrd;
								props[free_prop].noderef=wings[wingstart].fa->nfrd;
								props[free_prop].nodex=wings[wingstart].fa->nfru;
								props[free_prop].nodey=wings[wingstart].fa->nfrd; //ignored
								props[free_prop].offsetx=0.5;
								props[free_prop].offsety=0.0;
								props[free_prop].offsetz=0.0;
								props[free_prop].rot=Quaternion::IDENTITY;
								props[free_prop].wheel=0;
								props[free_prop].wheelrotdegree=0.0;
								props[free_prop].mirror=0;
								props[free_prop].pale=0;
								props[free_prop].spinner=0;
								props[free_prop].snode=NULL; //no visible prop
								props[free_prop].bpos[0]=0.0;
								props[free_prop].brate[0]=1.0;
								props[free_prop].beacontype='R';
								//no light
								props[free_prop].light[0]=0;
								//the flare billboard
								sprintf(propname, "prop-%s-%i", truckname, free_prop);
								props[free_prop].bbsnode[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
								props[free_prop].bbs[0]=gEnv->sceneManager->createBillboardSet(propname,1);
								props[free_prop].bbs[0]->createBillboard(0,0,0);
								if (props[free_prop].bbs[0])
								{
									props[free_prop].bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
									props[free_prop].bbs[0]->setMaterialName("tracks/redflare");
									props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
								}
								props[free_prop].bbsnode[0]->setVisible(false);
								props[free_prop].bbs[0]->setDefaultDimensions(0.5, 0.5);
								props[free_prop].animFlags[0]=0;
								props[free_prop].animMode[0]=0;
								free_prop++;
								//Right flash
								props[free_prop].noderef=wings[wingstart].fa->nbrd;
								props[free_prop].nodex=wings[wingstart].fa->nbru;
								props[free_prop].nodey=wings[wingstart].fa->nbrd; //ignored
								props[free_prop].offsetx=0.5;
								props[free_prop].offsety=0.0;
								props[free_prop].offsetz=0.0;
								props[free_prop].rot=Quaternion::IDENTITY;
								props[free_prop].wheel=0;
								props[free_prop].wheelrotdegree=0.0;
								props[free_prop].mirror=0;
								props[free_prop].pale=0;
								props[free_prop].spinner=0;
								props[free_prop].snode=NULL; //no visible prop
								props[free_prop].bpos[0]=0.5; //alt
								props[free_prop].brate[0]=1.0;
								props[free_prop].beacontype='w';
								//light
								sprintf(propname, "prop-%s-%i", truckname, free_prop);
								props[free_prop].light[0]=gEnv->sceneManager->createLight(propname);
								props[free_prop].light[0]->setType(Light::LT_POINT);
								props[free_prop].light[0]->setDiffuseColour( ColourValue(1.0, 1.0, 1.0));
								props[free_prop].light[0]->setSpecularColour( ColourValue(1.0, 1.0, 1.0));
								props[free_prop].light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
								props[free_prop].light[0]->setCastShadows(false);
								props[free_prop].light[0]->setVisible(false);
								//the flare billboard
								props[free_prop].bbsnode[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
								props[free_prop].bbs[0]=gEnv->sceneManager->createBillboardSet(propname,1);
								props[free_prop].bbs[0]->createBillboard(0,0,0);
								if (props[free_prop].bbs[0])
								{
									props[free_prop].bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
									props[free_prop].bbs[0]->setMaterialName("tracks/flare");
									props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
								}
								props[free_prop].bbsnode[0]->setVisible(false);
								props[free_prop].bbs[0]->setDefaultDimensions(1.0, 1.0);
								props[free_prop].animFlags[0]=0;
								props[free_prop].animMode[0]=0;
								free_prop++;
								hasposlights=true;
							}
							wingstart=free_wing;
							wingarea=warea(nodes[wings[free_wing].fa->nfld].AbsPosition, nodes[wings[free_wing].fa->nfrd].AbsPosition, nodes[wings[free_wing].fa->nbld].AbsPosition, nodes[wings[free_wing].fa->nbrd].AbsPosition);
						}
						else wingarea+=warea(nodes[wings[free_wing].fa->nfld].AbsPosition, nodes[wings[free_wing].fa->nfrd].AbsPosition, nodes[wings[free_wing].fa->nbld].AbsPosition, nodes[wings[free_wing].fa->nbrd].AbsPosition);
					}
				}

				free_wing++;
			}
			else if (c.mode == BTS_TURBOPROPS || c.mode == BTS_TURBOPROPS2 || c.mode == BTS_PISTONPROPS) //turboprops, turboprops2, pistonprops
			{
				//parse turboprops
				int ref,back,p1,p2,p3,p4;
				int couplenode=-1;
				float pitch=-10;
				bool isturboprops=true;
				float power;
				char propfoil[256];
				
				std::vector<int> special_node_numbers;
				special_node_numbers.push_back(-1);
				
				if (c.mode == BTS_TURBOPROPS)
				{
					int n = parse_args(c, args, 8);
					ref    = parse_node_number(c, args[0]);
					back   = parse_node_number(c, args[1]);
					p1     = parse_node_number(c, args[2]);
					p2     = parse_node_number(c, args[3]);
					p3     = parse_node_number(c, args[4], &special_node_numbers);
					p4     = parse_node_number(c, args[5], &special_node_numbers);
					power  = PARSEREAL(args[6]);
					strncpy(propfoil, args[7].c_str(),  255);
				}
				if (c.mode == BTS_TURBOPROPS2)
				{

					int n = parse_args(c, args, 9);
					ref    = parse_node_number(c, args[0]);
					back   = parse_node_number(c, args[1]);
					p1     = parse_node_number(c, args[2]);
					p2     = parse_node_number(c, args[3]);
					p3     = parse_node_number(c, args[4], &special_node_numbers);
					p4     = parse_node_number(c, args[5], &special_node_numbers);
					couplenode = parse_node_number(c, args[6], &special_node_numbers);
					power  = PARSEREAL(args[7]);
					strncpy(propfoil, args[8].c_str(),  255);
				}
				if (c.mode == BTS_PISTONPROPS)
				{
					int n = parse_args(c, args, 10);
					ref    = parse_node_number(c, args[0]);
					back   = parse_node_number(c, args[1]);
					p1     = parse_node_number(c, args[2]);
					p2     = parse_node_number(c, args[3]);
					p3     = parse_node_number(c, args[4], &special_node_numbers);
					p4     = parse_node_number(c, args[5], &special_node_numbers);
					couplenode = parse_node_number(c, args[6], &special_node_numbers);
					power  = PARSEREAL(args[7]);
					pitch  = PARSEREAL(args[8]);
					strncpy(propfoil, args[9].c_str(),  255);
					isturboprops = false;
				}

				if (free_aeroengine >= MAX_AEROENGINES)
				{
					parser_warning(c, "airoengine limit reached ("+TOSTRING(MAX_AEROENGINES)+")", PARSER_ERROR);
					continue;
				}

				if (!virtuallyLoaded)
				{
					char propname[256];
					sprintf(propname, "turboprop-%s-%i", truckname, free_aeroengine);
					Turboprop *tp=new Turboprop(propname, nodes, ref, back, p1, p2, p3, p4, couplenode, power, propfoil, free_aeroengine, trucknum, disable_smoke, !isturboprops, pitch, heathaze);
					aeroengines[free_aeroengine]=tp;
					driveable=AIRPLANE;
					if (!autopilot && state != NETWORKED)
						autopilot=new Autopilot(trucknum);
					//if (audio) audio->setupAeroengines(audiotype);
					//setup visual
					int i;
					float pscale=(nodes[ref].RelPosition-nodes[p1].RelPosition).length()/2.25;
					for (i=0; i<free_prop; i++)
					{
						if (props[i].pale && props[i].noderef==ref)
						{
							//setup size
							props[i].snode->scale(pscale,pscale,pscale);
							tp->addPale(props[i].snode);
						}
						if (props[i].spinner && props[i].noderef==ref)
						{
							props[i].snode->scale(pscale,pscale,pscale);
							tp->addSpinner(props[i].snode);
						}
					}
				}

				free_aeroengine++;
			}
			else if (c.mode == BTS_FUSEDRAG)
			{
				//parse fusedrag
				int front,back;
				float width, factor = 1.0f;
				char fusefoil[256] = "NACA0009.afl";
				int n = parse_args(c, args, 3);

				if (args[2] == "autocalc")
				{
					// fusedrag autocalculation
					front  = parse_node_number(c, args[0]);
					back   = parse_node_number(c, args[1]);
					// calculate fusedrag by truck size
					if (n > 3)
						factor  = PARSEREAL(args[3]);
					width  =  (fuse_z_max - fuse_z_min) * (fuse_y_max - fuse_y_min) * factor;
					if (n > 4)
						strncpy(fusefoil, args[4].c_str(), 255);
					if (!virtuallyLoaded)
						fuseAirfoil = new Airfoil(fusefoil);
					fuseFront   = &nodes[front];
					fuseBack    = &nodes[front];
					fuseWidth   = width;
					parser_warning(c, "Fusedrag autocalculation size: "+TOSTRING(width)+" m^2", PARSER_INFO);
				} else
				{
					// original fusedrag calculation
					front  = parse_node_number(c, args[0]);
					back   = parse_node_number(c, args[1]);
					width  = PARSEREAL(args[2]);
					if (n > 3)
						strncpy(fusefoil, args[3].c_str(), 255);
					
					if (!virtuallyLoaded)
						fuseAirfoil = new Airfoil(fusefoil);
					fuseFront   = &nodes[front];
					fuseBack    = &nodes[front];
					fuseWidth   = width;
				}
			}
			else if (c.mode == BTS_ENGOPTION)
			{
				//parse engoption
				float inertia;
				char type;
				float idlerpm = -1.0f, stallrpm = -1.0f, minidlemixture = -1.0f, maxidlemixture = -1.0f;
				float clutch = -1.0f, shifttime = -1.0f, clutchtime = -1.0f, postshifttime = -1.0f;
				int n = parse_args(c, args, 1);
				inertia = PARSEREAL(args[0]);
				if (n > 1) type = args[1][0];
				if (n > 2) clutch = PARSEREAL(args[2]);
				if (n > 3) shifttime = PARSEREAL(args[3]);
				if (n > 4) clutchtime = PARSEREAL(args[4]);
				if (n > 5) postshifttime = PARSEREAL(args[5]);
				if (n > 6) idlerpm = PARSEREAL(args[6]);
				if (n > 7) stallrpm = PARSEREAL(args[7]);
				if (n > 8) maxidlemixture = PARSEREAL(args[8]);
				if (n > 9) minidlemixture = PARSEREAL(args[9]);

				if (engine) engine->setOptions(inertia, type, clutch, shifttime, clutchtime, postshifttime, idlerpm, stallrpm, maxidlemixture, minidlemixture);
			}
			else if (c.mode == BTS_BRAKES)
			{
				// parse brakes
				int n = parse_args(c, args, 1);
				brakeforce = PARSEREAL(args[0]);
				// Read in footbrake force and handbrake force. If handbrakeforce is not present, set it to the default value 2*footbrake force to preserve older functionality
				hbrakeforce = 2.0f * brakeforce;
				if (n > 1) hbrakeforce = PARSEREAL(args[1]);
			}
			else if (c.mode == BTS_ROTATORS || c.mode == BTS_ROTATORS2)
			{
				//parse rotators
				int axis1, axis2,keys,keyl;
				int p1[4], p2[4];
				float rate;
				hascommands = 1;
				Real startDelay  = 0.0f;
				Real stopDelay   = 0.0f;
				Real force       = ROTATOR_FORCE_DEFAULT;
				Real tolerance   = ROTATOR_TOLERANCE_DEFAULT;
				char startFunction[50] = {};
				char stopFunction[50]  = {};
				char description[50]   = {};
				float engineCoupling = 1;
				bool needsEngine = false;
				int n = parse_args(c, args, 13);
				axis1 = parse_node_number(c, args[0]);
				axis2 = parse_node_number(c, args[1]);
				p1[0] = parse_node_number(c, args[2]);
				p1[1] = parse_node_number(c, args[3]);
				p1[2] = parse_node_number(c, args[4]);
				p1[3] = parse_node_number(c, args[5]);
				p2[0] = parse_node_number(c, args[6]);
				p2[1] = parse_node_number(c, args[7]);
				p2[2] = parse_node_number(c, args[8]);
				p2[3] = parse_node_number(c, args[9]);
				rate = PARSEREAL(args[10]);
				keys = PARSEINT(args[11]);
				keyl = PARSEINT(args[12]);

				if (c.mode == BTS_ROTATORS)
				{
					if (n > 13) startDelay = PARSEREAL(args[13]);
					if (n > 14) stopDelay = PARSEREAL(args[14]);
					if (n > 15) strncpy(startFunction, args[15].c_str(), 50);
					if (n > 16) strncpy(stopFunction, args[16].c_str(), 50);
					if (n > 17) engineCoupling = PARSEREAL(args[17]);
					if (n > 18) needsEngine = StringConverter::parseBool(args[18]);
				} else
				if (c.mode == BTS_ROTATORS2)
				{
					if (n > 13) force = PARSEREAL(args[13]);
					if (n > 14) tolerance = PARSEREAL(args[14]);
					if (n > 15) strncpy(description, args[15].c_str(), 50);
					if (n > 16) startDelay = PARSEREAL(args[16]);
					if (n > 17) stopDelay = PARSEREAL(args[17]);
					if (n > 18) strncpy(startFunction, args[18].c_str(), 50);
					if (n > 19) strncpy(stopFunction, args[19].c_str(), 50);
					if (n > 20) engineCoupling = PARSEREAL(args[20]);
					if (n > 21) needsEngine = StringConverter::parseBool(args[21]);
				}

				if (free_rotator >= MAX_ROTATORS)
				{
					parser_warning(c, "rotators limit reached ("+TOSTRING(MAX_ROTATORS)+")", PARSER_ERROR);
					continue;
				}
				rotators[free_rotator].angle=0;
				rotators[free_rotator].rate=rate;
				rotators[free_rotator].axis1=axis1;
				rotators[free_rotator].axis2=axis2;
				rotators[free_rotator].force=force;
				rotators[free_rotator].tolerance=tolerance;
				rotators[free_rotator].rotatorEngineCoupling=engineCoupling;
				rotators[free_rotator].rotatorNeedsEngine=needsEngine;
				for (int i=0; i<4; i++)
				{
					rotators[free_rotator].nodes1[i]=p1[i];
					rotators[free_rotator].nodes2[i]=p2[i];
				}
				//add short key
				commandkey[keys].rotators.push_back(-(free_rotator+1));
				if (String(description)  == "")
					commandkey[keys].description = "Rotate_Left/Right";
				else
					commandkey[keys].description = description;

				//add long key
				commandkey[keyl].rotators.push_back(free_rotator+1);
				commandkey[keyl].description = "";

				if (rotaInertia && startDelay > 0 && stopDelay > 0)
				{
					rotaInertia->setCmdKeyDelay(keys,startDelay,stopDelay,String (startFunction),String (stopFunction));
					rotaInertia->setCmdKeyDelay(keyl,startDelay,stopDelay,String (startFunction),String (stopFunction));
				}
				else if (rotaInertia && (inertia_startDelay > 0 || inertia_stopDelay > 0))
				{
					rotaInertia->setCmdKeyDelay(keys,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));
					rotaInertia->setCmdKeyDelay(keyl,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));
				}
				free_rotator++;
			}
			else if (c.mode == BTS_SCREWPROPS)
			{
				//parse screwprops
				int ref,back,up;
				float power;

				int n = parse_args(c, args, 4);
				ref   = parse_node_number(c, args[0]);
				back  = parse_node_number(c, args[1]);
				up    = parse_node_number(c, args[2]);
				power = PARSEREAL(args[3]);

				//if (audio) audio->setupBoat(truckmass);

				if (free_screwprop >= MAX_SCREWPROPS)
				{
					parser_warning(c, "screwprops limit reached ("+TOSTRING(MAX_SCREWPROPS)+")", PARSER_ERROR);
					continue;
				}
				if (!virtuallyLoaded)
					screwprops[free_screwprop]=new Screwprop(nodes, ref, back, up, power, trucknum);
				driveable=BOAT;
				free_screwprop++;
			}
			else if (c.mode == BTS_GUISETTINGS)
			{
				// guisettings
				char keyword[256];
				char value[256];
				int n = parse_args(c, args, 2);
				strncpy(keyword, args[0].c_str(), 255);
				strncpy(value,   args[1].c_str(), 255);

				if (!strncmp(keyword, "debugBeams", 255) && strnlen(value, 255) > 0)
				{
					// obsolete
				}
				if (!strncmp(keyword, "tachoMaterial", 255) && strnlen(value, 255) > 0)
				{
					tachomat = String(value);
				}
				else if (!strncmp(keyword, "speedoMaterial", 255) && strnlen(value, 255) > 0)
				{
					speedomat = String(value);
				}
				else if (!strncmp(keyword, "helpMaterial", 255) && strnlen(value, 255) > 0)
				{
					strncpy(helpmat, value, 254);
				}
				else if (!strncmp(keyword, "speedoMax", 255) && strnlen(value, 255) > 0)
				{
					float tmp = StringConverter::parseReal(String(value));
					if (tmp > 10 && tmp < 32000)
						speedoMax = tmp;
				}
				else if (!strncmp(keyword, "useMaxRPM", 255) && strnlen(value, 255) > 0)
				{
					int use = StringConverter::parseInt(String(value));
					useMaxRPMforGUI = (use == 1);
				}
				else if (!strncmp(keyword, "dashboard", 255) && strnlen(value, 255) > 0)
				{
					dashBoardLayouts.push_back(std::pair<Ogre::String, bool>(String(value), false));
				}
				else if (!strncmp(keyword, "texturedashboard", 255) && strnlen(value, 255) > 0)
				{
					dashBoardLayouts.push_back(std::pair<Ogre::String, bool>(String(value), true));
				}

			}
			else if (c.mode == BTS_MINIMASS)
			{
				//parse minimass
				//sets the minimum node mass
				//usefull for very light vehicles with lots of nodes (e.g. small airplanes)
				parse_args(c, args, 1);
				minimass = PARSEREAL(args[0]);
			}
			else if (c.mode == BTS_EXHAUSTS)
			{
				// parse exhausts
				if (disable_smoke)
					continue;
				int id1, id2;
				float factor;
				char material[256] = "";

				int n = parse_args(c, args, 2);
				id1 = parse_node_number(c, args[0]);
				id2 = parse_node_number(c, args[1]);
				if (n > 2) factor = PARSEREAL(args[2]);
				if (n > 3) strncpy(material, args[3].c_str(), 255);

				exhaust_t e;
				e.emitterNode = id1;
				e.directionNode = id2;
				e.isOldFormat = false;
				if (!virtuallyLoaded)
				{
					e.smokeNode = parent->createChildSceneNode();
					char wname[256];
					sprintf(wname, "exhaust-%d-%s", (int)exhausts.size(), truckname);
					if (strnlen(material,50) == 0 || String(material) == "default")
						strcpy(material, "tracks/Smoke");

					if (usedSkin)
					{
						String newMat = usedSkin->getReplacementForMaterial(material);
						if (!newMat.empty())
							strncpy(material, newMat.c_str(), 50);
					}

					e.smoker = gEnv->sceneManager->createParticleSystem(wname, material);
					if (!e.smoker) continue;
					e.smoker->setVisibilityFlags(DEPTHMAP_DISABLED); // disable particles in depthmap
					e.smokeNode->attachObject(e.smoker);
					e.smokeNode->setPosition(nodes[e.emitterNode].AbsPosition);
				}
				nodes[id1].isHot=true;
				nodes[id2].isHot=true;
				nodes[id1].iIsSkin=true;
				nodes[id2].iIsSkin=true;
				exhausts.push_back(e);
			}
			else if (c.mode == BTS_PARTICLES)
			{
				//particles
				if (!cparticle_enabled)
					continue;
				// parse particle
				int id1, id2;
				char psystem[256] = "";

				int n = parse_args(c, args, 3);
				id1 = parse_node_number(c, args[0]);
				id2 = parse_node_number(c, args[1]);
				strncpy(psystem, args[2].c_str(), 255);

				if (free_cparticle >= MAX_CPARTICLES)
				{
					parser_warning(c, "custom particles limit reached ("+TOSTRING(MAX_CPARTICLES)+")", PARSER_ERROR);
					continue;
				}

				cparticles[free_cparticle].emitterNode = id1;
				cparticles[free_cparticle].directionNode = id2;
				if (!virtuallyLoaded)
				{
					cparticles[free_cparticle].snode = parent->createChildSceneNode();
					char wname[256];
					sprintf(wname, "cparticle-%i-%s", free_cparticle, truckname);
					cparticles[free_cparticle].psys = gEnv->sceneManager->createParticleSystem(wname, psystem);
					if (!cparticles[free_cparticle].psys) continue;
					cparticles[free_cparticle].psys->setVisibilityFlags(DEPTHMAP_DISABLED); // disable particles in depthmap
					cparticles[free_cparticle].snode->attachObject(cparticles[free_cparticle].psys);
					cparticles[free_cparticle].snode->setPosition(nodes[cparticles[free_cparticle].emitterNode].AbsPosition);
					//shut down the emitters
					cparticles[free_cparticle].active=false;
					for (int i=0; i<cparticles[free_cparticle].psys->getNumEmitters(); i++) cparticles[free_cparticle].psys->getEmitter(i)->setEnabled(false);
				}
				free_cparticle++;
			}
			else if (c.mode == BTS_TURBOJETS) //turbojets
			{
				//parse turbojets
				int front,back,ref, rev;
				float len, fdiam, bdiam, drthrust, abthrust;
				int n = parse_args(c, args, 9);
				front = parse_node_number(c, args[0]);
				back  = parse_node_number(c, args[1]);
				ref   = parse_node_number(c, args[2]);
				rev   = PARSEINT(args[3]);
				drthrust = PARSEREAL(args[4]);
				abthrust = PARSEREAL(args[5]);
				fdiam = PARSEREAL(args[6]);
				bdiam = PARSEREAL(args[7]);
				len   = PARSEREAL(args[8]);

				if (free_aeroengine >= MAX_AEROENGINES)
				{
					parser_warning(c, "airoengine limit reached ("+TOSTRING(MAX_AEROENGINES)+")", PARSER_ERROR);
					continue;
				}

				if (!virtuallyLoaded)
				{
					char propname[256];
					sprintf(propname, "turbojet-%s-%i", truckname, free_aeroengine);
					Turbojet *tj=new Turbojet(propname, free_aeroengine, trucknum, nodes, front, back, ref, drthrust, rev!=0, abthrust>0, abthrust, fdiam, bdiam, len, disable_smoke, heathaze, materialFunctionMapper, usedSkin, materialReplacer);
					aeroengines[free_aeroengine]=tj;
					driveable=AIRPLANE;
					if (!autopilot && state != NETWORKED)
						autopilot=new Autopilot(trucknum);
					//if (audio) audio->setupAeroengines(TURBOJETS);
				}
				free_aeroengine++;
			}
			else if (c.mode == BTS_RIGIDIFIERS)
			{
				//parse rigidifiers
				int na,nb,nc;
				float k = DEFAULT_RIGIDIFIER_SPRING;
				float d = DEFAULT_RIGIDIFIER_DAMP;
				int n = parse_args(c, args, 3);
				na = parse_node_number(c, args[0]);
				nb = parse_node_number(c, args[1]);
				nc = parse_node_number(c, args[2]);
				if (n > 3) k  = PARSEREAL(args[3]);
				if (n > 4) d  = PARSEREAL(args[4]);

				if (free_rigidifier >= MAX_RIGIDIFIERS)
				{
					parser_warning(c, "rigidifiers limit reached ("+TOSTRING(MAX_RIGIDIFIERS)+")", PARSER_ERROR);
					continue;
				}

				rigidifiers[free_rigidifier].a=&nodes[na];
				rigidifiers[free_rigidifier].b=&nodes[nb];
				rigidifiers[free_rigidifier].c=&nodes[nc];
				rigidifiers[free_rigidifier].k=k;
				rigidifiers[free_rigidifier].d=d;
				rigidifiers[free_rigidifier].alpha=2.0*acos((nodes[na].RelPosition-nodes[nb].RelPosition).getRotationTo(nodes[nc].RelPosition-nodes[nb].RelPosition).w);
				rigidifiers[free_rigidifier].lastalpha=rigidifiers[free_rigidifier].alpha;
				rigidifiers[free_rigidifier].beama=0;
				rigidifiers[free_rigidifier].beamc=0;
				//searching for associated beams
				for (int i=0; i<free_beam; i++)
				{
					if ((beams[i].p1==&nodes[na] && beams[i].p2==&nodes[nb]) || (beams[i].p2==&nodes[na] && beams[i].p1==&nodes[nb])) rigidifiers[free_rigidifier].beama=&beams[i];
					if ((beams[i].p1==&nodes[nc] && beams[i].p2==&nodes[nb]) || (beams[i].p2==&nodes[nc] && beams[i].p1==&nodes[nb])) rigidifiers[free_rigidifier].beamc=&beams[i];
				}
				free_rigidifier++;
			}
			else if (c.mode == BTS_AIRBRAKES)
			{
				//parse airbrakes
				int ref, nx, ny, na;
				float ox, oy, oz;
				float tx1, tx2, tx3, tx4;
				float wd, len, liftcoef = 1.0f;
				float maxang;
				int n = parse_args(c, args, 14);
				ref = parse_node_number(c, args[0]);
				nx = parse_node_number(c, args[1]);
				ny = parse_node_number(c, args[2]);
				na = parse_node_number(c, args[3]);
				ox = PARSEREAL(args[4]);
				oy = PARSEREAL(args[5]);
				oz = PARSEREAL(args[6]);
				wd = PARSEREAL(args[7]);
				len = PARSEREAL(args[8]);
				maxang = PARSEREAL(args[9]);
				tx1 = PARSEREAL(args[10]);
				tx2 = PARSEREAL(args[11]);
				tx3 = PARSEREAL(args[12]);
				tx4 = PARSEREAL(args[13]);
				if (n > 14) liftcoef = PARSEREAL(args[14]);

				if (free_airbrake >= MAX_AIRBRAKES)
				{
					parser_warning(c, "airbrakes limit reached ("+TOSTRING(MAX_AIRBRAKES)+")", PARSER_ERROR);
					continue;
				}
				if (liftcoef != 1.0f)
					parser_warning(c, "Airbrakes force coefficent: " + TOSTRING(liftcoef), PARSER_ERROR);

				if (!virtuallyLoaded)
					airbrakes[free_airbrake]=new Airbrake(truckname, free_airbrake, &nodes[ref], &nodes[nx], &nodes[ny], &nodes[na], Vector3(ox,oy,oz), wd, len, maxang, texname, tx1,tx2,tx3,tx4,liftcoef);
				free_airbrake++;
			}
			else if (c.mode == BTS_FLEXBODIES)
			{
				//parse flexbodies
				int ref, nx, ny;
				float ox, oy, oz;
				float rx, ry, rz;
				char meshname[256];

				std::vector<int> special_numbers;
				special_numbers.push_back(-1);

				int n = parse_args(c, args, 10);
				ref = parse_node_number(c, args[0], &special_numbers);
				nx = parse_node_number(c, args[1], &special_numbers);
				ny = parse_node_number(c, args[2], &special_numbers);
				ox = PARSEREAL(args[3]);
				oy = PARSEREAL(args[4]);
				oz = PARSEREAL(args[5]);
				rx = PARSEREAL(args[6]);
				ry = PARSEREAL(args[7]);
				rz = PARSEREAL(args[8]);
				strncpy(meshname, args[9].c_str(), 255);

				Vector3 offset=Vector3(ox, oy, oz);
				Quaternion rot=Quaternion(Degree(rz), Vector3::UNIT_Z);
				rot=rot*Quaternion(Degree(ry), Vector3::UNIT_Y);
				rot=rot*Quaternion(Degree(rx), Vector3::UNIT_X);
				char uname[256];
				sprintf(uname, "flexbody-%s-%i", truckname, free_flexbody);
				//read an extra line!
				c.line = ds->getLine();
				StringUtil::trim(c.line);
				c.linecounter++;
				if (c.line == "forset")
				{
					parser_warning(c, "No forset statement after a flexbody", PARSER_ERROR);
					continue;
				}

				if (free_flexbody >= MAX_FLEXBODIES)
				{
					parser_warning(c, "flexbodies limit reached ("+TOSTRING(MAX_FLEXBODIES)+")", PARSER_ERROR);
					continue;
				}
				if (!virtuallyLoaded)
					flexbodies[free_flexbody]=new FlexBody(nodes, free_node, meshname, uname, ref, nx, ny, offset, rot, const_cast<char *>(c.line.substr(6).c_str()), materialFunctionMapper, usedSkin, (shadowmode!=0), materialReplacer);
				free_flexbody++;
			}
			else if (c.mode == BTS_HOOKGROUP)
			{
				//parse hookgroups
				int id1=0, group=-1;
				bool lockNodes = true;
				int n = parse_args(c, args, 2);
				id1       = parse_node_number(c, args[0]);
				group     = PARSEINT(args[1]);
				if (n > 2) lockNodes = (PARSEINT(args[2]) != 0);

				hook_t h;
				h.hookNode  = &nodes[id1];
				h.group     = group;
				h.locked    = UNLOCKED;
				h.lockNode  = 0;
				h.lockTruck = 0;
				h.lockNodes = lockNodes;
				hooks.push_back(h);

			}
			else if (c.mode == BTS_MATERIALFLAREBINDINGS)
			{
				// parse materialflarebindings
				int flareid;
				char material[256]="";
				memset(material, 0, 255);

				int n = parse_args(c, args, 2);
				flareid = PARSEINT(args[0]);
				strncpy(material, args[1].c_str(), 255);

				String materialName = String(material);
				String newMaterialName = materialName + "_mfb_" + String(truckname);
				MaterialPtr mat = MaterialManager::getSingleton().getByName(materialName);
				if (mat.isNull())
				{
					parser_warning(c, "Error in materialbindings: material " + materialName + " was not found", PARSER_ERROR);
					continue;
				}
				//clone the material
				MaterialPtr newmat = mat->clone(newMaterialName);
				//create structes and add
				MaterialFunctionMapper::materialmapping_t t;
				t.originalmaterial = materialName;
				t.material = newMaterialName;
				t.type=0;
				if (materialFunctionMapper)
					materialFunctionMapper->addMaterial(flareid, t);
			}
			else if (c.mode == BTS_SOUNDSOURCES)
			{
				//parse soundsources
				int ref;
				char script[256];
				int n = parse_args(c, args, 2);
				ref = PARSEINT(args[0]); // DO NOT check nodes here, they may come afterwards
				strncpy(script, args[1].c_str(), 255);

	#ifdef USE_OPENAL
				addSoundSource(SoundScriptManager::getSingleton().createInstance(script, trucknum), ref, -2, &c);
	#endif //OPENAL
			}
			else if (c.mode == BTS_SOUNDSOURCES2)
			{
				//parse soundsources2
				int ref, type;
				char script[256];
				int n = parse_args(c, args, 3);
				ref = PARSEINT(args[0]); // DO NOT check nodes here, they may come afterwards
				type = PARSEINT(args[1]);
				strncpy(script, args[2].c_str(), 255);

#ifdef USE_OPENAL
				addSoundSource(SoundScriptManager::getSingleton().createInstance(script, trucknum), ref, type, &c);
#endif //OPENAL
			}
			else if (c.mode == BTS_SOUNDSOURCES3)
			{
				//parse soundsources3
				int ref, mode, itemNum;
				char script[256];
				int n = parse_args(c, args, 5);
				ref = PARSEINT(args[0]); // DO NOT check nodes here, they may come afterwards
				mode = PARSEINT(args[1]);
				int slType = SL_DEFAULT;
				if     (args[2] == "command")     slType = SL_COMMAND;
				else if (args[2] == "hydro")       slType = SL_HYDRO;
				else if (args[2] == "collision")   slType = SL_COLLISION;
				else if (args[2] == "shock")       slType = SL_SHOCKS;
				else if (args[2] == "brake")       slType = SL_BRAKES;
				else if (args[2] == "rope")        slType = SL_ROPES;
				else if (args[2] == "tie")         slType = SL_TIES;
				else if (args[2] == "particle")    slType = SL_PARTICLES;
				else if (args[2] == "axle")        slType = SL_AXLES;
				else if (args[2] == "flare")       slType = SL_FLARES;
				else if (args[2] == "flexbody")    slType = SL_FLEXBODIES;
				else if (args[2] == "exhaust")     slType = SL_EXHAUSTS;
				else if (args[2] == "videocamera") slType = SL_VIDEOCAMERA;

				itemNum = PARSEINT(args[3]);
				strncpy(script, args[4].c_str(), 255);
#ifdef USE_OPENAL
				addSoundSource(SoundScriptManager::getSingleton().createInstance(script, trucknum, NULL, slType, itemNum), ref, mode, &c);
#endif //OPENAL
			}
			else if (c.mode == BTS_ENVMAP)
			{
				// parse envmap
				// we do nothing of this for the moment
			}
			else if (c.mode == BTS_MANAGEDMATERIALS)
			{
				// parse managedmaterials
				char material[256];
				material[0]=0;
				char type[256];
				type[0]=0;
				int n = parse_args(c, args, 2);
				strncpy(material, args[0].c_str(), 255);
				strncpy(type, args[1].c_str(), 255);

				//first, check if work has already been done
				MaterialPtr tstmat=(MaterialPtr)(MaterialManager::getSingleton().getByName(material));
				if (!tstmat.isNull())
				{
					//material already exists, probably because the vehicle was already spawned previously
					parser_warning(c, "Warning: managed material '" + String(material) +"' already exists", PARSER_ERROR);
					continue;
				}

				if (!strcmp(type, "flexmesh_standard"))
				{
					char maintex[256];
					maintex[0]=0;
					char dmgtex[256];
					dmgtex[0]=0;
					char spectex[256];
					spectex[0]=0;
					n = parse_args(c, args, 3);
					strncpy(material, args[0].c_str(), 255);
					strncpy(type, args[1].c_str(), 255);
					strncpy(maintex, args[2].c_str(), 255);
					if (n > 3) strncpy(dmgtex, args[3].c_str(), 255);
					if (n > 4) strncpy(spectex, args[4].c_str(), 255);

					//different cases
					//caution, this is hardwired against the managed.material file
					if (strlen(dmgtex)==0 || dmgtex[0]=='-')
					{
						//no damage texture
						if (strlen(spectex)==0 || spectex[0]=='-')
						{
							//no specular texture
							MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_standard/simple"));
							if (srcmat.isNull())
							{
								parser_warning(c, "Material 'managed/flexmesh_standard/simple' missing!", PARSER_ERROR);
								continue;
							}

							MaterialPtr dstmat=srcmat->clone(material);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
							if (managedmaterials_doublesided)
								dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
							dstmat->compile();
						}
						else
						{
							//specular, but no damage
							MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_standard/specularonly"));
							if (srcmat.isNull())
							{
								parser_warning(c, "Material 'managed/flexmesh_standard/specularonly' missing!", PARSER_ERROR);
								continue;
							}

							MaterialPtr dstmat=srcmat->clone(material);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
							dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
							if (managedmaterials_doublesided)
							{
								dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
								dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
							}
							dstmat->compile();
						}
					}
					else
					{
						//damage texture
						if (strlen(spectex)==0 || spectex[0]=='-')
						{
							//no specular texture
							MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_standard/damageonly"));
							if (srcmat.isNull())
							{
								parser_warning(c, "Material 'managed/flexmesh_standard/damageonly' missing!", PARSER_ERROR);
								continue;
							}

							MaterialPtr dstmat=srcmat->clone(material);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(dmgtex);
							if (managedmaterials_doublesided)
								dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
							dstmat->compile();
						}
						else
						{
							//specular, and damage
							MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_standard/speculardamage"));
							if (srcmat.isNull())
							{
								parser_warning(c, "Material 'managed/flexmesh_standard/speculardamage' missing!", PARSER_ERROR);
								continue;
							}

							MaterialPtr dstmat=srcmat->clone(material);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(2)->setTextureName(dmgtex);
							dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
							if (managedmaterials_doublesided)
							{
								dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
								dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
							}
							dstmat->compile();
						}
					}
				}
				else if (!strcmp(type, "flexmesh_transparent"))
				{
					char maintex[256];
					maintex[0]=0;
					char dmgtex[256];
					dmgtex[0]=0;
					char spectex[256];
					spectex[0]=0;
					n = parse_args(c, args, 3);
					strncpy(material, args[0].c_str(), 255);
					strncpy(type, args[1].c_str(), 255);
					strncpy(maintex, args[2].c_str(), 255);
					if (n > 3) strncpy(dmgtex, args[3].c_str(), 255);
					if (n > 4) strncpy(spectex, args[4].c_str(), 255);

					//different cases
					//caution, this is hardwired against the managed.material file
					if (strlen(dmgtex)==0 || dmgtex[0]=='-')
					{
						//no damage texture
						if (strlen(spectex)==0 || spectex[0]=='-')
						{
							//no specular texture
							MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_transparent/simple"));
							if (srcmat.isNull())
							{
								parser_warning(c, "Material 'managed/flexmesh_transparent/simple' missing!", PARSER_ERROR);
								continue;
							}

							MaterialPtr dstmat=srcmat->clone(material);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
							if (managedmaterials_doublesided)
								dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
							dstmat->compile();
						}
						else
						{
							//specular, but no damage
							MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_transparent/specularonly"));
							if (srcmat.isNull())
							{
								parser_warning(c, "Material 'managed/flexmesh_transparent/specularonly' missing!", PARSER_ERROR);
								continue;
							}

							MaterialPtr dstmat=srcmat->clone(material);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
							dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
							if (managedmaterials_doublesided)
							{
								dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
								dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
							}
							dstmat->compile();
						}
					}
					else
					{
						//damage texture
						if (strlen(spectex)==0 || spectex[0]=='-')
						{
							//no specular texture
							MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_transparent/damageonly"));
							if (srcmat.isNull())
							{
								parser_warning(c, "Material 'managed/flexmesh_transparent/damageonly' missing!", PARSER_ERROR);
								continue;
							}

							MaterialPtr dstmat=srcmat->clone(material);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(dmgtex);
							if (managedmaterials_doublesided)
								dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
							dstmat->compile();
						}
						else
						{
							//specular, and damage
							MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_transparent/speculardamage"));
							if (srcmat.isNull())
							{
								parser_warning(c, "Material 'managed/flexmesh_transparent/speculardamage' missing!", PARSER_ERROR);
								continue;
							}

							MaterialPtr dstmat=srcmat->clone(material);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
							dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(2)->setTextureName(dmgtex);
							dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
							if (managedmaterials_doublesided)
							{
								dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
								dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
							}
							dstmat->compile();
						}
					}
				}
				else if (!strcmp(type, "mesh_standard"))
				{
					char maintex[256];
					maintex[0]=0;
					char spectex[256];
					spectex[0]=0;
					n = parse_args(c, args, 3);
					strncpy(material, args[0].c_str(), 255);
					strncpy(type, args[1].c_str(), 255);
					strncpy(maintex, args[2].c_str(), 255);
					if (n > 3) strncpy(spectex, args[3].c_str(), 255);

					//different cases
					//caution, this is hardwired against the managed.material file
					if (strlen(spectex)==0 || spectex[0]=='-')
					{
						//no specular texture
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/mesh_standard/simple"));
						if (srcmat.isNull())
						{
							parser_warning(c, "Material 'managed/mesh_standard/simple' missing!", PARSER_ERROR);
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						if (managedmaterials_doublesided)
							dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->compile();
					}
					else
					{
						//specular
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/mesh_standard/specular"));
						if (srcmat.isNull())
						{
							parser_warning(c, "Material 'managed/mesh_standard/specular' missing!", PARSER_ERROR);
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
						dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
						if (managedmaterials_doublesided)
						{
							dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
							dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
						}
						dstmat->compile();
					}
				}
				else if (!strcmp(type, "mesh_transparent"))
				{
					char maintex[256];
					maintex[0]=0;
					char spectex[256];
					spectex[0]=0;
					n = parse_args(c, args, 3);
					strncpy(material, args[0].c_str(), 255);
					strncpy(type, args[1].c_str(), 255);
					strncpy(maintex, args[2].c_str(), 255);
					if (n > 3) strncpy(spectex, args[3].c_str(), 255);

					//different cases
					//caution, this is hardwired against the managed.material file
					if (strlen(spectex)==0 || spectex[0]=='-')
					{
						//no specular texture
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/mesh_transparent/simple"));
						if (srcmat.isNull())
						{
							parser_warning(c, "Material 'managed/mesh_transparent/simple' missing!", PARSER_ERROR);
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						if (managedmaterials_doublesided)
							dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->compile();
					}
					else
					{
						//specular
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/mesh_transparent/specular"));
						if (srcmat.isNull())
						{
							parser_warning(c, "Material 'managed/mesh_transparent/specular' missing!", PARSER_ERROR);
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
						dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
						if (managedmaterials_doublesided)
						{
							dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
							dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
						}
						dstmat->compile();
					}
				}
				else
				{
					parser_warning(c, "Unknown effect", PARSER_ERROR);
					continue;
				}

			}
			else if (c.mode == BTS_SECTIONCONFIG)
			{
				// parse sectionconfig
				int n = parse_args(c, args, 2);
				// version unused
				//version = args[1]
				String sectionName = args[2];
				sectionconfigs.push_back(sectionName);
				c.mode=savedmode;
			}
			else if (c.mode == BTS_SECTION)
			{
				// parse section
				int version=0;
				char sectionName[10][256];
				for (int i=0;i<10;i++) memset(sectionName, 0, 255); // clear
				if (c.line.size() < 8) continue;
				int n = parse_args(c, args, 2);
				version = PARSEINT(args[0]);
				for (int i = 0; i < 10; i++)
				{
					if (n > (i + 1))
						strncpy(sectionName[i], args[i + 1].c_str(), 255);
				}
				
				if (truckconfig.empty() && strlen(sectionName[0]) > 0)
				{
					// compatibility mode: if no section was specified, use the first
					truckconfig.push_back(sectionName[0]);
				}

				bool found = false;
				for (int i=0;i<10;i++)
				{
					for (std::vector<String>::iterator it=truckconfig.begin(); it!=truckconfig.end(); it++)
					{
						if (sectionName[i] == *it)
						{
							found = true;
							break;
						}
					}
				}
				
				in_section = true;
				
				if (found)
					continue;
				else
					// wait for end_section otherwise
					c.mode=BTS_IN_SECTION;
			}
			/* c.mode BTS_IN_SECTION is reserved */
			else if (c.mode == BTS_TORQUECURVE)
			{
				// parse torquecurve
				if (engine && engine->getTorqueCurve())
					engine->getTorqueCurve()->processLine(String(c.line));
			} else if (c.mode == BTS_ADVANCEDDRAG)
			{
				//parse advanced drag
				float drag;
				int n = parse_args(c, args, 1);
				drag = PARSEREAL(args[0]);
				advanced_total_drag = drag;
				advanced_drag       = true;
			}
			else if (c.mode == BTS_AXLES)
			{
				// parse axle section
				// search for wheel

				if (virtuallyLoaded)
				{
					free_axle++;
					continue;
				}

				if (!free_wheel)
				{
					parser_warning(c, "AXLE ERROR: the axle section must come AFTER some wheels", PARSER_ERROR);
					continue;
				}

				axles[free_axle] = new Axle();

				int wheel_node[2][2] = {{0, 0}, {0, 0}};
				Ogre::StringVector options = Ogre::StringUtil::split(c.line, ",");
				Ogre::StringVector::iterator cur = options.begin();

				for (; cur != options.end(); ++cur)
				{
					Ogre::StringUtil::trim(*cur);

					parser_warning(c, "AXLE: Parsing property: [" + *cur + "]", PARSER_INFO );

					switch(cur->at(0))
					{
					// wheels
					case 'w':
						// dirty repetitive method, could stand to be cleaned up
						if ( cur->at(1) == '1')
						{
							int results = sscanf(cur->c_str(), "w1(%d %d)", &wheel_node[0][0], &wheel_node[0][1]);
							if (results != 2 ) break;
						}
						else if ( cur->at(1) == '2')
						{
							int results = sscanf(cur->c_str(), "w2(%d %d)", &wheel_node[1][0], &wheel_node[1][1]);
							if (results != 2) break;
						}
						break;
					case 'd':
					{
							char diffs[10] = {0};
							int results = sscanf(cur->c_str(), "d(%9s)", diffs);
							if (results == 0 ) break;
							for (int i = 0; i < 10; ++i)
							{
								switch(diffs[i])
								{
								case 'l': axles[free_axle]->addDiffType(LOCKED_DIFF); break;
								case 'o': axles[free_axle]->addDiffType(OPEN_DIFF); break;
								//case 'v': axles[free_axle]->addDiffType(VISCOUS_DIFF); break;
								case 's': axles[free_axle]->addDiffType(SPLIT_DIFF); break;
								//case 'm': axles[free_axle]->addDiffType(LIMITED_SLIP_DIFF); break;
								}
							}
						break;
					}
					case 's':
						parser_warning(c, "AXLE: selection property not yet available", PARSER_ERROR);
						break;
					case 'r':
						parser_warning(c, "AXLE: Gear ratio property not yet available", PARSER_ERROR);
						break;
					default:
						parser_warning(c, "AXLE: malformed property: " + *cur, PARSER_ERROR);
						break;
					}

				}

				//
				for ( int i = 0; i < free_wheel &&  (axles[free_axle]->wheel_1 < 0 || axles[free_axle]->wheel_2 < 0); ++i)
				{
					if ( ( wheels[i].refnode0->id == wheel_node[0][0] || wheels[i].refnode0->id == wheel_node[0][1]) &&
						( wheels[i].refnode1->id == wheel_node[0][0] || wheels[i].refnode1->id == wheel_node[0][1]))
					{
						axles[free_axle]->wheel_1 = i;
					}
					if ( ( wheels[i].refnode0->id == wheel_node[1][0] || wheels[i].refnode0->id == wheel_node[1][1]) &&
						( wheels[i].refnode1->id == wheel_node[1][0] || wheels[i].refnode1->id == wheel_node[1][1]))
					{
					axles[free_axle]->wheel_2 = i;
					}
				}


				if ( axles[free_axle]->wheel_1 < 0 || axles[free_axle]->wheel_2 < 0 )
				{
					// if one or the other is null
					if ( axles[free_axle]->wheel_1 < 0)
					{
						parser_warning(c, "AXLE: could not find wheel 1 nodes: " +
							TOSTRING(wheel_node[0][0]) + " " +
							TOSTRING(wheel_node[0][1]) , PARSER_ERROR);
					}
					if ( axles[free_axle]->wheel_2 < 0)
					{
						parser_warning(c, "AXLE: could not find wheel 2 nodes: " +
						TOSTRING(wheel_node[1][0]) + " " +
						TOSTRING(wheel_node[1][1]) , PARSER_ERROR);
					}
					continue;
				}

				// test if any differentials have been defined
				if ( axles[free_axle]->availableDiffs().empty() )
				{
					parser_warning(c, "AXLE: nodiffs defined, defaulting to Open and Locked", PARSER_INFO);
					axles[free_axle]->addDiffType(OPEN_DIFF);
					axles[free_axle]->addDiffType(LOCKED_DIFF);
				}

				parser_warning(c, "AXLE: Created: w1(" + TOSTRING(wheel_node[0][0]) + ") " +
					TOSTRING(wheel_node[0][1]) + ", w2(" + TOSTRING(wheel_node[1][0]) + " " +
					TOSTRING(wheel_node[1][1]) + ")", PARSER_INFO);
				++free_axle;
			}

			else if (c.mode == BTS_RAILGROUPS) parseRailGroupLine(c);
			else if (c.mode == BTS_SLIDENODES) parseSlideNodeLine(c);
			else if (c.mode == BTS_COLLISIONBOXES) parseCollisionBoxLine(c);
			else if (c.mode == BTS_NODECOLLISION)
			{
				// parse nodecollision
				int nodenum  = -1;
				float radius =  0;
				int n = parse_args(c, args, 2);
				nodenum = parse_node_number(c, args[0]);
				radius = PARSEREAL(args[1]);

				if (nodenum >= free_node || nodenum < 0)
					continue;

				nodes[nodenum].collRadius = radius;
			}
			else if (c.mode == BTS_VIDCAM)
			{
				if (virtuallyLoaded) continue;

				VideoCamera *v = VideoCamera::parseLine(this, c);
				if (v)
				{
					vidcams.push_back(v);
				}
				continue;
			}

		} catch(ParseException &)
		{
			c.mode = BTS_NONE;
			parser_warning(c, "got parsing exception, falling back to no section", PARSER_ERROR);
			// we use this catcher to continue after an error was thrown, cleans up the code a lot
			continue;
		}
	};
	
	// we should post-process the torque curve if existing
	if (engine)
	{
		int result = engine->getTorqueCurve()->spaceCurveEvenly(engine->getTorqueCurve()->getUsedSpline());
		if (result)
		{
			engine->getTorqueCurve()->setTorqueModel("default");
			if (result == 1)
			{
				parser_warning(c, "TorqueCurve: Points (rpm) must be in an ascending order. Using default curve", PARSER_ERROR);
			}
		}
	}
	
	//calculate gwps height offset
	//get a starting value
	posnode_spawn_height=nodes[0].RelPosition.y;
	//start at 0 to avoid a crash whith a 1-node truck
	for (int i=0; i<free_node; i++)
	{
		// scan and store the y-coord for the lowest node of the truck
		if (nodes[i].RelPosition.y <= posnode_spawn_height)
			posnode_spawn_height = nodes[i].RelPosition.y;
	}

	if (cameranodepos[0] > 0)
	{
		// store the y-difference between the trucks lowest node and the campos-node for the gwps system
		posnode_spawn_height = nodes[cameranodepos[0]].RelPosition.y - posnode_spawn_height;
	} else
	{
		//this can not be an airplane, just set it to 0.
		posnode_spawn_height = 0.0f;
	}
	
	if (!loading_finished) {
		parser_warning(c, "Reached end of file "+ filename+ ". No 'end' was found! Did you forgot it? ", PARSER_ERROR);
	}

	//cameras workaround
	for (int i=0; i<freecamera; i++)
	{
		//LogManager::getSingleton().logMessage("Camera dir="+StringConverter::toString(nodes[cameranodedir[i]].RelPosition-nodes[cameranodepos[i]].RelPosition)+" roll="+StringConverter::toString(nodes[cameranoderoll[i]].RelPosition-nodes[cameranodepos[i]].RelPosition));
		revroll[i]=(nodes[cameranodedir[i]].RelPosition-nodes[cameranodepos[i]].RelPosition).crossProduct(nodes[cameranoderoll[i]].RelPosition-nodes[cameranodepos[i]].RelPosition).y>0;
		if (revroll[i])
		{
			parser_warning(c, "camera definition is probably invalid and has been corrected. It should be center, back, left", PARSER_OBSOLETE);
		}
	}
	
	//wing closure
	if (wingstart!=-1)
	{
		if (autopilot) autopilot->setInertialReferences(&nodes[leftlight], &nodes[rightlight], fuseBack, &nodes[cameranodepos[0]]);
		//inform wing segments
		float span=(nodes[wings[wingstart].fa->nfrd].RelPosition-nodes[wings[free_wing-1].fa->nfld].RelPosition).length();
		//		float chord=(nodes[wings[wingstart].fa->nfrd].Position-nodes[wings[wingstart].fa->nbrd].Position).length();
		parser_warning(c, "Full Wing "+TOSTRING(wingstart)+"-"+TOSTRING(free_wing-1)+" SPAN="+TOSTRING(span)+" AREA="+TOSTRING(wingarea), PARSER_INFO);
		wings[wingstart].fa->enableInducedDrag(span,wingarea, false);
		wings[free_wing-1].fa->enableInducedDrag(span,wingarea, true);
		//wash calculator
		wash_calculator(rot, c);
	}
	//add the cab visual
	parser_warning(c, "creating cab", PARSER_INFO);
	if (free_texcoord>0 && free_cab>0)
	{
		//closure
		subtexcoords[free_sub]=free_texcoord;
		subcabs[free_sub]=free_cab;
		char wname[256];
		sprintf(wname, "cab-%s",truckname);
		char wnamei[256];
		sprintf(wnamei, "cabobj-%s",truckname);
		//the cab materials are as follow:
		//texname: base texture with emissive(2 pass) or without emissive if none available(1 pass), alpha cutting
		//texname-trans: transparency texture (1 pass)
		//texname-back: backface texture: black+alpha cutting (1 pass)
		//texname-noem: base texture without emissive (1 pass), alpha cutting

		//material passes must be:
		//0: normal texture
		//1: transparent (windows)
		//2: emissive
		/*strcpy(texname, "testtex");
		char transmatname[256];
		sprintf(transmatname, "%s-trans", texname);
		char backmatname[256];
		sprintf(backmatname, "%s-back", texname);
		hasEmissivePass=1;*/

		MaterialPtr mat=(MaterialPtr)(MaterialManager::getSingleton().getByName(texname));
		if (mat.isNull())
		{
			
#ifdef USE_MYGUI
			Console *console = Console::getSingletonPtrNoCreation();
			if (console) console->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, "unable to load vehicle (Material '"+String(texname)+"' missing!): " + filename, "error.png", 30000, true);
#endif // USE_MYGUI

			parser_warning(c, "Material '"+String(texname)+"' missing!", PARSER_FATAL_ERROR);
			return -13;
		}

		//-trans
		char transmatname[256];
		sprintf(transmatname, "%s-trans", texname);
		MaterialPtr transmat=mat->clone(transmatname);
		if (mat->getTechnique(0)->getNumPasses()>1) transmat->getTechnique(0)->removePass(1);
		transmat->getTechnique(0)->getPass(0)->setAlphaRejectSettings(CMPF_LESS_EQUAL, 128);
		transmat->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
		if (transmat->getTechnique(0)->getPass(0)->getNumTextureUnitStates()>0)
			transmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(TFO_NONE);
		transmat->compile();

		//-back
		char backmatname[256];
		sprintf(backmatname, "%s-back", texname);
		MaterialPtr backmat=mat->clone(backmatname);
		if (mat->getTechnique(0)->getNumPasses()>1) backmat->getTechnique(0)->removePass(1);
		if (transmat->getTechnique(0)->getPass(0)->getNumTextureUnitStates()>0)
			backmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setColourOperationEx(LBX_SOURCE1, LBS_MANUAL, LBS_MANUAL, ColourValue(0,0,0),ColourValue(0,0,0));
		if (shadowOptimizations)
			backmat->setReceiveShadows(false);
		//just in case
		//backmat->getTechnique(0)->getPass(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);
		//backmat->getTechnique(0)->getPass(0)->setAlphaRejectSettings(CMPF_GREATER, 128);
		backmat->compile();

		//-noem and -noem-trans
		if (mat->getTechnique(0)->getNumPasses()>1)
		{
			hasEmissivePass=1;
			char clomatname[256];
			sprintf(clomatname, "%s-noem", texname);
			MaterialPtr clomat=mat->clone(clomatname);
			clomat->getTechnique(0)->removePass(1);
			clomat->compile();
		}

		//base texture is not modified
		//	mat->compile();


		parser_warning(c, "creating mesh", PARSER_INFO);
		cabMesh = NULL;
		if (!virtuallyLoaded)
			cabMesh=new FlexObj(nodes, free_texcoord, texcoords, free_cab, cabs, free_sub, subtexcoords, subcabs, texname, wname, subisback, backmatname, transmatname);
		parser_warning(c, "creating entity", PARSER_INFO);

		if (!virtuallyLoaded)
		{
			parser_warning(c, "creating cabnode", PARSER_INFO);
			cabNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
			Entity *ec = 0;
			try
			{
				parser_warning(c, "loading cab", PARSER_INFO);
				ec = gEnv->sceneManager->createEntity(wnamei, wname);
				//		ec->setRenderQueueGroup(RENDER_QUEUE_6);
				parser_warning(c, "attaching cab", PARSER_INFO);
				if (ec)
				{
					deletion_Entities.emplace_back(ec);
					cabNode->attachObject(ec);
				}
			} catch(...)
			{
				parser_warning(c, "error loading mesh: "+String(wname));
			}
			MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, ColourValue(0.5, 1, 0.5));
			if (materialFunctionMapper) materialFunctionMapper->replaceMeshMaterials(ec);
			if (materialReplacer) materialReplacer->replaceMeshMaterials(ec);
			if (usedSkin) usedSkin->replaceMeshMaterials(ec);
		}
	};
	parser_warning(c, "cab ok", PARSER_INFO);
	//	mWindow->setDebugText("Beam number:"+ TOSTRING(free_beam));

	if (c.mode == BTS_DESCRIPTION)
		parser_warning(c, "description section not closed with end_description", PARSER_ERROR);

	if (c.mode == BTS_COMMENT)
		parser_warning(c, "comment section not closed with end_comment", PARSER_ERROR);

	if (c.mode == BTS_SECTION)
		parser_warning(c, "section section not closed with end_section", PARSER_ERROR);

	// check for some things
	if (strnlen(guid, 128) == 0)
	{
		parser_warning(c, "vehicle uses no GUID, skinning will be impossible", PARSER_OBSOLETE);
	}

   // now generate the hash of it
	{
		// copy whole truck into a string
		String code;
		ds->seek(0); // from start
		code.resize(ds->size());
		ds->read(&code[0], ds->size());

		// and build the hash over it
		char hash_result[250];
		memset(hash_result, 0, 249);
		RoR::CSHA1 sha1;
		sha1.UpdateHash((uint8_t *)code.c_str(), (uint32_t)code.size());
		sha1.Final();
		sha1.ReportHash(hash_result, RoR::CSHA1::REPORT_HEX_SHORT);
		beamHash = String(hash_result);
	}


	// WARNING: this must come LAST
 	if (!SSETTING("vehicleOutputFile", "").empty())
	{
		// serialize the truck in a special format :)
		String fn = SSETTING("vehicleOutputFile", "");
		serialize(fn, &scope_log);
	}
	



	parser_warning(c, "parsing done", PARSER_INFO);
	return 0;
}

void SerializedRig::serialize(Ogre::String targetFilename, ScopeLog *scope_log)
{
	// get the path info
	Ogre::String out_basename, out_ext, out_path;
	Ogre::StringUtil::splitFullFilename(targetFilename, out_basename, out_ext, out_path);

	// then save
	if (out_ext == "json")
	{
		// now save it
		JSONObject root;

		// Adding values
		root[L"truckname"] = new JSONValue(realtruckname.c_str());

		int infos_count = 0;
		int warnings_count = 0;
		int errors_count = 0;
		int fatals_count = 0;
		std::vector <parsecontext_t> &warnings = getWarnings();
		std::vector <parsecontext_t>::iterator it;
		JSONArray error_lines;
		for (it = warnings.begin(); it != warnings.end(); it++)
		{
			if (it->warningLvl      == PARSER_INFO)
			{
				infos_count++;
				continue;
			}
			else if (it->warningLvl == PARSER_WARNING)
			{
				warnings_count++;
				continue;
			}
			else if (it->warningLvl == PARSER_ERROR)
				errors_count++;
			else if (it->warningLvl == PARSER_FATAL_ERROR)
				fatals_count++;
			error_lines.push_back(new JSONValue(TOSTRING(it->linecounter).c_str()));
		}
		root[L"error_lines"]     = new JSONValue(error_lines);

		root[L"infos"]      = new JSONValue(TOSTRING(infos_count).c_str());
		root[L"warnings"]   = new JSONValue(TOSTRING(warnings_count).c_str());
		root[L"errors"]     = new JSONValue(TOSTRING(errors_count).c_str());
		root[L"fatals"]     = new JSONValue(TOSTRING(fatals_count).c_str());
		root[L"rorversion"] = new JSONValue(ROR_VERSION_STRING);
		root[L"hash"]       = new JSONValue(beamHash.c_str());

		if (!virtuallyLoaded) // && mSceneNode)
		{
	 		// now calculate the bounds with respect of the nodes and beams
			calcBoundingBoxes();
			calcLowestNode();

			root[L"minx"]     = new JSONValue(TOSTRING(boundingBox.getMinimum().x).c_str());
			root[L"miny"]     = new JSONValue(TOSTRING(boundingBox.getMinimum().y).c_str());
			root[L"minz"]     = new JSONValue(TOSTRING(boundingBox.getMinimum().z).c_str());

			root[L"maxx"]     = new JSONValue(TOSTRING(boundingBox.getMaximum().x).c_str());
			root[L"maxy"]     = new JSONValue(TOSTRING(boundingBox.getMaximum().y).c_str());
			root[L"maxz"]     = new JSONValue(TOSTRING(boundingBox.getMaximum().z).c_str());
		}

		if (scope_log)
		{
			root[L"ogre_info"]       = new JSONValue(TOSTRING(scope_log->info).c_str());
			root[L"ogre_warnings"]   = new JSONValue(TOSTRING(scope_log->warning).c_str());
			root[L"ogre_errors"]     = new JSONValue(TOSTRING(scope_log->error).c_str());
			root[L"ogre_obsolete"]   = new JSONValue(TOSTRING(scope_log->obsoleteWarning).c_str());
		}

		// Create nodes array
		JSONArray nodes_array;
		for (int i = 0; i < free_node; i++)
		{
			JSONObject node;
			node[L"id"] = new JSONValue(TOSTRING(i).c_str());
			//node[L"options"] = new JSONValue(TOSTRING(nodes[i].).c_str());
			node[L"coord"] = new JSONValue(TOSTRING(nodes[i].AbsPosition).c_str());
			nodes_array.push_back(new JSONValue(node));
		}
		root[L"nodes"] = new JSONValue(nodes_array);

		// Create beams array
		JSONArray beams_array;
		for (int i = 0; i < free_beam; i++)
		{
			JSONObject beam;
			beam[L"id1"] = new JSONValue(TOSTRING(beams[i].p1->id).c_str());
			beam[L"id2"] = new JSONValue(TOSTRING(beams[i].p2->id).c_str());
			beams_array.push_back(new JSONValue(beam));
		}
		root[L"beams"] = new JSONValue(beams_array);

		// Create final value
		JSONValue *final_value = new JSONValue(root);

		// Print it
		FILE *fo = fopen(targetFilename.c_str(), "w");
#ifdef WIN32
		fwprintf(fo, L"%ls", final_value->Stringify().c_str());
#else
		// TODO: FIX
#endif
		fclose(fo);

		//LOG("vehicle serialized to json successfully: "+targetFilename);
		// Clean up
		delete final_value;
	} else
	{
		LOG("unsupported output file format: " + out_ext);
	}
}

void SerializedRig::init_node(int pos, Real x, Real y, Real z, int type, Real m, int iswheel, Real friction, int id, int wheelid, Real nfriction, Real nvolume, Real nsurface, Real nloadweight)
{
	nodes[pos].AbsPosition=Vector3(x,y,z);
	nodes[pos].RelPosition=Vector3(x,y,z)-origin;
	nodes[pos].smoothpos=nodes[pos].AbsPosition;
	nodes[pos].iPosition=Vector3(x,y,z);
	if (pos != 0)
		nodes[pos].iDistance=(nodes[0].AbsPosition - Vector3(x,y,z)).squaredLength();
	else
		nodes[pos].iDistance=0;
	nodes[pos].Velocity=Vector3::ZERO;
	nodes[pos].Forces=Vector3::ZERO;
	nodes[pos].locked=m<0.0;
	nodes[pos].mass=m;
	nodes[pos].iswheel=iswheel;
	nodes[pos].wheelid=wheelid;
	nodes[pos].friction_coef=nfriction;
	nodes[pos].volume_coef=nvolume;
	nodes[pos].surface_coef=nsurface;
	if (nloadweight >=0.0f)
	{
		nodes[pos].masstype=NODE_LOADED;
		nodes[pos].overrideMass=true;
		nodes[pos].mass=nloadweight;
	}
	nodes[pos].disable_particles=false;
	nodes[pos].disable_sparks=false;
	nodes[pos].masstype=type;
	nodes[pos].contactless=0;
	nodes[pos].contacted=0;
	nodes[pos].lockednode=0;
	nodes[pos].buoyanceForce=Vector3::ZERO;
	nodes[pos].buoyancy=truckmass/15.0;//DEFAULT_BUOYANCY;
	nodes[pos].lastdrag=Vector3(0,0,0);
	float grav = -9.81f;
	if(gEnv->terrainManager) // trucks can also virtually load, no terrain then
		grav = gEnv->terrainManager->getGravity();
	nodes[pos].gravimass=Vector3(0, grav*m,0);
	nodes[pos].wetstate=DRY;
	nodes[pos].isHot=false;
	nodes[pos].overrideMass=false;
	nodes[pos].id=id;
	nodes[pos].collisionBoundingBoxID=-1;
	nodes[pos].collRadius=0;
	nodes[pos].collTestTimer=0;
	nodes[pos].iIsSkin=false;
	nodes[pos].isSkin=nodes[pos].iIsSkin;
	nodes[pos].pos=pos;
	nodes[pos].mSceneNode = NULL;
	nodes[pos].mouseGrabMode=0;
//		nodes[pos].tsmooth=Vector3::ZERO;
	if (type==NODE_LOADED) masscount++;
}

int SerializedRig::add_beam(Ogre::SceneNode* parent, node_t *p1 , node_t *p2 , int type , float strength , float spring , float damp , int detachgroupstate/* =DEFAULT_DETACHER_GROUP  */, float length/* =-1.0  */, float shortbound/* =-1.0  */, float longbound/* =-1.0  */, float precomp/* =1.0  */, float diameter/* =DEFAULT_BEAM_DIAMETER  */, parsecontext_t *c/* =0 */)
{
	int pos=free_beam;

	beams[pos].p1=p1;
	beams[pos].p2=p2;
	beams[pos].p2truck=0;
	beams[pos].type=type;
	beams[pos].detacher_group=detachgroupstate;
	if (length<0.0)
	{
		//calculate the length
		Vector3 t;
		t=p1->RelPosition;
		t=t-p2->RelPosition;
		beams[pos].L=precomp*t.length();
	} else
	{
		beams[pos].L=length;
	}
	beams[pos].k=spring;
	beams[pos].d=damp;
	beams[pos].broken=0;
	beams[pos].Lhydro=beams[pos].L;
	beams[pos].refL=beams[pos].L;
	beams[pos].hydroRatio=0.0;
	beams[pos].hydroFlags=0;
	beams[pos].animFlags=0;
	beams[pos].stress=0.0;
	beams[pos].lastforce=Vector3(0,0,0);
	beams[pos].isCentering=false;
	beams[pos].isOnePressMode=0;
	beams[pos].isForceRestricted=false;
	beams[pos].autoMovingMode=0;
	beams[pos].autoMoveLock=false;
	beams[pos].pressedCenterMode=false;
	beams[pos].disabled=false;
	beams[pos].shock=0;
	if (default_deform<beam_creak) default_deform=beam_creak;
	beams[pos].default_deform=default_deform * default_deform_scale;
	beams[pos].minmaxposnegstress=default_deform * default_deform_scale;
	beams[pos].maxposstress=default_deform * default_deform_scale;
	beams[pos].maxnegstress=-default_deform * default_deform_scale;
	beams[pos].plastic_coef=default_plastic_coef;
	beams[pos].default_plastic_coef=default_plastic_coef;
	beams[pos].strength=strength;
	beams[pos].iStrength=strength;
	beams[pos].diameter=default_beam_diameter;
	beams[pos].minendmass=1.0;
	beams[pos].diameter = diameter;
	beams[pos].scale=0.0;
	beams[pos].mEntity=NULL;
	beams[pos].mSceneNode=NULL;
	if (shortbound!=-1.0)
	{
		beams[pos].bounded    = SHOCK1;
		beams[pos].shortbound = shortbound;
		beams[pos].longbound  = longbound;

	} else
	{
		beams[pos].bounded = NOSHOCK;
	}

	//        if (type!=BEAM_VIRTUAL && type!=BEAM_INVISIBLE)
	if (type!=BEAM_VIRTUAL && !virtuallyLoaded)
	{
		//setup visuals
		//the cube is 100x100x100
		char bname[256];
		sprintf(bname, "beam-%s-%i", truckname, pos);
		try
		{
			beams[pos].mEntity = gEnv->sceneManager->createEntity(bname, "beam.mesh");
		} catch(...)
		{
			parser_warning(c, "error loading mesh: beam.mesh", PARSER_ERROR);
		}
		// no materialmapping for beams!
		//		ec->setCastShadows(false);
		deletion_Entities.emplace_back(beams[pos].mEntity);
		if (beams[pos].mEntity && (type==BEAM_HYDRO || type==BEAM_MARKED))
			beams[pos].mEntity->setMaterialName("tracks/Chrome");
		else if (beams[pos].mEntity)
			beams[pos].mEntity->setMaterialName(default_beam_material);
		beams[pos].mSceneNode = beamsRoot->createChildSceneNode();
		//            beams[pos].mSceneNode->attachObject(ec);
		//            beams[pos].mSceneNode->setScale(default_beam_diameter/100.0,length/100.0,default_beam_diameter/100.0);
		beams[pos].mSceneNode->setScale(beams[pos].diameter, length, beams[pos].diameter);

		// colourize beams in simple c.mode
		ColourValue c = ColourValue::Blue;
		if (type == BEAM_HYDRO)
			c = ColourValue::Red;
		else if (type == BEAM_HYDRO)
			c = ColourValue::Red;
		if (beams[pos].mEntity)
			MaterialFunctionMapper::replaceSimpleMeshMaterials(beams[pos].mEntity, c);

	}

 	if (beams[pos].mSceneNode && beams[pos].mEntity && !(type==BEAM_VIRTUAL || type==BEAM_INVISIBLE || type==BEAM_INVISIBLE_HYDRO))
		beams[pos].mSceneNode->attachObject(beams[pos].mEntity);//beams[pos].mSceneNode->setVisible(0);

	free_beam++;
	return pos;
}

void SerializedRig::addWheel(Ogre::SceneNode* parent, float radius , float width , int rays , int node1 , int node2 , int snode , int braked , int propulsed , int torquenode , float mass , float wspring , float wdamp , char* texf , char* texb , bool meshwheel/* =false  */, bool meshwheel2/* =false  */, float rimradius/* =0.0  */, bool rimreverse/* =false  */, parsecontext_t *c/* =0 */)
{
	if (propulsed)
	{
		propwheelcount++;
	}
	int i;
	int nodebase=free_node;
	int node3;
	int contacter_wheel=1;
	//ignore the width parameter
	width=(nodes[node1].RelPosition-nodes[node2].RelPosition).length();
	//enforce the "second node must have a larger Z coordinate than the first" constraint
	if (nodes[node1].RelPosition.z>nodes[node2].RelPosition.z)
	{
		//swap
		node3=node1;
		node1=node2;
		node2=node3;
	}
	//ignore the sign of snode, just do the thing automatically
	//if (snode<0) node3=-snode; else node3=snode;
	if (snode<0) snode=-snode;
	bool closest1=false;
	if (snode!=9999) closest1=(nodes[snode].RelPosition-nodes[node1].RelPosition).length()<(nodes[snode].RelPosition-nodes[node2].RelPosition).length();

	//unused:
	//Real px=nodes[node1].Position.x;
	//Real py=nodes[node1].Position.y;
	//Real pz=nodes[node1].Position.z;

	Vector3 axis=nodes[node2].RelPosition-nodes[node1].RelPosition;
	axis.normalise();
	Vector3 rayvec = axis.perpendicular() * radius;
	// old rayvec:
	//Vector3 rayvec=Vector3(0, radius, 0);
	Quaternion rayrot=Quaternion(Degree(-360.0/(Real)(rays*2)), axis);
	for (i=0; i<rays; i++)
	{
		//with propnodes and variable friction
//			init_node(nodebase+i*2, px+radius*sin((Real)i*6.283185307179/(Real)rays), py+radius*cos((Real)i*6.283185307179/(Real)rays), pz, NODE_NORMAL, mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width);
		Vector3 raypoint;
		raypoint=nodes[node1].RelPosition+rayvec;
		rayvec=rayrot*rayvec;
		init_node(nodebase+i*2, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width, -1, free_wheel, default_node_friction, default_node_volume, default_node_surface, NODE_LOADWEIGHT_DEFAULT);

		// outer ring has wheelid%2 != 0
		nodes[nodebase+i*2].iswheel = free_wheel*2+1;

		if (contacter_wheel)
		{
			contacters[free_contacter].nodeid=nodebase+i*2;
			contacters[free_contacter].contacted=0;
			contacters[free_contacter].opticontact=0;
			free_contacter++;;
		}
//			init_node(nodebase+i*2+1, px+radius*sin((Real)i*6.283185307179/(Real)rays), py+radius*cos((Real)i*6.283185307179/(Real)rays), pz+width, NODE_NORMAL, mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width);
		raypoint=nodes[node2].RelPosition+rayvec;

		rayvec=rayrot*rayvec;
		init_node(nodebase+i*2+1, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width, -1, free_wheel, default_node_friction, default_node_volume, default_node_surface, NODE_LOADWEIGHT_DEFAULT);

		// inner ring has wheelid%2 == 0
		nodes[nodebase+i*2+1].iswheel = free_wheel*2+2;
		if (contacter_wheel)
		{
			contacters[free_contacter].nodeid=nodebase+i*2+1;
			contacters[free_contacter].contacted=0;
			contacters[free_contacter].opticontact=0;
			free_contacter++;;
		}
		//wheel object
		wheels[free_wheel].nodes[i*2]=&nodes[nodebase+i*2];
		wheels[free_wheel].nodes[i*2+1]=&nodes[nodebase+i*2+1];
	}
	free_node+=2*rays;
	for (i=0; i<rays; i++)
	{
		if (!meshwheel2)
		{
			//normal meshwheels (old)
			//bounded
			add_beam(parent, &nodes[node1], &nodes[nodebase+i*2], BEAM_INVISIBLE, default_break, wspring, wdamp, DEFAULT_DETACHER_GROUP, -1.0, 0.66, 0.0);
			add_beam(parent, &nodes[node2], &nodes[nodebase+i*2+1], BEAM_INVISIBLE, default_break, wspring, wdamp, DEFAULT_DETACHER_GROUP, -1.0, 0.66, 0.0);
			add_beam(parent, &nodes[node2], &nodes[nodebase+i*2], BEAM_INVISIBLE, default_break, wspring, wdamp, DEFAULT_DETACHER_GROUP);
			add_beam(parent, &nodes[node1], &nodes[nodebase+i*2+1], BEAM_INVISIBLE, default_break, wspring, wdamp, DEFAULT_DETACHER_GROUP);
			//reinforcement
			add_beam(parent, &nodes[nodebase+i*2], &nodes[nodebase+i*2+1], BEAM_INVISIBLE, default_break, wspring, wdamp, DEFAULT_DETACHER_GROUP);
			add_beam(parent, &nodes[nodebase+i*2], &nodes[nodebase+((i+1)%rays)*2], BEAM_INVISIBLE, default_break, wspring, wdamp, DEFAULT_DETACHER_GROUP);
			add_beam(parent, &nodes[nodebase+i*2+1], &nodes[nodebase+((i+1)%rays)*2+1], BEAM_INVISIBLE, default_break, wspring, wdamp, DEFAULT_DETACHER_GROUP);
			add_beam(parent, &nodes[nodebase+i*2+1], &nodes[nodebase+((i+1)%rays)*2], BEAM_INVISIBLE, default_break, wspring, wdamp, DEFAULT_DETACHER_GROUP);
		} else
		{
			//this wheels use set_beam_defaults-settings for the tiretread beams (meshwheels2)
			//bounded
			add_beam(parent, &nodes[node1], &nodes[nodebase+i*2], BEAM_INVISIBLE, default_break, wspring, wdamp, DEFAULT_DETACHER_GROUP, -1.0, 0.66, 0.15);
			add_beam(parent, &nodes[node2], &nodes[nodebase+i*2+1], BEAM_INVISIBLE, default_break, wspring, wdamp, DEFAULT_DETACHER_GROUP, -1.0, 0.66, 0.15);
			add_beam(parent, &nodes[node2], &nodes[nodebase+i*2], BEAM_INVISIBLE, default_break, wspring, wdamp, DEFAULT_DETACHER_GROUP);
			add_beam(parent, &nodes[node1], &nodes[nodebase+i*2+1], BEAM_INVISIBLE, default_break, wspring, wdamp, DEFAULT_DETACHER_GROUP);
			//reinforcement (tire tread)
			add_beam(parent, &nodes[nodebase+i*2], &nodes[nodebase+i*2+1], BEAM_INVISIBLE, default_break, default_spring, default_damp, DEFAULT_DETACHER_GROUP);
			add_beam(parent, &nodes[nodebase+i*2], &nodes[nodebase+((i+1)%rays)*2], BEAM_INVISIBLE, default_break, default_spring, default_damp, DEFAULT_DETACHER_GROUP);
			add_beam(parent, &nodes[nodebase+i*2+1], &nodes[nodebase+((i+1)%rays)*2+1], BEAM_INVISIBLE, default_break, default_spring, default_damp, DEFAULT_DETACHER_GROUP);
			add_beam(parent, &nodes[nodebase+i*2+1], &nodes[nodebase+((i+1)%rays)*2], BEAM_INVISIBLE, default_break, default_spring, default_damp, DEFAULT_DETACHER_GROUP);
		}

			//add_beam(parent, &nodes[nodebase+i*2], &nodes[nodebase+((i+1)%rays)*2+1], BEAM_INVISIBLE, default_break, wspring, wdamp);

		if (snode!=9999)
		{
			//back beams //BEAM_VIRTUAL

			if (closest1) {add_beam(parent, &nodes[snode], &nodes[nodebase+i*2], BEAM_VIRTUAL, default_break, wspring, wdamp);}
			else         {add_beam(parent, &nodes[snode], &nodes[nodebase+i*2+1], BEAM_VIRTUAL, default_break, wspring, wdamp);};
			/* THIS ALMOST WORKS BUT IT IS INSTABLE AT SPEED !!!!
			//rigidifier version
			if (free_rigidifier >= MAX_RIGIDIFIERS)
			{
				parser_warning(c, "rigidifiers limit reached ...");
			}

			int na=(closest1)?node2:node1;
			int nb=(closest1)?node1:node2;
			int nc=snode;
			rigidifiers[free_rigidifier].a=&nodes[na];
			rigidifiers[free_rigidifier].b=&nodes[nb];
			rigidifiers[free_rigidifier].c=&nodes[nc];
			rigidifiers[free_rigidifier].k=wspring;
			rigidifiers[free_rigidifier].d=wdamp;
			rigidifiers[free_rigidifier].alpha=2.0*acos((nodes[na].RelPosition-nodes[nb].RelPosition).getRotationTo(nodes[nc].RelPosition-nodes[nb].RelPosition).w);
			rigidifiers[free_rigidifier].lastalpha=rigidifiers[free_rigidifier].alpha;
			rigidifiers[free_rigidifier].beama=0;
			rigidifiers[free_rigidifier].beamc=0;
			//searching for associated beams
			for (int i=0; i<free_beam; i++)
			{
				if ((beams[i].p1==&nodes[na] && beams[i].p2==&nodes[nb]) || (beams[i].p2==&nodes[na] && beams[i].p1==&nodes[nb])) rigidifiers[free_rigidifier].beama=&beams[i];
				if ((beams[i].p1==&nodes[nc] && beams[i].p2==&nodes[nb]) || (beams[i].p2==&nodes[nc] && beams[i].p1==&nodes[nb])) rigidifiers[free_rigidifier].beamc=&beams[i];
			}
			free_rigidifier++;
			*/
		}
	}
	//wheel object
	wheels[free_wheel].braked=braked;
	wheels[free_wheel].propulsed=propulsed;
	wheels[free_wheel].nbnodes=2*rays;
	wheels[free_wheel].refnode0=&nodes[node1];
	wheels[free_wheel].refnode1=&nodes[node2];
	wheels[free_wheel].radius=radius;
	wheels[free_wheel].speed=0.0;
	wheels[free_wheel].rp=0;
	wheels[free_wheel].rp1=0;
	wheels[free_wheel].rp2=0;
	wheels[free_wheel].rp3=0;
	wheels[free_wheel].width=width;
	wheels[free_wheel].arm=&nodes[torquenode];
	wheels[free_wheel].lastContactInner=Vector3::ZERO;
	wheels[free_wheel].lastContactOuter=Vector3::ZERO;
	wheels[free_wheel].firstLock=false;
	if (propulsed>0)
	{
		//for inter-differential locking
		proppairs[proped_wheels]=free_wheel;
		proped_wheels++;
	}
	if (braked) braked_wheels++;
	//find near attach
	Real l1=(nodes[node1].RelPosition-nodes[torquenode].RelPosition).length();
	Real l2=(nodes[node2].RelPosition-nodes[torquenode].RelPosition).length();
	if (l1<l2) wheels[free_wheel].near_attach=&nodes[node1]; else wheels[free_wheel].near_attach=&nodes[node2];
	//visuals
	char wname[256];
	sprintf(wname, "wheel-%s-%i",truckname, free_wheel);
	char wnamei[256];
	sprintf(wnamei, "wheelobj-%s-%i",truckname, free_wheel);
	//	strcpy(texf, "tracks/wheelface,");
	vwheels[free_wheel].meshwheel = meshwheel;
	if (!virtuallyLoaded)
	{
		if (meshwheel)
		{
			vwheels[free_wheel].fm=new FlexMeshWheel(wname, nodes, node1, node2, nodebase, rays, texf, texb, rimradius, rimreverse, materialFunctionMapper, usedSkin, materialReplacer);
			try
			{
				Entity *ec = gEnv->sceneManager->createEntity(wnamei, wname);
				vwheels[free_wheel].cnode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
				if (ec)
				{
					deletion_Entities.emplace_back(ec);
					vwheels[free_wheel].cnode->attachObject(ec);
				}
				MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, ColourValue(0, 0.5, 0.5));
				if (materialFunctionMapper) materialFunctionMapper->replaceMeshMaterials(ec);
				if (materialReplacer) materialReplacer->replaceMeshMaterials(ec);
				if (usedSkin) usedSkin->replaceMeshMaterials(ec);
			} catch(...)
			{
				parser_warning(c, "error loading mesh: "+String(wname), PARSER_ERROR);
			}
		}
		else
		{
			vwheels[free_wheel].fm=new FlexMesh(wname, nodes, node1, node2, nodebase, rays, texf, texb);
			try
			{
				Entity *ec = gEnv->sceneManager->createEntity(wnamei, wname);
				MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, ColourValue(0, 0.5, 0.5));
				if (materialFunctionMapper) materialFunctionMapper->replaceMeshMaterials(ec);
				if (materialReplacer) materialReplacer->replaceMeshMaterials(ec);
				if (usedSkin) usedSkin->replaceMeshMaterials(ec);
				vwheels[free_wheel].cnode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
				if (ec)
				{
					deletion_Entities.emplace_back(ec);
					vwheels[free_wheel].cnode->attachObject(ec);
				}
			} catch(...)
			{
				parser_warning(c, "error loading mesh: "+String(wname), PARSER_ERROR);
			}
		}
	}
	free_wheel++;
}

void SerializedRig::addWheel2(Ogre::SceneNode* parent, float radius , float radius2 , float width , int rays , int node1 , int node2 , int snode , int braked , int propulsed , int torquenode , float mass , float wspring , float wdamp , float wspring2 , float wdamp2 , char* texf , char* texb , parsecontext_t *c/* =0 */)
{
	int i;
	int nodebase=free_node;
	int node3;
	int contacter_wheel=1;
	//ignore the width parameter
	width=(nodes[node1].RelPosition-nodes[node2].RelPosition).length();
	//enforce the "second node must have a larger Z coordinate than the first" constraint
	if (nodes[node1].RelPosition.z>nodes[node2].RelPosition.z)
	{
		//swap
		node3=node1;
		node1=node2;
		node2=node3;
	}
	//ignore the sign of snode, just do the thing automatically
	//if (snode<0) node3=-snode; else node3=snode;
	if (snode<0) snode=-snode;
	bool closest1=false;
	if (snode!=9999) closest1=(nodes[snode].RelPosition-nodes[node1].RelPosition).length()<(nodes[snode].RelPosition-nodes[node2].RelPosition).length();

	//unused:
	//Real px=nodes[node1].Position.x;
	//Real py=nodes[node1].Position.y;
	//Real pz=nodes[node1].Position.z;

	Vector3 axis=nodes[node2].RelPosition-nodes[node1].RelPosition;
	axis.normalise();
	Vector3 rayvec=Vector3(0, radius, 0);
	Quaternion rayrot=Quaternion(Degree(-360.0/(Real)rays), axis);
	Quaternion rayrot2=Quaternion(Degree(-180.0/(Real)rays), axis);
	Vector3 rayvec2=Vector3(0, radius2, 0);
	rayvec2=rayrot2*rayvec2;
	//rim nodes
	for (i=0; i<rays; i++)
	{
		//with propnodes
		Vector3 raypoint=nodes[node1].RelPosition+rayvec;
		init_node(nodebase+i*2, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(4.0*rays),1, -1, -1, free_wheel, default_node_friction, default_node_volume, default_node_surface, NODE_LOADWEIGHT_DEFAULT);
		// outer ring has wheelid%2 != 0
		nodes[nodebase+i*2].iswheel = free_wheel*2+1;

		raypoint=nodes[node2].RelPosition+rayvec;
		init_node(nodebase+i*2+1, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(4.0*rays),1, -1, -1, free_wheel, default_node_friction, default_node_volume, default_node_surface, NODE_LOADWEIGHT_DEFAULT);

		// inner ring has wheelid%2 == 0
		nodes[nodebase+i*2+1].iswheel = free_wheel*2+2;
		//wheel object
		wheels[free_wheel].nodes[i*2]=&nodes[nodebase+i*2];
		wheels[free_wheel].nodes[i*2+1]=&nodes[nodebase+i*2+1];
		rayvec=rayrot*rayvec;
	}
	//tire nodes
	for (i=0; i<rays; i++)
	{
		//with propnodes and variable friction
		Vector3 raypoint=nodes[node1].RelPosition+rayvec2;
		init_node(nodebase+2*rays+i*2, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, 0.67*mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width, -1, free_wheel, default_node_friction, default_node_volume, default_node_surface);
		// outer ring has wheelid%2 != 0
		nodes[nodebase+2*rays+i*2].iswheel = free_wheel*2+1;
		if (contacter_wheel)
		{
			contacters[free_contacter].nodeid=nodebase+2*rays+i*2;
			contacters[free_contacter].contacted=0;
			contacters[free_contacter].opticontact=0;
			free_contacter++;;
		}
		raypoint=nodes[node2].RelPosition+rayvec2;
		init_node(nodebase+2*rays+i*2+1, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, 0.33*mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width, -1,  free_wheel, default_node_friction, default_node_volume, default_node_surface);

		// inner ring has wheelid%2 == 0
		nodes[nodebase+2*rays+i*2+1].iswheel = free_wheel*2+2;
		if (contacter_wheel)
		{
			contacters[free_contacter].nodeid=nodebase+2*rays+i*2+1;
			contacters[free_contacter].contacted=0;
			contacters[free_contacter].opticontact=0;
			free_contacter++;;
		}
		//wheel object
//			wheels[free_wheel].nodes[i*2]=&nodes[nodebase+i*2];
//			wheels[free_wheel].nodes[i*2+1]=&nodes[nodebase+i*2+1];
		rayvec2=rayrot*rayvec2; //this is not a bug
	}
	free_node+=4*rays;
	for (i=0; i<rays; i++)
	{
		//rim
		//bounded
		add_beam(parent, &nodes[node1], &nodes[nodebase+i*2], BEAM_INVISIBLE, default_break, wspring, wdamp, DEFAULT_DETACHER_GROUP, -1.0, 0.66, 0.0);
		add_beam(parent, &nodes[node2], &nodes[nodebase+i*2+1], BEAM_INVISIBLE, default_break, wspring, wdamp, DEFAULT_DETACHER_GROUP, -1.0, 0.66, 0.0);
		add_beam(parent, &nodes[node2], &nodes[nodebase+i*2], BEAM_INVISIBLE, default_break, wspring, wdamp);
		add_beam(parent, &nodes[node1], &nodes[nodebase+i*2+1], BEAM_INVISIBLE, default_break, wspring, wdamp);
		//reinforcement
		add_beam(parent, &nodes[node1], &nodes[nodebase+i*2], BEAM_INVISIBLE, default_break, wspring, wdamp);
		add_beam(parent, &nodes[nodebase+i*2], &nodes[nodebase+i*2+1], BEAM_INVISIBLE, default_break, wspring, wdamp);
		add_beam(parent, &nodes[nodebase+i*2], &nodes[nodebase+((i+1)%rays)*2], BEAM_INVISIBLE, default_break, wspring, wdamp);
		add_beam(parent, &nodes[nodebase+i*2+1], &nodes[nodebase+((i+1)%rays)*2+1], BEAM_INVISIBLE, default_break, wspring, wdamp);
		add_beam(parent, &nodes[nodebase+i*2], &nodes[nodebase+((i+1)%rays)*2+1], BEAM_INVISIBLE, default_break, wspring, wdamp);
		//reinforcement
		add_beam(parent, &nodes[nodebase+i*2+1], &nodes[nodebase+((i+1)%rays)*2], BEAM_INVISIBLE, default_break, wspring, wdamp);
		if (snode!=9999)
		{
			//back beams
			if (closest1)
			{
				add_beam(parent, &nodes[snode], &nodes[nodebase+i*2], BEAM_VIRTUAL, default_break, wspring, wdamp);
			} else
			{
				add_beam(parent, &nodes[snode], &nodes[nodebase+i*2+1], BEAM_VIRTUAL, default_break, wspring, wdamp);
			};
		}
		//tire
		//band
		//init_beam(free_beam , &nodes[nodebase+2*rays+i*2], &nodes[nodebase+2*rays+i*2+1], BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		//pressure_beams[free_pressure_beam]=free_beam-1; free_pressure_beam++;
		int pos;
		pos=add_beam(parent, &nodes[nodebase+2*rays+i*2], &nodes[nodebase+2*rays+((i+1)%rays)*2], BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(parent, &nodes[nodebase+2*rays+i*2], &nodes[nodebase+2*rays+((i+1)%rays)*2+1], BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(parent, &nodes[nodebase+2*rays+i*2+1], &nodes[nodebase+2*rays+((i+1)%rays)*2], BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(parent, &nodes[nodebase+2*rays+i*2+1], &nodes[nodebase+2*rays+((i+1)%rays)*2+1], BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		//walls
		pos=add_beam(parent, &nodes[nodebase+2*rays+i*2], &nodes[nodebase+i*2], BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(parent, &nodes[nodebase+2*rays+i*2], &nodes[nodebase+((i+1)%rays)*2], BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(parent, &nodes[nodebase+2*rays+i*2+1], &nodes[nodebase+i*2+1], BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(parent, &nodes[nodebase+2*rays+i*2+1], &nodes[nodebase+((i+1)%rays)*2+1], BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		//reinforcement
		pos=add_beam(parent, &nodes[nodebase+2*rays+i*2], &nodes[nodebase+i*2+1], BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(parent, &nodes[nodebase+2*rays+i*2], &nodes[nodebase+((i+1)%rays)*2+1], BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(parent, &nodes[nodebase+2*rays+i*2+1], &nodes[nodebase+i*2], BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(parent, &nodes[nodebase+2*rays+i*2+1], &nodes[nodebase+((i+1)%rays)*2], BEAM_INVISIBLE, default_break, wspring2, wdamp2);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		//backpressure, bounded
		pos=add_beam(parent, &nodes[node1], &nodes[nodebase+2*rays+i*2], BEAM_INVISIBLE, default_break, wspring2, wdamp2, DEFAULT_DETACHER_GROUP, -1.0, radius/radius2, 0.0);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
		pos=add_beam(parent, &nodes[node2], &nodes[nodebase+2*rays+i*2+1], BEAM_INVISIBLE, default_break, wspring2, wdamp2, DEFAULT_DETACHER_GROUP, -1.0, radius/radius2, 0.0);
		pressure_beams[free_pressure_beam]=pos; free_pressure_beam++;
	}
	//wheel object
	wheels[free_wheel].braked=braked;
	wheels[free_wheel].propulsed=propulsed;
	wheels[free_wheel].nbnodes=2*rays;
	wheels[free_wheel].refnode0=&nodes[node1];
	wheels[free_wheel].refnode1=&nodes[node2];
	wheels[free_wheel].radius=radius;
	wheels[free_wheel].speed=0.0;
	wheels[free_wheel].width=width;
	wheels[free_wheel].rp=0;
	wheels[free_wheel].rp1=0;
	wheels[free_wheel].rp2=0;
	wheels[free_wheel].rp3=0;
	wheels[free_wheel].arm=&nodes[torquenode];
	if (propulsed)
	{
		//for inter-differential locking
		proppairs[proped_wheels]=free_wheel;
		proped_wheels++;
	}
	if (braked) braked_wheels++;
	//find near attach
	Real l1=(nodes[node1].RelPosition-nodes[torquenode].RelPosition).length();
	Real l2=(nodes[node2].RelPosition-nodes[torquenode].RelPosition).length();
	if (l1<l2) wheels[free_wheel].near_attach=&nodes[node1]; else wheels[free_wheel].near_attach=&nodes[node2];
	//visuals
	char wname[256];
	sprintf(wname, "wheel-%s-%i",truckname, free_wheel);
	char wnamei[256];
	sprintf(wnamei, "wheelobj-%s-%i",truckname, free_wheel);
	//	strcpy(texf, "tracks/wheelface,");
	if (!virtuallyLoaded)
	{
		vwheels[free_wheel].fm=new FlexMesh(wname, nodes, node1, node2, nodebase, rays, texf, texb, true, radius/radius2);
		try
		{
			Entity *ec = gEnv->sceneManager->createEntity(wnamei, wname);
			MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, ColourValue(0, 0.5, 0.5));
			if (materialFunctionMapper) materialFunctionMapper->replaceMeshMaterials(ec);
			if (materialReplacer) materialReplacer->replaceMeshMaterials(ec);
			if (usedSkin) usedSkin->replaceMeshMaterials(ec);
			//	ec->setMaterialName("tracks/wheel");
			//ec->setMaterialName("Test/ColourTest");
			vwheels[free_wheel].cnode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
			if (ec)
			{
				deletion_Entities.emplace_back(ec);
				vwheels[free_wheel].cnode->attachObject(ec);
			}
			//	cnode->setPosition(1000,2,940);
			free_wheel++;
		} catch(...)
		{
			parser_warning(c, "error loading mesh: "+String(wname), PARSER_ERROR);
		}
	}
}

void SerializedRig::addWheel3(Ogre::SceneNode* parent, float radius , float radius2 , float width , int rays , int node1 , int node2 , int snode , int braked , int propulsed , int torquenode , float mass , float wspring , float wdamp , float rspring , float rdamp , char* texf , char* texb , bool meshwheel/* =false  */, bool meshwheel2/* =false  */, float rimradius/* =0.0  */, bool rimreverse/* =false  */, parsecontext_t *c/* =0 */)
{
	if (propulsed)
	{
		propwheelcount++;
	}

	int i               = 0;
	int nodebase        = free_node;
	int node3           = 0;
	int contacter_wheel = 1;

	//ignore the width parameter
	width=(nodes[node1].RelPosition-nodes[node2].RelPosition).length();
	//enforce the "second node must have a larger Z coordinate than the first" constraint
	if (nodes[node1].RelPosition.z>nodes[node2].RelPosition.z)
	{
		//swap ->hacky and does not always work correct with truck spawn rotation, the truck is already rotated here. Fix with using the parsed node z-position instead of using the relpos ?
		node3=node1;
		node1=node2;
		node2=node3;
	}
	//ignore the sign of snode, just do the thing automatically
	if (snode<0)
		snode=-snode;

	bool closest1=false;
	if (snode!=9999)
		closest1=(nodes[snode].RelPosition-nodes[node1].RelPosition).length()<(nodes[snode].RelPosition-nodes[node2].RelPosition).length();

	Vector3 axis=nodes[node2].RelPosition-nodes[node1].RelPosition;
	axis.normalise();
	Quaternion rayrot=Quaternion(Degree(-360.0/(Real)(rays*2)), axis);

	//rim nodes
	Vector3 rayvec = axis.perpendicular() * radius2;

	for (i=0; i<rays; i++)
	{
	//with propnodes and variable friction
		Vector3 raypoint;
		raypoint=nodes[node1].RelPosition+rayvec;
		rayvec=rayrot*rayvec;
		init_node(nodebase+i*2, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(4.0*rays),1, WHEEL_FRICTION_COEF*width, -1, free_wheel, default_node_friction, default_node_volume, default_node_surface, NODE_LOADWEIGHT_DEFAULT);
		nodes[nodebase+i*2].iswheel = 0;
		raypoint=nodes[node2].RelPosition+rayvec;
		rayvec=rayrot*rayvec;
		init_node(nodebase+i*2+1, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(4.0*rays),1, WHEEL_FRICTION_COEF*width, -1, free_wheel, default_node_friction, default_node_volume, default_node_surface, NODE_LOADWEIGHT_DEFAULT);
		nodes[nodebase+i*2+1].iswheel = 0;
	}
	free_node+=2*rays;

	//tire nodes
	rayvec = axis.perpendicular() * radius;
	//rotate half a node step (tire to rim node offset)
	rayvec= rayrot * rayvec;


	for (i=0; i<rays; i++)
	{
	//with propnodes and variable friction
		Vector3 raypoint;
		raypoint=nodes[node1].RelPosition+rayvec;
		rayvec=rayrot*rayvec;
		init_node(nodebase+i*2+rays*2, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(4.0*rays),1, WHEEL_FRICTION_COEF*width, -1, free_wheel, default_node_friction, default_node_volume, default_node_surface, NODE_LOADWEIGHT_DEFAULT);
		nodes[nodebase+i*2+rays*2].iswheel = free_wheel*2+1;
		if (contacter_wheel)
		{
			contacters[free_contacter].nodeid=nodebase+i*2+rays*2;
			contacters[free_contacter].contacted=0;
			contacters[free_contacter].opticontact=0;
			free_contacter++;;
		}
		raypoint=nodes[node2].RelPosition+rayvec;
		rayvec=rayrot*rayvec;
		init_node(nodebase+i*2+1+rays*2, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(4.0*rays),1, WHEEL_FRICTION_COEF*width, -1, free_wheel, default_node_friction, default_node_volume, default_node_surface, NODE_LOADWEIGHT_DEFAULT);
		nodes[nodebase+i*2+1+rays*2].iswheel = free_wheel*2+2;
		if (contacter_wheel)
		{
			contacters[free_contacter].nodeid=nodebase+i*2+1+rays*2;
			contacters[free_contacter].contacted=0;
			contacters[free_contacter].opticontact=0;
			free_contacter++;;
		}
		//wheel object
		wheels[free_wheel].nodes[i*2]=&nodes[nodebase+i*2+rays*2];
		wheels[free_wheel].nodes[i*2+1]=&nodes[nodebase+i*2+1+rays*2];
	}
	free_node+=2*rays;

	for (i=0; i<rays; i++)
	{
		//rim axis to rim ring
		add_beam(parent, &nodes[node1], &nodes[nodebase+i*2], BEAM_INVISIBLE, default_break, rspring, rdamp, DEFAULT_DETACHER_GROUP);
		add_beam(parent, &nodes[node2], &nodes[nodebase+i*2+1], BEAM_INVISIBLE, default_break, rspring, rdamp, DEFAULT_DETACHER_GROUP);
		add_beam(parent, &nodes[node2], &nodes[nodebase+i*2], BEAM_INVISIBLE, default_break, rspring, rdamp, DEFAULT_DETACHER_GROUP);
		add_beam(parent, &nodes[node1], &nodes[nodebase+i*2+1], BEAM_INVISIBLE, default_break, rspring, rdamp, DEFAULT_DETACHER_GROUP);
		//reinforcement rim ring
		add_beam(parent, &nodes[nodebase+i*2], &nodes[nodebase+i*2+1], BEAM_INVISIBLE, default_break, rspring, rdamp, DEFAULT_DETACHER_GROUP);
		add_beam(parent, &nodes[nodebase+i*2], &nodes[nodebase+((i+1)%rays)*2], BEAM_INVISIBLE, default_break, rspring, rdamp, DEFAULT_DETACHER_GROUP);
		add_beam(parent, &nodes[nodebase+i*2+1], &nodes[nodebase+((i+1)%rays)*2+1], BEAM_INVISIBLE, default_break, rspring, rdamp, DEFAULT_DETACHER_GROUP);
		add_beam(parent, &nodes[nodebase+i*2+1], &nodes[nodebase+((i+1)%rays)*2], BEAM_INVISIBLE, default_break, rspring, rdamp, DEFAULT_DETACHER_GROUP);
	}

	int rimnode;
	int tirenode;

	for (i=0; i<rays; i++)
	{
		//this wheels use set_beam_defaults-settings for the tiretread beams like meshwheels2
		//rim to tiretread

		rimnode = nodebase + i*2;
		tirenode = nodebase + i*2 + rays*2;

		add_beam(parent, &nodes[rimnode], &nodes[tirenode], BEAM_INVISIBLE, default_break, wspring/2.0f, wdamp, DEFAULT_DETACHER_GROUP);

		if (i == 0)
		{
			add_beam(parent, &nodes[rimnode], &nodes[tirenode+rays*2-1], BEAM_INVISIBLE, default_break, wspring/2.0f, wdamp, DEFAULT_DETACHER_GROUP);
			add_beam(parent, &nodes[rimnode], &nodes[tirenode+rays*2-2], BEAM_INVISIBLE, default_break, wspring/2.0f, wdamp, DEFAULT_DETACHER_GROUP);
		} else
		{
			add_beam(parent, &nodes[rimnode], &nodes[tirenode-1], BEAM_INVISIBLE, default_break, wspring/2.0f, wdamp, DEFAULT_DETACHER_GROUP);
			add_beam(parent, &nodes[rimnode], &nodes[tirenode-2], BEAM_INVISIBLE, default_break, wspring/2.0f, wdamp, DEFAULT_DETACHER_GROUP);
		}

		add_beam(parent, &nodes[rimnode+1], &nodes[tirenode], BEAM_INVISIBLE, default_break, wspring/2.0f, wdamp, DEFAULT_DETACHER_GROUP);
		add_beam(parent, &nodes[rimnode+1], &nodes[tirenode+1], BEAM_INVISIBLE, default_break, wspring/2.0f, wdamp, DEFAULT_DETACHER_GROUP);

		if (i == 0)
		{
			add_beam(parent, &nodes[rimnode+1], &nodes[tirenode+rays*2-1], BEAM_INVISIBLE, default_break, wspring/2.0f, wdamp, DEFAULT_DETACHER_GROUP);
		} else
		{
			add_beam(parent, &nodes[rimnode+1], &nodes[tirenode-1], BEAM_INVISIBLE, default_break, wspring/2.0f, wdamp, DEFAULT_DETACHER_GROUP);
		}

		//reinforcement (tire tread)
		add_beam(parent, &nodes[rimnode+rays*2], &nodes[nodebase+i*2+1+rays*2], BEAM_INVISIBLE, default_break, default_spring, default_damp, DEFAULT_DETACHER_GROUP);
		add_beam(parent, &nodes[rimnode+rays*2], &nodes[nodebase+((i+1)%rays)*2+rays*2], BEAM_INVISIBLE, default_break, default_spring, default_damp, DEFAULT_DETACHER_GROUP);
		add_beam(parent, &nodes[nodebase+i*2+1+rays*2], &nodes[nodebase+((i+1)%rays)*2+1+rays*2], BEAM_INVISIBLE, default_break, default_spring, default_damp, DEFAULT_DETACHER_GROUP);
		add_beam(parent, &nodes[rimnode+1+rays*2], &nodes[nodebase+((i+1)%rays)*2+rays*2], BEAM_INVISIBLE, default_break, default_spring, default_damp, DEFAULT_DETACHER_GROUP);

		if (snode!=9999)
		{

			if (closest1)
			{
				add_beam(parent, &nodes[snode], &nodes[nodebase+i*2+rays*2], BEAM_VIRTUAL, default_break, wspring, wdamp);
			} else
			{
				add_beam(parent, &nodes[snode], &nodes[nodebase+i*2+1+rays*2], BEAM_VIRTUAL, default_break, wspring, wdamp);
			};
		}
	}

	//calculate the point where the support beams get stiff and prevent the tire tread nodes bounce into the rim rimradius / tire radius and add 5%, this is a shortbound calc in % !
	float length = 1.0f - ((radius2 / radius) * 0.95f);
	float ropespring = rspring;

	if (ropespring <= DEFAULT_SPRING)
		ropespring = DEFAULT_SPRING;

	for (i=0; i<rays; i++)
	{
		//tiretread anti collapse reinforcements, using precalced support beams
		tirenode = nodebase + i*2 + rays*2;
		add_beam(parent, &nodes[node1], &nodes[tirenode], BEAM_INVISIBLE, default_break, wspring/2.0f, wdamp, DEFAULT_DETACHER_GROUP, -1.0f, length, 0.0);
		add_beam(parent, &nodes[node2], &nodes[tirenode+1], BEAM_INVISIBLE, default_break, wspring/2.0f, wdamp, DEFAULT_DETACHER_GROUP, -1.0f, length, 0.0);
	}

	//wheel object
	wheels[free_wheel].braked=braked;
	wheels[free_wheel].propulsed=propulsed;
	wheels[free_wheel].nbnodes=2*rays;
	wheels[free_wheel].refnode0=&nodes[node1];
	wheels[free_wheel].refnode1=&nodes[node2];
	wheels[free_wheel].radius=radius;
	wheels[free_wheel].speed=0.0;
	wheels[free_wheel].rp=0;
	wheels[free_wheel].rp1=0;
	wheels[free_wheel].rp2=0;
	wheels[free_wheel].rp3=0;
	wheels[free_wheel].width=width;
	wheels[free_wheel].arm=&nodes[torquenode];
	wheels[free_wheel].lastContactInner=Vector3::ZERO;
	wheels[free_wheel].lastContactOuter=Vector3::ZERO;
	wheels[free_wheel].firstLock=false;
	if (propulsed>0)
	{
		//for inter-differential locking
		proppairs[proped_wheels]=free_wheel;
		proped_wheels++;
	}
	if (braked) braked_wheels++;
	//find near attach
	Real l1=(nodes[node1].RelPosition-nodes[torquenode].RelPosition).length();
	Real l2=(nodes[node2].RelPosition-nodes[torquenode].RelPosition).length();
	if (l1<l2) wheels[free_wheel].near_attach=&nodes[node1]; else wheels[free_wheel].near_attach=&nodes[node2];
	//visuals
	char wname[256];
	sprintf(wname, "wheel-%s-%i",truckname, free_wheel);
	char wnamei[256];
	sprintf(wnamei, "wheelobj-%s-%i",truckname, free_wheel);
	//	strcpy(texf, "tracks/wheelface,");
	vwheels[free_wheel].meshwheel = meshwheel;

	if (!virtuallyLoaded)
	{
		vwheels[free_wheel].fm=new FlexMeshWheel(wname, nodes, node1, node2, nodebase, rays, texf, texb, rimradius, rimreverse, materialFunctionMapper, usedSkin, materialReplacer);
		try
		{
			Entity *ec = gEnv->sceneManager->createEntity(wnamei, wname);
			vwheels[free_wheel].cnode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
			if (ec)
			{
				deletion_Entities.emplace_back(ec);
				vwheels[free_wheel].cnode->attachObject(ec);
			}
			MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, ColourValue(0, 0.5, 0.5));
			if (materialFunctionMapper) materialFunctionMapper->replaceMeshMaterials(ec);
			if (materialReplacer) materialReplacer->replaceMeshMaterials(ec);
			if (usedSkin) usedSkin->replaceMeshMaterials(ec);
		} catch(...)
		{
			parser_warning(c, "error loading mesh: "+String(wname), PARSER_ERROR);
		}
	}
	free_wheel++;
}

void SerializedRig::addCamera(int nodepos, int nodedir, int noderoll)
{
	cameranodepos[freecamera]=nodepos;
	cameranodedir[freecamera]=nodedir;
	cameranoderoll[freecamera]=noderoll;
	freecamera++;
}

float SerializedRig::warea(Vector3 ref, Vector3 x, Vector3 y, Vector3 aref)
{
	return (((x-ref).crossProduct(y-ref)).length()+((x-aref).crossProduct(y-aref)).length())*0.5f;
}

bool SerializedRig::parseRailGroupLine(parsecontext_t c)
{
	const Ogre::String line = String(c.line);
	Ogre::StringVector options = Ogre::StringUtil::split(line, ",");

	if ( options.size() < 3)
	{
		parser_warning(c, "RAILGROUP: not enough nodes: " + String(line), PARSER_ERROR);
		return false;
	}

	unsigned int railID = StringConverter::parseInt(options[0]);
	options.erase( options.begin() );

	Rail* newRail = parseRailString( options , c);
	if ( !newRail ) return false;
	RailGroup* newGroup = new RailGroup( newRail, railID );

	mRailGroups.push_back(newGroup); // keep track of all allocated Rails
	return true;
}

bool SerializedRig::parseSlideNodeLine(parsecontext_t c)
{
	const Ogre::String line = String(c.line);
	// add spaces as a separator to remove them
	Ogre::StringVector options = Ogre::StringUtil::split(line, ", ");

	if ( options.size() < 2)
	{
		parser_warning(c, "SLIDENODE: not enough options provided: " + String(line), PARSER_ERROR);
		return false;
	}

	
	// find node ///////////////////////////////////////////////////////////////
	int nodeid = parse_node_number( c, options.front());
	node_t* node = getNode( nodeid );
	options.erase(options.begin()); // remove front element

	if ( !node )
	{
		parser_warning(c, "SLIDENODE: invalid node id: " +
				TOSTRING( nodeid ) , PARSER_ERROR);
		return false;
	}

	RailGroup* rgGroup = NULL;
	// options,
	//if ends are open or closed ends, ie can node slide right off the end of a Rail
	// attachment constraints, none, self, foreign, all

	//parser_warning(c, "SLIDENODE: making new SlideNode", PARSER_INFO);
	SlideNode newSlideNode = SlideNode(node, NULL);
	//unsigned int quantity = 1;
	
	// check if tolerance value was provided ///////////////////////////////////
	for ( bool moreOptions = true; moreOptions && options.size() > 0; )
	{
		std::pair<char, Ogre::String> option(options.back()[0],
				Ogre::String( options.back().begin() + 1, options.back().end() ) );
		
		switch( option.first )
		{
		// tolerance in meters, default 0.0f
		case 't': case 'T':
		{			
			parser_warning(c, "SLIDENODE: setting tolerance to : " + option.second , PARSER_INFO);
			newSlideNode.setThreshold( StringConverter::parseReal( option.second ) );
		}
		break;
		
		// specify a rail group
		case 'g': case 'G':
		{
			
			rgGroup = getRailFromID( StringConverter::parseReal( option.second ) );
			if ( !rgGroup )
			{
				parser_warning(c, "RAILGROUP: warning could not find "
						"a Railgroup with corresponding ID: " +
						option.second + ". Will check for anonymous rail", PARSER_WARNING);
			}
			else
				newSlideNode.setDefaultRail( rgGroup );
			
			break;
		}
		
		// Spring Rate in N/m, default 9000000
		case 's': case 'S':
			newSlideNode.setSpringRate( StringConverter::parseReal( option.second ) );
			break;
		
		// Force when beam breaks in N, default 1000000
		case 'b': case 'B':
			newSlideNode.setBreakForce( StringConverter::parseReal( option.second ) );
			break;
		// Attachment Rate in seconds, default disabled
		case 'r': case 'R':
			newSlideNode.setAttachmentRate( StringConverter::parseReal( option.second ) );
			break;

		// Maximum attachment Distance in meters, default infinity
		case 'd': case 'D':
			newSlideNode.setAttachmentDistance( StringConverter::parseReal( option.second ) );
			break;
		
		// Attachment constraints, bit Flags, default none
		case 'c': case 'C':
		{
			switch( option.second[0] )
			{
			case 'a': newSlideNode.setAttachRule( ATTACH_ALL ); break;
			case 'f': newSlideNode.setAttachRule( ATTACH_FOREIGN ); break;
			case 's': newSlideNode.setAttachRule( ATTACH_SELF ); break;
			case 'n': newSlideNode.setAttachRule( ATTACH_NONE ); break;
			}
		}
		break;
		
		// quantity, how many rails this node can slide on
		case 'q': case'Q': break;
			
		
		// no more options
		default:
			moreOptions = false;
			break;
		
		}
		
		if ( moreOptions ) options.pop_back();
	}

	// find beam ///////////////////////////////////////////////////////////////
	// rail builder allocates the memory for each rail, it will not free it,
	if ( !rgGroup && options.size() > 0 )
	{
		Rail* newRail = parseRailString( options , c);
		rgGroup = (newRail) ? new RailGroup(newRail) : NULL;

		newSlideNode.setDefaultRail( rgGroup );
		mRailGroups.push_back(rgGroup); // keep track of all allocated Rails
	}

	//parser_warning(c, "SLIDENODE: Adding Slide Rail", PARSER_INFO);
	mSlideNodes.push_back( newSlideNode );
	return true;
}

bool SerializedRig::parseCollisionBoxLine(parsecontext_t c)
{
	Ogre::StringVector nodeList = Ogre::StringUtil::split(c.line, ", ");
	
	if (nodeList.size() < 2)
	{
		parser_warning(c, "COLLISIONBOX: not enough nodes provided: " + String(c.line), PARSER_ERROR);
		return false;
	}

	for (Ogre::StringVector::iterator it = nodeList.begin(); it != nodeList.end(); ++it)
	{
		int nodeid = StringConverter::parseInt(*it);

		if (!getNode(nodeid))
		{
			parser_warning(c, "COLLISIONBOX: invalid node id: " + TOSTRING(nodeid) , PARSER_ERROR);
			continue;
		}

		nodes[nodeid].collisionBoundingBoxID = collisionBoundingBoxes.size();
	}

	collisionBoundingBoxes.resize(collisionBoundingBoxes.size() + 1);

	return true;
}

// Utility functions ///////////////////////////////////////////////////////////
beam_t* SerializedRig::getBeam(unsigned int node1ID, unsigned int node2ID)
{

	for (unsigned  int j = 0; j < (unsigned  int)free_beam; ++j)
		if ( (beams[j].p1->id == (int)node1ID && beams[j].p2->id == (int)node2ID) ||
			(beams[j].p2->id == (int)node1ID && beams[j].p1->id == (int)node2ID) )
			return &beams[j];

	return NULL;
}

beam_t* SerializedRig::getBeam(node_t* node1, node_t* node2)
{
	// check for nulls
	if ( !node1 || !node2 ) return NULL;
	return getBeam( node1->id, node2->id);
}

node_t* SerializedRig::getNode(unsigned int id)
{
	for ( unsigned int i = 0 ; i < (unsigned  int)free_node; ++i)
		if ( nodes[i].id == (int)id )
			return &nodes[i];

	// node not found
	return NULL;
}

RailGroup* SerializedRig::getRailFromID(unsigned int id)
{
	for (std::vector< RailGroup* >::iterator it = mRailGroups.begin(); it != mRailGroups.end(); it++)
	{
		if ( (*it)->getID() == id ) return (*it);
	}
	// Rail not found
	return NULL;
}

Rail* SerializedRig::parseRailString( const Ogre::StringVector & railStrings, parsecontext_t c)
{
	std::vector<int> nodeids;

	// convert all strings to integers
	for ( unsigned int i = 0; i < railStrings.size(); ++i)
	{
		// check if value is a node range
		if ( railStrings[i].find("-") != Ogre::String::npos )	
		{
			size_t pos = railStrings[i].find("-");
    		
			// from start to '-'
			const int start = parse_node_number(c, railStrings[i].substr(0, pos) );
			// from character after '-' to end
			const int end = parse_node_number(c, railStrings[i].substr(++pos) );
    		
			// if start is a lower value than the end, we need to start at the
			// higher value and decrement to the lower value. to avoid duplicate
			// logic (ie one loop for positive and one loop for negative
			// compare the negative values, and add a negative 1 to the counter.
    		
			int inc = ( start < end ? +1 : -1 );
    		
			// multiply by the increment value (1 or -1), if we are decrmenting
			// compare the negative value to essentially flip the <= operator to
			// a >= operator ie
			//MYASSERT( !!( start <= end ) == !!(-start >= -end );
    		
			for (int j = start ; inc * j <= inc * end; j += inc )
			{
				// if end is lower than start then
				nodeids.push_back( j );
			}
    		
			continue;		
		}
    	
		nodeids.push_back(parse_node_number(c, railStrings[i]));
	}

	return getRails( nodeids,  c);
}

Rail* SerializedRig::getRails(const std::vector<int>& nodeids, parsecontext_t c)
{
	// find beam ///////////////////////////////////////////////////////////////
	// rail builder allocates the memory for each rail, it will not free it
	RailBuilder builder;

	if ( nodeids.front() == nodeids.back() )
	{
		parser_warning(c, "RAIL: Looping SlideRail", PARSER_INFO);
			builder.loopRail();
	}

	for ( unsigned int i = 0; i < (nodeids.size() - 1); ++i)
	{
		beam_t* beam = getBeam( nodeids[i], nodeids[i+1] );

		// Verify Beam
		if ( !beam )
		{
			parser_warning(c, "RAIL: invalid beam: " +
					TOSTRING(nodeids[i]) + ", " +
					TOSTRING(nodeids[i + 1]) , PARSER_ERROR);
			return NULL;
		}

		builder.pushBack( beam );
	}
	// get completed Rail transfers memory ownership
	return builder.getCompletedRail();
}

void SerializedRig::addSoundSource(SoundScriptInstance *ssi, int nodenum, int type /* = -2 */, parsecontext_t *c /* = 0 */)
{
	if (!ssi) return; //fizzle
	if (free_soundsource>=MAX_SOUNDSCRIPTS_PER_TRUCK)
	{
		parser_warning(c, "Error, too many sound sources per vehicle!", PARSER_ERROR);
		return;
	}
	soundsources[free_soundsource].ssi=ssi;
	soundsources[free_soundsource].nodenum=nodenum;
	soundsources[free_soundsource].type=type;
	free_soundsource++;
}

void SerializedRig::wash_calculator(Quaternion rot, parsecontext_t c)
{
	Quaternion invrot=rot.Inverse();
	//we will compute wash
	int w,p;
	for (p=0; p<free_aeroengine; p++)
	{
		Vector3 prop=invrot*nodes[aeroengines[p]->getNoderef()].RelPosition;
		float radius=aeroengines[p]->getRadius();
		for (w=0; w<free_wing; w++)
		{
			//left wash
			Vector3 wcent=invrot*((nodes[wings[w].fa->nfld].RelPosition+nodes[wings[w].fa->nfrd].RelPosition)/2.0);
			//check if wing is near enough along X (less than 15m back)
			if (wcent.x>prop.x && wcent.x<prop.x+15.0)
			{
				//check if it's okay vertically
				if (wcent.y>prop.y-radius && wcent.y<prop.y+radius)
				{
					//okay, compute wash coverage ratio along Z
					float wleft=(invrot*nodes[wings[w].fa->nfld].RelPosition).z;
					float wright=(invrot*nodes[wings[w].fa->nfrd].RelPosition).z;
					float pleft=prop.z+radius;
					float pright=prop.z-radius;
					float aleft=wleft;
					if (pleft<aleft) aleft=pleft;
					float aright=wright;
					if (pright>aright) aright=pright;
					if (aright<aleft)
					{
						//we have a wash
						float wratio=(aleft-aright)/(wleft-wright);
						wings[w].fa->addwash(p, wratio);
						parser_warning(c, "Wing "+TOSTRING(w)+" is washed by prop "+TOSTRING(p)+" at "+TOSTRING((float)(wratio*100.0))+"%", PARSER_INFO);
					}
				}
			}
		}
	}
}

void SerializedRig::calcBoundingBoxes()
{
	//BES_GFX_START(BES_GFX_calcBox);

	boundingBox.setExtents(nodes[0].AbsPosition.x, nodes[0].AbsPosition.y, nodes[0].AbsPosition.z, nodes[0].AbsPosition.x, nodes[0].AbsPosition.y, nodes[0].AbsPosition.z);
	collisionBoundingBoxes.clear();

	for (int i=0; i < free_node; i++)
	{
		boundingBox.merge(Ogre::Vector3(nodes[i].AbsPosition.x, nodes[i].AbsPosition.y, nodes[i].AbsPosition.z));
		if (nodes[i].collisionBoundingBoxID >= 0)
		{
			if ((unsigned int) nodes[i].collisionBoundingBoxID >= collisionBoundingBoxes.size())
			{
				collisionBoundingBoxes.push_back(Ogre::AxisAlignedBox(nodes[i].AbsPosition.x, nodes[i].AbsPosition.y, nodes[i].AbsPosition.z, nodes[i].AbsPosition.x, nodes[i].AbsPosition.y, nodes[i].AbsPosition.z));
			} else
			{
				collisionBoundingBoxes[nodes[i].collisionBoundingBoxID].merge(nodes[i].AbsPosition);
			}
		}
	}

	for (unsigned int i = 0; i < collisionBoundingBoxes.size(); i++)
	{
		collisionBoundingBoxes[i].setMinimum(collisionBoundingBoxes[i].getMinimum() - Vector3(0.05f, 0.05f, 0.05f));
		collisionBoundingBoxes[i].setMaximum(collisionBoundingBoxes[i].getMaximum() + Vector3(0.05f, 0.05f, 0.05f));
	}

	boundingBox.setMinimum(boundingBox.getMinimum() - Vector3(0.05f, 0.05f, 0.05f));
	boundingBox.setMaximum(boundingBox.getMaximum() + Vector3(0.05f, 0.05f, 0.05f));

	predictedBoundingBox = boundingBox;
	predictedCollisionBoundingBoxes = collisionBoundingBoxes;
	//BES_GFX_STOP(BES_GFX_calcBox);
}

void SerializedRig::calcLowestNode()
{
	lowestnode = 0;
	float miny = nodes[0].AbsPosition.y;

	for (int i=0; i < free_node; i++)
	{
		if (nodes[i].AbsPosition.y < miny)
		{
			miny = nodes[i].AbsPosition.y;
			lowestnode = i;
		}
	}
}

void SerializedRig::parser_warning(parsecontext_t &context, Ogre::String text, int errlvl)
{
	if (ignoreProblems) return;

	String errstr = "INFO    ";
	if (errlvl == PARSER_WARNING)
		errstr    = "WARNING ";
	else if (errlvl == PARSER_ERROR)
		errstr    = "ERROR   ";
	else if (errlvl == PARSER_FATAL_ERROR)
		errstr    = "FATAL   ";
	else if (errlvl == PARSER_OBSOLETE)
		errstr    = "OBSOLETE";
	
	String txt;
	if (BSETTING("REPO_MODE", false))
	{
		// custom code for the repo only
		String fn = context.filename;
		std::replace(fn.begin(), fn.end(), ' ', '_');

		String link = "<a href=\""+fn+".html#L"+Ogre::StringConverter::toString(context.linecounter)+"\" >"+fn+":"+Ogre::StringConverter::toString(context.linecounter)+"</a>";
		txt = "BIO|"+errstr+"|"+link+" | "+String(context.modeString)+" | " + text;
	} else
	{
		// normal, client output
		txt = "BIO|"+errstr+"|"+context.filename+":"+Ogre::StringConverter::toString(context.linecounter, 4, '0')+" | "+String(context.modeString)+" | " + text;
	}

	LOG(txt);
	// add the warning to the vector
	parsecontext_t context_copy = context;
	context_copy.warningText = txt;
	context_copy.warningLvl = errlvl;
	warnings.push_back(context_copy);
}

void SerializedRig::parser_warning(parsecontext_t *context, Ogre::String text, int errlvl)
{
	if (ignoreProblems) return;

	if (context)
		parser_warning(*context, text, errlvl);
	else
		LOG(text);
}

int SerializedRig::parse_args(parsecontext_t &context, Ogre::StringVector &args, int minArgNum)
{
	try
	{
		args = Ogre::StringUtil::split(context.line, ":|, \t");
	} catch(Exception &e)
	{
		parser_warning(context, "Exception on parsing: "+e.getFullDescription(), PARSER_ERROR);
		args.clear();
		throw(ParseException());
	}
	int n = (int)args.size();
	if (n < minArgNum)
	{
		parser_warning(context, "Too less arguments: "+TOSTRING(n)+" provided, "+TOSTRING(minArgNum)+" required. ", PARSER_ERROR);
		args.clear();
		throw(ParseException());
	}
	return n;
}

int SerializedRig::parse_node_number(parsecontext_t &context, Ogre::String s, std::vector<int> *special_numbers, bool ignoreError)
{
	if (free_node == 0)
	{
		// we got this before the user added any nodes, thats bad
		// but for compatibility reasons (soundsources, camera) we will return what was asked for and report an error only
		parser_warning(context, "Error: using node '"+s+"' before actually declaring that node. Please move this section ("+context.modeString+") after the nodes/nodes2 section.", PARSER_ERROR);
		return PARSEINT(s);
	}

	if (!node_names.empty())
	{
		// whooo, named nodes (nodes2)
		std::map<Ogre::String, int>::iterator it = node_names.find(s);
		if (it != node_names.end())
		{
			// found it, return integer node number value
			return it->second;
		}
	}

	int id = PARSEINT(s);

	// fix special case
	if (special_numbers && std::find(special_numbers->begin(), special_numbers->end(), id) != special_numbers->end())
	{
		// in there, valid
		return id;
	}


	if (id < 0)
	{
		id = -id;
		parser_warning(context, "Error: invalid node number "+s+", less than zero, using positive number, please fix", PARSER_OBSOLETE);
	}

	// check if the number is roughly correct
	if (id < free_node)
	{
		return id;
	} else
	{
		parser_warning(context, "Error: invalid node number "+s+", bigger than existing nodes ("+TOSTRING(free_node)+")", PARSER_ERROR);
	}

	if (!ignoreError)
	{
		throw(ParseException());
	}

	return 0;
}
