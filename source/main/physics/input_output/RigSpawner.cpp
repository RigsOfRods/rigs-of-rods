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
	@file   RigSpawner.cpp
	@brief  Vehicle spawning logic.
	@author Petr Ohlidal
	@date   12/2013
*/

/*
	NOTE:
	RoR's coding standards demand / * * / comments only.
	This file contains also // comments which are meant to be temporary (until this parser is stable).
	They speak about the legacy parser implementation and try to address possible bugs. 
*/

/*
	DISABLED:
		Hashing
		ScopeLog (What is it for?)

	QUESTIONS:
		init_node() param 'friction' is unused.
*/

#include "RoRPrerequisites.h"
#include "RigSpawner.h"

#include "AirBrake.h"
#include "Airfoil.h"
#include "AutoPilot.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "BitFlags.h"
#include "Buoyance.h"
#include "CmdKeyInertia.h"
#include "Collisions.h"
#include "Console.h"
#include "DashboardManager.h"
#include "Differentials.h"
#include "DustManager.h"
#include "FlexAirfoil.h"
#include "FlexBody.h"
#include "FlexMesh.h"
#include "FlexMeshWheel.h"
#include "FlexObj.h"
#include "InputEngine.h"
#include "MaterialFunctionMapper.h"
#include "MaterialReplacer.h"
#include "MeshObject.h"
#include "PointColDetector.h"
#include "ScrewProp.h"
#include "Settings.h"
#include "Skin.h"
#include "SlideNode.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "ThreadPool.h"
#include "TorqueCurve.h"
#include "TurboJet.h"
#include "TurboProp.h"
#include "VideoCamera.h"

#include <OgreMaterialManager.h>
#include <OgreSceneManager.h>
#include <OgreMovableObject.h>
#include <OgreParticleSystem.h>
#include <OgreEntity.h>

/* -------------------------------------------------------------------------- */
/* Prepare for loading
/* -------------------------------------------------------------------------- */

void RigSpawner::Setup( 
	Beam *rig,
	boost::shared_ptr<RigDef::File> file,
	Ogre::SceneNode *parent,
	Ogre::Vector3 const & spawn_position,
	Ogre::Quaternion const & spawn_rotation
)
{
	m_rig = rig;
	m_file = file;
	m_parent_scene_node = parent;
	m_spawn_position = spawn_position;
	m_spawn_rotation = spawn_rotation;
	m_current_keyword = RigDef::File::KEYWORD_INVALID;
	m_enable_background_loading = BSETTING("Background Loading", false);
	m_wing_area = 0.f;
	m_fuse_z_min = 1000.0f;
	m_fuse_z_max = -1000.0f;
	m_fuse_y_min = 1000.0f;
	m_fuse_y_max = -1000.0f;

	// add custom include path
	if (!SSETTING("resourceIncludePath", "").empty())
	{
		Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
			SSETTING("resourceIncludePath", ""), 
			"FileSystem", 
			"customInclude"
		);
	}

	// initialize custom include path
	if (!SSETTING("resourceIncludePath", "").empty())
	{
		Ogre::ResourceBackgroundQueue::getSingleton().initialiseResourceGroup("customInclude");
	}
}

/* -------------------------------------------------------------------------- */
/* Actual loading.
/* -------------------------------------------------------------------------- */

#define PROCESS_SECTION_IN_ANY_MODULE(_KEYWORD_, _CLASS_, _FIELD_, _FUNCTION_)             \
{                                                                                          \
    SetCurrentKeyword(_KEYWORD_);                                                          \
    auto module_itor = m_selected_modules.begin();                                         \
    for (; module_itor != m_selected_modules.end(); module_itor++)                         \
    {                                                                                      \
        _CLASS_ *def = module_itor->get()->_FIELD_.get();                                  \
        if (def != nullptr)                                                                \
        {                                                                                  \
            try {                                                                          \
                _FUNCTION_(*def);                                                          \
            }                                                                              \
            catch (Exception ex)                                                           \
            {                                                                              \
                AddMessage(Message::TYPE_ERROR, ex.what());                                \
            }                                                                              \
            catch (...)                                                                    \
            {                                                                              \
                AddMessage(Message::TYPE_ERROR, "An unknown exception has occured");       \
            }                                                                              \
            break;                                                                         \
        }                                                                                  \
    }                                                                                      \
    SetCurrentKeyword(RigDef::File::KEYWORD_INVALID);                                      \
}

#define PROCESS_SECTION_IN_ALL_MODULES(_KEYWORD_, _CLASS_, _FIELD_, _FUNCTION_)            \
{                                                                                          \
    SetCurrentKeyword(_KEYWORD_);                                                          \
    auto module_itor = m_selected_modules.begin();                                         \
    for (; module_itor != m_selected_modules.end(); module_itor++)                         \
    {                                                                                      \
        std::vector<_CLASS_>::iterator section_itor = module_itor->get()->_FIELD_.begin(); \
        for (; section_itor != module_itor->get()->_FIELD_.end(); section_itor++)          \
        {                                                                                  \
            try {                                                                          \
                _FUNCTION_(*section_itor);                                                 \
            }                                                                              \
            catch (RigSpawner::Exception & ex)                                             \
            {                                                                              \
                AddMessage(Message::TYPE_ERROR, ex.what());                                \
            }                                                                              \
            catch (...)                                                                    \
            {                                                                              \
                AddMessage(Message::TYPE_ERROR, "An unknown exception has occured");       \
            }                                                                              \
        }                                                                                  \
    }                                                                                      \
    SetCurrentKeyword(RigDef::File::KEYWORD_INVALID);                                      \
}

rig_t *RigSpawner::SpawnRig()
{
	InitializeRig();

	/* Vehicle name */
	m_rig->realtruckname = m_file->name;

	/* Flags in root module */
	m_rig->forwardcommands             = m_file->forward_commands;
	m_rig->importcommands              = m_file->import_commands;
	m_rig->wheel_contact_requested     = m_file->rollon;
	m_rig->rescuer                     = m_file->rescuer;
	m_rig->disable_default_sounds      = m_file->disable_default_sounds;
	m_rig->hideInChooser               = m_file->hide_in_chooser;
	m_rig->slideNodesConnectInstantly  = m_file->slide_nodes_connect_instantly;

	/* Section 'authors' in root module */
	ProcessAuthors();

	/* Section 'fileinfo' in root module */
	ProcessFileInfo();
	
	/* Section 'guid' in root module */
	if (! m_file->guid.empty())
	{
		strncpy(m_rig->guid, m_file->guid.c_str(), 128);
	}

	/* Section 'minimass' in root module */
	if (m_file->_minimum_mass_set)
	{
		m_rig->minimass = m_file->minimum_mass;
	}

	/* Section 'fileformatversion' in root module */
	m_rig->fileformatversion = m_file->file_format_version;

	/* Section 'managedmaterials' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_MANAGEDMATERIALS, RigDef::ManagedMaterial, managed_materials, ProcessManagedMaterial);

	/* Section 'gobals' in any module */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_GLOBALS, RigDef::Globals, globals, ProcessGlobals);

	/* Section 'help' in any module */
	ProcessHelp();

	/* Inline-section 'submesh_groundmodel' in any module */
	ProcessSubmeshGroundmodel();

	/* Sections 'nodes' & 'nodes2' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_NODES, RigDef::Node, nodes, ProcessNode);

	/* Old-format exhaust (defined by flags 'x/y' in section 'nodes', one per vehicle) */
	if (m_rig->smokeId != 0 && m_rig->smokeRef != 0)
	{
		AddExhaust(m_rig->smokeId, m_rig->smokeRef, true, nullptr);
	}

	/* Section 'beams' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_BEAMS, RigDef::Beam, beams, ProcessBeam);

	/* Section 'cinecam' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CINECAM, RigDef::Cinecam, cinecam, ProcessCinecam);

	/* Section 'shocks' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SHOCKS, RigDef::Shock, shocks, ProcessShock);

	/* Section 'shocks2' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SHOCKS2, RigDef::Shock2, shocks_2, ProcessShock2);

	/* Section 'commands' and 'commands2' (Use generated nodes) */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_COMMANDS2, RigDef::Command2, commands_2, ProcessCommand);

	/* Section 'ties' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TIES, RigDef::Tie, ties, ProcessTie);

	/* Section 'animators' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ANIMATORS, RigDef::Animator, animators, ProcessAnimator);

	/* Section 'hydros' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_HYDROS, RigDef::Hydro, hydros, ProcessHydro);

	/* Section 'wheels' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WHEELS, RigDef::Wheel, wheels, ProcessWheel);

	/* Section 'turbojets' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TURBOJETS, RigDef::Turbojet, turbojets, ProcessTurbojet);

	/* Section 'wings' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WINGS, RigDef::Wing, wings, ProcessWing);

	/* Section 'fusedrag' */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_FUSEDRAG, RigDef::Fusedrag, fusedrag, ProcessFusedrag);

	/* Section 'contacters' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CONTACTERS, RigDef::Node::Id, contacters, ProcessContacter);

	/* Sections 'flares' and 'flares2' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FLARES2, RigDef::Flare2, flares_2, ProcessFlare2);

	/* Section 'cameras' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CAMERAS, RigDef::Camera, cameras, ProcessCamera);

	/* Section 'videocamera' */
	/* 
		Videocameras must be processed before "props" otherwise they won't work. Reason unknown.
		Tested on:
			Gavril MZ2 [http://www.rigsofrods.com/threads/74698]
			Sisu SA-150 [http://www.rigsofrods.com/repository/view/3986]
	*/
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_VIDEOCAMERA, RigDef::VideoCamera, videocameras, ProcessVideoCamera);

	/* Section 'props' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_PROPS, RigDef::Prop, props, ProcessProp);

	/* Section 'engine' in any module */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_ENGINE, RigDef::Engine, engine, ProcessEngine);

	/* Section 'engoption' in any module */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_ENGOPTION, RigDef::Engoption, engoption, ProcessEngoption);

	/* Section 'brakes' in any module */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_BRAKES, RigDef::Brakes, brakes, ProcessBrakes);

	/* Section 'TractionControl' in any module. */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_TRACTION_CONTROL, RigDef::TractionControl, traction_control, ProcessTractionControl);

	/* Section 'AntiLockBrakes' in any module. */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_ANTI_LOCK_BRAKES, RigDef::AntiLockBrakes, anti_lock_brakes, ProcessAntiLockBrakes);

	/* Section 'SlopeBrake' in any module. */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_SLOPE_BRAKE, RigDef::SlopeBrake, slope_brake, ProcessSlopeBrake);

	/* Section 'wheels2' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WHEELS2, RigDef::Wheel2, wheels_2, ProcessWheel2);

	/* Section 'meshwheels' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_MESHWHEELS, RigDef::MeshWheel, mesh_wheels, ProcessMeshWheel);

	/* Section 'meshwheels2' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_MESHWHEELS2, RigDef::MeshWheel2, mesh_wheels_2, ProcessMeshWheel2);

	/* Section 'flexbodywheels' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FLEXBODYWHEELS, RigDef::FlexBodyWheel, flex_body_wheels, ProcessFlexBodyWheel);

	/* Section 'rotators' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROTATORS, RigDef::Rotator, rotators, ProcessRotator);

	/* Section 'rotators_2' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROTATORS2, RigDef::Rotator2, rotators_2, ProcessRotator2);

	/* Section 'rotators_2' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROTATORS2, RigDef::Rotator2, rotators_2, ProcessRotator2);

	/* Section 'triggers' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TRIGGERS, RigDef::Trigger, triggers, ProcessTrigger);

	/* Section 'triggers' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_LOCKGROUPS, RigDef::Lockgroup, lockgroups, ProcessLockgroup);

	/* Section 'hooks' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_HOOKS, RigDef::Hook, hooks, ProcessHook);	

	/* Section 'railgroups'*/
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_RAILGROUPS, RigDef::RailGroup, railgroups, ProcessRailGroup);

	/* Section 'slidenodes'*/
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SLIDENODES, RigDef::SlideNode, slidenodes, ProcessSlidenode);

	/* Section 'ropes'*/
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROPES, RigDef::Rope, ropes, ProcessRope);

	/* Section 'ropables' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROPABLES, RigDef::Ropable, ropables, ProcessRopable);

	/* Section 'particles' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_PARTICLES, RigDef::Particle, particles, ProcessParticle);

	/* Section 'torquecurve' in any module. */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_TORQUECURVE, RigDef::TorqueCurve, torque_curve, ProcessTorqueCurve);

	/* Section 'cruisecontrol' in any module. */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_CRUISECONTROL, RigDef::CruiseControl, cruise_control, ProcessCruiseControl);

	/* Section 'cruisecontrol' in any module. */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_SPEEDLIMITER, RigDef::SpeedLimiter, speed_limiter, ProcessSpeedLimiter);

	/* Section 'axles' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_AXLES, RigDef::Axle, axles, ProcessAxle);

	/* Section 'collisionboxes' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_COLLISIONBOXES, RigDef::CollisionBox, collision_boxes, ProcessCollisionBox);

	/* Section 'materialflarebindings' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_MATERIALFLAREBINDINGS, RigDef::MaterialFlareBinding, material_flare_bindings, ProcessMaterialFlareBinding);

	/* Section 'submeshes' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SUBMESH, RigDef::Submesh, submeshes, ProcessSubmesh);

	/* Section 'exhausts' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_EXHAUSTS, RigDef::Exhaust, exhausts, ProcessExhaust);

	/* Section 'guisettings' in any module */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_GUISETTINGS, RigDef::GuiSettings, gui_settings, ProcessGuiSettings);

	/* Section 'extcamera' */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_EXTCAMERA, RigDef::ExtCamera, ext_camera, ProcessExtCamera);

	/* Section 'camerarail' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CAMERARAIL, RigDef::CameraRail, camera_rails, ProcessCameraRail);	

	/* Section 'airbrakes' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_AIRBRAKES, RigDef::Airbrake, airbrakes, ProcessAirbrake);

	/* Section 'pistonprops' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_PISTONPROPS, RigDef::Pistonprop, pistonprops, ProcessPistonprop);

	/* Sections 'turboprops' and 'turboprops2' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TURBOPROPS2, RigDef::Turboprop2, turboprops_2, ProcessTurboprop2);

	/* Section 'screwprops' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SCREWPROPS, RigDef::Screwprop, screwprops, ProcessScrewprop);

	/* Section 'set_skeleton_settings' */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_SET_SKELETON_SETTINGS, RigDef::SkeletonSettings, skeleton_settings, ProcessSkeletonSettings);

	/* Section 'flexbodies' (Uses generated nodes) */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FLEXBODIES, boost::shared_ptr<RigDef::Flexbody>, flexbodies, ProcessFlexbody);

#ifdef USE_OPENAL

	/* Section 'soundsources' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SOUNDSOURCES, RigDef::SoundSource, soundsources, ProcessSoundSource);

	/* Section 'soundsources2' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SOUNDSOURCES2, RigDef::SoundSource2, soundsources2, ProcessSoundSource2);

#endif /* USE_OPENAL */

#if 0 // TEMPLATES

	/* Section '' */
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File:, RigDef:, , Process);

	/* Section '' */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File:, RigDef:, , Process);

#endif

	m_rig->loading_finished = true;

	/* POST-PROCESSING */
	FinalizeRig();

	/* Pass ownership */
	rig_t *rig = m_rig;
	m_rig = nullptr;
	return rig;
}

void RigSpawner::InitializeRig()
{
	m_rig->mCamera = nullptr;
	// clear rig parent structure
	memset(m_rig->nodes, 0, sizeof(node_t) * MAX_NODES);
	m_rig->free_node = 0;
	memset(m_rig->beams, 0, sizeof(beam_t) * MAX_BEAMS);
	m_rig->free_beam = 0;
	memset(m_rig->contacters, 0, sizeof(contacter_t) * MAX_CONTACTERS);
	m_rig->free_contacter = 0;
	memset(m_rig->rigidifiers, 0, sizeof(rigidifier_t) * MAX_RIGIDIFIERS);
	m_rig->free_rigidifier = 0;
	memset(m_rig->wheels, 0, sizeof(wheel_t) * MAX_WHEELS);
	m_rig->free_wheel = 0;
	memset(m_rig->vwheels, 0, sizeof(vwheel_t) * MAX_WHEELS);
	m_rig->ropes.clear();
	m_rig->ropables.clear();
	m_rig->ties.clear();
	m_rig->hooks.clear();
	memset(m_rig->wings, 0, sizeof(wing_t) * MAX_WINGS);
	m_rig->free_wing = 0;

	// commands contain complex data structures, do not memset them ...
	for (int i=0;i<MAX_COMMANDS+1;i++)
	{
		m_rig->commandkey[i].commandValue=0;
		m_rig->commandkey[i].beams.clear();
		m_rig->commandkey[i].rotators.clear();
		m_rig->commandkey[i].description="";
	}

	memset(m_rig->rotators, 0, sizeof(rotator_t) * MAX_ROTATORS);
	m_rig->free_rotator = 0;
	m_rig->flares.clear();
	m_rig->free_flare = 0;
	memset(m_rig->props, 0, sizeof(prop_t) * MAX_PROPS);
	m_rig->free_prop = 0;
	memset(m_rig->shocks, 0, sizeof(shock_t) * MAX_SHOCKS);
	m_rig->free_shock = 0;
	m_rig->free_active_shock = 0;
	m_rig->exhausts.clear();
	memset(m_rig->cparticles, 0, sizeof(cparticle_t) * MAX_CPARTICLES);
	m_rig->free_cparticle = 0;
	m_rig->nodes_debug.clear();
	m_rig->beams_debug.clear();
	memset(m_rig->soundsources, 0, sizeof(soundsource_t) * MAX_SOUNDSCRIPTS_PER_TRUCK);
	m_rig->free_soundsource = 0;
	memset(m_rig->pressure_beams, 0, sizeof(int) * MAX_PRESSURE_BEAMS);
	m_rig->free_pressure_beam = 0;
	memset(m_rig->aeroengines, 0, sizeof(AeroEngine *) * MAX_AEROENGINES);
	m_rig->free_aeroengine = 0;
	memset(m_rig->cabs, 0, sizeof(int) * (MAX_CABS*3));
	m_rig->free_cab = 0;
	memset(m_rig->subisback, 0, sizeof(int) * MAX_SUBMESHES);
	memset(m_rig->hydro, 0, sizeof(int) * MAX_HYDROS);
	m_rig->free_hydro = 0;
	for (int i=0;i<MAX_TEXCOORDS;i++) m_rig->texcoords[i] = Ogre::Vector3::ZERO;
	m_rig->free_texcoord=0;
	memset(m_rig->subtexcoords, 0, sizeof(int) * MAX_SUBMESHES);
	m_rig->free_sub = 0;
	memset(m_rig->subcabs, 0, sizeof(int) * MAX_SUBMESHES);
	memset(m_rig->collcabs, 0, sizeof(int) * MAX_CABS);
	memset(m_rig->collcabstype, 0, sizeof(int) * MAX_CABS);
	memset(m_rig->inter_collcabrate, 0, sizeof(collcab_rate_t) * MAX_CABS);
	m_rig->free_collcab = 0;
	memset(m_rig->intra_collcabrate, 0, sizeof(collcab_rate_t) * MAX_CABS);
	memset(m_rig->buoycabs, 0, sizeof(int) * MAX_CABS);
	m_rig->free_buoycab = 0;
	memset(m_rig->buoycabtypes, 0, sizeof(int) * MAX_CABS);
	memset(m_rig->airbrakes, 0, sizeof(Airbrake *) * MAX_AIRBRAKES);
	m_rig->free_airbrake = 0;
	memset(m_rig->skidtrails, 0, sizeof(Skidmark *) * (MAX_WHEELS*2));
	m_rig->useSkidmarks = false;
	memset(m_rig->flexbodies, 0, sizeof(FlexBody *) * MAX_FLEXBODIES);
	m_rig->free_flexbody = 0;
	m_rig->vidcams.clear();
	m_rig->description.clear();
	m_rig->free_axle = 0;
	m_rig->free_camerarail = 0;
	m_rig->free_screwprop = 0;

	memset(m_rig->guid, 0, 128);
	
	m_rig->hasfixes=0;
	m_rig->wingstart=-1;
	
	m_rig->realtruckname = "";
	m_rig->loading_finished=false;
	m_rig->forwardcommands=false;

	m_rig->wheel_contact_requested = false;
	m_rig->rescuer = false;
	m_rig->disable_default_sounds=false;
	m_rig->detacher_group_state=DEFAULT_DETACHER_GROUP; // initialize default(0) var for detacher_group_state
	m_rig->slopeBrake=false;
	m_rig->categoryid=-1;
	m_rig->truckversion=-1;
	m_rig->externalcameramode=0;
	m_rig->externalcameranode=-1;
	m_rig->authors.clear();
	m_rig->slideNodesConnectInstantly=false;

	m_rig->default_beam_diameter=DEFAULT_BEAM_DIAMETER;
	m_rig->skeleton_beam_diameter=BEAM_SKELETON_DIAMETER;
	strcpy(m_rig->default_beam_material, "tracks/beam");
	m_rig->default_plastic_coef=0;
	m_rig->default_node_friction=NODE_FRICTION_COEF_DEFAULT;
	m_rig->default_node_volume=NODE_VOLUME_COEF_DEFAULT;
	m_rig->default_node_surface=NODE_SURFACE_COEF_DEFAULT;
	m_rig->default_node_loadweight=NODE_LOADWEIGHT_DEFAULT;

	m_rig->odometerTotal = 0;
	m_rig->odometerUser  = 0;
	m_rig->dashBoardLayouts.clear();

	memset(m_rig->default_node_options, 0, 49);
	memset(m_rig->texname, 0, 1023);
	memset(m_rig->helpmat, 0, 255);
	
	m_rig->fadeDist=150.0;
	m_rig->collrange=DEFAULT_COLLISION_RANGE;
	m_rig->masscount=0;
	m_rig->disable_smoke = !SETTINGS.getBooleanSetting("Particles", true);
	m_rig->smokeId=0;
	m_rig->smokeRef=0;
	m_rig->editorId=-1;
	m_rig->beambreakdebug  = SETTINGS.getBooleanSetting("Beam Break Debug", false);
	m_rig->beamdeformdebug = SETTINGS.getBooleanSetting("Beam Deform Debug", false);
	m_rig->triggerdebug    = SETTINGS.getBooleanSetting("Trigger Debug", false);
	m_rig->rotaInertia = nullptr;
	m_rig->hydroInertia = nullptr;
	m_rig->cmdInertia = nullptr;
	m_rig->truckmass=0;
	m_rig->loadmass=0;
	m_rig->buoyance = nullptr;

	m_rig->free_fixes=0;
	m_rig->propwheelcount=0;
	m_rig->free_commands=0;
	m_rig->fileformatversion=0;
	m_rig->sectionconfigs.clear();

	m_rig->origin=Ogre::Vector3::ZERO;
	m_rig->mSlideNodes.clear();

	m_rig->engine = nullptr;
	m_rig->hascommands=0;
	m_rig->hashelp=0;
	m_rig->cinecameranodepos[0]=-1;
	m_rig->freecinecamera=0;
	m_rig->cablight = nullptr;
	m_rig->cablightNode = nullptr;
	m_rig->deletion_sceneNodes.clear();
	m_rig->deletion_Objects.clear();
	m_rig->netCustomLightArray[0] = -1;
	m_rig->netCustomLightArray[1] = -1;
	m_rig->netCustomLightArray[2] = -1;
	m_rig->netCustomLightArray[3] = -1;
	m_rig->netCustomLightArray_counter = 0;
	m_rig->materialFunctionMapper = nullptr;

	m_rig->driversseatfound=false;
	m_rig->ispolice=false;
	m_rig->state=SLEEPING;
	m_rig->hasposlights=false;
	m_rig->heathaze=false;
	m_rig->autopilot = nullptr;
	m_rig->hfinder = nullptr;
	m_rig->fuseAirfoil = nullptr;
	m_rig->fuseFront = nullptr;
	m_rig->fuseBack = nullptr;
	m_rig->fuseWidth=0;
	m_rig->brakeforce=30000.0;
	m_rig->hbrakeforce = 2 * m_rig->brakeforce;
	m_rig->debugVisuals = SETTINGS.getBooleanSetting("DebugBeams", false);
	m_rig->shadowOptimizations = SETTINGS.getBooleanSetting("Shadow optimizations", true);

	m_rig->proped_wheels=0;
	m_rig->braked_wheels=0;

	m_rig->speedomat = "";
	m_rig->tachomat = "";
	m_rig->speedoMax=140;
	m_rig->useMaxRPMforGUI=false;
	m_rig->minimass=50.0;
	m_rig->cparticle_enabled=false;
	m_rig->advanced_drag=false;
	m_rig->advanced_node_drag=0;
	m_rig->advanced_total_drag=0;
	m_rig->freecamera=0;
	m_rig->hasEmissivePass=0;
	m_rig->cabMesh = nullptr;
	m_rig->cabNode = nullptr;
	m_rig->cameranodepos[0]=-1;
	m_rig->cameranodedir[0]=-1;
	m_rig->cameranoderoll[0]=-1;
	m_rig->lowestnode=0;

	m_rig->subMeshGroundModelName = "";

	m_rig->materialReplacer = new MaterialReplacer();

	m_rig->beamHash = "";

	/* Init code from Beam::Beam() */

	m_rig->airbrakeval = 0;
	m_rig->alb_minspeed = 0.0f;
	m_rig->alb_mode = 0;
	m_rig->alb_notoggle = false;
	m_rig->alb_notoggle = false;
	m_rig->alb_present = false;
	m_rig->alb_pulse = 1;
	m_rig->alb_pulse_state = false;
	m_rig->alb_ratio = 0.0f;
	m_rig->animTimer = 0.0f;
	m_rig->antilockbrake = 0;

	m_rig->cabMesh = nullptr;
	m_rig->cablight = nullptr;
	m_rig->cablightNode = nullptr;

	m_rig->cc_mode = false;
	m_rig->cc_can_brake = false;
	m_rig->cc_target_rpm = 0.0f;
	m_rig->cc_target_speed = 0.0f;
	m_rig->cc_target_speed_lower_limit = 0.0f;

	m_rig->collisionRelevant = false;

	m_rig->debugVisuals = 0;

	m_rig->driverSeat = nullptr;

	m_rig->heathaze = !m_rig->disable_smoke && BSETTING("HeatHaze", false);
	m_rig->hideInChooser = false;

	m_rig->previousCrank = 0.f;

	m_rig->sl_enabled = false;
	m_rig->sl_speed_limit = 0.f;

	m_rig->tc_fade = 0.f;
	m_rig->tc_mode = 0;
	m_rig->tc_present = false;
	m_rig->tc_pulse = 1;
	m_rig->tc_pulse_state = false;
	m_rig->tc_ratio = 0.f;
	m_rig->tc_wheelslip = 0.f;
	m_rig->tcalb_timer = 0.f;

	m_rig->tractioncontrol = 0;

#ifdef USE_MYGUI
	m_rig->dash = new DashBoardManager();
#endif // USE_MYGUI

#ifdef FEAT_TIMING
	// this enables beam engine timing statistics
	statistics = BES.getClient(tnum, BES_CORE);
	statistics_gfx = BES.getClient(tnum, BES_GFX);
#endif

	for (int i=0; i<MAX_SUBMESHES; i++)
	{
		m_rig->subisback[i] = 0;
	}

	m_rig->simpleSkeletonNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
	m_rig->deletion_sceneNodes.emplace_back(m_rig->simpleSkeletonNode);
	
	m_rig->beamsRoot = m_parent_scene_node;

	m_rig->useSkidmarks = BSETTING("Skidmarks", false);

	/* Collisions */

	m_rig->disableTruckTruckCollisions = BSETTING("DisableCollisions", false);
	if (! m_rig->disableTruckTruckCollisions)
	{
		m_rig->interPointCD.emplace_back(new PointColDetector());

		if (gEnv->threadPool != nullptr)
		{
			for (int i=1; i<gEnv->threadPool->getSize(); i++)
			{
				m_rig->interPointCD.emplace_back(new PointColDetector());
			}
		}
	}
	m_rig->disableTruckTruckSelfCollisions = BSETTING("DisableSelfCollisions", false);
	if (! m_rig->disableTruckTruckSelfCollisions)
	{
		m_rig->intraPointCD.emplace_back(new PointColDetector());

		if (gEnv->threadPool != nullptr)
		{
			for (int i=1; i<gEnv->threadPool->getSize(); i++)
			{
				m_rig->intraPointCD.emplace_back(new PointColDetector());
			}
		}
	}

	m_rig->submesh_ground_model = gEnv->collisions->defaultgm;
	m_rig->cparticle_enabled = BSETTING("Particles", true);
	m_rig->dustp   = DustManager::getSingleton().getDustPool("dust");
	m_rig->dripp   = DustManager::getSingleton().getDustPool("dripp");
	m_rig->sparksp = DustManager::getSingleton().getDustPool("sparks");
	m_rig->clumpp  = DustManager::getSingleton().getDustPool("clump");
	m_rig->splashp = DustManager::getSingleton().getDustPool("splash");
	m_rig->ripplep = DustManager::getSingleton().getDustPool("ripple");

	m_rig->materialFunctionMapper = new MaterialFunctionMapper();
	m_rig->cmdInertia   = new CmdKeyInertia();
	m_rig->hydroInertia = new CmdKeyInertia();
	m_rig->rotaInertia  = new CmdKeyInertia();

	/* Lights mode */
	m_rig->flaresMode = 3; // on by default
	Ogre::String light_mode = SSETTING("Lights", "Only current vehicle, main lights");
	if (light_mode == "None (fastest)")
		m_rig->flaresMode = 0;
	else if (light_mode == "No light sources")
		m_rig->flaresMode = 1;
	else if (light_mode == "Only current vehicle, main lights")
		m_rig->flaresMode = 2;
	else if (light_mode == "All vehicles, main lights")
		m_rig->flaresMode = 3;
	else if (light_mode == "All vehicles, all lights")
		m_rig->flaresMode = 4;

}

void RigSpawner::FinalizeRig()
{
	// we should post-process the torque curve if existing
	if (m_rig->engine)
	{
		int result = m_rig->engine->getTorqueCurve()->spaceCurveEvenly(m_rig->engine->getTorqueCurve()->getUsedSpline());
		if (result)
		{
			m_rig->engine->getTorqueCurve()->setTorqueModel("default");
			if (result == 1)
			{
				AddMessage(Message::TYPE_ERROR, "TorqueCurve: Points (rpm) must be in an ascending order. Using default curve");
			}
		}
	}
	
	//calculate gwps height offset
	//get a starting value
	m_rig->posnode_spawn_height=m_rig->nodes[0].RelPosition.y;
	//start at 0 to avoid a crash whith a 1-node truck
	for (int i=0; i<m_rig->free_node; i++)
	{
		// scan and store the y-coord for the lowest node of the truck
		if (m_rig->nodes[i].RelPosition.y <= m_rig->posnode_spawn_height)
		{
			m_rig->posnode_spawn_height = m_rig->nodes[i].RelPosition.y;
		}
	}

	if (m_rig->cameranodepos[0] > 0)
	{
		// store the y-difference between the trucks lowest node and the campos-node for the gwps system
		m_rig->posnode_spawn_height = m_rig->nodes[m_rig->cameranodepos[0]].RelPosition.y - m_rig->posnode_spawn_height;
	} 
	else
	{
		//this can not be an airplane, just set it to 0.
		m_rig->posnode_spawn_height = 0.0f;
	}

	//cameras workaround
	for (int i=0; i<m_rig->freecamera; i++)
	{
		//LogManager::getSingleton().logMessage("Camera dir="+StringConverter::toString(nodes[cameranodedir[i]].RelPosition-nodes[cameranodepos[i]].RelPosition)+" roll="+StringConverter::toString(nodes[cameranoderoll[i]].RelPosition-nodes[cameranodepos[i]].RelPosition));
		Ogre::Vector3 dir_node_offset = GetNode(m_rig->cameranodedir[i]).RelPosition - GetNode(m_rig->cameranodepos[i]).RelPosition;
		Ogre::Vector3 roll_node_offset = GetNode(m_rig->cameranoderoll[i]).RelPosition - GetNode(m_rig->cameranodepos[i]).RelPosition;
		Ogre::Vector3 cross = dir_node_offset.crossProduct(roll_node_offset);
		
		m_rig->revroll[i]=cross.y > 0;//(nodes[cameranodedir[i]].RelPosition-nodes[cameranodepos[i]].RelPosition).crossProduct(nodes[cameranoderoll[i]].RelPosition-nodes[cameranodepos[i]].RelPosition).y>0;
		if (m_rig->revroll[i])
		{
			AddMessage(Message::TYPE_WARNING, "camera definition is probably invalid and has been corrected. It should be center, back, left");
		}
	}
	
	//wing closure
	if (m_rig->wingstart!=-1)
	{
		if (m_rig->autopilot != nullptr) 
		{
			m_rig->autopilot->setInertialReferences(
				& GetNode(m_airplane_left_light),
				& GetNode(m_airplane_right_light),
				m_rig->fuseBack,
				& GetNode(m_rig->cameranodepos[0])
				);
		}
		//inform wing segments
		float span=GetNode(m_rig->wings[m_rig->wingstart].fa->nfrd).RelPosition.distance(GetNode(m_rig->wings[m_rig->free_wing-1].fa->nfld).RelPosition);
		
		//parser_warning(c, "Full Wing "+TOSTRING(wingstart)+"-"+TOSTRING(free_wing-1)+" SPAN="+TOSTRING(span)+" AREA="+TOSTRING(wingarea), PARSER_INFO);
		m_rig->wings[m_rig->wingstart].fa->enableInducedDrag(span,m_wing_area, false);
		m_rig->wings[m_rig->free_wing-1].fa->enableInducedDrag(span,m_wing_area, true);
		//wash calculator
		WashCalculator(m_spawn_rotation);
	}
	//add the cab visual
	if (m_rig->free_texcoord>0 && m_rig->free_cab>0)
	{
		//closure
		m_rig->subtexcoords[m_rig->free_sub]=m_rig->free_texcoord;
		m_rig->subcabs[m_rig->free_sub]=m_rig->free_cab;
		char wname[256];
		sprintf(wname, "cab-%s",m_rig->truckname);
		char wnamei[256];
		sprintf(wnamei, "cabobj-%s",m_rig->truckname);
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

		Ogre::MaterialPtr mat=Ogre::MaterialManager::getSingleton().getByName(m_rig->texname);
		if (mat.isNull())
		{
			
#ifdef USE_MYGUI
			Console *console = Console::getSingletonPtrNoCreation();
			if (console) console->putMessage(
				Console::CONSOLE_MSGTYPE_INFO, 
				Console::CONSOLE_SYSTEM_ERROR, 
				"unable to load vehicle (Material '"+Ogre::String(m_rig->texname)+"' missing!): " + m_rig->realtruckname, 
				"error.png", 
				30000, 
				true
			);
#endif // USE_MYGUI

			Ogre::String msg = "Material '"+Ogre::String(m_rig->texname)+"' missing!";
			AddMessage(Message::TYPE_ERROR, msg);
			return;
		}

		//-trans
		char transmatname[256];
		sprintf(transmatname, "%s-trans", m_rig->texname);
		Ogre::MaterialPtr transmat=mat->clone(transmatname);
		if (mat->getTechnique(0)->getNumPasses()>1)
		{
			transmat->getTechnique(0)->removePass(1);
		}
		transmat->getTechnique(0)->getPass(0)->setAlphaRejectSettings(Ogre::CMPF_LESS_EQUAL, 128);
		transmat->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
		if (transmat->getTechnique(0)->getPass(0)->getNumTextureUnitStates()>0)
		{
			transmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_NONE);
		}
		transmat->compile();

		//-back
		char backmatname[256];
		sprintf(backmatname, "%s-back", m_rig->texname);
		Ogre::MaterialPtr backmat=mat->clone(backmatname);
		if (mat->getTechnique(0)->getNumPasses()>1)
		{
			backmat->getTechnique(0)->removePass(1);
		}
		if (transmat->getTechnique(0)->getPass(0)->getNumTextureUnitStates()>0)
		{
			backmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setColourOperationEx(
				Ogre::LBX_SOURCE1, 
				Ogre::LBS_MANUAL, 
				Ogre::LBS_MANUAL, 
				Ogre::ColourValue(0,0,0),
				Ogre::ColourValue(0,0,0)
			);
		}
		if (m_rig->shadowOptimizations)
		{
			backmat->setReceiveShadows(false);
		}
		//just in case
		//backmat->getTechnique(0)->getPass(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);
		//backmat->getTechnique(0)->getPass(0)->setAlphaRejectSettings(CMPF_GREATER, 128);
		backmat->compile();

		//-noem and -noem-trans
		if (mat->getTechnique(0)->getNumPasses()>1)
		{
			m_rig->hasEmissivePass=1;
			char clomatname[256];
			sprintf(clomatname, "%s-noem", m_rig->texname);
			Ogre::MaterialPtr clomat=mat->clone(clomatname);
			clomat->getTechnique(0)->removePass(1);
			clomat->compile();
		}

		//base texture is not modified
		//	mat->compile();


		//parser_warning(c, "creating mesh", PARSER_INFO);
		m_rig->cabMesh =new FlexObj(
			m_rig->nodes, 
			m_rig->free_texcoord, 
			m_rig->texcoords, 
			m_rig->free_cab, 
			m_rig->cabs, 
			m_rig->free_sub, 
			m_rig->subtexcoords, 
			m_rig->subcabs, 
			m_rig->texname, 
			wname, 
			m_rig->subisback, 
			backmatname, 
			transmatname
		);
		//parser_warning(c, "creating entity", PARSER_INFO);

		
		//parser_warning(c, "creating cabnode", PARSER_INFO);
		m_rig->cabNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
		Ogre::Entity *ec = nullptr;
		try
		{
			//parser_warning(c, "loading cab", PARSER_INFO);
			ec = gEnv->sceneManager->createEntity(wnamei, wname);
			//		ec->setRenderQueueGroup(RENDER_QUEUE_6);
			//parser_warning(c, "attaching cab", PARSER_INFO);
			if (ec)
			{
				m_rig->deletion_Entities.emplace_back(ec);
				m_rig->cabNode->attachObject(ec);
			}
		} catch(...)
		{
			//parser_warning(c, "error loading mesh: "+String(wname));
			Ogre::String msg = "error loading mesh: "+Ogre::String(wname);
			AddMessage(Message::TYPE_ERROR, msg);
		}
		MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, Ogre::ColourValue(0.5, 1, 0.5));
		if (m_rig->materialFunctionMapper != nullptr)
		{
			m_rig->materialFunctionMapper->replaceMeshMaterials(ec);
		}
		if (m_rig->materialReplacer != nullptr) 
		{
			m_rig->materialReplacer->replaceMeshMaterials(ec);
		}
		if (m_rig->usedSkin != nullptr) 
		{
			m_rig->usedSkin->replaceMeshMaterials(ec);
		}
		
	};
	//parser_warning(c, "cab ok", PARSER_INFO);
	//	mWindow->setDebugText("Beam number:"+ TOSTRING(free_beam));
/*
	if (c.mode == BTS_DESCRIPTION)
		parser_warning(c, "description section not closed with end_description", PARSER_ERROR);

	if (c.mode == BTS_COMMENT)
		parser_warning(c, "comment section not closed with end_comment", PARSER_ERROR);

	if (c.mode == BTS_SECTION)
		parser_warning(c, "section section not closed with end_section", PARSER_ERROR);
*/
	// check for some things
	if (strnlen(m_rig->guid, 128) == 0)
	{
		//parser_warning(c, "vehicle uses no GUID, skinning will be impossible", PARSER_OBSOLETE);
		AddMessage(Message::TYPE_WARNING, "vehicle uses no GUID, skinning will be impossible");
	}

	m_rig->lowestnode = FindLowestNodeInRig();

#if 0 // hashing + scope_log disabled

   // now generate the hash of it
	{
		// copy whole truck into a string
		Ogre::String code;
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
	
#endif
}

/* -------------------------------------------------------------------------- */
/* Processing functions and utilities.
/* -------------------------------------------------------------------------- */

void RigSpawner::WashCalculator(Ogre::Quaternion const & rot)
{
	Ogre::Quaternion invrot=rot.Inverse();
	//we will compute wash
	int w,p;
	for (p=0; p<m_rig->free_aeroengine; p++)
	{
		Ogre::Vector3 prop=invrot*m_rig->nodes[m_rig->aeroengines[p]->getNoderef()].RelPosition;
		float radius=m_rig->aeroengines[p]->getRadius();
		for (w=0; w<m_rig->free_wing; w++)
		{
			//left wash
			Ogre::Vector3 wcent=invrot*((m_rig->nodes[m_rig->wings[w].fa->nfld].RelPosition+m_rig->nodes[m_rig->wings[w].fa->nfrd].RelPosition)/2.0);
			//check if wing is near enough along X (less than 15m back)
			if (wcent.x>prop.x && wcent.x<prop.x+15.0)
			{
				//check if it's okay vertically
				if (wcent.y>prop.y-radius && wcent.y<prop.y+radius)
				{
					//okay, compute wash coverage ratio along Z
					float wleft=(invrot*m_rig->nodes[m_rig->wings[w].fa->nfld].RelPosition).z;
					float wright=(invrot*m_rig->nodes[m_rig->wings[w].fa->nfrd].RelPosition).z;
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
						m_rig->wings[w].fa->addwash(p, wratio);
						Ogre::String msg = "Wing "+TOSTRING(w)+" is washed by prop "+TOSTRING(p)+" at "+TOSTRING((float)(wratio*100.0))+"%";
						AddMessage(Message::TYPE_INFO, msg);
					}
				}
			}
		}
	}
}

void RigSpawner::ProcessTurbojet(RigDef::Turbojet & def)
{
	int front,back,ref;
	front = GetNodeIndexOrThrow(def.front_node);//parse_node_number(c, args[0]);
	back  = GetNodeIndexOrThrow(def.back_node);//parse_node_number(c, args[1]);
	ref   = GetNodeIndexOrThrow(def.side_node);//parse_node_number(c, args[2]);
	
	char propname[256];
	sprintf(propname, "turbojet-%s-%i", m_rig->truckname, m_rig->free_aeroengine);
	Turbojet *tj=new Turbojet(
		propname, 
		m_rig->free_aeroengine, 
		m_rig->trucknum, 
		m_rig->nodes, 
		front, 
		back, 
		ref, 
		def.dry_thrust, //drthrust, 
		def.is_reversable != 0, //rev!=0, 
		def.wet_thrust > 0, //abthrust>0, 
		def.wet_thrust, //abthrust, 
		def.front_diameter,
		def.back_diameter, //bdiam, 
		def.nozzle_length, //len, 
		m_rig->disable_smoke, 
		m_rig->heathaze, 
		m_rig->materialFunctionMapper, 
		m_rig->usedSkin, 
		m_rig->materialReplacer);

	m_rig->aeroengines[m_rig->free_aeroengine]=tj;
	m_rig->driveable=AIRPLANE;
	if (m_rig->autopilot == nullptr && m_rig->state != NETWORKED)
		m_rig->autopilot=new Autopilot(m_rig->trucknum);
	//if (audio) audio->setupAeroengines(TURBOJETS);
	
	m_rig->free_aeroengine++;
}

void RigSpawner::ProcessSkeletonSettings(RigDef::SkeletonSettings & def)
{
	m_rig->fadeDist = def.visibility_range_meters;
	m_rig->skeleton_beam_diameter = def.beam_thickness_meters;
}

void RigSpawner::ProcessScrewprop(RigDef::Screwprop & def)
{
	if (! CheckScrewpropLimit(1))
	{
		return;
	}

	int ref_node_idx = GetNodeIndexOrThrow(def.prop_node);
	int back_node_idx = GetNodeIndexOrThrow(def.back_node);
	int top_node_idx = GetNodeIndexOrThrow(def.top_node);

	m_rig->screwprops[m_rig->free_screwprop] = new Screwprop(
		m_rig->nodes,
		ref_node_idx,
		back_node_idx,
		top_node_idx,
		def.power,
		m_rig->trucknum
	);
	m_rig->driveable=BOAT;
	m_rig->free_screwprop++;
}

void RigSpawner::ProcessFusedrag(RigDef::Fusedrag & def)
{
	//parse fusedrag
	int front_node_idx = GetNodeIndexOrThrow(def.front_node);
	int back_node_idx = GetNodeIndexOrThrow(def.rear_node);
	float width = 1.f;
	float factor = 1.f;
	char fusefoil[256];
	strncpy(fusefoil, def.airfoil_name.c_str(), 255);

	if (def.autocalc)
	{
		// fusedrag autocalculation

		// calculate fusedrag by truck size
		if (def._area_coefficient_set)
		{
			factor = def.area_coefficient;
		}
		width  =  (m_fuse_z_max - m_fuse_z_min) * (m_fuse_y_max - m_fuse_y_min) * factor;			
		
		m_rig->fuseAirfoil = new Airfoil(fusefoil);
		
		m_rig->fuseFront   = & GetNode(front_node_idx); //&nodes[front];
		m_rig->fuseBack    = & GetNode(front_node_idx);  //&nodes[front];
		m_rig->fuseWidth   = width;
		AddMessage(Message::TYPE_INFO, "Fusedrag autocalculation size: "+TOSTRING(width)+" m^2");
	} else
	{
		// original fusedrag calculation

		width  = def.approximate_width;
					
		m_rig->fuseAirfoil = new Airfoil(fusefoil);
		
		m_rig->fuseFront   = & GetNode(front_node_idx); //&nodes[front];
		m_rig->fuseBack    = & GetNode(front_node_idx);  //&nodes[front];
		m_rig->fuseWidth   = width;
	}
}

void RigSpawner::BuildAerialEngine(
	int ref_node_index,
	int back_node_index,
	int blade_1_node_index,
	int blade_2_node_index,
	int blade_3_node_index,
	int blade_4_node_index,
	int couplenode_index,
	bool is_turboprops,
	Ogre::String const & airfoil,
	float power,
	float pitch
	)
{
	char propname[256];
	sprintf(propname, "turboprop-%s-%i", m_rig->truckname, m_rig->free_aeroengine);

	Turboprop *turbo_prop = new Turboprop(
		propname,
		m_rig->nodes, 
		ref_node_index,
		back_node_index,
		blade_1_node_index,
		blade_2_node_index,
		blade_3_node_index,
		blade_4_node_index,
		couplenode_index,
		power,
		airfoil,
		m_rig->free_aeroengine,
		m_rig->trucknum,
		m_rig->disable_smoke,
		! is_turboprops,
		pitch,
		m_rig->heathaze
	);
	m_rig->aeroengines[m_rig->free_aeroengine] = turbo_prop;
	m_rig->free_aeroengine++;
	m_rig->driveable = AIRPLANE;

	/* Autopilot */
	if (m_rig->autopilot == nullptr && m_rig->state != NETWORKED)
	{
		m_rig->autopilot = new Autopilot(m_rig->trucknum);
	}

	/* Visuals */
	float scale = GetNode(ref_node_index).RelPosition.distance(GetNode(blade_1_node_index).RelPosition) / 2.25f;
	for (unsigned int i = 0; i < static_cast<unsigned int>(m_rig->free_prop); i++)
	{
		prop_t & prop = m_rig->props[i];
		if (prop.noderef == ref_node_index)
		{
			if (prop.pale == 1)
			{
				prop.snode->scale(scale, scale, scale);
				turbo_prop->addPale(prop.snode);
			}
			if (prop.spinner == 1)
			{
				prop.snode->scale(scale, scale, scale);
				turbo_prop->addSpinner(prop.snode);
			}
		}
	}
}

void RigSpawner::ProcessTurboprop2(RigDef::Turboprop2 & def)
{
	int couple_node_index = (def.couple_node.IsValid())	? GetNodeIndexOrThrow(def.couple_node) : -1;

	BuildAerialEngine(
		GetNodeIndexOrThrow(def.reference_node),
		GetNodeIndexOrThrow(def.axis_node),
		GetNodeIndexOrThrow(def.blade_tip_nodes[0]),
		GetNodeIndexOrThrow(def.blade_tip_nodes[1]),
		GetNodeIndexOrThrow(def.blade_tip_nodes[2]),
		GetNodeIndexOrThrow(def.blade_tip_nodes[3]),
		couple_node_index,
		true,
		def.airfoil,
		def.turbine_power_kW,
		-10
	);
}

void RigSpawner::ProcessPistonprop(RigDef::Pistonprop & def)
{
	int couple_node_index = (def._couple_node_set) ? GetNodeIndexOrThrow(def.couple_node) : -1;

	BuildAerialEngine(
		GetNodeIndexOrThrow(def.reference_node),
		GetNodeIndexOrThrow(def.axis_node),
		GetNodeIndexOrThrow(def.blade_tip_nodes[0]),
		GetNodeIndexOrThrow(def.blade_tip_nodes[1]),
		GetNodeIndexOrThrow(def.blade_tip_nodes[2]),
		GetNodeIndexOrThrow(def.blade_tip_nodes[3]),
		couple_node_index,
		true,
		def.airfoil,
		def.turbine_power_kW,
		def.pitch
	);
}

void RigSpawner::ProcessAirbrake(RigDef::Airbrake & def)
{
	if (! CheckAirBrakeLimit(1))
	{
		return;
	}

	m_rig->airbrakes[m_rig->free_airbrake] = new Airbrake(
		m_rig->truckname,
		m_rig->free_airbrake, 
		GetNodePointerOrThrow(def.reference_node), 
		GetNodePointerOrThrow(def.x_axis_node),//&nodes[nx], 
		GetNodePointerOrThrow(def.y_axis_node),//&nodes[ny], 
		GetNodePointerOrThrow(def.aditional_node),//&nodes[na], 
		def.offset,//Vector3(ox,oy,oz), 
		def.width,//wd, 
		def.height,//len, 
		def.max_inclination_angle,//maxang, 
		m_rig->texname, 
		//tx1,tx2,tx3,tx4,liftcoef);
		def.texcoord_x1,
		def.texcoord_y1,
		def.texcoord_x2,
		def.texcoord_y2,
		def.lift_coefficient
	);
}

void RigSpawner::ProcessWing(RigDef::Wing & def)
{
	if (! CheckWingLimit(1))
	{
		return;
	}

	std::stringstream wing_name;
	wing_name << "wing-" << m_rig->truckname << "-" << m_rig->free_wing;
	std::stringstream wing_obj_name;
	wing_obj_name << "wingobj-" << m_rig->truckname << "-" << m_rig->free_wing;

	unsigned int wing_index = m_rig->free_wing;
	m_rig->free_wing++;
	wing_t & wing = m_rig->wings[wing_index];

	int node_indices[8];
	for (unsigned int i = 0; i < 8; i++)
	{
		node_indices[i] = GetNodeIndexOrThrow(def.nodes[i]);
	}

	wing.fa = new FlexAirfoil(
		wing_name.str(),
		m_rig->nodes,
		node_indices[0],
		node_indices[1],
		node_indices[2],
		node_indices[3],
		node_indices[4],
		node_indices[5],
		node_indices[6],
		node_indices[7],
		m_rig->texname,
		Ogre::Vector2(def.tex_coords[0], def.tex_coords[1]),
		Ogre::Vector2(def.tex_coords[2], def.tex_coords[3]),
		Ogre::Vector2(def.tex_coords[4], def.tex_coords[5]),
		Ogre::Vector2(def.tex_coords[6], def.tex_coords[7]),
		def.control_surface,
		def.chord_point,
		def.min_deflection,
		def.max_deflection,
		def.airfoil,
		def.efficancy_coef,
		m_rig->aeroengines,
		m_rig->state != NETWORKED
	);
					
	Ogre::Entity *entity = nullptr;
	try
	{
		entity = gEnv->sceneManager->createEntity(wing_obj_name.str(), wing_name.str());
	}
	catch (...)
	{
		AddMessage(Message::TYPE_ERROR, "Failed to load mesh (flexbody wing): " + wing_name.str());
		return;
	}
	m_rig->deletion_Entities.emplace_back(entity);
	MaterialFunctionMapper::replaceSimpleMeshMaterials(entity, Ogre::ColourValue(0.5, 1, 0));
	if (m_rig->materialFunctionMapper) 
	{
		m_rig->materialFunctionMapper->replaceMeshMaterials(entity);
	}
	if (m_rig->materialReplacer) 
	{
		m_rig->materialReplacer->replaceMeshMaterials(entity);
	}
	if (m_rig->usedSkin) 
	{
		m_rig->usedSkin->replaceMeshMaterials(entity);
	}
	wing.cnode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
	wing.cnode->attachObject(entity);
					
	//induced drag
	if (m_rig->wingstart==-1) 
	{
		m_rig->wingstart=wing_index;
		m_wing_area=ComputeWingArea(
			GetNode(wing.fa->nfld).AbsPosition, 
			GetNode(wing.fa->nfrd).AbsPosition, 
			GetNode(wing.fa->nbld).AbsPosition, 
			GetNode(wing.fa->nbrd).AbsPosition
		);
	}
	else
	{
		// disable position lights on trucks
		if (m_rig->driveable==TRUCK) 
		{
			m_rig->hasposlights=true;
		}

		if (node_indices[1] != m_rig->wings[wing_index - 1].fa->nfld)
		{
			wing_t & start_wing    = m_rig->wings[m_rig->wingstart];
			wing_t & previous_wing = m_rig->wings[wing_index - 1];

			//discontinuity
			//inform wing segments
			float span = GetNode( start_wing.fa->nfrd).RelPosition.distance( GetNode(previous_wing.fa->nfld).RelPosition );
			
			start_wing.fa->enableInducedDrag(span, m_wing_area, false);
			previous_wing.fa->enableInducedDrag(span, m_wing_area, true);

			//we want also to add positional lights for first wing
			if (!m_rig->hasposlights && m_rig->flaresMode>0)
			{
				if (! CheckPropLimit(4))
				{
					return;
				}

				//Left green
				m_airplane_left_light=previous_wing.fa->nfld;
				prop_t & left_green_prop = m_rig->props[m_rig->free_prop];
				m_rig->free_prop++;

				left_green_prop.noderef=previous_wing.fa->nfld;
				left_green_prop.nodex=previous_wing.fa->nflu;
				left_green_prop.nodey=previous_wing.fa->nfld; //ignored
				left_green_prop.offsetx=0.5;
				left_green_prop.offsety=0.0;
				left_green_prop.offsetz=0.0;
				left_green_prop.rot=Ogre::Quaternion::IDENTITY;
				left_green_prop.wheel=nullptr;
				left_green_prop.wheelrotdegree=0.0;
				left_green_prop.mirror=0;
				left_green_prop.pale=0;
				left_green_prop.spinner=0;
				left_green_prop.snode=nullptr; //no visible prop
				left_green_prop.bpos[0]=0.0;
				left_green_prop.brate[0]=1.0;
				left_green_prop.beacontype='L';
				left_green_prop.light[0]=nullptr; //no light
				//the flare billboard
				char propname[256];
				sprintf(propname, "prop-%s-%i", m_rig->truckname, m_rig->free_prop);
				left_green_prop.bbsnode[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
				left_green_prop.bbs[0]=gEnv->sceneManager->createBillboardSet(propname,1);
				left_green_prop.bbs[0]->createBillboard(0,0,0);
				if (left_green_prop.bbs[0])
				{
					left_green_prop.bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
					left_green_prop.bbs[0]->setMaterialName("tracks/greenflare");
					left_green_prop.bbsnode[0]->attachObject(left_green_prop.bbs[0]);
				}
				left_green_prop.bbsnode[0]->setVisible(false);
				left_green_prop.bbs[0]->setDefaultDimensions(0.5, 0.5);
				left_green_prop.animFlags[0]=0;
				left_green_prop.animMode[0]=0;
				
				//Left flash
				prop_t & left_flash_prop = m_rig->props[m_rig->free_prop];
				m_rig->free_prop++;

				left_flash_prop.noderef=previous_wing.fa->nbld;
				left_flash_prop.nodex=previous_wing.fa->nblu;
				left_flash_prop.nodey=previous_wing.fa->nbld; //ignored
				left_flash_prop.offsetx=0.5;
				left_flash_prop.offsety=0.0;
				left_flash_prop.offsetz=0.0;
				left_flash_prop.rot=Ogre::Quaternion::IDENTITY;
				left_flash_prop.wheel=nullptr;
				left_flash_prop.wheelrotdegree=0.0;
				left_flash_prop.mirror=0;
				left_flash_prop.pale=0;
				left_flash_prop.spinner=0;
				left_flash_prop.snode=nullptr; //no visible prop
				left_flash_prop.bpos[0]=0.5; //alt
				left_flash_prop.brate[0]=1.0;
				left_flash_prop.beacontype='w';
				//light
				sprintf(propname, "prop-%s-%i", m_rig->truckname, m_rig->free_prop);
				left_flash_prop.light[0]=gEnv->sceneManager->createLight(propname);
				left_flash_prop.light[0]->setType(Ogre::Light::LT_POINT);
				left_flash_prop.light[0]->setDiffuseColour( Ogre::ColourValue(1.0, 1.0, 1.0));
				left_flash_prop.light[0]->setSpecularColour( Ogre::ColourValue(1.0, 1.0, 1.0));
				left_flash_prop.light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
				left_flash_prop.light[0]->setCastShadows(false);
				left_flash_prop.light[0]->setVisible(false);
				//the flare billboard
				left_flash_prop.bbsnode[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
				left_flash_prop.bbs[0]=gEnv->sceneManager->createBillboardSet(propname,1);
				left_flash_prop.bbs[0]->createBillboard(0,0,0);
				if (left_flash_prop.bbs[0])
				{
					left_flash_prop.bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
					left_flash_prop.bbs[0]->setMaterialName("tracks/flare");
					left_flash_prop.bbsnode[0]->attachObject(left_flash_prop.bbs[0]);
				}
				left_flash_prop.bbsnode[0]->setVisible(false);
				left_flash_prop.bbs[0]->setDefaultDimensions(1.0, 1.0);
				
				//Right red
				m_airplane_right_light=previous_wing.fa->nfrd;
				prop_t & right_red_prop = m_rig->props[m_rig->free_prop];
				m_rig->free_prop++;

				
				right_red_prop.noderef=start_wing.fa->nfrd;
				right_red_prop.nodex=start_wing.fa->nfru;
				right_red_prop.nodey=start_wing.fa->nfrd; //ignored
				right_red_prop.offsetx=0.5;
				right_red_prop.offsety=0.0;
				right_red_prop.offsetz=0.0;
				right_red_prop.rot=Ogre::Quaternion::IDENTITY;
				right_red_prop.wheel=nullptr;
				right_red_prop.wheelrotdegree=0.0;
				right_red_prop.mirror=0;
				right_red_prop.pale=0;
				right_red_prop.spinner=0;
				right_red_prop.snode=nullptr; //no visible prop
				right_red_prop.bpos[0]=0.0;
				right_red_prop.brate[0]=1.0;
				right_red_prop.beacontype='R';
				right_red_prop.light[0]=nullptr; /* No light */
				//the flare billboard
				sprintf(propname, "prop-%s-%i", m_rig->truckname, m_rig->free_prop);
				right_red_prop.bbsnode[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
				right_red_prop.bbs[0]=gEnv->sceneManager->createBillboardSet(propname,1);
				right_red_prop.bbs[0]->createBillboard(0,0,0);
				if (right_red_prop.bbs[0])
				{
					right_red_prop.bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
					right_red_prop.bbs[0]->setMaterialName("tracks/redflare");
					right_red_prop.bbsnode[0]->attachObject(right_red_prop.bbs[0]);
				}
				right_red_prop.bbsnode[0]->setVisible(false);
				right_red_prop.bbs[0]->setDefaultDimensions(0.5, 0.5);
				right_red_prop.animFlags[0]=0;
				right_red_prop.animMode[0]=0;
				
				//Right flash
				prop_t & right_flash_prop = m_rig->props[m_rig->free_prop];
				m_rig->free_prop++;

				right_flash_prop.noderef=start_wing.fa->nbrd;
				right_flash_prop.nodex=start_wing.fa->nbru;
				right_flash_prop.nodey=start_wing.fa->nbrd; //ignored
				right_flash_prop.offsetx=0.5;
				right_flash_prop.offsety=0.0;
				right_flash_prop.offsetz=0.0;
				right_flash_prop.rot=Ogre::Quaternion::IDENTITY;
				right_flash_prop.wheel=nullptr;
				right_flash_prop.wheelrotdegree=0.0;
				right_flash_prop.mirror=0;
				right_flash_prop.pale=0;
				right_flash_prop.spinner=0;
				right_flash_prop.snode=nullptr; //no visible prop
				right_flash_prop.bpos[0]=0.5; //alt
				right_flash_prop.brate[0]=1.0;
				right_flash_prop.beacontype='w';
				//light
				sprintf(propname, "prop-%s-%i", m_rig->truckname, m_rig->free_prop);
				right_flash_prop.light[0]=gEnv->sceneManager->createLight(propname);
				right_flash_prop.light[0]->setType(Ogre::Light::LT_POINT);
				right_flash_prop.light[0]->setDiffuseColour( Ogre::ColourValue(1.0, 1.0, 1.0));
				right_flash_prop.light[0]->setSpecularColour( Ogre::ColourValue(1.0, 1.0, 1.0));
				right_flash_prop.light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
				right_flash_prop.light[0]->setCastShadows(false);
				right_flash_prop.light[0]->setVisible(false);
				//the flare billboard
				right_flash_prop.bbsnode[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
				right_flash_prop.bbs[0]=gEnv->sceneManager->createBillboardSet(propname,1);
				right_flash_prop.bbs[0]->createBillboard(0,0,0);
				if (right_flash_prop.bbs[0] != nullptr)
				{
					right_flash_prop.bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
					right_flash_prop.bbs[0]->setMaterialName("tracks/flare");
					right_flash_prop.bbsnode[0]->attachObject(right_flash_prop.bbs[0]);
				}
				right_flash_prop.bbsnode[0]->setVisible(false);
				right_flash_prop.bbs[0]->setDefaultDimensions(1.0, 1.0);
				right_flash_prop.animFlags[0]=0;
				right_flash_prop.animMode[0]=0;
				
				m_rig->hasposlights=true;
			}

			m_rig->wingstart=wing_index;
			m_wing_area=ComputeWingArea(
				GetNode(wing.fa->nfld).AbsPosition, 
				GetNode(wing.fa->nfrd).AbsPosition, 
				GetNode(wing.fa->nbld).AbsPosition, 
				GetNode(wing.fa->nbrd).AbsPosition
			);
		}
		else 
		{
			m_wing_area+=ComputeWingArea(
				GetNode(wing.fa->nfld).AbsPosition, 
				GetNode(wing.fa->nfrd).AbsPosition, 
				GetNode(wing.fa->nbld).AbsPosition, 
				GetNode(wing.fa->nbrd).AbsPosition
			);
		}
	}				
}

float RigSpawner::ComputeWingArea(Ogre::Vector3 const & ref, Ogre::Vector3 const & x, Ogre::Vector3 const & y, Ogre::Vector3 const & aref)
{
	return (((x-ref).crossProduct(y-ref)).length()+((x-aref).crossProduct(y-aref)).length())*0.5f;
}

void RigSpawner::ProcessSoundSource2(RigDef::SoundSource2 & def)
{
	int mode = (def.mode == RigDef::SoundSource2::MODE_CINECAM) ? def.cinecam_index : def.mode;
	int node_index = FindNodeIndex_AcceptNonExistentNumbered(def.node);
	if (node_index == -1)
	{
		return;
	}
	AddSoundSource(
			m_rig,
			SoundScriptManager::getSingleton().createInstance(def.sound_script_name, m_rig->trucknum), 
			node_index,
			mode
		);
}

void RigSpawner::AddSoundSourceInstance(Beam *vehicle, Ogre::String const & sound_script_name, int node_index, int type)
{
	AddSoundSource(vehicle, SoundScriptManager::getSingleton().createInstance(sound_script_name, vehicle->trucknum, nullptr), node_index);
}

void RigSpawner::AddSoundSource(Beam *vehicle, SoundScriptInstance *sound_script, int node_index, int type)
{
	if (! CheckSoundScriptLimit(vehicle, 1))
	{
		return;
	}

	if (sound_script == nullptr)
	{
		return;
	}

	vehicle->soundsources[vehicle->free_soundsource].ssi=sound_script;
	vehicle->soundsources[vehicle->free_soundsource].nodenum=node_index;
	vehicle->soundsources[vehicle->free_soundsource].type=type;
	vehicle->free_soundsource++;
}

void RigSpawner::ProcessSoundSource(RigDef::SoundSource & def)
{
	AddSoundSource(
			m_rig,
			SoundScriptManager::getSingleton().createInstance(def.sound_script_name, m_rig->trucknum), 
			GetNodeIndexOrThrow(def.node),
			-2
		);
}

void RigSpawner::ProcessCameraRail(RigDef::CameraRail & def)
{
	std::vector<RigDef::Node::Id>::iterator itor = def.nodes.begin();
	for(; itor != def.nodes.end(); itor++)
	{
		if (! CheckCameraRailLimit(1))
		{
			return;
		}
		m_rig->cameraRail[m_rig->free_camerarail] = GetNodeIndexOrThrow(*itor);
	}
}

void RigSpawner::ProcessExtCamera(RigDef::ExtCamera & def)
{
	m_rig->externalcameramode = def.mode;
	if (def.node.IsValid())
	{
		m_rig->externalcameranode = GetNodeIndexOrThrow(def.node);
	}
}

void RigSpawner::ProcessVideoCamera(RigDef::VideoCamera & def)
{
	VideoCamera *v = VideoCamera::Setup(this, def);
	if (v != nullptr)
	{
		m_rig->vidcams.push_back(v);
	}
	else
	{
		AddMessage(Message::TYPE_ERROR, "Failed to create video camera");
	}
}

void RigSpawner::ProcessGuiSettings(RigDef::GuiSettings & def)
{
	// guisettings
	/*char keyword[256];
	char value[256];
	int n = parse_args(c, args, 2);
	strncpy(keyword, args[0].c_str(), 255);
	strncpy(value,   args[1].c_str(), 255);*/

	m_rig->tachomat = def.tacho_material;
	m_rig->speedomat = def.speedo_material;
	strncpy(m_rig->helpmat, def.help_material.c_str(), 254);

	/*if (!strncmp(keyword, "debugBeams", 255) && strnlen(value, 255) > 0)
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
	}*/
	if (def.speedo_highest_kph > 10 && def.speedo_highest_kph < 32000)
	{
		m_rig->speedoMax = def.speedo_highest_kph; /* Handles default */
	}
	else
	{
		std::stringstream msg;
		msg << "Invalid 'speedo_highest_kph' value (" << def.speedo_highest_kph << "), allowed range is <10 -32000>. Falling back to default...";
		AddMessage(Message::TYPE_ERROR, msg.str());
		m_rig->speedoMax = RigDef::GuiSettings::DEFAULT_SPEEDO_MAX;
	}
	m_rig->useMaxRPMforGUI = def.use_max_rpm;  /* Handles default */

/*	else if (!strncmp(keyword, "speedoMax", 255) && strnlen(value, 255) > 0)
	{
		float tmp = StringConverter::parseReal(String(value));
		if (tmp > 10 && tmp < 32000)
			speedoMax = tmp;
	}
	else if (!strncmp(keyword, "useMaxRPM", 255) && strnlen(value, 255) > 0)
	{
		int use = StringConverter::parseInt(String(value));
		useMaxRPMforGUI = (use == 1);
	}*/

	std::list<Ogre::String>::iterator dash_itor = def.dashboard_layouts.begin();
	for ( ; dash_itor != def.dashboard_layouts.end(); dash_itor++)
	{
		m_rig->dashBoardLayouts.push_back(std::pair<Ogre::String, bool>(*dash_itor, false));
	}

	std::list<Ogre::String>::iterator rtt_itor = def.rtt_dashboard_layouts.begin();
	for ( ; rtt_itor != def.rtt_dashboard_layouts.end(); rtt_itor++)
	{
		m_rig->dashBoardLayouts.push_back(std::pair<Ogre::String, bool>(*rtt_itor, true));
	}

	/*else if (!strncmp(keyword, "dashboard", 255) && strnlen(value, 255) > 0)
	{
		dashBoardLayouts.push_back(std::pair<Ogre::String, bool>(String(value), false));
	}
	else if (!strncmp(keyword, "texturedashboard", 255) && strnlen(value, 255) > 0)
	{
		dashBoardLayouts.push_back(std::pair<Ogre::String, bool>(String(value), true));
	}*/
}

void RigSpawner::ProcessExhaust(RigDef::Exhaust & def)
{
	// parse exhausts
	if (m_rig->disable_smoke)
	{
		return;
	}
	/*int id1, id2;
	float factor;
	char material[256] = "";

	int n = parse_args(c, args, 2);
	id1 = parse_node_number(c, args[0]);
	id2 = parse_node_number(c, args[1]);
	if (n > 2) factor = PARSEREAL(args[2]);
	if (n > 3) strncpy(material, args[3].c_str(), 255);*/

	node_t & ref_node = GetNodeOrThrow(def.reference_node);//id1;
	node_t & dir_node = GetNodeOrThrow(def.direction_node);//id2;

	exhaust_t exhaust;
	exhaust.emitterNode = ref_node.pos;
	exhaust.directionNode = dir_node.pos;
	exhaust.isOldFormat = false;
	
	exhaust.smokeNode = m_parent_scene_node->createChildSceneNode();
	std::stringstream instance_name;
	instance_name << "exhaust-" << m_rig->exhausts.size() << "-" << m_rig->truckname;
	Ogre::String material_name;
	if (def.material_name.empty() || def.material_name == "default")
	{
		material_name = "tracks/Smoke";
	}
	else
	{
		material_name = def.material_name;
	}

	/*char wname[256];
	sprintf(wname, "exhaust-%d-%s", (int)exhausts.size(), truckname);
	if (strnlen(material,50) == 0 || String(material) == "default")
		strcpy(material, "tracks/Smoke");*/

	if (m_rig->usedSkin)
	{
		Ogre::String newMat = m_rig->usedSkin->getReplacementForMaterial(material_name);
		if (!newMat.empty())
		{
			//strncpy(material, newMat.c_str(), 50);
			material_name = newMat;
		}
	}

	exhaust.smoker = gEnv->sceneManager->createParticleSystem(instance_name.str(), material_name);
	if (!exhaust.smoker)
	{
		AddMessage(Message::TYPE_ERROR, "Failed to create exhaust particle system");
		return;
	}
	exhaust.smoker->setVisibilityFlags(DEPTHMAP_DISABLED); // disable particles in depthmap
	exhaust.smokeNode->attachObject(exhaust.smoker);
	exhaust.smokeNode->setPosition(m_rig->nodes[exhaust.emitterNode].AbsPosition);
	
	ref_node.isHot=true;
	dir_node.isHot=true;
	ref_node.iIsSkin=true;
	dir_node.iIsSkin=true;
	m_rig->exhausts.push_back(exhaust);
}

void RigSpawner::ProcessSubmeshGroundmodel()
{
	SetCurrentKeyword(RigDef::File::KEYWORD_SUBMESH_GROUNDMODEL);

	std::list<boost::shared_ptr<RigDef::File::Module>>::iterator module_itor = m_selected_modules.begin();
	for (; module_itor != m_selected_modules.end(); module_itor++)
	{
		if (! module_itor->get()->submeshes_ground_model_name.empty())
		{
			m_rig->subMeshGroundModelName = module_itor->get()->submeshes_ground_model_name;
			break;
		}
	}

	SetCurrentKeyword(RigDef::File::KEYWORD_INVALID);
};

void RigSpawner::ProcessSubmesh(RigDef::Submesh & def)
{
	if (! CheckSubmeshLimit(1))
	{
		return;
	}

	m_rig->subtexcoords[m_rig->free_sub]=m_rig->free_texcoord;
	m_rig->subcabs[m_rig->free_sub]=m_rig->free_cab;
	m_rig->free_sub++;
	//initialize the next
	m_rig->subisback[m_rig->free_sub]=0;

	/* TEXCOORDS */

	std::vector<RigDef::Texcoord>::iterator texcoord_itor = def.texcoords.begin();
	for ( ; texcoord_itor != def.texcoords.end(); texcoord_itor++)
	{
		if (! CheckTexcoordLimit(1))
		{
			break;
		}

		m_rig->texcoords[m_rig->free_texcoord] = Ogre::Vector3(GetNodeIndexOrThrow(texcoord_itor->node), texcoord_itor->u, texcoord_itor->v);//(id, x, y);
		m_rig->free_texcoord++;
	}

	/* CAB */

	std::vector<RigDef::Cab>::iterator cab_itor = def.cab_triangles.begin();
	for ( ; cab_itor != def.cab_triangles.end(); cab_itor++)
	{
		if (! CheckCabLimit(1))
		{
			return;
		}
		else if (m_rig->free_collcab >= MAX_CABS)
		{
			std::stringstream msg;
			msg << "Collcab limit (" << MAX_CABS << ") exceeded";
			AddMessage(Message::TYPE_ERROR, msg.str());
			return;
		}

		/*char type='n';
		int n = parse_args(c, args, 3);
		int id1 = parse_node_number(c, args[0]);
		int id2 = parse_node_number(c, args[1]);
		int id3 = parse_node_number(c, args[2]);
		if (n > 3) type = args[3][0];*/

		m_rig->cabs[m_rig->free_cab*3]=GetNodeIndexOrThrow(cab_itor->nodes[0]); //id1;
		m_rig->cabs[m_rig->free_cab*3+1]=GetNodeIndexOrThrow(cab_itor->nodes[1]);//id2;
		m_rig->cabs[m_rig->free_cab*3+2]=GetNodeIndexOrThrow(cab_itor->nodes[2]);//id3;

		//if (type=='c') 
		if (BITMASK_IS_1(cab_itor->options, RigDef::Cab::OPTION_c_CONTACT))
		{
			m_rig->collcabs[m_rig->free_collcab]=m_rig->free_cab; 
			m_rig->collcabstype[m_rig->free_collcab]=0; 
			m_rig->free_collcab++;
		}
		//if (type=='p') 
		if (BITMASK_IS_1(cab_itor->options, RigDef::Cab::OPTION_p_10xTOUGHER))
		{
			m_rig->collcabs[m_rig->free_collcab]=m_rig->free_cab; 
			m_rig->collcabstype[m_rig->free_collcab]=1; 
			m_rig->free_collcab++;
		}
		//if (type=='u') 
		if (BITMASK_IS_1(cab_itor->options, RigDef::Cab::OPTION_u_INVULNERABLE))
		{
			m_rig->collcabs[m_rig->free_collcab]=m_rig->free_cab; 
			m_rig->collcabstype[m_rig->free_collcab]=2; 
			m_rig->free_collcab++;
		}
		//if (type=='b')
		if (BITMASK_IS_1(cab_itor->options, RigDef::Cab::OPTION_b_BUOYANT))
		{
			m_rig->buoycabs[m_rig->free_buoycab]=m_rig->free_cab; 
			m_rig->collcabstype[m_rig->free_collcab]=0; 
			m_rig->buoycabtypes[m_rig->free_buoycab]=Buoyance::BUOY_NORMAL; 
			m_rig->free_buoycab++;   
			if (m_rig->buoyance == nullptr)
			{
				m_rig->buoyance=new Buoyance();
			}
		}
		//if (type=='r')
		if (BITMASK_IS_1(cab_itor->options, RigDef::Cab::OPTION_r_BUOYANT_ONLY_DRAG))
		{
			m_rig->buoycabs[m_rig->free_buoycab]=m_rig->free_cab; 
			m_rig->collcabstype[m_rig->free_collcab]=0; 
			m_rig->buoycabtypes[m_rig->free_buoycab]=Buoyance::BUOY_DRAGONLY; 
			m_rig->free_buoycab++; 
			if (m_rig->buoyance == nullptr)
			{
				m_rig->buoyance=new Buoyance();
			}
		}
		//if (type=='s') 
		if (BITMASK_IS_1(cab_itor->options, RigDef::Cab::OPTION_s_BUOYANT_NO_DRAG))
		{
			m_rig->buoycabs[m_rig->free_buoycab]=m_rig->free_cab; 
			m_rig->collcabstype[m_rig->free_collcab]=0; 
			m_rig->buoycabtypes[m_rig->free_buoycab]=Buoyance::BUOY_DRAGLESS; 
			m_rig->free_buoycab++; 
			if (m_rig->buoyance == nullptr)
			{
				m_rig->buoyance=new Buoyance();
			}
		}

		int collcabs_type = -1;
		if (cab_itor->GetOption_D_ContactBuoyant())
		{
			collcabs_type = 0;
		}
		if (cab_itor->GetOption_F_10xTougherBuoyant())
		{
			collcabs_type = 1;
		}
		if (cab_itor->GetOption_S_UnpenetrableBuoyant())
		{
			collcabs_type = 2;
		}

		//if (type=='D' || type == 'F' || type == 'S')
		if (collcabs_type != -1)
		{

			if (m_rig->free_collcab >= MAX_CABS)
			{
				std::stringstream msg;
				msg << "Collcab limit (" << MAX_CABS << ") exceeded";
				AddMessage(Message::TYPE_ERROR, msg.str());
				return;
			}
			else if (m_rig->free_buoycab >= MAX_CABS)
			{
				std::stringstream msg;
				msg << "Buoycab limit (" << MAX_CABS << ") exceeded";
				AddMessage(Message::TYPE_ERROR, msg.str());
				return;
			}

			/*if (free_collcab >= MAX_CABS || )
			{
				parser_warning(c, "unable to create buoycabs: cabs limit reached ("+TOSTRING(MAX_CABS)+")", PARSER_ERROR);
				continue;
			}*/
			m_rig->collcabs[m_rig->free_collcab]=m_rig->free_cab;
			m_rig->collcabstype[m_rig->free_collcab]=collcabs_type;
			m_rig->free_collcab++;
			m_rig->buoycabs[m_rig->free_buoycab]=m_rig->free_cab; 
			m_rig->buoycabtypes[m_rig->free_buoycab]=Buoyance::BUOY_NORMAL; 
			m_rig->free_buoycab++; 
			if (m_rig->buoyance == nullptr)
			{
				m_rig->buoyance=new Buoyance();
			}
		}
		m_rig->free_cab++;
	}

	/* BACKMESH */

	/* TODO: Verify this code, I ported it from SerializedRig::loadTruck() without fully understanding it :( ~only_a_ptr */

	//close the current mesh
	if (! CheckCabLimit(1))
	{
		return;
	}

	m_rig->subtexcoords[m_rig->free_sub]=m_rig->free_texcoord;
	m_rig->subcabs[m_rig->free_sub]=m_rig->free_cab;

	//make it normal
	m_rig->subisback[m_rig->free_sub]=0;
	m_rig->free_sub++;

	//add an extra front mesh
	int i;
	int start = (m_rig->free_sub==1) ? 0 : m_rig->subtexcoords[m_rig->free_sub-2];
	//texcoords
	for (i=start; i<m_rig->subtexcoords[m_rig->free_sub-1]; i++)
	{
		m_rig->texcoords[m_rig->free_texcoord] = m_rig->texcoords[i];;
		m_rig->free_texcoord++;
	}
	//cab
	start =  (m_rig->free_sub==1) ? 0 : m_rig->subcabs[m_rig->free_sub-2];

	for (i=start; i<m_rig->subcabs[m_rig->free_sub-1]; i++)
	{
		m_rig->cabs[m_rig->free_cab*3]=m_rig->cabs[i*3];
		m_rig->cabs[m_rig->free_cab*3+1]=m_rig->cabs[i*3+1];
		m_rig->cabs[m_rig->free_cab*3+2]=m_rig->cabs[i*3+2];
		m_rig->free_cab++;
	}
	//finish it, this is a window
	m_rig->subisback[m_rig->free_sub]=2;
	//close the current mesh
	m_rig->subtexcoords[m_rig->free_sub]=m_rig->free_texcoord;
	m_rig->subcabs[m_rig->free_sub]=m_rig->free_cab;
	//make is transparent
	m_rig->free_sub++;


	//add an extra back mesh
	//texcoords
	start = (m_rig->free_sub==1) ? 0 : m_rig->subtexcoords[m_rig->free_sub-2];
	for (i=start; i<m_rig->subtexcoords[m_rig->free_sub-1]; i++)
	{
		m_rig->texcoords[m_rig->free_texcoord]=m_rig->texcoords[i];;
		m_rig->free_texcoord++;
	}

	//cab
	start = (m_rig->free_sub==1) ? 0 : m_rig->subcabs[m_rig->free_sub-2];
	for (i=start; i<m_rig->subcabs[m_rig->free_sub-1]; i++)
	{
		m_rig->cabs[m_rig->free_cab*3]=m_rig->cabs[i*3+1];
		m_rig->cabs[m_rig->free_cab*3+1]=m_rig->cabs[i*3];
		m_rig->cabs[m_rig->free_cab*3+2]=m_rig->cabs[i*3+2];
		m_rig->free_cab++;
	}
	
	m_rig->subisback[m_rig->free_sub]=1;
}

void RigSpawner::ProcessFlexbody(boost::shared_ptr<RigDef::Flexbody> def)
{
	if (! CheckFlexbodyLimit(1))
	{
		return;
	}

	std::stringstream name;
	name << "flexbody-" << m_rig->truckname << "-" << m_rig->free_flexbody;

	Ogre::Quaternion rot=Ogre::Quaternion(Ogre::Degree(def->rotation.z), Ogre::Vector3::UNIT_Z);
	rot=rot*Ogre::Quaternion(Ogre::Degree(def->rotation.y), Ogre::Vector3::UNIT_Y);
	rot=rot*Ogre::Quaternion(Ogre::Degree(def->rotation.x), Ogre::Vector3::UNIT_X);

	/* Collect nodes */
	std::vector<unsigned int> node_indices;
	node_indices.reserve(100);
	bool nodes_found = CollectNodesFromRanges(def->forset, node_indices);

	if (! nodes_found)
	{
		std::stringstream msg;
		msg << "Failed to collect nodes from node-ranges, skipping flexbody '" << def->mesh_name << "'";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return;
	}

	int reference_node = FindNodeIndex(def->reference_node);
	int x_axis_node = FindNodeIndex(def->x_axis_node);
	int y_axis_node = FindNodeIndex(def->y_axis_node);
	if (reference_node == -1 || x_axis_node == -1 || y_axis_node == -1)
	{
		AddMessage(Message::TYPE_ERROR, "Failed to find required nodes, skipping flexbody '" + def->mesh_name + "'");
	}

	m_rig->flexbodies[m_rig->free_flexbody] = new FlexBody(
		m_rig->nodes, 
		m_rig->free_node, 
		def->mesh_name, 
		name.str(), 
		reference_node,
		x_axis_node,
		y_axis_node,
		def->offset, 
		rot,
		node_indices,
		m_rig->materialFunctionMapper, 
		m_rig->usedSkin, 
		1, 
		m_rig->materialReplacer
		);
	m_rig->free_flexbody++;
}

int RigSpawner::FindNodeIndex_AcceptNonExistentNumbered(RigDef::Node::Id & node_id)
{
	int index = FindNodeIndex(node_id, true);
	if (index != -1)
	{
		return index;
	}

	if (node_id.Str().empty()) /* Is defined by number? */
	{
		std::stringstream msg;
		msg << "Node with number '" << node_id.Num() << "' doesn't exist (searched in definition and generated nodes). "
			<< "Accepting it anyway for backwards compatibility. "
			<< "Please fix as soon as possible.";
		AddMessage(Message::TYPE_WARNING, msg.str());
		return static_cast<int>(node_id.Num());
	}
	else
	{
		std::stringstream msg;
		msg << "Node named '" << node_id.Str() << "' doesn't exist (searched in definition and generated nodes).";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return -1;
	}
}

void RigSpawner::ProcessProp(RigDef::Prop & def)
{
	if (! CheckPropLimit(1))
	{
		return;
	}

	prop_t & prop = m_rig->props[m_rig->free_prop];
	m_rig->free_prop++;
	memset(&prop, 0, sizeof(prop_t)); /* Initialize prop memory to avoid invalid pointers. */

	prop.noderef         = GetNodeIndexOrThrow(def.reference_node);
	prop.nodex           = FindNodeIndex_AcceptNonExistentNumbered(def.x_axis_node);
	prop.nodey           = FindNodeIndex_AcceptNonExistentNumbered(def.y_axis_node);
	if (prop.nodex == -1 || prop.nodey == -1)
	{
		return;
	}
	prop.offsetx         = def.offset.x;
	prop.offsety         = def.offset.y;
	prop.offsetz         = def.offset.z;
	prop.orgoffsetX      = def.offset.x;
	prop.orgoffsetY      = def.offset.y;
	prop.orgoffsetZ      = def.offset.z;
	prop.rotaX           = def.rotation.x;
	prop.rotaY           = def.rotation.y;
	prop.rotaZ           = def.rotation.z;
	prop.rot             = Ogre::Quaternion(Ogre::Degree(def.rotation.z), Ogre::Vector3::UNIT_Z);
	prop.rot             = prop.rot * Ogre::Quaternion(Ogre::Degree(def.rotation.y), Ogre::Vector3::UNIT_Y);
	prop.rot             = prop.rot * Ogre::Quaternion(Ogre::Degree(def.rotation.x), Ogre::Vector3::UNIT_X);
	prop.cameramode      = def.camera_settings.mode; /* Handles default value */
	prop.wheelrotdegree  = 160.f;
	/* Set no animation by default */
	prop.animKey[0]      = -1;
	prop.animKeyState[0] = -1.f;

	/* SPECIAL PROPS */

	/* Rear view mirror (left) */
	if (def.special == RigDef::Prop::SPECIAL_LEFT_REAR_VIEW_MIRROR)
	{
		prop.mirror = 1;
	}

	/* Rear view mirror (right) */
	if (def.special == RigDef::Prop::SPECIAL_RIGHT_REAR_VIEW_MIRROR)
	{
		prop.mirror = -1;
	}

	/* Custom steering wheel */
	Ogre::Vector3 steering_wheel_offset = Ogre::Vector3::ZERO;
	if (def.special == RigDef::Prop::SPECIAL_STEERING_WHEEL_LEFT_HANDED)
	{
		steering_wheel_offset = Ogre::Vector3(-0.67, -0.61,0.24);
	}
	if (def.special == RigDef::Prop::SPECIAL_STEERING_WHEEL_RIGHT_HANDED)
	{
		steering_wheel_offset = Ogre::Vector3(0.67, -0.61,0.24);
	}
	if (steering_wheel_offset != Ogre::Vector3::ZERO)
	{
		if (def.special_prop_steering_wheel._offset_is_set)
		{
			steering_wheel_offset = def.special_prop_steering_wheel.offset;
		}
		prop.wheel = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
		MeshObject *mesh_object = new MeshObject(
			def.special_prop_steering_wheel.mesh_name,
			"",
			prop.wheel,
			m_rig->usedSkin,
			m_enable_background_loading
			);
		mesh_object->setSimpleMaterialColour(Ogre::ColourValue(0, 0.5, 0.5));
		mesh_object->setMaterialFunctionMapper(m_rig->materialFunctionMapper, m_rig->materialReplacer);
	}

	/* CREATE THE PROP */

	prop.snode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
	prop.mo = new MeshObject(def.mesh_name, "", prop.snode, m_rig->usedSkin, m_enable_background_loading);
	prop.mo->setSimpleMaterialColour(Ogre::ColourValue(1, 1, 0));
	prop.mo->setMaterialFunctionMapper(m_rig->materialFunctionMapper, m_rig->materialReplacer);
	prop.mo->setCastShadows(true); // Orig code {{ prop.mo->setCastShadows(shadowmode != 0); }}, shadowmode has default value 1 and changes with undocumented directive 'set_shadows'
	prop.beacontype = 'n'; // Orig: hardcoded in BTS_PROPS

	if (def.special == RigDef::Prop::SPECIAL_SPINPROP)
	{
		prop.spinner = 1;
		prop.mo->setCastShadows(false);
		prop.snode->setVisible(false);
	}
	else if(def.special == RigDef::Prop::SPECIAL_PALE)
	{
		prop.pale = 1;
	}
	else if(def.special == RigDef::Prop::SPECIAL_DRIVER_SEAT)
	{
		m_rig->driversseatfound = true;
		m_rig->driverSeat = & prop;
		prop.mo->setMaterialName("driversseat");
	}
	else if(def.special == RigDef::Prop::SPECIAL_SPINPROP)
	{
		m_rig->driversseatfound = true;
		m_rig->driverSeat = & prop;
	}
	else if (def.special == RigDef::Prop::SPECIAL_STEERING_WHEEL_LEFT_HANDED || def.special == RigDef::Prop::SPECIAL_STEERING_WHEEL_RIGHT_HANDED)
	{
		prop.wheelrotdegree = def.special_prop_steering_wheel.rotation_angle;
	}
	else if (m_rig->flaresMode > 0)
	{
		if(def.special == RigDef::Prop::SPECIAL_BEACON)
		{
			prop.beacontype = 'b';
			prop.bpos[0] = 2.0 * 3.14 * (std::rand() / RAND_MAX);
			prop.brate[0] = 4.0 * 3.14 + (std::rand() / RAND_MAX) - 0.5;
			/* the light */
			prop.light[0] = gEnv->sceneManager->createLight();
			prop.light[0]->setType(Ogre::Light::LT_SPOTLIGHT);
			prop.light[0]->setDiffuseColour(def.special_prop_beacon.color);
			prop.light[0]->setSpecularColour(def.special_prop_beacon.color);
			prop.light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
			prop.light[0]->setSpotlightRange( Ogre::Degree(35), Ogre::Degree(45) );
			prop.light[0]->setCastShadows(false);
			prop.light[0]->setVisible(false);
			/* the flare billboard */
			prop.bbsnode[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
			prop.bbs[0] = gEnv->sceneManager->createBillboardSet(1); //(propname,1);
			prop.bbs[0]->createBillboard(0,0,0);
			if (prop.bbs[0])
			{
				prop.bbs[0]->setMaterialName(def.special_prop_beacon.flare_material_name);
				prop.bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
			}
			prop.bbs[0]->setVisible(false);
			prop.bbsnode[0]->setVisible(false);
		}
		else if(def.special == RigDef::Prop::SPECIAL_REDBEACON)
		{
			prop.brate[0] = 1.0;
			prop.beacontype = 'r';
			//the light
			prop.light[0]=gEnv->sceneManager->createLight();//propname);
			prop.light[0]->setType(Ogre::Light::LT_POINT);
			prop.light[0]->setDiffuseColour( Ogre::ColourValue(1.0, 0.0, 0.0));
			prop.light[0]->setSpecularColour( Ogre::ColourValue(1.0, 0.0, 0.0));
			prop.light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
			prop.light[0]->setCastShadows(false);
			prop.light[0]->setVisible(false);
			//the flare billboard
			prop.bbsnode[0] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
			prop.bbs[0]=gEnv->sceneManager->createBillboardSet(1); //propname,1);
			prop.bbs[0]->createBillboard(0,0,0);
			if (prop.bbs[0])
			{
				prop.bbs[0]->setMaterialName("tracks/redbeaconflare");
				prop.bbs[0]->setVisibilityFlags(DEPTHMAP_DISABLED);
			}
			if (prop.bbs[0])
			{
				prop.bbsnode[0]->attachObject(prop.bbs[0]);
			}
			prop.bbsnode[0]->setVisible(false);
			prop.bbs[0]->setDefaultDimensions(1.0, 1.0);
		}
		else if(def.special == RigDef::Prop::SPECIAL_LIGHTBAR)
		{
			int k;
			m_rig->ispolice = true;
			prop.beacontype='p';
			for (k=0; k<4; k++)
			{
				prop.bpos[k]=2.0*3.14*(std::rand()/RAND_MAX);
				prop.brate[k]=4.0*3.14+(std::rand()/RAND_MAX)-0.5;
				prop.bbs[k]=0;
				//the light
				//char rpname[256];
				//sprintf(rpname,"%s-%i", propname, k);
				prop.light[k]=gEnv->sceneManager->createLight(); //rpname);
				prop.light[k]->setType(Ogre::Light::LT_SPOTLIGHT);
				if (k>1)
				{
					prop.light[k]->setDiffuseColour( Ogre::ColourValue(1.0, 0.0, 0.0));
					prop.light[k]->setSpecularColour( Ogre::ColourValue(1.0, 0.0, 0.0));
				}
				else
				{
					prop.light[k]->setDiffuseColour( Ogre::ColourValue(0.0, 0.5, 1.0));
					prop.light[k]->setSpecularColour( Ogre::ColourValue(0.0, 0.5, 1.0));
				}
				prop.light[k]->setAttenuation(50.0, 1.0, 0.3, 0.0);
				prop.light[k]->setSpotlightRange( Ogre::Degree(35), Ogre::Degree(45) );
				prop.light[k]->setCastShadows(false);
				prop.light[k]->setVisible(false);
				//the flare billboard
				prop.bbsnode[k] = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
				prop.bbs[k]=gEnv->sceneManager->createBillboardSet(1); //rpname,1);
				prop.bbs[k]->createBillboard(0,0,0);
				if (prop.bbs[k])
				{
					if (k>1)
					{
						prop.bbs[k]->setMaterialName("tracks/brightredflare");
					}
					else
					{
						prop.bbs[k]->setMaterialName("tracks/brightblueflare");
					}

					if (prop.bbs[k])
					{
						prop.bbs[k]->setVisibilityFlags(DEPTHMAP_DISABLED);
						prop.bbsnode[k]->attachObject(prop.bbs[k]);
					}
				}
				prop.bbsnode[k]->setVisible(false);
			}
		}
	}	

	/* PROCESS ANIMATIONS */

	if (def.animations.size() > 10)
	{
		std::stringstream msg;
		msg << "Prop (mesh: " << def.mesh_name << ") has too many animations: " << def.animations.size() << " (max. is 10). Using first 10 ...";
		AddMessage(Message::TYPE_WARNING, msg.str());
	}

	std::list<RigDef::Animation>::iterator anim_itor = def.animations.begin();
	int anim_index = 0;
	while (anim_itor != def.animations.end() && anim_index < 10)
	{
		prop.animKeyState[anim_index] = -1.0f; // Orig: hardcoded in {add_animation}

		/* Arg #1: ratio */
		prop.animratio[anim_index] = anim_itor->ratio;
		if (anim_itor->ratio == 0) 
		{
			std::stringstream msg;
			msg << "Prop (mesh: " << def.mesh_name << ") has invalid animation ratio (0), using it anyway (compatibility)...";
			AddMessage(Message::TYPE_WARNING, msg.str());
		}

		/* Arg #2: option1 (lower limit) */
		prop.animOpt1[anim_index] = anim_itor->lower_limit; /* Handles default */

		/* Arg #3: option2 (upper limit) */
		prop.animOpt2[anim_index] = anim_itor->upper_limit; /* Handles default */

		/* Arg #4: source */
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_AIRSPEED)) { /* (NOTE: code formatting relaxed) */
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_AIRSPEED);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_VERTICAL_VELOCITY)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_VVI);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_ALTIMETER_100K)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_ALTIMETER);
			prop.animOpt3[anim_index] = 1.f;
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_ALTIMETER_10K)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_ALTIMETER);
			prop.animOpt3[anim_index] = 2.f;
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_ALTIMETER_1K)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_ALTIMETER);
			prop.animOpt3[anim_index] = 3.f;
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_ANGLE_OF_ATTACK)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_AOA);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_FLAP)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_FLAP);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_AIR_BRAKE)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_AIRBRAKE);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_ROLL)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_ROLL);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_PITCH)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_PITCH);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_BRAKES)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_BRAKE);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_ACCEL)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_ACCEL);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_CLUTCH)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_CLUTCH);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_SPEEDO)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_SPEEDO);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_TACHO)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_TACHO);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_TURBO)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_TURBO);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_PARKING)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_PBRAKE);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_SHIFT_LEFT_RIGHT)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_SHIFTER);
			prop.animOpt3[anim_index] = 1.0f;
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_SHIFT_BACK_FORTH)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_SHIFTER);
			prop.animOpt3[anim_index] = 2.0f;
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_SHIFTERLIN)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_SHIFTER);
			prop.animOpt3[anim_index] = 3.0f;
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_SEQUENTIAL_SHIFT)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_SHIFTER);
			prop.animOpt3[anim_index] = 4.0f;
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_TORQUE)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_TORQUE);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_HEADING)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_HEADING);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_DIFFLOCK)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_DIFFLOCK);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_STEERING_WHEEL)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_STEERING);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_AILERON)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_AILERONS);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_ELEVATOR)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_ELEVATORS);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_AIR_RUDDER)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_ARUDDER);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_BOAT_RUDDER)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_BRUDDER);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_BOAT_THROTTLE)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_BTHROTTLE);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_PERMANENT)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_PERMANENT);
		}
		if (BITMASK_IS_1(anim_itor->source, RigDef::Animation::SOURCE_EVENT)) {
			BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_EVENT);
		}
		/* Motor-indexed sources */
		std::list<RigDef::Animation::MotorSource>::iterator source_itor = anim_itor->motor_sources.begin();
		for ( ; source_itor != anim_itor->motor_sources.end(); source_itor++)
		{
			if (BITMASK_IS_1(source_itor->source, RigDef::Animation::MotorSource::SOURCE_AERO_THROTTLE)) {
				BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_THROTTLE);
				prop.animOpt3[anim_index] = static_cast<float>(source_itor->motor);
			}
			if (BITMASK_IS_1(source_itor->source, RigDef::Animation::MotorSource::SOURCE_AERO_RPM)) {
				BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_RPM);
				prop.animOpt3[anim_index] = static_cast<float>(source_itor->motor);
			}
			if (BITMASK_IS_1(source_itor->source, RigDef::Animation::MotorSource::SOURCE_AERO_TORQUE)) {
				BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_AETORQUE);
				prop.animOpt3[anim_index] = static_cast<float>(source_itor->motor);
			}
			if (BITMASK_IS_1(source_itor->source, RigDef::Animation::MotorSource::SOURCE_AERO_PITCH)) {
				BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_AEPITCH);
				prop.animOpt3[anim_index] = static_cast<float>(source_itor->motor);
			}
			if (BITMASK_IS_1(source_itor->source, RigDef::Animation::MotorSource::SOURCE_AERO_STATUS)) {
				BITMASK_SET_1(prop.animFlags[anim_index], ANIM_FLAG_AESTATUS);
				prop.animOpt3[anim_index] = static_cast<float>(source_itor->motor);
			}
		}
		if (prop.animFlags[anim_index] == 0)
		{
			AddMessage(Message::TYPE_ERROR, "Failed to identify animation source");
		}

		/* Anim modes */
		if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_ROTATION_X)) {
			BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_ROTA_X);
		}
		if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_ROTATION_Y)) {
			BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_ROTA_Y);
		}
		if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_ROTATION_Z)) {
			BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_ROTA_Z);
		}
		if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_OFFSET_X)) {
			BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_OFFSET_X);
		}
		if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_OFFSET_Y)) {
			BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_OFFSET_Y);
		}
		if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_OFFSET_Z)) {
			BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_OFFSET_Z);
		}
		if (prop.animMode[anim_index] == 0)
		{
			AddMessage(Message::TYPE_ERROR, "Failed to identify animation mode");
		}

		if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_AUTO_ANIMATE)) 
		{
			BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_AUTOANIMATE);

			if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_ROTATION_X)) {
				prop.animOpt1[anim_index] = anim_itor->lower_limit + prop.rotaX;
				prop.animOpt2[anim_index] = anim_itor->upper_limit + prop.rotaX;
				prop.animOpt4[anim_index] = prop.rotaX;
			}
			if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_ROTATION_Y)) {
				prop.animOpt1[anim_index] = anim_itor->lower_limit + prop.rotaY;
				prop.animOpt2[anim_index] = anim_itor->upper_limit + prop.rotaY;
				prop.animOpt4[anim_index] = prop.rotaY;
			}
			if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_ROTATION_Z)) {
				prop.animOpt1[anim_index] = anim_itor->lower_limit + prop.rotaZ;
				prop.animOpt2[anim_index] = anim_itor->upper_limit + prop.rotaZ;
				prop.animOpt4[anim_index] = prop.rotaZ;
			}
			if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_OFFSET_X)) {
				prop.animOpt1[anim_index] = anim_itor->lower_limit + prop.orgoffsetX;
				prop.animOpt2[anim_index] = anim_itor->upper_limit + prop.orgoffsetX;
				prop.animOpt4[anim_index] = prop.orgoffsetX;
			}
			if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_OFFSET_Y)) {
				prop.animOpt1[anim_index] = anim_itor->lower_limit + prop.orgoffsetY;
				prop.animOpt2[anim_index] = anim_itor->upper_limit + prop.orgoffsetY;
				prop.animOpt4[anim_index] = prop.orgoffsetY;
			}
			if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_OFFSET_Z)) {
				prop.animOpt1[anim_index] = anim_itor->lower_limit + prop.orgoffsetZ;
				prop.animOpt2[anim_index] = anim_itor->upper_limit + prop.orgoffsetZ;
				prop.animOpt4[anim_index] = prop.orgoffsetZ;
			}
		}
		if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_NO_FLIP)) 
		{
			BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_NOFLIP);
		}
		if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_BOUNCE)) 
		{
			BITMASK_SET_1(prop.animMode[anim_index], ANIM_MODE_BOUNCE);
			prop.animOpt5[anim_index] = 1.f;
		}
		if (BITMASK_IS_1(anim_itor->mode, RigDef::Animation::MODE_EVENT_LOCK)) 
		{
			prop.animKeyState[anim_index] = 0.0f;
			prop.lastanimKS[anim_index] = 0.0f;
		}
		
		/* Parameter 'event:' */

		if (! anim_itor->event.empty())
		{
			// we are using keys as source
			prop.animFlags[anim_index] |= ANIM_FLAG_EVENT;

			int event_id = INPUTENGINE.resolveEventName(anim_itor->event);
			if (event_id == -1)
			{
				AddMessage(Message::TYPE_ERROR, "Unknown animation event: " + anim_itor->event);
			}
			else
			{
				prop.animKey[anim_index] = event_id;
			}
		}

		/* Advance */
		anim_itor++;
		anim_index++;
	}
}

void RigSpawner::ProcessMaterialFlareBinding(RigDef::MaterialFlareBinding & def)
{
	std::stringstream mat_clone_name;
	mat_clone_name << def.material_name << "_mfb_" << m_rig->truckname;
	Ogre::MaterialPtr material = CloneMaterial(def.material_name, mat_clone_name.str());
	if (material.isNull())
	{
		return;
	}
	
	if (m_rig->materialFunctionMapper != nullptr)
	{
		MaterialFunctionMapper::materialmapping_t mapping;
		mapping.originalmaterial = def.material_name;
		mapping.material = mat_clone_name.str();
		mapping.type = 0; // Orig: hardcoded in BTS_MATERIALFLAREBINDINGS

		m_rig->materialFunctionMapper->addMaterial(def.flare_number, mapping);
	}
}

void RigSpawner::ProcessFlare2(RigDef::Flare2 & def)
{
	if (m_rig->flaresMode == 0)
	{
		return;
	}

	int blink_delay = def.blink_delay_milis;
	float size = def.size;

	/* Backwards compatibility */
	if (blink_delay == -2) 
	{
		if (def.type == RigDef::Flare2::TYPE_l_LEFT_BLINKER || def.type == RigDef::Flare2::TYPE_l_LEFT_BLINKER)
		{
			blink_delay = -1; /* Default blink */
		}
		else
		{
			blink_delay = 0; /* Default no blink */
		}
	}
	
	if (size == -2.f && def.type == RigDef::Flare2::TYPE_f_HEADLIGHT)
	{
		size = 1.f;
	}
	else if ((size == -2.f && def.type != RigDef::Flare2::TYPE_f_HEADLIGHT) || size == -1.f)
	{
		size = 0.5f;
	}

	flare_t flare;
	flare.type                 = def.type;
	flare.controlnumber        = def.control_number;
	flare.controltoggle_status = false;
	flare.blinkdelay           = (blink_delay == -1.f) ? 0.5f : blink_delay / 1000.f;
	flare.blinkdelay_curr      = 0.f;
	flare.blinkdelay_state     = false;
	flare.noderef              = GetNodeIndexOrThrow(def.reference_node);
	flare.nodex                = def.x;
	flare.nodey                = def.y;
	flare.offsetx              = def.offset.x;
	flare.offsety              = def.offset.y;
	flare.offsetz              = def.offset.z;
	flare.size                 = size;

	/* Visuals */
	flare.snode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
	std::stringstream flare_name;
	flare_name << "flare-" << m_rig->truckname << "-" << m_rig->free_flare;
	flare.bbs = gEnv->sceneManager->createBillboardSet(flare_name.str(), 1);
	bool using_default_material = true;
	if (flare.bbs == nullptr)
	{
		AddMessage(Message::TYPE_WARNING, "Failed to create flare: '" + flare_name.str() + "', continuing without it (compatibility)...");
	}
	else
	{
		flare.bbs->createBillboard(0,0,0);
		flare.bbs->setVisibilityFlags(DEPTHMAP_DISABLED);
		
		if (def.material_name.length() == 0 || def.material_name == "default")
		{
			if (def.type == RigDef::Flare2::TYPE_b_BRAKELIGHT)
			{
				flare.bbs->setMaterialName("tracks/brakeflare");
			}
			else if (def.type == RigDef::Flare2::TYPE_l_LEFT_BLINKER || (def.type == RigDef::Flare2::TYPE_r_RIGHT_BLINKER))
			{
				flare.bbs->setMaterialName("tracks/blinkflare");
			}
			else
			{
				flare.bbs->setMaterialName("tracks/flare");
			}
		}
		else
		{
			using_default_material = false;
			flare.bbs->setMaterialName(def.material_name);
		}
		flare.snode->attachObject(flare.bbs);
	}
	flare.isVisible = true;
	flare.light = nullptr;

	if (m_rig->flaresMode >= 2 && size > 0.001)
	{
		//if (type == 'f' && usingDefaultMaterial && flaresMode >=2 && size > 0.001)
		if (def.type == RigDef::Flare2::TYPE_f_HEADLIGHT && using_default_material )
		{
			/* front light */
			flare.light=gEnv->sceneManager->createLight(flare_name.str());
			flare.light->setType(Ogre::Light::LT_SPOTLIGHT);
			flare.light->setDiffuseColour( Ogre::ColourValue(1, 1, 1));
			flare.light->setSpecularColour( Ogre::ColourValue(1, 1, 1));
			flare.light->setAttenuation(400, 0.9, 0, 0);
			flare.light->setSpotlightRange( Ogre::Degree(35), Ogre::Degree(45) );
			flare.light->setCastShadows(false);
		}
	}
	if (m_rig->flaresMode >= 4 && size > 0.001)
	{
		//else if (type == 'f' && !usingDefaultMaterial && flaresMode >=4 && size > 0.001)
		if (def.type == RigDef::Flare2::TYPE_f_HEADLIGHT && ! using_default_material)
		{
			/* this is a quick fix for the red backlight when frontlight is switched on */
			flare.light=gEnv->sceneManager->createLight(flare_name.str());
			flare.light->setDiffuseColour( Ogre::ColourValue(1.0, 0, 0));
			flare.light->setSpecularColour( Ogre::ColourValue(1.0, 0, 0));
			flare.light->setAttenuation(10.0, 1.0, 0, 0);
		}
		//else if (type == 'R' && flaresMode >= 4 && size > 0.001)
		else if (def.type == RigDef::Flare2::TYPE_R_REVERSE_LIGHT)
		{
			flare.light=gEnv->sceneManager->createLight(flare_name.str());
			flare.light->setDiffuseColour(Ogre::ColourValue(1, 1, 1));
			flare.light->setSpecularColour(Ogre::ColourValue(1, 1, 1));
			flare.light->setAttenuation(20.0, 1, 0, 0);
		}
		//else if (type == 'b' && flaresMode >= 4 && size > 0.001)
		else if (def.type == RigDef::Flare2::TYPE_b_BRAKELIGHT)
		{
			flare.light=gEnv->sceneManager->createLight(flare_name.str());
			flare.light->setDiffuseColour( Ogre::ColourValue(1.0, 0, 0));
			flare.light->setSpecularColour( Ogre::ColourValue(1.0, 0, 0));
			flare.light->setAttenuation(10.0, 1.0, 0, 0);
		}
		//else if ((type == 'l' || type == 'r') && flaresMode >= 4 && size > 0.001)
		else if (def.type == RigDef::Flare2::TYPE_l_LEFT_BLINKER || (def.type == RigDef::Flare2::TYPE_r_RIGHT_BLINKER))
		{
			flare.light=gEnv->sceneManager->createLight(flare_name.str());
			flare.light->setDiffuseColour( Ogre::ColourValue(1, 1, 0));
			flare.light->setSpecularColour( Ogre::ColourValue(1, 1, 0));
			flare.light->setAttenuation(10.0, 1, 1, 0);
		}
		//else if ((type == 'u') && flaresMode >= 4 && size > 0.001)
		else if (def.type == RigDef::Flare2::TYPE_u_USER)
		{
			/* user light always white (TODO: improve this) */
			flare.light=gEnv->sceneManager->createLight(flare_name.str());
			flare.light->setDiffuseColour( Ogre::ColourValue(1, 1, 1));
			flare.light->setSpecularColour( Ogre::ColourValue(1, 1, 1));
			flare.light->setAttenuation(50.0, 1.0, 1, 0.2);

			if (m_rig->netCustomLightArray_counter < 4)
			{
				m_rig->netCustomLightArray[m_rig->netCustomLightArray_counter] = m_rig->free_flare;
				m_rig->netCustomLightArray_counter++;
			}
		}
	}

	/* Finalize light */
	if (flare.light != nullptr)
	{
		flare.light->setType(Ogre::Light::LT_SPOTLIGHT);
		flare.light->setSpotlightRange( Ogre::Degree(35), Ogre::Degree(45) );
		flare.light->setCastShadows(false);
	}
	m_rig->flares.push_back(flare);
	m_rig->free_flare++;
}

Ogre::MaterialPtr RigSpawner::CloneMaterial(Ogre::String const & source_name, Ogre::String const & clone_name)
{
	Ogre::MaterialPtr src_mat = static_cast<Ogre::MaterialPtr>(Ogre::MaterialManager::getSingleton().getByName(source_name));
	if (src_mat.isNull())
	{
		std::stringstream msg;
		msg << "Built-in material '" << source_name << "' missing! Skipping...";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return Ogre::MaterialPtr(nullptr);
	}
	return src_mat->clone(clone_name);
}

void RigSpawner::ProcessManagedMaterial(RigDef::ManagedMaterial & def)
{
	Ogre::MaterialPtr test = static_cast<Ogre::MaterialPtr>(Ogre::MaterialManager::getSingleton().getByName(def.name));
	if (! test.isNull())
	{
		std::stringstream msg;
		msg << "Managed material '" << def.name << "' already exists (probably because the vehicle was already spawned before)";
		AddMessage(Message::TYPE_WARNING, msg.str());
	}

	Ogre::MaterialPtr material;
	if (def.type == RigDef::ManagedMaterial::TYPE_FLEXMESH_STANDARD || def.type == RigDef::ManagedMaterial::TYPE_FLEXMESH_TRANSPARENT)
	{
		Ogre::String mat_name_base 
			= (def.type == RigDef::ManagedMaterial::TYPE_FLEXMESH_STANDARD) 
			? "managed/flexmesh_standard" 
			: "managed/flexmesh_transparent";

		if (def.HasDamagedDiffuseMap())
		{
			if (def.HasSpecularMap())
			{
				/* FLEXMESH, damage, specular */
				material = CloneMaterial(mat_name_base + "/speculardamage", def.name);
				if (material.isNull())
				{
					return;
				}
				material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(def.diffuse_map);
				material->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(def.specular_map);
				material->getTechnique(0)->getPass(0)->getTextureUnitState(2)->setTextureName(def.damaged_diffuse_map);
				material->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(def.specular_map);
			}
			else
			{
				/* FLEXMESH, damage, no_specular */
				material = CloneMaterial(mat_name_base + "/damageonly", def.name);
				if (material.isNull())
				{
					return;
				}
				material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(def.diffuse_map);
				material->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(def.damaged_diffuse_map);				
			}
		}
		else
		{
			if (def.HasSpecularMap())
			{
				/* FLEXMESH, no_damage, specular */
				material = CloneMaterial(mat_name_base + "/specularonly", def.name);
				if (material.isNull())
				{
					return;
				}
				material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(def.diffuse_map);
				material->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(def.specular_map);
				material->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(def.specular_map);
			}
			else
			{
				/* FLEXMESH, no_damage, no_specular */
				material = CloneMaterial(mat_name_base + "/simple", def.name);
				if (material.isNull())
				{
					return;
				}
				material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(def.diffuse_map);				
			}
		}
	}
	else if (def.type == RigDef::ManagedMaterial::TYPE_MESH_STANDARD || def.type == RigDef::ManagedMaterial::TYPE_MESH_TRANSPARENT)
	{
		Ogre::String mat_name_base 
			= (def.type == RigDef::ManagedMaterial::TYPE_MESH_STANDARD) 
			? "managed/mesh_standard" 
			: "managed/mesh_transparent";

		if (def.HasSpecularMap())
		{
			/* MESH, specular */
			material = CloneMaterial(mat_name_base + "/specular", def.name);
			if (material.isNull())
			{
				return;
			}
			material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(def.diffuse_map);
			material->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(def.specular_map);
			material->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(def.specular_map);
		}
		else
		{
			/* MESH, no_specular */
			material = CloneMaterial(mat_name_base + "/simple", def.name);
			if (material.isNull())
			{
				return;
			}
			material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(def.diffuse_map);				
		}
	}

	/* Finalize */
	if (def.options.double_sided)
	{
		material->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
		if (def.HasSpecularMap())
		{
			material->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
		}
	}
	material->compile();
}

void RigSpawner::ProcessCollisionBox(RigDef::CollisionBox & def)
{
	std::vector<RigDef::Node::Id>::iterator itor = def.nodes.begin();
	for ( ; itor != def.nodes.end(); itor++)
	{
		std::pair<unsigned int, bool> node_result = GetNodeIndex(*itor);
		if (! node_result.second)
		{
			std::stringstream msg;
			msg << "Invalid node '" << itor->ToString() << "'";
			continue;
		}
		m_rig->nodes[node_result.first].collisionBoundingBoxID = m_rig->collisionBoundingBoxes.size();
	}

	m_rig->collisionBoundingBoxes.resize(m_rig->collisionBoundingBoxes.size() + 1);
}

bool RigSpawner::AssignWheelToAxle(int & _out_axle_wheel, node_t *axis_node_1, node_t *axis_node_2)
{
	for (int i = 0; i < m_rig->free_wheel; i++)
	{
		wheel_t & wheel = m_rig->wheels[i];
		if	(	(wheel.refnode0 == axis_node_1 && wheel.refnode1 == axis_node_2)
			||	(wheel.refnode0 == axis_node_2 && wheel.refnode1 == axis_node_1)
			)
		{
			_out_axle_wheel = i;
			return true;
		}
	}
	return false;
}

void RigSpawner::ProcessAxle(RigDef::Axle & def)
{
	if (! CheckAxleLimit(1))
	{
		return;
	}

	node_t *wheel_1_node_1 = GetNodePointerOrThrow(def.wheels[0][0]);
	node_t *wheel_1_node_2 = GetNodePointerOrThrow(def.wheels[0][1]);
	node_t *wheel_2_node_1 = GetNodePointerOrThrow(def.wheels[1][0]);
	node_t *wheel_2_node_2 = GetNodePointerOrThrow(def.wheels[1][1]);
	Axle *axle = new Axle();

	if (! AssignWheelToAxle(axle->wheel_1, wheel_1_node_1, wheel_1_node_2))
	{
		std::stringstream msg;
		msg << "Couldn't find wheel with axis nodes '" << def.wheels[0][0].ToString()
			<< "' and '" << def.wheels[0][1].ToString() << "'";
		AddMessage(Message::TYPE_WARNING, msg.str());
	}

	if (! AssignWheelToAxle(axle->wheel_2, wheel_2_node_1, wheel_2_node_2))
	{
		std::stringstream msg;
		msg << "Couldn't find wheel with axis nodes '" << def.wheels[1][0].ToString()
			<< "' and '" << def.wheels[1][1].ToString() << "'";
		AddMessage(Message::TYPE_WARNING, msg.str());
	}

	if (def.options == 0)
	{
		AddMessage(Message::TYPE_INFO, "No differential defined, defaulting to Open & Locked");
		axle->addDiffType(OPEN_DIFF);
		axle->addDiffType(LOCKED_DIFF);
	}
	else
	{
		if (BITMASK_IS_1(def.options, RigDef::Axle::OPTION_s_SPLIT))
		{
			axle->addDiffType(SPLIT_DIFF);
		}
		if (BITMASK_IS_1(def.options, RigDef::Axle::OPTION_l_LOCKED))
		{
			axle->addDiffType(LOCKED_DIFF);
		}
		if (BITMASK_IS_1(def.options, RigDef::Axle::OPTION_o_OPEN))
		{
			axle->addDiffType(OPEN_DIFF);
		}
	}

	m_rig->axles[m_rig->free_axle] = axle;
	m_rig->free_axle++;
}

void RigSpawner::ProcessSpeedLimiter(RigDef::SpeedLimiter & def)
{
	m_rig->sl_enabled = true;
	m_rig->sl_speed_limit = def.max_speed;
	if (def.max_speed <= 0.f)
	{
		std::stringstream msg;
		msg << "Invalid parameter 'max_speed' (" << def.max_speed 
			<< ") must be positive nonzero number. Using it anyway (compatibility)";
	}
}

void RigSpawner::ProcessCruiseControl(RigDef::CruiseControl & def)
{
	m_rig->cc_target_speed_lower_limit = def.min_speed;
	if (m_rig->cc_target_speed_lower_limit <= 0.f)
	{
		std::stringstream msg;
		msg << "Invalid parameter 'lower_limit' (" << m_rig->cc_target_speed_lower_limit 
			<< ") must be positive nonzero number. Using it anyway (compatibility)";
	}
	m_rig->cc_can_brake = def.autobrake != 0;
}

void RigSpawner::ProcessTorqueCurve(RigDef::TorqueCurve & def)
{
	if (m_rig->engine == nullptr)
	{
		AddMessage(Message::TYPE_WARNING, "Section 'torquecurve' found but no 'engine' defined, skipping...");
		return;
	}

	TorqueCurve *target_torque_curve = m_rig->engine->getTorqueCurve();

	if (def.predefined_func_name.length() != 0)
	{
		target_torque_curve->setTorqueModel(def.predefined_func_name);
	}
	else
	{
		target_torque_curve->CreateNewCurve(); /* Use default name for custom curve */
		std::vector<RigDef::TorqueCurve::Sample>::iterator itor = def.samples.begin();
		for ( ; itor != def.samples.end(); itor++)
		{
			target_torque_curve->AddCurveSample(itor->power, itor->torque_percent);
		}
	}
}

void RigSpawner::ProcessParticle(RigDef::Particle & def)
{
	if (! m_rig->cparticle_enabled)
	{
		return;
	}

	unsigned int particle_index = m_rig->free_cparticle;
	m_rig->free_cparticle++;
	cparticle_t & particle = m_rig->cparticles[particle_index];

	particle.emitterNode = GetNodeIndexOrThrow(def.emitter_node);
	particle.directionNode = GetNodeIndexOrThrow(def.reference_node);

	/* Setup visuals */
	std::stringstream name;
	name << "cparticle-" << particle_index << "-" << m_rig->truckname;
	particle.snode = m_parent_scene_node->createChildSceneNode();
	particle.psys = gEnv->sceneManager->createParticleSystem(name.str(), def.particle_system_name);
	if (particle.psys == nullptr)
	{
		std::stringstream msg;
		msg << "Failed to create particle system '" << def.particle_system_name << "'";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return;
	}
	particle.psys->setVisibilityFlags(DEPTHMAP_DISABLED);
	particle.snode->attachObject(particle.psys);
	particle.snode->setPosition(GetNode(particle.emitterNode).AbsPosition);
	
	/* Shut down the emitters */
	particle.active = false; 
	for (unsigned int i = 0; i < particle.psys->getNumEmitters(); i++)
	{
		particle.psys->getEmitter(i)->setEnabled(false);
	}
}

void RigSpawner::ProcessRopable(RigDef::Ropable & def)
{
	ropable_t ropable;
	ropable.node = GetNodePointerOrThrow(def.node);
	ropable.group = def.group;
	ropable.used = 0; // Hardcoded in BTS_ROPABLES
	ropable.multilock = def.multilock;
	m_rig->ropables.push_back(ropable);

	ropable.node->iIsSkin = true;
}

void RigSpawner::ProcessTie(RigDef::Tie & def)
{
	if (! CheckBeamLimit(1))
	{
		return;
	}

	node_t & node_1 = GetNodeOrThrow(def.root_node);
	node_t & node_2 = GetNode( (node_1.pos == 0) ? 1 : 0 );

	int beam_index = m_rig->free_beam;
	beam_t & beam = AddBeam(node_1, node_2, def.beam_defaults, def.detacher_group);
	SetBeamStrength(beam, def.beam_defaults->GetScaledBreakingThreshold());
	CreateBeamVisuals(beam, beam_index, *def.beam_defaults, false);
	beam.k = def.beam_defaults->GetScaledSpringiness();
	beam.d = def.beam_defaults->GetScaledDamping();
	beam.type = (def.options == RigDef::Tie::OPTIONS_INVISIBLE) ? BEAM_INVISIBLE_HYDRO : BEAM_HYDRO;
	beam.L = def.max_reach_length;
	beam.refL = def.max_reach_length;
	beam.Lhydro = def.max_reach_length;
	beam.bounded = ROPE;
	beam.disabled = true;
	beam.commandRatioLong = def.auto_shorten_rate;
	beam.commandRatioShort = def.auto_shorten_rate;
	beam.commandShort = def.min_length;
	beam.commandLong = def.max_length;
	beam.maxtiestress = def.max_stress;

	/* Register tie */
	tie_t tie;
	tie.group = def.group;
	tie.tying = false;
	tie.tied = false;
	tie.beam = & beam;
	m_rig->ties.push_back(tie);

	m_rig->hascommands = 1;
}

void RigSpawner::ProcessRope(RigDef::Rope & def)
{
	if (! CheckBeamLimit(1))
	{
		return;
	}

	node_t & root_node = GetNodeOrThrow(def.root_node);
	node_t & end_node = GetNodeOrThrow(def.end_node);

	/* Add beam */
	beam_t & beam = AddBeam(root_node, end_node, def.beam_defaults, def.detacher_group);
	SetBeamStrength(beam, def.beam_defaults->GetScaledBreakingThreshold());
	beam.k = def.beam_defaults->GetScaledSpringiness();
	beam.d = def.beam_defaults->GetScaledDamping();
	beam.bounded = ROPE;
	beam.type = (def.invisible) ? BEAM_INVISIBLE_HYDRO : BEAM_HYDRO;

	/* Register rope */
	rope_t rope;
	rope.beam = & beam;
	rope.lockedto = & m_rig->nodes[0]; // Orig: hardcoded in BTS_ROPES
	rope.group = 0; // Orig: hardcoded in BTS_ROPES. TODO: To be used.
	m_rig->ropes.push_back(rope);

	root_node.iIsSkin = true;
	end_node.iIsSkin = true;
}

void RigSpawner::ProcessRailGroup(RigDef::RailGroup & def)
{
	Rail *rail = CreateRail(def.node_list);
	if (rail == nullptr)
	{
		return;
	}
	RailGroup *rail_group = new RailGroup(rail, def.id);
	m_rig->mRailGroups.push_back(rail_group);
}

void RigSpawner::ProcessSlidenode(RigDef::SlideNode & def)
{
	node_t & node = GetNodeOrThrow(def.slide_node);
	SlideNode slide_node(& node, nullptr);
	slide_node.setThreshold(def.tolerance);
	slide_node.setSpringRate(def.spring_rate);
	slide_node.setAttachmentRate(def.spring_rate);
	if (def._break_force_set)
	{
		slide_node.setBreakForce(def.break_force);
	}
	slide_node.setAttachmentDistance(def.max_attachment_distance);

	/* Constraints */
	if (BITMASK_IS_1(def.constraint_flags, RigDef::SlideNode::CONSTRAINT_ATTACH_ALL))
	{
		slide_node.setAttachRule( ATTACH_ALL );
	}
	if (BITMASK_IS_1(def.constraint_flags, RigDef::SlideNode::CONSTRAINT_ATTACH_SELF))
	{
		slide_node.setAttachRule( ATTACH_SELF );
	}
	if (BITMASK_IS_1(def.constraint_flags, RigDef::SlideNode::CONSTRAINT_ATTACH_FOREIGN))
	{
		slide_node.setAttachRule( ATTACH_FOREIGN );
	}
	if (BITMASK_IS_1(def.constraint_flags, RigDef::SlideNode::CONSTRAINT_ATTACH_NONE))
	{
		slide_node.setAttachRule( ATTACH_NONE );
	}

	/* RailGroup */
	RailGroup *rail_group = nullptr;
	if (def._railgroup_id_set)
	{
		std::vector<RailGroup*>::iterator itor = m_rig->mRailGroups.begin();
		for ( ; itor != m_rig->mRailGroups.end(); itor++)
		{
			if ((*itor)->getID() == def.railgroup_id)
			{
				rail_group = *itor;
				break;
			}
		}

		if (rail_group == nullptr)
		{
			std::stringstream msg;
			msg << "Specified rail group id '" << def.railgroup_id << "' not found. Ignoring slidenode...";
			AddMessage(Message::TYPE_ERROR, msg.str());
			return;
		}
	}
	else if (def.rail_node_ranges.size() > 0)
	{
		Rail *rail = CreateRail(def.rail_node_ranges);
		if (rail == nullptr)
		{
			return;
		}
		rail_group = new RailGroup(rail);
		m_rig->mRailGroups.push_back(rail_group);
	}
	else
	{
		AddMessage(Message::TYPE_ERROR, "No RailGroup available for SlideNode, skipping...");
	}

	slide_node.setDefaultRail(rail_group);
	m_rig->mSlideNodes.push_back(slide_node);
}

node_t* RigSpawner::FindGeneratedNodeInRig(RigDef::Node::Id & id, bool silent /* Default = false */)
{
	/* Only work with number-indexed nodes */
	if (id.Str().empty() && id.Num() < static_cast<unsigned int>(m_rig->free_node))
	{
		if (m_rig->nodes[id.Num()].id == -1) /* Make sure the node was generated */
		{
			return & m_rig->nodes[id.Num()];				
		}
		else
		{
			std::stringstream msg;
			msg << "Cannot use undefined node '" << id.ToString() << "'";
			AddMessage(Message::TYPE_ERROR, msg.str());
			return nullptr;
		}
	}
	else
	{
		if (! silent)
		{
			std::stringstream msg;
			msg << "Attempt to use non-existent node '" << id.ToString() << "'";
			AddMessage(Message::TYPE_ERROR, msg.str());
		}
		return nullptr;
	}
}

int RigSpawner::FindNodeIndex(RigDef::Node::Id & id, bool silent /* Default: false */)
{
	std::pair<unsigned int, bool> result = GetNodeIndex(id, /* quiet */ true);
	if (result.second)
	{
		return static_cast<int>(result.first);
	}
	else
	{
		node_t *node = FindGeneratedNodeInRig(id, silent);
		if (node == nullptr)
		{
			if (! silent)
			{
				std::stringstream msg;
				msg << "Failed to find node '" << id.ToString() << "', no such node defined in source file or dynamically generated during spawn.";
				AddMessage(Message::TYPE_ERROR, msg.str());
			}
			return -1; /* Node not found */
		}
		return node->pos;
	}
}

bool RigSpawner::CollectNodesFromRanges(
	std::vector<RigDef::Node::Range> & node_ranges,
	std::vector<unsigned int> & out_node_indices
	)
{
	std::vector<RigDef::Node::Range>::iterator itor = node_ranges.begin();
	for ( ; itor != node_ranges.end(); itor++)
	{
		if (itor->IsRange())
		{

			int result_a = FindNodeIndex(itor->start, /* silent */ false);
			int result_b = FindNodeIndex(itor->end,   /* silent */ true);

			unsigned int start = 0;
			unsigned int end = 0;

			if (result_a == -1)
			{
				return false;
			}
			else
			{
				start = static_cast<unsigned int>(result_a);
			}

			if (result_b == -1)
			{
				std::stringstream msg;
				msg << "Encountered non-existent node '" << itor->end.ToString() << "' in range [" << itor->start.ToString() << " - " << itor->end.ToString() << "], "
					<< "highest node index is '" << m_rig->free_node - 1 << "'.";

				if (itor->end.Str().empty()) /* If the node is numeric... */
				{
					msg << " However, this node must be accepted anyway for backwards compatibility."
						<< " Please fix this as soon as possible.";
					end = itor->end.Num();
					AddMessage(Message::TYPE_ERROR, msg.str());
				}
				else
				{
					AddMessage(Message::TYPE_ERROR, msg.str());
					return false;
				}
			}
			else
			{
				end = static_cast<unsigned int>(result_b);
			}

			if (end < start)
			{
				unsigned int swap = start;
				start = end;
				end = swap;
			}

			for (unsigned int i = start; i <= end; i++)
			{
				out_node_indices.push_back(i);
			}
		}
		else
		{
			out_node_indices.push_back(GetNodeIndexOrThrow(itor->start));
		}
	}
	return true;
}

Rail *RigSpawner::CreateRail(std::vector<RigDef::Node::Range> & node_ranges)
{
	/* Collect nodes */
	std::vector<unsigned int> node_indices;
	node_indices.reserve(100);

	CollectNodesFromRanges(node_ranges, node_indices);

	/* Find beams &build rail */
	RailBuilder rail_builder; /* rail builder allocates the memory for each rail, it will not free it */
	if (node_indices.front() == node_indices.back())
	{
		rail_builder.loopRail();
	}

	for (unsigned int i = 0; i < node_indices.size() - 1; i++)
	{
		beam_t *beam = FindBeamInRig(node_indices[i], node_indices[i + 1]);
		if (beam == nullptr)
		{
			std::stringstream msg;
			msg << "No beam between nodes indexed '" << node_indices[i] << "' and '" << node_indices[i + 1] << "'";
			AddMessage(Message::TYPE_ERROR, msg.str());
			return nullptr;
		}
		rail_builder.pushBack(beam);
	}

	return rail_builder.getCompletedRail(); /* Transfers memory ownership */
}

beam_t *RigSpawner::FindBeamInRig(unsigned int node_a_index, unsigned int node_b_index)
{
	node_t *node_a = & m_rig->nodes[node_a_index];
	node_t *node_b = & m_rig->nodes[node_b_index];

	for (unsigned int i = 0; i < static_cast<unsigned int>(m_rig->free_beam); i++)
	{
		if	(
				(GetBeam(i).p1 == node_a && GetBeam(i).p2 == node_b)
			||	(GetBeam(i).p2 == node_a && GetBeam(i).p1 == node_b)
			)
		{
			return & GetBeam(i);
		}
	}
	return nullptr;
}

void RigSpawner::ProcessHook(RigDef::Hook & def)
{
	/* Find the node */
	node_t *node = GetNodePointer(def.node);
	if (node ==  nullptr)
	{
		return;
	}
	
	/* Find the hook */
	hook_t *hook = nullptr;
	std::vector <hook_t>::iterator itor = m_rig->hooks.begin();
	for (; itor != m_rig->hooks.end(); itor++)
	{
		if (itor->hookNode == node)
		{
			hook = &*itor;
			break;
		}
	}

	if (hook == nullptr)
	{
		std::stringstream msg;
		msg << "Node '" << def.node.ToString() << "' is not a hook-node (not marked with flag 'h'), ignoring...";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return;
	}

	/* Process options */
	hook->lockrange = def.option_hook_range;
	hook->lockspeed = def.option_speed_coef * HOOK_SPEED_DEFAULT;
	hook->maxforce  = def.option_max_force;
	hook->group     = def.option_hookgroup;
	hook->lockgroup = def.option_lockgroup;
	hook->timer     = 0.f; // Hardcoded in BTS_HOOKS
	hook->timer_preset = def.option_timer;
	hook->beam->commandShort = def.option_minimum_range_meters;
	hook->selflock = BITMASK_IS_1(def.flags, RigDef::Hook::FLAG_SELF_LOCK);
	hook->nodisable = BITMASK_IS_1(def.flags, RigDef::Hook::FLAG_NO_DISABLE);
	if (BITMASK_IS_1(def.flags, RigDef::Hook::FLAG_AUTO_LOCK))
	{
		hook->autolock = true;
		if (hook->group == -1)
		{
			hook->group = -2; /* only overwrite hgroup when its still default (-1) */
		}
	}
	if (BITMASK_IS_1(def.flags, RigDef::Hook::FLAG_NO_ROPE))
	{
		hook->beam->bounded = NOSHOCK;
	}
	if (BITMASK_IS_1(def.flags, RigDef::Hook::FLAG_VISIBLE))
	{
		hook->visible = true;
		if (hook->beam->mSceneNode->numAttachedObjects() == 0)
		{
			hook->beam->mSceneNode->attachObject(hook->beam->mEntity);
		}
	}
}

void RigSpawner::ProcessLockgroup(RigDef::Lockgroup & lockgroup)
{
	std::vector<RigDef::Node::Id>::iterator itor = lockgroup.nodes.begin();
	for (; itor != lockgroup.nodes.end(); itor++)
	{
		GetNodeOrThrow(*itor).lockgroup = lockgroup.number;
	}
}

void RigSpawner::ProcessTrigger(RigDef::Trigger & def)
{
	if (! CheckBeamLimit(1) || ! CheckShockLimit(1))
	{
		return;
	}

	shock_t shock;
	/* Disable trigger on startup? (default enabled) */
	shock.trigger_enabled = ! BITMASK_IS_1(def.options, RigDef::Trigger::OPTION_x_START_OFF);

	unsigned int hydro_type = BEAM_HYDRO;
	unsigned int shock_flags = SHOCK_FLAG_NORMAL | SHOCK_FLAG_ISTRIGGER;
	/* Set the CommandKeys that are set in a commandkeyblocker or trigger to blocked on startup, default is released */
	bool cmd_key_block = BITMASK_IS_1(def.options, RigDef::Trigger::OPTION_b_BLOCK_KEYS);
	bool hook_toggle = false;
	bool trigger_blocker = false;
	bool inverted_trigger_blocker = false;

	unsigned int shortbound_trigger_key = def.shortbound_trigger_key;
	unsigned int longbound_trigger_key = def.longbound_trigger_key;

	m_rig->commandkey[shortbound_trigger_key].trigger_cmdkeyblock_state = false;
	if (longbound_trigger_key != -1)
	{
		m_rig->commandkey[longbound_trigger_key].trigger_cmdkeyblock_state = false;
	}

	/* Options */
	if (BITMASK_IS_1(def.options, RigDef::Trigger::OPTION_i_INVISIBLE))
	{
		hydro_type = BEAM_INVISIBLE_HYDRO;
		BITMASK_SET_1(shock_flags, SHOCK_FLAG_INVISIBLE);
	}
	if (BITMASK_IS_1(def.options, RigDef::Trigger::OPTION_B_BLOCK_TRIGGERS))
	{
		/* Blocker that enable/disable other triggers */
		trigger_blocker = true;
		BITMASK_SET_1(shock_flags, SHOCK_FLAG_TRG_BLOCKER);
	}
	if (BITMASK_IS_1(def.options, RigDef::Trigger::OPTION_s_SWITCH_CMD_NUM))
	{
		/* switch that exchanges cmdshort/cmdshort for all triggers with the same commandnumbers, default false */
		BITMASK_SET_1(shock_flags, SHOCK_FLAG_TRG_CMD_SWITCH);
	}
	if (BITMASK_IS_1(def.options, RigDef::Trigger::OPTION_c_COMMAND_STYLE))
	{
		/* trigger is set with commandstyle boundaries instead of shocksytle */
		shortbound_trigger_key = std::abs(static_cast<int>(shortbound_trigger_key) - 1);
		longbound_trigger_key -= 1;
	}
	if (BITMASK_IS_1(def.options, RigDef::Trigger::OPTION_A_INV_BLOCK_TRIGGERS))
	{
		/* Blocker that enable/disable other triggers, reversed activation method (inverted Blocker style, auto-ON) */
		inverted_trigger_blocker = true;
		BITMASK_SET_1(shock_flags, SHOCK_FLAG_TRG_BLOCKER_A);
	}
	if (BITMASK_IS_1(def.options, RigDef::Trigger::OPTION_h_UNLOCK_HOOKGROUPS_KEY))
	{
		hook_toggle = true;
		BITMASK_SET_1(shock_flags, SHOCK_FLAG_TRG_HOOK_UNLOCK);
	}
	if (BITMASK_IS_1(def.options, RigDef::Trigger::OPTION_H_LOCK_HOOKGROUPS_KEY))
	{
		hook_toggle = true;
		BITMASK_SET_1(shock_flags, SHOCK_FLAG_TRG_HOOK_LOCK);
	}
	if (BITMASK_IS_1(def.options, RigDef::Trigger::OPTION_t_CONTINUOUS))
	{
		/* this trigger sends values between 0 and 1 */
		hook_toggle = true;
		BITMASK_SET_1(shock_flags, SHOCK_FLAG_TRG_CONTINUOUS);
	}
	if (BITMASK_IS_1(def.options, RigDef::Trigger::OPTION_t_CONTINUOUS))
	{
		/* this trigger sends values between 0 and 1 */
		hook_toggle = true;
		BITMASK_SET_1(shock_flags, SHOCK_FLAG_TRG_CONTINUOUS);
	}

	bool is_engine_trigger = BITMASK_IS_1(def.options, RigDef::Trigger::OPTION_E_ENGINE_TRIGGER);

	if	(	! trigger_blocker 
		&&	! inverted_trigger_blocker
		&&	! hook_toggle 
		&&	! is_engine_trigger
		)
	{
		// make the full check
		if (shortbound_trigger_key < 1 || shortbound_trigger_key > MAX_COMMANDS)
		{
			std::stringstream msg;
			msg << "Invalid value of 'shortbound trigger key': '" << shortbound_trigger_key << "'. Must be between 1 and 84. Ignoring trigger...";
			AddMessage(Message::TYPE_ERROR, msg.str());
			return;
		}
	} 
	else if (! hook_toggle && ! is_engine_trigger)
	{
		// this is a Trigger-Blocker, make special check
		if (shortbound_trigger_key < 0 || longbound_trigger_key < 0)
		{
			AddMessage(Message::TYPE_ERROR, "Wrong command-eventnumber (Triggers). Trigger-Blocker deactivated.");
			return;
		}
	} 
	else if (is_engine_trigger)
	{
		if (trigger_blocker || inverted_trigger_blocker || hook_toggle || (shock_flags & SHOCK_FLAG_TRG_CMD_SWITCH))
		{
			AddMessage(Message::TYPE_ERROR, "Wrong command-eventnumber (Triggers). Engine trigger deactivated.");
			return;
		}
	}

	int node_1_index = FindNodeIndex_AcceptNonExistentNumbered(def.nodes[0]);
	int node_2_index = FindNodeIndex_AcceptNonExistentNumbered(def.nodes[1]);
	if (node_1_index == -1 || node_2_index == -1 )
	{
		return;
	}
	int beam_index = m_rig->free_beam;
	beam_t & beam = AddBeam(GetNode(node_1_index), GetNode(node_2_index), def.beam_defaults, def.detacher_group);
	beam.type = hydro_type;
	SetBeamStrength(beam, def.beam_defaults->breaking_threshold_constant);
	SetBeamSpring(beam, 0.f);
	SetBeamDamping(beam, 0.f);
	CalculateBeamLength(beam);
	beam.shortbound = def.contraction_trigger_limit;
	beam.longbound = def.expansion_trigger_limit;
	beam.bounded = SHOCK2;

	CreateBeamVisuals(beam, beam_index, hydro_type != BEAM_INVISIBLE_HYDRO);

	if (m_rig->triggerdebug)
	{
		LOG("Trigger added. BeamID " + TOSTRING(beam_index));
	}

	shock.beamid = beam_index;
	shock.trigger_switch_state = 0.0f;   // used as bool and countdowntimer, dont touch!
	if (!trigger_blocker && !inverted_trigger_blocker) // this is no trigger_blocker (A/B)
	{
		shock.trigger_cmdshort = shortbound_trigger_key;
		if (longbound_trigger_key != -1 || (longbound_trigger_key == -1 && hook_toggle))
		{
			// this is a trigger or a hook_toggle
			shock.trigger_cmdlong = longbound_trigger_key;
		}
		else
		{
			// this is a commandkeyblocker
			shock_flags |= SHOCK_FLAG_TRG_CMD_BLOCKER;
		}
	} 
	else 
	{
		// this is a trigger_blocker
		if (!inverted_trigger_blocker)
		{
			//normal BLOCKER
			shock_flags |= SHOCK_FLAG_TRG_BLOCKER;
			shock.trigger_cmdshort = shortbound_trigger_key;
			shock.trigger_cmdlong  = longbound_trigger_key;
		} 
		else
		{
			//inverted BLOCKER
			shock_flags |= SHOCK_FLAG_TRG_BLOCKER_A;
			shock.trigger_cmdshort = shortbound_trigger_key;
			shock.trigger_cmdlong  = longbound_trigger_key;
		}
	}

	if (cmd_key_block && !trigger_blocker)
	{
		m_rig->commandkey[shortbound_trigger_key].trigger_cmdkeyblock_state = true;
		if (longbound_trigger_key != -1)
		{
			m_rig->commandkey[longbound_trigger_key].trigger_cmdkeyblock_state = true;
		}
	}
	
	shock.trigger_boundary_t = def.boundary_timer;
	shock.flags              = shock_flags;
	shock.sbd_spring         = def.beam_defaults->springiness;
	shock.sbd_damp           = def.beam_defaults->damping_constant;
	shock.last_debug_state   = 0;

	/* Commit data to <rig_t> */
	
	m_rig->shocks[m_rig->free_shock] = shock;
	beam.shock = & m_rig->shocks[m_rig->free_shock];
	m_rig->free_shock++;
}

void RigSpawner::ProcessContacter(RigDef::Node::Id & node)
{
	unsigned int node_index = GetNodeIndexOrThrow(node);
	m_rig->contacters[m_rig->free_contacter].nodeid = node_index;
	GetNode(node_index).iIsSkin = true;
	m_rig->free_contacter++;
};

void RigSpawner::ProcessRotator(RigDef::Rotator & def)
{
	if (! CheckRotatorLimit(1))
	{
		return;
	}

	unsigned int rotator_index = m_rig->free_rotator;
	m_rig->free_rotator++;
	rotator_t & rotator = m_rig->rotators[rotator_index];

	rotator.angle = 0;
	rotator.rate = def.rate;
	rotator.axis1 = GetNodeIndexOrThrow(def.axis_nodes[0]);
	rotator.axis2     = GetNodeIndexOrThrow(def.axis_nodes[1]);
	rotator.force     = ROTATOR_FORCE_DEFAULT;
	rotator.tolerance = ROTATOR_TOLERANCE_DEFAULT;
	rotator.rotatorEngineCoupling = def.engine_coupling;
	rotator.rotatorNeedsEngine = def.needs_engine;
	for (unsigned int i = 0; i < 4; i++)
	{
		rotator.nodes1[i] = GetNodeIndexOrThrow(def.base_plate_nodes[i]);
		rotator.nodes2[i] = GetNodeIndexOrThrow(def.rotating_plate_nodes[i]);
	}

	/* Rotate left key */
	m_rig->commandkey[def.spin_left_key].rotators.push_back(- (static_cast<int>(rotator_index) + 1));
	m_rig->commandkey[def.spin_left_key].description = "Rotate_Left/Right";

	/* Rotate right key */
	m_rig->commandkey[def.spin_right_key].rotators.push_back(rotator_index + 1);

	_ProcessCommandKeyInertia(def.inertia, *def.inertia_defaults, def.spin_left_key, def.spin_right_key);

	m_rig->hascommands = 1;
}

void RigSpawner::ProcessRotator2(RigDef::Rotator2 & def)
{
	if (! CheckRotatorLimit(1))
	{
		return;
	}

	unsigned int rotator_index = m_rig->free_rotator;
	m_rig->free_rotator++;
	rotator_t & rotator = m_rig->rotators[rotator_index];

	rotator.angle = 0;
	rotator.rate = def.rate;
	rotator.axis1 = GetNodeIndexOrThrow(def.axis_nodes[0]);
	rotator.axis2     = GetNodeIndexOrThrow(def.axis_nodes[1]);
	rotator.force     = def.rotating_force; /* Default value is set in constructor */
	rotator.tolerance = def.tolerance; /* Default value is set in constructor */
	rotator.rotatorEngineCoupling = def.engine_coupling;
	rotator.rotatorNeedsEngine = def.needs_engine;
	for (unsigned int i = 0; i < 4; i++)
	{
		rotator.nodes1[i] = GetNodeIndexOrThrow(def.base_plate_nodes[i]);
		rotator.nodes2[i] = GetNodeIndexOrThrow(def.rotating_plate_nodes[i]);
	}

	/* Rotate left key */
	m_rig->commandkey[def.spin_left_key].rotators.push_back(- (static_cast<int>(rotator_index) + 1));
	if (! def.description.empty())
	{
		m_rig->commandkey[def.spin_left_key].description = def.description;
	}
	else
	{
		m_rig->commandkey[def.spin_left_key].description = "Rotate_Left/Right";
	}

	/* Rotate right key */
	m_rig->commandkey[def.spin_right_key].rotators.push_back(rotator_index + 1);

	_ProcessCommandKeyInertia(def.inertia, *def.inertia_defaults, def.spin_left_key, def.spin_right_key);

	m_rig->hascommands = 1;
}

void RigSpawner::_ProcessCommandKeyInertia(
	RigDef::OptionalInertia & inertia, 
	RigDef::DefaultInertia & inertia_defaults, 
	int contract_key, 
	int extend_key
)
{
	if (m_rig->cmdInertia != nullptr)
	{
		/* Handle placeholders */
		Ogre::String start_function;
		Ogre::String stop_function;
		if (! inertia.start_function.empty() && inertia.start_function != "/" && inertia.start_function != "-")
		{
			start_function = inertia.start_function;
		}
		if (! inertia.stop_function.empty() && inertia.stop_function != "/" && inertia.stop_function != "-")
		{
			stop_function = inertia.stop_function;
		}
		if (inertia._start_delay_factor_set && inertia._stop_delay_factor_set)
		{
			m_rig->cmdInertia->setCmdKeyDelay(
				contract_key,
				inertia.start_delay_factor,
				inertia.stop_delay_factor,
				start_function,
				stop_function
			);

			m_rig->cmdInertia->setCmdKeyDelay(
				extend_key,
				inertia.start_delay_factor,
				inertia.stop_delay_factor,
				start_function,
				stop_function
			);
		}
		else if (inertia._start_delay_factor_set || inertia._stop_delay_factor_set)
		{
			m_rig->cmdInertia->setCmdKeyDelay(
				contract_key,
				inertia_defaults.start_delay_factor,
				inertia_defaults.stop_delay_factor,
				inertia_defaults.start_function,
				inertia_defaults.stop_function
			);

			m_rig->cmdInertia->setCmdKeyDelay(
				extend_key,
				inertia_defaults.start_delay_factor,
				inertia_defaults.stop_delay_factor,
				inertia_defaults.start_function,
				inertia_defaults.stop_function
			);
		}
	}
}

void RigSpawner::ProcessCommand(RigDef::Command2 & def)
{
	if (! CheckBeamLimit(1))
	{
		return;
	}

	int beam_index = m_rig->free_beam;
	int node_1_index = FindNodeIndex(def.nodes[0]);
	int node_2_index = FindNodeIndex(def.nodes[1]);
	if (node_1_index == -1 || node_2_index == -1)
	{
		AddMessage(Message::TYPE_ERROR, "Failed to fetch node");
		return;
	}
	beam_t & beam = AddBeam(m_rig->nodes[node_1_index], m_rig->nodes[node_2_index], def.beam_defaults, def.detacher_group);
	CalculateBeamLength(beam);
	SetBeamStrength(beam, def.beam_defaults->GetScaledBreakingThreshold()); /* Override settings from AddBeam() */
	SetBeamSpring(beam, def.beam_defaults->GetScaledSpringiness());
	SetBeamDamping(beam, def.beam_defaults->GetScaledDamping());
	beam.type = BEAM_HYDRO;

	/* Options */
	if (BITMASK_IS_1(def.options, RigDef::Command2::OPTION_i_INVISIBLE)) {
		beam.type = BEAM_INVISIBLE_HYDRO;
	}
	if (BITMASK_IS_1(def.options, RigDef::Command2::OPTION_r_ROPE)) {
		beam.bounded = ROPE;
	}
	if (BITMASK_IS_1(def.options, RigDef::Command2::OPTION_p_PRESS_ONCE)) {
		beam.isOnePressMode = 1;
	}
	if (BITMASK_IS_1(def.options, RigDef::Command2::OPTION_o_PRESS_ONCE_CENTER)) {
		beam.isOnePressMode = 2;
	}
	if (BITMASK_IS_1(def.options, RigDef::Command2::OPTION_f_NOT_FASTER)) {
		beam.isForceRestricted = true;
	}
	if (BITMASK_IS_1(def.options, RigDef::Command2::OPTION_c_AUTO_CENTER)) {
		beam.isCentering = true;
	}

	beam.commandRatioShort     = def.shorten_rate;
	beam.commandRatioLong      = def.lengthen_rate;
	beam.commandShort          = def.max_contraction;
	beam.commandLong           = def.max_extension;
	beam.commandEngineCoupling = def.affect_engine;
	beam.commandNeedsEngine    = def.needs_engine;

	/* set the middle of the command, so its not required to recalculate this everytime ... */
	if (def.max_extension > def.max_contraction)
	{
		beam.centerLength = (def.max_extension - def.max_contraction) / 2 + def.max_contraction;
	}
	else
	{
		beam.centerLength = (def.max_contraction - def.max_extension) / 2 + def.max_extension;
	}

	_ProcessCommandKeyInertia(def.inertia, *def.inertia_defaults, def.contract_key, def.extend_key);	

	/* Add keys */
	m_rig->commandkey[def.contract_key].beams.push_back(-beam_index);
	m_rig->commandkey[def.contract_key].description = def.description;

	m_rig->commandkey[def.extend_key].beams.push_back(beam_index);
	m_rig->commandkey[def.extend_key].description = def.description;

	bool attach_to_scene = (beam.type != BEAM_VIRTUAL && beam.type != BEAM_INVISIBLE && beam.type != BEAM_INVISIBLE_HYDRO);
	CreateBeamVisuals(beam, beam_index, attach_to_scene);

	m_rig->free_commands++;
	m_rig->hascommands = 1;
}

void RigSpawner::ProcessAnimator(RigDef::Animator & def)
{
	if (! CheckBeamLimit(1) || ! CheckHydroLimit(1))
	{
		return;
	}

	if (m_rig->hydroInertia != nullptr)
	{
		if (def.inertia_defaults->start_delay_factor > 0 && def.inertia_defaults->stop_delay_factor > 0)
		{
			m_rig->hydroInertia->setCmdKeyDelay(
				m_rig->free_hydro,
				def.inertia_defaults->start_delay_factor,
				def.inertia_defaults->stop_delay_factor,
				def.inertia_defaults->start_function,
				def.inertia_defaults->stop_function
			);
		}
	}

	unsigned int hydro_type = BEAM_HYDRO;
	unsigned int anim_flags = 0;
	float anim_option = 0;

	/* Options. '{' intentionally misplaced. */

	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_VISIBLE)) {
		hydro_type = BEAM_HYDRO;
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_INVISIBLE)) {
		hydro_type = BEAM_INVISIBLE_HYDRO;
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_AIRSPEED)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_AIRSPEED);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_VERTICAL_VELOCITY)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_VVI);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_ANGLE_OF_ATTACK)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_AOA);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_FLAP)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_FLAP);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_AIR_BRAKE)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_AIRBRAKE);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_ROLL))	{
		BITMASK_SET_1(anim_flags, ANIM_FLAG_ROLL);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_PITCH)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_PITCH);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_BRAKES)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_BRAKE);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_ACCEL)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_ACCEL);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_CLUTCH)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_CLUTCH);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_SPEEDO)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_SPEEDO);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_TACHO)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_TACHO);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_TURBO)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_TURBO);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_PARKING)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_PBRAKE);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_TORQUE)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_TORQUE);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_BOAT_THROTTLE)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_BTHROTTLE);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_BOAT_RUDDER)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_BRUDDER);
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_SHIFT_LEFT_RIGHT)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_SHIFTER);
		anim_option = 1.f;
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_SHIFT_BACK_FORTH))	{
		BITMASK_SET_1(anim_flags, ANIM_FLAG_SHIFTER);
		anim_option = 2.f;
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_SEQUENTIAL_SHIFT)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_SHIFTER);
		anim_option = 3.f;
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_GEAR_SELECT)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_SHIFTER);
		anim_option = 4.f;
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_ALTIMETER_100K)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_ALTIMETER);
		anim_option = 1.f;
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_ALTIMETER_10K)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_ALTIMETER);
		anim_option = 2.f;
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_ALTIMETER_1K)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_ALTIMETER);
		anim_option = 3.f;
	}
	
	/* Aerial */
	if (BITMASK_IS_1(def.aero_animator.flags, RigDef::AeroAnimator::OPTION_THROTTLE)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_THROTTLE);
		anim_option = static_cast<float>(def.aero_animator.motor);
	}
	if (BITMASK_IS_1(def.aero_animator.flags, RigDef::AeroAnimator::OPTION_RPM)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_RPM);
		anim_option = static_cast<float>(def.aero_animator.motor);
	}
	if (BITMASK_IS_1(def.aero_animator.flags, RigDef::AeroAnimator::OPTION_TORQUE)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_AETORQUE);
		anim_option = static_cast<float>(def.aero_animator.motor);
	}
	if (BITMASK_IS_1(def.aero_animator.flags, RigDef::AeroAnimator::OPTION_PITCH)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_AEPITCH);
		anim_option = static_cast<float>(def.aero_animator.motor);
	}
	if (BITMASK_IS_1(def.aero_animator.flags, RigDef::AeroAnimator::OPTION_STATUS)) {
		BITMASK_SET_1(anim_flags, ANIM_FLAG_AESTATUS);
		anim_option = static_cast<float>(def.aero_animator.motor);
	}

	unsigned int beam_index = m_rig->free_beam;
	beam_t & beam = AddBeam(GetNode(def.nodes[0]), GetNode(def.nodes[1]), def.beam_defaults, def.detacher_group);
	/* set the limits to something with sense by default */
	beam.shortbound = 0.99999f;
	beam.longbound = 1000000.0f;
	beam.type = hydro_type;
	beam.hydroRatio = def.lenghtening_factor;
	CalculateBeamLength(beam);
	SetBeamStrength(beam, def.beam_defaults->GetScaledBreakingThreshold());
	SetBeamSpring(beam, def.beam_defaults->GetScaledSpringiness());
	SetBeamDamping(beam, def.beam_defaults->GetScaledDamping());
	CreateBeamVisuals(beam, beam_index, hydro_type != BEAM_INVISIBLE_HYDRO);

	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_SHORT_LIMIT)) 
	{
		beam.shortbound = def.short_limit;
	}
	if (BITMASK_IS_1(def.flags, RigDef::Animator::OPTION_LONG_LIMIT)) 
	{
		beam.longbound = def.long_limit;
	}

	m_rig->hydro[m_rig->free_hydro] = beam_index;
	m_rig->free_hydro++;
}

beam_t & RigSpawner::AddBeam(
	node_t & node_1, 
	node_t & node_2, 
	boost::shared_ptr<RigDef::BeamDefaults> & beam_defaults,
	int detacher_group
)
{
	/* Init */
	beam_t & beam = GetAndInitFreeBeam(node_1, node_2);
	beam.detacher_group = detacher_group;
	beam.diameter = beam_defaults->visual_beam_diameter;
	beam.disabled = false;

	/* Breaking threshold (strength) */
	float strength = beam_defaults->breaking_threshold_constant;
	beam.strength = strength;
	beam.iStrength = strength;

	/* Deformation */
	SetBeamDeformationThreshold(beam, beam_defaults);

	float plastic_coef = beam_defaults->plastic_deformation_coefficient;
	beam.plastic_coef = plastic_coef;
	beam.default_plastic_coef = plastic_coef;

	return beam;
}

void RigSpawner::SetBeamStrength(beam_t & beam, float strength)
{
	beam.strength = strength;
	beam.iStrength = strength;
}

void RigSpawner::ProcessHydro(RigDef::Hydro & def)
{
	if (! CheckHydroLimit(1) || ! CheckBeamLimit(1))
	{
		return;
	}

	unsigned int hydro_type = BEAM_HYDRO;
	unsigned int hydro_flags = HYDRO_FLAG_DIR;

	/* Options */
	if (BITMASK_IS_1(def.options, RigDef::Hydro::OPTION_i_INVISIBLE))
	{
		hydro_type = BEAM_INVISIBLE_HYDRO;
	}
	if (BITMASK_IS_1(def.options, RigDef::Hydro::OPTION_s_DISABLE_ON_HIGH_SPEED))
	{
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_SPEED);
	}
	if (BITMASK_IS_1(def.options, RigDef::Hydro::OPTION_a_INPUT_AILERON))
	{
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_AILERON);
	}
	if (BITMASK_IS_1(def.options, RigDef::Hydro::OPTION_r_INPUT_RUDDER))
	{
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_RUDDER);
	}
	if (BITMASK_IS_1(def.options, RigDef::Hydro::OPTION_e_INPUT_ELEVATOR))
	{
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_ELEVATOR);
	}
	if (BITMASK_IS_1(def.options, RigDef::Hydro::OPTION_u_INPUT_AILERON_ELEVATOR))
	{
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_AILERON);
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_ELEVATOR);
	}
	if (BITMASK_IS_1(def.options, RigDef::Hydro::OPTION_v_INPUT_InvAILERON_ELEVATOR))
	{
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_REV_AILERON);
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_ELEVATOR);
	}
	if (BITMASK_IS_1(def.options, RigDef::Hydro::OPTION_x_INPUT_AILERON_RUDDER))
	{
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_AILERON);
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_RUDDER);
	}
	if (BITMASK_IS_1(def.options, RigDef::Hydro::OPTION_y_INPUT_InvAILERON_RUDDER))
	{
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_REV_AILERON);
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_RUDDER);
	}
	if (BITMASK_IS_1(def.options, RigDef::Hydro::OPTION_g_INPUT_ELEVATOR_RUDDER))
	{
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_ELEVATOR);
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_RUDDER);
	}
	if (BITMASK_IS_1(def.options, RigDef::Hydro::OPTION_h_INPUT_InvELEVATOR_RUDDER))
	{
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_REV_ELEVATOR);
		BITMASK_SET_1(hydro_flags, HYDRO_FLAG_RUDDER);
	}

	/* Inertia */
	if (m_rig->hydroInertia != nullptr)
	{
		if (def.inertia._start_delay_factor_set && def.inertia._stop_delay_factor_set)
		{
			m_rig->hydroInertia->setCmdKeyDelay(
				m_rig->free_hydro,	
				def.inertia_defaults->start_delay_factor,
				def.inertia_defaults->stop_delay_factor,
				def.inertia_defaults->start_function,
				def.inertia_defaults->stop_function
			);
		}
		else if (def.inertia._start_delay_factor_set || def.inertia._stop_delay_factor_set)
		{
			m_rig->hydroInertia->setCmdKeyDelay(
				m_rig->free_hydro,	
				def.inertia_defaults->start_delay_factor,
				def.inertia_defaults->stop_delay_factor,
				def.inertia_defaults->start_function,
				def.inertia_defaults->stop_function
			);
		}
	}

	int beam_index = m_rig->free_beam;
	beam_t & beam = GetAndInitFreeBeam(GetNode(def.nodes[0]), GetNode(def.nodes[1]));
	SetBeamStrength(beam, def.beam_defaults->GetScaledBreakingThreshold());
	CalculateBeamLength(beam);
	SetBeamDeformationThreshold(beam, def.beam_defaults);
	beam.type                 = hydro_type;
	beam.k                    = def.beam_defaults->GetScaledSpringiness();
	beam.d                    = def.beam_defaults->GetScaledDamping();
	beam.detacher_group       = def.detacher_group;
	beam.hydroFlags           = hydro_flags;
	beam.hydroRatio           = def.lenghtening_factor;
	beam.plastic_coef         = def.beam_defaults->plastic_deformation_coefficient;
	beam.default_plastic_coef = def.beam_defaults->plastic_deformation_coefficient;

	CreateBeamVisuals(beam, beam_index, *def.beam_defaults, BITMASK_IS_0(def.options, RigDef::Hydro::OPTION_i_INVISIBLE));

	m_rig->hydro[m_rig->free_hydro] = beam_index;
	m_rig->free_hydro++;
}

void RigSpawner::ProcessShock2(RigDef::Shock2 & def)
{
	if (! CheckShockLimit(1) || ! CheckBeamLimit(1))
	{
		return;
	}

	node_t & node_1 = GetNode(def.nodes[0]);
	node_t & node_2 = GetNode(def.nodes[1]);
	float short_bound = def.short_bound;
	float long_bound = def.long_bound;
	unsigned int hydro_type = BEAM_HYDRO;
	unsigned int shock_flags = SHOCK_FLAG_NORMAL | SHOCK_FLAG_ISSHOCK2;

	if (BITMASK_IS_1(def.options, RigDef::Shock2::OPTION_i_INVISIBLE))
	{
		BITMASK_SET_1(shock_flags, SHOCK_FLAG_INVISIBLE);
		hydro_type = BEAM_INVISIBLE_HYDRO;
	}
	if (BITMASK_IS_1(def.options, RigDef::Shock2::OPTION_s_SOFT_BUMP_BOUNDS))
	{
		BITMASK_SET_0(shock_flags, SHOCK_FLAG_NORMAL); /* Not normal anymore */
		BITMASK_SET_1(shock_flags, SHOCK_FLAG_SOFTBUMP);
	}
	if (BITMASK_IS_1(def.options, RigDef::Shock2::OPTION_m_METRIC))
	{
		float beam_length = node_1.AbsPosition.distance(node_2.AbsPosition);
		short_bound /= beam_length;
		long_bound /= beam_length;
	}
	if (BITMASK_IS_1(def.options, RigDef::Shock2::OPTION_M_ABSOLUTE_METRIC))
	{
		float beam_length = node_1.AbsPosition.distance(node_2.AbsPosition);
		short_bound = (beam_length - short_bound) / beam_length;
		long_bound = (long_bound - beam_length) / beam_length;

		if (long_bound < 0.f)
		{
			AddMessage(
				Message::TYPE_WARNING, 
				"Metric shock length calculation failed, 'short_bound' less than beams spawn length. Resetting to beam's spawn length (short_bound = 0)"
			);
			long_bound = 0.f;
		}

		if (short_bound > 1.f)
		{
			AddMessage(
				Message::TYPE_WARNING, 
				"Metric shock length calculation failed, 'short_bound' less than 0 meters. Resetting to 0 meters (short_bound = 1)"
			);
			short_bound = 1.f;
		}
	}
	
	int beam_index = m_rig->free_beam;
	beam_t & beam = GetAndInitFreeBeam(node_1, node_2);
	SetBeamStrength(beam, def.beam_defaults->breaking_threshold_constant * 4.f);
	SetBeamDeformationThreshold(beam, def.beam_defaults);
	beam.type                 = hydro_type;
	beam.bounded              = SHOCK2;
	beam.k                    = def.spring_in;
	beam.d                    = def.damp_in;
	beam.shortbound           = short_bound;
	beam.longbound            = long_bound;
	beam.plastic_coef         = def.beam_defaults->plastic_deformation_coefficient;
	beam.default_plastic_coef = def.beam_defaults->plastic_deformation_coefficient;

	/* Length + pre-compression */
	CalculateBeamLength(beam);
	beam.L          *= def.precompression;
	beam.refL       *= def.precompression;
	beam.Lhydro     *= def.precompression;

	CreateBeamVisuals(beam, beam_index, hydro_type != BEAM_INVISIBLE_HYDRO);

	shock_t & shock  = GetFreeShock();
	shock.flags      = shock_flags;
	shock.sbd_spring = def.beam_defaults->springiness;
	shock.sbd_damp   = def.beam_defaults->damping_constant;
	shock.springin   = def.spring_in;
	shock.dampin     = def.damp_in;
	shock.springout  = def.spring_out;
	shock.dampout    = def.damp_out;
	shock.sprogin    = def.progress_factor_spring_in;
	shock.dprogin    = def.progress_factor_damp_in;
	shock.sprogout   = def.progress_factor_spring_out;
	shock.dprogout   = def.progress_factor_damp_out;

	beam.shock = & shock;
	shock.beamid = beam_index;
}

void RigSpawner::ProcessShock(RigDef::Shock & def)
{
	if (! CheckShockLimit(1) || ! CheckBeamLimit(1))
	{
		return;
	}

	node_t & node_1 = GetNode(def.nodes[0]);
	node_t & node_2 = GetNode(def.nodes[1]);
	float short_bound = def.short_bound;
	float long_bound = def.long_bound;
	unsigned int hydro_type = BEAM_HYDRO;
	unsigned int shock_flags = SHOCK_FLAG_NORMAL;

	if (BITMASK_IS_1(def.options, RigDef::Shock::OPTION_i_INVISIBLE))
	{
		BITMASK_SET_1(shock_flags, SHOCK_FLAG_INVISIBLE);
		hydro_type = BEAM_INVISIBLE_HYDRO;
	}
	if (BITMASK_IS_1(def.options, RigDef::Shock::OPTION_L_ACTIVE_LEFT))
	{
		BITMASK_SET_0(shock_flags, SHOCK_FLAG_NORMAL); /* Not normal anymore */
		BITMASK_SET_1(shock_flags, SHOCK_FLAG_LACTIVE);
		m_rig->free_active_shock++; /* This has no array associated with it. its just to determine if there are active shocks! */
	}
	if (BITMASK_IS_1(def.options, RigDef::Shock::OPTION_L_ACTIVE_LEFT))
	{
		BITMASK_SET_0(shock_flags, SHOCK_FLAG_NORMAL); /* Not normal anymore */
		BITMASK_SET_1(shock_flags, SHOCK_FLAG_RACTIVE);
		m_rig->free_active_shock++; /* This has no array associated with it. its just to determine if there are active shocks! */
	}
	if (BITMASK_IS_1(def.options, RigDef::Shock::OPTION_m_METRIC))
	{
		float beam_length = node_1.AbsPosition.distance(node_2.AbsPosition);
		short_bound /= beam_length;
		long_bound /= beam_length;
	}
	
	int beam_index = m_rig->free_beam;
	beam_t & beam = AddBeam(node_1, node_2, def.beam_defaults, DEFAULT_DETACHER_GROUP);
	beam.shortbound = short_bound;
	beam.longbound  = long_bound;
	beam.bounded    = SHOCK1;
	beam.type       = hydro_type;
	beam.k          = def.spring_rate;
	beam.d          = def.damping;
	SetBeamStrength(beam, def.beam_defaults->breaking_threshold_constant * 4.f);

	/* Length + pre-compression */
	CalculateBeamLength(beam);
	beam.L          *= def.precompression;
	beam.refL       *= def.precompression;
	beam.Lhydro     *= def.precompression;

	shock_t & shock  = GetFreeShock();
	shock.flags      = shock_flags;
	shock.sbd_spring = def.beam_defaults->springiness;
	shock.sbd_damp   = def.beam_defaults->damping_constant;

	/* Create beam visuals, but don't attach them to scene graph */
	/* Old parser did it like this, I don't know why ~ only_a_ptr 13-04-14 */
	CreateBeamVisuals(beam, beam_index, false);

	beam.shock = & shock;
	shock.beamid = beam_index;
}

void RigSpawner::FetchAxisNodes(
	node_t* & axis_node_1, 
	node_t* & axis_node_2, 
	RigDef::Node::Id axis_node_1_id,
	RigDef::Node::Id axis_node_2_id
)
{
	axis_node_1 = GetNodePointer(axis_node_1_id);
	axis_node_2 = GetNodePointer(axis_node_2_id);

	/* Enforce the "second node must have a larger Z coordinate than the first" constraint */
	if (axis_node_1->RelPosition.z > axis_node_2->RelPosition.z)
	{
		node_t *swap = axis_node_1;
		axis_node_1 = axis_node_2;
		axis_node_2 = swap;
	}	
}

void RigSpawner::ProcessFlexBodyWheel(RigDef::FlexBodyWheel & def)
{	
	/* Check capacities */
	CheckNodeLimit(def.num_rays * 4);
	CheckBeamLimit(def.num_rays * (def.rigidity_node.IsValid()) ? 26 : 25);
	CheckFlexbodyLimit(1);

	unsigned int base_node_index = m_rig->free_node;
	wheel_t & wheel = m_rig->wheels[m_rig->free_wheel];

	node_t *axis_node_1 = nullptr;
	node_t *axis_node_2 = nullptr;
	FetchAxisNodes(axis_node_1, axis_node_2, def.nodes[0], def.nodes[1]);

	/* Rigidity node */
	node_t *rigidity_node = nullptr;
	node_t *axis_node_closest_to_rigidity_node = nullptr;
	if (def.rigidity_node.IsValid())
	{
		rigidity_node = GetNodePointer(def.rigidity_node);
		Ogre::Real distance_1 = (rigidity_node->RelPosition - axis_node_1->RelPosition).length();
		Ogre::Real distance_2 = (rigidity_node->RelPosition - axis_node_2->RelPosition).length();
		axis_node_closest_to_rigidity_node = ((distance_1 < distance_2)) ? axis_node_1 : axis_node_2;
	}

	/* Node&beam generation */
	Ogre::Vector3 axis_vector = axis_node_2->RelPosition - axis_node_1->RelPosition;
	axis_vector.normalise();
	Ogre::Vector3 rim_ray_vector = axis_vector.perpendicular() * def.rim_radius;
	Ogre::Quaternion rim_ray_rotator = Ogre::Quaternion(Ogre::Degree(-360.f / (def.num_rays * 2)), axis_vector);

	wheel.width = axis_vector.length(); /* wheel_def.width is ignored. */

	/* Rim nodes */
	/* NOTE: node.iswheel is not used for rim nodes */
	for (unsigned int i = 0; i < def.num_rays; i++)
	{
		float node_mass = def.mass / (4.f * def.num_rays);

		/* Outer ring */
		Ogre::Vector3 ray_point = axis_node_1->RelPosition + rim_ray_vector;
		rim_ray_vector = rim_ray_rotator * rim_ray_vector;

		node_t & outer_node      = GetFreeNode();
		InitNode(outer_node, ray_point, def.node_defaults);

		outer_node.mass          = node_mass;
		outer_node.id            = -1; // Orig: hardcoded (addWheel2)
		outer_node.wheelid       = m_rig->free_wheel;
		outer_node.friction_coef = def.node_defaults->friction;

		/* Inner ring */
		ray_point = axis_node_2->RelPosition + rim_ray_vector;
		rim_ray_vector = rim_ray_rotator * rim_ray_vector;

		node_t & inner_node      = GetFreeNode();
		InitNode(inner_node, ray_point, def.node_defaults);

		inner_node.mass          = node_mass;
		inner_node.id            = -1; // Orig: hardcoded (addWheel2)
		inner_node.wheelid       = m_rig->free_wheel;
		inner_node.friction_coef = def.node_defaults->friction;

		/* Wheel object */
		wheel.nodes[i * 2]       = & outer_node;
		wheel.nodes[(i * 2) + 1] = & inner_node;
	}

	Ogre::Vector3 tyre_ray_vector = axis_vector.perpendicular() * def.tyre_radius;
	Ogre::Quaternion& tyre_ray_rotator = rim_ray_rotator;
	tyre_ray_vector = tyre_ray_rotator * tyre_ray_vector;

	/* Tyre nodes */
	for (unsigned int i = 0; i < def.num_rays; i++)
	{
		/* Outer ring */
		Ogre::Vector3 ray_point = axis_node_1->RelPosition + tyre_ray_vector;
		tyre_ray_vector = tyre_ray_rotator * tyre_ray_vector;

		node_t & outer_node = GetFreeNode();
		InitNode(outer_node, ray_point);
		outer_node.mass          = (0.67f * def.mass) / (2.f * def.num_rays);
		outer_node.id            = -1; // Orig: hardcoded (addWheel2)
		outer_node.wheelid       = m_rig->free_wheel;
		outer_node.friction_coef = def.node_defaults->friction;
		outer_node.volume_coef   = def.node_defaults->volume;
		outer_node.surface_coef  = def.node_defaults->surface;
		outer_node.iswheel       = m_rig->free_wheel*2+1;

		contacter_t & outer_contacter = m_rig->contacters[m_rig->free_contacter];
		outer_contacter.nodeid        = outer_node.pos; /* Node index */
		outer_contacter.contacted     = 0;
		outer_contacter.opticontact   = 0;
		m_rig->free_contacter++;

		/* Inner ring */
		ray_point = axis_node_2->RelPosition + tyre_ray_vector;
		tyre_ray_vector = tyre_ray_rotator * tyre_ray_vector;

		node_t & inner_node = GetFreeNode();
		InitNode(inner_node, ray_point);
		inner_node.mass          = (0.33f * def.mass) / (2.f * def.num_rays);
		inner_node.id            = -1; // Orig: hardcoded (addWheel2)
		inner_node.wheelid       = m_rig->free_wheel;
		inner_node.friction_coef = def.node_defaults->friction;
		inner_node.volume_coef   = def.node_defaults->volume;
		inner_node.surface_coef  = def.node_defaults->surface;
		inner_node.iswheel       = m_rig->free_wheel*2+2;

		contacter_t & inner_contacter = m_rig->contacters[m_rig->free_contacter];
		inner_contacter.nodeid        = inner_node.pos; /* Node index */
		inner_contacter.contacted     = 0;
		inner_contacter.opticontact   = 0;
		m_rig->free_contacter++;

		/* Wheel object */
		wheel.nodes[i * 2] = & outer_node;
		wheel.nodes[(i * 2) + 1] = & inner_node;
	}

	/* Beams */
	float strength = def.beam_defaults->breaking_threshold_constant;
	float rim_spring = def.rim_springiness;
	float rim_damp = def.rim_damping;
	float tyre_spring = def.tyre_springiness;
	float tyre_damp = def.tyre_damping;
	float tread_spring = def.beam_defaults->springiness;
	float tread_damp = def.beam_defaults->damping_constant;
	/* 
		Calculate the point where the support beams get stiff and prevent the tire tread nodes 
		bounce into the rim rimradius / tire radius and add 5%, this is a shortbound calc in % ! 
	*/
	float support_beams_short_bound = 1.0f - ((def.rim_radius / def.tyre_radius) * 0.95f);

	for (unsigned int i = 0; i < def.num_rays; i++)
	{
		/* --- Rim ---  */

		/* Rim axis to rim ring */
		unsigned int rim_outer_node_index = base_node_index + (i * 2);
		node_t *rim_outer_node = & m_rig->nodes[rim_outer_node_index];
		node_t *rim_inner_node = & m_rig->nodes[rim_outer_node_index + 1];

		AddWheelBeam(axis_node_1, rim_outer_node, rim_spring, rim_damp, def.beam_defaults);
		AddWheelBeam(axis_node_2, rim_inner_node, rim_spring, rim_damp, def.beam_defaults);
		AddWheelBeam(axis_node_2, rim_outer_node, rim_spring, rim_damp, def.beam_defaults);
		AddWheelBeam(axis_node_1, rim_inner_node, rim_spring, rim_damp, def.beam_defaults);

		/* Reinforcement rim ring */
		unsigned int rim_next_outer_node_index = base_node_index + (((i + 1) % def.num_rays) * 2);
		node_t *rim_next_outer_node = & m_rig->nodes[rim_next_outer_node_index];
		node_t *rim_next_inner_node = & m_rig->nodes[rim_next_outer_node_index + 1];

		AddWheelBeam(rim_outer_node, rim_inner_node,      rim_spring, rim_damp, def.beam_defaults);
		AddWheelBeam(rim_outer_node, rim_next_outer_node, rim_spring, rim_damp, def.beam_defaults);
		AddWheelBeam(rim_inner_node, rim_next_inner_node, rim_spring, rim_damp, def.beam_defaults);
		AddWheelBeam(rim_outer_node, rim_next_outer_node, rim_spring, rim_damp, def.beam_defaults);

		/* --- Tyre --- */

		/* These wheels use set_beam_defaults-settings for the tyretread beams like meshwheels2 */

		unsigned int tyre_node_index = rim_outer_node_index + (2 * def.num_rays);
		node_t *tyre_outer_node = & m_rig->nodes[tyre_node_index];
		node_t *tyre_inner_node = & m_rig->nodes[tyre_node_index + 1];
		unsigned int tyre_next_node_index = rim_next_outer_node_index + (2 * def.num_rays);
		node_t *tyre_next_outer_node = & m_rig->nodes[tyre_next_node_index];
		node_t *tyre_next_inner_node = & m_rig->nodes[tyre_next_node_index + 1];

		/* rim to tiretread */
		AddWheelBeam(rim_outer_node, tyre_outer_node,      tyre_spring/2.f, tyre_damp, def.beam_defaults);
		AddWheelBeam(rim_outer_node, tyre_next_inner_node, tyre_spring/2.f, tyre_damp, def.beam_defaults);
		AddWheelBeam(rim_outer_node, tyre_next_outer_node, tyre_spring/2.f, tyre_damp, def.beam_defaults);
		AddWheelBeam(rim_inner_node, tyre_outer_node,      tyre_spring/2.f, tyre_damp, def.beam_defaults);
		AddWheelBeam(rim_inner_node, tyre_inner_node,      tyre_spring/2.f, tyre_damp, def.beam_defaults);
		AddWheelBeam(rim_inner_node, tyre_next_inner_node, tyre_spring/2.f, tyre_damp, def.beam_defaults);

		/* Reinforcement (tire tread) */
		AddWheelBeam(tyre_outer_node, tyre_inner_node,      tread_spring, tread_damp, def.beam_defaults);
		AddWheelBeam(tyre_outer_node, tyre_next_outer_node, tread_spring, tread_damp, def.beam_defaults);
		AddWheelBeam(tyre_inner_node, tyre_next_inner_node, tread_spring, tread_damp, def.beam_defaults);
		AddWheelBeam(tyre_inner_node, tyre_next_outer_node, tread_spring, tread_damp, def.beam_defaults);
		if (rigidity_node != nullptr)
		{
			unsigned int beam_index = AddWheelBeam(rigidity_node, axis_node_closest_to_rigidity_node, tyre_spring, tyre_damp, def.beam_defaults);
			GetBeam(beam_index).type = BEAM_VIRTUAL;
		}

		/* Support beams */
		AddWheelBeam(axis_node_1, tyre_outer_node, tyre_spring/2.f, tyre_damp, def.beam_defaults, support_beams_short_bound, 0.f);
		AddWheelBeam(axis_node_2, tyre_outer_node, tyre_spring/2.f, tyre_damp, def.beam_defaults, support_beams_short_bound, 0.f);

	}

	/* Wheel object */
	wheel.braked = def.braking;
	wheel.propulsed = def.propulsion;
	wheel.nbnodes = 2 * def.num_rays;
	wheel.refnode0 = axis_node_1;
	wheel.refnode1 = axis_node_2;
	wheel.radius = def.rim_radius;
	wheel.arm = GetNodePointer(def.reference_arm_node);

	if (def.propulsion != RigDef::Wheels::PROPULSION_NONE)
	{
		/* for inter-differential locking */
		m_rig->proped_wheels++;
		m_rig->proppairs[m_rig->proped_wheels] = m_rig->free_wheel;
	}
	if (def.braking != RigDef::Wheels::BRAKING_NO)
	{
		m_rig->braked_wheels++;
	}

	/* Find near attach */
	Ogre::Real length_1 = (axis_node_1->RelPosition - wheel.arm->RelPosition).length();
	Ogre::Real length_2 = (axis_node_2->RelPosition - wheel.arm->RelPosition).length();
	wheel.near_attach = (length_1 < length_2) ? axis_node_1 : axis_node_2;

	/* Create visuals */
	BuildMeshWheelVisuals(
		m_rig->free_wheel, 
		base_node_index, 
		axis_node_1->pos,
		axis_node_2->pos,
		def.num_rays,
		def.rim_mesh_name,
		"tracks/trans", // Rim material name. Original parser: was hardcoded in BTS_FLEXBODYWHEELS
		def.rim_radius,
		def.side != RigDef::MeshWheel::SIDE_RIGHT
		);

	/* Create flexbody */
	char flexbody_name[256];
	sprintf(flexbody_name, "flexbody-%s-%i", m_rig->truckname, m_rig->free_flexbody);

	int num_nodes = def.num_rays * 4;
	std::vector<unsigned int> node_indices;
	node_indices.reserve(num_nodes);
	for (int i = 0; i < num_nodes; ++i)
	{
		node_indices.push_back( base_node_index + i );
	}

	m_rig->flexbodies[m_rig->free_flexbody] = new FlexBody(
		m_rig->nodes,
		m_rig->free_node,
		def.tyre_mesh_name,
		flexbody_name,
		axis_node_1->pos,
		axis_node_2->pos,
		static_cast<int>(base_node_index),
		Ogre::Vector3(0.5,0,0),
		Ogre::Quaternion::ZERO,
		node_indices,
		m_rig->materialFunctionMapper,
		m_rig->usedSkin,
		true, /* forceNoShadows */
		m_rig->materialReplacer
		);

	m_rig->free_flexbody++;

	/* Advance */
	m_rig->free_wheel++;
}

void RigSpawner::ProcessMeshWheel(RigDef::MeshWheel & meshwheel_def)
{
	unsigned int base_node_index = m_rig->free_node;
	node_t *axis_node_1 = GetNodePointer(meshwheel_def.nodes[0]);
	node_t *axis_node_2 = GetNodePointer(meshwheel_def.nodes[1]);

	/* Enforce the "second node must have a larger Z coordinate than the first" constraint */
	if (axis_node_1->RelPosition.z > axis_node_2->RelPosition.z)
	{
		node_t *swap = axis_node_1;
		axis_node_1 = axis_node_2;
		axis_node_2 = swap;
	}	

	unsigned int wheel_index = BuildWheelObjectAndNodes(
		meshwheel_def.num_rays,
		axis_node_1,
		axis_node_2,
		GetNodePointer(meshwheel_def.reference_arm_node),
		meshwheel_def.num_rays * 2,
		meshwheel_def.num_rays * 8,
		meshwheel_def.tyre_radius,
		meshwheel_def.propulsion,
		meshwheel_def.braking,
		meshwheel_def.node_defaults,
		meshwheel_def.mass
	);

	BuildWheelBeams(
		meshwheel_def.num_rays,
		base_node_index,
		axis_node_1,
		axis_node_2,
		meshwheel_def.spring,      /* Tyre */
		meshwheel_def.damping,     /* Tyre */
		meshwheel_def.spring,      /* Rim */
		meshwheel_def.damping,     /* Rim */
		meshwheel_def.beam_defaults,
		meshwheel_def.rigidity_node
	);

	BuildMeshWheelVisuals(
		wheel_index, 
		base_node_index, 
		axis_node_1->pos,
		axis_node_2->pos,
		meshwheel_def.num_rays,
		meshwheel_def.mesh_name,
		meshwheel_def.material_name,
		meshwheel_def.rim_radius,
		meshwheel_def.side != RigDef::MeshWheel::SIDE_RIGHT
		);
}

void RigSpawner::ProcessMeshWheel2(RigDef::MeshWheel2 & def)
{
	unsigned int base_node_index = m_rig->free_node;
	node_t *axis_node_1 = GetNodePointer(def.nodes[0]);
	node_t *axis_node_2 = GetNodePointer(def.nodes[1]);

	/* Enforce the "second node must have a larger Z coordinate than the first" constraint */
	if (axis_node_1->RelPosition.z > axis_node_2->RelPosition.z)
	{
		node_t *swap = axis_node_1;
		axis_node_1 = axis_node_2;
		axis_node_2 = swap;
	}	

	unsigned int wheel_index = BuildWheelObjectAndNodes(
		def.num_rays,
		axis_node_1,
		axis_node_2,
		GetNodePointer(def.reference_arm_node),
		def.num_rays * 2,
		def.num_rays * 8,
		def.tyre_radius,
		def.propulsion,
		def.braking,
		def.node_defaults,
		def.mass
	);

	/* --- Beams --- */
	/* Use data from directive 'set_beam_defaults' for the tiretread beams */
	float tyre_spring = def.tyre_springiness;
	float tyre_damp = def.tyre_damping;
	float rim_spring = def.beam_defaults->springiness;
	float rim_damp = def.beam_defaults->damping_constant;

	BuildWheelBeams(
		def.num_rays,
		base_node_index,
		axis_node_1,
		axis_node_2,
		tyre_spring,
		tyre_damp,
		rim_spring,
		rim_damp,
		def.beam_defaults,
		def.rigidity_node
	);

#if 0 // replaced
	for (unsigned int i = 0; i < def.num_rays; i++)
	{
		/* Bounded */
		unsigned int outer_ring_node_index = base_node_index + (i * 2);
		node_t *outer_ring_node = & m_rig->nodes[outer_ring_node_index];
		node_t *inner_ring_node = & m_rig->nodes[outer_ring_node_index + 1];
		
		AddWheelBeam(axis_node_1, outer_ring_node, tyre_spring, tyre_damp, def.beam_defaults, 0.66f, 0.f);
		AddWheelBeam(axis_node_2, inner_ring_node, tyre_spring, tyre_damp, def.beam_defaults, 0.66f, 0.f);
		AddWheelBeam(axis_node_2, outer_ring_node, tyre_spring, tyre_damp, def.beam_defaults);
		AddWheelBeam(axis_node_1, inner_ring_node, tyre_spring, tyre_damp, def.beam_defaults);

		/* Reinforcement */
		unsigned int next_outer_ring_node_index = base_node_index + (((i + 1) % def.num_rays) * 2);
		node_t *next_outer_ring_node = & m_rig->nodes[next_outer_ring_node_index];
		node_t *next_inner_ring_node = & m_rig->nodes[next_outer_ring_node_index + 1];

		AddWheelBeam(outer_ring_node, inner_ring_node,      rim_spring, rim_damp, def.beam_defaults);
		AddWheelBeam(outer_ring_node, next_outer_ring_node, rim_spring, rim_damp, def.beam_defaults);
		AddWheelBeam(inner_ring_node, next_inner_ring_node, rim_spring, rim_damp, def.beam_defaults);
		AddWheelBeam(inner_ring_node, next_outer_ring_node, rim_spring, rim_damp, def.beam_defaults);
	}
#endif

	/* --- Visuals --- */
	BuildMeshWheelVisuals(
		wheel_index, 
		base_node_index, 
		axis_node_1->pos,
		axis_node_2->pos,
		def.num_rays,
		def.mesh_name,
		def.material_name,
		def.rim_radius,
		def.side != RigDef::MeshWheel::SIDE_RIGHT
		);
}

void RigSpawner::BuildMeshWheelVisuals(
	unsigned int wheel_index,
	unsigned int base_node_index,
	unsigned int axis_node_1_index,
	unsigned int axis_node_2_index,
	unsigned int num_rays,
	Ogre::String mesh_name,
	Ogre::String material_name,
	float rim_radius,
	bool rim_reverse
)
{
	std::stringstream wheel_name;
	wheel_name << "wheel-" << m_rig->truckname << "-" << wheel_index;
	std::stringstream entity_name;
	entity_name << "wheelobj-" << m_rig->truckname << "-" << wheel_index;

	m_rig->vwheels[wheel_index].fm = new FlexMeshWheel(
		wheel_name.str(),
		m_rig->nodes,
		axis_node_1_index,
		axis_node_2_index,
		base_node_index,
		num_rays,
		mesh_name,
		material_name,
		rim_radius,
		rim_reverse,
		m_rig->materialFunctionMapper,
		m_rig->usedSkin,
		m_rig->materialReplacer
	);

	try
	{
		m_rig->vwheels[wheel_index].cnode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
		Ogre::Entity *ec = gEnv->sceneManager->createEntity(entity_name.str(), wheel_name.str());
		if (ec)
		{
			m_rig->deletion_Entities.emplace_back(ec);
			m_rig->vwheels[wheel_index].cnode->attachObject(ec);
		}
		
		MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, Ogre::ColourValue(0, 0.5, 0.5));
		if (m_rig->materialFunctionMapper != nullptr)
		{
			m_rig->materialFunctionMapper->replaceMeshMaterials(ec);
		}
		if (m_rig->materialReplacer != nullptr) 
		{	
			m_rig->materialReplacer->replaceMeshMaterials(ec);
		}
		if (m_rig->usedSkin != nullptr)
		{
			m_rig->usedSkin->replaceMeshMaterials(ec);
		}		
	} 
	catch(...)
	{
		std::stringstream msg;
		msg << "Error loading mesh: " << mesh_name;
		AddMessage(Message::TYPE_ERROR, msg.str());
	}
}

unsigned int RigSpawner::BuildWheelObjectAndNodes( 
	unsigned int num_rays,
	node_t *axis_node_1,
	node_t *axis_node_2,
	node_t *reference_arm_node,
	unsigned int reserve_nodes,
	unsigned int reserve_beams,
	float wheel_radius,
	RigDef::Wheels::Propulsion propulsion,
	RigDef::Wheels::Braking braking,
	boost::shared_ptr<RigDef::NodeDefaults> node_defaults,
	float wheel_mass,
	bool set_param_iswheel, /* Default: true */
	float wheel_width       /* Default: -1.f */
)
{
	/* Check capacity */
	CheckNodeLimit(reserve_nodes);
	CheckBeamLimit(reserve_beams);

	wheel_t & wheel = m_rig->wheels[m_rig->free_wheel];

	/* Axis */
	Ogre::Vector3 axis_vector = axis_node_2->RelPosition - axis_node_1->RelPosition;
	axis_vector.normalise();

	/* Wheel object */
	wheel.braked    = braking;
	wheel.propulsed = propulsion;
	wheel.nbnodes   = 2 * num_rays;
	wheel.refnode0  = axis_node_1;
	wheel.refnode1  = axis_node_2;
	wheel.radius    = wheel_radius;
	wheel.width     = (wheel_width < 0) ? axis_vector.length() : wheel_width;
	wheel.arm       = reference_arm_node;

	/* Find near attach */
	Ogre::Real length_1 = (axis_node_1->RelPosition - wheel.arm->RelPosition).length();
	Ogre::Real length_2 = (axis_node_2->RelPosition - wheel.arm->RelPosition).length();
	wheel.near_attach = (length_1 < length_2) ? axis_node_1 : axis_node_2;

	if (propulsion != RigDef::Wheels::PROPULSION_NONE)
	{
		m_rig->propwheelcount++;
		/* for inter-differential locking */
		m_rig->proped_wheels++;
		m_rig->proppairs[m_rig->proped_wheels] = m_rig->free_wheel;
	}
	if (braking != RigDef::Wheels::BRAKING_NO)
	{
		m_rig->braked_wheels++;
	}
	
	/* Nodes */
	Ogre::Vector3 ray_vector = axis_vector.perpendicular() * wheel_radius;
	Ogre::Quaternion ray_rotator = Ogre::Quaternion(Ogre::Degree(-360.0 / (num_rays * 2)), axis_vector);

#ifdef DEBUG_TRUCKPARSER2013
	// TRUCK PARSER 2013 DEBUG
	std::stringstream msg;
	msg << "\nDBG RigSpawner::BuildWheelObjectAndNodes()\nDBG nodebase:" << m_rig->free_node <<", axis-node-0:"<<axis_node_1->pos <<", axis-node-1:"<<axis_node_2->pos<<"\n";
	msg << "DBG ==== Adding nodes ====";
	// END
#endif

	for (unsigned int i = 0; i < num_rays; i++)
	{
		/* Outer ring */
		Ogre::Vector3 ray_point = axis_node_1->RelPosition + ray_vector;
		ray_vector = ray_rotator * ray_vector;

		node_t & outer_node = GetFreeNode();
		InitNode(outer_node, ray_point, node_defaults);
		outer_node.mass    = wheel_mass / (2.f * num_rays);
		outer_node.iswheel = (set_param_iswheel) ? (m_rig->free_wheel * 2) + 1 : 0;
		outer_node.id      = -1; // Orig: hardcoded (BTS_WHEELS)
		outer_node.wheelid = m_rig->free_wheel;

		contacter_t & outer_contacter = m_rig->contacters[m_rig->free_contacter];
		outer_contacter.nodeid        = outer_node.pos; /* Node index */
		outer_contacter.contacted     = 0;
		outer_contacter.opticontact   = 0;
		m_rig->free_contacter++;

		/* Inner ring */
		ray_point = axis_node_2->RelPosition + ray_vector;
		ray_vector = ray_rotator * ray_vector;

		node_t & inner_node = GetFreeNode();
		InitNode(inner_node, ray_point, node_defaults);
		inner_node.mass    = wheel_mass / (2.f * num_rays);
		inner_node.iswheel = (set_param_iswheel) ? (m_rig->free_wheel * 2) + 2 : 0;
		inner_node.id      = -1; // Orig: hardcoded (BTS_WHEELS)
		inner_node.wheelid = m_rig->free_wheel; 

		contacter_t & contacter = m_rig->contacters[m_rig->free_contacter];
		contacter.nodeid        = inner_node.pos; /* Node index */
		contacter.contacted     = 0;
		contacter.opticontact   = 0;
		m_rig->free_contacter++;

		/* Wheel object */
		wheel.nodes[i * 2] = & outer_node;
		wheel.nodes[(i * 2) + 1] = & inner_node;

#ifdef DEBUG_TRUCKPARSER2013
		// TRUCK PARSER 2013 DEBUG
		int modifier = 0;
		msg << "\nDBG\tN1: index=" << outer_node.pos + modifier <<", wheelid=" << outer_node.wheelid << ", iswheel=" << outer_node.iswheel 
			<<", X=" << outer_node.AbsPosition.x <<", Y=" << outer_node.AbsPosition.y <<", Z=" << outer_node.AbsPosition.z << std::endl
			<< "DBG\tN2: index=" << inner_node.pos + modifier <<", wheelid=" << inner_node.wheelid << ", iswheel=" << inner_node.iswheel 
			<<", X=" << inner_node.AbsPosition.x <<", Y=" << inner_node.AbsPosition.y <<", Z=" << inner_node.AbsPosition.z;
		// END
#endif
	}

#ifdef DEBUG_TRUCKPARSER2013
	// TRUCK PARSER 2013 DEBUG
	LOG(msg.str());
	// END
#endif

	/* Advance */
	unsigned int wheel_index = m_rig->free_wheel;
	m_rig->free_wheel++;
	return wheel_index;
}

int RigSpawner::FindLowestNodeInRig()
{
	int lowest_node_index = 0;
	float lowest_y = m_rig->nodes[0].AbsPosition.y;

	for (int i = 0; i < m_rig->free_node; i++)
	{
		float y = m_rig->nodes[i].AbsPosition.y;
		if (y < lowest_y)
		{
			lowest_y = y;
			lowest_node_index = i;
		}
	}

	return lowest_node_index;
}

void RigSpawner::BuildWheelBeams(
	unsigned int num_rays,
	unsigned int base_node_index,
	node_t *axis_node_1,
	node_t *axis_node_2,
	float tyre_spring,
	float tyre_damping,
	float rim_spring,
	float rim_damping,
	boost::shared_ptr<RigDef::BeamDefaults> beam_defaults,
	RigDef::Node::Id rigidity_node_id
)
{
#ifdef DEBUG_TRUCKPARSER2013
	// DEBUG
	std::stringstream msg;
	msg << "==== BEAMS ====";
	// END DEBUG
#endif

	/* Find out where to connect rigidity node */
	bool rigidity_beam_side_1 = false;
	node_t *rigidity_node = nullptr;
	if (rigidity_node_id.IsValid())
	{
		rigidity_node = GetNodePointerOrThrow(rigidity_node_id);
		float distance_1 = rigidity_node->RelPosition.distance(axis_node_1->RelPosition);
		float distance_2 = rigidity_node->RelPosition.distance(axis_node_2->RelPosition);
		rigidity_beam_side_1 = distance_1 < distance_2;
	}

	for (unsigned int i = 0; i < num_rays; i++)
	{
		/* Bounded */
		unsigned int outer_ring_node_index = base_node_index + (i * 2);
		node_t *outer_ring_node = & m_rig->nodes[outer_ring_node_index];
		node_t *inner_ring_node = & m_rig->nodes[outer_ring_node_index + 1];
		
		AddWheelBeam(axis_node_1, outer_ring_node, tyre_spring, tyre_damping, beam_defaults, 0.66f, 0.f);
		AddWheelBeam(axis_node_2, inner_ring_node, tyre_spring, tyre_damping, beam_defaults, 0.66f, 0.f);
		AddWheelBeam(axis_node_2, outer_ring_node, tyre_spring, tyre_damping, beam_defaults);
		AddWheelBeam(axis_node_1, inner_ring_node, tyre_spring, tyre_damping, beam_defaults);

		/* Reinforcement */
		unsigned int next_outer_ring_node_index = base_node_index + (((i + 1) % num_rays) * 2);
		node_t *next_outer_ring_node = & m_rig->nodes[next_outer_ring_node_index];
		node_t *next_inner_ring_node = & m_rig->nodes[next_outer_ring_node_index + 1];

		AddWheelBeam(outer_ring_node, inner_ring_node,      rim_spring, rim_damping, beam_defaults);
		AddWheelBeam(outer_ring_node, next_outer_ring_node, rim_spring, rim_damping, beam_defaults);
		AddWheelBeam(inner_ring_node, next_inner_ring_node, rim_spring, rim_damping, beam_defaults);
		AddWheelBeam(inner_ring_node, next_outer_ring_node, rim_spring, rim_damping, beam_defaults);

#ifdef DEBUG_TRUCKPARSER2013
		// TRUCK PARSER 2013 DEBUG
		int modifier = 0;
		msg<<"\nDBG\tBounded: ";
		msg<<"["<<axis_node_1->pos + modifier<<" "<<outer_ring_node->pos + modifier<<"] ";
		msg<<"["<<axis_node_2->pos + modifier<<" "<<inner_ring_node->pos + modifier<<"] ";
		msg<<"["<<axis_node_2->pos + modifier<<" "<<outer_ring_node->pos + modifier<<"] ";
		msg<<"["<<axis_node_1->pos + modifier<<" "<<inner_ring_node->pos + modifier<<"]";
		//reinforcement
		msg<<"\nDBG\tReinforcement: ";
		msg<<"["<<outer_ring_node->pos + modifier<<" "<<inner_ring_node->pos      + modifier<<"] ";
		msg<<"["<<outer_ring_node->pos + modifier<<" "<<next_outer_ring_node->pos + modifier<<"] ";
		msg<<"["<<inner_ring_node->pos + modifier<<" "<<next_inner_ring_node->pos + modifier<<"] ";
		msg<<"["<<inner_ring_node->pos + modifier<<" "<<next_outer_ring_node->pos + modifier<<"] ";
		// END
#endif

		/* Rigidity beams */
		if (rigidity_node != nullptr)
		{
			node_t *target_node = (rigidity_beam_side_1) ? outer_ring_node : inner_ring_node;
			unsigned int beam_index = AddWheelBeam(rigidity_node, target_node, tyre_spring, tyre_damping, beam_defaults, -1.f, -1.f, BEAM_VIRTUAL);
			m_rig->beams[beam_index].type = BEAM_VIRTUAL;

#ifdef DEBUG_TRUCKPARSER2013
			// DEBUG
			msg<<"\nDBG\tRigidityBeam: ["<<rigidity_node->pos + modifier<<" "<<target_node->pos + modifier<<"] ";
			msg<<((rigidity_beam_side_1) ? "(outer)" : "(inner)");
			// END
#endif
		}
	}
#ifdef DEBUG_TRUCKPARSER2013
	// TRUCK PARSER 2013 DEBUG
	LOG(msg.str());
	// END
#endif
}

unsigned int RigSpawner::AddWheel(RigDef::Wheel & wheel_def)
{
	unsigned int base_node_index = m_rig->free_node;
	node_t *axis_node_1 = GetNodePointer(wheel_def.nodes[0]);
	node_t *axis_node_2 = GetNodePointer(wheel_def.nodes[1]);

	/* Enforce the "second node must have a larger Z coordinate than the first" constraint */
	if (axis_node_1->RelPosition.z > axis_node_2->RelPosition.z)
	{
		node_t *swap = axis_node_1;
		axis_node_1 = axis_node_2;
		axis_node_2 = swap;
	}	

	unsigned int wheel_index = BuildWheelObjectAndNodes(
		wheel_def.num_rays,
		axis_node_1,
		axis_node_2,
		GetNodePointer(wheel_def.reference_arm_node),
		wheel_def.num_rays * 2,
		wheel_def.num_rays * 8,
		wheel_def.radius,
		wheel_def.propulsion,
		wheel_def.braking,
		wheel_def.node_defaults,
		wheel_def.mass,
		true,
		wheel_def.width
	);

	BuildWheelBeams(
		wheel_def.num_rays,
		base_node_index,
		axis_node_1,
		axis_node_2,
		wheel_def.springiness, /* Tyre */
		wheel_def.damping,     /* Tyre */
		wheel_def.springiness, /* Rim */
		wheel_def.damping,     /* Rim */
		wheel_def.beam_defaults,
		wheel_def.rigidity_node
	);

	CreateWheelVisuals(wheel_index, wheel_def, base_node_index);

	return wheel_index;
}

#if 0 // refactored into pieces
unsigned int RigSpawner::AddWheel(RigDef::Wheel & wheel_def)
{
	/* Check capacity */
	CheckNodeLimit(wheel_def.num_rays * 2);
	CheckBeamLimit(wheel_def.num_rays * 8);

	unsigned int base_node_index = m_rig->free_node;
	wheel_t & wheel = m_rig->wheels[m_rig->free_wheel];

	/* Enforce the "second node must have a larger Z coordinate than the first" constraint */
	unsigned int axis_nodes[2];
	if (GetNode(wheel_def.nodes[0]).RelPosition.z < GetNode(wheel_def.nodes[1]).RelPosition.z)
	{
		axis_nodes[0] = GetNodeIndexOrThrow(wheel_def.nodes[0]);
		axis_nodes[1] = GetNodeIndexOrThrow(wheel_def.nodes[1]);
	}
	else
	{
		axis_nodes[0] = GetNodeIndexOrThrow(wheel_def.nodes[1]);
		axis_nodes[1] = GetNodeIndexOrThrow(wheel_def.nodes[0]);
	}
	node_t & axis_node_1 = m_rig->nodes[axis_nodes[0]];
	node_t & axis_node_2 = m_rig->nodes[axis_nodes[1]];

	Ogre::Vector3 axis_vector = axis_node_2.RelPosition - axis_node_1.RelPosition;
	axis_vector.normalise();
	Ogre::Vector3 ray_vector = axis_vector.perpendicular() * wheel_def.radius;
	Ogre::Quaternion ray_rotator = Ogre::Quaternion(Ogre::Degree(-360.f / wheel_def.num_rays * 2), axis_vector);

	/* Nodes */
	for (unsigned int i = 0; i < wheel_def.num_rays; i++)
	{
		/* Outer ring */
		Ogre::Vector3 ray_point = axis_node_1.RelPosition + ray_vector;
		ray_vector = ray_rotator * ray_vector;

		node_t & outer_node = GetFreeNode();
		InitNode(outer_node, ray_point, wheel_def.node_defaults);
		outer_node.mass    = wheel_def.mass / (2.f * wheel_def.num_rays);
		outer_node.iswheel = (m_rig->free_wheel * 2) + 1;
		outer_node.id      = -1; // Orig: hardcoded (BTS_WHEELS)
		outer_node.wheelid = m_rig->free_wheel;

		contacter_t & contacter = m_rig->contacters[m_rig->free_contacter];
		contacter.nodeid        = outer_node.pos; /* Node index */
		contacter.contacted     = 0;
		contacter.opticontact   = 0;
		m_rig->free_contacter++;

		/* Inner ring */
		ray_point = axis_node_2.RelPosition + ray_vector;
		ray_vector = ray_rotator * ray_vector;

		node_t & inner_node = GetFreeNode();
		InitNode(inner_node, ray_point, wheel_def.node_defaults);
		inner_node.mass    = wheel_def.mass / (2.f * wheel_def.num_rays);
		inner_node.iswheel = (m_rig->free_wheel * 2) + 2;
		inner_node.id      = -1; // Orig: hardcoded (BTS_WHEELS)
		inner_node.wheelid = m_rig->free_wheel; 

		contacter_t & contacter = m_rig->contacters[m_rig->free_contacter];
		contacter.nodeid        = inner_node.pos; /* Node index */
		contacter.contacted     = 0;
		contacter.opticontact   = 0;
		m_rig->free_contacter++;

		/* Wheel object */
		wheel.nodes[i * 2] = & outer_node;
		wheel.nodes[(i * 2) + 1] = & inner_node;
	}

	/* Beams */
	for (unsigned int i = 0; i < wheel_def.num_rays; i++)
	{
		/* Bounded */
		unsigned int outer_ring_node_index = base_node_index + (i * 2);
		node_t *outer_ring_node = & m_rig->nodes[outer_ring_node_index];
		node_t *inner_ring_node = & m_rig->nodes[outer_ring_node_index + 1];
		
		unsigned int beam_index = SectionWheelsAddBeam(wheel_def, & axis_node_1, outer_ring_node);
		GetBeam(beam_index).shortbound = 0.66f;
		GetBeam(beam_index).longbound = 0.0f;
		beam_index = SectionWheelsAddBeam(wheel_def, & axis_node_2, inner_ring_node);
		GetBeam(beam_index).shortbound = 0.66f;
		GetBeam(beam_index).longbound = 0.0f;
		SectionWheelsAddBeam(wheel_def, & axis_node_2, outer_ring_node);
		SectionWheelsAddBeam(wheel_def, & axis_node_1, inner_ring_node);

		/* Reinforcement */
		unsigned int next_outer_ring_node_index = base_node_index + (((i + 1) % wheel_def.num_rays) * 2);
		node_t *next_outer_ring_node = & m_rig->nodes[next_outer_ring_node_index];
		node_t *next_inner_ring_node = & m_rig->nodes[next_outer_ring_node_index + 1];

		SectionWheelsAddBeam(wheel_def, outer_ring_node, inner_ring_node);
		SectionWheelsAddBeam(wheel_def, outer_ring_node, next_outer_ring_node);
		SectionWheelsAddBeam(wheel_def, inner_ring_node, next_inner_ring_node);
		SectionWheelsAddBeam(wheel_def, inner_ring_node, next_outer_ring_node);
	}

	/* Wheel object */
	wheel.braked    = wheel_def.braking;
	wheel.propulsed = wheel_def.propulsion;
	wheel.nbnodes   = 2 * wheel_def.num_rays;
	wheel.refnode0  = & axis_node_1;
	wheel.refnode1  = & axis_node_2;
	wheel.radius    = wheel_def.radius;
	wheel.width     = axis_vector.length(); /* wheel_def.width is ignored. */
	wheel.arm       = GetNodePointer(wheel_def.reference_arm_node);

	if (wheel_def.propulsion != RigDef::Wheels::PROPULSION_NONE)
	{
		m_rig->propwheelcount++;
		/* for inter-differential locking */
		m_rig->proped_wheels++;
		m_rig->proppairs[m_rig->proped_wheels] = m_rig->free_wheel;
	}
	if (wheel_def.braking != RigDef::Wheels::BRAKING_NO)
	{
		m_rig->braked_wheels++;
	}

	/* Find near attach */
	Ogre::Real length_1 = (axis_node_1.RelPosition - wheel.arm->RelPosition).length();
	Ogre::Real length_2 = (axis_node_2.RelPosition - wheel.arm->RelPosition).length();
	wheel.near_attach = & ((length_1 < length_2) ? axis_node_1 : axis_node_2);

	/* Advance */
	unsigned int wheel_index = m_rig->free_wheel;
	m_rig->free_wheel++;
	return wheel_index;
}
#endif

unsigned int RigSpawner::AddWheel2(RigDef::Wheel2 & wheel_2_def)
{
	/* Check capacity */
	CheckNodeLimit(wheel_2_def.num_rays * 4);
	CheckBeamLimit(wheel_2_def.num_rays * (wheel_2_def.rigidity_node.IsValid()) ? 26 : 25);

	unsigned int base_node_index = m_rig->free_node;
	wheel_t & wheel = m_rig->wheels[m_rig->free_wheel];

	/* Enforce the "second node must have a larger Z coordinate than the first" constraint */
	node_t *axis_node_1 = nullptr;
	node_t *axis_node_2 = nullptr;
	if (GetNode(wheel_2_def.nodes[0]).RelPosition.z < GetNode(wheel_2_def.nodes[1]).RelPosition.z)
	{
		node_t *axis_node_1 = GetNodePointer(wheel_2_def.nodes[0]);
		node_t *axis_node_2 = GetNodePointer(wheel_2_def.nodes[1]);
	}
	else
	{
		node_t *axis_node_1 = GetNodePointer(wheel_2_def.nodes[1]);
		node_t *axis_node_2 = GetNodePointer(wheel_2_def.nodes[0]);
	}

	/* Rigidity node */
	node_t *axis_node_closest_to_rigidity_node = nullptr;
	if (wheel_2_def.rigidity_node.IsValid())
	{
		node_t & rigidity_node = GetNode(wheel_2_def.rigidity_node);
		Ogre::Real distance_1 = (rigidity_node.RelPosition - axis_node_1->RelPosition).length();
		Ogre::Real distance_2 = (rigidity_node.RelPosition - axis_node_2->RelPosition).length();
		axis_node_closest_to_rigidity_node = ((distance_1 < distance_2)) ? axis_node_1 : axis_node_2;
	}

	/* Node&beam generation */
	Ogre::Vector3 axis_vector = axis_node_2->RelPosition - axis_node_1->RelPosition;
	axis_vector.normalise();
	Ogre::Vector3 rim_ray_vector = Ogre::Vector3(0, wheel_2_def.rim_radius, 0);
	Ogre::Quaternion rim_ray_rotator = Ogre::Quaternion(Ogre::Degree(-360.f / wheel_2_def.num_rays * 2), axis_vector);

	/* Width */
	wheel.width = axis_vector.length(); /* wheel_def.width is ignored. */

	/* Rim nodes */
	for (unsigned int i = 0; i < wheel_2_def.num_rays; i++)
	{
		float node_mass = wheel_2_def.mass / (4.f * wheel_2_def.num_rays);

		/* Outer ring */
		Ogre::Vector3 ray_point = axis_node_1->RelPosition + rim_ray_vector;
		rim_ray_vector = rim_ray_rotator * rim_ray_vector;

		node_t & outer_node = GetFreeNode();
		InitNode(outer_node, ray_point, wheel_2_def.node_defaults);
		outer_node.mass    = node_mass;
		outer_node.iswheel = (m_rig->free_wheel * 2) + 1;
		outer_node.id      = -1; // Orig: hardcoded (addWheel2)
		outer_node.wheelid = m_rig->free_wheel;

		/* Inner ring */
		ray_point = axis_node_2->RelPosition + rim_ray_vector;
		rim_ray_vector = rim_ray_rotator * rim_ray_vector;

		node_t & inner_node = GetFreeNode();
		InitNode(inner_node, ray_point, wheel_2_def.node_defaults);
		inner_node.mass    = node_mass;
		inner_node.iswheel = (m_rig->free_wheel * 2) + 2;
		inner_node.id      = -1; // Orig: hardcoded (addWheel2)
		inner_node.wheelid = m_rig->free_wheel;

		/* Wheel object */
		wheel.nodes[i * 2] = & outer_node;
		wheel.nodes[(i * 2) + 1] = & inner_node;
	}

	Ogre::Vector3 tyre_ray_vector = Ogre::Vector3(0, wheel_2_def.tyre_radius, 0);
	Ogre::Quaternion tyre_ray_rotator = Ogre::Quaternion(Ogre::Degree(-180.f / wheel_2_def.num_rays * 2), axis_vector);
	tyre_ray_vector = tyre_ray_rotator * tyre_ray_vector;

	/* Tyre nodes */
	for (unsigned int i = 0; i < wheel_2_def.num_rays; i++)
	{
		/* Outer ring */
		Ogre::Vector3 ray_point = axis_node_1->RelPosition + tyre_ray_vector;
		tyre_ray_vector = tyre_ray_rotator * tyre_ray_vector;

		node_t & outer_node = GetFreeNode();
		InitNode(outer_node, ray_point);
		outer_node.mass          = (0.67f * wheel_2_def.mass) / (2.f * wheel_2_def.num_rays);
		outer_node.iswheel       = (m_rig->free_wheel * 2) + 1;
		outer_node.id            = -1; // Orig: hardcoded (addWheel2)
		outer_node.wheelid       = m_rig->free_wheel;
		outer_node.friction_coef = wheel.width * WHEEL_FRICTION_COEF;
		outer_node.volume_coef   = wheel_2_def.node_defaults->volume;
		outer_node.surface_coef  = wheel_2_def.node_defaults->surface;

		contacter_t & contacter = m_rig->contacters[m_rig->free_contacter];
		contacter.nodeid        = outer_node.pos; /* Node index */
		contacter.contacted     = 0;
		contacter.opticontact   = 0;
		m_rig->free_contacter++;

		/* Inner ring */
		ray_point = axis_node_2->RelPosition + tyre_ray_vector;
		tyre_ray_vector = tyre_ray_rotator * tyre_ray_vector;

		node_t & inner_node = GetFreeNode();
		InitNode(inner_node, ray_point);
		inner_node.mass          = (0.33f * wheel_2_def.mass) / (2.f * wheel_2_def.num_rays);
		inner_node.iswheel       = (m_rig->free_wheel * 2) + 2;
		inner_node.id            = -1; // Orig: hardcoded (addWheel2)
		inner_node.wheelid       = m_rig->free_wheel;
		inner_node.friction_coef = wheel.width * WHEEL_FRICTION_COEF;
		inner_node.volume_coef   = wheel_2_def.node_defaults->volume;
		inner_node.surface_coef  = wheel_2_def.node_defaults->surface;

		contacter_t & inner_contacter = m_rig->contacters[m_rig->free_contacter];
		inner_contacter.nodeid        = inner_node.pos; /* Node index */
		inner_contacter.contacted     = 0;
		inner_contacter.opticontact   = 0;
		m_rig->free_contacter++;

		/* Wheel object */
		wheel.nodes[i * 2] = & outer_node;
		wheel.nodes[(i * 2) + 1] = & inner_node;
	}

	/* Beams */
	for (unsigned int i = 0; i < wheel_2_def.num_rays; i++)
	{
		/* --- Rim ---  */

		/* Bounded */
		unsigned int rim_outer_node_index = base_node_index + (i * 2);
		node_t *rim_outer_node = & m_rig->nodes[rim_outer_node_index];
		node_t *rim_inner_node = & m_rig->nodes[rim_outer_node_index + 1];

		unsigned int beam_index;
		beam_index = AddWheelRimBeam(wheel_2_def, axis_node_1, rim_outer_node);
		GetBeam(beam_index).shortbound = 0.66;
		beam_index = AddWheelRimBeam(wheel_2_def, axis_node_2, rim_inner_node);
		GetBeam(beam_index).shortbound = 0.66;
		AddWheelRimBeam(wheel_2_def, axis_node_2, rim_outer_node);
		AddWheelRimBeam(wheel_2_def, axis_node_1, rim_inner_node);

		/* Reinforcement */
		unsigned int rim_next_outer_node_index = base_node_index + (((i + 1) % wheel_2_def.num_rays) * 2);
		node_t *rim_next_outer_node = & m_rig->nodes[rim_next_outer_node_index];
		node_t *rim_next_inner_node = & m_rig->nodes[rim_next_outer_node_index + 1];

		AddWheelRimBeam(wheel_2_def, axis_node_1, rim_outer_node);
		AddWheelRimBeam(wheel_2_def, rim_outer_node, rim_inner_node);
		AddWheelRimBeam(wheel_2_def, rim_outer_node, rim_next_outer_node);
		AddWheelRimBeam(wheel_2_def, rim_inner_node, rim_next_inner_node);
		AddWheelRimBeam(wheel_2_def, rim_outer_node, rim_next_inner_node);
		AddWheelRimBeam(wheel_2_def, rim_inner_node, rim_next_outer_node);
		if (axis_node_closest_to_rigidity_node != nullptr)
		{
			beam_t & beam = GetFreeBeam();
			InitBeam(beam, GetNodePointer(wheel_2_def.rigidity_node), axis_node_closest_to_rigidity_node);
			beam.type = BEAM_VIRTUAL;
			beam.k = wheel_2_def.rim_springiness;
			beam.d = wheel_2_def.rim_damping;
			SetBeamStrength(beam, wheel_2_def.beam_defaults->breaking_threshold_constant);
		}

		/* --- Tyre --- */

		unsigned int tyre_node_index = rim_outer_node_index + (2 * wheel_2_def.num_rays);
		node_t *tyre_outer_node = & m_rig->nodes[tyre_node_index];
		node_t *tyre_inner_node = & m_rig->nodes[tyre_node_index + 1];
		unsigned int tyre_next_node_index = rim_next_outer_node_index + (2 * wheel_2_def.num_rays);
		node_t *tyre_next_outer_node = & m_rig->nodes[tyre_next_node_index];
		node_t *tyre_next_inner_node = & m_rig->nodes[tyre_next_node_index + 1];

		/* Tyre band */
		AddTyreBeam(wheel_2_def, tyre_outer_node, tyre_next_outer_node);
		AddTyreBeam(wheel_2_def, tyre_outer_node, tyre_next_inner_node);
		AddTyreBeam(wheel_2_def, tyre_inner_node, tyre_next_outer_node);
		AddTyreBeam(wheel_2_def, tyre_inner_node, tyre_next_inner_node);
		/* Tyre sidewalls */
		AddTyreBeam(wheel_2_def, tyre_outer_node, rim_outer_node);
		AddTyreBeam(wheel_2_def, tyre_outer_node, rim_next_outer_node);
		AddTyreBeam(wheel_2_def, tyre_inner_node, rim_inner_node);
		AddTyreBeam(wheel_2_def, tyre_inner_node, rim_next_inner_node);
		/* Reinforcement */
		AddTyreBeam(wheel_2_def, tyre_outer_node, rim_inner_node);
		AddTyreBeam(wheel_2_def, tyre_outer_node, rim_next_inner_node);
		AddTyreBeam(wheel_2_def, tyre_inner_node, rim_outer_node);
		AddTyreBeam(wheel_2_def, tyre_inner_node, rim_next_outer_node);
		/* Backpressure, bounded */
		AddTyreBeam(wheel_2_def, axis_node_1, tyre_outer_node);
		AddTyreBeam(wheel_2_def, axis_node_2, tyre_inner_node);
	}

	/* Wheel object */
	wheel.braked = wheel_2_def.braking;
	wheel.propulsed = wheel_2_def.propulsion;
	wheel.nbnodes = 2 * wheel_2_def.num_rays;
	wheel.refnode0 = axis_node_1;
	wheel.refnode1 = axis_node_2;
	wheel.radius = wheel_2_def.rim_radius;
	wheel.arm = GetNodePointer(wheel_2_def.reference_arm_node);

	if (wheel_2_def.propulsion != RigDef::Wheels::PROPULSION_NONE)
	{
		/* for inter-differential locking */
		m_rig->proped_wheels++;
		m_rig->proppairs[m_rig->proped_wheels] = m_rig->free_wheel;
	}
	if (wheel_2_def.braking != RigDef::Wheels::BRAKING_NO)
	{
		m_rig->braked_wheels++;
	}

	/* Find near attach */
	Ogre::Real length_1 = (axis_node_1->RelPosition - wheel.arm->RelPosition).length();
	Ogre::Real length_2 = (axis_node_2->RelPosition - wheel.arm->RelPosition).length();
	wheel.near_attach = (length_1 < length_2) ? axis_node_1 : axis_node_2;

	/* Advance */
	unsigned int wheel_index = m_rig->free_wheel;
	m_rig->free_wheel++;
	return wheel_index;
}

void RigSpawner::CreateWheelVisuals(unsigned int wheel_index, RigDef::Wheel & wheel_def, unsigned int node_base_index)
{
	CreateWheelVisuals(
		wheel_index, 
		node_base_index, 
		wheel_def.num_rays,
		wheel_def.face_material_name,
		wheel_def.band_material_name,
		false
		);
}

void RigSpawner::CreateWheelVisuals(unsigned int wheel_index, RigDef::Wheel2 & wheel_2_def, unsigned int node_base_index)
{
	CreateWheelVisuals(
		wheel_index, 
		node_base_index, 
		wheel_2_def.num_rays,
		wheel_2_def.face_material_name,
		wheel_2_def.band_material_name,
		true,
		wheel_2_def.rim_radius / wheel_2_def.tyre_radius
		);
}

void RigSpawner::CreateWheelVisuals(
	unsigned int wheel_index, 
	unsigned int node_base_index,
	unsigned int num_rays,
	Ogre::String const & rim_material_name,
	Ogre::String const & band_material_name,
	bool separate_rim,
	float rim_ratio
)
{
	wheel_t & wheel = m_rig->wheels[wheel_index];
	vwheel_t & visual_wheel = m_rig->vwheels[wheel_index];

	std::stringstream wheel_mesh_name;
	wheel_mesh_name << "wheel-" << m_rig->truckname << "-" << wheel_index;
	std::stringstream wheel_name_i;
	wheel_name_i << "wheelobj-" << m_rig->truckname << "-" << wheel_index;

	visual_wheel.meshwheel = false;
	visual_wheel.fm = new FlexMesh(
		wheel_mesh_name.str(),
		m_rig->nodes,
		wheel.refnode0->pos,
		wheel.refnode1->pos,
		node_base_index,
		num_rays,
		rim_material_name,
		band_material_name,
		separate_rim,
		rim_ratio
	);

	try
	{
		Ogre::Entity *ec = gEnv->sceneManager->createEntity(wheel_name_i.str(), wheel_mesh_name.str());
		MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, Ogre::ColourValue(0, 0.5, 0.5));
		if (m_rig->materialFunctionMapper != nullptr)
		{
			m_rig->materialFunctionMapper->replaceMeshMaterials(ec);
		}
		if (m_rig->materialReplacer != nullptr)
		{
			m_rig->materialReplacer->replaceMeshMaterials(ec);
		}
		if (m_rig->usedSkin != nullptr)
		{
			m_rig->usedSkin->replaceMeshMaterials(ec);
		}
		visual_wheel.cnode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
		m_rig->deletion_Entities.emplace_back(ec);
		visual_wheel.cnode->attachObject(ec);
	}
	catch (...)
	{
		std::stringstream msg;
		msg << "Failed to load mesh '" << wheel_mesh_name << "'";
		AddMessage(Message::TYPE_ERROR, msg.str());
	}
}

unsigned int RigSpawner::AddWheelBeam(
	node_t *node_1, 
	node_t *node_2, 
	float spring, 
	float damping, 
	boost::shared_ptr<RigDef::BeamDefaults> beam_defaults,
	float max_contraction,   /* Default: -1.f */
	float max_extension,     /* Default: -1.f */
	int type                 /* Default: BEAM_INVISIBLE */
)
{
	unsigned int index = m_rig->free_beam;
	beam_t & beam = AddBeam(*node_1, *node_2, beam_defaults, DEFAULT_DETACHER_GROUP); 
	beam.type = type;
	beam.k = spring;
	beam.d = damping;
	if (max_contraction > 0.f)
	{
		beam.shortbound = max_contraction;
		beam.longbound = max_extension;
		beam.bounded = SHOCK1;
	}
	CalculateBeamLength(beam);

	if (type != BEAM_VIRTUAL)
	{
		/* Create visuals, but don't attach to scene-graph (compatibility with show-skeleton function) */
		CreateBeamVisuals(beam, index, false);
	}

	return index;
}

unsigned int RigSpawner::AddWheelRimBeam(RigDef::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2)
{
	unsigned int beam_index = _SectionWheels2AddBeam(wheel_2_def, node_1, node_2);
	beam_t & beam = GetBeam(beam_index);
	beam.k = wheel_2_def.rim_springiness;
	beam.d = wheel_2_def.rim_damping;
	return beam_index;
}

unsigned int RigSpawner::AddTyreBeam(RigDef::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2)
{
	unsigned int beam_index = _SectionWheels2AddBeam(wheel_2_def, node_1, node_2);
	beam_t & beam = GetBeam(beam_index);
	beam.k = wheel_2_def.tyre_springiness;
	beam.d = wheel_2_def.tyre_damping;

	m_rig->pressure_beams[m_rig->free_pressure_beam] = beam_index;
	m_rig->free_pressure_beam++;

	return beam_index;
}

unsigned int RigSpawner::_SectionWheels2AddBeam(RigDef::Wheel2 & wheel_2_def, node_t *node_1, node_t *node_2)
{
	unsigned int index = m_rig->free_beam;
	beam_t & beam = GetFreeBeam();
	InitBeam(beam, node_1, node_2);
	beam.type = BEAM_INVISIBLE;
	SetBeamStrength(beam, wheel_2_def.beam_defaults->breaking_threshold_constant);
	return index;
}

RigDef::Wheel RigSpawner::DowngradeWheel2(RigDef::Wheel2 & wheel_2)
{
	RigDef::Wheel wheel;
	wheel.radius = wheel_2.tyre_radius;
	wheel.width = wheel_2.width;
	wheel.num_rays = wheel_2.num_rays;
	wheel.nodes[0] = wheel_2.nodes[0];
	wheel.nodes[1] = wheel_2.nodes[1];
	wheel.rigidity_node = wheel_2.rigidity_node;
	wheel.braking = wheel_2.braking;
	wheel.propulsion = wheel_2.propulsion;
	wheel.reference_arm_node = wheel_2.reference_arm_node;
	wheel.mass = wheel_2.mass;
	wheel.springiness = wheel_2.tyre_springiness;
	wheel.damping = wheel_2.tyre_damping;
	wheel.face_material_name = wheel_2.face_material_name;
	wheel.band_material_name = wheel_2.band_material_name;
	return wheel;
}

void RigSpawner::ProcessWheel2(RigDef::Wheel2 & def)
{
	if (m_rig->enable_wheel2)
	{
		unsigned int node_base_index = m_rig->free_node;
		unsigned int wheel_index = AddWheel2(def);
		CreateWheelVisuals(wheel_index, def, node_base_index);
	}
	else
	{
		AddWheel(DowngradeWheel2(def));
	}
};

void RigSpawner::ProcessWheel(RigDef::Wheel & def)
{
	AddWheel(def);
};

void RigSpawner::ProcessSlopeBrake(RigDef::SlopeBrake & def)
{
	/* #1: regulating_force */
	float force = def.regulating_force;
	if (force < 0.f || force > 20.f)
	{
		std::stringstream msg;
		msg << "Clamping 'regulating_force' value '" << force << "' to allowed range <0 - 20>";
		AddMessage(Message::TYPE_INFO, msg.str());
		force = (force < 0.f) ? 0.f : 20.f;
	}
	m_rig->slopeBrakeFactor = force;

	/* #2: attach_angle */
	float attach_angle = def.attach_angle;
	if (attach_angle < 1.f || attach_angle > 45.f)
	{
		std::stringstream msg;
		msg << "Clamping 'attach_angle' value '" << force << "' to allowed range <1 - 45>";
		AddMessage(Message::TYPE_INFO, msg.str());
		attach_angle = (attach_angle < 1.f) ? 1.f : 45.f;
	}
	m_rig->slopeBrakeAttAngle = attach_angle;

	/* #3: release_angle */
	float release_angle = def.release_angle;
	if (release_angle < 1.f || release_angle > 45.f)
	{
		std::stringstream msg;
		msg << "Clamping 'release_angle' value '" << force << "' to allowed range <1 - 45>";
		AddMessage(Message::TYPE_INFO, msg.str());
		release_angle = (release_angle < 1.f) ? 1.f : 45.f;
	}
	m_rig->slopeBrakeRelAngle = release_angle;
};

void RigSpawner::ProcessTractionControl(RigDef::TractionControl & def)
{
	/* #1: regulating_force */
	float force = def.regulation_force;
	if (force < 0.f || force > 20.f)
	{
		std::stringstream msg;
		msg << "Clamping 'regulating_force' value '" << force << "' to allowed range <0 - 20>";
		AddMessage(Message::TYPE_INFO, msg.str());
		force = (force < 0.f) ? 0.f : 20.f;
	}
	m_rig->tc_ratio = force;

	/* #2: wheelslip */
	m_rig->tc_wheelslip = (def.wheel_slip < 0.f) ? 0.f : def.wheel_slip;

	/* #3: fade_speed */
	m_rig->tc_fade = (def.fade_speed < 0.1f) ? 0.1f : def.fade_speed;

	/* #4: pulse/sec */
	float pulse = def.pulse_per_sec;
	if (pulse <= 1.0f || pulse >= 2000.0f)
	{
		m_rig->tc_pulse = 1;
	} 
	else
	{
		m_rig->tc_pulse = static_cast <unsigned int> (2000.f / std::fabs(pulse));
	}

	/* #5: mode */
	if (BITMASK_IS_1(def.mode, RigDef::AntiLockBrakes::MODE_ON))
	{
		m_rig->tc_mode = 1;
	}
	if (BITMASK_IS_1(def.mode, RigDef::AntiLockBrakes::MODE_OFF))
	{
		m_rig->tc_mode = 0;
	}
	if (BITMASK_IS_1(def.mode, RigDef::AntiLockBrakes::MODE_NO_TOGGLE))
	{
		m_rig->tc_notoggle = true;
	}
	if (BITMASK_IS_1(def.mode, RigDef::AntiLockBrakes::MODE_NO_DASHBOARD))
	{
		m_rig->tc_present = false;
	}
	else
	{
		m_rig->tc_present = true;
	}
};

void RigSpawner::ProcessAntiLockBrakes(RigDef::AntiLockBrakes & def)
{
	/* #1: regulating_force */
	float force = def.regulation_force;
	if (force < 0.f || force > 20.f)
	{
		std::stringstream msg;
		msg << "Clamping 'regulating_force' value '" << force << "' to allowed range <0 - 20>";
		AddMessage(Message::TYPE_INFO, msg.str());
		force = (force < 0.f) ? 0.f : 20.f;
	}
	m_rig->alb_ratio = force;

	/* #2: min_speed */
	/* Wheelspeed adaption: 60 sec * 60 mins / 1000(kilometer) = 3.6 to get meter per sec */
	float min_speed = def.min_speed / 3.6f;
	m_rig->alb_minspeed = std::max(0.5f, min_speed);

	/* #3: pulse_per_sec */
	float pulse = def.pulse_per_sec;
	if (pulse <= 1.0f || pulse >= 2000.0f)
	{
		m_rig->alb_pulse = 1;
	} 
	else
	{
		m_rig->alb_pulse = static_cast <unsigned int> (2000.f / std::fabs(pulse));
	}

	/* #4: mode */
	if (BITMASK_IS_1(def.mode, RigDef::AntiLockBrakes::MODE_ON))
	{
		m_rig->alb_mode = 1;
	}
	if (BITMASK_IS_1(def.mode, RigDef::AntiLockBrakes::MODE_OFF))
	{
		m_rig->alb_mode = 0;
	}
	if (BITMASK_IS_1(def.mode, RigDef::AntiLockBrakes::MODE_NO_TOGGLE))
	{
		m_rig->alb_notoggle = true;
	}
	if (BITMASK_IS_1(def.mode, RigDef::AntiLockBrakes::MODE_NO_DASHBOARD))
	{
		m_rig->alb_present = false;
	}
	else
	{
		m_rig->alb_present = true;
	}
}

void RigSpawner::ProcessBrakes(RigDef::Brakes & def)
{
	m_rig->brakeforce = def.default_braking_force;
	if (def._parking_brake_force_set)
	{
		m_rig->hbrakeforce = def.parking_brake_force;
	}
	else
	{
		m_rig->hbrakeforce = 2.f * m_rig->brakeforce;
	}
};

void RigSpawner::ProcessEngoption(RigDef::Engoption & def)
{
	/* Is this a land vehicle? */
	if (m_rig->engine == nullptr)
	{
		AddMessage(Message::TYPE_WARNING, "Section 'engoption' found but no engine defined. Skipping ...");
		return;
	}

	/* Find it */
	boost::shared_ptr<RigDef::Engoption> engoption;
	std::list<boost::shared_ptr<RigDef::File::Module>>::iterator module_itor = m_selected_modules.begin();
	for (; module_itor != m_selected_modules.end(); module_itor++)
	{
		if (module_itor->get()->engoption != nullptr)
		{
			engoption = module_itor->get()->engoption;
		}
	}

	/* Process it */
	m_rig->engine->setOptions(
		engoption->inertia,
		engoption->type,
		(engoption->_clutch_force_use_default) ? -1.f : engoption->clutch_force,
		engoption->shift_time,
		engoption->clutch_time,
		engoption->post_shift_time,
		engoption->idle_rpm,
		engoption->stall_rpm,
		engoption->max_idle_mixture,
		engoption->min_idle_mixture
	);
};

void RigSpawner::ProcessEngine(RigDef::Engine & def)
{
	/* Process it */
	m_rig->driveable = TRUCK;

	/* Process gear list to BeamEngine-compatible format */
	/* TODO: Move this to BeamEngine::BeamEngine() */
	std::vector<float> gears_compat;
	gears_compat.reserve(2 + def.gear_ratios.size());
	gears_compat.push_back(def.reverse_gear_ratio);
	gears_compat.push_back(def.neutral_gear_ratio);
	std::vector<float>::iterator itor = def.gear_ratios.begin();
	for (; itor < def.gear_ratios.end(); itor++)
	{
		gears_compat.push_back(*itor);
	}

	m_rig->engine = new BeamEngine(
		def.shift_down_rpm,
		def.shift_up_rpm,
		def.torque,
		gears_compat,
		def.global_gear_ratio,
		m_rig->trucknum
	);

	/* Are there any startup shifter settings? */
	Ogre::String gearbox_mode = SSETTING("gearbox_mode", "Automatic shift");
	if (gearbox_mode == "Manual shift - Auto clutch")
		m_rig->engine->setAutoMode(BeamEngine::SEMIAUTO);
	else if (gearbox_mode == "Fully Manual: sequential shift")
		m_rig->engine->setAutoMode(BeamEngine::MANUAL);
	else if (gearbox_mode == "Fully Manual: stick shift")
		m_rig->engine->setAutoMode(BeamEngine::MANUAL_STICK);
	else if (gearbox_mode == "Fully Manual: stick shift with ranges")
		m_rig->engine->setAutoMode(BeamEngine::MANUAL_RANGES);
};

void RigSpawner::ProcessHelp()
{
	SetCurrentKeyword(RigDef::File::KEYWORD_HELP);
	unsigned int material_count = 0;

	std::list<boost::shared_ptr<RigDef::File::Module>>::iterator module_itor = m_selected_modules.begin();
	for (; module_itor != m_selected_modules.end(); module_itor++)
	{
		if (! module_itor->get()->help_panel_material_name.empty())
		{
			strncpy(m_rig->helpmat, module_itor->get()->help_panel_material_name.c_str(), 255);
			material_count++;
		}
	}

	if (material_count > 1)
	{
		std::stringstream msg;
		msg << "Multiple (" << material_count << ") 'help' sections found. Using the last one...";
		AddMessage(Message::TYPE_WARNING, msg.str());	
	}

	SetCurrentKeyword(RigDef::File::KEYWORD_INVALID);
};

void RigSpawner::ProcessFileInfo()
{
	if (m_file->file_info != nullptr)
	{
		if (m_file->file_info->_has_unique_id)
		{
			strncpy(m_rig->uniquetruckid, m_file->file_info->unique_id.c_str(), 254);
		}

		if (m_file->file_info->_has_category_id)
		{
			m_rig->categoryid = m_file->file_info->category_id;
		}

		if (m_file->file_info->_has_file_version_set)
		{
			m_rig->truckversion = m_file->file_info->file_version;
		}
	}
}

void RigSpawner::ProcessAuthors()
{
	SetCurrentKeyword(RigDef::File::KEYWORD_FILEFORMATVERSION);

	std::vector<RigDef::Author>::iterator author_itor = m_file->authors.begin();
	for (; author_itor != m_file->authors.end(); author_itor++)
	{
		authorinfo_t author;
		author.type = author_itor->type;
		author.name = author_itor->name;
		author.email = author_itor->email;
		if (author_itor->_has_forum_account)
		{
			author.id = author_itor->forum_account_id;
		}
		m_rig->authors.push_back(author);
	}	

	SetCurrentKeyword(RigDef::File::KEYWORD_INVALID);
};

unsigned int RigSpawner::GetNodeIndexOrThrow(RigDef::Node::Id & id)
{
	std::pair<unsigned int, bool> result = GetNodeIndex(id);
	if (! result.second)
	{
		std::stringstream msg;
		msg << "Failed to retrieve required node '";
		if (! id.Str().empty())
		{
			msg << id.Str();
		}
		else
		{
			msg << id.Num();
		}
		msg << "'";
		throw Exception(msg.str());
	}
	return result.first;
}

node_t & RigSpawner::GetNodeOrThrow(RigDef::Node::Id & id)
{
	return m_rig->nodes[GetNodeIndexOrThrow(id)];
}

void RigSpawner::ProcessCamera(RigDef::Camera & def)
{
	/* Center node */
	if (def.center_node.IsValid())
	{
		m_rig->cameranodepos[m_rig->freecamera] 
			= static_cast<int>(GetNodeIndexOrThrow(def.center_node));
	}
	else
	{
		m_rig->cameranodepos[m_rig->freecamera] = -1;
	}

	/* Direction node */
	if (def.back_node.IsValid())
	{
		m_rig->cameranodedir[m_rig->freecamera] 
			= static_cast<int>(GetNodeIndexOrThrow(def.back_node));
	}
	else
	{
		m_rig->cameranodedir[m_rig->freecamera] = -1;
	}

	/* Roll node */
	if (def.left_node.IsValid())
	{
		m_rig->cameranoderoll[m_rig->freecamera] 
			= static_cast<int>(GetNodeIndexOrThrow(def.left_node));
	}
	else
	{
		m_rig->cameranoderoll[m_rig->freecamera] = -1;
	}

	/* Advance */
	m_rig->freecamera++;
};

node_t* RigSpawner::GetBeamNodePointer(RigDef::Node::Id & id)
{
	node_t* node = GetNodePointer(id);
	if (node != nullptr)
	{
		return node;
	}	
	if (id.Str().empty()) /* Is this numbered node? */
	{
		std::stringstream msg;
		msg << "Beam defined with non-existent node '" << id.ToString() << "'. Using it anyway for backwards compatibility. Please fix.";
		AddMessage(Message::TYPE_WARNING, msg.str());
		return & m_rig->nodes[id.Num()]; /* Backwards compatibility */
	}
	return nullptr;
}

void RigSpawner::ProcessBeam(RigDef::Beam & def)
{
	beam_t beam;
	memset(&beam, 0, sizeof(beam_t));

	/* Nodes */
	beam.p1 = GetBeamNodePointer(def.nodes[0]);
	if (beam.p1 == nullptr)
	{
		AddMessage(Message::TYPE_WARNING, "Could not find node, ignoring beam...");
		return;
	}
	beam.p2 = GetBeamNodePointer(def.nodes[1]);
	if (beam.p2 == nullptr)
	{
		AddMessage(Message::TYPE_WARNING, "Could not find node, ignoring beam...");
		return;
	}

	/* Misc. params */
	beam.disabled = false;
	beam.type = BEAM_NORMAL;
	beam.detacher_group = m_rig->detacher_group_state;
	beam.k = def.defaults->GetScaledSpringiness();
	beam.d = def.defaults->GetScaledDamping();
	beam.diameter = def.defaults->visual_beam_diameter;
	beam.minendmass = 1.f; // Orig = hardcoded in add_beam()
	beam.bounded = NOSHOCK; // Orig: if (shortbound) ... hardcoded in BTS_BEAMS
	// orig: beams[pos].p2truck=0; (redundant because of memset() )

	/* Deformation */
	SetBeamDeformationThreshold(beam, def.defaults);
			
	beam.plastic_coef         = def.defaults->plastic_deformation_coefficient;
	beam.default_plastic_coef = def.defaults->plastic_deformation_coefficient;

	/* Calculate length */
	// orig = precompression hardcoded to 1
	float beam_length = (beam.p1->RelPosition - beam.p2->RelPosition).length();
	beam.L      = beam_length;
	beam.Lhydro = beam_length;
	beam.refL   = beam_length;

	/* Strength */
	float beam_strength = def.defaults->GetScaledBreakingThreshold();
	beam.strength  = beam_strength;
	beam.iStrength = beam_strength;

	/* Options */
	if (BITMASK_IS_1(def.options, RigDef::Beam::OPTION_i_INVISIBLE))
	{
		beam.type = BEAM_INVISIBLE;
	}
	if (BITMASK_IS_1(def.options, RigDef::Beam::OPTION_r_ROPE))
	{
		beam.bounded = ROPE;
	}
	if (BITMASK_IS_1(def.options, RigDef::Beam::OPTION_s_SUPPORT))
	{
		beam.bounded = SUPPORTBEAM;
		beam.longbound = def.extension_break_limit;
	}

	CreateBeamVisuals(beam, m_rig->free_beam, BITMASK_IS_0(def.options, RigDef::Beam::OPTION_i_INVISIBLE));

	m_rig->beams[m_rig->free_beam] = beam;
	m_rig->free_beam++;
}

void RigSpawner::CreateBeamVisuals(beam_t &beam, int index, bool attach_entity_to_scene)
{
	/* Setup visuals */
	// In original loader, in BTS_BEAMS the visuals are always created but not always attached.
	// Is there a reason?
	std::stringstream beam_name;
	beam_name << "beam-" << m_rig->truckname << "-" << index;
	try
	{
		beam.mEntity = gEnv->sceneManager->createEntity(beam_name.str(), "beam.mesh");
	}
	catch (...)
	{
		throw Exception("Failed to load file 'beam.mesh' (should come with RoR installation)");
	}
	m_rig->deletion_Entities.push_back(beam.mEntity);
	beam.mSceneNode = m_rig->beamsRoot->createChildSceneNode();
	beam.mSceneNode->setScale(beam.diameter, -1, beam.diameter);

	/* Material */
	if (beam.type == BEAM_HYDRO || beam.type == BEAM_MARKED)
	{
		beam.mEntity->setMaterialName("tracks/Chrome");
	}

	/* Attach visuals */
	// In original loader, in BTS_BEAMS the visuals are always created but not always attached.
	// Is there a reason?
	if (attach_entity_to_scene)
	{
		beam.mSceneNode->attachObject(beam.mEntity);
	}
}

void RigSpawner::SetBeamDeformationThreshold(beam_t & beam, boost::shared_ptr<RigDef::BeamDefaults> beam_defaults)
{
	/*
	---------------------------------------------------------------------------
		Old parser logic
	---------------------------------------------------------------------------

	VAR default_deform              = BEAM_DEFORM (400,000)
	VAR default_deform_scale        = 1
	VAR beam_creak                  = BEAM_CREAK_DEFAULT (100,000)
	VAR enable_advanced_deformation = false

	add_beam()
		IF default_deform < beam_creak
			default_deform = beam_creak
		END IF

		VAR beam;
		beam.default_deform = default_deform * default_deform_scale
	END

	set_beam_defaults:
		IF (!enable_advanced_deformation && default_deform < BEAM_DEFORM)
		   default_deform = BEAM_DEFORM;
		END IF
    
		VAR user_default_deform
		VAR default_deform_set
    
		IF (default_deform_set)
			beam_creak = 0
			default_deform = user_default_deform
		END IF
  
	---------------------------------------------------------------------------
		TruckParser2013
	---------------------------------------------------------------------------    

	VAR beam_defaults
	{
		default_deform                = BEAM_DEFORM
		scale.default_deform          = 1
		_enable_advanced_deformation  = false
		_user_defined                 = false
		_default_deform_set           = false
	}

	add_beam:
    
		VAR default_deform = BEAM_DEFORM;
    
		IF (beam_defaults._user_defined)        
			IF (beam_defaults._default_deform_set)
				default_deform = beam_defaults.default_deform    
			END IF   
		END IF
    
		IF (beam_defaults._default_deform_set && default_deform < BEAM_CREAK_DEFAULT)
			default_deform = BEAM_CREAK_DEFAULT
		END IF
    
		VAR beam;
		beam.default_deform = default_deform * beam_defaults.scale.default_deform
	
	---------------------------------------------------------------------------
	*/

	float default_deform = BEAM_DEFORM;

	bool is_deform_user_defined = BITMASK_IS_1(beam_defaults->_user_specified_fields, RigDef::BeamDefaults::PARAM_DEFORM_THRESHOLD_CONSTANT);

	if (beam_defaults->_is_user_defined && is_deform_user_defined)
	{
		default_deform = beam_defaults->deformation_threshold_constant;
	}

	if (is_deform_user_defined && default_deform < BEAM_CREAK_DEFAULT)
	{
		default_deform = BEAM_CREAK_DEFAULT;
	}

	float deformation_threshold = default_deform * beam_defaults->scale.deformation_threshold_constant;

	beam.default_deform     = deformation_threshold;
	beam.minmaxposnegstress = deformation_threshold;
	beam.maxposstress       = deformation_threshold;
	beam.maxnegstress       = -(deformation_threshold);
}

void RigSpawner::CreateBeamVisuals(beam_t & beam, int beam_index, RigDef::BeamDefaults & beam_defaults, bool activate)
{
	std::stringstream beam_name;
	beam_name << "beam-" << m_rig->truckname << "-" << beam_index;
	try
	{
		beam.mEntity = gEnv->sceneManager->createEntity(beam_name.str(), "beam.mesh");
	}
	catch (...)
	{
		throw Exception("Failed to load file 'beam.mesh' (should come with RoR installation)");
	}
	m_rig->deletion_Entities.push_back(beam.mEntity);
	beam.mSceneNode = m_rig->beamsRoot->createChildSceneNode();
	beam.mSceneNode->setScale(beam.diameter, -1, beam.diameter);
	beam.mEntity->setMaterialName(beam_defaults.beam_material_name);

	/* Attach visuals */
	if (activate)
	{
		beam.mSceneNode->attachObject(beam.mEntity);
	}	
}

void RigSpawner::CalculateBeamLength(beam_t & beam)
{
	float beam_length = (beam.p1->RelPosition - beam.p2->RelPosition).length();
	beam.L = beam_length;
	beam.Lhydro = beam_length;
	beam.refL = beam_length;
}

void RigSpawner::InitBeam(beam_t & beam, node_t *node_1, node_t *node_2)
{
	beam.p1 = node_1;
	beam.p2 = node_2;

	/* Length */
	CalculateBeamLength(beam);

	/* Misc */
	beam.minendmass = 1.f; // Orig = hardcoded in add_beam()
}

void RigSpawner::AddMessage(RigSpawner::Message::Type type,	Ogre::String const & text)
{
	/* Add message to report */
	m_messages.push_back(Message(type, text, m_current_keyword));

	/* Log message immediately (to put other log messages in context) */
	std::stringstream report;
	report << " == RigSpawner: ";
	switch (type)
	{
		case (RigSpawner::Message::TYPE_INTERNAL_ERROR): 
			report << "INTERNAL ERROR"; 
			break;

		case (RigSpawner::Message::TYPE_ERROR):
			report << "ERROR";
			break;

		case (RigSpawner::Message::TYPE_WARNING):
			report << "WARNING";
			break;

		default:
			report << "INFO";
			break;
	}
	report << " (Keyword " << RigDef::File::KeywordToString(m_current_keyword) << ") " << text;
	Ogre::LogManager::getSingleton().logMessage(report.str());
}

std::pair<unsigned int, bool> RigSpawner::GetNodeIndex(RigDef::Node::Id & id, bool quiet /* Default: false */)
{
	if (id.Num() == RigDef::Node::Id::INVALID_ID_VALUE)
	{
		if (! quiet)
		{
			AddMessage(Message::TYPE_ERROR, "Attempt to find node marked 'invalid'");
		}
		return std::make_pair(0, false);
	}
	else if (! id.Str().empty())
	{
		std::map<Ogre::String, unsigned int>::iterator result = m_named_nodes.find(id.Str());
		if (result != m_named_nodes.end())
		{
			return std::make_pair(result->second, true);
		}
		else if (! quiet)
		{
			std::stringstream msg;
			msg << "Attempt to find non-existent node named '" << id.Str() << "'";
			AddMessage(Message::TYPE_WARNING, msg.str());
		}
		return std::make_pair(0, false);
	}
	else
	{
		std::map<unsigned int, unsigned int>::iterator result = m_numbered_nodes.find(id.Num());
		if (result != m_numbered_nodes.end())
		{
			return std::make_pair(result->second, true);
		}
		else if (! quiet)
		{
			std::stringstream msg;
			msg << "Attempt to find non-existent node with number '" << id.Num() << "'";
			AddMessage(Message::TYPE_WARNING, msg.str());
		}
		return std::make_pair(0, false);
	}
}

node_t* RigSpawner::GetNodePointer(RigDef::Node::Id & id)
{
	std::pair<unsigned int, bool> result = GetNodeIndex(id);
	if (result.second)
	{
		return & m_rig->nodes[result.first];
	}
	else
	{
		return nullptr;
	}
}

node_t* RigSpawner::GetNodePointerOrThrow(RigDef::Node::Id & id)
{
	node_t *node = GetNodePointer(id);
	if (node == nullptr)
	{
		std::stringstream msg;
		msg << "Required node '" << id.ToString() << "' not found";
		throw Exception(msg.str());
	}
	return node;
}

std::pair<unsigned int, bool> RigSpawner::AddNode(RigDef::Node::Id & id)
{
	if (id.Num() == RigDef::Node::Id::INVALID_ID_VALUE)
	{
		throw Exception("Attempt to add node marked 'invalid'");
	}
	else if (! id.Str().empty())
	{
		if (m_rig->free_node == 0) // Double check, Validator should take care of this...
		{
			std::stringstream msg;
			msg << "The first node defined in section 'nodes' must be '0', found '" << id.Str() << "'";
			throw Exception(msg.str());
		}
		else if ((m_rig->free_node + 1) > MAX_NODES)
		{
			std::stringstream msg;
			msg << "Node limit (" << MAX_NODES << ") exceeded with node '" << id.Str() << "'";
			throw Exception(msg.str());
		}

		std::pair<std::map<Ogre::String, unsigned int>::iterator, bool> result
			= m_named_nodes.insert(std::make_pair(id.Str(), m_rig->free_node));
		if (! result.second)
		{
			AddMessage(Message::TYPE_ERROR, Ogre::String("Ignoring node! Duplicate name: ") + id.Str());
		}
		else
		{
			m_rig->free_node++;
		}
		return std::make_pair(m_rig->free_node, result.second);
	}
	else
	{
		if (m_rig->free_node == 0 && id.Num() != 0) // Double check, Validator should take care of this...
		{
			std::stringstream msg;
			msg << "The first node defined in section 'nodes' must be '0', found '" << id.Num() << "'";
			throw Exception(msg.str());
		}
		else if ((m_rig->free_node + 1) > MAX_NODES)
		{
			std::stringstream msg;
			msg << "Node limit (" << MAX_NODES << ") exceeded with node '" << id.Num() << "'";
			throw Exception(msg.str());
		}

		unsigned int free_node_slot = m_rig->free_node;
		std::pair<std::map<unsigned int, unsigned int>::iterator, bool> result
			= m_numbered_nodes.insert(std::make_pair(id.Num(), free_node_slot));
		if (! result.second)
		{
			AddMessage(Message::TYPE_ERROR, Ogre::String("Ignoring node! Duplicate number: ") + TOSTRING(id.Num()));
		}
		else
		{
			m_rig->free_node++;
		}

		return std::make_pair(free_node_slot, result.second);
	}
}

void RigSpawner::ProcessNode(RigDef::Node & def)
{
	std::pair<unsigned int, bool> inserted_node = AddNode(def.id);
	if (! inserted_node.second)
	{
		return;
	}

	node_t & node = m_rig->nodes[inserted_node.first];
	node.pos = inserted_node.first; /* Node index */
	node.id = static_cast<int>(def.id.Num());
	node.id_str = def.id.Str();

	/* Positioning */
	Ogre::Vector3 node_position = m_spawn_position + m_spawn_rotation * def.position;
	node.AbsPosition = node_position; 
	node.RelPosition = node_position - m_rig->origin;
	node.smoothpos   = node_position;
	node.iPosition   = node_position;
	if (node.pos != 0)
	{
		node.iDistance = (m_rig->nodes[0].AbsPosition - node_position).squaredLength();
	}
		
	node.wetstate = DRY; // orig = hardcoded (init_node)
	// node.locked = false // Orig {{nodes[pos].locked=m<0.0;}} // always false due to hardcoded mass
	// node.iswheel = 0; // Hardcoded in orig
	// node.friction = 0; // Hardcoded in orig
	node.wheelid = -1; // Hardcoded in orig (bts_nodes, call to init_node())
	node.friction_coef = def.node_defaults->friction;
	node.volume_coef = def.node_defaults->volume;
	node.surface_coef = def.node_defaults->surface;
	node.collisionBoundingBoxID = -1; // orig = hardcoded (init_node)
	node.iIsSkin = true; // orig = hardcoded (bts_nodes)

	/* Mass */
	if (def.node_defaults->load_weight >= 0.f) // The >= operator is in orig.
	{
		// orig = further override of hardcoded default.
		node.mass = def.node_defaults->load_weight; 
		node.masstype = NODE_LOADED;
		node.overrideMass = true;
	}
	else
	{
		node.mass = 10; // Hardcoded in original (bts_nodes, call to init_node())
		node.masstype = NODE_NORMAL; // orig = hardcoded (bts_nodes)
	}

	/* Gravity */
	float gravity = -9.81f;
	if(gEnv->terrainManager)
	{
		gravity = gEnv->terrainManager->getGravity();
	}
	node.gravimass = Ogre::Vector3(0, node.mass * gravity, 0);

	/* Lockgroup */
	node.lockgroup = (m_file->lockgroup_default_nolock) ? 9999 : 0;

	/* Options */
	unsigned int options = def.options | def.node_defaults->options; /* Merge bit flags */
	if (BITMASK_IS_1(options, RigDef::Node::OPTION_l_LOAD_WEIGHT))
	{
		if (def._has_load_weight_override)
		{
			node.masstype = NODE_LOADED;
			node.overrideMass = true;
			node.mass = def.load_weight_override;
		}
		else
		{
			node.masstype = NODE_LOADED;
			m_rig->masscount++;
		}
	}
	if (BITMASK_IS_1(options, RigDef::Node::OPTION_e_TERRAIN_EDIT_POINT))
	{
		if(! m_rig->networking)
		{
			m_rig->editorId = node.pos;
		}
	}
	if (BITMASK_IS_1(options, RigDef::Node::OPTION_h_HOOK_POINT))
	{
		/* Link [current-node] -> [node-0] */
		/* If current node is 0, link [node-0] -> [node-1] */
		node_t & node_2 = (node.pos == 0) ? GetNode(1) : GetNode(0);
		unsigned int beam_index = m_rig->free_beam;

		beam_t & beam = AddBeam(node, node_2, def.beam_defaults, def.detacher_group);
		SetBeamStrength(beam, def.beam_defaults->GetScaledBreakingThreshold() * 100.f);
		beam.type              = BEAM_HYDRO;
		beam.d                 = def.beam_defaults->GetScaledDamping() * 0.1f;
		beam.k                 = def.beam_defaults->GetScaledSpringiness();
		beam.bounded           = ROPE;
		beam.disabled          = true;
		beam.L                 = HOOK_RANGE_DEFAULT;
		beam.Lhydro            = HOOK_RANGE_DEFAULT;
		beam.refL              = HOOK_RANGE_DEFAULT;
		beam.commandRatioShort = HOOK_SPEED_DEFAULT;
		beam.commandRatioLong  = HOOK_SPEED_DEFAULT;
		beam.commandShort      = 0.0f;
		beam.commandLong       = 1.0f;
		beam.maxtiestress      = HOOK_FORCE_DEFAULT;
		SetBeamDeformationThreshold(beam, def.beam_defaults);
		CreateBeamVisuals(beam, beam_index, true);
			
		// Logic cloned from SerializedRig.cpp, section BTS_NODES
		hook_t hook;
		hook.hookNode     = & node;
		hook.group        = -1;
		hook.locked       = UNLOCKED;
		hook.lockNode     = 0;
		hook.lockTruck    = 0;
		hook.lockNodes    = true;
		hook.lockgroup    = -1;
		hook.beam         = & beam;
		hook.maxforce     = HOOK_FORCE_DEFAULT;
		hook.lockrange    = HOOK_RANGE_DEFAULT;
		hook.lockspeed    = HOOK_SPEED_DEFAULT;
		hook.selflock     = false;
		hook.nodisable    = false;
		hook.timer        = 0.0f;
		hook.timer_preset = HOOK_LOCK_TIMER_DEFAULT;
		hook.autolock     = false;
		m_rig->hooks.push_back(hook);
	}
	node.buoyancy          = BITMASK_IS_1(options, RigDef::Node::OPTION_b_EXTRA_BUOYANCY) ? 10000.f : node.buoyancy;
	node.mouseGrabMode     = BITMASK_IS_1(options, RigDef::Node::OPTION_m_NO_MOUSE_GRAB) ? 1 : node.mouseGrabMode;
	node.mouseGrabMode     = BITMASK_IS_1(options, RigDef::Node::OPTION_n_MOUSE_GRAB) ? 2 : node.mouseGrabMode;
	node.contactless       = BITMASK_IS_1(options, RigDef::Node::OPTION_c_NO_GROUND_CONTACT) ? 1 : 0;
	node.disable_particles = BITMASK_IS_1(options, RigDef::Node::OPTION_p_NO_PARTICLES);
	node.disable_sparks    = BITMASK_IS_1(options, RigDef::Node::OPTION_f_NO_SPARKS);
		
	m_rig->smokeRef        = BITMASK_IS_1(options, RigDef::Node::OPTION_y_EXHAUST_DIRECTION) ? node.pos : 0;
	m_rig->smokeId         = BITMASK_IS_1(options, RigDef::Node::OPTION_x_EXHAUST_POINT) ? node.pos : 0;

#ifdef DEBUG_TRUCKPARSER2013
	// DEBUG
	std::stringstream msg;
	msg<<"DBG ProcessNode() pos="<<node.pos<<", id="<<node.id << ", X=" << node.AbsPosition.x << ", Y=" << node.AbsPosition.y << ", Z=" << node.AbsPosition.z;
	LOG(msg.str());
	// END
#endif
}

void RigSpawner::AddExhaust(
		unsigned int emitter_node_idx,
		unsigned int direction_node_idx,
		bool old_format,
		Ogre::String *user_material_name
	)
{
	if (m_rig->disable_smoke)
	{
		return;
	}
	exhaust_t exhaust;
	exhaust.emitterNode = emitter_node_idx;
	exhaust.directionNode = direction_node_idx;
	exhaust.isOldFormat = old_format;

	Ogre::String material_name;
	if (user_material_name != nullptr)
	{
		material_name = *user_material_name;
	}
	else
	{
		material_name = "tracks/Smoke";
	}

	if (m_rig->usedSkin != nullptr && m_rig->usedSkin->hasReplacementForMaterial(material_name))
	{
		Ogre::String new_material = m_rig->usedSkin->getReplacementForMaterial(material_name);
		if (! new_material.empty())
		{
			material_name = new_material;
		}
		else
		{
			AddMessage(Message::TYPE_INTERNAL_ERROR, "AddExhaust(): Material replacer returned empty string.");
		}
	}
	
	std::stringstream particle_sys_name;
	particle_sys_name << "exhaust-" << m_rig->exhausts.size() << "-" << m_rig->truckname;
	exhaust.smoker = gEnv->sceneManager->createParticleSystem(particle_sys_name.str(), material_name);
	if (exhaust.smoker == nullptr)
	{
		AddMessage(Message::TYPE_INTERNAL_ERROR, "Failed to create exhaust");
		return;
	}
	exhaust.smoker->setVisibilityFlags(DEPTHMAP_DISABLED); // Disable particles in depthmap

	
	exhaust.smokeNode = m_parent_scene_node->createChildSceneNode();
	exhaust.smokeNode->attachObject(exhaust.smoker);
	exhaust.smokeNode->setPosition(m_rig->nodes[exhaust.emitterNode].AbsPosition);

	m_rig->nodes[emitter_node_idx].isHot = true;
	m_rig->nodes[direction_node_idx].iIsSkin = true;
	m_rig->nodes[emitter_node_idx].isHot = true;
	m_rig->nodes[direction_node_idx].iIsSkin = true;

	m_rig->exhausts.push_back(exhaust);
}

bool RigSpawner::AddModule(Ogre::String const & module_name)
{
	std::map< Ogre::String, boost::shared_ptr<RigDef::File::Module> >::iterator result 
		= m_file->modules.find(module_name);

	if (result != m_file->modules.end())
	{
		m_selected_modules.push_back(result->second);
		LOG(" == RigSpawner: Module added to configuration: " + module_name);
		return true;
	}
	AddMessage(Message::TYPE_WARNING, "Selected module not found: " + module_name);
	return false;
}

void RigSpawner::ProcessCinecam(RigDef::Cinecam & def)
{
	if (! CheckNodeLimit(1) && ! CheckBeamLimit(8))
	{
		return;
	}
			
	/* Node */
	Ogre::Vector3 node_pos = m_spawn_position + m_spawn_rotation * def.position;
	node_t & camera_node = GetAndInitFreeNode(node_pos);
	camera_node.contactless = 1; // Orig: hardcoded in BTS_CINECAM
	camera_node.wheelid = -1;
	camera_node.friction_coef = NODE_FRICTION_COEF_DEFAULT;
	camera_node.id = -1;

	m_rig->cinecameranodepos[m_rig->freecinecamera] = camera_node.pos;
	m_rig->freecinecamera++;

	/* Beams */
	for (unsigned int i = 0; i < 8; i++)
	{
		int beam_index = m_rig->free_beam;
		beam_t & beam = AddBeam(camera_node, GetNode(def.nodes[i]), def.beam_defaults, DEFAULT_DETACHER_GROUP);
		beam.type = BEAM_INVISIBLE;
		CalculateBeamLength(beam);
		beam.k = def.spring;
		beam.d = def.damping;
		CreateBeamVisuals(beam, beam_index, *def.beam_defaults, false);
	}

	/* Cabin light */
	if (m_rig->flaresMode >= 2 && m_rig->cablight == nullptr)
	{
		std::stringstream light_name;
		light_name << "cabinlight-" << m_rig->truckname; // Original: "cabinglight" (probably typo) ~only_a_ptr
		m_rig->cablight = gEnv->sceneManager->createLight(light_name.str());
		m_rig->cablight->setType(Ogre::Light::LT_POINT);
		m_rig->cablight->setDiffuseColour( Ogre::ColourValue(0.4, 0.4, 0.3));
		m_rig->cablight->setSpecularColour( Ogre::ColourValue(0.4, 0.4, 0.3));
		m_rig->cablight->setAttenuation(20, 1, 0, 0);
		m_rig->cablight->setCastShadows(false);
		m_rig->cablight->setVisible(true);

		m_rig->cablightNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
		m_rig->cablightNode->attachObject(m_rig->cablight);
		m_rig->cablightNode->setVisible(false);
		m_rig->deletion_sceneNodes.emplace_back(m_rig->cablightNode);
	}
};

void RigSpawner::InitNode(node_t & node, Ogre::Vector3 const & position)
{
	/* Position */
	node.AbsPosition = position;
	node.RelPosition = position - m_rig->origin;
	node.smoothpos = position;
	node.iPosition = position;
	if (node.pos != 0)
	{
		node.iDistance = (m_rig->nodes[0].AbsPosition - position).squaredLength();
	}

	/* Misc. */
	node.collisionBoundingBoxID = -1; // orig = hardcoded (init_node)
	node.wetstate = DRY; // orig = hardcoded (init_node)
}

void RigSpawner::InitNode(
	node_t & node, 
	Ogre::Vector3 const & position,
	boost::shared_ptr<RigDef::NodeDefaults> node_defaults
)
{
	InitNode(node, position);
	node.friction_coef = node_defaults->friction;
	node.volume_coef = node_defaults->volume;
	node.surface_coef = node_defaults->surface;
}

void RigSpawner::ProcessGlobals(RigDef::Globals & def)
{

	m_rig->truckmass = def.dry_mass;
	m_rig->loadmass = def.cargo_mass;

	if (! def.material_name.empty())
	{
		Ogre::String material_name;

		/* Check for skin */
		if (m_rig->usedSkin != nullptr && m_rig->usedSkin->hasReplacementForMaterial(def.material_name))
		{
			Ogre::String skin_mat_name = m_rig->usedSkin->getReplacementForMaterial(def.material_name);
			if (! skin_mat_name.empty())
			{
				material_name = skin_mat_name;
			}
		}
		else
		{
			material_name = def.material_name;
		}

		/* Clone the material */
		Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName(material_name);
		if (mat.isNull())
		{
			std::stringstream msg;
			msg << "Material '" << material_name << "' defined in section 'globals' not found. Trying material 'tracks/transred'";
			AddMessage(Message::TYPE_ERROR, msg.str());

			mat = Ogre::MaterialManager::getSingleton().getByName("tracks/transred");
			if (mat.isNull())
			{
				throw Exception("Vehicle material (or a built-in replacement) was not found.");
			}
		}
		std::stringstream mat_clone_name;
		mat_clone_name << m_rig->texname << "-" << m_rig->truckname;
		mat->clone(mat_clone_name.str());
		strncpy(m_rig->texname, mat_clone_name.str().c_str(), 1024);
	}
}

/* -------------------------------------------------------------------------- */
/* Limits.
/* -------------------------------------------------------------------------- */

bool RigSpawner::CheckNodeLimit(unsigned int count)
{
	if ((m_rig->free_node + count) > MAX_NODES)
	{
		std::stringstream msg;
		msg << "Node limit (" << MAX_NODES << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckBeamLimit(unsigned int count)
{
	if ((m_rig->free_beam + count) > MAX_BEAMS)
	{
		std::stringstream msg;
		msg << "Beam limit (" << MAX_BEAMS << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckShockLimit(unsigned int count)
{
	if ((m_rig->free_shock + count) > MAX_SHOCKS)
	{
		std::stringstream msg;
		msg << "Shock limit (" << MAX_SHOCKS << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckRotatorLimit(unsigned int count)
{
	if ((m_rig->free_rotator + count) > MAX_ROTATORS)
	{
		std::stringstream msg;
		msg << "Rotator limit (" << MAX_ROTATORS << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckHydroLimit(unsigned int count)
{
	if ((m_rig->free_shock + count) > MAX_HYDROS)
	{
		std::stringstream msg;
		msg << "Hydro limit (" << MAX_HYDROS << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckParticleLimit(unsigned int count)
{
	if ((m_rig->free_cparticle + count) > MAX_CPARTICLES)
	{
		std::stringstream msg;
		msg << "Particle limit (" << MAX_CPARTICLES << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckAxleLimit(unsigned int count)
{
	if ((m_rig->free_axle + count) > MAX_WHEELS/2)
	{
		std::stringstream msg;
		msg << "Axle limit (" << MAX_WHEELS/2 << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckPropLimit(unsigned int count)
{	if ((m_rig->free_prop + count) > MAX_PROPS)
	{
		std::stringstream msg;
		msg << "Prop limit (" << MAX_PROPS << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckFlexbodyLimit(unsigned int count)
{
	if ((m_rig->free_flexbody + count) > MAX_FLEXBODIES)
	{
		std::stringstream msg;
		msg << "Flexbody limit (" << MAX_FLEXBODIES << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckSubmeshLimit(unsigned int count)
{
	if ((m_rig->free_sub + count) > MAX_SUBMESHES)
	{
		std::stringstream msg;
		msg << "Submesh limit (" << MAX_SUBMESHES << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckTexcoordLimit(unsigned int count)
{
	if ((m_rig->free_texcoord + count) > MAX_TEXCOORDS)
	{
		std::stringstream msg;
		msg << "Texcoord limit (" << MAX_TEXCOORDS << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

/* Static version */
bool RigSpawner::CheckSoundScriptLimit(Beam *vehicle, unsigned int count)
{
	//return CheckSoundScriptLimit(m_rig, count);
	if ((vehicle->free_soundsource + count) > MAX_SOUNDSCRIPTS_PER_TRUCK)
	{
		std::stringstream msg;
		msg << "SoundScript limit (" << MAX_SOUNDSCRIPTS_PER_TRUCK << ") exceeded";
		LOG(msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckSoundScriptLimit(unsigned int count)
{
	//return CheckSoundScriptLimit(m_rig, count);
	if ((m_rig->free_soundsource + count) > MAX_SOUNDSCRIPTS_PER_TRUCK)
	{
		std::stringstream msg;
		msg << "SoundScript limit (" << MAX_SOUNDSCRIPTS_PER_TRUCK << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckCabLimit(unsigned int count)
{
	if ((m_rig->free_cab + count) > MAX_CABS)
	{
		std::stringstream msg;
		msg << "Cab limit (" << MAX_CABS << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckCameraRailLimit(unsigned int count)
{
	if ((m_rig->free_wing + count) > MAX_WINGS)
	{
		std::stringstream msg;
		msg << "Wing limit (" << MAX_WINGS << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckWingLimit(unsigned int count)
{
	if ((m_rig->free_camerarail + count) > MAX_CAMERARAIL)
	{
		std::stringstream msg;
		msg << "CameraRail limit (" << MAX_CAMERARAIL << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckAirBrakeLimit(unsigned int count)
{
	if ((m_rig->free_airbrake + count) > MAX_AIRBRAKES)
	{
		std::stringstream msg;
		msg << "AirBrake limit (" << MAX_AIRBRAKES << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckAeroEngineLimit(unsigned int count)
{
	if ((m_rig->free_aeroengine + count) > MAX_AEROENGINES)
	{
		std::stringstream msg;
		msg << "AeroEngine limit (" << MAX_AEROENGINES << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

bool RigSpawner::CheckScrewpropLimit(unsigned int count)
{
	if ((m_rig->free_screwprop + count) > MAX_SCREWPROPS)
	{
		std::stringstream msg;
		msg << "Screwprop limit (" << MAX_SCREWPROPS << ") exceeded";
		AddMessage(Message::TYPE_ERROR, msg.str());
		return false;
	}
	return true;
}

node_t &RigSpawner:: GetNode(unsigned int node_index)
{
	return m_rig->nodes[node_index];
}

void RigSpawner::InitNode(unsigned int node_index, Ogre::Vector3 const & position)
{
	InitNode(m_rig->nodes[node_index], position);
}

beam_t & RigSpawner::GetBeam(unsigned int index)
{
	return m_rig->beams[index];
}

node_t & RigSpawner::GetFreeNode()
{
	CheckNodeLimit(1);
	node_t & node = m_rig->nodes[m_rig->free_node];
	node.pos = m_rig->free_node;
	m_rig->free_node++;
	return node;
}

beam_t & RigSpawner::GetFreeBeam()
{
	CheckBeamLimit(1);
	beam_t & beam = m_rig->beams[m_rig->free_beam];
	m_rig->free_beam++;
	return beam;
}

shock_t & RigSpawner::GetFreeShock()
{
	shock_t & shock = m_rig->shocks[m_rig->free_shock];
	m_rig->free_shock++;
	return shock;
}

beam_t & RigSpawner::GetAndInitFreeBeam(node_t & node_1, node_t & node_2)
{
	beam_t & beam = GetFreeBeam();
	beam.p1 = & node_1;
	beam.p2 = & node_2;
	return beam;
}

node_t & RigSpawner::GetAndInitFreeNode(Ogre::Vector3 const & position)
{
	node_t & node = GetFreeNode();
	InitNode(node, position);
	return node;
}

void RigSpawner::SetBeamSpring(beam_t & beam, float spring)
{
	beam.k = spring;
}

void RigSpawner::SetBeamDamping(beam_t & beam, float damping)
{
	beam.d = damping;
}

void RigSpawner::CalcBoundingBoxes(rig_t *rig)
{
	rig->boundingBox.setExtents(
		rig->nodes[0].AbsPosition.x, 
		rig->nodes[0].AbsPosition.y, 
		rig->nodes[0].AbsPosition.z, 
		rig->nodes[0].AbsPosition.x, 
		rig->nodes[0].AbsPosition.y, 
		rig->nodes[0].AbsPosition.z
	);
	rig->collisionBoundingBoxes.clear();

	for (int i=0; i < rig->free_node; i++)
	{
		rig->boundingBox.merge(Ogre::Vector3(rig->nodes[i].AbsPosition));
		if (rig->nodes[i].collisionBoundingBoxID >= 0)
		{
			if ((unsigned int) rig->nodes[i].collisionBoundingBoxID >= rig->collisionBoundingBoxes.size())
			{
				rig->collisionBoundingBoxes.push_back(Ogre::AxisAlignedBox(rig->nodes[i].AbsPosition.x, rig->nodes[i].AbsPosition.y, rig->nodes[i].AbsPosition.z, rig->nodes[i].AbsPosition.x, rig->nodes[i].AbsPosition.y, rig->nodes[i].AbsPosition.z));
			} else
			{
				rig->collisionBoundingBoxes[rig->nodes[i].collisionBoundingBoxID].merge(rig->nodes[i].AbsPosition);
			}
		}
	}

	for (unsigned int i = 0; i < rig->collisionBoundingBoxes.size(); i++)
	{
		rig->collisionBoundingBoxes[i].setMinimum(rig->collisionBoundingBoxes[i].getMinimum() - Ogre::Vector3(0.05f, 0.05f, 0.05f));
		rig->collisionBoundingBoxes[i].setMaximum(rig->collisionBoundingBoxes[i].getMaximum() + Ogre::Vector3(0.05f, 0.05f, 0.05f));
	}

	rig->boundingBox.setMinimum(rig->boundingBox.getMinimum() - Ogre::Vector3(0.05f, 0.05f, 0.05f));
	rig->boundingBox.setMaximum(rig->boundingBox.getMaximum() + Ogre::Vector3(0.05f, 0.05f, 0.05f));

	rig->predictedBoundingBox = rig->boundingBox;
	rig->predictedCollisionBoundingBoxes = rig->collisionBoundingBoxes;
}


void RigSpawner::SetupDefaultSoundSources(Beam *vehicle)
{
	int trucknum = vehicle->trucknum;
	int smokeId = vehicle->smokeId;

#ifdef USE_OPENAL
	if (SoundScriptManager::getSingleton().isDisabled()) 
	{
		return;
	}

	//engine
	if (vehicle->engine != nullptr) /* Land vehicle */
	{
		if (vehicle->engine->type=='t')
		{
			AddSoundSourceInstance(vehicle, "tracks/default_diesel", smokeId);
			AddSoundSourceInstance(vehicle, "tracks/default_force", smokeId);
			AddSoundSourceInstance(vehicle, "tracks/default_brakes", 0);
			AddSoundSourceInstance(vehicle, "tracks/default_parkbrakes", 0);
			AddSoundSourceInstance(vehicle, "tracks/default_reverse_beep", 0);
		}
		if (vehicle->engine->type=='c')
			AddSoundSourceInstance(vehicle, "tracks/default_car", smokeId);
		if (vehicle->engine->hasturbo)
			AddSoundSourceInstance(vehicle, "tracks/default_turbo", smokeId);
		if (vehicle->engine->hasair)
			AddSoundSourceInstance(vehicle, "tracks/default_air_purge", 0);
		//starter
		AddSoundSourceInstance(vehicle, "tracks/default_starter", 0);
		// turn signals
		AddSoundSourceInstance(vehicle, "tracks/default_turn_signal", 0);
	}
	if (vehicle->driveable==TRUCK)
	{
		//horn
		if (vehicle->ispolice)
			AddSoundSourceInstance(vehicle, "tracks/default_police", 0);
		else
			AddSoundSourceInstance(vehicle, "tracks/default_horn", 0);
		//shift
			AddSoundSourceInstance(vehicle, "tracks/default_shift", 0);
	}
	//pump
	if (vehicle->hascommands)
	{
		AddSoundSourceInstance(vehicle, "tracks/default_pump", 0);
	}
	//antilock brake
	if (vehicle->alb_present)
	{
		AddSoundSourceInstance(vehicle, "tracks/default_antilock", 0);
	}
	//tractioncontrol
	if (vehicle->tc_present)
	{
		AddSoundSourceInstance(vehicle, "tracks/default_tractioncontrol", 0);
	}
	//screetch
	if ((vehicle->driveable==TRUCK || vehicle->driveable==AIRPLANE) && vehicle->free_wheel != 0)
	{
		AddSoundSourceInstance(vehicle, "tracks/default_screetch", 0);
	}
	//break & creak
	AddSoundSourceInstance(vehicle, "tracks/default_break", 0);
	AddSoundSourceInstance(vehicle, "tracks/default_creak", 0);
	//boat engine
	if (vehicle->driveable==BOAT)
	{
		if (vehicle->totalmass>50000.0)
			AddSoundSourceInstance(vehicle, "tracks/default_marine_large", smokeId);
		else
			AddSoundSourceInstance(vehicle, "tracks/default_marine_small", smokeId);
		//no start/stop engine for boats, so set sound always on!
		SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_ENGINE);
		SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_ENGINE, 0.5);
	}
	//airplane warnings
	if (vehicle->driveable==AIRPLANE)
	{
		AddSoundSourceInstance(vehicle, "tracks/default_gpws_10", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_gpws_20", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_gpws_30", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_gpws_40", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_gpws_50", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_gpws_100", 0);

		AddSoundSourceInstance(vehicle, "tracks/default_gpws_pullup", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_gpws_minimums", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_gpws_apdisconnect", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_aoa_warning", 0);

		AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat01", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat02", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat03", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat04", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat05", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat06", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat07", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat08", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat09", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat10", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat11", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat12", 0);
		AddSoundSourceInstance(vehicle, "tracks/default_aivionic_chat13", 0);
	}
	//airplane engines
	for (int i=0; i<vehicle->free_aeroengine && i<8; i++)
	{
		int turbojet_node = vehicle->aeroengines[i]->getNoderef();
		Ogre::String index_str = TOSTRING(i+1);

		if (vehicle->aeroengines[i]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOJET)
		{
			AddSoundSourceInstance(vehicle, "tracks/default_turbojet_start" + index_str, turbojet_node);
			AddSoundSourceInstance(vehicle, "tracks/default_turbojet_lopower" + index_str, turbojet_node);
			AddSoundSourceInstance(vehicle, "tracks/default_turbojet_hipower" + index_str, turbojet_node);
			if (((Turbojet*)(vehicle->aeroengines[i]))->afterburnable)
			{
				AddSoundSourceInstance(vehicle, "tracks/default_turbojet_afterburner" + index_str, turbojet_node);
			}
		}
		else if (vehicle->aeroengines[i]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOPROP)
		{
			if (((Turboprop*)vehicle->aeroengines[i])->is_piston)
			{
				AddSoundSourceInstance(vehicle, "tracks/default_pistonprop_start" + index_str, turbojet_node);
				AddSoundSourceInstance(vehicle, "tracks/default_pistonprop_lopower" + index_str, turbojet_node);
				AddSoundSourceInstance(vehicle, "tracks/default_pistonprop_hipower" + index_str, turbojet_node);
			}
			else
			{
				AddSoundSourceInstance(vehicle, "tracks/default_turboprop_start" + index_str, turbojet_node);
				AddSoundSourceInstance(vehicle, "tracks/default_turboprop_lopower" + index_str, turbojet_node);
				AddSoundSourceInstance(vehicle, "tracks/default_turboprop_hipower" + index_str, turbojet_node);
			}
		}
	}

	// linked sounds
	for (int i=0; i<vehicle->free_commands; i++)
	{
		AddSoundSource(vehicle, SoundScriptManager::getSingleton().createInstance(Ogre::String("tracks/linked/default_command/extend"), trucknum, NULL, SL_COMMAND, i), 0);
		AddSoundSource(vehicle, SoundScriptManager::getSingleton().createInstance(Ogre::String("tracks/linked/default_command/retract"), trucknum, NULL, SL_COMMAND, -i), 0);
	}

#endif //OPENAL
}
