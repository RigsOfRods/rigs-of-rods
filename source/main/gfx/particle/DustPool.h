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
#ifndef __DustPool_H_
#define __DustPool_H_

#include "RoRPrerequisites.h"

#include <pthread.h>

class DustPool : public ZeroedMemoryAllocator
{
public:

	DustPool(const char* dname, int dsize);
	~DustPool();

	void setVisible(bool s);
	//Dust
	void malloc(Ogre::Vector3 pos, Ogre::Vector3 vel, Ogre::ColourValue col = Ogre::ColourValue(0.83, 0.71, 0.64, 1.0));
	//clumps
	void allocClump(Ogre::Vector3 pos, Ogre::Vector3 vel, Ogre::ColourValue col = Ogre::ColourValue(0.83, 0.71, 0.64, 1.0));
	//Rubber smoke
	void allocSmoke(Ogre::Vector3 pos, Ogre::Vector3 vel);
	//
	void allocSparks(Ogre::Vector3 pos, Ogre::Vector3 vel);
	//Water vapour
	void allocVapour(Ogre::Vector3 pos, Ogre::Vector3 vel, float time);

	void allocDrip(Ogre::Vector3 pos, Ogre::Vector3 vel, float time);

	void allocSplash(Ogre::Vector3 pos, Ogre::Vector3 vel);

	void allocRipple(Ogre::Vector3 pos, Ogre::Vector3 vel);

	void update(float gspeed);

protected:

	static const int MAX_DUSTS = 100;

	enum DustTypes { DUST_NORMAL, DUST_RUBBER, DUST_DRIP, DUST_VAPOUR, DUST_SPLASH, DUST_RIPPLE, DUST_SPARKS, DUST_CLUMP };

	Ogre::ColourValue colours[MAX_DUSTS];
	Ogre::ParticleSystem* pss[MAX_DUSTS];
	Ogre::SceneNode *sns[MAX_DUSTS];
	Ogre::Vector3 positions[MAX_DUSTS];
	Ogre::Vector3 velocities[MAX_DUSTS];
	bool    visible[MAX_DUSTS];
	float rates[MAX_DUSTS];
	int allocated;
	int size;
	int types[MAX_DUSTS];

	pthread_mutex_t allocation_mutex;
};

#endif // __DustPool_H_
