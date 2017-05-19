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
    @date   02/2015
    @author Petr Ohlidal
*/

#pragma once

#include "RigDef_Prerequisites.h"
#include "RigDef_File.h"
#include "RigEditor_LandVehicleWheel.h"
#include "RigEditor_Node.h"

namespace RoR
{

namespace RigEditor
{

class MeshWheel: public LandVehicleWheel
{
    friend class RigEditor::Rig;

public:

    MeshWheel(RigDef::MeshWheel const & def,  Node* inner, Node* outer, Node* rigidity):
        LandVehicleWheel(TYPE_MESHWHEEL),
        m_definition(def)
    {
        m_axis_inner_node = inner;
        m_axis_outer_node = outer;
        m_rigidity_node = rigidity;	
    }

    virtual void ReGenerateMeshData();

    static void GenerateMeshWheelGeometry(
        unsigned int num_rays,
        float wheel_radius,
        Ogre::Vector3 const & axis_point_inner,
        Ogre::Vector3 const & axis_point_outer,
        Ogre::Vector3* rigidity_point_ptr,
        bool rigidity_connect_outer_ring,
        Ogre::Vector3 const & reference_arm_node,
        std::vector<Ogre::Vector3>& out_vertices,
        std::vector<Edge>& out_edges,
        LandVehicleWheel::Type wheel_type,
        Ogre::AxisAlignedBox & out_aabb
        );

    inline RigDef::MeshWheel & GetDefinition() { return m_definition; }

protected:

    RigDef::MeshWheel        m_definition;
};

} // namespace RigEditor

} // namespace RoR
