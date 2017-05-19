/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

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

namespace RoR
{

namespace RigEditor
{

Beam::Beam(void* source, Type type, Node* node_0, Node* node_1):
    m_type(type),
    m_source(source)
{
    m_nodes[0] = node_0;
    m_nodes[1] = node_1;
}

Beam::~Beam()
{
}

void Beam::DeleteDefinition()
{
    switch (m_type)
    {
    case Beam::TYPE_CINECAM:
        break; // Generated; m_source == RigEditor::CineCamera. Do nothing.
    case Beam::TYPE_COMMAND_HYDRO:
        delete static_cast<RigDef::Command2*>(m_source);
        m_source = nullptr;
        break;
    case Beam::TYPE_SHOCK_ABSORBER:
        delete static_cast<RigDef::Shock*>(m_source);
        m_source = nullptr;
        break;
    case Beam::TYPE_SHOCK_ABSORBER_2:
        delete static_cast<RigDef::Shock2*>(m_source);
        m_source = nullptr;
        break;
    case Beam::TYPE_STEERING_HYDRO:
        delete static_cast<RigDef::Hydro*>(m_source);
        m_source = nullptr;
        break;
    case Beam::TYPE_PLAIN:
        delete static_cast<RigDef::Beam*>(m_source);
        m_source = nullptr;
        break;
    default:
        // Really shouldn't happen
        throw std::runtime_error("Beam::~Beam(): Unsupported Beam::Type encountered");
    }
}

} // namespace RigEditor

} // namespace RoR
