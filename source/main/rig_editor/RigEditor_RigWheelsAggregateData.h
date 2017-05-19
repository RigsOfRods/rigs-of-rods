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

    AggregateData structures provide a connection between rig elements and associated GUI panels.
    Usage:
        * Query info about selected elements in rig.
        * Provide data backend for GUI panel associated with the element.
        * Bulk update of selected elements in rig.

    Contents of *AggregateData structure:
        * Element count
        * Data fields (element-specific)
        * Uniformity flags (1 per each data field).
          - TRUE if the field's value is uniform across all aggregated elements, FALSE if not.
*/

#pragma once

#include "ConfigFile.h"
#include "GUI_OpenSaveFileDialog.h"
#include "RigDef_Node.h"
#include "RigDef_File.h"
#include "RigDef_Prerequisites.h"
#include "RigEditor_IMain.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RoRPrerequisites.h"

namespace RoR
{

namespace RigEditor
{

struct BaseWheelAggregateData
{
    virtual ~BaseWheelAggregateData(){}

    // Element count
    int num_elements;

    // Element data fields
    unsigned int num_rays;
    RigDef::Node::Ref axis_nodes[2];
    RigDef::Node::Ref rigidity_node;
    RigDef::Node::Ref reference_arm_node;
    RigDef::Wheels::Braking braking;
    RigDef::Wheels::Propulsion propulsion;
    float mass;

    virtual void Reset()
    {
        memset(this, 0, sizeof(BaseWheelAggregateData));
        m_flags = 0xFFFFFFFF; // Make uniformity flags TRUE by default
    }

    BITMASK_PROPERTY(m_flags,  1,  FIELD_RAY_COUNT_IS_UNIFORM            , IsRayCountUniform           , SetRayCountIsUniform)
    BITMASK_PROPERTY(m_flags,  2,  FIELD_BRAKING_MODE_IS_UNIFORM         , IsBrakingModeUniform        , SetBrakingModeIsUniform)
    BITMASK_PROPERTY(m_flags,  3,  FIELD_PROPULSION_MODE_IS_UNIFORM      , IsPropulsionModeUniform     , SetPropulsionModeIsUniform)
    BITMASK_PROPERTY(m_flags,  4,  FIELD_MASS_IS_UNIFORM                 , IsMassUniform               , SetMassIsUniform)

protected:
    unsigned int m_flags;
};

struct MeshWheel2AggregateData: public BaseWheelAggregateData
{
    // Tyre
    float tyre_radius;
    float tyre_spring;
    float tyre_damping;

    // Rim
    float rim_radius;
    float rim_spring;
    float rim_damping;
    float rim_deform_threshold;
    float rim_breaking_threshold;

    // Other
    bool           is_right_side;
    std::string    rim_mesh_name;
    std::string    tyre_material_name;
    
    // NOTE: 1-10 is reserved for BaseWheelAggregateData
    BITMASK_PROPERTY(m_flags, 11,  FIELD_TYRE_SPRING_IS_UNIFORM            , IsTyreSpringUniform          , SetTyreSpringIsUniform)
    BITMASK_PROPERTY(m_flags, 12,  FIELD_TYRE_DAMPING_IS_UNIFORM           , IsTyreDampingUniform         , SetTyreDampingIsUniform)
    BITMASK_PROPERTY(m_flags, 13,  FIELD_TYRE_RADIUS_IS_UNIFORM            , IsTyreRadiusUniform          , SetTyreRadiusIsUniform)
    BITMASK_PROPERTY(m_flags, 14,  FIELD_RIM_RADIUS_IS_UNIFORM             , IsRimRadiusUniform           , SetRimRadiusIsUniform)
    BITMASK_PROPERTY(m_flags, 15,  FIELD_RIM_SPRING_IS_UNIFORM             , IsRimSpringUniform           , SetRimSpringIsUniform)
    BITMASK_PROPERTY(m_flags, 16,  FIELD_RIM_DAMPING_IS_UNIFORM            , IsRimDampingUniform          , SetRimDampingIsUniform)
    BITMASK_PROPERTY(m_flags, 17,  FIELD_RIM_DEFORM_THRESHOLD_IS_UNIFORM   , IsDeformThresholdUniform     , SetDeformThresholdIsUniform)
    BITMASK_PROPERTY(m_flags, 18,  FIELD_RIM_BREAK_THRESHOLD_IS_UNIFORM    , IsBreakThresholdUniform      , SetBreakThresholdIsUniform)
    BITMASK_PROPERTY(m_flags, 19,  FIELD_SIDE_IS_UNIFORM                   , IsSideUniform                , SetSideIsUniform)
    BITMASK_PROPERTY(m_flags, 20,  FIELD_RIM_MESH_NAME_IS_UNIFORM          , IsRimMeshNameUniform         , SetRimMeshNameIsUniform)
    BITMASK_PROPERTY(m_flags, 21,  FIELD_TYRE_MATERIAL_NAME_IS_UNIFORM     , IsTyreMaterialNameUniform    , SetTyreMaterialNameIsUniform)

    virtual void Reset()
    {
        memset(this, 0, sizeof(MeshWheel2AggregateData));
        m_flags = 0xFFFFFFFF; // Make uniformity flags TRUE by default
    }

    inline unsigned int* GetFlagsPtr() { return &m_flags; }

    void AddWheel(MeshWheel2* data);

private:

    // m_flags ~~ Defined in BaseWheelAggregateData
};

struct FlexBodyWheelAggregateData: public BaseWheelAggregateData
{
    //;radius, rimradius, width, rays, n1, n2, ref-n, braked, propulsed, force-n, weight, tire-spring, tire-damp, rim-spring, rim-damp, rim-orientation,           rim-mesh            tire-mesh
    //   0.50,     0.300, 0.300,   16, 13, 11,  9999,      1,         1,      19,   92.5,      4500.0,     300.0,    3000000,      350,               r, testtruck-rim.mesh testtruck-wheel.mesh

    // Tyre
    float tyre_radius;
    float tyre_spring;
    float tyre_damping;

    // Rim
    float rim_radius;
    float rim_spring;
    float rim_damping;

    // Other
    bool           is_right_side;
    std::string    rim_mesh_name;
    std::string    tyre_mesh_name;
    
    // NOTE: 1-10 is reserved for BaseWheelAggregateData
    BITMASK_PROPERTY(m_flags, 11,  FIELD_TYRE_SPRING_IS_UNIFORM            , IsTyreSpringUniform          , SetTyreSpringIsUniform)
    BITMASK_PROPERTY(m_flags, 12,  FIELD_TYRE_DAMPING_IS_UNIFORM           , IsTyreDampingUniform         , SetTyreDampingIsUniform)
    BITMASK_PROPERTY(m_flags, 13,  FIELD_TYRE_RADIUS_IS_UNIFORM            , IsTyreRadiusUniform          , SetTyreRadiusIsUniform)
    BITMASK_PROPERTY(m_flags, 14,  FIELD_RIM_RADIUS_IS_UNIFORM             , IsRimRadiusUniform           , SetRimRadiusIsUniform)
    BITMASK_PROPERTY(m_flags, 15,  FIELD_RIM_SPRING_IS_UNIFORM             , IsRimSpringUniform           , SetRimSpringIsUniform)
    BITMASK_PROPERTY(m_flags, 16,  FIELD_RIM_DAMPING_IS_UNIFORM            , IsRimDampingUniform          , SetRimDampingIsUniform)
    BITMASK_PROPERTY(m_flags, 17,  FIELD_SIDE_IS_UNIFORM                   , IsSideUniform                , SetSideIsUniform)
    BITMASK_PROPERTY(m_flags, 18,  FIELD_RIM_MESH_NAME_IS_UNIFORM          , IsRimMeshNameUniform         , SetRimMeshNameIsUniform)
    BITMASK_PROPERTY(m_flags, 19,  FIELD_TYRE_MESH_NAME_IS_UNIFORM         , IsTyreMeshNameUniform        , SetTyreMeshNameIsUniform)

    virtual void Reset()
    {
        memset(this, 0, sizeof(FlexBodyWheelAggregateData));
        m_flags = 0xFFFFFFFF; // Make uniformity flags TRUE by default
    }

    inline unsigned int* GetFlagsPtr() { return &m_flags; }

    void AddWheel(FlexBodyWheel* data);

private:

    // m_flags ~~ Defined in BaseWheelAggregateData
};

struct AllWheelsAggregateData
{
    void AddWheel(LandVehicleWheel* wheel);
    void Reset()
    {
        num_elements = 0;
        meshwheels2_data.Reset();
        flexbodywheels_data.Reset();
    }
    bool ContainsMultipleWheelTypes()
    {
        int num_types = 0;
        if (meshwheels2_data.num_elements != 0) { ++num_types; }
        if (flexbodywheels_data.num_elements != 0) { ++num_types; }
        return (num_types > 1);
    }

    FlexBodyWheelAggregateData    flexbodywheels_data;
    MeshWheel2AggregateData       meshwheels2_data;
    int num_elements;
};


} // namespace RigEditor

} // namespace RoR
