/*
 * Components.h
 *
 *  Created on: Dec 30, 2012
 *      Author: chris
 */

#ifndef COMPONENTS_H_
#define COMPONENTS_H_

#include "OgrePrerequisites.h"
#include "physics/framework/PhysicalComponent.h"

namespace Framework
{
namespace Components
{
template<class TimeUnits> 
class UpdateComponent
{
protected:
	UpdateComponent() {}
public: 
	virtual ~UpdateComponent() {}
	virtual void updateForce(TimeUnits dt) = 0;
};


template<class PositionUnits, class VelocityUnits, class ForceUnits> 
class Coupler
{
protected:
	Coupler() {}
public: 
	virtual ~Coupler() {}

	typedef class Components::PhysicalComponent<PositionUnits, VelocityUnits, ForceUnits> NodeType;
	
	virtual const NodeType& getNode1() const = 0;
	virtual const NodeType& getNode2() const = 0;

};

typedef class Coupler<Units::Meter, Units::MetersPerSecond, Units::Newton> LinearSICoupler; 

}

/**
 * Most basic coupler, this is the equivalent of a metal rod with torque being
 * applied to either last. in terms of RoR it is the equivalent of a beam
 * hint: it is the same as a beam with different units. 
 */
template<class PositionUnits, class VelocityUnits, class ForceUnits, class TimeUnits> 
class Coupler : public Components::UpdateComponent<TimeUnits>
, Components::Coupler<PositionUnits, VelocityUnits, ForceUnits>
{
	
public:
	typedef class Components::Coupler<PositionUnits, VelocityUnits, ForceUnits> CouplerType;
	typedef class CouplerType::NodeType NodeType;
	
	Coupler(NodeType& n1, NodeType& n2)
	: _n1(n1)
	, _n2(n2)
	, _springRate(9000000.0f)
	, _dampingRate(0.0f)
	, _breakForce(std::numeric_limits<Ogre::Real>::infinity() )
	, _broken(false)
	{}
	
	virtual ~Coupler() {}
	
	/**
	 * updates the inter-axle forces, needs to be done separate from integration
	 */
	virtual void updateForce(TimeUnits dt)
	{
		ForceUnits interForce = getInterForce();
		_broken =  getLength(interForce) > _breakForce;

		// apply negative taken away from earlier equation
		_n1.applyForce(interForce);
		_n2.applyForce(-interForce);
	}
	
	Ogre::Real getLength(Ogre::Vector3 v) { return v.length(); }
	Ogre::Real getLength(Ogre::Real l) { return l; }
	
	
	/** calculate the torque between the two last components based on difference
	 * in rotational speed and position
	 * @return the forces generated from a difference in speed between the two
	 * components
	 */
	virtual ForceUnits getInterForce()
	{
		if(_broken) return ForceUnits(0.0);
		return (-getSpringRate() * getDeltaPosition()) + (-getDampingRate() * getDeltaVelocity());
	}

	
	/**
	 * Calculated the difference in positions between the two wheels
	 * @return
	 */
	virtual PositionUnits getDeltaPosition()
	{
		return _n1.getPosition() - _n2.getPosition();	
	}
	
	/** calculated the difference in speed between the two components 
	 * @return the difference in angular velocity (omega) between the two
	 * supplied components 
	 */
	virtual VelocityUnits getDeltaVelocity()
	{
		return _n1.getVelocity() - _n2.getVelocity();	
	}
	
	virtual const NodeType& getNode1() const
	{
		return _n1;
	}

	virtual const NodeType& getNode2() const
	{
		return _n2;
	}

	Ogre::Real getSpringRate() const
	{
		return _springRate;
	}

	void setSpringRate(Ogre::Real springRate)
	{
		_springRate = springRate;
	}

	Ogre::Real getDampingRate() const
	{
		return _dampingRate;
	}

	void setDampingRate(Ogre::Real dampingRate)
	{
		_dampingRate = dampingRate;
	}

	Ogre::Real getBreakForce() const
	{
		return _breakForce;
	}

	void setBreakForce(Ogre::Real breakForce)
	{
		_breakForce = breakForce;
	}

	bool isBroken() const
	{
		return _broken;
	}

	void setBroken(bool broken)
	{
		_broken = broken;
	}

private:
	NodeType& _n1;
	NodeType& _n2;

	Ogre::Real  _springRate; //! spring rate of the beam connecting shafts 1 and 2 
	Ogre::Real _dampingRate; //! damping rate between shafts
	Ogre::Real  _breakForce; //! Force at which Slide Node breaks from rail
    bool           _broken; //! check if beam is broken
	

};
typedef class Coupler<Units::Meter, Units::MetersPerSecond, Units::Newton, Units::Second> LinearSICoupler; 




/**
 * 
 */
template<class PositionUnits, class VelocityUnits, class ForceUnits, class TimeUnits> 
class GearBox : public Coupler<PositionUnits, VelocityUnits, ForceUnits, TimeUnits>
{

	typedef class Coupler<PositionUnits, VelocityUnits, ForceUnits, TimeUnits> BaseClass;
public:

	
	GearBox(BaseClass& n1
	, Components::PhysicalComponent<PositionUnits, VelocityUnits, TimeUnits>& n2
	, Ogre::Real gear_ratio)
	: Coupler<PositionUnits, VelocityUnits, ForceUnits, TimeUnits>(n1, n2)
    , _ratio(gear_ratio)
	{
		assert(_ratio >= 0.001 || -0.001 >= _ratio);
	}
	
	virtual ~GearBox() {}
	/**
	 * updates the inter-axle forces, needs to be done separate from the
	 * integrator
	 */
	virtual void updateForce(TimeUnits dt)
	{
		ForceUnits interForces = BaseClass::getInterForce();

		// apply negative taken away from earlier equation
		BaseClass::node1.applyForce(interForces/_ratio);
		BaseClass::node2.applyForce(-interForces);
	}
	
	/** number of input rotations per single output rotation, ie a ratio one 10
	 * means the input shaft rotates 10 times for every single time the output
	 * shaft rotates once.
	 * 
	 * with a ratio of 10 the output shaft rotates 10 times slower than the
	 * input shaft but also multiplies the torque by 10
	 */
	void setRatio(Ogre::Real ratio)
	{
		_ratio = ratio;
	}
	
	Ogre::Real getRatio() const
	{
		return _ratio;
	}

//protected:
	PositionUnits getDeltaPosition()
	{
		return BaseClass::node1.getPosition() - (BaseClass::node2.getPosition() / _ratio);	
	}

	VelocityUnits getDeltaVel()
	{
		return BaseClass::node1.getVelocity() - (BaseClass::node2.getVelocity() / _ratio);	
	}

private:
	//! gear ratio see setRotation
	//! @ref void setRatio(Real ratio)
	Ogre::Real _ratio;         
};

// TODO: look into using a decorator pattern instead of inheritance
template<class PositionUnits, class VelocityUnits, class ForceUnits, class TimeUnits> 
class Differential
: public Coupler<PositionUnits, VelocityUnits, ForceUnits, TimeUnits>
{
public:
	typedef class Coupler<PositionUnits, VelocityUnits, ForceUnits, TimeUnits> BaseClass;
	
	Differential(BaseComponent<PositionUnits, VelocityUnits, ForceUnits>& inputNode
	, BaseComponent<PositionUnits, VelocityUnits, ForceUnits>& n1
	, BaseComponent<PositionUnits, VelocityUnits, ForceUnits>& n2
	, Ogre::Real gear_ratio)
	: Coupler<PositionUnits, VelocityUnits, ForceUnits, TimeUnits>(n1, n2)
	, _inputNode(inputNode)
	, _ratio(gear_ratio)
	{
		assert(_ratio >= 0.001 || -0.001 >= _ratio);
	}

	virtual ~Differential(){}
	
	/**
	 * updates the inter-axle forces, needs to be done separate from the
	 * integrator
	 */
	virtual void updateForce(TimeUnits dt)
	{
		
		// corrective torques for keeping the different shafts aligned, there are
		// two corrective forces at play, keeping the input shaft aligned with the
		// wheels, and keeping the wheels aligned with each other
		// torque to keep wheels and input shaft aligned
		ForceUnits inputInterForce = getInputInterForce();
		
		// torque to keep wheels aligned, sum applied to input shaft would be 0 so
		// don't apply there.
		ForceUnits interForce = BaseClass::getInterForce();
		

		_inputNode.applyForce(inputInterForce/_ratio);
		BaseClass::node1.applyForce(-interForce - inputInterForce);
		BaseClass::node2.applyForce(interForce - inputInterForce);
	}

	
	/** number of input rotations per single output rotation, ie a ratio one 10
	 * means the input shaft rotates 10 times for every single time the output
	 * shaft rotates once.
	 * 
	 * with a ratio of 10 the output shaft rotates 10 times slower than the
	 * input shaft but also multiplies the torque by 10
	 */
	void setRatio(Ogre::Real ratio)
	{
		_ratio = ratio;
	}
	
	Ogre::Real getRatio() const
	{
		return _ratio;
	}
	
//protected:
	PositionUnits getOutputDeltaPosition()
	{
		return _inputNode.getPosition()/_ratio - (BaseClass::node1.getPosition() + BaseClass::node2.getPosition()) * 0.5;	
	}
	
	VelocityUnits getOutputDeltaVelocity()
	{
		
		return _inputNode.getVelocity()/_ratio - (BaseClass::node1.getVelocity() + BaseClass::node2.getVelocity()) * 0.5;	
	}
		
	ForceUnits getInputInterForce()
	{
		return (-BaseClass::getSpringRate() * getOutputDeltaPosition()) + (-BaseClass::getDampingRate() * getOutputDeltaVelocity() );
	}
	
private:
	PositionUnits _deltaPos; //! accumulated difference between ends of the shaft
	VelocityUnits _deltaVel; //! difference in speed between last of the shaft 

	//! input shaft for torque input/output
	Components::PhysicalComponent<PositionUnits, VelocityUnits, ForceUnits>& _inputNode;
	//! gear ratio see setRatio
	//! @ref void setRatio(Real ratio)
	Ogre::Real _ratio;         
	
};

} //namespace Framework


#endif /* COMPONENTS_H_ */
