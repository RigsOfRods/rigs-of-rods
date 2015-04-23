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
/*

/*	
	@file   
	@brief  Implements part of RigSpawner class. Code separated for easier debugging.
	@author Petr Ohlidal
	@date   12/2013
*/

#include "RigSpawner.h"

#include "Beam.h"

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

	// Vehicle name
	m_rig->realtruckname = m_file->name;

	// Flags in root module
	m_rig->forwardcommands             = m_file->forward_commands;
	m_rig->importcommands              = m_file->import_commands;
	m_rig->wheel_contact_requested     = m_file->rollon;
	m_rig->rescuer                     = m_file->rescuer;
	m_rig->disable_default_sounds      = m_file->disable_default_sounds;
	m_rig->hideInChooser               = m_file->hide_in_chooser;
	m_rig->slideNodesConnectInstantly  = m_file->slide_nodes_connect_instantly;

	// Section 'authors' in root module
	ProcessAuthors();

	// Section 'fileinfo' in root module
	ProcessFileInfo();
	
	// Section 'guid' in root module
	if (! m_file->guid.empty())
	{
		strncpy(m_rig->guid, m_file->guid.c_str(), 128);
	}

	// Section 'minimass' in root module
	if (m_file->_minimum_mass_set)
	{
		m_rig->minimass = m_file->minimum_mass;
	}

	// Section 'description'
	m_rig->description.assign(m_file->description.begin(), m_file->description.end());

	// Section 'fileformatversion' in root module
	m_rig->fileformatversion = m_file->file_format_version;

	// Section 'managedmaterials'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_MANAGEDMATERIALS, RigDef::ManagedMaterial, managed_materials, ProcessManagedMaterial);

	// Section 'gobals' in any module
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_GLOBALS, RigDef::Globals, globals, ProcessGlobals);

	// Section 'help' in any module.
	// NOTE: Must be done before "guisettings" (rig_t::helpmat override)
	ProcessHelp();

	// Section 'engine' in any module
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_ENGINE, RigDef::Engine, engine, ProcessEngine);

	// Section 'engoption' in any module
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_ENGOPTION, RigDef::Engoption, engoption, ProcessEngoption);

	// Section 'torquecurve' in any module.
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_TORQUECURVE, RigDef::TorqueCurve, torque_curve, ProcessTorqueCurve);

	// Section 'brakes' in any module
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_BRAKES, RigDef::Brakes, brakes, ProcessBrakes);

	// Section 'guisettings' in any module
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_GUISETTINGS, RigDef::GuiSettings, gui_settings, ProcessGuiSettings);

	// ---------------------------- Nodes & Beams ----------------------------

	// Sections 'nodes' & 'nodes2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_NODES, RigDef::Node, nodes, ProcessNode);

	// Old-format exhaust (defined by flags 'x/y' in section 'nodes', one per vehicle)
	if (m_rig->smokeId != 0 && m_rig->smokeRef != 0)
	{
		AddExhaust(m_rig->smokeId, m_rig->smokeRef, true, nullptr);
	}

	// Section 'beams'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_BEAMS, RigDef::Beam, beams, ProcessBeam);

	// ---------------------------- Node & Beam generating sections ----------------------------

	// Section 'shocks'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SHOCKS, RigDef::Shock, shocks, ProcessShock);

	// Section 'shocks2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SHOCKS2, RigDef::Shock2, shocks_2, ProcessShock2);

	// Section 'commands' and 'commands2' (Use generated nodes)
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_COMMANDS2, RigDef::Command2, commands_2, ProcessCommand);

	// Section 'hydros'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_HYDROS, RigDef::Hydro, hydros, ProcessHydro);

	// Section 'cinecam'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CINECAM, RigDef::Cinecam, cinecam, ProcessCinecam);

	// ---------------------------- Wheels ----------------------------

	// Section 'wheels'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WHEELS, RigDef::Wheel, wheels, ProcessWheel);

	// Section 'wheels2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WHEELS2, RigDef::Wheel2, wheels_2, ProcessWheel2);

	// Section 'meshwheels'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_MESHWHEELS, RigDef::MeshWheel, mesh_wheels, ProcessMeshWheel);

	// Section 'meshwheels2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_MESHWHEELS2, RigDef::MeshWheel2, mesh_wheels_2, ProcessMeshWheel2);

	// Section 'flexbodywheels'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FLEXBODYWHEELS, RigDef::FlexBodyWheel, flex_body_wheels, ProcessFlexBodyWheel);

	// ---------------------------- Other ----------------------------

	// Section 'AntiLockBrakes' in any module.
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_ANTI_LOCK_BRAKES, RigDef::AntiLockBrakes, anti_lock_brakes, ProcessAntiLockBrakes);

	// Section 'SlopeBrake' in any module.
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_SLOPE_BRAKE, RigDef::SlopeBrake, slope_brake, ProcessSlopeBrake);
	
	// Sections 'flares' and 'flares2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FLARES2, RigDef::Flare2, flares_2, ProcessFlare2);

	// Section 'axles'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_AXLES, RigDef::Axle, axles, ProcessAxle);

	// Section 'flexbodies' (Uses generated nodes)
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FLEXBODIES, boost::shared_ptr<RigDef::Flexbody>, flexbodies, ProcessFlexbody);

	// Section 'submeshes'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SUBMESH, RigDef::Submesh, submeshes, ProcessSubmesh);

	// Inline-section 'submesh_groundmodel' in any module
	ProcessSubmeshGroundmodel();

	// Section 'contacters'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CONTACTERS, RigDef::Node::Id, contacters, ProcessContacter);

	// Section 'cameras'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CAMERAS, RigDef::Camera, cameras, ProcessCamera);

	// Section 'hooks'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_HOOKS, RigDef::Hook, hooks, ProcessHook);	

	// Section 'ties'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TIES, RigDef::Tie, ties, ProcessTie);

	// Section 'ropables'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROPABLES, RigDef::Ropable, ropables, ProcessRopable);

	// Section 'wings'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WINGS, RigDef::Wing, wings, ProcessWing);

	// Section 'animators'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ANIMATORS, RigDef::Animator, animators, ProcessAnimator);

	// Section 'triggers'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TRIGGERS, RigDef::Trigger, triggers, ProcessTrigger);

	// Section 'slidenodes'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SLIDENODES, RigDef::SlideNode, slidenodes, ProcessSlidenode);

	// Section 'materialflarebindings'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_MATERIALFLAREBINDINGS, RigDef::MaterialFlareBinding, material_flare_bindings, ProcessMaterialFlareBinding);

	// Section 'airbrakes'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_AIRBRAKES, RigDef::Airbrake, airbrakes, ProcessAirbrake);

	// Section 'fusedrag'
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_FUSEDRAG, RigDef::Fusedrag, fusedrag, ProcessFusedrag);

	// Section 'turbojets'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TURBOJETS, RigDef::Turbojet, turbojets, ProcessTurbojet);

	// Section 'videocamera'
	// !!! MUST be processed before "props", otherwise they won't work
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_VIDEOCAMERA, RigDef::VideoCamera, videocameras, ProcessVideoCamera);

	// Section 'props'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_PROPS, RigDef::Prop, props, ProcessProp);

	// Section 'TractionControl' in any module.
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_TRACTION_CONTROL, RigDef::TractionControl, traction_control, ProcessTractionControl);

	// Section 'rotators'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROTATORS, RigDef::Rotator, rotators, ProcessRotator);

	// Section 'rotators_2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROTATORS2, RigDef::Rotator2, rotators_2, ProcessRotator2);

	// Section 'rotators_2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROTATORS2, RigDef::Rotator2, rotators_2, ProcessRotator2);

	// Section 'lockgroups'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_LOCKGROUPS, RigDef::Lockgroup, lockgroups, ProcessLockgroup);

	// Section 'railgroups'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_RAILGROUPS, RigDef::RailGroup, railgroups, ProcessRailGroup);

	// Section 'ropes'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROPES, RigDef::Rope, ropes, ProcessRope);

	// Section 'particles'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_PARTICLES, RigDef::Particle, particles, ProcessParticle);

	// Section 'cruisecontrol' in any module.
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_CRUISECONTROL, RigDef::CruiseControl, cruise_control, ProcessCruiseControl);

	// Section 'cruisecontrol' in any module.
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_SPEEDLIMITER, RigDef::SpeedLimiter, speed_limiter, ProcessSpeedLimiter);

	// Section 'collisionboxes'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_COLLISIONBOXES, RigDef::CollisionBox, collision_boxes, ProcessCollisionBox);

	// Section 'exhausts'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_EXHAUSTS, RigDef::Exhaust, exhausts, ProcessExhaust);

	// Section 'extcamera'
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_EXTCAMERA, RigDef::ExtCamera, ext_camera, ProcessExtCamera);

	// Section 'camerarail'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CAMERARAIL, RigDef::CameraRail, camera_rails, ProcessCameraRail);	

	// Section 'pistonprops'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_PISTONPROPS, RigDef::Pistonprop, pistonprops, ProcessPistonprop);

	// Sections 'turboprops' and 'turboprops2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TURBOPROPS2, RigDef::Turboprop2, turboprops_2, ProcessTurboprop2);

	// Section 'screwprops'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SCREWPROPS, RigDef::Screwprop, screwprops, ProcessScrewprop);

	// Section 'set_skeleton_settings'
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_SET_SKELETON_SETTINGS, RigDef::SkeletonSettings, skeleton_settings, ProcessSkeletonSettings);

#ifdef USE_OPENAL

	// Section 'soundsources'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SOUNDSOURCES, RigDef::SoundSource, soundsources, ProcessSoundSource);

	// Section 'soundsources2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SOUNDSOURCES2, RigDef::SoundSource2, soundsources2, ProcessSoundSource2);

#endif // USE_OPENAL

	m_rig->loading_finished = true;

	// POST-PROCESSING
	FinalizeRig();

	// Pass ownership
	rig_t *rig = m_rig;
	m_rig = nullptr;
	return rig;
}
