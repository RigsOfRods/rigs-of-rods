/*
 * BeamComponents.h
 *
 *  Created on: Jan 14, 2013
 *      Author: chris
 */

#ifndef BEAMCOMPONENTS_H_
#define BEAMCOMPONENTS_H_
#include "datatypes/beam.h"
#include "physics/framework/UpdateComponents.h"
#include "physics/framework/PhysicalComponent.h"
#include "physics/framework/NodeComponent.h"

/** 
 * this is a beam that acts like a node. It shall contain a ratio that indicates
 * where upon the beam forces are acted upon, and values are calculated from.
 */
class BeamComponent
: public Framework::Components::LinearSIComponent
, public Framework::Components::LinearSICoupler
{

public:
	typedef typename Framework::Components::LinearSICoupler::NodeType NodeType;
	
	BeamComponent(beam_t* beam)
	: _ratio(0.5)
	, _invRatio(0.5)
	, _beam(beam)
	, _n1(_beam ? _beam->p1 : NULL)
	, _n2(_beam ? _beam->p2 : NULL)
	{
	}
	
	virtual ~BeamComponent()
	{
		_beam = NULL;
	}
	
	/**
	 * applies force to the IntegrateComponent object, applied force is added
	 * to already existing force
	 * @param force amount of force to apply to the IntegrateComponent object
	 */
	virtual void applyForce(Units::Newton force)
	{
		_n1.applyForce(force * _invRatio);
		_n2.applyForce(force * _ratio);
	}	
	
	/**
	 * Retrieves the current force value
	 * @return current force
	 */
	virtual Units::Newton getForces() const
	{
		return (_n1.getForces() * _invRatio ) + (_n2.getForces() * _ratio);
	}
	
	/**
	 * Retrieves the current position
	 * @return current position
	 */
	virtual Units::Meter getPosition() const
	{
		return (_n1.getPosition() * _invRatio ) + (_n2.getPosition() * _ratio);
	}
	
	/**
	 * Retrieves the current velocity
	 * @return current velocity
	 */
	virtual Units::MetersPerSecond getVelocity() const
	{
		return (_n1.getVelocity() * _invRatio ) + (_n2.getVelocity() * _ratio);
	}
	
	bool isBroken()
	{
		return !_beam || _beam->broken || !_n1.getNode() || !_n2.getNode();
	}
	
	void setRatio(Ogre::Real ratio)
	{
		_ratio = ratio;
		_invRatio = 1 - ratio;
	}
	
	Ogre::Real getRatio() const
	{
		return _ratio;
	}
	
	beam_t* getBeam() const
	{
		return _beam;
	}
	
	void setBeam(beam_t* beam)
	{
		_beam = beam;
		
		_n1.setNode(_beam ? _beam->p1 : NULL);
		_n2.setNode(_beam ? _beam->p2 : NULL);
	}
	
	virtual const NodeType& getNode1() const
	{
		return _n1;
	}
	virtual const NodeType& getNode2() const
	{
		return _n2;
	}
	
private:
    //! ratio of length along the slide beam where the virtual node is
    //! 0.0f = p1, 1.0f = p2
	Ogre::Real _ratio;
	Ogre::Real _invRatio;
	
	beam_t* _beam;

	NodeComponent _n1;
	NodeComponent _n2;
};


#endif /* BEAMCOMPONENTS_H_ */
