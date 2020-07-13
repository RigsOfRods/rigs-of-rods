/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2020 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

/// Simulation: An edge in the softbody structure
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
    short bounded;      //!< { SHOCK1=1, SHOCK2=2, SHOCK3=3, TRIGGER=4, SUPPORTBEAM=5, ROPE=6 }
    short bm_type;      //!< { BEAM_NORMAL, BEAM_HYDRO, BEAM_VIRTUAL }
    bool bm_inter_actor;       //!< in case p2 is on another actor
    Actor* bm_locked_actor;    //!< in case p2 is on another actor

    /// Multipurpose; excludes beam from physics, controls visibility (gfx) and indicates multiple other states (hooks/ties).
    /// Users:
    ///   Actor::SyncReset()              -- WRITE: sets to true for hook-beams and tie-beams, false on all other beams.
    ///   Actor::DisjoinInterActorBeams() -- WRITE: sets to false on all beams managed as 'inter-actor'
    ///   Actor::ToggleTies(int group)   -- READ: uses it to determine locked/unlocked status.
    ///                                -- WRITE: sets true when tie gets unlocked, false when it gets locked.
    ///   Actor::ToggleHooks()             -- WRITE sets to false when hook is unlocked.
    ///   Actor::GetNumActiveConnectedBeams()    -- READ: Counts beams connected to node; Excludes 'disabled' beams.
    ///   Actor::calcForcesEulerCompute() -- READ Saves value to replay buffer
    ///   Actor::CalcBeams()            -- READ excludes beam from physics
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
    ///   Actor::updateVisual()        -- READ: when 'disabled' or 'broken', hides all visuals - and vice versa.
    ///   Actor::calcForcesEulerCompute() -- READ Excludes broken hydros from ForceFeedback
    ///                                  -- READ Saves value to replay buffer
    ///   Actor::CalcBeams()         -- WRITE: when regular beam breaks, 'true' is set to it and all beams in it's detacher group.
    ///                             -- WRITE: when SUPPORTBEAM breaks, it's set to 'disabled' + 'broken'
    ///   Actor::CalcBeamsInterActor() -- WRITE: when beam breaks (special conditions), it's set to 'disabled' + 'broken'
    ///   SlideNode::UpdateForces() -- READ: Stops updates when sliding beam is broken.
    ///   SlideNode::UpdatePosition() -- READ: Stops updates when sliding beam is broken.
    ///                NOTE: SlideNode handling may be a mistake - the feature was contributed, not developed by core team.
    /// ## TODO: Separate physics/visual meaning, create sensible usage pattern ~ only_a_ptr, 12/2017
    bool bm_broken;

    Ogre::Real shortbound;
    Ogre::Real longbound;
    Ogre::Real refL;       //!< reference length

    shock_t *shock;

    Ogre::Real initial_beam_strength; ///< for reset
    Ogre::Real default_beam_deform; ///< for reset

    Ogre::Real debug_k; //< debug shock spring_rate
    Ogre::Real debug_d; //< debug shock damping
    Ogre::Real debug_v; //< debug shock velocity
};
