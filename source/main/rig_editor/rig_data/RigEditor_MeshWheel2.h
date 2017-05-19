/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.com/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
    @file   
    @date   11/2014
    @author Petr Ohlidal
*/

#pragma once

#include "RigDef_Prerequisites.h"
#include "RigDef_File.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_LandVehicleWheel.h"
#include "RigEditor_Node.h"

namespace RoR
{

namespace RigEditor
{

class MeshWheel2: public LandVehicleWheel
{
    friend class RigEditor::Rig;

public:

    MeshWheel2(RigDef::MeshWheel const & def,  Node* inner, Node* outer, Node* rigidity, Node* reference_arm_node);

    virtual void ReGenerateMeshData();

    virtual void Update(AllWheelsAggregateData *data);

    inline RigDef::MeshWheel & GetDefinition() { return m_definition; }

protected:

    RigDef::MeshWheel        m_definition;
};

} // namespace RigEditor

} // namespace RoR
