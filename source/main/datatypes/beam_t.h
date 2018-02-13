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
    bool p2truck;       //!< in case p2 is on another truck

    /// Multipurpose; excludes beam from physics, controls visibility (gfx) and indicates multiple other states (hooks/ties).
    /// Users:
    ///   Beam::SyncReset()              -- WRITE: sets to true for hook-beams and tie-beams, false on all other beams.
    ///   Beam::disjoinInterTruckBeams() -- WRITE: sets to false on all beams managed as 'inter-truck'
    ///   Beam::tieToggle(int group)   -- READ: uses it to determine locked/unlocked status.
    ///                                -- WRITE: sets true when tie gets unlocked, false when it gets locked.
    ///   Beam::hookToggle()             -- WRITE sets to false when hook is unlocked.
    ///   Beam::nodeBeamConnections()    -- READ: Counts beams connected to node; Excludes 'disabled' beams.
    ///   Beam::calcForcesEulerCompute() -- READ Saves value to replay buffer
    ///   Beam::calcBeams()            -- READ excludes beam from physics
    ///                                -- WRITE: when SUPPORTBEAM breaks, it's set to true.
    ///                                -- WRITE: when regular beam breaks, 'true' is set to it and all beams in it's detacher group.
    ///   Beam::calcBeamsInterTruck()    -- READ: excludes beam from physics
    ///                                  -- WRITE: when beam breaks (special conditions), it's set to true.
    ///   Beam::calcHooks() -- READ/WRITE: If disabled during locking, it's enabled
    ///                     -- WRITE: When locking fails, beam is reset to disabled
    ///   Beam::updateVisual() -- READ: when 'disabled', hides all visuals. This is probably why 'disabled' is always set for 'broken' beams.
    ///   RigSpawner* -- WRITE: Ties and hook-beams are init to disabled. Others to enabled.
    /// ## TODO: Separate physics/visual meaning, create sensible usage pattern ~ only_a_ptr, 12/2017
    bool bm_disabled;
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
