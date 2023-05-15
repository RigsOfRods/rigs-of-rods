/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2009 Christopher Ritchey
    Copyright 2018 Petr Ohlidal

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

/// @file SlideNode.h
/// @author Christopher Ritchey
/// @date 10/30/2009

#pragma once

#include "ForwardDeclarations.h"

#include <OgreVector3.h>
#include <vector>

namespace RoR {

/// @addtogroup Physics
/// @{

/// A series of RailSegment-s for SlideNode to slide along. Can be closed in a loop.
struct RailGroup
{
    RailGroup(ActorPtr a): rg_actor(a), rg_id(-1) {}

    /// Search for closest rail segment (the one with closest node in it) in the entire RailGroup
    RailGroupSegmentID_t FindClosestSegment(Ogre::Vector3 const& point );

    /// Check if the slidenode should skip to a neighbour rail segment
    RailGroupSegmentID_t CheckCurSlideSegment(RailGroupSegmentID_t seg, Ogre::Vector3 const& point);

    /// Get prev/next segment, with loop resolution
    RailGroupSegmentID_t GetPrevSegment(RailGroupSegmentID_t curSeg) { if (curSeg > 0) return (curSeg - 1); else if (rg_segments_loop) return (static_cast<RailGroupSegmentID_t>(rg_segments.size() - 1)); else return RAILGROUPSEGMENTID_INVALID; }
    RailGroupSegmentID_t GetNextSegment(RailGroupSegmentID_t curSeg) { if (curSeg < static_cast<RailGroupSegmentID_t>(rg_segments.size() - 1)) return (curSeg + 1); else if (rg_segments_loop) return RailGroupSegmentID_t(0); else return RAILGROUPSEGMENTID_INVALID; }

    ActorPtr                 rg_actor;    //!< Actor owning this railgroup
    std::vector<BeamID_t>    rg_segments; //!< A chain of beams
    bool                     rg_segments_loop = false;
    int                      rg_id;       //!< Spawn context - matching separately defined rails with slidenodes.
};

class SlideNode
{
public:
    /// @param sliding_node valid node acting as a slide node
    /// @param rail initial RailGroup to slide on, or NULL.
    SlideNode(ActorPtr actor, NodeNum_t sliding_node, RailGroup* rail);

    /// Returns the node index of the slide node
    NodeNum_t GetSlideNodeId() { return m_sliding_node; }

    /// Updates the corrective forces and applies these forces to the beam
    /// @param dt delta time in seconds
    void UpdateForces(float dt);

    /// Checks for current rail segment and updates ideal position of the node
    void UpdatePosition();

    /// Sets rail to initially use when spawned or reset
    void SetDefaultRail(RailGroup* rail)
    {
        m_initial_railgroup = m_cur_railgroup = rail;
        this->ResetPositions();
    }

    /// Recalculates the closest position on current RailGroup
    void ResetPositions();

    /// Move back to initial rail, reset 'broken' flag and recalculate closest position
    void ResetSlideNode()
    {
        m_cur_railgroup = m_initial_railgroup;
        sn_slide_broken = false;
        this->ResetPositions();
    }

    void           SetCorThreshold(float threshold )   { m_initial_threshold = m_cur_threshold = fabs( threshold ); } //!< Distance from a beam before corrective forces take effect
    void           SetSpringRate(float rate )          { m_spring_rate = fabs( rate); }      //!< Spring force used to calculate corrective forces
    void           SetBreakForce(float breakRate )     { m_break_force = fabs( breakRate); } //!< Force required to break the Node from the Rail
    void           SetAttachmentRate(float rate )      { m_attach_rate = fabs( rate); }      //!< How long it will take for springs to fully attach to the Rail
    void           SetAttachmentDistance(float dist )  { m_attach_distance = fabs( dist); }  //!< Maximum distance this spring node is allowed to reach out for a Rail
    float          GetAttachmentDistance() const       { return m_attach_distance; }         //!< Maximum distance this spring node is allowed to reach out for a Rail

    bool sn_attach_self:1;      //!< Attach/detach to rails on the current vehicle only
    bool sn_attach_foreign:1;   //!< Attach/detach to rails only on other vehicles
    bool sn_slide_broken:1;     //!< The slidenode was pulled away from the rail

private:
    /// Calculate forces between the ideal and actual position of the sliding node.
    Ogre::Vector3 CalcCorrectiveForces();

    ActorPtr       m_actor;               //!< owner of the nodes and beams
    NodeNum_t      m_sliding_node;        //!< node that is sliding
    RailGroup*     m_initial_railgroup;   //!< Initial Rail group on spawn
    RailGroup*     m_cur_railgroup;       //!< Current Rail group, used for attachments
    RailGroupSegmentID_t m_cur_rail_seg;  //!< Current rail segment (a beam) we are sliding on
    float          m_node_forces_ratio;   //!< Ratio of length along the slide beam where the virtual node is "0.0f = p1, 1.0f = p2"
    Ogre::Vector3  m_ideal_position;      //!< Where the node SHOULD be. (World, m)
    float          m_initial_threshold;   //!< Distance from beam calculating corrective forces (m)
    float          m_cur_threshold;       //!< Distance away from beam before corrective forces begin to act on the node (m)
    float          m_spring_rate;         //!< Spring rate holding node to rail (N/m)
    float          m_break_force;         //!< Force at which Slide Node breaks away from the rail (N)
    float          m_attach_rate;         //!< How fast the cur threshold changes when attaching (i.e. how long it will take for springs to fully attach to the Rail) (m/s)
    float          m_attach_distance;     //!< Maximum distance slide node will attach to a beam (m)

// Methods //////////////////////////// TO BE SORTED /////////////////////////////////////////
public:

    /**
     * @return The current position of the SlideNode
     */
    const Ogre::Vector3& GetSlideNodePosition() const;

    /**
     * @param toAttach Which rail to attach to, Pass NULL to detach this
     * SlideNode from any rail.
     */
    void AttachToRail(RailGroup* toAttach);

    /**
     * @param beam
     * @param point
     * @return value is always positive, if beam is null return infinity
     */
    static Ogre::Real getLenTo( ActorPtr& actor, const BeamID_t beam, const Ogre::Vector3& point );

    /**
     *
     * @param beam
     * @return value is always positive, if beam is null return infinity
     */
    Ogre::Real getLenTo(ActorPtr& actor, const BeamID_t beamid) const;
};

/// @} // addtogroup Physics

} // namespace RoR

