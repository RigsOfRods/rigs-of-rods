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
	@file   RigEditor_Beam.h
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

class Beam
{
	friend class RigEditor::Rig;

public:
	Beam(RigDef::File::Module* def_module, int def_index, RigEditor::Node* node_0, RigEditor::Node* node_1);

	RigEditor::Node* GetNodeA()
	{
		return m_nodes[0];
	}

	RigEditor::Node* GetNodeB()
	{
		return m_nodes[1];
	}

	void SetColor(Ogre::ColourValue const & color)
	{
		m_color = color;
	}

	Ogre::ColourValue const & GetColor() const
	{
		return m_color;
	}

	RigDef::Beam & GetDefinition();

protected:

	void EraseDefinition();

	void SetDefIndex(int def_index)
	{
		m_def_index = def_index;
	}

	int GetDefIndex()
	{
		return m_def_index;
	}

	RigDef::File::Module*  m_def_module;
	int                    m_def_index;
	RigEditor::Node*       m_nodes[2];
	Ogre::ColourValue      m_color;
};

} // namespace RigEditor

} // namespace RoR
