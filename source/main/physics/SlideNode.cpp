/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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
/**
 @file SlideNode.cpp
 @author Christopher Ritchey
 @date 10/30/2009

This source file is part of Rigs of Rods
Copyright 2009 Christopher Ritchey

For more information, see http://www.rigsofrods.org/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "SlideNode.h"

#include "BeamData.h"

// RAIL GROUP IMPLEMENTATION ///////////////////////////////////////////////////

RailGroup::RailGroup(Rail* start):
    m_start_rail(start)
{
    static unsigned int id_counter = 7000000;
    m_id = id_counter;
    ++id_counter;
}

RailGroup::RailGroup(Rail* start, unsigned int id): m_start_rail(start), m_id(id)
{}

void RailGroup::CleanUpRailGroup()
{
	Rail* cur = m_start_rail;
	if ( cur->snr_prev )
    {
        cur->snr_prev = cur->snr_prev->snr_next = nullptr;
    }

	while( cur->snr_next )
	{
		cur = cur->snr_next;
		delete cur->snr_prev;
		cur->snr_prev = nullptr;
	}
		
	delete cur;
}

// RAIL IMPLEMENTATION /////////////////////////////////////////////////////////
Rail::Rail() : snr_prev(nullptr), snr_beam(nullptr), snr_next(nullptr)
{}

Rail::Rail(beam_t* beam) : snr_prev(nullptr), snr_beam(beam), snr_next(nullptr)
{}

Rail::Rail(beam_t* beam, Rail* prev, Rail* next) : snr_prev(prev), snr_beam(beam), snr_next(next)
{}

Rail::Rail(Rail& o):
    snr_prev(o.snr_prev), snr_beam(o.snr_beam), snr_next(o.snr_next)
{
    o.snr_beam = nullptr;
    o.snr_prev = nullptr;
    o.snr_next = nullptr;
}

// RAIL BUILDER IMPLEMENTATION /////////////////////////////////////////////////

// pass a rail value, this class does not manage memory
RailBuilder::RailBuilder() :
    mStart(NULL),
    mFront(mStart),
    mBack(mStart),
    mLoop(false),
    mRetreived(false)
{
    /* do nothing */
}

RailBuilder::RailBuilder(Rail* start) :
    mStart(start),
    mFront(mStart),
    mBack(mStart),
    mLoop(false),
    mRetreived(false)
{
    /* do nothing */
}

RailBuilder::~RailBuilder()
{
    if (!mRetreived && mStart)
    {
        Rail* cur = mStart;
        if (cur->snr_prev)
            cur->snr_prev = cur->snr_prev->snr_next = NULL;
        while (cur->snr_next)
        {
            cur = cur->snr_next;
            delete cur->snr_prev;
            cur->snr_prev = NULL;
        }

        delete cur;
        cur = NULL;
    }

    mStart = NULL;
    mFront = NULL;
    mBack = NULL;
}

void RailBuilder::pushBack(beam_t* next)
{
    if (!mStart)
    {
        mStart = new Rail(next);
        mBack = mStart;
        mFront = mStart;
    }
    else if (!mStart->snr_beam)
    {
        mStart->snr_beam = next;
    }
    else
    {
        mBack->snr_next = new Rail(next, mBack, NULL);
        mBack = mBack->snr_next;
    }
}

void RailBuilder::pushFront(beam_t* prev)
{
    if (!mStart)
    {
        mStart = new Rail(prev);
        mBack = mStart;
        mFront = mStart;
    }
    else if (!mStart->snr_beam)
    {
        mStart->snr_beam = prev;
    }
    else
        mFront->snr_prev = new Rail(prev, NULL, mFront);
    mFront = mFront->snr_prev;
}

//! wrapper method
void RailBuilder::loopRail(bool doLoop)
{
    (doLoop) ? loopRail() : unLoopRail();
}

void RailBuilder::loopRail()
{
    mLoop = true;
}

void RailBuilder::unLoopRail()
{
    mLoop = false;
}

Rail* RailBuilder::getCompletedRail()
{
    if (mLoop)
    {
        mFront->snr_prev = mBack;
        mBack->snr_next = mFront;
    }

    mRetreived = true;
    return mStart;
}

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

Rail* SlideNode::getClosestRailAll(RailGroup* railGroup, const Ogre::Vector3& point)
{
    if (!railGroup)
        return NULL;

    Rail* closestRail = (Rail*) railGroup->GetStartRail();
    Rail* curRail = (Rail*) railGroup->GetStartRail()->snr_next;

    Ogre::Real lenToClosest = getLenTo(closestRail, point);
    Ogre::Real lenToCurrent = std::numeric_limits<Ogre::Real>::infinity();

    while (curRail &&
        // loop detection, if both id's are the same on the start and the current
        // then it has detected a loop. Hmm however this won't work if we loop
        // back on the start but don't loop.
        ((curRail->snr_beam->p1->id != railGroup->GetStartRail()->snr_beam->p1->id) ||
            (curRail->snr_beam->p2->id != railGroup->GetStartRail()->snr_beam->p2->id)))
    {
        lenToCurrent = getLenTo(curRail, point);

        if (lenToCurrent < lenToClosest)
        {
            closestRail = curRail;
            lenToClosest = getLenTo(closestRail, point);
        }
        curRail = curRail->snr_next;
    };

    return closestRail;
}

Rail* SlideNode::getClosestRail(const Rail* rail, const Ogre::Vector3& point)
{
    Rail* curRail = (Rail*) rail;

    Ogre::Real lenToCurrent = getLenTo(curRail, point);
    Ogre::Real lenToPrev = getLenTo(curRail->snr_prev, point);
    Ogre::Real lenToNext = getLenTo(curRail->snr_next, point);

    if (lenToCurrent > lenToPrev || lenToCurrent > lenToNext)
    {
        if (lenToPrev < lenToNext)
            curRail = curRail->snr_prev;
        else
            curRail = curRail->snr_next;
    }

    return curRail;
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
    mSlidingRail = getClosestRail(mSlidingRail, mSlidingNode->AbsPosition);
    mSlidingBeam = mSlidingRail->snr_beam;

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
    mSlidingRail = getClosestRailAll(mCurRailGroup, mSlidingNode->AbsPosition);
    mSlidingBeam = (mSlidingRail ? mSlidingRail->snr_beam : NULL);
    UpdatePosition();
}

Ogre::Real SlideNode::getLenTo(const RailGroup* group, const Ogre::Vector3& point)
{
    if (!group)
        return std::numeric_limits<Ogre::Real>::infinity();

    return getLenTo(group->GetStartRail(), point);
}

Ogre::Real SlideNode::getLenTo(const Rail* rail, const Ogre::Vector3& point)
{
    if (!rail)
        return std::numeric_limits<Ogre::Real>::infinity();

    return getLenTo(rail->snr_beam, point);
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

Ogre::Real SlideNode::getLenTo(const Rail* rail) const
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
