/*
 * VirtualWheel.h
 *
 *  Created on: Dec 30, 2012
 *      Author: chris
 */

#ifndef VIRTUALWHEEL_H_
#define VIRTUALWHEEL_H_

#include "VirtualNodePosition.h"
#include "framework/IntegrateComponents.h"

#include <OgrePrerequisites.h>
#include <vector>

/** virtual wheel is an IntegrateComponent that connects to many VirtualNodePositions
 * 
 */
class VirtualWheel: public Framework::AngularSIComponent
{
private:
	/// These are the virtual node positions for each node on the wheel.
	std::vector<VirtualNodePosition> nodes;
	
	int rays = 0;
public:
	VirtualWheel();
	virtual ~VirtualWheel();
	
};

#endif /* VIRTUALWHEEL_H_ */
