/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2009      Christopher Ritchey

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

#include "ApproxMath.h"
#include "RoRPrerequisites.h"

#include <OgreVector3.h>

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

struct Rail : public ZeroedMemoryAllocator
{
    Rail();
    Rail( beam_t* newBeam );
    Rail( beam_t* newBeam, Rail* newPrev, Rail* newNext );
    Rail( Rail& other );

    Rail*   snr_prev;
    beam_t* snr_beam;
    Rail*   snr_next;
};

/// A series of softbody beams which node slides along. Can be closed in a loop. The term of 'Rail' means 'single beam' here.
class RailGroup : public ZeroedMemoryAllocator
{
public:
    RailGroup(Rail* start);
    RailGroup(Rail* start, unsigned int id);

    const Rail*       GetStartRail() const    { return m_start_rail; }
    unsigned int      GetId() const           { return m_id; }
    
    /// clears up all the allocated memory this is intentionally not made into a
    ///destructor to avoid issues like copying Rails when storing in a container
    /// it is assumed that if next is not null then next->prev is not null either
    void CleanUpRailGroup();
    
private:
        
    /// Identification, for anonymous rails (defined inline) the ID will be
    /// > 7000000, for Rail groups defined int he rail group section the Id
    /// needs to be less than 7000000
    unsigned int m_id;
    Rail*        m_start_rail; //!< Beginning Rail in the Group
};

/**
 * Rail builder is a utility used for created linked lists effectively and
 * painlessly.
 */
class RailBuilder : public ZeroedMemoryAllocator
{
public:

    RailBuilder();
    ~RailBuilder();
        
    void pushBack(beam_t* next);
    void loopRail();
    
    /**
     * @return the completed rail, if called before instance goes out of reference
     * the internal memory is null referenced, if not the memorey is reclaimed
     */
    Rail* getCompletedRail();

private:
    Rail*    mStart;    //! Start of the Rail series
    Rail*    mFront;    //! Front of the Rail, not necessarily the start
    Rail*     mBack;    //! Last rail in the series
    bool      mLoop;    //! Check if rail is to be looped
    bool mRetreived;    //! Check if RailBuilder needs to deallocate Rails
    
};

class SlideNode : public ZeroedMemoryAllocator
{
public:

    bool sn_attach_self:1;      //!< attach/detach to rails on the current vehicle only
    bool sn_attach_foreign:1;   //!< attach/detach to rails only on other vehicles
    bool sn_slide_broken:1;     //!< The slidenode was pulled away from the rail

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
    const Ogre::Vector3& getNodePosition() const;

    /**
     * @return The position where the SlideNode should be location on the slide beam
     */
    const Ogre::Vector3& getIdealPosition() const;

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
        return mCurRailGroup ? mCurRailGroup->GetId() : std::numeric_limits<unsigned int>::infinity();
    }

    /**
     * resets the slide Rail back to the initial rail, restores all broken rail
     * connections and finds the position along the rail closest to the SlideNode
     */
    void reset()
    {
        mCurRailGroup = mOrgRailGroup;
        sn_slide_broken = false;
        ResetPositions();
    }

    /**
     * Recalculates the closest position for this SlideNode
     */
    void ResetPositions();

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
    Ogre::Vector3 getCorrectiveForces();
};

