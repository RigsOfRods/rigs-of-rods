/*
 * node.h
 *
 *  Created on: Dec 29, 2012
 *      Author: chris
 */

#pragma once

#include <OgrePrerequisites.h>

#include "RoRPrerequisites.h"

/**
* SIM-CORE; Node.
*/
struct node_t
{
    node_t()               { memset(this, 0, sizeof(node_t)); }
    node_t(size_t _pos)    { memset(this, 0, sizeof(node_t)); pos = static_cast<short>(_pos); }

    Ogre::Vector3 RelPosition; //!< relative to the local physics origin (one origin per truck) (shaky)
    Ogre::Vector3 AbsPosition; //!< absolute position in the world (shaky)

    Ogre::Vector3 Velocity;
    Ogre::Vector3 Forces;

    Ogre::Real mass;
    float collTestTimer;

    // 'iswheel' was a multipurpose 'short' before; these bitflags will be padded to 2 bytes by compiler because 'short' follows. ~ only_a_ptr, 11/2017
    bool iswheel:1;
    bool iswheel_net:1; // Ulteq's workarounds for networking: https://github.com/RigsOfRods/rigs-of-rods/commit/78a7ec2f1dc3a6c2a192f31532dd82f06302802e and https://github.com/RigsOfRods/rigs-of-rods/commit/194baf439e006058bb9f3b81aea16b169a2c20d6
    bool is_wheel_left_side:1; // With regard to vehicle's orientation
    short locked;  //!< {UNLOCKED | PRELOCK | LOCKED}

    bool contacted;
    bool contactless;
    bool disable_particles;
    bool disable_sparks;

    // <-- 64 Bytes -->

    Ogre::Real buoyancy;
    Ogre::Real friction_coef;
    Ogre::Real surface_coef;
    Ogre::Real volume_coef;

    float wettime; //!< Cumulative time this node has been wet. When wet, dripping particles are produced.
    short wheelid; //!< Wheel index
    short lockgroup;
    short pos;     //!< This node's index in rig_t::nodes array.
    short id;      //!< Numeric identifier assigned in rig-definition file (if used), or -1 if the node was generated dynamically.
    char wetstate; //!< {DRY | DRIPPING | WET}
    char collisionBoundingBoxID;

    bool contacter;
    bool overrideMass;
    bool loadedMass;
    bool isHot;    //!< Makes this node emit vapour particles when in contact with water.

    Ogre::Vector3 initial_pos;
    bool          no_mouse_grab;
};
