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
    beam_t()                              { memset(this, 0, sizeof(beam_t)); }

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
    // removed bool
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

    Ogre::Real initial_beam_strength; ///< for reset
    Ogre::Real default_beam_deform; ///< for reset
    Ogre::Real default_beam_plastic_coef; ///< for reset
};

enum class InterBeamType
{
    IB_NONE,
    IB_HOOK,
    IB_ROPE,
    IB_TIE
};

/// IMPORTANT NOTE:
/// The update of this struct is governed by "(ib_beam::disabled == false)", not "(ib_actor_slave != nullptr)"
///   because during locking, the "ib_actor_slave" may be already filled while the beam is still disabled.
///   This is how HOOKS seem to operate.
///   **** review of previously written code needed ****
struct inter_beam_t
{
    inter_beam_t():
        ib_type(InterBeamType::IB_NONE),
        ib_entry_index(0),
        ib_actor_master(nullptr),
        ib_actor_slave(nullptr),
        ib_rest_node2(nullptr)
    {}

    beam_t         ib_beam;
    InterBeamType  ib_type;
    size_t         ib_entry_index;
    Beam*          ib_actor_master; ///< Corresponds to node1
    Beam*          ib_actor_slave;  ///< Corresponds to node2
    node_t*        ib_rest_node2;
};
