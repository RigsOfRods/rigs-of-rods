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
#include "RigEditor_Types.h"

namespace RoR
{

namespace RigEditor
{

class Node
{
public:

	struct Flags
	{
		static const int HOVERED     = BITMASK(1);
		static const int SELECTED    = BITMASK(2);
	};

	Node(RigDef::File::Module* module, int index):
		m_def_module(module),
		m_def_index(index),
		m_flags(0),
		m_screen_position(0, 0)
	{}

	Ogre::Vector3 const & GetPosition()
	{
		return GetDefinition().position;
	}

	void SetScreenPosition(Vector2int const & screen_pos)
	{
		m_screen_position = screen_pos;
	}

	Vector2int const & GetScreenPosition() const
	{
		return m_screen_position;
	}

	void SetHovered(bool value)
	{
		Bitmask_SetBool(value, m_flags, Flags::HOVERED);
	}

	bool IsHovered() const
	{
		return BITMASK_IS_1(m_flags, Flags::HOVERED);
	}

	void SetSelected(bool value)
	{
		Bitmask_SetBool(value, m_flags, Flags::SELECTED);
	}

	bool IsSelected() const
	{
		return BITMASK_IS_1(m_flags, Flags::SELECTED);
	}

	RigDef::Node & GetDefinition()
	{
		return m_def_module->nodes[m_def_index];
	}

private:
	RigDef::File::Module*  m_def_module;
	int                    m_def_index;

	Vector2int     m_screen_position;
	int            m_flags;
};

} // namespace RigEditor

} // namespace RoR
