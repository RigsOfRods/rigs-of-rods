/*
 * beam.h
 *
 *  Created on: Dec 29, 2012
 *      Author: chris
 */

#pragma once

#include <OgrePrerequisites.h>

#include "RoRPrerequisites.h"

/// Simulation: An edge in the softbody structure
///  ##TODO: This struct is a mess - it's blatantly fat (bad CPU cache use) and carries data which it shouldn't (gfx/special-cases)
///          There were attempts to sort the data, but in current state, it's no use.
///          PLAN: Until v0.5, this struct will be left mostly as-is for stability. ~ only_a_ptr, 12/2017
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
    short bm_type;      //!< { BEAM_NORMAL, BEAM_HYDRO, BEAM_VIRTUAL, BEAM_INVISIBLE, BEAM_INVISIBLE_HYDRO }
    bool bm_inter_actor;       //!< in case p2 is on another actor

    /// Multipurpose; excludes beam from physics, controls visibility (gfx) and indicates multiple other states (hooks/ties).
    /// Users:
    ///   Actor::SyncReset()              -- WRITE: sets to true for hook-beams and tie-beams, false on all other beams.
    ///   Actor::DisjoinInterActorBeams() -- WRITE: sets to false on all beams managed as 'inter-actor'
    ///   Actor::ToggleTies(int group)   -- READ: uses it to determine locked/unlocked status.
    ///                                -- WRITE: sets true when tie gets unlocked, false when it gets locked.
    ///   Actor::ToggleHooks()             -- WRITE sets to false when hook is unlocked.
    ///   Actor::GetNumActiveConnectedBeams()    -- READ: Counts beams connected to node; Excludes 'disabled' beams.
    ///   Actor::calcForcesEulerCompute() -- READ Saves value to replay buffer
    ///   Actor::ReplayStep()             -- WRITE: fills from replay buffer
    ///   Actor::calcBeams()            -- READ excludes beam from physics
    ///                                -- WRITE: when SUPPORTBEAM breaks, it's set to 'disabled' + 'broken'
    ///                                -- WRITE: when regular beam breaks, 'true' is set to it and all beams in it's detacher group.
    ///   Actor::CalcBeamsInterActor()    -- READ: excludes beam from physics
    ///                                  -- WRITE: when beam breaks (special conditions), it's set to 'disabled' + 'broken'
    ///   Actor::calcHooks() -- READ/WRITE: If disabled during locking, it's enabled
    ///                     -- WRITE: When locking fails, beam is reset to disabled
    ///   Actor::updateVisual() -- READ: when 'disabled' or 'broken', hides all visuals - and vice versa.
    ///   ActorSpawner* -- WRITE: Ties and hook-beams are init to disabled. Others to enabled.
    /// ## TODO: Separate physics/visual meaning, create sensible usage pattern ~ only_a_ptr, 12/2017
    bool bm_disabled;

    /// Multipurpose: excludes beam from physics (slidenodes) and force feedback, controls visibility
    ///   Actor::SyncReset()           -- WRITE: set to false
    ///   Actor::ReplayStep()          -- WRITE: fills from replay buffer
    ///   Actor::updateVisual()        -- READ: when 'disabled' or 'broken', hides all visuals - and vice versa.
    ///   Actor::calcForcesEulerCompute() -- READ Excludes broken hydros from ForceFeedback
    ///                                  -- READ Saves value to replay buffer
    ///   Actor::calcBeams()         -- WRITE: when regular beam breaks, 'true' is set to it and all beams in it's detacher group.
    ///                             -- WRITE: when SUPPORTBEAM breaks, it's set to 'disabled' + 'broken'
    ///   Actor::CalcBeamsInterActor() -- WRITE: when beam breaks (special conditions), it's set to 'disabled' + 'broken'
    ///   SlideNode::UpdateForces() -- READ: Stops updates when sliding beam is broken.
    ///   SlideNode::UpdatePosition() -- READ: Stops updates when sliding beam is broken.
    ///                NOTE: SlideNode handling may be a mistake - the feature was contributed, not developed by core team.
    /// ## TODO: Separate physics/visual meaning, create sensible usage pattern ~ only_a_ptr, 12/2017
    bool bm_broken;

    bool autoMoveLock;

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

    Ogre::Real maxtiestress;
    Ogre::Real diameter;

    shock_t *shock;
    Ogre::SceneNode *mSceneNode; //!< visual
    Ogre::Entity *mEntity; //!< visual

    Ogre::Real initial_beam_strength; ///< for reset
    Ogre::Real default_beam_deform; ///< for reset
    Ogre::Real default_beam_plastic_coef; ///< for reset
};
