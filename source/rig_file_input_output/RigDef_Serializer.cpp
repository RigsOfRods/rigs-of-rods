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
	m_inertia_function_width(10),
	m_bool_width(5), // strlen("false") == 5
	m_command_key_width(2),
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

	// Select source
	File::Module* source_module = m_rig_def->root_module.get();

	// Write individual elements
	ProcessDescription();
	ProcessAuthors();
	ProcessGlobals(source_module);
	ProcessFileinfo();
	WriteFlags();

	// Section 'minimass'
	if (m_rig_def->_minimum_mass_set)
	{
		m_stream << "minimass" << endl << "\t" << m_rig_def->minimum_mass << endl << endl;
	}

	// Structure
	ProcessNodes(source_module);
	ProcessBeams(source_module);
	ProcessShocks(source_module);
	ProcessShocks2(source_module);
	ProcessHydros(source_module);
	ProcessCommands2(source_module);

	// Features
	ProcessCinecam(source_module);
	
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

void Serializer::ProcessCinecam(File::Module* module)
{
	m_stream << "cinecam" << endl << endl;

	for (auto itor = module->cinecam.begin(); itor != module->cinecam.end(); ++itor)
	{
		m_stream << "\t"
			<< setw(m_float_width) << itor->position.x << ", "
			<< setw(m_float_width) << itor->position.y << ", "
			<< setw(m_float_width) << itor->position.z << ", ";
		m_stream
			<< setw(m_node_id_width) << itor->nodes[0].ToString() << ", "
			<< setw(m_node_id_width) << itor->nodes[1].ToString() << ", "
			<< setw(m_node_id_width) << itor->nodes[2].ToString() << ", "
			<< setw(m_node_id_width) << itor->nodes[3].ToString() << ", "
			<< setw(m_node_id_width) << itor->nodes[4].ToString() << ", "
			<< setw(m_node_id_width) << itor->nodes[5].ToString() << ", "
			<< setw(m_node_id_width) << itor->nodes[6].ToString() << ", "
			<< setw(m_node_id_width) << itor->nodes[7].ToString() << ", ";
		m_stream
			<< setw(m_float_width) << itor->spring << ", "
			<< setw(m_float_width) << itor->damping;
	}

	m_stream << endl; // Empty line
}

void Serializer::ProcessBeams(File::Module* module)
{
	// Group beams by presets
	std::map< BeamDefaults*, std::vector<Beam*> > beams_by_preset;
	auto itor_end = module->beams.end();
	for (auto itor = module->beams.begin(); itor != itor_end; ++itor)
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

void Serializer::ProcessShocks(File::Module* module)
{
	// Group beams by presets
	std::map< BeamDefaults*, std::vector<Shock*> > shocks_by_preset;
	auto itor_end = module->shocks.end(); 
	for (auto itor = module->shocks.begin(); itor != itor_end; ++itor)
	{
		Shock & shock = *itor;
		BeamDefaults* preset = shock.beam_defaults.get();

		// Ensure preset is in map
		auto found_itor = shocks_by_preset.find(preset);
		if (found_itor == shocks_by_preset.end())
		{
			// Preset not in map, insert it and add shock.
			std::vector<Shock*> list;
			list.reserve(100);
			list.push_back(&shock);
			shocks_by_preset.insert(std::make_pair(preset, list));
		}
		else
		{
			// Preset in map, just add shock.
			found_itor->second.push_back(&shock);
		}
	}

	// Write shocks to file
	m_stream << "shocks" << endl << endl;
	auto preset_itor_end = shocks_by_preset.end();
	for (auto preset_itor = shocks_by_preset.begin(); preset_itor != preset_itor_end; ++preset_itor)
	{
		// Write preset
		BeamDefaults* preset = preset_itor->first;
		ProcessBeamDefaults(preset);

		// Write shocks
		auto shock_list = preset_itor->second;
		auto shock_itor_end = shock_list.end();
		for (auto shock_itor = shock_list.begin(); shock_itor != shock_itor_end; ++shock_itor)
		{
			Shock & shock = *(*shock_itor);
			ProcessShock(shock);
		}
	}

	// Empty line
	m_stream << endl;
}

void Serializer::ProcessShocks2(File::Module* module)
{
	// Group beams by presets
	std::map< BeamDefaults*, std::vector<Shock2*> > shocks_by_preset;
	auto itor_end = module->shocks_2.end(); 
	for (auto itor = module->shocks_2.begin(); itor != itor_end; ++itor)
	{
		Shock2 & shock = *itor;
		BeamDefaults* preset = shock.beam_defaults.get();

		// Ensure preset is in map
		auto found_itor = shocks_by_preset.find(preset);
		if (found_itor == shocks_by_preset.end())
		{
			// Preset not in map, insert it and add shock.
			std::vector<Shock2*> list;
			list.reserve(100);
			list.push_back(&shock);
			shocks_by_preset.insert(std::make_pair(preset, list));
		}
		else
		{
			// Preset in map, just add shock.
			found_itor->second.push_back(&shock);
		}
	}

	// Write shocks to file
	m_stream << "shocks2" << endl << endl;
	auto preset_itor_end = shocks_by_preset.end();
	for (auto preset_itor = shocks_by_preset.begin(); preset_itor != preset_itor_end; ++preset_itor)
	{
		// Write preset
		BeamDefaults* preset = preset_itor->first;
		ProcessBeamDefaults(preset);

		// Write shocks
		auto shock_list = preset_itor->second;
		auto shock_itor_end = shock_list.end();
		for (auto shock_itor = shock_list.begin(); shock_itor != shock_itor_end; ++shock_itor)
		{
			Shock2 & shock = *(*shock_itor);
			ProcessShock2(shock);
		}
	}

	// Empty line
	m_stream << endl;
}

void Serializer::ProcessHydros(File::Module* module)
{
	// Group by presets
	std::map< BeamDefaults*, std::vector<Hydro*> > grouped_by_preset;
	auto itor_end = module->hydros.end(); 
	for (auto itor = module->hydros.begin(); itor != itor_end; ++itor)
	{
		Hydro & hydro = *itor;
		BeamDefaults* preset = hydro.beam_defaults.get();

		// Ensure preset is in map
		auto found_itor = grouped_by_preset.find(preset);
		if (found_itor == grouped_by_preset.end())
		{
			// Preset not in map, insert it and add hydro.
			std::vector<Hydro*> list;
			list.reserve(100);
			list.push_back(&hydro);
			grouped_by_preset.insert(std::make_pair(preset, list));
		}
		else
		{
			// Preset in map, just add hydro.
			found_itor->second.push_back(&hydro);
		}
	}

	// Write hydros to file
	m_stream << "hydros" << endl << endl;
	auto preset_itor_end = grouped_by_preset.end();
	for (auto preset_itor = grouped_by_preset.begin(); preset_itor != preset_itor_end; ++preset_itor)
	{
		// Write preset
		BeamDefaults* preset = preset_itor->first;
		ProcessBeamDefaults(preset);

		// Write hydros
		auto hydro_list = preset_itor->second;
		auto hydro_itor_end = hydro_list.end();
		for (auto hydro_itor = hydro_list.begin(); hydro_itor != hydro_itor_end; ++hydro_itor)
		{
			Hydro & hydro = *(*hydro_itor);
			ProcessHydro(hydro);
		}
	}

	// Empty line
	m_stream << endl;
}

void Serializer::ProcessCommands2(File::Module* module)
{
	// Group by presets and _format_version
	std::map< BeamDefaults*, std::vector<Command2*> > commands_by_preset;
	auto itor_end = module->commands_2.end(); 
	for (auto itor = module->commands_2.begin(); itor != itor_end; ++itor)
	{
		Command2 & command = *itor;
		BeamDefaults* preset = command.beam_defaults.get();

		// Ensure preset is in map
		auto found_itor = commands_by_preset.find(preset);
		if (found_itor == commands_by_preset.end())
		{
			// Preset not in map, insert it and add command.
			std::vector<Command2*> list;
			list.reserve(100);
			list.push_back(&command);
			commands_by_preset.insert(std::make_pair(preset, list));
		}
		else
		{
			// Preset in map, just add command.
			found_itor->second.push_back(&command);
		}
	}

	// Write section "commands2" to file (commands from section "commands" are converted)
	m_stream << "commands" << endl << endl;
	auto preset_itor_end = commands_by_preset.end();
	for (auto preset_itor = commands_by_preset.begin(); preset_itor != preset_itor_end; ++preset_itor)
	{
		// Write preset
		BeamDefaults* preset = preset_itor->first;
		ProcessBeamDefaults(preset);

		// Write hydros
		auto command_list = preset_itor->second;
		auto command_itor_end = command_list.end();
		for (auto command_itor = command_list.begin(); command_itor != command_itor_end; ++command_itor)
		{
			Command2 & command = *(*command_itor);
			ProcessCommand2(command);
		}
	}

	// Empty line
	m_stream << endl;
}

void Serializer::ProcessCommand2(Command2 & def)
{
	m_stream << "\t"
		<< std::setw(m_node_id_width) << def.nodes[0].ToString()    << ", "
		<< std::setw(m_node_id_width) << def.nodes[1].ToString()    << ", ";

	m_stream << std::setw(m_float_width) << def.shorten_rate        << ", ";
	m_stream << std::setw(m_float_width) << def.lengthen_rate       << ", ";
	m_stream << std::setw(m_float_width) << def.max_contraction     << ", "; // So-called 'shortbound'
	m_stream << std::setw(m_float_width) << def.max_extension       << ", "; // So-called 'longbound'
	m_stream << std::setw(m_command_key_width) << def.contract_key  << ", ";
	m_stream << std::setw(m_command_key_width) << def.extend_key    << ", ";

	// Options
	unsigned int options = def.options;
	if (options == 0)
	{
		m_stream << "n"; // Placeholder, does nothing.
	}
	else
	{
		if (BITMASK_IS_1(options, Command2::OPTION_c_AUTO_CENTER))
		{
			m_stream << "c";
		}
		if (BITMASK_IS_1(options, Command2::OPTION_f_NOT_FASTER))
		{
			m_stream << "f";
		}
		if (BITMASK_IS_1(options, Command2::OPTION_i_INVISIBLE))
		{
			m_stream << "i";
		}
		if (BITMASK_IS_1(options, Command2::OPTION_o_PRESS_ONCE_CENTER))
		{
			m_stream << "o";
		}
		if (BITMASK_IS_1(options, Command2::OPTION_p_PRESS_ONCE))
		{
			m_stream << "p";
		}
		if (BITMASK_IS_1(options, Command2::OPTION_r_ROPE))
		{
			m_stream << "r";
		}
	}
	m_stream << ", ";

	// Description
	m_stream << (def.description.length() > 0 ? def.description : "_") << ", ";
	
	// Inertia 
	m_stream << std::setw(m_float_width) << def.inertia.start_delay_factor        << ", ";
	m_stream << std::setw(m_float_width) << def.inertia.stop_delay_factor         << ", ";
	m_stream << std::setw(m_inertia_function_width) << def.inertia.start_function << ", ";
	m_stream << std::setw(m_inertia_function_width) << def.inertia.stop_function  << ", ";
	m_stream << std::setw(m_float_width) << def.affect_engine                     << ", ";
	m_stream << std::setw(m_bool_width) << (def.needs_engine ? "true" : "false");
}

void Serializer::ProcessHydro(Hydro & def)
{
	m_stream << "\t"
		<< std::setw(m_node_id_width) << def.nodes[0].ToString() << ", "
		<< std::setw(m_node_id_width) << def.nodes[1].ToString() << ", ";

	m_stream << std::setw(m_float_width) << def.lenghtening_factor      << ", ";
	if (def.options == 0)
	{
		m_stream << "n"; // Placeholder, does nothing
	}
	else
	{
		unsigned int options = def.options;
		if (BITMASK_IS_1(def.options, Hydro::OPTION_a_INPUT_AILERON))
		{
			m_stream << "a";
		}
		if (BITMASK_IS_1(def.options, Hydro::OPTION_e_INPUT_ELEVATOR))
		{
			m_stream << "e";
		}
		if (BITMASK_IS_1(def.options, Hydro::OPTION_g_INPUT_ELEVATOR_RUDDER))
		{
			m_stream << "g";
		}
		if (BITMASK_IS_1(def.options, Hydro::OPTION_h_INPUT_InvELEVATOR_RUDDER))
		{
			m_stream << "h";
		}
		if (BITMASK_IS_1(def.options, Hydro::OPTION_i_INVISIBLE))
		{
			m_stream << "i";
		}
		if (BITMASK_IS_1(def.options, Hydro::OPTION_r_INPUT_RUDDER))
		{
			m_stream << "r";
		}
		if (BITMASK_IS_1(def.options, Hydro::OPTION_s_DISABLE_ON_HIGH_SPEED))
		{
			m_stream << "s";
		}
		if (BITMASK_IS_1(def.options, Hydro::OPTION_u_INPUT_AILERON_ELEVATOR))
		{
			m_stream << "u";
		}
		if (BITMASK_IS_1(def.options, Hydro::OPTION_v_INPUT_InvAILERON_ELEVATOR))
		{
			m_stream << "v";
		}
		if (BITMASK_IS_1(def.options, Hydro::OPTION_x_INPUT_AILERON_RUDDER))
		{
			m_stream << "x";
		}
		if (BITMASK_IS_1(def.options, Hydro::OPTION_y_INPUT_InvAILERON_RUDDER))
		{
			m_stream << "y";
		}
	}
	m_stream << ", ";

	// Inertia
	OptionalInertia & inertia = def.inertia;
	m_stream << std::setw(m_float_width) << inertia.start_delay_factor  << ", ";
	m_stream << std::setw(m_float_width) << inertia.stop_delay_factor   << ", ";
	m_stream << std::setw(m_inertia_function_width) << inertia.start_function << ", ";
	m_stream << std::setw(m_inertia_function_width) << inertia.stop_function;
}

void Serializer::ProcessShock(Shock & def)
{
	m_stream << "\t"
		<< std::setw(m_node_id_width) << def.nodes[0].ToString() << ", "
		<< std::setw(m_node_id_width) << def.nodes[1].ToString() << ", ";
	m_stream << std::setw(m_float_width) << def.spring_rate      << ", ";
	m_stream << std::setw(m_float_width) << def.damping          << ", ";
	m_stream << std::setw(m_float_width) << def.short_bound      << ", ";
	m_stream << std::setw(m_float_width) << def.long_bound       << ", ";
	m_stream << std::setw(m_float_width) << def.precompression   << ", ";

	// Options
	if (def.options == 0)
	{
		m_stream << "n"; // Placeholder
	}
	else
	{
		if (BITMASK_IS_1(def.options, Shock::OPTION_i_INVISIBLE))
		{
			m_stream << "i";
		}
		if (BITMASK_IS_1(def.options, Shock::OPTION_m_METRIC))
		{
			m_stream << "m";
		}
		if (BITMASK_IS_1(def.options, Shock::OPTION_L_ACTIVE_LEFT))
		{
			m_stream << "L";
		}
		if (BITMASK_IS_1(def.options, Shock::OPTION_R_ACTIVE_RIGHT))
		{
			m_stream << "R";
		}
	}

	// Empty line
	m_stream << endl;
}

void Serializer::ProcessShock2(Shock2 & def)
{
	m_stream << "\t"
		<< std::setw(m_node_id_width) << def.nodes[0].ToString() << ", "
		<< std::setw(m_node_id_width) << def.nodes[1].ToString() << ", ";

	m_stream << std::setw(m_float_width) << def.spring_in                  << ", ";
	m_stream << std::setw(m_float_width) << def.damp_in                    << ", ";
	m_stream << std::setw(m_float_width) << def.progress_factor_spring_in  << ", ";
	m_stream << std::setw(m_float_width) << def.progress_factor_damp_in    << ", ";

	m_stream << std::setw(m_float_width) << def.spring_out                 << ", ";
	m_stream << std::setw(m_float_width) << def.damp_out                   << ", ";
	m_stream << std::setw(m_float_width) << def.progress_factor_spring_out << ", ";
	m_stream << std::setw(m_float_width) << def.progress_factor_damp_out   << ", ";

	m_stream << std::setw(m_float_width) << def.short_bound                << ", ";
	m_stream << std::setw(m_float_width) << def.long_bound                 << ", ";
	m_stream << std::setw(m_float_width) << def.precompression             << ", ";

	// Options
	if (def.options != 0)
	{
		m_stream << "n"; // Placeholder
	}
	else
	{
		if (BITMASK_IS_1(def.options, Shock2::OPTION_i_INVISIBLE))
		{
			m_stream << "i";
		}
		if (BITMASK_IS_1(def.options, Shock2::OPTION_m_METRIC))
		{
			m_stream << "m";
		}
		if (BITMASK_IS_1(def.options, Shock2::OPTION_M_ABSOLUTE_METRIC))
		{
			m_stream << "M";
		}
		if (BITMASK_IS_1(def.options, Shock2::OPTION_s_SOFT_BUMP_BOUNDS))
		{
			m_stream << "s";
		}
	}

	// Empty line
	m_stream << endl;
}

void Serializer::ProcessBeamDefaults(BeamDefaults* beam_defaults)
{
	if (beam_defaults == nullptr)
	{
		return;
	}
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

void Serializer::ProcessNodes(File::Module* module)
{
	// Group nodes by presets
	std::map< NodeDefaults*, std::vector<Node*> > nodes_by_presets;
	auto itor_end = module->nodes.end(); 
	for (auto itor = module->nodes.begin(); itor != itor_end; ++itor)
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
	m_stream << "set_node_defaults ";
	if (node_defaults == nullptr)
	{
		m_stream << "-1, -1, -1, -1, n" << endl;
		return;
	}

	m_stream
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

void Serializer::ProcessGlobals(File::Module* module)
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
