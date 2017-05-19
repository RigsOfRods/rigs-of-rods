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

#include "RigEditor_MeshWheel.h"

#include "RigDef_File.h"

using namespace RoR;
using namespace RigEditor;

void MeshWheel::ReGenerateMeshData()
{
    Ogre::Vector3 axis_point_inner = m_axis_inner_node->GetPosition();
    Ogre::Vector3 axis_point_outer = m_axis_outer_node->GetPosition();
    Ogre::Vector3 rigidity_point;
    if (m_rigidity_node != nullptr)
    {
        rigidity_point = m_rigidity_node->GetPosition();
    }

    m_vertices.clear();
    m_edges.clear();
    MeshWheel::GenerateMeshWheelGeometry(
        m_definition.num_rays,
        m_definition.tyre_radius,
        axis_point_inner,
        axis_point_outer,
        (m_rigidity_node != nullptr) ? &rigidity_point : nullptr,
        m_rigidity_node_connect_outer_ring,
        m_reference_arm_node->GetPosition(),
        m_vertices,
        m_edges,
        GetType(),
        m_aabb
        );

    this->SetGeometryIsDirty(false);
    this->SetHasRayCountChanged(false);
}

void MeshWheel::GenerateMeshWheelGeometry(
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
    )
{
    // GENERATE VERTICES

    LandVehicleWheel::BuildWheelNodes(out_vertices, num_rays, axis_point_inner, axis_point_outer, reference_arm_node, rigidity_point_ptr, wheel_radius, out_aabb);

    // GENERATE EDGES

    // Prepare buffers
    out_edges.clear();
    out_edges.reserve((rigidity_point_ptr != nullptr) ? num_rays * 8 : num_rays * 9);
    // Determine edge types
    Edge::Type bounded_type = Edge::TYPE_INVALID;
    Edge::Type reinforcement_type = Edge::TYPE_INVALID;
    Edge::Type rigidity_type = Edge::TYPE_INVALID;
    switch (wheel_type)
    {
    case LandVehicleWheel::TYPE_MESHWHEEL:
        bounded_type       = Edge::TYPE_MESHWHEEL_BOUNDED;
        reinforcement_type = Edge::TYPE_MESHWHEEL_REINFORCEMENT;
        rigidity_type      = Edge::TYPE_MESHWHEEL_RIGIDITY;
        break;
    case LandVehicleWheel::TYPE_MESHWHEEL_2:
        bounded_type       = Edge::TYPE_MESHWHEEL2_BOUNDED;
        reinforcement_type = Edge::TYPE_MESHWHEEL2_REINFORCEMENT;
        rigidity_type      = Edge::TYPE_MESHWHEEL2_RIGIDITY;
        break;
    default:
        break;
    }
    // Generate!
    const unsigned axis_inner_index = 0;
    const unsigned axis_outer_index = 1;
    const unsigned rigidity_index = 2;
    const unsigned base_index = 3;
    for (unsigned int i = 0; i < num_rays; ++i)
    {
        // Bounded
        unsigned int inner_point_index = base_index + (i * 2);
        unsigned int outer_point_index = inner_point_index + 1;
        
        out_edges.push_back(Edge(axis_inner_index, inner_point_index, bounded_type));
        out_edges.push_back(Edge(axis_outer_index, outer_point_index, bounded_type));
        out_edges.push_back(Edge(axis_outer_index, inner_point_index, bounded_type));
        out_edges.push_back(Edge(axis_inner_index, outer_point_index, bounded_type));

        // Reinforcement
        unsigned int next_inner_index = base_index + ((i + 1) % num_rays) * 2;
        unsigned int next_outer_index = next_inner_index + 1;

        out_edges.push_back(Edge(inner_point_index, outer_point_index, reinforcement_type));
        out_edges.push_back(Edge(inner_point_index, next_inner_index,  reinforcement_type));
        out_edges.push_back(Edge(outer_point_index, next_outer_index,  reinforcement_type));
        out_edges.push_back(Edge(outer_point_index, next_inner_index,  reinforcement_type));
        
        // Rigidity beams 
        if (rigidity_point_ptr != nullptr)
        {
            unsigned rigidity_target_index = (rigidity_connect_outer_ring) ? outer_point_index : inner_point_index;
            out_edges.push_back(Edge(rigidity_index, rigidity_target_index, bounded_type));
        }
    }
}

