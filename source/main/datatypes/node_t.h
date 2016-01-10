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
	Ogre::SceneNode *mSceneNode; //!< visual

	Ogre::Vector3 RelPosition; //!< relative to the local physics origin (one origin per truck) (shaky)
	Ogre::Vector3 AbsPosition; //!< absolute position in the world (shaky)
	Ogre::Vector3 smoothpos; //!< absolute, per-frame smooth, must be used for visual effects only
	Ogre::Vector3 iPosition; //!< initial position, absolute

	Ogre::Vector3 Velocity;

	Ogre::Vector3 Forces;
	Ogre::Vector3 buoyanceForce;

	Ogre::Vector3 lastdrag;
	Ogre::Vector3 gravimass;

	Ogre::Vector3 lockedPosition; //!< absolute
	Ogre::Vector3 lockedForces;
	Ogre::Vector3 lockedVelocity;

	Ogre::Real mass;
	Ogre::Real buoyancy;
	Ogre::Real friction_coef;
	Ogre::Real surface_coef;
	Ogre::Real volume_coef;

	float wettime; //!< Cumulative time this node has been in contact with water. When wet, produces dripping particles.
	float collRadius;
	float collTestTimer;

	int collisionBoundingBoxID;
	int contacted; //!< Boolean
	int contactless; //!< Bool{0/1}
	int id; //!< Numeric identifier assigned in rig-definition file (if used), or -1 if the node was generated dynamically.
	int iswheel; //!< 0=no, 1, 2=wheel1  3,4=wheel2, etc...
	int locked;
	int lockgroup;
	int masstype; //!< Loaded (by vehicle cargo)? {0/1}
	int mouseGrabMode;           //!< { 0=Mouse grab, 1=No mouse grab, 2=Mouse grab with force display}
	int pos;                     //!< This node's index in rig_t::nodes array.
	int wetstate; //!< {DRY | DRIPPING | WET}
	int wheelid; //!< Wheel index

	bool contacter;
	bool disable_particles;
	bool disable_sparks;
	bool isHot; //!< Makes this node emit vapour particles when in contact with water.
	bool overrideMass;
};
