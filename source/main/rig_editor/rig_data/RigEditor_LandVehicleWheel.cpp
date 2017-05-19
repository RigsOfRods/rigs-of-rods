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

#include "RigEditor_LandVehicleWheel.h"

#include "RigDef_File.h"
#include "RigEditor_Config.h"

#include <vector>
#include <OgreVector3.h>
#include <OgreAxisAlignedBox.h>

using namespace RoR;
using namespace RigEditor;

// Old spawner (v0.4.0.7):
// WHEELS, MESHWHEELS, MESHWHEELS_2 => SerializedRig::addWheel()
// WHEELS_2                         => SerializedRig::addWheel2()
// FLEX_BODY_WHEELS                 => SerializedRig::addWheel3()

// This equals addWheel()
void LandVehicleWheel::BuildWheelNodes( 
    std::vector<Ogre::Vector3> & out_positions,
    unsigned int num_rays,
    Ogre::Vector3 const & axis_node_inner,
    Ogre::Vector3 const & axis_node_outer,
    Ogre::Vector3 const & reference_arm_node_pos,
    Ogre::Vector3* rigidity_node_ptr,
    float wheel_radius,
    Ogre::AxisAlignedBox & out_aabb
)
{
    // Prepare buffer & insert initial points
    out_positions.clear();
    out_positions.reserve(num_rays * 2);
    out_positions.push_back(axis_node_inner);
    out_positions.push_back(axis_node_outer);
    out_positions.push_back( (rigidity_node_ptr != nullptr) ? *rigidity_node_ptr : Ogre::Vector3::ZERO );

    // Find near attach
    Ogre::Real length_1 = axis_node_inner.distance(reference_arm_node_pos);
    Ogre::Real length_2 = axis_node_outer.distance(reference_arm_node_pos);
    Ogre::Vector3 const & near_attach = (length_1 < length_2) ? axis_node_inner : axis_node_outer;

    // Axis
    Ogre::Vector3 axis_vector = axis_node_outer - axis_node_inner;
    axis_vector.normalise();

    // Generated nodes
    Ogre::Vector3 ray_vector = axis_vector.perpendicular() * wheel_radius;
    Ogre::Quaternion ray_rotator = Ogre::Quaternion(Ogre::Degree(-360.0 / (num_rays * 2)), axis_vector);
    Ogre::AxisAlignedBox aabb;
    aabb.setNull();

    for (unsigned int i = 0; i < num_rays; i++)
    {
        // Outer ring
        Ogre::Vector3 pos = axis_node_inner + ray_vector;
        aabb.merge(pos);
        out_positions.push_back(pos);
        ray_vector = ray_rotator * ray_vector;

        // Inner ring
        pos = axis_node_outer + ray_vector;
        aabb.merge(pos);
        out_positions.push_back(pos);
        ray_vector = ray_rotator * ray_vector;
    }
    out_aabb = aabb;
}

void LandVehicleWheel::RigidityNodeUpdateConnectivity()
{
    // Find out where to connect rigidity node
    if (m_rigidity_node != nullptr)
    {
        const Ogre::Vector3 rigidity_pos = m_rigidity_node->GetPosition();
        const float distance_inner = rigidity_pos.squaredDistance(m_axis_inner_node->GetPosition()); // Squared is faster
        const float distance_outer = rigidity_pos.squaredDistance(m_axis_inner_node->GetPosition());
        m_rigidity_node_connect_outer_ring = distance_inner > distance_outer;
    }
}

Ogre::ColourValue const & LandVehicleWheel::Edge::GetColorValue(Edge::Type type, Config* config)
{
    switch (type)
    {
    case Edge::TYPE_FLEXBODYWHEEL_RIM_CONNECTION:     return config->flexbodywheel_rim_connection_color;
    case Edge::TYPE_FLEXBODYWHEEL_RIM_REINFORCEMENT:  return config->flexbodywheel_rim_reinforcement_color;
    case Edge::TYPE_FLEXBODYWHEEL_TYRE_CONNECTION:	  return config->flexbodywheel_tyre_connection_color;
    case Edge::TYPE_FLEXBODYWHEEL_TYRE_REINFORCEMENT: return config->flexbodywheel_tyre_reinforcement_color;
    case Edge::TYPE_FLEXBODYWHEEL_TYRE_RIGIDITY:	  return config->flexbodywheel_tyre_rigidity_color;
    case Edge::TYPE_MESHWHEEL2_BOUNDED:		          return config->meshwheel2_beam_bounded_color;
    case Edge::TYPE_MESHWHEEL2_REINFORCEMENT:         return config->meshwheel2_beam_reinforcement_color;
    case Edge::TYPE_MESHWHEEL2_RIGIDITY:              return config->meshwheel2_beam_rigidity_color;
    case Edge::TYPE_MESHWHEEL_BOUNDED:				  return config->meshwheel_beam_bounded_color;
    case Edge::TYPE_MESHWHEEL_REINFORCEMENT:		  return config->meshwheel_beam_reinforcement_color;
    case Edge::TYPE_MESHWHEEL_RIGIDITY:				  return config->meshwheel_beam_rigidity_color;

    default:
        assert(false && "Invalid wheel edge type!");
        return Ogre::ColourValue::White;
    }
}

LandVehicleWheel::~LandVehicleWheel()
{}
