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

#include "ApproxMath.h"
#include "RoRPrerequisites.h"

#include <OgreVector3.h>

#include "physics/framework/BeamComponents.h"
#include "physics/framework/ThresholdCoupler.h"
#include "physics/framework/UpdateComponents.h"
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
	
	int Id() const { return curBeam->p1->id << 16 | ( curBeam->p2->id & 0x00FF); }
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
	 *  destructor to avoid issues like copying Rails when storing in a container
	 *  it is assumed that if next is not null then next->prev is not null either
	 */
	void cleanUp()
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
 * TODO refactor slide nodes to make use of integrate and update components
 * some parts needs to be removed that pertain to calculating the forces like a
 * spring.
 * 
 * This class will also need to be added to two lists, one to keep track of 
 * SlideNode instances, and another to handle integration
 *---
 *  what needs to be done is move all of the force calculations to an
 *  IntegrateComponent, and convert SlideNode to an UpdateComponent. 
 */
class SlideNode
: public Framework::Components::UpdateComponent<Units::Second>
, public ZeroedMemoryAllocator
{

// Members /////////////////////////////////////////////////////////////////////
public:
	/* no public  members */
private:
    NodeComponent _slidingNode; //!< node that is sliding
    BeamComponent _slidingBeam; //!< current beam sliding on
    
    LinearSIThresholdCoupler _coupler; //! beam used to calculate corrective forces
    
    RailGroup*   mOrgRailGroup; //!< initial Rail group on spawn
    RailGroup*   mCurRailGroup; //!< current Rail group, used for attachments
    Rail*         mSlidingRail; //!< current rail we are sliding on

    Ogre::Real    mAttachDist; //!< maximum distance slide node will attach to a beam (m)

    unsigned int mBoolSettings; //!< bit Array for storing beam settings
	
// Methods /////////////////////////////////////////////////////////////////////
public:
        /**
         *
         * @param slidingNode pointer to the node acting as a slide node
         * @param slidingRail pointer to the rail group the node is initially
         * sliding on. nullptr is an acceptable value.
         * @return New SlideNode instance
         */
    SlideNode(node_t* slidingNode, RailGroup* slidingRail);
    virtual ~SlideNode();

    /**
     *  I want to get rid of this off, load to the update components.
     *  I want slide node to only be aware of where the node should be, and only 
     *  update where the node should be. The rest should be offloaded to components
     *   
     * Updates the corrective forces and applies these forces to the beam
     * @param dt size of the current time
     */
    virtual void updateForce(float dt);

    /**
     * updates the positional information, such as where the ideal location
     * of the slide node is supposed to be. It also updated the positional cache
     * that stores information about the beam being slid upon
     */
    void UpdatePosition();

    // TODO handled by IntegrateComponent
    /**
     * @return The current position of the SlideNode
     */
    const Ogre::Vector3& getNodePosition() const;

    /**
     * @return Id of the SlideNode
     */
    unsigned int getNodeID() const;

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
    	_coupler.setBroken(false);
    	ResetPositions();
    }

    /**
     * Recalculates the closest position for this SlideNode
     */
    void ResetPositions();

    /**
     * @param toAttach Which rail this slideNode starts sliding on when reset
     * or spawning
     */
    void setDefaultRail(RailGroup* toAttach)
    {
    	mOrgRailGroup = mCurRailGroup = toAttach;
		ResetPositions();
    }

    /**
     * @param toAttach Which rail to attach to, Pass nullptr to detach this
     * SlideNode from any rail.
     */
    void attachToRail(RailGroup* toAttach)
    {
		mCurRailGroup = toAttach;
		ResetPositions();
		_coupler.setCurThreshold(_slidingBeam.isBroken() ? getLenTo(_slidingBeam) : _coupler.getInitThreshold());
    }



    //! distance from a beam before corrective forces take effect
	void setThreshold( Ogre::Real threshold ) { 
		_coupler.setInitThreshold(threshold);
		_coupler.setCurThreshold( threshold );
	}
	//! spring force used to calculate corrective forces
	void setSpringRate( Ogre::Real rate ){ _coupler.setSpringRate( rate ); }
	//! Damping rate used to calculate corrective forces
	void setDampingRate( Ogre::Real rate ){ _coupler.setDampingRate( rate ); }
	//! Force required to break the Node from the Rail
	void setBreakForce( Ogre::Real breakRate ) { _coupler.setBreakForce( breakRate ); }
	//! how long it will take for springs to fully attach to the Rail
	void setAttachmentRate( Ogre::Real rate ) { _coupler.setAttachRate( rate ); }
	//! maximum distance this spring node is allowed to reach out for a Rail
	void setAttachmentDistance( Ogre::Real dist ){ mAttachDist = fabs( dist ); }
	
	//! Distance away from beam before corrective forces begin to act on the node
	Ogre::Real getThreshold () const { return _coupler.getCurThreshold(); }
	//! spring force used to calculate corrective forces
	Ogre::Real getSpringRate() const { return _coupler.getSpringRate(); }
	//! Damping rate used to calculate corrective forces
	Ogre::Real getDampingRate() const { return _coupler.getDampingRate(); }
	//! Force required to break the No_slidingBeamde from the Rail
	Ogre::Real getBreakForce() const { return _coupler.getBreakForce(); }
	//! how long it will take for springs to fully attach to the Rail
	Ogre::Real getAttachmentRate() const { return _coupler.getAttachRate(); }
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
    static Ogre::Real getLenTo( const RailGroup* group, const Ogre::Vector3& point );

    /**
     * @param rail
     * @param point
     * @return value is always positive, if rail is null return infinity.
     */
    static Ogre::Real getLenTo( const Rail* rail, const Ogre::Vector3& point );

    /**
     * @param beam
     * @param point
     * @return value is always positive, if beam is null return infinity
     */
    static Ogre::Real getLenTo( const beam_t* beam, const Ogre::Vector3& point );

	/**
	 * @param beam
	 * @param point
	 * @return value is always positive, if beam is null return infinity
	 */
	static Ogre::Real getLenTo( const BeamComponent& beam, const Ogre::Vector3& point );


    /**
     * @param group
     * @return value is always positive, if group is null return infinity.
     */
    Ogre::Real getLenTo( const RailGroup* group ) const;

    /**
     * @param rail
     * @return value is always positive, if rail is null return infinity.
     */
    Ogre::Real getLenTo( const Rail* rail ) const;

    /**
     *
     * @param beam
     * @return value is always positive, if beam is null return infinity
     */
    Ogre::Real getLenTo( const beam_t* beam) const;

	/**
	 *
	 * @param beam
	 * @return value is always positive, if beam is null return infinity
	 */
	Ogre::Real getLenTo( const BeamComponent& beam) const;

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
