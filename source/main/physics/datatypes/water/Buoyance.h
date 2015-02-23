/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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
#ifndef __Buoyance_H_
#define __Buoyance_H_

#include "RoRPrerequisites.h"

class Buoyance : public ZeroedMemoryAllocator
{
public:

	Buoyance();
	~Buoyance();

	//compute tetrahedron volume
	inline float computeVolume(Ogre::Vector3 o, Ogre::Vector3 a, Ogre::Vector3 b, Ogre::Vector3 c);

	//compute pressure and drag force on a submerged triangle
	Ogre::Vector3 computePressureForceSub(Ogre::Vector3 a, Ogre::Vector3 b, Ogre::Vector3 c, Ogre::Vector3 vel, int type);
	
	//compute pressure and drag forces on a random triangle
	Ogre::Vector3 computePressureForce(Ogre::Vector3 a, Ogre::Vector3 b, Ogre::Vector3 c, Ogre::Vector3 vel, int type);
	
	void computeNodeForce(node_t *a, node_t *b, node_t *c, int doupdate, int type);

	void setsink(int v);

	enum { BUOY_NORMAL, BUOY_DRAGONLY, BUOY_DRAGLESS };

private:

	DustPool *splashp, *ripplep;
	int sink;
	int update;
};

#endif // __Buoyance_H_
