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

#include "RigEditor_FlexBodyWheel.h"

#include "RigDef_File.h"
#include "RigEditor_RigWheelsAggregateData.h"

using namespace RoR;
using namespace RigEditor;

void FlexBodyWheel::ReGenerateMeshData()
{
    Ogre::Vector3 axis_point_inner = m_axis_inner_node->GetPosition();
    Ogre::Vector3 axis_point_outer = m_axis_outer_node->GetPosition();

    // Prepare buffers
    m_vertices.clear();
    m_vertices.reserve(m_definition.num_rays * 4);
    m_vertices.push_back(axis_point_inner);
    m_vertices.push_back(axis_point_outer);

    unsigned int num_edges = (m_definition.num_rays * 25);
    unsigned int axis_index_inner = 0;
    unsigned int axis_index_outer = 1;
    unsigned int rigidity_index = 2; // Always reserve space for rigidity node, even if unused
    unsigned int base_node_index = 3;
    bool rigidity_node_use_outer = false; // Only relevant if rigidity node exists
    if (m_rigidity_node != nullptr)
    {
        Ogre::Vector3 rigidity_pos = m_rigidity_node->GetPosition();

        unsigned int num_edges = (m_definition.num_rays * 26);
        m_vertices.push_back(rigidity_pos);

        // Find out which side of the wheel is closer to the rigidity node
        float distance_inner = rigidity_pos.squaredDistance(axis_point_inner); // Squared is faster
        float distance_outer = rigidity_pos.squaredDistance(axis_point_outer);
        rigidity_node_use_outer = distance_outer < distance_inner;
    }
    else
    {
        m_vertices.push_back(Ogre::Vector3::ZERO); // Dummy value for rigidity node
    }
    m_edges.clear();
    m_edges.reserve(num_edges);

    // Init calculation
    Ogre::Vector3 axis_vector = axis_point_inner - axis_point_outer;
    axis_vector.normalise();
    Ogre::Vector3 rim_ray_vector = axis_vector.perpendicular() * m_definition.rim_radius;
    Ogre::Quaternion rim_ray_rotator = Ogre::Quaternion(Ogre::Degree(-360.f / (m_definition.num_rays * 2)), axis_vector);

    // Vertices - rim
    for (unsigned int i = 0; i < m_definition.num_rays; i++)
    {
        // Outer ring
        m_vertices.push_back(axis_point_inner + rim_ray_vector);
        rim_ray_vector = rim_ray_rotator * rim_ray_vector;

        // Inner ring
        m_vertices.push_back(axis_point_outer + rim_ray_vector);
        rim_ray_vector = rim_ray_rotator * rim_ray_vector;
    }

    Ogre::Vector3 tyre_ray_vector = axis_vector.perpendicular() * m_definition.tyre_radius;
    Ogre::Quaternion& tyre_ray_rotator = rim_ray_rotator;
    tyre_ray_vector = tyre_ray_rotator * tyre_ray_vector;

    // Vertices - tyre
    for (unsigned int i = 0; i < m_definition.num_rays; i++)
    {
        // Outer ring
        m_vertices.push_back(axis_point_inner + tyre_ray_vector);
        tyre_ray_vector = tyre_ray_rotator * tyre_ray_vector;
        
        // Inner ring
        m_vertices.push_back(axis_point_outer + tyre_ray_vector);
        tyre_ray_vector = tyre_ray_rotator * tyre_ray_vector;
    }

    // Indices - rim
    for (unsigned int i = 0; i < m_definition.num_rays; i++)
    {
        // Rim axis to rim ring
        unsigned int rim_inner_index = base_node_index + (i * 2);
        unsigned int rim_outer_index = rim_inner_index + 1;

        m_edges.push_back(Edge(axis_index_inner, rim_inner_index, Edge::TYPE_FLEXBODYWHEEL_RIM_CONNECTION));
        m_edges.push_back(Edge(axis_index_outer, rim_outer_index, Edge::TYPE_FLEXBODYWHEEL_RIM_CONNECTION));
        m_edges.push_back(Edge(axis_index_outer, rim_inner_index, Edge::TYPE_FLEXBODYWHEEL_RIM_CONNECTION));
        m_edges.push_back(Edge(axis_index_inner, rim_outer_index, Edge::TYPE_FLEXBODYWHEEL_RIM_CONNECTION));
        
        // Reinforcement rim ring
        unsigned int rim_next_inner_index = base_node_index + (((i + 1) % m_definition.num_rays) * 2);
        unsigned int rim_next_outer_index = rim_next_inner_index + 1;

        m_edges.push_back(Edge(rim_inner_index, rim_outer_index,      Edge::TYPE_FLEXBODYWHEEL_RIM_REINFORCEMENT));
        m_edges.push_back(Edge(rim_inner_index, rim_next_inner_index, Edge::TYPE_FLEXBODYWHEEL_RIM_REINFORCEMENT));
        m_edges.push_back(Edge(rim_outer_index, rim_next_outer_index, Edge::TYPE_FLEXBODYWHEEL_RIM_REINFORCEMENT));
        m_edges.push_back(Edge(rim_outer_index, rim_next_inner_index, Edge::TYPE_FLEXBODYWHEEL_RIM_REINFORCEMENT));
    }

    // Indices - tyre
    for (unsigned int i = 0; i < m_definition.num_rays; i++)
    {
        
        int rim_inner_index = base_node_index + (2 * i);
        int rim_outer_index = rim_inner_index + 1;
        int tire_inner_index = rim_inner_index + (2 * m_definition.num_rays);
        int tire_previous_inner_index = (i == 0) ? (base_node_index + (4 * m_definition.num_rays) - 2) : tire_inner_index - 2;
        int tire_outer_index = tire_inner_index + 1;
        int tire_previous_outer_index = tire_previous_inner_index + 1;
        int tire_next_inner_index = base_node_index + (2 * m_definition.num_rays) + (2 * (i % m_definition.num_rays));
        int tire_next_outer_index = tire_next_inner_index + 1;

        // Rim to tire tread
        m_edges.push_back(Edge(rim_inner_index, tire_inner_index         , Edge::TYPE_FLEXBODYWHEEL_TYRE_CONNECTION));
        m_edges.push_back(Edge(rim_inner_index, tire_previous_outer_index, Edge::TYPE_FLEXBODYWHEEL_TYRE_CONNECTION));
        m_edges.push_back(Edge(rim_inner_index, tire_previous_inner_index, Edge::TYPE_FLEXBODYWHEEL_TYRE_CONNECTION));
        m_edges.push_back(Edge(rim_outer_index, tire_inner_index         , Edge::TYPE_FLEXBODYWHEEL_TYRE_CONNECTION));
        m_edges.push_back(Edge(rim_outer_index, tire_outer_index         , Edge::TYPE_FLEXBODYWHEEL_TYRE_CONNECTION));
        m_edges.push_back(Edge(rim_outer_index, tire_previous_outer_index, Edge::TYPE_FLEXBODYWHEEL_TYRE_CONNECTION));
        
        // Reinforcement tire tread
        m_edges.push_back(Edge(tire_inner_index, tire_outer_index     , Edge::TYPE_FLEXBODYWHEEL_TYRE_REINFORCEMENT));
        m_edges.push_back(Edge(tire_inner_index, tire_next_inner_index, Edge::TYPE_FLEXBODYWHEEL_TYRE_REINFORCEMENT));
        m_edges.push_back(Edge(tire_outer_index, tire_next_outer_index, Edge::TYPE_FLEXBODYWHEEL_TYRE_REINFORCEMENT));
        m_edges.push_back(Edge(tire_outer_index, tire_next_inner_index, Edge::TYPE_FLEXBODYWHEEL_TYRE_REINFORCEMENT));

        // Rigidity edge
        int rigidity_target_index = rigidity_node_use_outer ? tire_outer_index : tire_inner_index;
        m_edges.push_back(Edge(rigidity_index, rigidity_target_index, Edge::TYPE_FLEXBODYWHEEL_TYRE_RIGIDITY));
    }

    //tiretread anti collapse reinforcements, using precalced support beams
    for (unsigned int i=0; i<m_definition.num_rays; i++)
    {
        int tire_inner_index = base_node_index + i*2 + m_definition.num_rays*2;
        int tire_outer_index = tire_inner_index + 1;

        m_edges.push_back(Edge(axis_index_inner, tire_inner_index, Edge::TYPE_FLEXBODYWHEEL_TYRE_REINFORCEMENT));
        m_edges.push_back(Edge(axis_index_outer, tire_outer_index, Edge::TYPE_FLEXBODYWHEEL_TYRE_REINFORCEMENT));
    }

    this->UpdateAABB();

    this->SetGeometryIsDirty(false);
    this->SetHasRayCountChanged(false);
}

void FlexBodyWheel::UpdateAABB()
{
    auto end = m_vertices.end();
    auto itor = m_vertices.begin();
    m_aabb.setNull();

    for (; end != itor; ++itor)
    {
        m_aabb.merge(*itor);
    }
}

void FlexBodyWheel::Update(AllWheelsAggregateData *all_data)
{
    assert(all_data != nullptr);
    FlexBodyWheelAggregateData& data = all_data->flexbodywheels_data;
    if (data.num_elements == 0)
    {
        return;
    }

    // Ray count
    if (data.IsRayCountUniform() && data.num_rays != m_definition.num_rays) 
    { 
        m_definition.num_rays = data.num_rays;
        this->SetGeometryIsDirty(true);
        this->SetHasRayCountChanged(true);
    }
    // Radius
    if (data.IsTyreRadiusUniform())
    {
        this->SetGeometryIsDirty(true);
        m_definition.tyre_radius = data.tyre_radius;
    }
    if (data.IsRimRadiusUniform())
    { 
        this->SetGeometryIsDirty(true);
        m_definition.rim_radius = data.rim_radius; 
    }
    // Nodes
    if (data.num_elements == 1)
    {
        if (data.axis_nodes[0] != m_definition.nodes[0]) {
            m_definition.nodes[0] = data.axis_nodes[0];
            this->SetGeometryIsDirty(true);
        }
        if (data.axis_nodes[1] != m_definition.nodes[1]) {
            m_definition.nodes[1] = data.axis_nodes[1];
            this->SetGeometryIsDirty(true);
        }
        if (data.rigidity_node != m_definition.rigidity_node) {
            m_definition.rigidity_node = data.rigidity_node;
            this->SetGeometryIsDirty(true);
        }
        if (data.reference_arm_node != m_definition.reference_arm_node) {
            m_definition.reference_arm_node = data.reference_arm_node;
        }
    }
    // Other
    if (data.IsBrakingModeUniform())      { m_definition.braking = data.braking; }
    if (data.IsMassUniform())             { m_definition.mass = data.mass; }
    if (data.IsPropulsionModeUniform())   { m_definition.propulsion = data.propulsion; }
    if (data.IsRimDampingUniform())       { m_definition.beam_defaults->damping_constant = data.rim_damping; }
    if (data.IsRimMeshNameUniform())      { m_definition.rim_mesh_name = data.rim_mesh_name; }
    if (data.IsRimSpringUniform())        { m_definition.beam_defaults->springiness = data.rim_spring; }
    if (data.IsSideUniform())             { m_definition.side = data.is_right_side ? RigDef::MeshWheel::SIDE_RIGHT : RigDef::MeshWheel::SIDE_LEFT; }
    if (data.IsTyreDampingUniform())      { m_definition.tyre_damping = data.tyre_damping; }
    if (data.IsTyreMeshNameUniform())     { m_definition.tyre_mesh_name = data.tyre_mesh_name; }
    if (data.IsTyreSpringUniform())       { m_definition.tyre_springiness = data.tyre_spring; }
}
