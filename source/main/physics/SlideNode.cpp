/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2009 Christopher Ritchey

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

#include "BeamData.h"

// SLIDE NODES IMPLEMENTATION //////////////////////////////////////////////////
SlideNode::SlideNode(node_t* slidingNode, RailGroup* slidingRail):

    mSlidingNode(slidingNode),
    mSlidingBeam(NULL),
    mOrgRailGroup(slidingRail),
    mCurRailGroup(mOrgRailGroup),
    mSlidingRail(NULL),
    mRatio(0.0f),

    mInitThreshold(0.0f),
    mCurThreshold(0.0f),
    mSpringRate(9000000),
    mBreakForce(std::numeric_limits<Ogre::Real>::infinity()), // beam won't break

    mAttachRate(1.0f),
    mAttachDist(0.1f),
    sn_attach_foreign(false),
    sn_attach_self(false),
    sn_slide_broken(false)
{
    // make sure they exist
    MYASSERT( mSlidingNode );
}

SlideNode::~SlideNode()
{
    mSlidingNode = NULL;
    mOrgRailGroup = NULL;
    mCurRailGroup = NULL;
    mSlidingBeam = NULL;
    mSlidingRail = NULL;
}

void SlideNode::UpdateForces(float dt)
{
    // only do calcs if we have a beam to slide on
    if (!mSlidingBeam || mSlidingBeam->bm_broken || sn_slide_broken)
    {
        return;
    }

    // the threshold changes when attaching a node to a beam, if it's more than
    // the initial amount then then we're still attaching.
    if (mCurThreshold > mInitThreshold)
    {
        mCurThreshold -= (mAttachRate * dt);
    }

    Ogre::Vector3 perpForces = getCorrectiveForces();
    // perpendicular Forces are distributed according to the position along the Beam
    if (perpForces.length() > mBreakForce)
    {
        sn_slide_broken = true;
    }

    mSlidingNode->Forces += -perpForces;
    mSlidingBeam->p1->Forces += perpForces * (1 - mRatio);
    mSlidingBeam->p2->Forces += perpForces * mRatio;
}

RailSegment* SlideNode::FindClosestRailSegment(RailGroup* rg, const Ogre::Vector3& point)
{
    if (!rg)
        return nullptr;

    float closest_dist_sq = getLenTo(&rg->rg_segments[0], point);
    size_t closest_seg = 0;

    for (size_t i = 1; i < rg->rg_segments.size(); ++i)
    {
        const float dist_sq = getLenTo(&rg->rg_segments[i], point);
        if (dist_sq < closest_dist_sq)
        {
            closest_dist_sq = dist_sq;
            closest_seg = i;
        }
    }

    return &rg->rg_segments[closest_seg];
}

RailSegment* SlideNode::FindClosestSurroundingSegment(RailSegment* seg, const Ogre::Vector3& point)
{
    float closest_dist_sq = getLenTo(seg, point);
    RailSegment* closest_seg = seg;

    if (seg->rs_prev != nullptr)
    {
        const float dist_sq = getLenTo(seg->rs_prev, point);
        if (dist_sq < closest_dist_sq)
        {
            closest_seg = seg->rs_prev;
            closest_dist_sq = dist_sq;
        }
    }

    if (seg->rs_next != nullptr)
    {
        const float dist_sq = getLenTo(seg->rs_next, point);
        if (dist_sq < closest_dist_sq)
        {
            closest_seg = seg->rs_next;
        }
    }

    return closest_seg;
}

void SlideNode::UpdatePosition()
{
    // only do calcs if we have a beam to slide on
    if (!mSlidingBeam || mSlidingBeam->bm_broken)
    {
        mIdealPosition = mSlidingNode->AbsPosition;
        return;
    }

    // find which beam to use
    mSlidingRail = FindClosestSurroundingSegment(mSlidingRail, mSlidingNode->AbsPosition);
    mSlidingBeam = mSlidingRail->rs_beam;

    // Get vector for beam
    Ogre::Vector3 b = mSlidingBeam->p2->AbsPosition;
    b -= mSlidingBeam->p1->AbsPosition;

    // pre-compute normal
    const Ogre::Real bLen = b.length();

    // normalize b instead of using another variable
    b.normalise();

    // Get dot product along the b beam
    const Ogre::Real aDotBUnit = (mSlidingNode->AbsPosition - mSlidingBeam->p1->AbsPosition).dotProduct(b);

    // constrain Value between the two end points
    const Ogre::Real len = std::max(0.0f, std::min(aDotBUnit, bLen));
    mIdealPosition = b;
    mIdealPosition *= len;
    mIdealPosition += mSlidingBeam->p1->AbsPosition;

    // calculate(cache) the ratio between the the two end points,
    // if bLen = 0.0f it means the beam is zero length so pick an end point
    mRatio = (bLen > 0.0f) ? len / bLen : 0.0f;
}

const Ogre::Vector3& SlideNode::getNodePosition() const
{
    return mSlidingNode->AbsPosition;
}

const Ogre::Vector3& SlideNode::getIdealPosition() const
{
    return mIdealPosition;
}

unsigned int SlideNode::getNodeID() const
{
    return mSlidingNode->id;
}

void SlideNode::ResetPositions()
{
    mSlidingRail = FindClosestRailSegment(mCurRailGroup, mSlidingNode->AbsPosition);
    mSlidingBeam = (mSlidingRail ? mSlidingRail->rs_beam : nullptr);
    UpdatePosition();
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

    return fast_length(nearestPointOnLine(beam->p1->AbsPosition, beam->p2->AbsPosition, point) - point);
}

Ogre::Real SlideNode::getLenTo(const RailGroup* group) const
{
    return getLenTo(group, mSlidingNode->AbsPosition);
}

Ogre::Real SlideNode::getLenTo(const RailSegment* rail) const
{
    return getLenTo(rail, mSlidingNode->AbsPosition);
}

Ogre::Real SlideNode::getLenTo(const beam_t* beam) const
{
    return getLenTo(beam, mSlidingNode->AbsPosition);
}

Ogre::Vector3 SlideNode::getCorrectiveForces()
{
    const Ogre::Vector3 force = (mIdealPosition - mSlidingNode->AbsPosition);
    const Ogre::Real beamLen = std::max(0.0f, force.length() - mCurThreshold);
    const Ogre::Real forceLen = -mSpringRate * beamLen;
    return (force.normalisedCopy() * forceLen);
}
