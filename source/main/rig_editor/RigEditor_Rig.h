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
	@file   RigEditor_Rig.h
	@date   06/2014
	@author Petr Ohlidal
*/

#pragma once

#include "RigDef_File.h"
#include "RigDef_Prerequisites.h"
#include "RoRPrerequisites.h"

#include "OgreAxisAlignedBox.h"

namespace RoR
{

namespace RigEditor
{

class Rig
{
	friend class RigFactory;

	/** Constructed by RigEditor::RigFactory */
	Rig():
		m_beams_dynamic_mesh(nullptr),
		m_nodes_dynamic_mesh(nullptr),
		m_aabb(Ogre::AxisAlignedBox::BOX_NULL),
		m_modified(false)
	{}

	~Rig();

public:

	Ogre::ManualObject* GetBeamsDynamicMesh()
	{
		return m_beams_dynamic_mesh;
	}

	Ogre::ManualObject* GetNodesDynamicMesh()
	{
		return m_nodes_dynamic_mesh;
	}

	void UpdateBoundingBox(Ogre::Vector3 const & point);

private:

	/* STRUCTURE */
	std::unordered_map<RigDef::Node::Id, Node*, RigDef::Node::Id::Hasher> m_nodes;
	std::vector<Beam*> m_beams;
	Ogre::AxisAlignedBox m_aabb;

	/* VISUALS */
	Ogre::ManualObject*  m_beams_dynamic_mesh;
	Ogre::ManualObject*  m_nodes_dynamic_mesh;

	/* UTILITY */
	bool                 m_modified;
};

} // namespace RigEditor

} // namespace RoR
