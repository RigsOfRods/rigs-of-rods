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
 @file SlideNode.h
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

#ifndef __SLIDENODE_H_
#define __SLIDENODE_H_

#include "RoRPrerequisites.h"

#include "ApproxMath.h"
#include "BeamData.h"

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
 *
 */
//! @{
static inline Ogre::Vector3 nearestPoint(const Ogre::Vector3& pt1,
		const Ogre::Vector3& pt2,
		const Ogre::Vector3& tp)
{	
	const Ogre::Vector3 a = tp - pt1;
	const Ogre::Vector3 b = fast_normalise(pt2-pt1);

	return pt1 + (a.dotProduct(b)) * b;
}

static inline Ogre::Vector3 nearestPointOnLine(const Ogre::Vector3& pt1,
		const Ogre::Vector3& pt2,
		const Ogre::Vector3& tp)
{	
	Ogre::Vector3 a = tp - pt1;
	Ogre::Vector3 b = pt2 - pt1;
	Ogre::Real len = fast_length(b);

	b = fast_normalise(b);
	len = std::max(0.0f, std::min(a.dotProduct(b), len));	
	b *= len;
	
	a = pt1;
	a += b;
	return a;
}
//! @}

/******************************************************************************\
|                                                                              |
|   CLASSES IN THIS FILE STORED IN BEAM NEED TO HAVE A DECLARATION IN beam.h   |
|                                                                              |
\******************************************************************************/

//! can only be attached to rail specified at start
#define ATTACH_NONE (0 << 0)
//! attach/detach to rails on the current vehicle only
#define ATTACH_SELF (1 << 1)
//! attach/detach to rails only on other vehicles
#define ATTACH_FOREIGN  (1 << 2)
//!attach/detach to any rail, on the current vehicle or others
#define ATTACH_ALL ( ATTACH_SELF | ATTACH_FOREIGN )

#define MASK_ATTACH_RULES ATTACH_ALL
#define MASK_SLIDE_BROKEN (1 << 3)

/**
 *
 */
class Rail : public ZeroedMemoryAllocator
{
// Members /////////////////////////////////////////////////////////////////////
private:
	/* no private members */
public:
	Rail* prev;
	beam_t* curBeam;
	Rail* next;
	
// Methods /////////////////////////////////////////////////////////////////////
private:
	/* no private methods */
public:
	Rail();
	Rail( beam_t* newBeam );
	Rail( beam_t* newBeam, Rail* newPrev, Rail* newNext );
	Rail( Rail& other );
};

/**
 *
 */
class RailGroup : public ZeroedMemoryAllocator
{
// Members /////////////////////////////////////////////////////////////////////
public:
	/* no public members */
private:
	Rail*     mStart; //!< Beginning Rail in the Group
		
	//! Identification, for anonymous rails (defined inline) the ID will be
	//! > 7000000, for Rail groups defined int he rail group section the Id
	//! needs to be less than 7000000
	unsigned int mId;
	
	static unsigned int nextId;

// Methods /////////////////////////////////////////////////////////////////////
public:
	RailGroup(Rail* start): mStart(start), mId(nextId) { MYASSERT(mStart); nextId++; }
	RailGroup(Rail* start, unsigned int id): mStart(start), mId(id) { MYASSERT(mStart); }

	const Rail* getStartRail() const { return mStart; }
	unsigned int getID() const { return mId; }
	
	/** clears up all the allocated memory this is intentionally not made into a
	 * destructor to avoid issues like copying Rails when storing in a container
	 *  it is assumed that if next is not null then next->prev is not null either
	 */
	void cleanUp()
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
	
private:
	/* no private members */
};

/**
 * Rail builder is a utility used for created linked lists effectively and
 * painlessly.
 */
class RailBuilder : public ZeroedMemoryAllocator
{
// Members /////////////////////////////////////////////////////////////////////
public:
	/* no public members */
private:
	Rail*    mStart;    //! Start of the Rail series
	Rail*    mFront;    //! Front of the Rail, not necessarily the start
	Rail*     mBack;    //! Last rail in the series
	bool      mLoop;    //! Check if rail is to be looped
	bool mRetreived;    //! Check if RailBuilder needs to deallocate Rails
	
// Methods /////////////////////////////////////////////////////////////////////
public:
	// pass a rail value, this class does not manage memory
	RailBuilder();
	RailBuilder(Rail* start);
	~RailBuilder();
		
	/**
	 *
	 * @param next beam attaches to the last beam in the end of the series
	 */
	void pushBack(beam_t* next);
	/**
	 *
	 * @param prev adds another beam to the front of the rail series
	 */
	void pushFront(beam_t* prev);
	
	//! wrapper method
	void loopRail(bool doLoop);	
	void loopRail();
	void unLoopRail();
	
	/**
	 *
	 * @return the completed rail, if called before instance goes out of reference
	 * the internal memory is null referenced, if not the memorey is reclaimed
	 */
	Rail* getCompletedRail();
	
private:
	/* no private methods */
	
};

/**
 *
 */
class SlideNode : public ZeroedMemoryAllocator
{

// Members /////////////////////////////////////////////////////////////////////
public:
	/* no public  members */
private:
    node_t*     mSlidingNode; //!< pointer to node that is sliding
    beam_t*     mSlidingBeam; //!< pointer to current beam sliding on
    RailGroup* mOrgRailGroup; //!< initial Rail group on spawn
    RailGroup* mCurRailGroup; //!< current Rail group, used for attachments
    Rail*       mSlidingRail; //!< current rail we are sliding on

    //! ratio of length along the slide beam where the virtual node is
    //! 0.0f = p1, 1.0f = p2
    Ogre::Real mRatio;

    Ogre::Vector3 mIdealPosition; //!< Where the node SHOULD be. (m)

    Ogre::Real mInitThreshold; //!< distance from beam calculating corrective forces (m)
    Ogre::Real  mCurThreshold; //!< currenth threshold, used for attaching a beam (m)
    Ogre::Real    mSpringRate; //!< Spring rate holding node to rail (N/m)
    Ogre::Real    mBreakForce; //!< Force at which Slide Node breaks from rail (N)

    Ogre::Real    mAttachRate; //!< how fast the cur threshold changes (m/s)
    Ogre::Real    mAttachDist; //!< maximum distance slide node will attach to a beam (m)

        unsigned int mBoolSettings; //!< bit Array for storing beam settings

// Methods /////////////////////////////////////////////////////////////////////
public:
        /**
         *
         * @param slidingNode pointer to the node acting as a slide node
         * @param slidingRail pointer to the rail group the node is initially
         * sliding on. NULL is an acceptable value.
         * @return New SlideNode instance
         */
    SlideNode(node_t* slidingNode, RailGroup* slidingRail);
    virtual ~SlideNode();

    /**
     * Updates the corrective forces and applies these forces to the beam
     * @param dt size of the current time
     */
    void UpdateForces(float dt);

    /**
     * updates the positional information, such as where the ideal location
     * of the slide node is supposed to be. It also updated the positional cache
     * that stores information about the beam being slid upon
     */
    void UpdatePosition();

    /**
     * @return The current position of the SlideNode
     */
    const Ogre::Vector3& getNodePosition() const  { return mSlidingNode->AbsPosition; }

    /**
     * @return The position where the SlideNode should be location on the slide beam
     */
    const Ogre::Vector3& getIdealPosition() const { return mIdealPosition; }

    /**
     * @return Id of the SlideNode
     */
    unsigned int getNodeID() const  { return mSlidingNode->id; }

    /**
     * @return Id of the rail this SlideNode is on, returns infinity if no rail
     * is defiined
     */
    unsigned int getRailID() const
    {
    	return mCurRailGroup ? mCurRailGroup->getID() : std::numeric_limits<unsigned int>::infinity();
    }

    /**
     * resets the slide Rail back to the initial rail, restores all broken rail
     * connections and finds the position along the rail closest to the SlideNode
     */
    void reset()
    {
    	mCurRailGroup = mOrgRailGroup;
    	resetFlag( MASK_SLIDE_BROKEN );
    	ResetPositions();
    }

    /**
     * Recalculates the closest position for this SlideNode
     */
    void ResetPositions()
    {
    	mSlidingRail = getClosestRailAll(mCurRailGroup, mSlidingNode->AbsPosition);
    	mSlidingBeam = (mSlidingRail ? mSlidingRail->curBeam : NULL );
    	UpdatePosition();
    }

    /**
     * @param toAttach Which Rail this SLideNode starts sliding on when reset
     * or spawning
     */
    void setDefaultRail(RailGroup* toAttach)
    {
    	mOrgRailGroup = mCurRailGroup = toAttach;
		ResetPositions();
    }

    /**
     * @param toAttach Which rail to attach to, Pass NULL to detach this
     * SlideNode from any rail.
     */
    void attachToRail(RailGroup* toAttach)
    {
		mCurRailGroup = toAttach;
		ResetPositions();
		mCurThreshold = (mSlidingBeam ? getLenTo(mSlidingBeam) : mInitThreshold);
    }



    //! distance from a beam before corrective forces take effect
	void setThreshold( Ogre::Real threshold ) { mInitThreshold = mCurThreshold = fabs( threshold ); }
	//! spring force used to calculate corrective forces
	void setSpringRate( Ogre::Real rate ){ mSpringRate = fabs( rate); }
	//! Force required to break the Node from the Rail
	void setBreakForce( Ogre::Real breakRate ) { mBreakForce = fabs( breakRate); }
	//! how long it will take for springs to fully attach to the Rail
	void setAttachmentRate( Ogre::Real rate ) { mAttachRate = fabs( rate); }
	//! maximum distance this spring node is allowed to reach out for a Rail
	void setAttachmentDistance( Ogre::Real dist ){ mAttachDist = fabs( dist); }
	
	//! Distance away from beam before corrective forces begin to act on the node
	Ogre::Real getThreshold () const { return mCurThreshold; }
	//! spring force used to calculate corrective forces
	Ogre::Real getSpringRate() const { return mSpringRate; }
	//! Force required to break the Node from the Rail
	Ogre::Real getBreakForce() const { return mBreakForce; }
	//! how long it will take for springs to fully attach to the Rail
	Ogre::Real getAttachmentRate() const { return mAttachRate; }
	//! maximum distance this spring node is allowed to reach out for a Rail
	Ogre::Real getAttachmentDistance() const { return mAttachDist; }


	/**
	 * Allows slide node to attach to a rail it was not assigned to.
	 * @param attachMask bit field for attachment rules
	 */
	void setAttachRule( unsigned int attachFlag ) { setFlag(attachFlag); }
	
	/**
	 * @param attachFlag
	 * @return
	 */
	bool getAttachRule( unsigned int attachFlag) { return getFlag(attachFlag); }
	

	/**
	 * @param group
	 * @param point
	 * @return value is always positive, if group is null return infinity.
	 */
    static Ogre::Real getLenTo( const RailGroup* group, const Ogre::Vector3& point )
    {
    	if ( !group ) return std::numeric_limits<Ogre::Real>::infinity();
    	
    	return getLenTo( group->getStartRail(), point);	
    }

    /**
     * @param rail
     * @param point
     * @return value is always positive, if rail is null return infinity.
     */
    static Ogre::Real getLenTo( const Rail* rail, const Ogre::Vector3& point )
    {
    	if ( !rail ) return std::numeric_limits<Ogre::Real>::infinity();
    	
    	return getLenTo( rail->curBeam, point);	
    }

    /**
     * @param beam
     * @param point
     * @return value is always positive, if beam is null return infinity
     */
    static Ogre::Real getLenTo( const beam_t* beam, const Ogre::Vector3& point )
    {
    	if ( !beam ) return std::numeric_limits<Ogre::Real>::infinity();
    	
    	return fast_length( nearestPointOnLine(beam->p1->AbsPosition, beam->p2->AbsPosition, point)- point );	
    }


    /**
     * @param group
     * @return value is always positive, if group is null return infinity.
     */
    Ogre::Real getLenTo( const RailGroup* group ) const
    {
    	return getLenTo( group, mSlidingNode->AbsPosition );	
    }

    /**
     * @param rail
     * @return value is always positive, if rail is null return infinity.
     */
    Ogre::Real getLenTo( const Rail* rail ) const
    {
    	return getLenTo( rail, mSlidingNode->AbsPosition );	
    }

    /**
     *
     * @param beam
     * @return value is always positive, if beam is null return infinity
     */
    Ogre::Real getLenTo( const beam_t* beam) const
    {
    	return getLenTo( beam,  mSlidingNode->AbsPosition );
    }

    /**
     * Finds the closest rail to the point, non-incremental version.
     *
     * This method iterates through the entire Rail and returns the rail closest
     * to the provided point.
     *
     * This is O(n) complexity due to every rail being checked against, useful
     * when SlideNodes are moved outside of the integrator to avoid beam explosion.
     * @see getClosestRail for the incremental version.
     *
     * @bug if rail loops uses the start rail later in the sequence but does not
     * form a loop it will exit.
     *
     * @param rail rail to start from
     * @param point point from which to check distance to
     * @return which rail is closest to the slide Beam
     */
    static Rail* getClosestRailAll(RailGroup* railGroup, const Ogre::Vector3& point );

    /**
     * Find closest Rail to a point, incremental version.
     *
     * This method assumed that the current rail is already the closest rail and
     * checks distance of neighboring rails.
     *
     * Under normal operation this method is O(1). This could potentially loop
     * through the entire Rail series but it extremely unlikely. This method
     * keeps incrementing to the next or previous rail if it is closer than the
     * current rail. In order for the method to loop more than twice the node
     * would have to move beyond 2 rail segments in a single integrator
     * iteration, which is extremely unlikely.
     *
     * @see getClosestRailAll for the non-incremental version.
     *
     * @param rail rail to start from
     * @param point point from which to check distance to
     * @return which rail is closest to the slide Beam
     */
    static Rail* getClosestRail(const Rail* rail, const Ogre::Vector3& point );

private:
    /**
     * returns the forces used to keep the slide node in alignment with the
     * slide beam.
     * @return forces between the ideal position and the slide node
     */
    Ogre::Vector3 getCorrectiveForces()
    {
		const Ogre::Vector3 force = (mIdealPosition - mSlidingNode->AbsPosition);
		const Ogre::Real  beamLen = std::max( 0.0f, force.length() - mCurThreshold );
		const Ogre::Real forceLen = -mSpringRate * beamLen;
		return (force.normalisedCopy() * forceLen);
    }

    /**
     *  sets a specific bit or bits to 1
     * @param flag bit(s) to set
     */
    void setFlag( unsigned int flag ) { mBoolSettings |= flag; }

    /**
     * sets a specific bit or bits to 0
     * @param flag bit(s) to be reset
     */
    void resetFlag( unsigned int flag ) { mBoolSettings &= ~flag; }

    /**
     * Retrieves the state of specific bit or bits
     * @param flag bit(s) to check against
     * @return true if one of the bits is 1
     */
    bool getFlag( unsigned int flag ) { return (mBoolSettings & flag) != 0; }

};

#endif // __SLIDENODE_H_
