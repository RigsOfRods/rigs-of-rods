/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.org/

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

#define PROCESS_SECTION_IN_ANY_MODULE(_KEYWORD_, _FIELD_, _FUNCTION_)                      \
{                                                                                          \
    SetCurrentKeyword(_KEYWORD_);                                                          \
    auto module_itor = m_selected_modules.begin();                                         \
    auto module_end  = m_selected_modules.end();                                           \
    for (; module_itor != module_end; ++module_itor)                                       \
    {                                                                                      \
        auto* def = module_itor->get()->_FIELD_.get();                                     \
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

#define PROCESS_SECTION_IN_ALL_MODULES(_KEYWORD_, _FIELD_, _FUNCTION_)                     \
{                                                                                          \
    SetCurrentKeyword(_KEYWORD_);                                                          \
    auto module_itor = m_selected_modules.begin();                                         \
    auto module_end  = m_selected_modules.end();                                           \
    for (; module_itor != module_end; ++module_itor)                                       \
    {                                                                                      \
        auto& field = module_itor->get()->_FIELD_;                                         \
        auto section_itor = field.begin();                                                 \
        auto section_end  = field.end();                                                   \
        for (; section_itor != section_end; ++section_itor)                                \
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
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_MANAGEDMATERIALS, managed_materials, ProcessManagedMaterial);

	// Section 'gobals' in any module
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_GLOBALS, globals, ProcessGlobals);

	// Section 'help' in any module.
	// NOTE: Must be done before "guisettings" (rig_t::helpmat override)
	ProcessHelp();

	// Section 'engine' in any module
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_ENGINE, engine, ProcessEngine);

	// Section 'engoption' in any module
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_ENGOPTION, engoption, ProcessEngoption);

	/* Section 'engturbo' in any module */
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_ENGTURBO, engturbo, ProcessEngturbo);

	// Section 'torquecurve' in any module.
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_TORQUECURVE, torque_curve, ProcessTorqueCurve);

	// Section 'brakes' in any module
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_BRAKES, brakes, ProcessBrakes);

	// Section 'guisettings' in any module
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_GUISETTINGS, gui_settings, ProcessGuiSettings);

	// ---------------------------- User-defined nodes ----------------------------

	// Sections 'nodes' & 'nodes2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_NODES, nodes, ProcessNode);

	// Old-format exhaust (defined by flags 'x/y' in section 'nodes', one per vehicle)
	if (m_rig->smokeId != 0 && m_rig->smokeRef != 0)
	{
		AddExhaust(m_rig->smokeId, m_rig->smokeRef, true, nullptr);
	}

	// ---------------------------- Node generating sections ----------------------------

	// Section 'cinecam'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CINECAM, cinecam, ProcessCinecam);

	// ---------------------------- Wheels (also generate nodes) ----------------------------

	// Section 'wheels'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WHEELS, wheels, ProcessWheel);

	// Section 'wheels2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WHEELS2, wheels_2, ProcessWheel2);

	// Section 'meshwheels'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_MESHWHEELS, mesh_wheels, ProcessMeshWheel);

	// Section 'meshwheels2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_MESHWHEELS2, mesh_wheels_2, ProcessMeshWheel2);

	// Section 'flexbodywheels'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FLEXBODYWHEELS, flex_body_wheels, ProcessFlexBodyWheel);

	// ---------------------------- User-defined beams ----------------------------
	//              (may reference any generated/user-defined node)

	// Section 'beams'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_BEAMS, beams, ProcessBeam);

	// Section 'shocks'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SHOCKS, shocks, ProcessShock);

	// Section 'shocks2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SHOCKS2, shocks_2, ProcessShock2);

	// Section 'commands' and 'commands2' (Use generated nodes)
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_COMMANDS2, commands_2, ProcessCommand);

	// Section 'hydros'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_HYDROS, hydros, ProcessHydro);

	// Section 'triggers'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TRIGGERS, triggers, ProcessTrigger);

	// Section 'ropes'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROPES, ropes, ProcessRope);

	// ---------------------------- Other ----------------------------

	// Section 'AntiLockBrakes' in any module.
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_ANTI_LOCK_BRAKES, anti_lock_brakes, ProcessAntiLockBrakes);

	// Section 'SlopeBrake' in any module.
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_SLOPE_BRAKE, slope_brake, ProcessSlopeBrake);
	
	// Sections 'flares' and 'flares2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FLARES2, flares_2, ProcessFlare2);

	// Section 'axles'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_AXLES, axles, ProcessAxle);

	// Section 'submeshes'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SUBMESH, submeshes, ProcessSubmesh);

	// Inline-section 'submesh_groundmodel' in any module
	ProcessSubmeshGroundmodel();

	// Section 'contacters'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CONTACTERS, contacters, ProcessContacter);

	// Section 'cameras'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CAMERAS, cameras, ProcessCamera);

	// Section 'hooks'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_HOOKS, hooks, ProcessHook);	

	// Section 'ties'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TIES, ties, ProcessTie);

	// Section 'ropables'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROPABLES, ropables, ProcessRopable);

	// Section 'wings'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_WINGS, wings, ProcessWing);

	// Section 'animators'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ANIMATORS, animators, ProcessAnimator);

	// Section 'materialflarebindings'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_MATERIALFLAREBINDINGS, material_flare_bindings, ProcessMaterialFlareBinding);

	// Section 'airbrakes'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_AIRBRAKES, airbrakes, ProcessAirbrake);

	// Section 'fusedrag'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FUSEDRAG, fusedrag, ProcessFusedrag);

	// Section 'turbojets'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TURBOJETS, turbojets, ProcessTurbojet);

	// Section 'videocamera'
	// !!! MUST be processed before "props", otherwise they won't work
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_VIDEOCAMERA, videocameras, ProcessVideoCamera);

	// Section 'props'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_PROPS, props, ProcessProp);

	// Section 'TractionControl' in any module.
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_TRACTION_CONTROL, traction_control, ProcessTractionControl);

	// Section 'rotators'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROTATORS, rotators, ProcessRotator);

	// Section 'rotators_2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_ROTATORS2, rotators_2, ProcessRotator2);

	// Section 'lockgroups'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_LOCKGROUPS, lockgroups, ProcessLockgroup);

	// Section 'railgroups'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_RAILGROUPS, railgroups, ProcessRailGroup);

	// Section 'slidenodes'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SLIDENODES, slidenodes, ProcessSlidenode);

	// Section 'particles'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_PARTICLES, particles, ProcessParticle);

	// Section 'cruisecontrol' in any module.
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_CRUISECONTROL, cruise_control, ProcessCruiseControl);

	// Section 'speedlimiter' in any module.
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_SPEEDLIMITER, speed_limiter, ProcessSpeedLimiter);

	// Section 'collisionboxes'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_COLLISIONBOXES, collision_boxes, ProcessCollisionBox);

	// Section 'exhausts'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_EXHAUSTS, exhausts, ProcessExhaust);

	// Section 'extcamera'
	PROCESS_SECTION_IN_ANY_MODULE(RigDef::File::KEYWORD_EXTCAMERA, ext_camera, ProcessExtCamera);

	// Section 'camerarail'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_CAMERARAIL, camera_rails, ProcessCameraRail);	

	// Section 'pistonprops'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_PISTONPROPS, pistonprops, ProcessPistonprop);

	// Sections 'turboprops' and 'turboprops2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_TURBOPROPS2, turboprops_2, ProcessTurboprop2);

	// Section 'screwprops'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SCREWPROPS, screwprops, ProcessScrewprop);

    // Section 'flexbodies' (Uses generated nodes)
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FLEXBODIES, flexbodies, ProcessFlexbody);

    // Section 'fixes'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_FIXES, fixes, ProcessFixedNode);

#ifdef USE_OPENAL

	// Section 'soundsources'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SOUNDSOURCES, soundsources, ProcessSoundSource);

	// Section 'soundsources2'
	PROCESS_SECTION_IN_ALL_MODULES(RigDef::File::KEYWORD_SOUNDSOURCES2, soundsources2, ProcessSoundSource2);

#endif // USE_OPENAL

	m_rig->loading_finished = true;

	// POST-PROCESSING
	FinalizeRig();

	// Pass ownership
	rig_t *rig = m_rig;
	m_rig = nullptr;
	return rig;
}
