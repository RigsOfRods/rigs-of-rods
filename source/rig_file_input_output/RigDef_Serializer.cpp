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
	m_node_id_width(5),
	m_float_precision(6),
	m_inertia_function_width(10),
	m_bool_width(5), // strlen("false") == 5
	m_command_key_width(2),
	m_float_width(10)
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
	ProcessManagedMaterialsAndOptions(source_module);

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
	ProcessSlideNodes(source_module);
	ProcessTies(source_module);
	ProcessRopes(source_module);
	ProcessFixes(source_module);

	// Wheels
	ProcessMeshWheels(source_module);
	ProcessMeshWheels2(source_module);
	ProcessWheels(source_module);
	ProcessWheels2(source_module);
	ProcessFlexBodyWheels(source_module);

	// Driving
	ProcessEngine(source_module);
	ProcessEngoption(source_module);
	ProcessBrakes(source_module);
	ProcessAntiLockBrakes(source_module);
	ProcessTractionControl(source_module);
	ProcessSlopeBrake(source_module);
	ProcessTorqueCurve(source_module);
	ProcessCruiseControl(source_module);
	ProcessSpeedLimiter(source_module);
	ProcessAxles(source_module);

	// Features
	ProcessCinecam(source_module);
	ProcessAnimators(source_module);
	ProcessContacters(source_module);
	ProcessTriggers(source_module);
	ProcessLockgroups(source_module);
	ProcessHooks(source_module);
	ProcessRailGroups(source_module);
	ProcessRopables(source_module);
	ProcessParticles(source_module);
	ProcessCollisionBoxes(source_module);
	// TODO: detacher_group
	// TODO: rigidifiers
	ProcessFlares2(source_module);
	
	// Finalize
	m_stream << "end" << endl;
	m_stream.close();
}

/* TEMPLATE
void Serializer::ProcessHooks(File::Module* module)
{
	if (module->hooks.empty())
	{
		return;
	}
	m_stream << "hooks" << endl;
	auto end_itor = module->hooks.end();
	for (auto itor = module->hooks.begin(); itor != end_itor; ++itor)
	{
		RigDef::Hook & def = *itor;

		m_stream << "\n\t" << def.node.ToString();
	}
	m_stream << endl << endl; // Empty line
}
*/

void Serializer::ProcessFlares2(File::Module* module)
{
	if (module->flares_2.empty())
	{
		return;
	}
	m_stream << "flares2" << endl;
	auto end_itor = module->flares_2.end();
	for (auto itor = module->flares_2.begin(); itor != end_itor; ++itor)
	{
		RigDef::Flare2 & def = *itor;

		m_stream << "\n\t" << def.reference_node.ToString()
			<< ", " << def.x
			<< ", " << def.y
			<< ", " << def.offset.x
			<< ", " << def.offset.y
			<< ", " << def.type
			<< ", " << def.control_number
			<< ", " << def.blink_delay_milis
			<< ", " << def.size
			<< " " << def.material_name;
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessManagedMaterialsAndOptions(File::Module* module)
{
	if (module->managed_materials.empty())
	{
		return;
	}
	m_stream << "managedmaterials" << endl;
	auto end_itor = module->managed_materials.end();
	bool first = true;
	ManagedMaterialsOptions mm_options;
	for (auto itor = module->managed_materials.begin(); itor != end_itor; ++itor)
	{
		RigDef::ManagedMaterial & def = *itor;

		if (first || (mm_options.double_sided != def.options.double_sided))
		{
			mm_options.double_sided = def.options.double_sided;
			m_stream << "\n\tset_managedmaterials_options " << (int) mm_options.double_sided;
		}
		// Name
		m_stream << "\n\t" << def.name << " ";
		// Type
		switch (def.type)
		{
		case ManagedMaterial::TYPE_FLEXMESH_STANDARD:
			m_stream << "flexmesh_standard ";
			break;
		case ManagedMaterial::TYPE_FLEXMESH_TRANSPARENT:
			m_stream << "flexmesh_transparent ";
			break;
		case ManagedMaterial::TYPE_MESH_STANDARD:
			m_stream << "mesh_standard ";
			break;
		case ManagedMaterial::TYPE_MESH_TRANSPARENT:
			m_stream << "mesh_transparent ";
			break;
		default:
			;
		}
		// Diffuse texture filename
		m_stream << def.diffuse_map << " ";
		// Diffuse damage-texture filename
		if (def.type == ManagedMaterial::TYPE_FLEXMESH_STANDARD || def.type == ManagedMaterial::TYPE_FLEXMESH_TRANSPARENT)
		{
			m_stream << (def.damaged_diffuse_map.empty() ? "-" : def.damaged_diffuse_map) << " ";
		}
		// Specular texture
		m_stream << (def.specular_map.empty() ? "-" : def.specular_map);

		first = false;
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessCollisionBoxes(File::Module* module)
{
	if (module->collision_boxes.empty())
	{
		return;
	}
	m_stream << "collisionboxes" << endl;
	auto end_itor = module->collision_boxes.end();
	for (auto itor = module->collision_boxes.begin(); itor != end_itor; ++itor)
	{
		RigDef::CollisionBox & def = *itor;

		auto nodes_end = def.nodes.end();
		auto node_itor = def.nodes.begin();
		m_stream << node_itor->ToString();
		++node_itor;
		for (; node_itor != nodes_end; ++node_itor)
		{
			m_stream << ", " << node_itor->ToString();
		}
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessAxles(File::Module* module)
{
	if (module->axles.empty())
	{
		return;
	}
	m_stream << "axles" << endl;
	auto end_itor = module->axles.end();
	for (auto itor = module->axles.begin(); itor != end_itor; ++itor)
	{
		RigDef::Axle & def = *itor;

		m_stream << "\n\t"
			<< "w1(" << def.wheels[0][0].ToString() << " " << def.wheels[0][1].ToString() << "), "
			<< "w2(" << def.wheels[1][0].ToString() << " " << def.wheels[1][1].ToString() << ")";
		if (! def.options.empty())
		{
			m_stream << ", d(";
			auto end = def.options.end();
			for (auto itor = def.options.begin(); itor != end; ++itor)
			{
				m_stream << *itor;
			}
			m_stream << ")";
		}
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessCruiseControl(File::Module* module)
{
	if (! module->cruise_control)
	{
		return;
	}
	m_stream << "cruisecontrol " 
		<< module->cruise_control->min_speed << ", " 
		<< (int) module->cruise_control->autobrake
		<< endl << endl;
}

void Serializer::ProcessSpeedLimiter(File::Module* module)
{
	if (! module->speed_limiter)
	{
		return;
	}
	m_stream << "speedlimiter " 
		<< module->speed_limiter->max_speed
		<< endl << endl;
}

void Serializer::ProcessTorqueCurve(File::Module* module)
{
	if (! module->torque_curve)
	{
		return;
	}
	m_stream << "torquecurve" << endl;
	if (module->torque_curve->predefined_func_name.empty())
	{
		auto itor_end = module->torque_curve->samples.end();
		auto itor = module->torque_curve->samples.begin();
		for (; itor != itor_end; ++itor)
		{
			m_stream << "\n\t" << itor->power << ", " << itor->torque_percent;
		}
	}
	else
	{
		m_stream << "\n\t" << module->torque_curve->predefined_func_name;
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessParticles(File::Module* module)
{
	if (module->particles.empty())
	{
		return;
	}
	m_stream << "particles" << endl;
	auto end_itor = module->particles.end();
	for (auto itor = module->particles.begin(); itor != end_itor; ++itor)
	{
		RigDef::Particle & def = *itor;

		m_stream << "\n\t" 
			<< setw(m_node_id_width) << def.emitter_node.ToString() << ", "
			<< setw(m_node_id_width) << def.reference_node.ToString() << ", "
			<< def.particle_system_name;
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessRopables(File::Module* module)
{
	if (module->ropables.empty())
	{
		return;
	}
	m_stream << "ropables" << endl;
	auto end_itor = module->ropables.end();
	for (auto itor = module->ropables.begin(); itor != end_itor; ++itor)
	{
		RigDef::Ropable & def = *itor;

		m_stream << "\n\t" << def.node.ToString()
			<< ", " << def.group
			<< ", " << (int) def.multilock;
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessTies(File::Module* module)
{
	if (module->ties.empty())
	{
		return;
	}
	m_stream << "ties" << endl;
	auto end_itor = module->ties.end();
	for (auto itor = module->ties.begin(); itor != end_itor; ++itor)
	{
		RigDef::Tie & def = *itor;

		m_stream << "\n\t" << def.root_node.ToString()
			<< ", " << setw(m_float_width) << def.max_reach_length
			<< ", " << setw(m_float_width) << def.auto_shorten_rate
			<< ", " << setw(m_float_width) << def.min_length
			<< ", " << setw(m_float_width) << def.max_length
			<< ", " << (def.options == Tie::OPTIONS_INVISIBLE ? "i" : "n") 
			<< ", " << setw(m_float_width) << def.max_stress
			<< ", " << def.group;
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessFixes(File::Module* module)
{
	if (module->fixes.empty())
	{
		return;
	}
	m_stream << "fixes" << endl;
	auto end_itor = module->fixes.end();
	for (auto itor = module->fixes.begin(); itor != end_itor; ++itor)
	{
		m_stream << "\n\t" << setw(m_node_id_width) << itor->ToString();
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessRopes(File::Module* module)
{
	if (module->ropes.empty())
	{
		return;
	}
	m_stream << "ropes" << endl;
	auto end_itor = module->ropes.end();
	bool first = true;
	BeamDefaults* beam_defaults = nullptr;
	for (auto itor = module->ropes.begin(); itor != end_itor; ++itor)
	{
		RigDef::Rope & def = *itor;

		if (first || (def.beam_defaults.get() != beam_defaults))
		{
			ProcessBeamDefaults(def.beam_defaults.get(), "\t");
		}

		m_stream << "\n\t" 
			<< setw(m_node_id_width) << def.root_node.ToString() << ", "
			<< setw(m_node_id_width) << def.end_node.ToString();
		if (def.invisible)
		{
			m_stream << ", i";
		}
		first = false;
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessRailGroups(File::Module* module)
{
	if (module->railgroups.empty())
	{
		return;
	}
	m_stream << "railgroups" << endl << endl;
	auto end_itor = module->railgroups.end();
	for (auto itor = module->railgroups.begin(); itor != end_itor; ++itor)
	{
		RigDef::RailGroup & def = *itor;

		m_stream << "\n\t" << def.id;
		auto node_end = def.node_list.end();
		for (auto node_itor = def.node_list.begin(); node_itor != node_end; ++node_itor)
		{
			m_stream << ", " << node_itor->start.ToString();
			if (node_itor->IsRange())
			{
				m_stream << " - " << node_itor->end.ToString();
			}
		}
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessSlideNodes(File::Module* module)
{
	if (module->slidenodes.empty())
	{
		return;
	}
	m_stream << "slidenodes" << endl << endl;
	auto end_itor = module->slidenodes.end();
	for (auto itor = module->slidenodes.begin(); itor != end_itor; ++itor)
	{
		RigDef::SlideNode & def = *itor;

		m_stream << "\n\t" << def.slide_node.ToString();

		// Define rail - either list of nodes, or raigroup ID
		if (!def.rail_node_ranges.empty())
		{
			auto end = def.rail_node_ranges.end();
			auto itor = def.rail_node_ranges.begin();
			for (; itor != end; ++itor)
			{
				m_stream << ", " << itor->start.ToString();
				if (itor->IsRange())
				{
					m_stream << " - " << itor->end.ToString();
				}
			}
		}
		else
		{
			m_stream << ", g" << def.railgroup_id;
		}

		// Params
		m_stream
			<< ", s" << def.spring_rate
			<< ", b" << def.break_force
			<< ", t" << def.tolerance
			<< ", r" << def.attachment_rate
			<< ", d" << def.max_attachment_distance;

		// Constraint flags (cX)
		     if (def.HasConstraint_a_AttachAll())     { m_stream << ", ca"; }
		else if (def.HasConstraint_f_AttachForeign()) { m_stream << ", cf"; }
		else if (def.HasConstraint_s_AttachSelf())    { m_stream << ", cs"; }
		else if (def.HasConstraint_n_AttachNone())    { m_stream << ", cn"; }
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessHooks(File::Module* module)
{
	if (module->hooks.empty())
	{
		return;
	}
	m_stream << "hooks" << endl << endl;
	auto end_itor = module->hooks.end();
	for (auto itor = module->hooks.begin(); itor != end_itor; ++itor)
	{
		RigDef::Hook & def = *itor;

		m_stream << "\n\t" << def.node.ToString();

		// Boolean options
		if (def.HasOptionAutoLock())  { m_stream << ", auto-lock"; }
		if (def.HasOptionNoDisable()) { m_stream << ", nodisable"; }
		if (def.HasOptionNoRope())    { m_stream << ", norope";    }
		if (def.HasOptionSelfLock())  { m_stream << ", self-lock"; }
		if (def.HasOptionVisible())   { m_stream << ", visible";   }

		// Key-value options
		m_stream 
			<< ", hookrange: " << def.option_hook_range
			<< ", speedcoef: " << def.option_speed_coef
			<< ", maxforce: "  << def.option_max_force
			<< ", hookgroup: " << def.option_hookgroup
			<< ", lockgroup: " << def.option_lockgroup
			<< ", timer: "     << def.option_timer
			<< ", shortlimit: "<< def.option_minimum_range_meters;
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessLockgroups(File::Module* module)
{
	if (module->lockgroups.empty())
	{
		return;
	}
	m_stream << "lockgroups" << endl << endl;
	auto end_itor = module->lockgroups.end();
	for (auto itor = module->lockgroups.begin(); itor != end_itor; ++itor)
	{
		RigDef::Lockgroup & def = *itor;

		m_stream << "\n\t" << def.number;
		auto nodes_end = def.nodes.end();
		for (auto nodes_itor = def.nodes.begin(); nodes_itor != nodes_end; ++nodes_itor)
		{
			m_stream << ", " << nodes_itor->ToString();
		}
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessTriggers(File::Module* module)
{
	if (module->triggers.empty())
	{
		return;
	}
	m_stream << "animators" << endl << endl;
	auto end_itor = module->triggers.end();
	for (auto itor = module->triggers.begin(); itor != end_itor; ++itor)
	{
		RigDef::Trigger & def = *itor;

		m_stream << "\n\t"
			<< def.nodes[0].ToString()       << ", "
			<< def.nodes[1].ToString()       << ", "
			<< def.contraction_trigger_limit << ", "
			<< def.expansion_trigger_limit   << ", "
			<< def.shortbound_trigger_action << ", "
			<< def.longbound_trigger_action  << ", ";

		if (def.HasFlag_i_Invisible()          ) { m_stream << "i"; }
		if (def.HasFlag_c_CommandStyle()       ) { m_stream << "c"; }
		if (def.HasFlag_x_StartDisabled()      ) { m_stream << "x"; }
		if (def.HasFlag_b_KeyBlocker()         ) { m_stream << "b"; }
		if (def.HasFlag_B_TriggerBlocker()     ) { m_stream << "B"; }
		if (def.HasFlag_A_InvTriggerBlocker()  ) { m_stream << "A"; }
		if (def.HasFlag_s_CmdNumSwitch()       ) { m_stream << "s"; }
		if (def.HasFlag_h_UnlocksHookGroup()   ) { m_stream << "h"; }
		if (def.HasFlag_H_LocksHookGroup()     ) { m_stream << "H"; }
		if (def.HasFlag_t_Continuous()         ) { m_stream << "t"; }
		if (def.HasFlag_E_EngineTrigger()      ) { m_stream << "E"; }

		m_stream << " " << def.boundary_timer;
	}
	m_stream << endl << endl; // Empty line
}

#define ANIMATOR_ADD_FLAG(DEF_VAR, AND_VAR, BITMASK_CONST, NAME_STR) \
	if (AND_VAR) { m_stream << " | "; } \
	if (BITMASK_IS_1((DEF_VAR).flags, RigDef::Animator::BITMASK_CONST)) { \
		AND_VAR = true; \
		m_stream << NAME_STR; \
	}

#define ANIMATOR_ADD_AERIAL_FLAG(DEF_VAR, AND_VAR, BITMASK_CONST, NAME_STR) \
	if (AND_VAR) { m_stream << " | "; } \
	if (BITMASK_IS_1((DEF_VAR).aero_animator.flags, RigDef::AeroAnimator::BITMASK_CONST)) { \
		AND_VAR = true; \
		m_stream << NAME_STR << DEF_VAR.aero_animator.motor; \
	}

#define ANIMATOR_ADD_LIMIT(DEF_VAR, AND_VAR, BITMASK_CONST, NAME_STR, VALUE) \
	if (AND_VAR) { m_stream << " | "; } \
	if (BITMASK_IS_1((DEF_VAR).aero_animator.flags, RigDef::Animator::BITMASK_CONST)) { \
		AND_VAR = true; \
		m_stream << NAME_STR << ": " << VALUE; \
	}

void Serializer::ProcessAnimators(File::Module* module)
{
	if (module->animators.empty())
	{
		return;
	}
	m_stream << "animators" << endl << endl;
	auto end_itor = module->animators.end();
	for (auto itor = module->animators.begin(); itor != end_itor; ++itor)
	{
		RigDef::Animator & def = *itor;

		m_stream << "\t"
			<< def.nodes[0].ToString() << ", "
			<< def.nodes[1].ToString() << ", "
			<< def.lenghtening_factor << ", ";

		// Options
		bool and = false;
		ANIMATOR_ADD_FLAG(def, and, OPTION_VISIBLE          , "vis")
		ANIMATOR_ADD_FLAG(def, and, OPTION_INVISIBLE        , "inv")
		ANIMATOR_ADD_FLAG(def, and, OPTION_AIRSPEED         , "airspeed")
		ANIMATOR_ADD_FLAG(def, and, OPTION_VERTICAL_VELOCITY, "vvi")
		ANIMATOR_ADD_FLAG(def, and, OPTION_ALTIMETER_100K   , "altimeter100k")
		ANIMATOR_ADD_FLAG(def, and, OPTION_ALTIMETER_10K    , "altimeter10k")
		ANIMATOR_ADD_FLAG(def, and, OPTION_ALTIMETER_1K     , "altimeter1k")
		ANIMATOR_ADD_FLAG(def, and, OPTION_ANGLE_OF_ATTACK  , "aoa")
		ANIMATOR_ADD_FLAG(def, and, OPTION_FLAP             , "flap")
		ANIMATOR_ADD_FLAG(def, and, OPTION_AIR_BRAKE        , "airbrake")
		ANIMATOR_ADD_FLAG(def, and, OPTION_ROLL             , "roll")
		ANIMATOR_ADD_FLAG(def, and, OPTION_PITCH            , "pitch")
		ANIMATOR_ADD_FLAG(def, and, OPTION_BRAKES           , "brakes")
		ANIMATOR_ADD_FLAG(def, and, OPTION_ACCEL            , "accel")
		ANIMATOR_ADD_FLAG(def, and, OPTION_CLUTCH           , "clutch")
		ANIMATOR_ADD_FLAG(def, and, OPTION_SPEEDO           , "speedo")
		ANIMATOR_ADD_FLAG(def, and, OPTION_TACHO            , "tacho")
		ANIMATOR_ADD_FLAG(def, and, OPTION_TURBO            , "turbo")
		ANIMATOR_ADD_FLAG(def, and, OPTION_PARKING          , "parking")
		ANIMATOR_ADD_FLAG(def, and, OPTION_SHIFT_LEFT_RIGHT , "shifterman1")
		ANIMATOR_ADD_FLAG(def, and, OPTION_SHIFT_BACK_FORTH , "shifterman2")
		ANIMATOR_ADD_FLAG(def, and, OPTION_SEQUENTIAL_SHIFT , "sequential")
		ANIMATOR_ADD_FLAG(def, and, OPTION_GEAR_SELECT      , "shifterlin")
		ANIMATOR_ADD_FLAG(def, and, OPTION_TORQUE           , "torque")
		ANIMATOR_ADD_FLAG(def, and, OPTION_DIFFLOCK         , "difflock")
		ANIMATOR_ADD_FLAG(def, and, OPTION_BOAT_RUDDER      , "rudderboat")
		ANIMATOR_ADD_FLAG(def, and, OPTION_BOAT_THROTTLE    , "throttleboat")
		ANIMATOR_ADD_FLAG(def, and, OPTION_SHORT_LIMIT      , "shortlimit")
		ANIMATOR_ADD_FLAG(def, and, OPTION_LONG_LIMIT       , "longlimit")

		ANIMATOR_ADD_AERIAL_FLAG(def, and, OPTION_THROTTLE , "throttle")
		ANIMATOR_ADD_AERIAL_FLAG(def, and, OPTION_RPM      , "rpm")
		ANIMATOR_ADD_AERIAL_FLAG(def, and, OPTION_TORQUE   , "aerotorq")
		ANIMATOR_ADD_AERIAL_FLAG(def, and, OPTION_PITCH    , "aeropit")
		ANIMATOR_ADD_AERIAL_FLAG(def, and, OPTION_STATUS   , "aerostatus")

		ANIMATOR_ADD_LIMIT(def, and, OPTION_SHORT_LIMIT    , "shortlimit", def.short_limit)
		ANIMATOR_ADD_LIMIT(def, and, OPTION_LONG_LIMIT     , "longlimit",  def.long_limit)
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessContacters(File::Module* module)
{
	if (module->contacters.empty())
	{
		return;
	}
	m_stream << "contacters" << endl << endl;
	auto end_itor = module->rotators.end();
	for (auto itor = module->rotators.begin(); itor != end_itor; ++itor)
	{
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessRotators(File::Module* module)
{
	if (module->rotators.empty())
	{
		return;
	}
	m_stream << "rotators" << endl << endl;
	auto end_itor = module->rotators.end();
	for (auto itor = module->rotators.begin(); itor != end_itor; ++itor)
	{
		Rotator & def = *itor;

		// Axis nodes
		m_stream
			<< def.axis_nodes[0].ToString() << ", "
			<< def.axis_nodes[1].ToString() << ", ";

		// Baseplate nodes
		for (int i = 0; i < 4; ++i)
		{
			m_stream << def.base_plate_nodes[i].ToString() << ", ";
		}

		// Rotating plate nodes
		for (int i = 0; i < 4; ++i)
		{
			m_stream << def.rotating_plate_nodes[i].ToString() << ", ";
		}
		
		// Attributes
		m_stream << def.rate << ", " << def.spin_left_key << ", " << def.spin_right_key << ", ";

		// Inertia
		m_stream 
			<< def.inertia.start_delay_factor << ", "
			<< def.inertia.stop_delay_factor  << ", "
			<< def.inertia.start_function	  << ", "
			<< def.inertia.stop_function	  << ", "
			<< def.engine_coupling            << ", "
			<< (def.needs_engine ? "true" : "false");
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessRotators2(File::Module* module)
{
	if (module->rotators_2.empty())
	{
		return;
	}
	m_stream << "rotators2" << endl << endl;
	auto end_itor = module->rotators_2.end();
	for (auto itor = module->rotators_2.begin(); itor != end_itor; ++itor)
	{
		Rotator2 & def = *itor;

		// Axis nodes
		m_stream
			<< def.axis_nodes[0].ToString() << ", "
			<< def.axis_nodes[1].ToString() << ", ";

		// Baseplate nodes
		for (int i = 0; i < 4; ++i)
		{
			m_stream << def.base_plate_nodes[i].ToString() << ", ";
		}

		// Rotating plate nodes
		for (int i = 0; i < 4; ++i)
		{
			m_stream << def.rotating_plate_nodes[i].ToString() << ", ";
		}
		
		// Attributes
		m_stream 
			<< def.rate            << ", " 
			<< def.spin_left_key   << ", " 
			<< def.spin_right_key  << ", "
			<< def.rotating_force  << ", "
			<< def.tolerance	   << ", "
			<< def.description	   << ", ";

		// Inertia
		m_stream 
			<< def.inertia.start_delay_factor << ", "
			<< def.inertia.stop_delay_factor  << ", "
			<< def.inertia.start_function	  << ", "
			<< def.inertia.stop_function	  << ", "
			<< def.engine_coupling            << ", "
			<< (def.needs_engine ? "true" : "false");
	}
	m_stream << endl << endl; // Empty line
}

void Serializer::ProcessFlexBodyWheels(File::Module* module)
{
	if (module->flex_body_wheels.empty())
	{
		return;
	}
	m_stream << "flexbodywheels" << endl << endl;
	auto end_itor = module->flex_body_wheels.end();
	for (auto itor = module->flex_body_wheels.begin(); itor != end_itor; ++itor)
	{
		m_stream << "\t"
			<< setw(m_float_width)   << itor->tyre_radius                   << ", "
			<< setw(m_float_width)   << itor->rim_radius                    << ", "
			<< setw(m_float_width)   << itor->width                         << ", "
			<< setw(3)               << itor->num_rays                      << ", "
			<< setw(m_node_id_width) << itor->nodes[0].ToString()           << ", "
			<< setw(m_node_id_width) << itor->nodes[1].ToString()           << ", "
			<< setw(m_node_id_width) << itor->rigidity_node.ToString()      << ", "
			<< setw(3)               << itor->braking                       << ", "
			<< setw(3)               << itor->propulsion                    << ", "
			<< setw(m_node_id_width) << itor->reference_arm_node.ToString() << ", "
			<< setw(m_float_width)   << itor->mass                          << ", "
			<< setw(m_float_width)   << itor->tyre_springiness              << ", "
			<< setw(m_float_width)   << itor->tyre_damping                  << ", "
			                         << (static_cast<char>(itor->side))     << ", "
									 << itor->rim_mesh_name                 << " " // Separator = space!
									 << itor->tyre_mesh_name
									 << endl;
	}

	m_stream << endl; // Empty line
}

void Serializer::ProcessSlopeBrake(File::Module* module)
{
	if (module->slope_brake)
	{
		m_stream << "SlopeBrake "
			<< module->slope_brake->regulating_force << ", "
			<< module->slope_brake->attach_angle << ", "
			<< module->slope_brake->release_angle 
			<< endl << endl;
	}
}

void Serializer::ProcessTractionControl(File::Module* module)
{
	if (module->traction_control)
	{
		RigDef::TractionControl* alb = module->traction_control.get();

		m_stream << "TractionControl "
			<< alb->regulation_force << ", "
			<< alb->wheel_slip << ", "
			<< alb->fade_speed << ", "
			<< alb->pulse_per_sec << ", mode: ";
		// Modes
		bool and = false;
		if (alb->GetModeIsOn())        { m_stream << " ON ";       and = true; }
		if (and) { m_stream << "&"; }
		if (alb->GetModeIsOff())       { m_stream << " OFF ";      and = true; }
		if (and) { m_stream << "&"; }
		if (alb->GetModeNoDashboard()) { m_stream << " NODASH ";   and = true; }
		if (and) { m_stream << "&"; }
		if (alb->GetModeNoToggle())    { m_stream << " NOTOGGLE "; and = true; }
	}
}

void Serializer::ProcessBrakes(File::Module* module)
{
	if (module->brakes)
	{
		m_stream << "brakes\n\t" 
			<< module->brakes->default_braking_force << ", "
			<< module->brakes->parking_brake_force;
	}
}

void Serializer::ProcessAntiLockBrakes(File::Module* module)
{
	if (module->anti_lock_brakes == nullptr)
	{
		return;
	}
	RigDef::AntiLockBrakes* alb = module->anti_lock_brakes.get();

	m_stream << "AntiLockBrakes "
		<< alb->regulation_force << ", "
		<< alb->min_speed << ", "
		<< alb->pulse_per_sec << ", mode: ";
	// Modes
	bool and = false;
	if (alb->GetModeIsOn())        { m_stream << " ON ";       and = true; }
	if (and) { m_stream << "&"; }
	if (alb->GetModeIsOff())       { m_stream << " OFF ";      and = true; }
	if (and) { m_stream << "&"; }
	if (alb->GetModeNoDashboard()) { m_stream << " NODASH ";   and = true; }
	if (and) { m_stream << "&"; }
	if (alb->GetModeNoToggle())    { m_stream << " NOTOGGLE "; and = true; }
}

void Serializer::ProcessEngine(File::Module* module)
{
	if (module->engine.get() == nullptr)
	{
		return;
	}
	
	m_stream << "engine\n\t" 
		<< setw(m_float_width)   << module->engine->shift_down_rpm                << ", "
		<< setw(m_float_width)   << module->engine->shift_up_rpm                  << ", "
		<< setw(m_float_width)   << module->engine->torque                        << ", "
		<< setw(m_float_width)   << module->engine->global_gear_ratio             << ", "
		<< setw(m_float_width)   << module->engine->reverse_gear_ratio            << ", "
		<< setw(m_float_width)   << module->engine->neutral_gear_ratio;
		
	auto itor_end = module->engine->gear_ratios.end();
	auto itor = module->engine->gear_ratios.end();
	for (; itor != itor_end; ++itor)
	{
		m_stream << ", " << *itor;
	}
	m_stream << endl << endl;
}

void Serializer::ProcessEngoption(File::Module* module)
{
	if (module->engoption.get() == nullptr)
	{
		return;
	}
	
	m_stream << "engoption" << endl << "\t" 
		<< setw(m_float_width)   << module->engoption->inertia           << ", "
		                         << module->engoption->type              << ", "
		<< setw(m_float_width)   << module->engoption->clutch_force      << ", "
		<< setw(m_float_width)   << module->engoption->shift_time        << ", "
		<< setw(m_float_width)   << module->engoption->clutch_time       << ", "
		<< setw(m_float_width)   << module->engoption->post_shift_time   << ", "
		<< setw(m_float_width)   << module->engoption->stall_rpm         << ", "
		<< setw(m_float_width)   << module->engoption->idle_rpm          << ", "
		<< setw(m_float_width)   << module->engoption->max_idle_mixture  << ", "
		<< setw(m_float_width)   << module->engoption->min_idle_mixture;
	
	m_stream << endl << endl;
}

void Serializer::ProcessHelp(File::Module* module)
{
	if (module->help_panel_material_name.empty())
	{
		return;
	}
	m_stream << "help\n\t" << module->help_panel_material_name << endl << endl;
}

void Serializer::ProcessWheels2(File::Module* module)
{
	if (module->wheels_2.empty())
	{
		return;
	}
	m_stream << "wheels2" << endl << endl;
	auto end_itor = module->wheels_2.end();
	for (auto itor = module->wheels_2.begin(); itor != end_itor; ++itor)
	{
		m_stream << "\t"
			<< setw(m_float_width)   << itor->tyre_radius                   << ", "
			<< setw(m_float_width)   << itor->rim_radius                    << ", "
			<< setw(m_float_width)   << itor->width                         << ", "
			<< setw(3)               << itor->num_rays                      << ", "
			<< setw(m_node_id_width) << itor->nodes[0].ToString()           << ", "
			<< setw(m_node_id_width) << itor->nodes[1].ToString()           << ", "
			<< setw(m_node_id_width) << itor->rigidity_node.ToString()      << ", "
			<< setw(3)               << itor->braking                       << ", "
			<< setw(3)               << itor->propulsion                    << ", "
			<< setw(m_node_id_width) << itor->reference_arm_node.ToString() << ", "
			<< setw(m_float_width)   << itor->mass                          << ", "
			<< setw(m_float_width)   << itor->rim_springiness               << ", "
			<< setw(m_float_width)   << itor->rim_damping                   << ", "
			<< setw(m_float_width)   << itor->tyre_springiness              << ", "
			<< setw(m_float_width)   << itor->tyre_damping                  << ", "
			                         << itor->face_material_name            << " " // Separator = space!
			                         << itor->band_material_name            << " " // Separator = space!
									 ;
		m_stream << endl;
	}

	m_stream << endl; // Empty line
}

void Serializer::ProcessWheels(File::Module* module)
{
	if (module->wheels.empty())
	{
		return;
	}
	m_stream << "wheels" << endl << endl;
	auto end_itor = module->wheels.end();
	for (auto itor = module->wheels.begin(); itor != end_itor; ++itor)
	{
		m_stream << "\t"
			<< setw(m_float_width)   << itor->radius                        << ", "
			<< setw(m_float_width)   << itor->width                         << ", "
			<< setw(3)               << itor->num_rays                      << ", "
			<< setw(m_node_id_width) << itor->nodes[0].ToString()           << ", "
			<< setw(m_node_id_width) << itor->nodes[1].ToString()           << ", "
			<< setw(m_node_id_width) << itor->rigidity_node.ToString()      << ", "
			<< setw(3)               << itor->braking                       << ", "
			<< setw(3)               << itor->propulsion                    << ", "
			<< setw(m_node_id_width) << itor->reference_arm_node.ToString() << ", "
			<< setw(m_float_width)   << itor->mass                          << ", "
			<< setw(m_float_width)   << itor->springiness                   << ", "
			<< setw(m_float_width)   << itor->damping                       << ", "
			                         << itor->face_material_name            << " " // Separator = space!
			                         << itor->band_material_name            << " " // Separator = space!
									 ;
		m_stream << endl;
	}

	m_stream << endl; // Empty line
}

void Serializer::ProcessMeshWheels(File::Module* module)
{
	if (module->mesh_wheels.empty())
	{
		return;
	}
	m_stream << "meshwheels" << endl << endl;
	auto end_itor = module->mesh_wheels.end();
	for (auto itor = module->mesh_wheels.begin(); itor != end_itor; ++itor)
	{
		m_stream << "\t"
			<< setw(m_float_width)   << itor->tyre_radius                   << ", "
			<< setw(m_float_width)   << itor->rim_radius                    << ", "
			<< setw(m_float_width)   << itor->width                         << ", "
			<< setw(3)               << itor->num_rays                      << ", "
			<< setw(m_node_id_width) << itor->nodes[0].ToString()           << ", "
			<< setw(m_node_id_width) << itor->nodes[1].ToString()           << ", "
			<< setw(m_node_id_width) << itor->rigidity_node.ToString()      << ", "
			<< setw(3)               << itor->braking                       << ", "
			<< setw(3)               << itor->propulsion                    << ", "
			<< setw(m_node_id_width) << itor->reference_arm_node.ToString() << ", "
			<< setw(m_float_width)   << itor->mass                          << ", "
			<< setw(m_float_width)   << itor->spring                        << ", "
			<< setw(m_float_width)   << itor->damping                       << ", "
			                         << (static_cast<char>(itor->side))     << ", "
			                         << itor->mesh_name                     << " " // Separator = space!
			                         << itor->material_name;
		m_stream << endl;
	}

	m_stream << endl; // Empty line
}

void Serializer::ProcessMeshWheels2(File::Module* module)
{
	if (module->mesh_wheels_2.empty())
	{
		return;
	}
	m_stream << "meshwheels2" << endl << endl;
	auto end_itor = module->mesh_wheels_2.end();
	for (auto itor = module->mesh_wheels_2.begin(); itor != end_itor; ++itor)
	{
		m_stream << "\t"
			<< setw(m_float_width)   << itor->tyre_radius                   << ", "
			<< setw(m_float_width)   << itor->rim_radius                    << ", "
			<< setw(m_float_width)   << itor->width                         << ", "
			<< setw(3)               << itor->num_rays                      << ", "
			<< setw(m_node_id_width) << itor->nodes[0].ToString()           << ", "
			<< setw(m_node_id_width) << itor->nodes[1].ToString()           << ", "
			<< setw(m_node_id_width) << itor->rigidity_node.ToString()      << ", "
			<< setw(3)               << itor->braking                       << ", "
			<< setw(3)               << itor->propulsion                    << ", "
			<< setw(m_node_id_width) << itor->reference_arm_node.ToString() << ", "
			<< setw(m_float_width)   << itor->mass                          << ", "
			<< setw(m_float_width)   << itor->tyre_springiness              << ", "
			<< setw(m_float_width)   << itor->tyre_damping                  << ", "
			                         << (static_cast<char>(itor->side))     << ", "
			                         << itor->mesh_name                     << " " // Separator = space!
			                         << itor->material_name;
		m_stream << endl;
	}

	m_stream << endl; // Empty line
}

void Serializer::ProcessCinecam(File::Module* module)
{
	if (module->cinecam.empty())
	{
		return;
	}

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
	if (module->beams.empty())
	{
		return;
	}

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
	if (module->shocks.empty())
	{
		return;
	}

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
	if (module->shocks_2.empty())
	{
		return;
	}

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
	if (module->hydros.empty())
	{
		return;
	}

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
	m_stream << endl << endl;
}

void Serializer::ProcessCommands2(File::Module* module)
{
	if (module->commands_2.empty())
	{
		return;
	}

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
	if (def.options.empty())
	{
		m_stream << "n"; // Placeholder, does nothing
	}
	else
	{
		m_stream << def.options;
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

void Serializer::ProcessBeamDefaults(BeamDefaults* beam_defaults, const char* prefix)
{
	if (beam_defaults == nullptr)
	{
		return;
	}
	m_stream << prefix << "set_beam_defaults       " // Align with "set_beam_defaults_scale"
		<< beam_defaults->springiness << ", "
		<< beam_defaults->damping_constant << ", "
		<< beam_defaults->deformation_threshold_constant << ", "
		<< beam_defaults->breaking_threshold_constant << ", "
		<< beam_defaults->visual_beam_diameter << ", "
		<< beam_defaults->beam_material_name << ", "
		<< beam_defaults->plastic_deformation_coefficient 
		<< endl;

	BeamDefaultsScale & scale = beam_defaults->scale;
	m_stream << prefix << "set_beam_defaults_scale "
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
	if (module->nodes.empty())
	{
		return;
	}

	// Group nodes by presets + find node-zero
	std::map< NodeDefaults*, std::vector<Node*> > nodes_by_presets;
	Node* node_zero = nullptr;
	auto itor_end = module->nodes.end(); 
	for (auto itor = module->nodes.begin(); itor != itor_end; ++itor)
	{
		Node & node = *itor;

		// Check zero node
		if (node.id.IsValid() && node.id.Str().empty() && node.id.Num() == 0)
		{
			if (node_zero != nullptr)
			{
				throw std::runtime_error("FATAL: Multiple nodes zero!!!");
			}
			node_zero = &node;
			continue;
		}

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

	// == Write nodes to file ==
	m_stream << "nodes" << endl << endl;

	// Node zero first
	if (node_zero == nullptr)
	{
		throw std::runtime_error("FATAL: Node zero not defined!!!");
	}
	ProcessNodeDefaults(node_zero->node_defaults.get());
	ProcessNode(*node_zero);

	// Other nodes
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
		m_stream << " " << node.load_weight_override;
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
		m_stream << "author " << def.type << " ";
		if (def._has_forum_account)
		{
			m_stream << def.forum_account_id << " ";
		}
		else
		{
			m_stream << "-1 ";
		}
		m_stream  << def.name << " " << def.email << endl;
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
