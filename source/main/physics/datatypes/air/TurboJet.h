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
#ifndef __Turbojet_H_
#define __Turbojet_H_

#include "RoRPrerequisites.h"
#include "AeroEngine.h"

class Turbojet: public AeroEngine, public ZeroedMemoryAllocator
{
	friend class RigInspector;

public:

	Turbojet(char* propname, int tnumber, int trucknum, node_t *nd, int tnodefront, int tnodeback, int tnoderef, float tmaxdrythrust, bool treversable, bool tafterburnable, float tafterburnthrust, float diskdiam, float nozdiam, float nozlength, bool disable_smoke, bool heathaze, MaterialFunctionMapper *mfm, Skin *usedSkin, MaterialReplacer *mr);

	void flipStart();
	void reset();
	void setRPM(float _rpm);
	void setThrottle(float val);
	void toggleReverse();
	void updateForces(float dt, int doUpdate);
	void updateVisuals();

	Ogre::Vector3 getAxis() {return axis;};

	bool getIgnition() {return ignition;};
	bool getWarmup() {return warmup;};
	bool isFailed() {return failed;};
	float getAfterburner() {return (float)afterburner;};
	float getRPM() { return rpm; };
	float getRPMpc() { return rpm; };
	float getRadius() {return radius;};
	float getThrottle();
	float getpropwash() {return propwash;};
	int getNoderef() {return nodeback;};
	int getType() {return AEROENGINE_TYPE_TURBOJET;};

	bool afterburnable;

private:

	MaterialReplacer *mr;
	Ogre::ParticleSystem* heathazePS;
	Ogre::ParticleSystem* smokePS;
	Ogre::SceneNode *absnode;
	Ogre::SceneNode *nzsnode;
	Ogre::Vector3 axis;
	bool afterburner;
	bool failed;
	bool heathaze;
	bool ignition;
	bool reversable;
	bool reverse;
	bool warmup;
	float afterburnthrust; //!< in kN
	float area;
	float exhaust_velocity; //!< in m/s
	float lastflip;
	float maxdrythrust; //!< in kN
	float propwash;
	float radius;
	float reflen;
	float rpm; //!< in percent!
	float throtle;
	float timer;
	float warmupstart;
	float warmuptime;
	int ab_id;
	int mod_id;
	int nodeback;
	int nodefront;
	int noderef;
	int number;
	int src_id;
	int thr_id;
	int trucknum;
	node_t *nodes;
	Ogre::SceneNode *smokeNode;
};

#endif // __Turbojet_H_
