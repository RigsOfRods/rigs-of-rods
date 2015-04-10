/*
 * wheel.h
 *
 *  Created on: Dec 29, 2012
 *      Author: chris
 */

#ifndef WHEEL_H_
#define WHEEL_H_
#include "RoRPrerequisites.h"
#include <OgrePrerequisites.h>

/**
* SIM-CORE; Wheel.
*/
struct wheel_t
{
	int nbnodes;
	node_t* nodes[50];
	/**
	 * Defines the braking characteristics of a wheel. Wheels are braked by three mechanisms:
	 * A footbrake, a handbrake/parkingbrake, and directional brakes used for skidsteer steering.
	 * - 0 = no  footbrake, no  handbrake, no  direction control -- wheel is unbraked
	 * - 1 = yes footbrake, yes handbrake, no  direction control
	 * - 2 = yes footbrake, yes handbrake, yes direction control (braked when truck steers to the left)
	 * - 3 = yes footbrake, yes handbrake, yes direction control (braked when truck steers to the right)
	 * - 4 = yes footbrake, no  handbrake, no  direction control -- wheel has footbrake only, such as with the front wheels of a normal car
	 **/
	int braked;
	node_t* arm;
	node_t* near_attach;
	node_t* refnode0;
	node_t* refnode1;
	int propulsed;
	Ogre::Real radius;
	Ogre::Real speed;
	Ogre::Real delta_rotation; //!< Difference in wheel position
	float rp;
	float rp1; //<! Networking; triple buffer
	float rp2; //<! Networking; triple buffer
	float rp3; //<! Networking; triple buffer
	float width;

	// for skidmarks
	Ogre::Vector3 lastContactInner;
	Ogre::Vector3 lastContactOuter;
	Ogre::Vector3 lastRotationVec;
	bool firstLock;
	float lastSlip;
	int lastContactType;
	ground_model_t *lastGroundModel;
	
	//skidmarks v2
	bool isSkiding;

	// for improved collision code
	int lastEventHandler;
};

/**
* SIM-CORE; Visual wheel.
*/
struct vwheel_t
{
	node_t *p1;
	node_t *p2;
	Flexable *fm;
	Ogre::SceneNode *cnode;
	bool meshwheel;
};


#endif /* WHEEL_H_ */
