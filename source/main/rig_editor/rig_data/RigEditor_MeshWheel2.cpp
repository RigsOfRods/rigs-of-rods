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

#include "RigEditor_MeshWheel2.h"

#include "RigDef_File.h"
#include "RigEditor_MeshWheel.h"
#include "RigEditor_RigWheelsAggregateData.h"

using namespace RoR;
using namespace RigEditor;

MeshWheel2::MeshWheel2(RigDef::MeshWheel const & def,  Node* inner, Node* outer, Node* rigidity, Node* reference_arm_node):
        LandVehicleWheel(TYPE_MESHWHEEL_2),
        m_definition(def)
{
    m_axis_inner_node = inner;
    m_axis_outer_node = outer;
    m_rigidity_node = rigidity;
    m_reference_arm_node = reference_arm_node;
    // Create private copy of BeamDefaults, which act as additional data container (rim parameters)
    m_definition.beam_defaults = std::shared_ptr<RigDef::BeamDefaults>(new RigDef::BeamDefaults(*def.beam_defaults));
}

void MeshWheel2::ReGenerateMeshData()
{
    Ogre::Vector3 axis_point_inner = m_axis_inner_node->GetPosition();
    Ogre::Vector3 axis_point_outer = m_axis_outer_node->GetPosition();
    Ogre::Vector3 rigidity_point;
    if (m_rigidity_node != nullptr)
    {
        rigidity_point = m_rigidity_node->GetPosition();
    }

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

void MeshWheel2::Update(AllWheelsAggregateData *all_data)
{
    assert(all_data != nullptr);
    MeshWheel2AggregateData& data = all_data->meshwheels2_data;
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
    if (data.IsBreakThresholdUniform())   { m_definition.beam_defaults->breaking_threshold    = data.rim_breaking_threshold; }
    if (data.IsDeformThresholdUniform())  { m_definition.beam_defaults->deformation_threshold = data.rim_deform_threshold; }
    if (data.IsRimDampingUniform())       { m_definition.beam_defaults->damping_constant      = data.rim_damping; }
    if (data.IsRimSpringUniform())        { m_definition.beam_defaults->springiness           = data.rim_spring; }
    if (data.IsBrakingModeUniform())      { m_definition.braking       = data.braking; }
    if (data.IsMassUniform())             { m_definition.mass          = data.mass; }
    if (data.IsPropulsionModeUniform())   { m_definition.propulsion    = data.propulsion; }
    if (data.IsRimMeshNameUniform())      { m_definition.mesh_name     = data.rim_mesh_name; }
    if (data.IsRimRadiusUniform())        { m_definition.rim_radius    = data.rim_radius; }
    if (data.IsSideUniform())             { m_definition.side          = data.is_right_side ? RigDef::MeshWheel::SIDE_RIGHT : RigDef::MeshWheel::SIDE_LEFT; }
    if (data.IsTyreDampingUniform())      { m_definition.damping       = data.tyre_damping; }
    if (data.IsTyreMaterialNameUniform()) { m_definition.material_name = data.tyre_material_name; }
    if (data.IsTyreSpringUniform())       { m_definition.spring        = data.tyre_spring; }
}
