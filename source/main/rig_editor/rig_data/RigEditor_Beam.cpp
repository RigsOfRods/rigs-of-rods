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
	@file   RigEditor_Beam.cpp
	@date   09/2014
	@author Petr Ohlidal
*/

#include "RigDef_File.h"
#include "RigEditor_Beam.h"

using namespace RoR;
using namespace RoR::RigEditor;

RigEditor::Beam::Beam(RigDef::Beam const & definition, RigEditor::Node* node_0, RigEditor::Node* node_1):
	m_definition(definition)
{
	m_nodes[0] = node_0;
	m_nodes[1] = node_1;
}

RigEditor::Node* RigEditor::Beam::GetNodeA()
{
	return m_nodes[0];
}

RigEditor::Node* RigEditor::Beam::GetNodeB()
{
	return m_nodes[1];
}

void RigEditor::Beam::SetColor(Ogre::ColourValue const & color)
{
	m_color = color;
}

Ogre::ColourValue const & RigEditor::Beam::GetColor() const
{
	return m_color;
}
