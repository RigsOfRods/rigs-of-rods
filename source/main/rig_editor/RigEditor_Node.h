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
	@file   RigEditor_Node.h
	@date   06/2014
	@author Petr Ohlidal
*/

#pragma once

#include "RigDef_Prerequisites.h"
#include "RoRPrerequisites.h"

namespace RoR
{

namespace RigEditor
{

class Node
{
public:
	Node(RigDef::Node & node):
		m_def_node(node)
	{}

	Ogre::Vector3 const & GetPosition()
	{
		return m_def_node.position;
	}

private:
	RigDef::Node & m_def_node;
};

} // namespace RigEditor

} // namespace RoR
