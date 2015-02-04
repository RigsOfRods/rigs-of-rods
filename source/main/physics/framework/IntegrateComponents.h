/*
 * IntegrateComponents.h
 *
 *  Created on: Dec 30, 2012
 *      Author: chris
 */

#ifndef INTEGRATECOMPONENTS_H_
#define INTEGRATECOMPONENTS_H_

#include "OgrePrerequisites.h"

namespace Framework
{

namespace Components
{

template<class TimeUnits> 
class IntegrateComponent
{
private:
	IntegrateComponent() {}
public:
	virtual ~IntegrateComponent() {}
	
	/** sets torque to zero, base component is not a physical object
	 * @param dt amount of time for which torque is applied
	 */
	virtual void integrate( TimeUnits dt ) 
	{
		
#if 0
		PositionUnits posA = pos + (vel*dt) + (0.5 *force*dt*dt);
		VelocityUnits velA = vel + force*dt;

		PositionUnits posB = pos + (0.5*(vel + mPrevRPM)*dt) + (0.25*(force+mPrevTorque)*dt*dt);
		VelocityUnits velB = vel + 0.5*(force+mPrevTorque)*dt;
		
		pos = 0.5 *(posA + posB);
		vel = 0.5 * (velA + velB);
		mPrevTorque = force;
		mPrevRPM = vel;
#elif 0
		VelocityUnits velA = curState.vel + curState.force*dt;
		PositionUnits posA = curState.pos + (velA*dt) + (0.5 *curState.force*dt*dt);

		VelocityUnits velB = curState.vel + 0.5*(curState.force + prevState.force)*dt;
		PositionUnits posB = curState.pos + (0.5*(velB + curState.vel)*dt) + (0.25*(curState.force + prevState.force)*dt*dt);

		prevState = curState;
		
		// setup state for next state
		curState.pos = 0.5 *(posA + posB);
		curState.vel = 0.5 * (velA + velB);
		curState.force = 0.0f;
		
#elif 0
		PositionUnits posA = pos + vel*dt;
		VelocityUnits velA = vel + force*dt;

		PositionUnits posB = pos + 0.5*(vel + mPrevRPM)*dt;
		VelocityUnits velB = vel + 0.5*(force+mPrevTorque)*dt;
		pos = 0.5 *(posA + posB);
		vel = 0.5 * (velA + velB);
		mPrevTorque = force;
		mPrevRPM = vel;
#elif 0
		curState.vel += curState.force*dt;
		curState.pos += (curState.vel*dt) + (0.5 *curState.force*dt*dt);
#elif 0
		curState.vel += curState.force*dt;
		curState.pos += (curState.vel*dt);
#elif 0
		pos += (0.25*(vel + mPrevRPM)*dt) + (0.25*(force+mPrevTorque)*dt*dt);
		vel += 0.5*(force+mPrevTorque)*dt;	

		mPrevTorque = force;
		mPrevRPM = vel;
#elif 0
		pos += (0.25*(vel + mPrevRPM)*dt);
		vel += 0.5*(force+mPrevTorque)*dt;	

		mPrevTorque = force;
		mPrevRPM = vel;
#endif
	}
};

}



} // namespace Framework





#endif /* INTEGRATECOMPONENTS_H_ */
