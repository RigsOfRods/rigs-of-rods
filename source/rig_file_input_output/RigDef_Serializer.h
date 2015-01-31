/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2015 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/**
	@file   RigDef_Serializer.h
	@author Petr Ohlidal
	@date   10/2014
*/

#pragma once

#include "RigDef_File.h"

namespace RigDef
{

/**
	@class  Serializer
	@author Petr Ohlidal

	@brief Serializes the RigDef::File data structure to file.
*/
class Serializer
{

public:

	Serializer(boost::shared_ptr<RigDef::File> rig_def, Ogre::String const & file_path);

	virtual ~Serializer();

	void Serialize();

protected:

	void ProcessAuthors();
	void ProcessGlobals(File::Module* module);
	void ProcessDescription();
	void ProcessGuid();
	void ProcessFileinfo();
	void WriteFlags();
	void ProcessHelp(File::Module* module);

	// Audio video
	void ProcessCinecam(File::Module*);

	// Structure
	void ProcessNodes(File::Module*);
	void ProcessNode(Node & node);
	void ProcessNodeDefaults(NodeDefaults* node_defaults);
	void ProcessNodeOptions(unsigned int options);
	
	void ProcessBeams(File::Module*);
	void ProcessBeamDefaults(BeamDefaults* beam_defaults, const char* prefix = "");
	void ProcessBeam(Beam & beam);

	void ProcessShocks(File::Module*);
	void ProcessShocks2(File::Module*);
	void ProcessShock(Shock & def);
	void ProcessShock2(Shock2 & def);

	void ProcessHydros(File::Module*);
	void ProcessHydro(Hydro & def);
	void ProcessRotators(File::Module* module);
	void ProcessRotators2(File::Module* module);

	void ProcessCommands2(File::Module*);
	void ProcessCommand2(Command2 & def);
	void ProcessSlideNodes(File::Module* module);
	void ProcessRopes(File::Module* module);
	void ProcessFixes(File::Module* module);
	void ProcessTies(File::Module* module);

	// Land vehicle
	void ProcessEngine(File::Module* module);
	void ProcessEngoption(File::Module* module);
	void ProcessBrakes(File::Module* module);
	void ProcessAntiLockBrakes(File::Module* module);
	void ProcessTractionControl(File::Module* module);
	void ProcessSlopeBrake(File::Module* module);
	void ProcessTorqueCurve(File::Module* module);
	void ProcessCruiseControl(File::Module* module);
	void ProcessSpeedLimiter(File::Module* module);
	void ProcessAxles(File::Module* module);

	// Wheels
	void ProcessMeshWheels2(File::Module* module);
	void ProcessMeshWheels(File::Module* module);
	void ProcessWheels(File::Module* module);
	void ProcessWheels2(File::Module* module);
	void ProcessFlexBodyWheels(File::Module* module);

	// Features
	void ProcessAnimators(File::Module* module);
	void ProcessContacters(File::Module* module);
	void ProcessTriggers(File::Module* module);
	void ProcessLockgroups(File::Module* module);
	void ProcessHooks(File::Module* module);
	void ProcessRailGroups(File::Module* module);
	void ProcessRopables(File::Module* module);
	void ProcessParticles(File::Module* module);
	void ProcessCollisionBoxes(File::Module* module);
	void ProcessManagedMaterialsAndOptions(File::Module* module);
	void ProcessFlares2(File::Module* module);

	/* TODO: 
	5.5 Look & Feel

    5.5.5 Materialflarebindings
    5.5.6 Props
        5.5.6.1 Special Props
    5.5.7 Add_animation
    5.5.8 Flexbodies
        5.5.8.1 (sub-section) "Prop-like"
        5.5.8.2 (sub-directive) forset
        5.5.8.3 (sub-directive) disable_flexbody_shadow
        5.5.8.4 (sub-directive) flexbody_camera_mode
    5.5.9 Submesh
        5.5.9.1 (sub-section) texcoords
        5.5.9.2 (sub-section) cab
        5.5.9.3 (sub-directive) backmesh
    5.5.10 submesh_groundmodel
    5.5.11 Exhausts
    5.5.12 Sections
    5.5.13 Guisettings
    5.5.14 Set_skeleton_settings
    5.5.15 Videocamera
    5.5.16 Extcamera
    5.5.17 Camerarail
    5.5.18 Envmap

5.6 Sounds

    5.6.1 disabledefaultsounds
    5.6.2 Soundsources
    5.6.3 Soundsources2
        5.6.3.1 Default
            5.6.3.1.1 Engine (Diesel)
            5.6.3.1.2 Engine (Gasoline)
            5.6.3.1.3 Airplane (Prop)
            5.6.3.1.4 Airplane (Jet)
            5.6.3.1.5 Airplane (Piston)
            5.6.3.1.6 Marine (Large)
            5.6.3.1.7 Marine (Small)
    5.6.4 Soundsources3

5.7 Aircraft

    5.7.1 Wings
    5.7.2 Airbrakes
    5.7.3 Turboprops
    5.7.4 Fusedrag
    5.7.5 Turbojets
    5.7.6 Pistonprops

5.8 Boats

    5.8.1 Screwprops
	*/

protected:

	std::ofstream                     m_stream;
	Ogre::String                      m_file_path;
	boost::shared_ptr<RigDef::File>   m_rig_def;
	int                               m_float_precision;
	int                               m_float_width;
	int                               m_bool_width;
	int                               m_node_id_width;
	int                               m_command_key_width;
	int                               m_inertia_function_width;

	
};

} // namespace RigDef
