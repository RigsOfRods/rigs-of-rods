/*
 * beam.h
 *
 *  Created on: Dec 29, 2012
 *      Author: chris
 */

#pragma once

#include <OgrePrerequisites.h>

#include "RoRPrerequisites.h"

/**
* SIM-CORE; Beam data.
*/
struct beam_t
{
    node_t *p1;
    node_t *p2;
    Ogre::Real k; //!< tensile spring
    Ogre::Real d; //!< damping factor
    Ogre::Real L; //!< length
    Ogre::Real minmaxposnegstress;
    Ogre::Real maxposstress;
    Ogre::Real maxnegstress;
    Ogre::Real strength;
    Ogre::Real stress;
    Ogre::Real plastic_coef;
    int detacher_group;	//!< Attribute: detacher group number (integer)
    short bounded;      //!< { SHOCK1=1, SHOCK2=2, SUPPORTBEAM=3, ROPE=4 }
    short type;         //!< { BEAM_NORMAL=0, BEAM_HYDRO=1, BEAM_VIRTUAL=2, BEAM_MARKED=3, BEAM_INVISIBLE=4, BEAM_INVISIBLE_HYDRO=5 }
    bool p2truck;       //!< in case p2 is on another truck
    bool disabled;
    bool broken;
    bool autoMoveLock;

    // < -- 64 Bytes -->

    Ogre::Real shortbound;
    Ogre::Real longbound;
    Ogre::Real refL;       //!< reference length
    Ogre::Real Lhydro;     //!< hydro reference len
    Ogre::Real hydroRatio; //!< hydro rotation ratio
    Ogre::Real commandRatioLong;
    Ogre::Real commandRatioShort;
    Ogre::Real commandShort; //<! Max. contraction; proportional to orig. length
    Ogre::Real commandLong;  //<! Max. extension; proportional to orig. length
    Ogre::Real commandEngineCoupling;
    float centerLength;
    float animOption;
    int animFlags;
    int hydroFlags;
    short isOnePressMode;
    char autoMovingMode;
    bool playsSound;
    bool pressedCenterMode;
    bool isForceRestricted;
    bool commandNeedsEngine;
    bool isCentering;

    // < -- 128 Bytes -->

    Ogre::Real maxtiestress;
    Ogre::Real diameter;

    shock_t *shock;
    Ogre::SceneNode *mSceneNode; //!< visual
    Ogre::Entity *mEntity; //!< visual
};
