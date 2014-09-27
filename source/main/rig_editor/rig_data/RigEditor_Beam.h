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
#include "RigEditor_ForwardDeclarations.h"
#include "RoRPrerequisites.h"

namespace RoR
{

namespace RigEditor
{

class Beam
{
	friend class RigEditor::Rig;

public:
	Beam(RigDef::Beam const & definition, RigEditor::Node* node_0, RigEditor::Node* node_1);

	RigEditor::Node* GetNodeA();

	RigEditor::Node* GetNodeB();

	void SetColor(Ogre::ColourValue const & color);

	Ogre::ColourValue const & GetColor() const;

protected:

	RigDef::Beam           m_definition;

	RigEditor::Node*       m_nodes[2];
	Ogre::ColourValue      m_color;
};

} // namespace RigEditor

} // namespace RoR
