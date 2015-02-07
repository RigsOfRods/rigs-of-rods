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

#include "BeamData.h"

#include "physics/framework/NodeComponent.h"

// RAIL GROUP IMPLEMENTATION ///////////////////////////////////////////////////
unsigned int RailGroup::nextId = 7000000;

// RAIL IMPLEMENTATION /////////////////////////////////////////////////////////
Rail::Rail() : prev(nullptr), curBeam( nullptr ), next(nullptr) {}
Rail::Rail( beam_t* newBeam ) : prev(nullptr), curBeam( newBeam ), next(nullptr) {}
Rail::Rail( beam_t* newBeam, Rail* newPrev, Rail* newNext ) :
	prev(newPrev), curBeam( newBeam ), next(newNext) {}

Rail::Rail( Rail& o) : prev(o.prev), curBeam(o.curBeam), next(o.next)
{
	o.prev = nullptr;
	o.curBeam = nullptr;
	o.next = nullptr;
}

// RAIL BUILDER IMPLEMENTATION /////////////////////////////////////////////////

// pass a rail value, this class does not manage memory
RailBuilder::RailBuilder() :
	mStart(nullptr),
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
		if ( cur->prev ) cur->prev = cur->prev->next = nullptr;
		while( cur->next )
		{
			cur = cur->next;
			delete cur->prev;
			cur->prev = nullptr;
		}
		
		delete cur;
		cur = nullptr;
	}
	
	mStart = nullptr;
	mFront = nullptr;
	mBack = nullptr;
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
		mBack->next = new Rail(next, mBack, nullptr);
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
	mFront->prev = new Rail(prev, nullptr, mFront);
	mFront = mFront->prev;	
}

//! wrapper method
void RailBuilder::loopRail(bool doLoop)
{
	mLoop = doLoop;
}

void RailBuilder::loopRail()
{
	loopRail(true);	
}

void RailBuilder::unLoopRail()
{
	loopRail(false);
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
SlideNode::SlideNode(node_t* slidingNode, RailGroup* slidingRail)
: _slidingNode(slidingNode)
, _slidingBeam( nullptr )
, _coupler(_slidingNode, _slidingBeam)

, mOrgRailGroup(slidingRail)
, mCurRailGroup(mOrgRailGroup)
, mSlidingRail( nullptr )

, mAttachDist( 0.1f )
, mBoolSettings( ATTACH_NONE )
{
}

SlideNode::~SlideNode()
{
	mOrgRailGroup = nullptr;
	mCurRailGroup = nullptr;
	 mSlidingRail = nullptr;
}

void SlideNode::updateForce(float dt)
{
	UpdatePosition();
	
	// only do calcs if we have a beam to slide on
	if ( _slidingBeam.isBroken() || _coupler.isBroken() )
	{
		return;
	}
	_coupler.updateForce(dt);
}


Rail* SlideNode::getClosestRailAll(RailGroup* railGroup, const Ogre::Vector3& point )
{
	if ( !railGroup ) return nullptr;

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
	if ( _slidingBeam.isBroken() )
	{
    	_coupler.setBroken(true);
		return;
	}
		
	// find which beam to use
	mSlidingRail = getClosestRail( mSlidingRail, _slidingNode.getPosition() );
	_slidingBeam.setBeam(mSlidingRail->curBeam);
	
	
	// Get vector for beam
	Ogre::Vector3 b = _slidingBeam.getNode2().getPosition();
	b -= _slidingBeam.getNode1().getPosition();
	
	// pre-compute normal
	const Ogre::Real bLen = b.length();
	
	// normalize b instead of using another variable
	b.normalise();
	
	// Get dot product along the b beam
	const Ogre::Real aDotBUnit = (_slidingNode.getPosition() - _slidingBeam.getNode1().getPosition()).dotProduct( b );
	
	// constrain Value between the two end points
	const Ogre::Real len = std::max( 0.0f, std::min( aDotBUnit, bLen) );
	
	// if bLen = 0.0f it means the beam is zero length so pick an end point
	_slidingBeam.setRatio((bLen > 0.0f) ? len/bLen : 0.0f);
}

const Ogre::Vector3& SlideNode::getNodePosition() const  
{ 
	return _slidingNode.getPosition();
}

unsigned int SlideNode::getNodeID() const  
{ 
	return _slidingNode.getId();
}

void SlideNode::ResetPositions()
{
	mSlidingRail = getClosestRailAll(mCurRailGroup, _slidingNode.getPosition());
	_slidingBeam.setBeam(mSlidingRail ? mSlidingRail->curBeam : nullptr );
    UpdatePosition();
}

Ogre::Real SlideNode::getLenTo( const RailGroup* group, const Ogre::Vector3& point )
{
    if ( !group ) return std::numeric_limits<Ogre::Real>::infinity();
    	
    return getLenTo( group->getStartRail(), point);	
}

Ogre::Real SlideNode::getLenTo( const Rail* rail, const Ogre::Vector3& point )
{
    if ( !rail ) return std::numeric_limits<Ogre::Real>::infinity();
    	
    return getLenTo( rail->curBeam, point);	
}

Ogre::Real SlideNode::getLenTo( const beam_t* beam, const Ogre::Vector3& point )
{
    if ( !beam ) return std::numeric_limits<Ogre::Real>::infinity();
    	
    return fast_length( nearestPointOnLine(beam->p1->AbsPosition, beam->p2->AbsPosition, point)- point );	
}

Ogre::Real SlideNode::getLenTo( const BeamComponent& beam, const Ogre::Vector3& point )
{
	if ( beam.getBeam() ) return std::numeric_limits<Ogre::Real>::infinity();

	return fast_length( nearestPointOnLine(beam.getNode1().getPosition(), beam.getNode2().getPosition(), point)- point );
}

Ogre::Real SlideNode::getLenTo( const RailGroup* group ) const
{
    return getLenTo( group, _slidingNode.getPosition() );
}

Ogre::Real SlideNode::getLenTo( const Rail* rail ) const
{
    return getLenTo( rail, _slidingNode.getPosition() );
}

Ogre::Real SlideNode::getLenTo( const beam_t* beam) const
{
    return getLenTo( beam,  _slidingNode.getPosition() );
}

Ogre::Real SlideNode::getLenTo( const BeamComponent& beam) const
{
	return getLenTo( beam,  _slidingNode.getPosition() );
 }

