/*
 * VirtualNodePosition.h
 *
 *  Created on: Dec 29, 2012
 *      Author: chris
 */

#ifndef VIRTUALNODEPOSITION_H_
#define VIRTUALNODEPOSITION_H_
#include <OgrePrerequisites.h>

struct node_t;

/** this is class to generalize the idea that a node should be at a certain position.
 * This idea was started by slide nodes, this intends to expand the same method
 * into a more generic and reusable implementation.
 * 
 * TODO this appears to be a form of Framework::Coupler<,,,>
 */
class VirtualNodePosition
{
// Members /////////////////////////////////////////////////////////////////////
private:
	/// reference to the physical node this instance is controlling
	node_t* _node;
	
	// use a node_t instead?
	Ogre::Vector3 _pos;
	Ogre::Vector3 _vel;
	
	Ogre::Real _springRate;
	Ogre::Real _dampRate;
public:
	/* no public members */

// Methods /////////////////////////////////////////////////////////////////////
private:
	Ogre::Vector3 getBeamForces();
	Ogre::Vector3 getDampingForces();
	
public:
	VirtualNodePosition(node_t* node);
	VirtualNodePosition(node_t& node);
	virtual ~VirtualNodePosition();

	void UpdateForces();
	virtual void integrate(Ogre::Real dt) { /* do nothing */ }
	virtual void applyforce(const Ogre::Vector3& force) { /* do nothing */ }

	//! 
	void setPosition(const Ogre::Vector3& pos){ _pos = pos; }
	//! 
	void setVelocity(const Ogre::Vector3& vel){ _vel = vel; }
	//! spring force used to calculate corrective forces
	void setSpringRate( Ogre::Real rate ){ _springRate = fabs( rate ); }
	//! spring force used to calculate corrective forces
	void setDampRate( Ogre::Real rate ){ _dampRate = fabs( rate ); }

	//!	
	const Ogre::Vector3& getPosition() { return _pos; }
	//! 
	const Ogre::Vector3& getVelocity() { return _vel; }
	//! spring force used to calculate corrective forces
	Ogre::Real getSpringRate() const { return _springRate; }
	//! spring force used to calculate corrective forces
	Ogre::Real getDampRate() const { return _dampRate; }
};

#endif /* VIRTUALNODEPOSITION_H_ */
