/*
 * ThresholdCoupler.h
 *
 *  Created on: Jan 30, 2013
 *      Author: chris
 */

#ifndef THRESHOLDCOUPLER_H_
#define THRESHOLDCOUPLER_H_

#include "UpdateComponents.h"

/**
 * ThresholdCoupler
 * feeling like this should be an adapter pattern instead of inheritance
 */
template<class PositionUnits, class VelocityUnits, class ForceUnits, class TimeUnits> 
class ThresholdCoupler : public Framework::Coupler<PositionUnits, VelocityUnits, ForceUnits, TimeUnits>
{

	typedef class Framework::Coupler<PositionUnits, VelocityUnits, ForceUnits, TimeUnits> BaseClass;
public:
	typedef class BaseClass::NodeType NodeType;
	
	ThresholdCoupler(NodeType& n1, NodeType& n2)
	: Framework::Coupler<PositionUnits, VelocityUnits, ForceUnits, TimeUnits>(n1, n2)
	, _initThreshold(0.0)
	, _curThreshold(0.0)
	, _attachRate(1.0)
	{}

	virtual ~ThresholdCoupler() {}

	virtual void updateForce(TimeUnits dt)
	{		
		// the threshold changes when attaching a node to a beam, if it's more than
		// the initial amount then then we're still attaching.
		if ( _curThreshold > _initThreshold )
		{
			_curThreshold -= (_attachRate * dt);
		}
		
		BaseClass::updateForce(dt);
	}
	/**
	 * Calculated the difference in positions between the two wheels
	 * @return
	 */
	virtual PositionUnits getDeltaPosition()
	{
		PositionUnits base = BaseClass::getDeltaPosition();
		return base - (base.normalisedCopy() * _curThreshold);
	}

	//! Distance away from beam before corrective forces begin to act on the node
	Ogre::Real getCurThreshold() const
	{
		return _curThreshold;
	}

	//! Distance away from beam before corrective forces begin to act on the node
	void setCurThreshold(Ogre::Real curThreshold)
	{
		_curThreshold = fabs(curThreshold);
	}

	 //! distance from beam calculating corrective forces (m)s
	Ogre::Real getInitThreshold() const
	{
		return _initThreshold;
	}
	
	 //! distance from beam calculating corrective forces (m)
	void setInitThreshold(Ogre::Real initThreshold)
	{
		_initThreshold = fabs(initThreshold);
	}

	//! how long it will take for springs to fully attach to the Rail
	Ogre::Real getAttachRate() const
	{
		return _attachRate;
	}

	//! how long it will take for springs to fully attach to the Rail
	void setAttachRate(Ogre::Real attachRate)
	{
		_attachRate = fabs(attachRate);
	}

private:
    Ogre::Real _initThreshold; //!< distance from beam calculating corrective forces (m)
    Ogre::Real  _curThreshold; //!< current threshold, used for attaching a beam (m)

    Ogre::Real    _attachRate; //!< how fast the cur threshold changes (m/s)
};

typedef class ThresholdCoupler<Units::Meter, Units::MetersPerSecond, Units::Newton, Units::Second> LinearSIThresholdCoupler; 


#endif /* THRESHOLDCOUPLER_H_ */
