/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2018 Petr Ohlidal & contributors

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

#include <OgrePrerequisites.h>

#include "RoRPrerequisites.h"

/// Physics: A vertex in the softbody structure
struct node_t
{
    // REFACTOR IN PROGRESS: Currently nodes are adressed mostly by pointers or int32_t indices,
    //     although there was always a hidden soft limit of 2^16 nodes (because of `short node_t::pos`).
    //     Let's use `uint16_t` indices everywhere to be clear.      ~ only_a_ptr, 04/2018
    static const uint16_t INVALID_IDX = std::numeric_limits<uint16_t>::max();
    static const int8_t   INVALID_BBOX = -1;

    node_t()               { memset(this, 0, sizeof(node_t)); nd_coll_bbox_id = INVALID_BBOX; }
    node_t(size_t _pos)    { memset(this, 0, sizeof(node_t)); nd_coll_bbox_id = INVALID_BBOX; pos = static_cast<short>(_pos); }

    Ogre::Vector3 RelPosition; //!< relative to the local physics origin (one origin per actor) (shaky)
    Ogre::Vector3 AbsPosition; //!< absolute position in the world (shaky)

    Ogre::Vector3 Velocity;
    Ogre::Vector3 Forces;

    Ogre::Real mass;

    Ogre::Real buoyancy;
    Ogre::Real friction_coef;
    Ogre::Real surface_coef;
    Ogre::Real volume_coef;

    short iswheel; //!< 0=no, 1, 2=wheel1  3,4=wheel2, etc...
    short wheelid; //!< Wheel index
    short nd_lockgroup;
    short pos;     //!< This node's index in Actor::ar_nodes array.
    short id;      //!< Numeric identifier assigned in truckfile (if used), or -1 if the node was generated dynamically.

    Ogre::Vector3 initial_pos;

    Ogre::Vector3   nd_last_collision_slip;  //!< Physics state; last collision slip vector
    Ogre::Vector3   nd_last_collision_force; //!< Physics state; last collision force
    ground_model_t* nd_last_collision_gm;    //!< Physics state; last collision 'ground model' (surface definition)
    int8_t          nd_coll_bbox_id;         //!< Optional attribute (-1 = none) - multiple collision bounding boxes defined in truckfile

    // Bit flags
    bool            nd_loaded_mass:1;        //!< User defined attr; mass is calculated from 'globals/loaded-mass' rather than 'globals/dry-mass'
    bool            nd_override_mass:1;      //!< User defined attr; mass is user-specified rather than calculated (override the calculation)
    bool            nd_immovable: 1;         //!< Attr; User-defined
    bool            nd_no_mouse_grab:1;      //!< Attr; User-defined
    bool            nd_contacter:1;          //!< Attr; This node is part of collision triangle
    bool            nd_no_ground_contact:1;  //!< User-defined attr; node ignores contact with ground
    bool            nd_has_ground_contact:1; //!< Physics state
    bool            nd_under_water:1;        //!< State; GFX hint
};
