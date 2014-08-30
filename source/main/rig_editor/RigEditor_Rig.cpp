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
	@file   RigEditor_Rig.cpp
	@date   08/2014
	@author Petr Ohlidal
*/

#include "RigEditor_Rig.h"

#include "RigEditor_Beam.h"
#include "RigEditor_Node.h"

#include <OgreManualObject.h>

using namespace RoR;
using namespace RoR::RigEditor;

Rig::~Rig()
{
	/* Clear visuals */
	if (m_beams_dynamic_mesh != nullptr)
	{
		delete m_beams_dynamic_mesh;
		m_beams_dynamic_mesh = nullptr;
	}
	if (m_nodes_dynamic_mesh != nullptr)
	{
		delete m_nodes_dynamic_mesh;
		m_nodes_dynamic_mesh = nullptr;
	}
	if (m_wheels_dynamic_mesh != nullptr)
	{
		delete m_wheels_dynamic_mesh;
		m_wheels_dynamic_mesh = nullptr;
	}

	/* Clear structure */
	for (auto itor = m_beams.begin(); itor != m_beams.end(); ++itor)
	{
		delete *itor;
	}
	m_beams.empty();

	for (auto itor = m_nodes.begin(); itor != m_nodes.end(); ++itor)
	{
		delete (*itor).second;
	}
	m_beams.empty();
}
