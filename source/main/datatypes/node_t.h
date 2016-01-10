/*
 * node.h
 *
 *  Created on: Dec 29, 2012
 *      Author: chris
 */

#pragma once

#include <OgrePrerequisites.h>

/**
* SIM-CORE; Node.
*/
struct node_t
{
	Ogre::Vector3 RelPosition; //!< relative to the local physics origin (one origin per truck) (shaky)
	Ogre::Vector3 AbsPosition; //!< absolute position in the world (shaky)

	Ogre::Vector3 Velocity;
	Ogre::Vector3 Forces;
	
	float collTestTimer;
	int iswheel;  //!< 0=no, 1, 2=wheel1  3,4=wheel2, etc...
	int locked;   //!< {UNLOCKED | PRELOCK | LOCKED}

	bool contacted;
	bool contactless;
	bool disable_particles;
	bool disable_sparks;

	// <-- 64 Bytes -->

	Ogre::Real mass;

	int wheelid;  //!< Wheel index
	int lockgroup;
	int wetstate; //!< {DRY | DRIPPING | WET}
	int collisionBoundingBoxID;

	float collRadius;

	Ogre::Real buoyancy;
	Ogre::Real friction_coef;
	Ogre::Real surface_coef;
	Ogre::Real volume_coef;

	bool contacter;
	bool overrideMass;
	bool loadedMass;
	bool isHot; //!< Makes this node emit vapour particles when in contact with water.

	Ogre::Vector3 smoothpos; //!< absolute, per-frame smooth, must be used for visual effects only
	int pos;      //!< This node's index in rig_t::nodes array.
	int id;       //!< Numeric identifier assigned in rig-definition file (if used), or -1 if the node was generated dynamically.

	// <-- 128 Bytes -->
};
