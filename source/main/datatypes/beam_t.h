/*
 * beam.h
 *
 *  Created on: Dec 29, 2012
 *      Author: chris
 */

#pragma once

#include <OgrePrerequisites.h>

/**
* SIM-CORE; Beam data.
*/
struct beam_t
{
	Ogre::SceneNode *mSceneNode; //!< visual
	Ogre::Entity *mEntity; //!< visual

	node_t *p1;
	node_t *p2;
	Beam *p2truck; //!< in case p2 is on another truck
	shock_t *shock;

	Ogre::Real k; //!< tensile spring
	Ogre::Real d; //!< damping factor
	Ogre::Real L; //!< length
	Ogre::Real minmaxposnegstress;

	Ogre::Real maxposstress;
	Ogre::Real maxnegstress;
	Ogre::Real shortbound;
	Ogre::Real longbound;
	Ogre::Real strength;
	Ogre::Real stress;

	Ogre::Real plastic_coef;
	Ogre::Real refL;       //!< reference length
	Ogre::Real Lhydro;     //!< hydro reference len
	Ogre::Real hydroRatio; //!< hydro rotation ratio

	Ogre::Real commandRatioLong;
	Ogre::Real commandRatioShort;
	Ogre::Real commandShort; //<! Max. contraction; proportional to orig. length
	Ogre::Real commandLong;  //<! Max. extension; proportional to orig. length
	Ogre::Real commandEngineCoupling;
	Ogre::Real maxtiestress;
	Ogre::Real diameter;
	Ogre::Real default_deform;
	Ogre::Real default_plastic_coef;

	float animOption;
	float centerLength;
	float iStrength; //!< initial strength
	float scale;

	//! Values (unnamed enum) { SHOCK1=1, SHOCK2=2, SUPPORTBEAM=3, ROPE=4 }
	int bounded;
	//! Beam type (unnamed enum) { BEAM_NORMAL=0, BEAM_HYDRO=1, BEAM_VIRTUAL=2, BEAM_MARKED=3, BEAM_INVISIBLE=4, BEAM_INVISIBLE_HYDRO=5 }
	int type;
	int hydroFlags;
	int animFlags;
	int autoMovingMode;
	int detacher_group;	//!< Attribute: detacher group number (integer)
	int isOnePressMode;

	bool autoMoveLock;
	bool commandNeedsEngine;
	bool isCentering;
	bool isForceRestricted;
	bool pressedCenterMode;

	bool disabled;
	bool broken;
};
