/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2014-2017 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.org/

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

#pragma once

#include "RoRPrerequisites.h"
#include <OgrePrerequisites.h>

/**
* SIM-CORE; Wheel.
*/
struct wheel_t
{
    enum class BrakeCombo /// Wheels are braked by three mechanisms: A footbrake, a handbrake/parkingbrake, and directional brakes used for skidsteer steering.
    {
        NONE,                 /// - 0 = no  footbrake, no  handbrake, no  direction control -- wheel is unbraked
        FOOT_HAND,            /// - 1 = yes footbrake, yes handbrake, no  direction control
        FOOT_HAND_SKID_LEFT,  /// - 2 = yes footbrake, yes handbrake, yes direction control (braked when vehicle steers to the left)
        FOOT_HAND_SKID_RIGHT, /// - 3 = yes footbrake, yes handbrake, yes direction control (braked when vehicle steers to the right)
        FOOT_ONLY             /// - 4 = yes footbrake, no  handbrake, no  direction control -- footbrake only, such as with the front wheels of a passenger car
    };

    int         wh_num_nodes;
    node_t*     wh_nodes[50];             // TODO: remove limit, make this dyn-allocated ~ only_a_ptr, 08/2017
    BrakeCombo  wh_braking;
    node_t*     wh_arm_node;
    node_t*     wh_near_attach_node;
    node_t*     wh_axis_node_0;
    node_t*     wh_axis_node_1;
    int         wh_propulsed;             // TODO: add enum ~ only_a_ptr, 08/2017
    Ogre::Real  wh_radius;
    Ogre::Real  wh_speed;
    Ogre::Real  wh_last_speed;
    Ogre::Real  wh_avg_speed;
    Ogre::Real  wh_antilock_coef;
    Ogre::Real  wh_tc_coef;
    Ogre::Real  wh_torque;
    float       wh_net_rp;
    float       wh_net_rp1;           //<! Networking; triple buffer
    float       wh_net_rp2;           //<! Networking; triple buffer
    float       wh_net_rp3;           //<! Networking; triple buffer
    float       wh_width;
    int         wh_detacher_group;
    bool        wh_is_detached;

    // Debug
    float debug_rpm;
    float debug_torque;
    Ogre::Vector3 debug_vel;
    Ogre::Vector3 debug_slip;
    Ogre::Vector3 debug_force;
    Ogre::Vector3 debug_scaled_cforce;
};

