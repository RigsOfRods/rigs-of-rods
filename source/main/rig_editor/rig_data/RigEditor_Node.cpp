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

Node::Node(RigDef::Node const & def):
	m_definition(def),
	m_flags(0),
	m_screen_position(0, 0)
{}

Node::Node()
{}

Ogre::Vector3 const & RigEditor::Node::GetPosition()
{
	return m_definition.position;
}

void RigEditor::Node::SetScreenPosition(Vector2int const & screen_pos)
{
	m_screen_position = screen_pos;
}

Vector2int const & RigEditor::Node::GetScreenPosition() const
{
	return m_screen_position;
}

void RigEditor::Node::SetHovered(bool value)
{
	Bitmask_SetBool(value, m_flags, Flags::HOVERED);
}

bool RigEditor::Node::IsHovered() const
{
	return BITMASK_IS_1(m_flags, Flags::HOVERED);
}

void RigEditor::Node::SetSelected(bool value)
{
	Bitmask_SetBool(value, m_flags, Flags::SELECTED);
}

bool RigEditor::Node::IsSelected() const
{
	return BITMASK_IS_1(m_flags, Flags::SELECTED);
}

void RigEditor::Node::Translate(Ogre::Vector3 const & offset)
{
	m_definition.position += offset;
}

bool RigEditor::Node::UnlinkBeam(Beam* beam_to_delete)
{
	for (auto itor = m_linked_beams.begin(); itor != m_linked_beams.end(); ++itor)
	{
		Beam* beam = *itor;
		if ( beam_to_delete == beam)
		{
			m_linked_beams.erase(itor);
			return true; // Found and erased
		}
	}
	return false; // Not found
}

RigDef::Node::Id const & RigEditor::Node::GetId()
{
	return m_definition.id;
}
