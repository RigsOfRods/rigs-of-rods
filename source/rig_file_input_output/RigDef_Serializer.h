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

	void ProcessNodes(File::Module*);

	void ProcessNode(Node & node);

	void ProcessNodeDefaults(NodeDefaults* node_defaults);

	void ProcessNodeOptions(unsigned int options);

	void ProcessBeams(File::Module*);

	void ProcessBeamDefaults(BeamDefaults* beam_defaults);

	void ProcessBeam(Beam & beam);

	void ProcessShocks(File::Module*);

	void ProcessShocks2(File::Module*);

	void ProcessShock(Shock & def);

	void ProcessShock2(Shock2 & def);

	void ProcessHydros(File::Module*);

	void ProcessHydro(Hydro & def);

	void ProcessCommands2(File::Module*);

	void ProcessCommand2(Command2 & def);

	void ProcessCinecam(File::Module*);

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
