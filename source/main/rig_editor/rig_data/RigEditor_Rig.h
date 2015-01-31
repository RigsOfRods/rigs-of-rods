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
	@file   RigEditor_Rig.h
	@date   06/2014
	@author Petr Ohlidal
*/

#pragma once

#include <unordered_map>

#include "RigDef_File.h"
#include "RigDef_Prerequisites.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_Types.h"
#include "RoRPrerequisites.h"
#include "OgreAxisAlignedBox.h"
#include "RigEditor_Node.h"

namespace RoR
{

namespace RigEditor
{

class Rig
{
public:

	Rig(Config* config);

	~Rig();

	void UpdateBoundingBox(Ogre::Vector3 const & point);

	void RefreshNodeScreenPosition(Node & node, CameraHandler* camera_handler);

	/**
	* @return True if new node was focused.
	*/
	bool RefreshMouseHoveredNode(Vector2int const & mouse_position);

	void RefreshAllNodesScreenPositions(CameraHandler* camera_handler);

	Node* GetMouseHoveredNode() const
	{
		return m_mouse_hovered_node;
	}

	void RefreshNodesDynamicMeshes(Ogre::SceneNode* parent_scene_node);

	void AttachToScene(Ogre::SceneNode* parent_scene_node);

	void DetachFromScene();

	bool ToggleMouseHoveredNodeSelected();

	void DeselectAllNodes();

	Node& CreateNewNode(Ogre::Vector3 const & position);

	void ClearMouseHoveredNode();

	void TranslateSelectedNodes(Ogre::Vector3 const & offset, CameraHandler* camera_handler);

	void RefreshBeamsDynamicMesh();

	/** Deselects all nodes. If none was selected, selects all.
	*/
	void DeselectOrSelectAllNodes();

	/**
	* @return True if beam existed and was deleted, false otherwise.
	*/
	bool DeleteBeamBetweenNodes(Node* n1, Node* n2);

	void DeleteBeamsBetweenSelectedNodes();

	int DeleteAttachedBeams(Node* node);

	void DeleteSelectedNodes();

	bool DeleteNode(Node* node);

	bool DeleteBeam(Beam* beam);

	void DeleteBeam(std::vector<RigEditor::Beam*>::iterator & delete_itor);

	void Build(
			boost::shared_ptr<RigDef::File> rig_def, 
			RigEditor::Main* rig_editor,
			std::list<Ogre::String>* report = nullptr
		);

	/** Blender-like extrusion; makes linked copies of selected nodes, selects new nodes.
	*/
	void ExtrudeSelectedNodes();

	Beam & CreateNewBeam(Node* n1, Node* n2);

	void SelectedNodesCommitPositionUpdates();

	void SelectedNodesCancelPositionUpdates();

	RigProperties* GetProperties();

	boost::shared_ptr<RigDef::File> Export();

	void QuerySelectedNodesData(RigAggregateNodesData* result);

	void QuerySelectedBeamsData(RigAggregateBeams2Data* result);

	void UpdateSelectedBeamsList();

	inline unsigned int GetNumSelectedBeams() const { return m_selected_beams.size(); }
	
	// Node/beam updaters
	void SelectedNodesUpdateAttributes     (const RigAggregateNodesData      *data);
	void SelectedPlainBeamsUpdateAttributes(const RigAggregatePlainBeamsData *data);
	void SelectedMixedBeamsUpdateAttributes(const MixedBeamsAggregateData          *data);
	void SelectedShocksUpdateAttributes    (const RigAggregateShocksData     *data);
	void SelectedShocks2UpdateAttributes   (const RigAggregateShocks2Data    *data);
	void SelectedHydrosUpdateAttributes    (const RigAggregateHydrosData     *data);
	void SelectedCommands2UpdateAttributes (const RigAggregateCommands2Data  *data);

	/** Rig building utility function
	*/
	Node* FindNode(
		RigDef::Node::Id const & node_id, 
		Ogre::String const & section_name,
		std::list<Ogre::String>* report = nullptr
		);

private:

	bool ProcessMeshwheels2(std::vector<RigDef::MeshWheel2> & list, std::list<Ogre::String>* report = nullptr);

	void BuildWheelNodes( 
		std::vector<Ogre::Vector3> & out_positions,
		unsigned int num_rays,
		Ogre::Vector3 axis_nodes[2],
		Ogre::Vector3 const & reference_arm_node,
		float wheel_radius
		);

private:

	/* STRUCTURE */

	std::unordered_map<RigDef::Node::Id, Node, RigDef::Node::Id::Hasher> m_nodes;
	std::list<Beam>          m_beams;
	std::list<CineCamera>    m_cinecameras;
	std::list<MeshWheel2>    m_mesh_wheels_2;
	Ogre::AxisAlignedBox     m_aabb;
	unsigned int             m_highest_node_id;
	
	/* PROPERTIES */

	std::unique_ptr<RigProperties>    m_properties;

	/* STATE */
	Node*                m_mouse_hovered_node;
	std::list<Beam*>     m_selected_beams;

	/* VISUALS */
	std::unique_ptr<Ogre::ManualObject>  m_beams_dynamic_mesh;
	std::unique_ptr<Ogre::ManualObject>  m_nodes_dynamic_mesh;
	std::unique_ptr<Ogre::ManualObject>  m_nodes_hover_dynamic_mesh;
	std::unique_ptr<Ogre::ManualObject>  m_nodes_selected_dynamic_mesh;
	std::unique_ptr<Ogre::ManualObject>  m_wheels_dynamic_mesh;

	/* UTILITY */
	bool                 m_modified;
	Config*              m_config;
};

} // namespace RigEditor

} // namespace RoR
