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
	m_node_id_width(10),
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

	// Structure
	ProcessNodes();
	ProcessBeams();
	
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

void Serializer::ProcessBeams()
{
	// Group beams by presets
	std::map< BeamDefaults*, std::vector<Beam*> > beams_by_preset;
	auto itor_end = m_rig_def->root_module->beams.end(); 
	for (auto itor = m_rig_def->root_module->beams.begin(); itor != itor_end; ++itor)
	{
		Beam & beam = *itor;
		BeamDefaults* preset = beam.defaults.get();

		// Ensure preset is in map
		auto found_itor = beams_by_preset.find(preset);
		if (found_itor == beams_by_preset.end())
		{
			// Preset not in map, insert it and add beam.
			std::vector<Beam*> list;
			list.reserve(100);
			list.push_back(&beam);
			beams_by_preset.insert(std::make_pair(preset, list));
		}
		else
		{
			// Preset in map, just add beam.
			found_itor->second.push_back(&beam);
		}
	}

	// Write beams to file
	m_stream << "beams" << endl << endl;
	auto preset_itor_end = beams_by_preset.end();
	for (auto preset_itor = beams_by_preset.begin(); preset_itor != preset_itor_end; ++preset_itor)
	{
		// Write preset
		BeamDefaults* preset = preset_itor->first;
		ProcessBeamDefaults(preset);

		// Write beams
		std::vector<Beam*> & beam_list = preset_itor->second;
		auto beam_itor_end = beam_list.end();
		for (auto beam_itor = beam_list.begin(); beam_itor != beam_itor_end; ++beam_itor)
		{
			Beam & beam = *(*beam_itor);
			ProcessBeam(beam);
		}
	}

	// Empty line
	m_stream << endl;
}

void Serializer::ProcessBeamDefaults(BeamDefaults* beam_defaults)
{
	m_stream << "set_beam_defaults       "
		<< beam_defaults->springiness << ", "
		<< beam_defaults->damping_constant << ", "
		<< beam_defaults->deformation_threshold_constant << ", "
		<< beam_defaults->breaking_threshold_constant << ", "
		<< beam_defaults->visual_beam_diameter << ", "
		<< beam_defaults->beam_material_name << ", "
		<< beam_defaults->plastic_deformation_coefficient 
		<< endl;

	BeamDefaultsScale & scale = beam_defaults->scale;
	m_stream << "set_beam_defaults_scale "
		<< scale.springiness << ", "
		<< scale.damping_constant << ", "
		<< scale.deformation_threshold_constant << ", "
		<< scale.breaking_threshold_constant 
		<< endl;
}

void Serializer::ProcessBeam(Beam & beam)
{
	m_stream << "\t"
		<< std::setw(m_node_id_width) << beam.nodes[0].ToString() << ", "
		<< std::setw(m_node_id_width) << beam.nodes[1].ToString() << ", ";

	// Options
	if (beam.options == 0u)
	{
		m_stream << "n"; // Placeholder
	}
	else
	{
		if (BITMASK_IS_1(beam.options, Beam::OPTION_i_INVISIBLE))
		{
			m_stream << "i";
		}
		if (BITMASK_IS_1(beam.options, Beam::OPTION_r_ROPE))
		{
			m_stream << "r";
		}
		if (BITMASK_IS_1(beam.options, Beam::OPTION_s_SUPPORT))
		{
			m_stream << "s";
		}
	}

	if (beam._has_extension_break_limit)
	{
		m_stream << ", " << beam.extension_break_limit;
	}
	
	m_stream << endl;
}

void Serializer::ProcessNodes()
{
	// Group nodes by presets
	std::map< NodeDefaults*, std::vector<Node*> > nodes_by_presets;
	auto itor_end = m_rig_def->root_module->nodes.end(); 
	for (auto itor = m_rig_def->root_module->nodes.begin(); itor != itor_end; ++itor)
	{
		Node & node = *itor;
		NodeDefaults* preset = node.node_defaults.get();

		// Ensure preset is in map
		auto found_itor = nodes_by_presets.find(preset);
		if (found_itor == nodes_by_presets.end())
		{
			// Preset not in map, insert it and add node.
			std::vector<Node*> list;
			list.reserve(100);
			list.push_back(&node);
			nodes_by_presets.insert(std::make_pair(preset, list));
		}
		else
		{
			// Preset in map, just add node.
			found_itor->second.push_back(&node);
		}
	}

	// Write nodes to file
	m_stream << "nodes" << endl << endl;
	auto preset_itor_end = nodes_by_presets.end();
	for (auto preset_itor = nodes_by_presets.begin(); preset_itor != preset_itor_end; ++preset_itor)
	{
		// Write preset
		NodeDefaults* preset = preset_itor->first;
		ProcessNodeDefaults(preset);

		// Write nodes
		std::vector<Node*> & node_list = preset_itor->second;
		auto node_itor_end = node_list.end();
		for (auto node_itor = node_list.begin(); node_itor != node_itor_end; ++node_itor)
		{
			Node & node = *(*node_itor);
			ProcessNode(node);
		}
	}

	// Empty line
	m_stream << endl;
}

void Serializer::ProcessNodeDefaults(NodeDefaults* node_defaults)
{
	m_stream << "set_node_defaults "
		<< node_defaults->load_weight << ", "
		<< node_defaults->friction << ", "
		<< node_defaults->volume << ", "
		<< node_defaults->surface << ", ";

	ProcessNodeOptions(node_defaults->options);

	m_stream << endl;
}

void Serializer::ProcessNodeOptions(unsigned int options)
{
	// Mouse grab
	m_stream << (BITMASK_IS_1(options, Node::OPTION_m_NO_MOUSE_GRAB) ? "m" : "n");

	if (BITMASK_IS_1(options, Node::OPTION_b_EXTRA_BUOYANCY))
	{
		m_stream << "b";
	}
	if (BITMASK_IS_1(options, Node::OPTION_c_NO_GROUND_CONTACT))
	{
		m_stream << "c";
	}
	if (BITMASK_IS_1(options, Node::OPTION_f_NO_SPARKS))
	{
		m_stream << "f";
	}
	if (BITMASK_IS_1(options, Node::OPTION_h_HOOK_POINT))
	{
		m_stream << "h";
	}
	if (BITMASK_IS_1(options, Node::OPTION_l_LOAD_WEIGHT))
	{
		m_stream << "l";
	}
	if (BITMASK_IS_1(options, Node::OPTION_L_LOG))
	{
		m_stream << "L";
	}
	if (BITMASK_IS_1(options, Node::OPTION_p_NO_PARTICLES))
	{
		m_stream << "p";
	}
	if (BITMASK_IS_1(options, Node::OPTION_x_EXHAUST_POINT))
	{
		m_stream << "x";
	}
	if (BITMASK_IS_1(options, Node::OPTION_y_EXHAUST_DIRECTION))
	{
		m_stream << "y";
	}
}

void Serializer::ProcessNode(Node & node)
{
	m_stream 
		<< "\t" 
		<< std::setw(m_node_id_width) << node.id.ToString() << ", " 
		<< std::setw(m_float_width) << node.position.x << ", " 
		<< std::setw(m_float_width) << node.position.y << ", " 
		<< std::setw(m_float_width) << node.position.z << ", ";
	
	ProcessNodeOptions(node.options);
	
	// Load mass
	if (node._has_load_weight_override)
	{
		m_stream << ", " << node.load_weight_override;
	}
	m_stream << endl;
}

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
