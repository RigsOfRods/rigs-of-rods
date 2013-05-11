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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 24th of August 2009

#ifndef __BeamFactory_H_
#define __BeamFactory_H_

#include "RoRPrerequisites.h"

#include "Beam.h"
#include "StreamableFactory.h"
#include "TwoDReplay.h"

#include <pthread.h>

class BeamFactory : public StreamableFactory < BeamFactory, Beam >, public ZeroedMemoryAllocator
{
	friend class Network;
	friend class RoRFrameListener;

public:
	BeamFactory();
	~BeamFactory();

	Beam *createLocal(int slotid);
	Beam *createLocal(Ogre::Vector3 pos, Ogre::Quaternion rot, Ogre::String fname, collision_box_t *spawnbox = NULL, bool ismachine = false, int flareMode = 0, const std::vector<Ogre::String> *truckconfig = 0, Skin *skin = 0, bool freePosition = false);
	Beam *createRemoteInstance(stream_reg_t *reg);

	Beam *getBeam(int source_id, int stream_id); // used by character

	Beam *getCurrentTruck() { return (current_truck<0)?0:trucks[current_truck]; };
	Beam *getTruck(int number) { return trucks[number]; };
	Beam **getTrucks() { return trucks; };
	int getPreviousTruckNumber() { return previous_truck; };
	int getCurrentTruckNumber() { return current_truck; };
	int getTruckCount() { return free_truck; };
	bool allTrucksActivated() { return allActivated; };

	void setCurrentTruck(int new_truck);

	bool removeBeam(Beam *b);
	void removeCurrentTruck();
	void removeTruck(Collisions *collisions, const Ogre::String &inst, const Ogre::String &box);
	void removeTruck(int truck);
	
	bool enterRescueTruck();
	void repairTruck(Collisions *collisions, const Ogre::String &inst, const Ogre::String &box, bool keepPosition=false);

	void updateVisual(float dt);
	void updateAI(float dt);

	inline unsigned long getPhysFrame() { return physFrame; };

	void calcPhysics(float dt);
	void recalcGravityMasses();

	int updateSimulation(float dt);

	/* Returns whether or not the bounding boxes of truck a and truck b intersect. */
	bool checkTruckIntersection(int a, int b);
	void activateAllTrucks();
	void checkSleepingState();
	void sendAllTrucksSleeping();

	void windowResized();

protected:
	Ogre::SceneNode *parent;
	
	Beam *trucks[MAX_TRUCKS];
	int free_truck;
	int previous_truck;
	int current_truck;

	// set by "activate all vehicles"
	bool allActivated;

	TwoDReplay *tdr;

	unsigned long physFrame;

	bool checkForActive(int j, std::bitset<MAX_TRUCKS> &sleepyList);
	void recursiveActivation(int j);

	int getFreeTruckSlot();
	int findTruckInsideBox(Collisions *collisions, const Ogre::String &inst, const Ogre::String &box);

	// functions used by friends
	void netUserAttributesChanged(int source, int streamid);
	void localUserAttributesChanged(int newid);

	bool syncRemoteStreams();
	void updateGUI();
	void removeInstance(Beam *b);
	void removeInstance(stream_del_t *del);
	void _deleteTruck(Beam *b);
};

#endif // __BeamFactory_H_
