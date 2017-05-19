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
    @author Petr Ohlidal
    @date   02/2015
*/

#pragma once

#include "BitFlags.h"
#include "RigDef_File.h"
#include "RigDef_Prerequisites.h"
#include "RigEditor_LandVehicleWheel.h"
#include "RigEditor_Node.h"
#include "RigEditor_Types.h"

#include <OgreVector3.h>
#include <vector>

namespace RoR
{

namespace RigEditor
{

class FlexBodyWheel: public LandVehicleWheel
{
public:
    FlexBodyWheel(RigDef::FlexBodyWheel & def, Node* inner, Node* outer, Node* rigidity, Node* reference_arm):
        LandVehicleWheel(TYPE_FLEXBODYWHEEL),
        m_definition(def)
    {
        m_axis_inner_node = inner;
        m_axis_outer_node = outer;
        m_rigidity_node = rigidity;
        m_reference_arm_node = reference_arm;
    }

    virtual void ReGenerateMeshData();
    
    virtual void Update(AllWheelsAggregateData *data);

    inline RigDef::FlexBodyWheel & GetDefinition() { return m_definition; }

private:
    void UpdateAABB();

    RigDef::FlexBodyWheel        m_definition;
};

} // namespace RigEditor

} // namespace RoR

