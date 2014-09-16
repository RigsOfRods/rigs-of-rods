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
	@file   RigEditor_Rig.cpp
	@date   08/2014
	@author Petr Ohlidal
*/

#include "RigEditor_Rig.h"

#include "RigEditor_Beam.h"
#include "RigEditor_CameraHandler.h"
#include "RigEditor_Config.h"
#include "RigEditor_Node.h"

#include <OgreManualObject.h>
#include <OgreSceneManager.h>

using namespace RoR;
using namespace RoR::RigEditor;

Rig::Rig(Config* config):
	m_beams_dynamic_mesh(nullptr),
	m_nodes_dynamic_mesh(nullptr),
	m_nodes_hover_dynamic_mesh(nullptr),
	m_wheels_dynamic_mesh(nullptr),
	m_node_closest_to_mouse(nullptr),
	m_aabb(Ogre::AxisAlignedBox::BOX_NULL),
	m_config(config),
	m_modified(false),
	m_mouse_box_extent(10)
{}

Rig::~Rig()
{
	/* Clear visuals */
	if (m_beams_dynamic_mesh != nullptr)
	{
		delete m_beams_dynamic_mesh;
		m_beams_dynamic_mesh = nullptr;
	}
	if (m_nodes_dynamic_mesh != nullptr)
	{
		delete m_nodes_dynamic_mesh;
		m_nodes_dynamic_mesh = nullptr;
	}
	if (m_wheels_dynamic_mesh != nullptr)
	{
		delete m_wheels_dynamic_mesh;
		m_wheels_dynamic_mesh = nullptr;
	}
	if (m_nodes_hover_dynamic_mesh != nullptr)
	{
		delete m_nodes_hover_dynamic_mesh;
		m_nodes_hover_dynamic_mesh = nullptr;
	}

	/* Clear structure */
	for (auto itor = m_beams.begin(); itor != m_beams.end(); ++itor)
	{
		delete *itor;
	}
	m_beams.clear();

	m_nodes.clear();
}

void Rig::RefreshNodeScreenPosition(Node & node, CameraHandler* camera_handler)
{
	float z_distance;
	Vector2int screen_pos(-1, -1);
	bool node_visible = camera_handler->ConvertWorldToScreenPosition(node.GetPosition(), screen_pos, z_distance);
	if (node_visible)
	{
		node.SetScreenPosition(screen_pos);
	}
}

void Rig::RefreshAllNodesScreenPositions(CameraHandler* camera_handler)
{
	for (auto itor = m_nodes.begin(); itor != m_nodes.end(); ++itor)
	{
		RefreshNodeScreenPosition(itor->second, camera_handler);
	}
}

bool Rig::RefreshNodeClosestToMouse(Vector2int const & mouse_position)
{
	/*
	Lookup method: for each node
		- Compute mouse offset
		- Convert it's components to abs()
		- Pick the larger component as 'distance'
		- Compare 'distance' to previous node
	*/

	auto itor = m_nodes.begin();
	Node* result = m_node_closest_to_mouse;
	if (result == nullptr)
	{
		result = &itor->second;
		++itor;
	}
	Vector2int node_mouse_offset = result->GetScreenPosition() - mouse_position;
	node_mouse_offset.x *= (node_mouse_offset.x < 0) ? -1 : 1; // Absolute value
	node_mouse_offset.y *= (node_mouse_offset.y < 0) ? -1 : 1; // Absolute value
	int result_mouse_distance = (node_mouse_offset.x > node_mouse_offset.y) ? node_mouse_offset.x : node_mouse_offset.y;
	
	auto itor_end = m_nodes.end();

	for ( ; itor != itor_end ; ++itor)
	{
		node_mouse_offset = itor->second.GetScreenPosition() - mouse_position;
		node_mouse_offset.x *= (node_mouse_offset.x < 0) ? -1 : 1; // Absolute value
		node_mouse_offset.y *= (node_mouse_offset.y < 0) ? -1 : 1; // Absolute value
		int current_mouse_distance = (node_mouse_offset.x > node_mouse_offset.y) ? node_mouse_offset.x : node_mouse_offset.y;
		if (current_mouse_distance < result_mouse_distance)
		{
			result_mouse_distance = current_mouse_distance;
			result = &itor->second;
		}
	}
	bool ret_val = (m_node_closest_to_mouse != result);
	m_node_closest_to_mouse = result;
	return ret_val;
}

void Rig::RefreshNodesDynamicMeshes(Ogre::SceneNode* parent_scene_node)
{
	// DEBUG TEST: Just highlight the node closest to mouse. 

	/* Nodes */
	m_nodes_dynamic_mesh->beginUpdate(0);
	for (auto itor = m_nodes.begin(); itor != m_nodes.end(); ++itor)
	{
		// DEBUG (Windows, VC10)
		// Conditional breakpoint: "if the node IDs match"
		//
		// Current node:
		//     Definition:
		//         ((*(((*current_node).m_def_module).px)).nodes)._Myfirst[(*(current_node)).m_def_index]
		//     ID:
		//         ((((*(((*current_node).m_def_module).px)).nodes)._Myfirst[(*(current_node)).m_def_index]).id).m_id_num
		// 
		// Closest node:
		//     Definition:
		//         ((*(((*((*(this)).m_node_closest_to_mouse)).m_def_module).px)).nodes)._Myfirst[(*((*(this)).m_node_closest_to_mouse)).m_def_index]
		//     ID:
		//         ((((*(((*((*(this)).m_node_closest_to_mouse)).m_def_module).px)).nodes)._Myfirst[(*((*(this)).m_node_closest_to_mouse)).m_def_index]).id).m_id_num

		RigEditor::Node* current_node = &itor->second;
		if (current_node != m_node_closest_to_mouse)
		{
			m_nodes_dynamic_mesh->position((*itor).second.GetPosition());
			m_nodes_dynamic_mesh->colour(m_config->node_generic_color);
		}
	}
	m_nodes_dynamic_mesh->end();

	/* Hover nodes */
	if (m_node_closest_to_mouse != nullptr)
	{
		m_nodes_hover_dynamic_mesh->beginUpdate(0);
		m_nodes_hover_dynamic_mesh->position(m_node_closest_to_mouse->GetPosition());
		m_nodes_hover_dynamic_mesh->colour(m_config->node_hover_color);
		m_nodes_hover_dynamic_mesh->end();
		
		if (! m_nodes_hover_dynamic_mesh->isAttached())
		{
			parent_scene_node->attachObject(m_nodes_hover_dynamic_mesh);
		}
	}
	else
	{
		if (m_nodes_hover_dynamic_mesh->isAttached())
		{
			m_nodes_hover_dynamic_mesh->detachFromParent();
		}
	}
}

void Rig::AttachToScene(Ogre::SceneNode* parent_scene_node)
{
	assert(parent_scene_node != nullptr);

	parent_scene_node->attachObject(m_beams_dynamic_mesh);
	parent_scene_node->attachObject(m_nodes_dynamic_mesh);
	parent_scene_node->attachObject(m_nodes_hover_dynamic_mesh);
	parent_scene_node->attachObject(m_wheels_dynamic_mesh);
}

void Rig::DetachFromScene()
{
	m_beams_dynamic_mesh->detachFromParent();
	m_nodes_dynamic_mesh->detachFromParent();
	m_nodes_hover_dynamic_mesh->detachFromParent();
	m_wheels_dynamic_mesh->detachFromParent();
}
