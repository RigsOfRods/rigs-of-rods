/*
 * beam.h
 *
 *  Created on: Dec 29, 2012
 *      Author: chris
 */

#ifndef BEAM_H_
#define BEAM_H_

#include "RoRPrerequisites.h"
#include "OgrePrerequisites.h"

/**
* SIM-CORE; Beam data.
*/
struct beam_t
{
	node_t *p1;
	node_t *p2;
	Beam *p2truck; //!< in case p2 is on another truck
	bool disabled;
	Ogre::Real k; //!< tensile spring
	Ogre::Real d; //!< damping factor
	Ogre::Real L; //!< length
	Ogre::Real minmaxposnegstress;

	//! Beam type (unnamed enum) { BEAM_NORMAL=0, BEAM_HYDRO=1, BEAM_VIRTUAL=2, BEAM_MARKED=3, BEAM_INVISIBLE=4, BEAM_INVISIBLE_HYDRO=5 }
	int type;

	Ogre::Real maxposstress;
	Ogre::Real maxnegstress;
	Ogre::Real shortbound;
	Ogre::Real longbound;
	Ogre::Real strength;
	Ogre::Real stress;

	//! Values (unnamed enum) { SHOCK1=1, SHOCK2=2, SUPPORTBEAM=3, ROPE=4 }
	int bounded;

	bool broken;
	Ogre::Real plastic_coef;
	Ogre::Real refL;       //!< reference length
	Ogre::Real Lhydro;     //!< hydro reference len
	Ogre::Real hydroRatio; //!< hydro rotation ratio
	int hydroFlags;
	int animFlags;
	float animOption;
	Ogre::Real commandRatioLong;
	Ogre::Real commandRatioShort;
	Ogre::Real commandShort; //<! Max. contraction; proportional to orig. length
	Ogre::Real commandLong;  //<! Max. extension; proportional to orig. length
	Ogre::Real commandEngineCoupling;
	Ogre::Real maxtiestress;
	Ogre::Real diameter;
	bool commandNeedsEngine;
	int detacher_group;	//!< Attribute: detacher group number (integer)
	Ogre::Vector3 lastforce;
	bool isCentering;
	int isOnePressMode;
	bool isForceRestricted;
	float iStrength; //!< initial strength
	Ogre::Real default_deform;
	Ogre::Real default_plastic_coef;
	int autoMovingMode;
	bool autoMoveLock;
	bool pressedCenterMode;
	float centerLength;
	float minendmass;
	float scale;
	shock_t *shock;
	Ogre::SceneNode *mSceneNode; //!< visual
	Ogre::Entity *mEntity; //!< visual
};

#endif /* BEAM_H_ */
