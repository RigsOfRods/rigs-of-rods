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
#include "RigEditor_Node.h"
#include "RigEditor_Types.h"

#include <OgreColourValue.h>
#include <OgreVector3.h>
#include <vector>

namespace RoR
{

namespace RigEditor
{

class LandVehicleWheel
{
public:

    struct Edge
    {
        enum Type
        {
            TYPE_FLEXBODYWHEEL_RIM_CONNECTION,
            TYPE_FLEXBODYWHEEL_RIM_REINFORCEMENT,
            TYPE_FLEXBODYWHEEL_TYRE_CONNECTION,
            TYPE_FLEXBODYWHEEL_TYRE_REINFORCEMENT,
            TYPE_FLEXBODYWHEEL_TYRE_RIGIDITY,

            TYPE_MESHWHEEL_BOUNDED,
            TYPE_MESHWHEEL_REINFORCEMENT,
            TYPE_MESHWHEEL_RIGIDITY,

            TYPE_MESHWHEEL2_BOUNDED,
            TYPE_MESHWHEEL2_REINFORCEMENT,
            TYPE_MESHWHEEL2_RIGIDITY,

            TYPE_INVALID = 0xFFFFFFFF
        };

        Edge(unsigned index1, unsigned index2, Type type):
            type(type)
        {
            indices[0] = index1;	
            indices[1] = index2;
        }

        static Ogre::ColourValue const & GetColorValue(Type type, Config* config);

        Type type;
        unsigned int indices[2];
    };

    enum Type
    {
        TYPE_WHEEL,
        TYPE_WHEEL_2,
        TYPE_MESHWHEEL,
        TYPE_MESHWHEEL_2,
        TYPE_FLEXBODYWHEEL,

        TYPE_INVALID = 0xFFFFFFFF
    };

    LandVehicleWheel(Type type):
        m_type(type),
        m_flags(0),
        m_axis_inner_node(nullptr),
        m_axis_outer_node(nullptr),
        m_rigidity_node(nullptr),
        m_reference_arm_node(nullptr),
        m_rigidity_node_connect_outer_ring(false)
    {
        SetGeometryIsDirty(true);
        m_aabb.setNull();
    }

    virtual ~LandVehicleWheel();

    BITMASK_PROPERTY(m_flags,  1, IS_GEOMETRY_DIRTY,          IsGeometryDirty,         SetGeometryIsDirty)
    BITMASK_PROPERTY(m_flags,  2, IS_SELECTION_STATE_DIRTY,   IsSelectionStateDirty,   SetSelectionStateIsDirty)
    BITMASK_PROPERTY(m_flags,  3, IS_SELECTED,                IsSelected,              SetIsSelected)
    BITMASK_PROPERTY(m_flags,  4, IS_HOVERED,                 IsHovered,               SetIsHovered)
    BITMASK_PROPERTY(m_flags,  5, HAS_RAY_COUNT_CHANGED,      HasRayCountChanged,      SetHasRayCountChanged)
    // When user clicks "select this wheel", the wheel is scheduled for selection
    // Actual selection is performed during main-loop update
    BITMASK_PROPERTY(m_flags,  6, IS_SCHEDULED_FOR_SELECT,    IsScheduledForSelect,    SetIsScheduledForSelect)
    BITMASK_PROPERTY(m_flags,  7, IS_SCHEDULED_FOR_DESELECT,  IsScheduledForDeselect,  SetIsScheduledForDeselect)

    // Type
    inline Type GetType() const { return m_type; }

    // Nodes
    inline void SetAxisInnerNode(Node* node) { m_axis_inner_node = node; }
    inline void SetAxisOuterNode(Node* node) { m_axis_outer_node = node; }
    inline void SetReferenceArmNode(Node* node) { m_reference_arm_node = node; }
    inline void SetRigidityNode (Node* node) 
    { 
        m_rigidity_node = node;
        RigidityNodeUpdateConnectivity();
    }
    inline Node* GetAxisInnerNode() { return m_axis_inner_node; }
    inline Node* GetAxisOuterNode() { return m_axis_outer_node; }
    inline Node* GetRigidityNode () { return m_rigidity_node; }
    /// Find out where to connect rigidity node
    void RigidityNodeUpdateConnectivity();

    // Geometry
    static void BuildWheelNodes( 
        std::vector<Ogre::Vector3> & out_positions,
        unsigned int num_rays,
        Ogre::Vector3 const & axis_node_inner,
        Ogre::Vector3 const & axis_node_outer,
        Ogre::Vector3 const & reference_arm_node,
        Ogre::Vector3* rigidity_node_ptr,
        float wheel_radius,
        Ogre::AxisAlignedBox & out_aabb
        );
    inline std::vector<Ogre::Vector3>& GetVertices() { return m_vertices; }
    inline std::vector<Edge>& GetEdges() { return m_edges; }
    virtual void ReGenerateMeshData() {}
    virtual void Update(AllWheelsAggregateData *data) {}

    inline Ogre::Vector3 GetAabbEdgesMin(float padding = 0.f) const
    {
        return m_aabb.getMinimum() - Ogre::Vector3(padding, padding, padding); 
    }
    inline Ogre::Vector3 GetAabbEdgesMax(float padding = 0.f) const
    { 
        return m_aabb.getMaximum() + Ogre::Vector3(padding, padding, padding); 
    }

protected:
    Node*                        m_axis_inner_node;
    Node*                        m_axis_outer_node;
    Node*                        m_rigidity_node;
    bool                         m_rigidity_node_connect_outer_ring;
    Node*                        m_reference_arm_node;
    std::vector<Ogre::Vector3>   m_vertices;
    std::vector<Edge>            m_edges;
    unsigned int                 m_flags;
    Ogre::AxisAlignedBox         m_aabb;
private:
    Type                         m_type;
};

} // namespace RigEditor

} // namespace RoR
