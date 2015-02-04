/*
 * PhysicalComponent.h
 *
 *  Created on: Jan 16, 2013
 *      Author: chris
 */

#ifndef PHYSICALCOMPONENT_H_
#define PHYSICALCOMPONENT_H_

#include "physics/framework/Units.h"

namespace Framework
{

namespace Components
{

template<class PositionUnits, class VelocityUnits, class ForceUnits> 
class PhysicalComponent
{
protected:
	PhysicalComponent() {}
public:
	virtual ~PhysicalComponent() {}
	
	/**
	 * applies force to the IntegrateComponent object, applied force is added
	 * to already existing force
	 * @param force amount of force to apply to the IntegrateComponent object
	 */
	virtual void applyForce(ForceUnits force) = 0;	
	
	/**
	 * Retrieves the current force value
	 * @return current force
	 */
	virtual ForceUnits getForces() const = 0;
	
	/**
	 * Retrieves the current position
	 * @return current position
	 */
	virtual PositionUnits getPosition() const = 0;
	
	/**
	 * Retrieves the current velocity
	 * @return current velocity
	 */
	virtual VelocityUnits getVelocity() const = 0;
};

typedef class PhysicalComponent<Units::Meter, Units::MetersPerSecond, Units::Newton> LinearSIComponent;

template<class MassUnits> 
class MassiveComponent
{
protected:
	// private so this class cannot be used directly
	MassiveComponent() {}
public:
	virtual ~MassiveComponent() {}

	/**
	 * changes the mass of the DriveTrainComponent object, when this happens momentum
	 * is conserved. When mass goes up, RPMs go down, and vice versa
	 * @param mass new mass of the DriveTrainComponent object
	 */
	virtual void setMass(MassUnits mass) = 0;
	/** 
	 * Retrieves the current mass value
	 * @return current mass
	 */
	virtual MassUnits getMass() const = 0;
	
};

}

template<class PositionUnits, class VelocityUnits, class ForceUnits>
struct State
{
	State()
	: pos(0.0f), vel(0.0f), force(0.0f)
	{ /* blank */ }
	
	State( PositionUnits position, VelocityUnits velocity, ForceUnits force)
	: pos(position), vel(velocity), force(force)
	{ /* blank */ }
	
	PositionUnits pos;
	VelocityUnits vel;
	ForceUnits force;
	
	
	void operator=(const State<PositionUnits, VelocityUnits, ForceUnits>& rhs)
	{
		pos =  rhs.pos;
		vel = rhs.vel;
		force = rhs.force;
	}
	
	void reset()
	{
		pos = 0.0f;
		vel = 0.0f;
		force = 0.0f;
	}
	
};

/**
 * @class BaseComponent the most basic building block for components
 * this class should not contain any knowledge of physics other than integration
 */
template<class PositionUnits, class VelocityUnits, class ForceUnits> 
class BaseComponent
: public Components::PhysicalComponent<ForceUnits, PositionUnits, VelocityUnits>
{
public: 
	BaseComponent() {}
	virtual ~BaseComponent() {}

	/**
	 * applies torque to the DriveTrainComponent object, applied torque is added
	 * to already existing torque
	 * @param torque amount of torque to apply to the DriveTrainComponent object
	 */
	void applyForce(ForceUnits force) { curState.force += force; }
	
	/**
	 *  Retrieves the current force value
	 * @return current force
	 */
	ForceUnits getForces() const { return curState.force; }
	/**
	 *  retrieves the current angular position
	 * @return current position
	 */
	PositionUnits getPosition() const { return curState.pos; }
	/**
	 * Retrieves the current Velocity
	 * @return current Velocity
	 */
	VelocityUnits getVelocity() const { return curState.vel; }
	
protected:
	State<ForceUnits, PositionUnits, VelocityUnits> curState;
	State<ForceUnits, PositionUnits, VelocityUnits> prevState;
	
};

/**
 * @class DriveTrainComponent any object that rotates, it can have torque applied to it for 
 * a specified amount of time. 
 */
template<class PositionUnits, class VelocityUnits, class ForceUnits, class MassUnits> 
class PhysicalComponent : public BaseComponent<PositionUnits, VelocityUnits, ForceUnits>
{
public:
	typedef class BaseComponent<PositionUnits, VelocityUnits, ForceUnits> BaseClass;
	/**
	 * 
	 * @param mass
	 * @return a PhysicalComponent object
	 */
	PhysicalComponent(MassUnits mass)
	: BaseComponent<PositionUnits, VelocityUnits, ForceUnits>(), _mass(mass) {}
	

	void applyForce(ForceUnits force) { BaseClass::applyForce(force * _invMass); }
	/**
	 * changes the mass of the DriveTrainComponent object, when this happens momentum
	 * is conserved. When mass goes up, RPMs go down, and vice versa
	 * @param mass new mass of the DriveTrainComponent object
	 */
	void setMass(MassUnits mass) { _mass = mass; _invMass = 1/mass; }
	/** 
	 * Retrieves the current mass value
	 * @return current mass
	 */
	MassUnits getMass() const { return _mass; }
private:
	MassUnits _mass;
	MassUnits _invMass;
};

typedef class PhysicalComponent<Units::Meter, Units::MetersPerSecond, Units::Newton, Units::Kilogram> LinearSIComponent;
typedef class PhysicalComponent<Units::Radian, Units::RadiansPerSecond, Units::NewtonMeter, Units::Kilogram> AngularSIComponent;

}

#endif /* PHYSICALCOMPONENT_H_ */
