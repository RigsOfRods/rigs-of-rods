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
	@file   RigDef_Serializer.cpp
	@author Petr Ohlidal
	@date   10/2014
*/

#include "RigDef_Serializer.h"

#include "RigDef_File.h"

#include <fstream>
#include <OgreStringConverter.h>

using namespace RigDef;
using namespace std;

Serializer::Serializer(boost::shared_ptr<RigDef::File> def_file, Ogre::String const & file_path):
	m_file_path(file_path),
	m_rig_def(def_file),
	m_float_precision(10),
	m_float_width(16) // .e+001 = 6characters
{}

Serializer::~Serializer()
{}

void Serializer::Serialize()
{
	// Open file
	m_stream.open(m_file_path);
	m_stream.precision(m_float_precision); // Permanent

	// Write header
	m_stream << m_rig_def->name << endl << endl;

	// Write banner
	m_stream
		<< "; ---------------------------------------------------------------------------- ;" << endl
		<< "; Rigs of Rods (www.rigsofrods.com)                                            ;" << endl
		<< "; =================================                                            ;" << endl
		<< ";                                                                              ;" << endl
		<< "; This is a rig definition file.                                               ;" << endl
		<< "; See http://www.rigsofrods.com/wiki/pages/Truck_Description_File for details. ;" << endl
		<< "; ---------------------------------------------------------------------------- ;" << endl
		<< endl;

	// Write individual elements
	ProcessDescription();
	ProcessAuthors();
	ProcessGlobals(m_rig_def->root_module);
	ProcessFileinfo();
	WriteFlags();

	// Section 'minimass'
	if (m_rig_def->_minimum_mass_set)
	{
		m_stream << "minimass" << endl << "\t" << m_rig_def->minimum_mass << endl << endl;
	}
	
	// Finalize
	m_stream << "end" << endl;
	m_stream.close();
}

/*

void Serializer::Process()
{
	if (m_rig_def->description.size() != 0)
	{
		for (auto itor = m_rig_def->description.begin(); itor != m_rig_def->description.end(); ++itor)
		{
		
		}
	}
}

*/

void Serializer::WriteFlags()
{
	if (m_rig_def->enable_advanced_deformation)
	{
		m_stream << "enable_advanced_deformation" << endl << endl;
	}
	if (m_rig_def->hide_in_chooser)
	{
		m_stream << "hideInChooser" << endl << endl;
	}
	if (m_rig_def->slide_nodes_connect_instantly)
	{
		m_stream << "slidenode_connect_instantly" << endl << endl;
	}
	if (m_rig_def->lockgroup_default_nolock)
	{
		m_stream << "lockgroup_default_nolock" << endl << endl;
	}
	if (m_rig_def->rollon)
	{
		m_stream << "rollon" << endl << endl;
	}
	if (m_rig_def->rescuer)
	{
		m_stream << "rescuer" << endl << endl;
	}
	if (m_rig_def->disable_default_sounds)
	{
		m_stream << "disabledefaultsounds" << endl << endl;
	}
	if (m_rig_def->forward_commands)
	{
		m_stream << "forwardcommands" << endl << endl;
	}
	if (m_rig_def->import_commands)
	{
		m_stream << "importcommands" << endl << endl;
	}
}

void Serializer::ProcessFileinfo()
{
	if (m_rig_def->file_info.get() != nullptr)
	{
		m_stream << "fileinfo ";
		// UID
		if (m_rig_def->file_info->_has_unique_id)
		{
			m_stream << m_rig_def->file_info->unique_id;
		}
		else
		{
			m_stream << "-1";
		}
		// Category ID
		if (m_rig_def->file_info->_has_category_id)
		{
			m_stream << ", " << m_rig_def->file_info->category_id;
		}
		else if (m_rig_def->file_info->_has_file_version_set)
		{
			m_stream << ", -1";
		}
		// Version
		if (m_rig_def->file_info->_has_file_version_set)
		{
			m_stream << ", " << m_rig_def->file_info->file_version;
		}

		m_stream << endl << endl;
	}
}

void Serializer::ProcessGuid()
{
	if (! m_rig_def->guid.empty())
	{
		m_stream << "guid " << m_rig_def->guid << endl << endl;
	}
}

void Serializer::ProcessDescription()
{
	if (m_rig_def->description.size() != 0)
	{
		m_stream << "description";
		for (auto itor = m_rig_def->description.begin(); itor != m_rig_def->description.end(); ++itor)
		{
			std::string line = *itor;
			m_stream << endl << line;
		}
		m_stream << endl << "end_description" << endl << endl;
	}
}

void Serializer::ProcessAuthors()
{
	for (auto itor = m_rig_def->authors.begin(); itor != m_rig_def->authors.end(); ++itor)
	{
		Author & def = *itor;
		m_stream << "author " << def.type << " " << def.name << " ";
		if (def._has_forum_account)
		{
			m_stream << def.forum_account_id << " ";
		}
		else
		{
			m_stream << "-1 ";
		}
		m_stream << def.email << endl;
	}
	m_stream << endl;
}

void Serializer::ProcessGlobals(boost::shared_ptr<RigDef::File::Module> module)
{
	if (module->globals.get() != nullptr)
	{
		m_stream 
			<< "globals" << endl
			<< "\t" << module->globals->dry_mass << ", "
			<< module->globals->cargo_mass << ", "
			<< module->globals->material_name << endl << endl;
	}
}
