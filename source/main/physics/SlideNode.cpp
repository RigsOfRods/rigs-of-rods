/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

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
/**
 @file SlideNode.cpp
 @author Christopher Ritchey
 @date 10/30/2009

This source file is part of Rigs of Rods
Copyright 2009 Christopher Ritchey

For more information, see http://www.rigsofrods.com/

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

// RAIL GROUP IMPLEMENTATION ///////////////////////////////////////////////////
unsigned int RailGroup::nextId = 7000000;

// RAIL IMPLEMENTATION /////////////////////////////////////////////////////////
Rail::Rail() : prev(NULL), curBeam( NULL ), next(NULL) {}
Rail::Rail( beam_t* newBeam ) : prev(NULL), curBeam( newBeam ), next(NULL) {}
Rail::Rail( beam_t* newBeam, Rail* newPrev, Rail* newNext ) :
	prev(newPrev), curBeam( newBeam ), next(newNext) {}

Rail::Rail( Rail& o) : prev(o.prev), curBeam(o.curBeam), next(o.next)
{
	o.prev = NULL;
	o.curBeam = NULL;
	o.next = NULL;
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
	if ( !mRetreived && mStart )
	{
		Rail* cur = mStart;
		if ( cur->prev ) cur->prev = cur->prev->next = NULL;
		while( cur->next )
		{
			cur = cur->next;
			delete cur->prev;
			cur->prev = NULL;
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
	if ( !mStart )
	{
		mStart = new Rail( next );
		mBack = mStart;
		mFront = mStart;
	}
	else if ( !mStart->curBeam )
	{
		mStart->curBeam = next;
	}
	else
	{
		mBack->next = new Rail(next, mBack, NULL);
		mBack = mBack->next;
	}
	
}
void RailBuilder::pushFront(beam_t* prev)
{
	if ( !mStart )
	{
		mStart = new Rail( prev );
		mBack = mStart;
		mFront = mStart;
	}
	else if ( !mStart->curBeam )
	{
		mStart->curBeam = prev;
	}
	else
	mFront->prev = new Rail(prev, NULL, mFront);
	mFront = mFront->prev;	
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
	if ( mLoop )
	{
		mFront->prev = mBack;
		mBack->next =  mFront;
	}

	mRetreived = true;
	return mStart;
}

// SLIDE NODES IMPLEMENTATION //////////////////////////////////////////////////
SlideNode::SlideNode(node_t* slidingNode, RailGroup* slidingRail):

	mSlidingNode(slidingNode),
	mSlidingBeam( NULL ),
	mOrgRailGroup(slidingRail),
	mCurRailGroup(mOrgRailGroup),
	mSlidingRail( NULL ),
	mRatio(0.0f),

	mInitThreshold( 0.0f ),
	mCurThreshold( 0.0f ),
	mSpringRate( 9000000 ),
	mBreakForce( std::numeric_limits<Ogre::Real>::infinity() ), // beam won't break

	mAttachRate( 1.0f ),
	mAttachDist( 0.1f ),
	mBoolSettings( ATTACH_NONE )
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
	if ( !mSlidingBeam || mSlidingBeam->broken || getFlag( MASK_SLIDE_BROKEN ) )
	{
		return;
	}
	
	// the threshold changes when attaching a node to a beam, if it's more than
	// the initial amount then then we're still attaching.
	if ( mCurThreshold > mInitThreshold )
	{
		mCurThreshold -= (mAttachRate * dt);
	}
	
	Ogre::Vector3 perpForces = getCorrectiveForces();
	// perpendicular Forces are distributed according to the position along the Beam
	if ( perpForces.length() > mBreakForce ) setFlag( MASK_SLIDE_BROKEN );
	mSlidingNode->Forces += -perpForces;
	mSlidingBeam->p1->Forces += perpForces * (1 - mRatio);
	mSlidingBeam->p2->Forces += perpForces * mRatio;
}


Rail* SlideNode::getClosestRailAll(RailGroup* railGroup, const Ogre::Vector3& point )
{
	if ( !railGroup ) return NULL;

	Rail* closestRail = (Rail*) railGroup->getStartRail();
	Rail* curRail = (Rail*) railGroup->getStartRail()->next;

	Ogre::Real lenToClosest = getLenTo(closestRail, point);
	Ogre::Real lenToCurrent = std::numeric_limits<Ogre::Real>::infinity();
	
	while( curRail &&
	// loop detection, if both id's are the same on the start and the current
	// then it has detected a loop. Hmm however this won't work if we loop
	// back on the start but don't loop.
	( ( curRail->curBeam->p1->id != railGroup->getStartRail()->curBeam->p1->id ) ||
	( curRail->curBeam->p2->id != railGroup->getStartRail()->curBeam->p2->id ) ) )
	{
		lenToCurrent = getLenTo(curRail, point);
		
		if ( lenToCurrent < lenToClosest)
		{
			closestRail = curRail;
			lenToClosest = getLenTo(closestRail, point);
		}
		curRail = curRail->next;
		
	};
	
	return closestRail;
}

Rail* SlideNode::getClosestRail(const Rail* rail, const Ogre::Vector3& point )
{	
	Rail* curRail = (Rail*) rail;

	Ogre::Real lenToCurrent = getLenTo(curRail, point);
	Ogre::Real lenToPrev = getLenTo(curRail->prev, point);
	Ogre::Real lenToNext = getLenTo(curRail->next, point);
	
	
	if ( lenToCurrent > lenToPrev || lenToCurrent > lenToNext )
	{
		if ( lenToPrev < lenToNext) curRail = curRail->prev;
		else curRail = curRail->next;
	}
	
	return curRail;
}

void SlideNode::UpdatePosition()
{
	// only do calcs if we have a beam to slide on
	if ( !mSlidingBeam || mSlidingBeam->broken )
	{
		mIdealPosition = mSlidingNode->AbsPosition;
		return;
	}
		
	// find which beam to use
	mSlidingRail = getClosestRail( mSlidingRail, mSlidingNode->AbsPosition );
	mSlidingBeam = mSlidingRail->curBeam;
	
	// Get vector for beam
	Ogre::Vector3 b = mSlidingBeam->p2->AbsPosition;
	b -= mSlidingBeam->p1->AbsPosition;
	
	// pre-compute normal
	const Ogre::Real bLen = b.length();
	
	// normalize b instead of using another variable
	b.normalise();
	
	// Get dot product along the b beam
	const Ogre::Real aDotBUnit = (mSlidingNode->AbsPosition - mSlidingBeam->p1->AbsPosition).dotProduct( b );
	
	// constrain Value between the two end points
	const Ogre::Real len = std::max( 0.0f, std::min( aDotBUnit, bLen) );
	mIdealPosition  = b;
	mIdealPosition *= len;
	mIdealPosition += mSlidingBeam->p1->AbsPosition;
	
	// calculate(cache) the ratio between the the two end points,
	// if bLen = 0.0f it means the beam is zero length so pick an end point
	mRatio = (bLen > 0.0f) ? len/bLen : 0.0f;
}
