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
    @date   03/2015
    @author Petr Ohlidal
*/

#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_FlexBodyWheel.h"
#include "RigEditor_RigWheelsAggregateData.h"
#include "RigEditor_LandVehicleWheel.h"
#include "RigEditor_MeshWheel.h"
#include "RigEditor_MeshWheel2.h"

using namespace RoR;
using namespace RigEditor;

void AllWheelsAggregateData::AddWheel(LandVehicleWheel* wheel)
{
    LandVehicleWheel::Type verified_type = LandVehicleWheel::TYPE_INVALID;
    switch (wheel->GetType())
    {
    case LandVehicleWheel::TYPE_MESHWHEEL_2:
    case LandVehicleWheel::TYPE_FLEXBODYWHEEL:
        verified_type = wheel->GetType();
        break;
    default:
        assert(false && "Unsupported wheel type");
        break;
    }
    if (verified_type == LandVehicleWheel::TYPE_INVALID)
    {
        return;
    }

    // Process wheel
    this->num_elements++;
    if (verified_type == LandVehicleWheel::TYPE_MESHWHEEL_2)
    {
        MeshWheel2* mw = static_cast<MeshWheel2*>(wheel);
        this->meshwheels2_data.AddWheel(mw);
    }
    else if (verified_type == LandVehicleWheel::TYPE_FLEXBODYWHEEL)
    {
        FlexBodyWheel* flexwheel = static_cast<FlexBodyWheel*>(wheel);
        this->flexbodywheels_data.AddWheel(flexwheel);
    }
}

void MeshWheel2AggregateData::AddWheel(MeshWheel2* data)
{
    ++num_elements;

    RigDef::MeshWheel& def = data->GetDefinition();
    if (num_elements == 1)
    {
        num_rays               = def.num_rays;
        axis_nodes[0]          = def.nodes[0];
        axis_nodes[1]          = def.nodes[1];
        rigidity_node          = def.rigidity_node;
        reference_arm_node     = def.reference_arm_node;
        propulsion             = def.propulsion;
        braking                = def.braking;
        mass                   = def.mass;

        tyre_radius            = def.tyre_radius;
        tyre_spring            = def.spring;
        tyre_damping           = def.damping;
        
        rim_radius             = def.rim_radius;
        rim_spring             = def.beam_defaults->springiness;           // Scale doesn't apply here
        rim_damping            = def.beam_defaults->damping_constant;      // Scale doesn't apply here
        rim_deform_threshold   = def.beam_defaults->deformation_threshold; // Scale doesn't apply here
        rim_breaking_threshold = def.beam_defaults->breaking_threshold;    // Scale doesn't apply here

        is_right_side          = (def.side == RigDef::MeshWheel::SIDE_RIGHT);
        rim_mesh_name          = def.mesh_name;
        tyre_material_name     = def.material_name;
    }
    else
    {
        rigidity_node     .SetRegularState_IsValid(false);
        reference_arm_node.SetRegularState_IsValid(false);
        axis_nodes[0]     .SetRegularState_IsValid(false);
        axis_nodes[1]     .SetRegularState_IsValid(false);

        SetRayCountIsUniform        (IsRayCountUniform        () && (num_rays     == def.num_rays)); 
        SetBrakingModeIsUniform     (IsBrakingModeUniform     () && (braking      == def.braking));
        SetPropulsionModeIsUniform  (IsPropulsionModeUniform  () && (propulsion   == def.propulsion));
        SetMassIsUniform            (IsMassUniform            () && (mass         == def.mass));

        SetTyreSpringIsUniform      (IsTyreSpringUniform      () && (tyre_spring  == def.spring));
        SetTyreDampingIsUniform     (IsTyreDampingUniform     () && (tyre_damping == def.damping));
        SetTyreRadiusIsUniform      (IsTyreRadiusUniform      () && (tyre_radius  == def.tyre_radius));
        
        SetRimRadiusIsUniform       (IsRimRadiusUniform       () && (rim_radius   == def.rim_radius));
        SetRimSpringIsUniform       (IsRimSpringUniform       () && (rim_spring   == def.beam_defaults->springiness)); // Scale doesn't apply here
        SetRimDampingIsUniform      (IsRimDampingUniform      () && (rim_damping            == def.beam_defaults->damping_constant)); // Scale doesn't apply here
        SetDeformThresholdIsUniform (IsDeformThresholdUniform () && (rim_deform_threshold   == def.beam_defaults->deformation_threshold)); // Scale doesn't apply here
        SetBreakThresholdIsUniform  (IsBreakThresholdUniform  () && (rim_breaking_threshold == def.beam_defaults->breaking_threshold)); // Scale doesn't apply here
        
        SetSideIsUniform            (IsSideUniform            () && (is_right_side == (def.side == RigDef::MeshWheel::SIDE_RIGHT)));
        SetRimMeshNameIsUniform     (IsRimMeshNameUniform     () && (rim_mesh_name == def.mesh_name));
        SetTyreMaterialNameIsUniform(IsTyreMaterialNameUniform() && (tyre_material_name == def.material_name));
    }
    
    return;
}

void FlexBodyWheelAggregateData::AddWheel(FlexBodyWheel* data)
{
    ++num_elements;
    RigDef::FlexBodyWheel& def = data->GetDefinition();
    if (num_elements == 1)
    {
        num_rays               = def.num_rays;
        axis_nodes[0]          = def.nodes[0];
        axis_nodes[1]          = def.nodes[1];
        rigidity_node          = def.rigidity_node;
        reference_arm_node     = def.reference_arm_node;
        propulsion             = def.propulsion;
        braking                = def.braking;
        mass                   = def.mass;

        tyre_radius            = def.tyre_radius;
        tyre_spring            = def.tyre_springiness;
        tyre_damping           = def.tyre_damping;
        
        rim_radius             = def.rim_radius;
        rim_spring             = def.beam_defaults->GetScaledSpringiness();
        rim_damping            = def.beam_defaults->GetScaledDamping();
        
        is_right_side          = (def.side == RigDef::MeshWheel::SIDE_RIGHT);
        rim_mesh_name          = def.rim_mesh_name;
        tyre_mesh_name         = def.tyre_mesh_name;
        return;
    }
    else
    {
        rigidity_node     .SetRegularState_IsValid(false);
        reference_arm_node.SetRegularState_IsValid(false);
        axis_nodes[0]     .SetRegularState_IsValid(false);
        axis_nodes[1]     .SetRegularState_IsValid(false);

        SetRayCountIsUniform        (IsRayCountUniform        () && (num_rays == def.num_rays)); 
        SetBrakingModeIsUniform     (IsBrakingModeUniform     () && (braking == def.braking));
        SetPropulsionModeIsUniform  (IsPropulsionModeUniform  () && (propulsion == def.propulsion));
        SetMassIsUniform            (IsMassUniform            () && (mass == def.mass));

        SetTyreSpringIsUniform      (IsTyreSpringUniform      () && (tyre_spring == def.tyre_springiness));
        SetTyreDampingIsUniform     (IsTyreDampingUniform     () && (tyre_damping == def.tyre_damping));
        SetTyreRadiusIsUniform      (IsTyreRadiusUniform      () && (tyre_radius == def.tyre_radius));
        
        SetRimRadiusIsUniform       (IsRimRadiusUniform       () && (rim_radius == def.rim_radius));
        SetRimSpringIsUniform       (IsRimSpringUniform       () && (rim_spring == def.beam_defaults->GetScaledSpringiness()));
        SetRimDampingIsUniform      (IsRimDampingUniform      () && (rim_damping == def.beam_defaults->GetScaledDamping()));
        
        SetSideIsUniform            (IsSideUniform            () && (is_right_side == (def.side == RigDef::MeshWheel::SIDE_RIGHT)));
        SetRimMeshNameIsUniform     (IsRimMeshNameUniform     () && (rim_mesh_name == def.rim_mesh_name));
        SetTyreMeshNameIsUniform    (IsTyreMeshNameUniform    () && (tyre_mesh_name == def.tyre_mesh_name));
    }
}
