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
	@file   RigEditor_Node.cpp
	@date   09/2014
	@author Petr Ohlidal
*/

#include "RigDef_File.h"
#include "RigEditor_Node.h"

using namespace RoR;
using namespace RoR::RigEditor;

Node::Node(RigDef::File::Module* module, int index):
	m_def_module(module),
	m_def_index(index),
	m_flags(0),
	m_screen_position(0, 0)
{}

RigDef::Node & RigEditor::Node::GetDefinition()
{
	return m_def_module->nodes[m_def_index];
}

void RigEditor::Node::EraseDefinition()
{
	m_def_module->nodes.erase(m_def_module->nodes.begin() + m_def_index);
}

Ogre::Vector3 const & RigEditor::Node::GetPosition()
{
	return GetDefinition().position;
}
