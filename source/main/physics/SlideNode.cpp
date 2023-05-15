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

/// @file SlideNode.cpp
/// @author Christopher Ritchey
/// @date 10/30/2009

#include "SlideNode.h"

#include "Actor.h"
#include "Application.h"
#include "SimData.h"

using namespace RoR;

/**
 * Find the point on a line defined by pt1 and pt2 that
 * is nearest to a given point tp
 *       tp
 *       /|
 *    A / |
 *     /  |
 *    /   |
 *  pt1---o--------------pt2
 *     B'      B
 *
 * @param pt1 origin of two beams
 * @param pt2 end point of beam being projected onto
 * @param tp end point of beam being projected
 * @return point is space from origin
 */
Ogre::Vector3 NearestPointOnLine(const Ogre::Vector3& pt1, const Ogre::Vector3& pt2, const Ogre::Vector3& tp)
{
    Ogre::Vector3 a = tp - pt1;
    Ogre::Vector3 b = pt2 - pt1;
    Ogre::Real len = b.normalise();

    len = std::max(0.0f, std::min(a.dotProduct(b), len));

    return pt1 + b * len;
}

// SLIDE NODES IMPLEMENTATION //////////////////////////////////////////////////
SlideNode::SlideNode(ActorPtr actor, NodeNum_t slidingNode, RailGroup* slidingRail):
    m_actor(actor),
    m_sliding_node(slidingNode),
    m_initial_railgroup(slidingRail),
    m_cur_railgroup(m_initial_railgroup),
    m_cur_rail_seg(RAILGROUPSEGMENTID_INVALID),
    m_node_forces_ratio(0.0f),

    m_initial_threshold(0.0f),
    m_cur_threshold(0.0f),
    m_spring_rate(9000000),
    m_break_force(std::numeric_limits<Ogre::Real>::infinity()), // beam won't break

    m_attach_rate(1.0f),
    m_attach_distance(0.1f),
    sn_attach_foreign(false),
    sn_attach_self(false),
    sn_slide_broken(false)
{
    // make sure they exist
    ROR_ASSERT(m_actor->ar_state != ActorState::DISPOSED);
    ROR_ASSERT( m_sliding_node != NODENUM_INVALID );
}

void SlideNode::UpdateForces(float dt)
{
    ROR_ASSERT(m_actor->ar_state != ActorState::DISPOSED);

    // only do calcs if we have a beam to slide on
    if (!m_cur_railgroup || m_cur_railgroup->rg_actor->ar_beams[m_cur_railgroup->rg_segments[m_cur_rail_seg]].bm_broken || sn_slide_broken)
    {
        return;
    }

    // the threshold changes when attaching a node to a beam, if it's more than
    // the initial amount then then we're still attaching.
    if (m_cur_threshold > m_initial_threshold)
    {
        m_cur_threshold -= (m_attach_rate * dt);
    }

    Ogre::Vector3 perpForces = this->CalcCorrectiveForces();
    // perpendicular Forces are distributed according to the position along the Beam
    if (perpForces.length() > m_break_force)
    {
        sn_slide_broken = true;
    }

    m_actor->ar_nodes[m_sliding_node].Forces += -perpForces;
    beam_t& sliding_beam = m_cur_railgroup->rg_actor->ar_beams[m_cur_railgroup->rg_segments[m_cur_rail_seg]];
  
    m_actor->ar_nodes[sliding_beam.p1num].Forces += perpForces * (1 - m_node_forces_ratio);
    m_actor->ar_nodes[sliding_beam.p2num].Forces += perpForces * m_node_forces_ratio;
}

RailGroupSegmentID_t RailGroup::FindClosestSegment(const Ogre::Vector3& point)
{
    float closest_dist_sq = std::numeric_limits<float>::max();
    RailGroupSegmentID_t closest_seg = RAILGROUPSEGMENTID_INVALID;

    for (size_t i = 0; i < this->rg_segments.size(); ++i)
    {
        const float dist_sq = SlideNode::getLenTo(rg_actor, rg_segments[i], point);
        if (dist_sq < closest_dist_sq)
        {
            closest_dist_sq = dist_sq;
            closest_seg = static_cast<RailGroupSegmentID_t>(i);
        }
    }

    return closest_seg;
}

RailGroupSegmentID_t RailGroup::CheckCurSlideSegment(RailGroupSegmentID_t cur_segid, Ogre::Vector3 const& point)
{
    float closest_dist_sq = SlideNode::getLenTo(rg_actor, rg_segments[cur_segid], point);
    RailGroupSegmentID_t closest_segid = cur_segid;

    {
        const RailGroupSegmentID_t prev_segid = this->GetPrevSegment(cur_segid);
        if (prev_segid != RAILGROUPSEGMENTID_INVALID)
        {
            const float prev_dist = SlideNode::getLenTo(rg_actor, rg_segments[prev_segid], point);
            if (prev_dist < closest_dist_sq)
            {
                closest_segid = prev_segid;
                closest_dist_sq = prev_dist;
            }
        }
    }

    {
        const RailGroupSegmentID_t next_segid = this->GetNextSegment(cur_segid);
        if (next_segid != RAILGROUPSEGMENTID_INVALID)
        {
            const float prev_dist = SlideNode::getLenTo(rg_actor, rg_segments[next_segid], point);
            if (prev_dist < closest_dist_sq)
            {
                closest_segid = next_segid;
                closest_dist_sq = prev_dist;
            }
        }
    }

    return closest_segid;
}

void SlideNode::UpdatePosition()
{
    ROR_ASSERT(m_actor->ar_state != ActorState::DISPOSED);

    // only do calcs if we have a beam to slide on
    if (!m_cur_railgroup || m_actor->ar_beams[m_cur_railgroup->rg_segments[m_cur_rail_seg]].bm_broken)
    {
        m_ideal_position = m_actor->ar_nodes[m_sliding_node].AbsPosition;
        return;
    }

    // find which beam to use
    m_cur_rail_seg = m_cur_railgroup->CheckCurSlideSegment(m_cur_rail_seg, m_actor->ar_nodes[m_sliding_node].AbsPosition);

    beam_t& sliding_beam = m_cur_railgroup->rg_actor->ar_beams[m_cur_railgroup->rg_segments[m_cur_rail_seg]];
    node_t& node_p1 = m_actor->ar_nodes[sliding_beam.p1num];
    node_t& node_p2 = m_actor->ar_nodes[sliding_beam.p2num];

    // Get vector for beam
    Ogre::Vector3 b = node_p2.AbsPosition - node_p1.AbsPosition;

    // pre-compute normal
    const Ogre::Real bLen = b.normalise();

    // Get dot product along the b beam
    const Ogre::Real aDotBUnit = (m_actor->ar_nodes[m_sliding_node].AbsPosition - node_p1.AbsPosition).dotProduct(b);

    // constrain Value between the two end points
    const Ogre::Real len = std::max(0.0f, std::min(aDotBUnit, bLen));
    m_ideal_position = node_p1.AbsPosition + b * len;

    // calculate(cache) the ratio between the the two end points,
    // if bLen = 0.0f it means the beam is zero length so pick an end point
    m_node_forces_ratio = (bLen > 0.0f) ? len / bLen : 0.0f;
}

const Ogre::Vector3& SlideNode::GetSlideNodePosition() const
{
    return m_actor->ar_nodes[m_sliding_node].AbsPosition;
}

void SlideNode::ResetPositions()
{
    if (m_cur_railgroup != nullptr)
    {
        m_cur_rail_seg = m_cur_railgroup->FindClosestSegment(m_actor->ar_nodes[m_sliding_node].AbsPosition);
        this->UpdatePosition();
    }
}

Ogre::Real SlideNode::getLenTo(ActorPtr& actor, const BeamID_t beamid, const Ogre::Vector3& point)
{
    ROR_ASSERT(actor->ar_state != ActorState::DISPOSED);

    if (beamid == BEAMID_INVALID)
        return std::numeric_limits<Ogre::Real>::infinity();

    Ogre::Vector3 p1pos = actor->ar_nodes[actor->ar_beams[beamid].p1num].AbsPosition;
    Ogre::Vector3 p2pos = actor->ar_nodes[actor->ar_beams[beamid].p2num].AbsPosition;

    return (NearestPointOnLine(p1pos, p2pos, point) - point).length();
}

Ogre::Real SlideNode::getLenTo(ActorPtr& actor, const BeamID_t beamid) const
{
    return getLenTo(actor, beamid, m_actor->ar_nodes[m_sliding_node].AbsPosition);
}

Ogre::Vector3 SlideNode::CalcCorrectiveForces()
{
    Ogre::Vector3 force = (m_ideal_position - m_actor->ar_nodes[m_sliding_node].AbsPosition);
    const Ogre::Real beamLen = std::max(0.0f, force.normalise() - m_cur_threshold);
    const Ogre::Real forceLen = -m_spring_rate * beamLen;
    return (force * forceLen);
}

void SlideNode::AttachToRail(RailGroup* toAttach)
{
    m_cur_railgroup = toAttach;
    this->ResetPositions();

    // record the node's distance from the rail as threshold
    if (m_cur_railgroup)
    {
        m_cur_threshold = this->getLenTo(
            m_cur_railgroup->rg_actor, m_cur_railgroup->rg_segments[m_cur_rail_seg]);
    }
    else
    {
        m_cur_threshold = m_initial_threshold;
    }
}
