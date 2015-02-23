/*
 * node.h
 *
 *  Created on: Dec 29, 2012
 *      Author: chris
 */

#ifndef NODE_H_
#define NODE_H_

#include "OgrePrerequisites.h"

/**
* SIM-CORE; Node.
*/
struct node_t
{
	Ogre::Vector3 RelPosition; //!< relative to the local physics origin (one origin per truck) (shaky)
	Ogre::Vector3 AbsPosition; //!< absolute position in the world (shaky)
	Ogre::Vector3 Velocity;
	Ogre::Vector3 Forces;
	Ogre::Real inverted_mass;
	Ogre::Real mass;
	Ogre::Vector3 lastNormal;
	int locked;
	int iswheel; //!< 0=no, 1, 2=wheel1  3,4=wheel2, etc...
	int wheelid; //!< Wheel index
	int masstype; //!< Loaded (by vehicle cargo)? {0/1}
	int wetstate; //!< {DRY | DRIPPING | WET}
	int contactless; //!< Bool{0/1}
	int lockednode;
	int lockgroup;
	Ogre::Vector3 lockedPosition; //!< absolute
	Ogre::Vector3 lockedForces;
	Ogre::Vector3 lockedVelocity;
	int contacted; //!< Boolean
	Ogre::Real friction_coef;
	Ogre::Real buoyancy;
	Ogre::Real volume_coef;
	Ogre::Real surface_coef;
	Ogre::Vector3 lastdrag;
	Ogre::Vector3 gravimass;
	float wettime; //!< Cumulative time this node has been in contact with water. When wet, produces dripping particles.
	bool isHot; //!< Makes this node emit vapour particles when in contact with water.
	bool overrideMass;
	bool disable_particles;
	bool disable_sparks;
	Ogre::Vector3 buoyanceForce;
	int id; //!< Numeric identifier assigned in rig-definition file (if used), or -1 if the node was generated dynamically.
	int collisionBoundingBoxID;
	float collRadius;
	float collTestTimer;
	Ogre::Vector3 iPosition; //!< initial position, absolute
	Ogre::Real    iDistance; //!< initial distance from node0 during loading - used to check for loose parts
	Ogre::Vector3 smoothpos; //!< absolute, per-frame smooth, must be used for visual effects only
	bool iIsSkin;
	bool isSkin;
	bool contacter;
	int mouseGrabMode;           //!< { 0=Mouse grab, 1=No mouse grab, 2=Mouse grab with force display}
	int pos;                     //!< This node's index in rig_t::nodes array.
	Ogre::SceneNode *mSceneNode; //!< visual
};


#endif /* NODE_H_ */
