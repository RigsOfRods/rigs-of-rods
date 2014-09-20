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
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   RigEditor_RigFactory.cpp
	@date   06/2014
	@author Petr Ohlidal
*/

#include "RigEditor_RigFactory.h"

#include "RigDef_File.h"
#include "RigEditor_Beam.h"
#include "RigEditor_Config.h"
#include "RigEditor_Main.h"
#include "RigEditor_Node.h"
#include "RigEditor_Rig.h"

#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgreManualObject.h>
#include <OgreSceneManager.h>
#include <OgreRenderOperation.h>

using namespace RoR;
using namespace RoR::RigEditor;

Rig* RigFactory::BuildRig(
	boost::shared_ptr<RigDef::File> rig_def, 
	std::vector< boost::shared_ptr<RigDef::File::Module> > & selected_modules, 
	RigEditor::Main* rig_editor
	)
{
	RigEditor::Config & config = *rig_editor->GetConfig();
	RigEditor::Rig* rig = new Rig(rig_editor->GetConfig());

	rig->m_rig_def = rig_def;
	
	/* Process nodes (section "nodes") */

	unsigned int highest_numeric_id = 0;
	for (auto module_itor = selected_modules.begin(); module_itor != selected_modules.end(); module_itor++)
	{
		int node_index = 0;
		for (auto node_itor = (*module_itor)->nodes.begin(); node_itor != (*module_itor)->nodes.end(); node_itor++)
		{
			bool done = false;
			RigDef::Node::Id & node_id = node_itor->id;
			while (true)
			{
				auto result = rig->m_nodes.insert( std::pair<RigDef::Node::Id, Node>(node_id, Node(module_itor->get(), node_index)) );
				if (result.second == true)
				{
					// Update bounding box
					rig->m_aabb.merge(node_itor->position);

					// Update highest numeric ID
					if (node_id.Str().empty()) // Is numerically indexes?
					{
						highest_numeric_id = (node_id.Num() > highest_numeric_id) ? node_id.Num() : highest_numeric_id;
					}

					break;
				}
				else
				{
					std::stringstream msg;
					msg << "[WARNING] BuildRig(): Duplicate node ID: " << node_id.ToString();
					node_id.SetStr(node_id.ToString() + "-dup");
					msg << ", changed to: " << node_id.ToString();
					AddMessage(module_itor->get(), msg.str());
					rig->m_modified = true;
				}
			}
			node_index++;
		}
	}
	rig->m_highest_node_id = highest_numeric_id;

	/* Process beams (section "beams") */
	std::vector<RigDef::Beam> unlinked_beams_to_retry; // Linked to invalid nodes, will retry after other sections (esp. wheels) were processed
	// TODO: Implement the retry.

	for (auto module_itor = selected_modules.begin(); module_itor != selected_modules.end(); module_itor++)
	{
		for (auto beam_itor = (*module_itor)->beams.begin(); beam_itor != (*module_itor)->beams.end(); beam_itor++)
		{
			Node* nodes[] = {nullptr, nullptr};

			/* Find node 0 */
			auto result = rig->m_nodes.find(beam_itor->nodes[0]);
			if (result == rig->m_nodes.end())
			{
				/* Node 0 not found */
				unlinked_beams_to_retry.push_back(*beam_itor);
				std::stringstream msg;
				msg << "[Warning] BuildRig(): Beam[ " << (*beam_itor).nodes[0].ToString() << ", " << (*beam_itor).nodes[1].ToString() << "]: Node 0 not found. Will retry later.";
				AddMessage(module_itor->get(), msg.str());
			}
			else
			{
				nodes[0] = &result->second; // Assign node 0

				/* Find node 1 */
				result = rig->m_nodes.find(beam_itor->nodes[1]);
				if (result == rig->m_nodes.end())
				{
					 /* Node 1 not found */
					unlinked_beams_to_retry.push_back(*beam_itor);
					std::stringstream msg;
					msg << "[Warning] BuildRig(): Beam[ " << (*beam_itor).nodes[0].ToString() << ", " << (*beam_itor).nodes[1].ToString() << "]: Node 1 not found. Will retry later.";
					AddMessage(module_itor->get(), msg.str());
				}
				else
				{
					nodes[1] = &result->second; // Assing node 1
				}
			}
		
			// Colorize beams
			unsigned int beam_options = beam_itor->options;
			Ogre::ColourValue const & color =
				(BITMASK_IS_1(beam_options, RigDef::Beam::OPTION_i_INVISIBLE))
				?	config.beam_invisible_color
				:	(BITMASK_IS_1(beam_options, RigDef::Beam::OPTION_r_ROPE))
					?	config.beam_rope_color
					:	(BITMASK_IS_1(beam_options, RigDef::Beam::OPTION_s_SUPPORT))
						?	config.beam_support_color
						:	config.beam_generic_color;
			
			// Allocate and save
			Beam* beam = new Beam(*beam_itor, nodes[0], nodes[1]);
			beam->SetColor(color);
			rig->m_beams.push_back(beam);
		}
	}

	/* CREATE MESH OF BEAMS */

	/* Prepare material */
	if (! Ogre::MaterialManager::getSingleton().resourceExists("rig-editor-skeleton-material"))
	{
		Ogre::MaterialPtr mat = static_cast<Ogre::MaterialPtr>(
			Ogre::MaterialManager::getSingleton().create("rig-editor-skeleton-material", 
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)
		);

		mat->getTechnique(0)->getPass(0)->createTextureUnitState();
		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
		mat->setLightingEnabled(false);
		mat->setReceiveShadows(false);
	}

	/* Create mesh */
	rig->m_beams_dynamic_mesh = rig_editor->GetOgreSceneManager()->createManualObject();
	rig->m_beams_dynamic_mesh->estimateIndexCount(rig->m_beams.size() * 2);
	rig->m_beams_dynamic_mesh->estimateVertexCount(rig->m_nodes.size());
	rig->m_beams_dynamic_mesh->setCastShadows(false);
	rig->m_beams_dynamic_mesh->setDynamic(true);
	rig->m_beams_dynamic_mesh->setRenderingDistance(300);

	/* Init */
	rig->m_beams_dynamic_mesh->begin("rig-editor-skeleton-material", Ogre::RenderOperation::OT_LINE_LIST);

	/* Process beams */
	for (auto itor = rig->m_beams.begin(); itor != rig->m_beams.end(); itor++)
	{
		rig->m_beams_dynamic_mesh->position((*itor)->GetNodeA()->GetPosition());
		rig->m_beams_dynamic_mesh->colour((*itor)->GetColor());
		rig->m_beams_dynamic_mesh->position((*itor)->GetNodeB()->GetPosition());
		rig->m_beams_dynamic_mesh->colour((*itor)->GetColor());
	}

	/* Finalize */
	rig->m_beams_dynamic_mesh->end();

	/* CREATE MESH OF NODES */

	/* Prepare material */
	if (! Ogre::MaterialManager::getSingleton().resourceExists("rig-editor-skeleton-nodes-material"))
	{
		Ogre::MaterialPtr node_mat = static_cast<Ogre::MaterialPtr>(
			Ogre::MaterialManager::getSingleton().create("rig-editor-skeleton-nodes-material", 
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)
		);

		node_mat->getTechnique(0)->getPass(0)->createTextureUnitState();
		node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
		node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
		node_mat->setLightingEnabled(false);
		node_mat->setReceiveShadows(false);
		node_mat->setPointSize(config.node_generic_point_size);
	}

	/* Create mesh */
	rig->m_nodes_dynamic_mesh = rig_editor->GetOgreSceneManager()->createManualObject();
	rig->m_nodes_dynamic_mesh->estimateVertexCount(rig->m_nodes.size());
	rig->m_nodes_dynamic_mesh->setCastShadows(false);
	rig->m_nodes_dynamic_mesh->setDynamic(true);
	rig->m_nodes_dynamic_mesh->setRenderingDistance(300);

	/* Init */
	rig->m_nodes_dynamic_mesh->begin("rig-editor-skeleton-nodes-material", Ogre::RenderOperation::OT_POINT_LIST);

	/* Process nodes */
	for (auto itor = rig->m_nodes.begin(); itor != rig->m_nodes.end(); itor++)
	{
		rig->m_nodes_dynamic_mesh->position((*itor).second.GetPosition());
		rig->m_nodes_dynamic_mesh->colour(config.node_generic_color);
	}

	/* Finalize */
	rig->m_nodes_dynamic_mesh->end();

	/* CREATE MESH OF HOVERED NODES */

	/* Prepare material */
	if (! Ogre::MaterialManager::getSingleton().resourceExists("rig-editor-skeleton-nodes-hover-material"))
	{
		Ogre::MaterialPtr node_mat = static_cast<Ogre::MaterialPtr>(
			Ogre::MaterialManager::getSingleton().create("rig-editor-skeleton-nodes-hover-material", 
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)
		);

		node_mat->getTechnique(0)->getPass(0)->createTextureUnitState();
		node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
		node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
		node_mat->setLightingEnabled(false);
		node_mat->setReceiveShadows(false);
		node_mat->setPointSize(config.node_hover_point_size);
	}

	/* Create mesh */
	rig->m_nodes_hover_dynamic_mesh = rig_editor->GetOgreSceneManager()->createManualObject();
	rig->m_nodes_hover_dynamic_mesh->estimateVertexCount(10);
	rig->m_nodes_hover_dynamic_mesh->setCastShadows(false);
	rig->m_nodes_hover_dynamic_mesh->setDynamic(true);
	rig->m_nodes_hover_dynamic_mesh->setRenderingDistance(300);

	/* Init */
	rig->m_nodes_hover_dynamic_mesh->begin("rig-editor-skeleton-nodes-hover-material", Ogre::RenderOperation::OT_POINT_LIST);

	/* Fill some dummy vertices (to properly initialise the object) */
	rig->m_nodes_hover_dynamic_mesh->position(Ogre::Vector3::UNIT_X);
	rig->m_nodes_hover_dynamic_mesh->colour(Ogre::ColourValue::Red);
	rig->m_nodes_hover_dynamic_mesh->position(Ogre::Vector3::UNIT_Y);
	rig->m_nodes_hover_dynamic_mesh->colour(Ogre::ColourValue::Green);
	rig->m_nodes_hover_dynamic_mesh->position(Ogre::Vector3::UNIT_Z);
	rig->m_nodes_hover_dynamic_mesh->colour(Ogre::ColourValue::Blue);

	/* Finalize */
	rig->m_nodes_hover_dynamic_mesh->end();

	/* CREATE MESH OF SELECTED NODES */

	/* Prepare material */
	static const Ogre::String nodes_selected_mat_name("rig-editor-skeleton-nodes-selected-material");
	if (! Ogre::MaterialManager::getSingleton().resourceExists(nodes_selected_mat_name))
	{
		Ogre::MaterialPtr node_mat = static_cast<Ogre::MaterialPtr>(
			Ogre::MaterialManager::getSingleton().create(
				nodes_selected_mat_name, 
				Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
			)
		);

		node_mat->getTechnique(0)->getPass(0)->createTextureUnitState();
		node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
		node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
		node_mat->setLightingEnabled(false);
		node_mat->setReceiveShadows(false);
		node_mat->setPointSize(config.node_selected_point_size);
	}

	/* Create mesh */
	rig->m_nodes_selected_dynamic_mesh = rig_editor->GetOgreSceneManager()->createManualObject();
	rig->m_nodes_selected_dynamic_mesh->estimateVertexCount(10);
	rig->m_nodes_selected_dynamic_mesh->setCastShadows(false);
	rig->m_nodes_selected_dynamic_mesh->setDynamic(true);
	rig->m_nodes_selected_dynamic_mesh->setRenderingDistance(300);

	/* Init */
	rig->m_nodes_selected_dynamic_mesh->begin(nodes_selected_mat_name, Ogre::RenderOperation::OT_POINT_LIST);

	/* Fill some dummy vertices (to properly initialise the object) */
	rig->m_nodes_selected_dynamic_mesh->position(Ogre::Vector3::UNIT_X);
	rig->m_nodes_selected_dynamic_mesh->colour(Ogre::ColourValue::Red);
	rig->m_nodes_selected_dynamic_mesh->position(Ogre::Vector3::UNIT_Y);
	rig->m_nodes_selected_dynamic_mesh->colour(Ogre::ColourValue::Green);
	rig->m_nodes_selected_dynamic_mesh->position(Ogre::Vector3::UNIT_Z);
	rig->m_nodes_selected_dynamic_mesh->colour(Ogre::ColourValue::Blue);

	/* Finalize */
	rig->m_nodes_selected_dynamic_mesh->end();

	/* CREATE MESH OF WHEELS (beams only) */

	/* Prepare material */
	if (! Ogre::MaterialManager::getSingleton().resourceExists("rig-editor-skeleton-wheels-material"))
	{
		Ogre::MaterialPtr node_mat = static_cast<Ogre::MaterialPtr>(
			Ogre::MaterialManager::getSingleton().create("rig-editor-skeleton-wheels-material", 
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)
		);

		node_mat->getTechnique(0)->getPass(0)->createTextureUnitState();
		node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
		node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
		node_mat->setLightingEnabled(false);
		node_mat->setReceiveShadows(false);
	}

	/* Analyze */
	int wheels_vertex_count = 0;
	int wheels_index_count = 0;
	for (auto module_itor = selected_modules.begin(); module_itor != selected_modules.end(); module_itor++)
	{
		/* Meshwheels2 */
		auto & list = module_itor->get()->mesh_wheels_2;
		for (auto itor = list.begin(); itor != list.end(); ++itor)
		{
			int num_lines = (itor->num_rays * 8);
			wheels_index_count += num_lines * 2;
			wheels_vertex_count += num_lines * 2;
		}
	}

	/* Create mesh */
	rig->m_wheels_dynamic_mesh = rig_editor->GetOgreSceneManager()->createManualObject();
	rig->m_wheels_dynamic_mesh->estimateVertexCount(wheels_vertex_count);
	rig->m_wheels_dynamic_mesh->estimateIndexCount(wheels_index_count);
	rig->m_wheels_dynamic_mesh->setCastShadows(false);
	rig->m_wheels_dynamic_mesh->setDynamic(true);
	rig->m_wheels_dynamic_mesh->setRenderingDistance(300);

	/* Init */
	rig->m_wheels_dynamic_mesh->begin("rig-editor-skeleton-wheels-material", Ogre::RenderOperation::OT_LINE_LIST);

	/* Process wheels */
	for (auto module_itor = selected_modules.begin(); module_itor != selected_modules.end(); module_itor++)
	{
		/* Meshwheels2 */
		ProcessMeshwheels2(module_itor->get()->mesh_wheels_2, rig, *rig_editor->GetConfig(), module_itor->get()->name);
	}

	/* Finalize */
	rig->m_wheels_dynamic_mesh->end();
	
	/* DONE */

	return rig;
}

void RigFactory::AddMessage(RigDef::File::Module* module, std::string const & text)
{
	AddMessage(module->name, text);
}

void RigFactory::AddMessage(std::string const & module_name, std::string const & text)
{
	std::stringstream msg;
	msg << text << " (module: " << module_name << ")";
	m_messages.push_back(msg.str());
}

bool RigFactory::ProcessMeshwheels2(std::vector<RigDef::MeshWheel2> & list, RigEditor::Rig * rig, Config & config, std::string const & module_name)
{
	for (auto itor = list.begin(); itor != list.end(); ++itor)
	{
		auto def = *itor;

		/* Find axis nodes */
		RigEditor::Node* axis_nodes[] = {nullptr, nullptr};
		auto node_result = rig->m_nodes.find(def.nodes[0]);
		if (node_result == rig->m_nodes.end())
		{
			std::stringstream msg;
			msg << "[Error] ProcessMeshwheels2(): Axis node [0] not found (id: " << def.nodes[0].ToString() << "). Wheel not processed.";
			AddMessage(module_name, msg.str());
			return false;
		}
		axis_nodes[0] = &node_result->second;
		node_result = rig->m_nodes.find(def.nodes[1]);
		if (node_result == rig->m_nodes.end())
		{
			std::stringstream msg;
			msg << "[Error] ProcessMeshwheels2(): Axis node [1] not found (id: " << def.nodes[1].ToString() << "). Wheel not processed.";
			AddMessage(module_name, msg.str());
			return false;
		}
		axis_nodes[1] = &node_result->second;

		/* Find reference arm node */
		node_result = rig->m_nodes.find(def.reference_arm_node);
		if (node_result == rig->m_nodes.end())
		{
			std::stringstream msg;
			msg << "[Error] ProcessMeshwheels2(): Reference arm node not found (id: " << def.reference_arm_node.ToString() << "). Wheel not processed.";
			AddMessage(module_name, msg.str());
			return false;
		}
		RigEditor::Node* reference_arm_node = &node_result->second;

		/* Generate nodes */
		std::vector<Ogre::Vector3> generated_nodes;
		generated_nodes.reserve(def.num_rays * 2);
		Ogre::Vector3 axis_node_positions[] = {axis_nodes[0]->GetPosition(), axis_nodes[1]->GetPosition()};
		BuildWheelNodes(generated_nodes, def.num_rays, axis_node_positions, reference_arm_node->GetPosition(), def.tyre_radius);

		/* Find out where to connect rigidity node */
		bool rigidity_beam_side_1 = false;
		RigEditor::Node* rigidity_node = nullptr;
		if (def.rigidity_node.IsValid())
		{
			node_result = rig->m_nodes.find(def.rigidity_node);
			if (node_result == rig->m_nodes.end())
			{
				std::stringstream msg;
				msg << "[Error] ProcessMeshwheels2(): Rigidity node not found (id: " << def.rigidity_node.ToString() << "). Wheel not processed.";
				AddMessage(module_name, msg.str());
				return false;
			}
			rigidity_node = &node_result->second;

			float distance_1 = rigidity_node->GetPosition().distance(axis_nodes[0]->GetPosition());
			float distance_2 = rigidity_node->GetPosition().distance(axis_nodes[1]->GetPosition());
			rigidity_beam_side_1 = distance_1 < distance_2;
		}

		/* Generate beams */
		Ogre::ManualObject * manual_mesh = rig->m_wheels_dynamic_mesh;
		for (unsigned int i = 0; i < def.num_rays; i++)
		{
			/* Bounded */
			unsigned int outer_ring_node_index = (i * 2);
			Ogre::Vector3 const & outer_ring_node_pos = generated_nodes[outer_ring_node_index];
			Ogre::Vector3 const & inner_ring_node_pos = generated_nodes[outer_ring_node_index + 1];
		
			manual_mesh->position(axis_node_positions[0]);
			manual_mesh->colour(config.meshwheel2_beam_bounded_color);
			manual_mesh->position(outer_ring_node_pos);
			manual_mesh->colour(config.meshwheel2_beam_bounded_color);
			
			manual_mesh->position(axis_node_positions[1]);
			manual_mesh->colour(config.meshwheel2_beam_bounded_color);
			manual_mesh->position(inner_ring_node_pos);
			manual_mesh->colour(config.meshwheel2_beam_bounded_color);

			manual_mesh->position(axis_node_positions[1]);
			manual_mesh->colour(config.meshwheel2_beam_bounded_color);
			manual_mesh->position(outer_ring_node_pos);
			manual_mesh->colour(config.meshwheel2_beam_bounded_color);

			manual_mesh->position(axis_node_positions[0]);
			manual_mesh->colour(config.meshwheel2_beam_bounded_color);
			manual_mesh->position(inner_ring_node_pos);
			manual_mesh->colour(config.meshwheel2_beam_bounded_color);

			/* Reinforcement */
			unsigned int next_outer_ring_node_index = ((i + 1) % def.num_rays) * 2;
			Ogre::Vector3 const & next_outer_ring_node_pos = generated_nodes[next_outer_ring_node_index];
			Ogre::Vector3 const & next_inner_ring_node_pos = generated_nodes[next_outer_ring_node_index + 1];

			manual_mesh->position(outer_ring_node_pos);
			manual_mesh->colour(config.meshwheel2_beam_reinforcement_color);
			manual_mesh->position(inner_ring_node_pos);
			manual_mesh->colour(config.meshwheel2_beam_reinforcement_color);

			manual_mesh->position(outer_ring_node_pos);
			manual_mesh->colour(config.meshwheel2_beam_reinforcement_color);
			manual_mesh->position(next_outer_ring_node_pos);
			manual_mesh->colour(config.meshwheel2_beam_reinforcement_color);

			manual_mesh->position(inner_ring_node_pos);
			manual_mesh->colour(config.meshwheel2_beam_reinforcement_color);
			manual_mesh->position(next_inner_ring_node_pos);
			manual_mesh->colour(config.meshwheel2_beam_reinforcement_color);

			manual_mesh->position(inner_ring_node_pos);
			manual_mesh->colour(config.meshwheel2_beam_reinforcement_color);
			manual_mesh->position(next_outer_ring_node_pos);
			manual_mesh->colour(config.meshwheel2_beam_reinforcement_color);

			/* Rigidity beams */
			if (rigidity_node != nullptr)
			{
				Ogre::Vector3 const & target_node_pos = (rigidity_beam_side_1) ? outer_ring_node_pos : inner_ring_node_pos;

				manual_mesh->position(rigidity_node->GetPosition());
				manual_mesh->colour(config.meshwheel2_beam_rigidity_color);
				manual_mesh->position(target_node_pos);
				manual_mesh->colour(config.meshwheel2_beam_rigidity_color);
			}
		}
	}
	return true;
}

void RigFactory::BuildWheelNodes( 
	std::vector<Ogre::Vector3> & out_positions,
	unsigned int num_rays,
	Ogre::Vector3 axis_nodes_pos[2],
	Ogre::Vector3 const & reference_arm_node_pos,
	float wheel_radius
)
{
	/* Find near attach */
	Ogre::Real length_1 = axis_nodes_pos[0].distance(reference_arm_node_pos);
	Ogre::Real length_2 = axis_nodes_pos[1].distance(reference_arm_node_pos);
	Ogre::Vector3 const & near_attach = (length_1 < length_2) ? axis_nodes_pos[0] : axis_nodes_pos[1];

	/* Axis */
	Ogre::Vector3 axis_vector = axis_nodes_pos[1] - axis_nodes_pos[0];
	axis_vector.normalise();
	
	/* Nodes */
	Ogre::Vector3 ray_vector = axis_vector.perpendicular() * wheel_radius;
	Ogre::Quaternion ray_rotator = Ogre::Quaternion(Ogre::Degree(-360.0 / (num_rays * 2)), axis_vector);

	for (unsigned int i = 0; i < num_rays; i++)
	{
		/* Outer ring */
		out_positions.push_back(axis_nodes_pos[0] + ray_vector);
		ray_vector = ray_rotator * ray_vector;

		/* Inner ring */
		out_positions.push_back(axis_nodes_pos[1] + ray_vector);
		ray_vector = ray_rotator * ray_vector;
	}
}
