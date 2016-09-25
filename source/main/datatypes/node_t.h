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
	Ogre::Vector3 RelPosition; //!< relative to the local physics origin (one origin per truck) (shaky)
	Ogre::Vector3 AbsPosition; //!< absolute position in the world (shaky)

	Ogre::Vector3 Velocity;
	Ogre::Vector3 Forces;
	
	Ogre::Real mass;
	float collTestTimer;
	short iswheel; //!< 0=no, 1, 2=wheel1  3,4=wheel2, etc...
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
};
