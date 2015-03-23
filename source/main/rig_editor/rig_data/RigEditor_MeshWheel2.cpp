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
	@file   
	@author Petr Ohlidal
	@date   02/2015
*/

#include "RigEditor_MeshWheel2.h"

#include "RigDef_File.h"
#include "RigEditor_MeshWheel.h"

using namespace RoR;
using namespace RigEditor;

void MeshWheel2::ReGenerateMeshData()
{
	Ogre::Vector3 axis_point_inner = m_axis_inner_node->GetPosition();
	Ogre::Vector3 axis_point_outer = m_axis_outer_node->GetPosition();
	Ogre::Vector3 rigidity_point;
	if (m_rigidity_node != nullptr)
	{
		rigidity_point = m_rigidity_node->GetPosition();
	}

	MeshWheel::GenerateMeshWheelGeometry(
		m_definition.num_rays,
		m_definition.tyre_radius,
		axis_point_inner,
		axis_point_outer,
		(m_rigidity_node != nullptr) ? &rigidity_point : nullptr,
		m_rigidity_node_connect_outer_ring,
		m_reference_arm_node->GetPosition(),
		m_vertices,
		m_edges,
		GetType(),
		m_aabb
		);

	this->SetGeometryIsDirty(false);
	this->SetHasRayCountChanged(false);
}
