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
SlideNode::SlideNode(node_t* slidingNode, RailGroup* slidingRail):

    m_sliding_node(slidingNode),
    m_sliding_beam(NULL),
    m_initial_railgroup(slidingRail),
    m_cur_railgroup(m_initial_railgroup),
    m_cur_rail_seg(NULL),
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
    ROR_ASSERT( m_sliding_node );
}

void SlideNode::UpdateForces(float dt)
{
    // only do calcs if we have a beam to slide on
    if (!m_sliding_beam || m_sliding_beam->bm_broken || sn_slide_broken)
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

    m_sliding_node->Forces += -perpForces;
    m_sliding_beam->p1->Forces += perpForces * (1 - m_node_forces_ratio);
    m_sliding_beam->p2->Forces += perpForces * m_node_forces_ratio;
}

RailSegment* RailGroup::FindClosestSegment(const Ogre::Vector3& point)
{
    float closest_dist_sq = SlideNode::getLenTo(&this->rg_segments[0], point);
    size_t closest_seg = 0;

    for (size_t i = 1; i < this->rg_segments.size(); ++i)
    {
        const float dist_sq = SlideNode::getLenTo(&this->rg_segments[i], point);
        if (dist_sq < closest_dist_sq)
        {
            closest_dist_sq = dist_sq;
            closest_seg = i;
        }
    }

    return &this->rg_segments[closest_seg];
}

RailSegment* RailSegment::CheckCurSlideSegment(const Ogre::Vector3& point)
{
    float closest_dist_sq = SlideNode::getLenTo(this, point);
    RailSegment* closest_seg = this;

    if (this->rs_prev != nullptr)
    {
        const float dist_sq = SlideNode::getLenTo(this->rs_prev, point);
        if (dist_sq < closest_dist_sq)
        {
            closest_seg = this->rs_prev;
            closest_dist_sq = dist_sq;
        }
    }

    if (this->rs_next != nullptr)
    {
        const float dist_sq = SlideNode::getLenTo(this->rs_next, point);
        if (dist_sq < closest_dist_sq)
        {
            closest_seg = this->rs_next;
        }
    }

    return closest_seg;
}

int SlideNode::GetSlideNodeId()
{
    return m_sliding_node->pos;
}

void SlideNode::UpdatePosition()
{
    // only do calcs if we have a beam to slide on
    if (!m_sliding_beam || m_sliding_beam->bm_broken)
    {
        m_ideal_position = m_sliding_node->AbsPosition;
        return;
    }

    // find which beam to use
    m_cur_rail_seg = m_cur_rail_seg->CheckCurSlideSegment(m_sliding_node->AbsPosition);
    m_sliding_beam = m_cur_rail_seg->rs_beam;

    // Get vector for beam
    Ogre::Vector3 b = m_sliding_beam->p2->AbsPosition - m_sliding_beam->p1->AbsPosition;

    // pre-compute normal
    const Ogre::Real bLen = b.normalise();

    // Get dot product along the b beam
    const Ogre::Real aDotBUnit = (m_sliding_node->AbsPosition - m_sliding_beam->p1->AbsPosition).dotProduct(b);

    // constrain Value between the two end points
    const Ogre::Real len = std::max(0.0f, std::min(aDotBUnit, bLen));
    m_ideal_position = m_sliding_beam->p1->AbsPosition + b * len;

    // calculate(cache) the ratio between the the two end points,
    // if bLen = 0.0f it means the beam is zero length so pick an end point
    m_node_forces_ratio = (bLen > 0.0f) ? len / bLen : 0.0f;
}

const Ogre::Vector3& SlideNode::GetSlideNodePosition() const
{
    return m_sliding_node->AbsPosition;
}

void SlideNode::ResetPositions()
{
    if (m_cur_railgroup != nullptr)
    {
        m_cur_rail_seg = m_cur_railgroup->FindClosestSegment(m_sliding_node->AbsPosition);
        m_sliding_beam = (m_cur_rail_seg ? m_cur_rail_seg->rs_beam : nullptr);
        this->UpdatePosition();
    }
}

Ogre::Real SlideNode::getLenTo(const RailGroup* group, const Ogre::Vector3& point)
{
    if (!group)
        return std::numeric_limits<Ogre::Real>::infinity();

    return getLenTo(&group->rg_segments[0], point);
}

Ogre::Real SlideNode::getLenTo(const RailSegment* rail, const Ogre::Vector3& point)
{
    if (!rail)
        return std::numeric_limits<Ogre::Real>::infinity();

    return getLenTo(rail->rs_beam, point);
}

Ogre::Real SlideNode::getLenTo(const beam_t* beam, const Ogre::Vector3& point)
{
    if (!beam)
        return std::numeric_limits<Ogre::Real>::infinity();

    return (NearestPointOnLine(beam->p1->AbsPosition, beam->p2->AbsPosition, point) - point).length();
}

Ogre::Real SlideNode::getLenTo(const RailGroup* group) const
{
    return getLenTo(group, m_sliding_node->AbsPosition);
}

Ogre::Real SlideNode::getLenTo(const RailSegment* rail) const
{
    return getLenTo(rail, m_sliding_node->AbsPosition);
}

Ogre::Real SlideNode::getLenTo(const beam_t* beam) const
{
    return getLenTo(beam, m_sliding_node->AbsPosition);
}

Ogre::Vector3 SlideNode::CalcCorrectiveForces()
{
    Ogre::Vector3 force = (m_ideal_position - m_sliding_node->AbsPosition);
    const Ogre::Real beamLen = std::max(0.0f, force.normalise() - m_cur_threshold);
    const Ogre::Real forceLen = -m_spring_rate * beamLen;
    return (force * forceLen);
}
