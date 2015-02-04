/*
 * VirtualNodePosition.cpp
 *
 *  Created on: Dec 29, 2012
 *      Author: chris
 */

#include "VirtualNodePosition.h"
#include "datatypes/node.h"

#include <OgreVector3.h>


VirtualNodePosition::VirtualNodePosition(node_t* node) : _node(node), _springRate(0.0), _dampRate(0.0) {}
VirtualNodePosition::VirtualNodePosition(node_t& node) : _node(&node), _springRate(0.0), _dampRate(0.0) {}

VirtualNodePosition::~VirtualNodePosition() {}

void VirtualNodePosition::UpdateForces()
{
	Ogre::Vector3 forces = getBeamForces() +  getDampingForces();
	_node->Forces += forces;
	// perform call back on applying forces to the virtual node position
	// -= forces
}
 

Ogre::Vector3 VirtualNodePosition::getBeamForces()
{
	const Ogre::Real mCurThreshold = 0.0;
	
	// get the difference in position from where we are, and where 
	// we are supposed to be
	const Ogre::Vector3 deltaLen = _node->AbsPosition - _pos;
	
	// remove the lower limit, this is the area in which there is no force
	const Ogre::Real beamLen = std::max( 0.0f, deltaLen.length() - mCurThreshold );
	deltaLen.normalise();
	// get the magnitude based on the modified difference in length	
	return -_springRate * (deltaLen * beamLen);
}

Ogre::Vector3 VirtualNodePosition::getDampingForces()
{
	const Ogre::Vector3 deltaLen = _node->AbsPosition - _pos;
	const Ogre::Vector3 deltaVel = _node->Velocity - _vel;
	
	return -_dampRate * deltaVel.dotProduct(deltaLen);
}

