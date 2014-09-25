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

RigEditor::Beam::Beam(RigDef::File::Module* def_module, int def_index, RigEditor::Node* node_0, RigEditor::Node* node_1):
	m_def_module(def_module),
	m_def_index(def_index)
{
	m_nodes[0] = node_0;
	m_nodes[1] = node_1;
}

RigDef::Beam & RigEditor::Beam::GetDefinition()
{
	return m_def_module->beams[m_def_index];
}

void RigEditor::Beam::EraseDefinition()
{
	m_def_module->beams.erase(m_def_module->beams.begin() + m_def_index);
}
