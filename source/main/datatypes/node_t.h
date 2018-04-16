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

    Ogre::Vector3 RelPosition; //!< relative to the local physics origin (one origin per actor) (shaky)
    Ogre::Vector3 AbsPosition; //!< absolute position in the world (shaky)

    Ogre::Vector3 Velocity;
    Ogre::Vector3 Forces;

    Ogre::Real mass;
    float collTestTimer;
    short iswheel; //!< 0=no, 1, 2=wheel1  3,4=wheel2, etc...

    Ogre::Real buoyancy;
    Ogre::Real friction_coef;
    Ogre::Real surface_coef;
    Ogre::Real volume_coef;

    short wheelid; //!< Wheel index
    short nd_lockgroup;
    short pos;     //!< This node's index in Actor::ar_nodes array.
    short id;      //!< Numeric identifier assigned in truckfile (if used), or -1 if the node was generated dynamically.
    char collisionBoundingBoxID;

    bool overrideMass;
    bool loadedMass;

    Ogre::Vector3 initial_pos;

    ground_model_t* nd_collision_gm;    //!< Physics state; last collision 'ground model' (surface definition)
    float           nd_collision_slip;  //!< Physics state; last collision slip velocity
    
    // Bit flags
    bool           nd_immovable: 1;     //!< Attr; User-defined
    bool           nd_no_mouse_grab:1;  //!< Attr; User-defined
    bool           nd_contacter:1;      //!< Attr; This node is part of collision triangle
    bool           nd_no_ground_contact:1;  //!< User-defined attr; node ignores contact with ground
    bool           nd_has_contact:1;    //!< Physics state
    bool           nd_under_water:1;    //!< State; GFX hint
};
