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
#ifndef __AutoPilot_H_
#define __AutoPilot_H_

#include "RoRPrerequisites.h"
#include "TerrainObjectManager.h"

class Autopilot : public ZeroedMemoryAllocator
{
public:

	enum {HEADING_NONE, HEADING_FIXED, HEADING_WLV, HEADING_NAV};
	enum {ALT_NONE, ALT_FIXED, ALT_VS};
	enum {LOCALIZER_VERTICAL, LOCALIZER_HORIZONTAL, LOCALIZER_NDB, LOCALIZER_VOR};

	int heading;
	bool wantsdisconnect;

	Autopilot(int trucknum);
	void reset();
	void disconnect();
	void setInertialReferences(node_t *refl, node_t *refr, node_t *refb, node_t *refc);
	int toggleHeading(int mode);
	int toggleAlt(int mode);
	bool toggleIAS();
	bool toggleGPWS();
	int adjHDG(int d);
	int adjALT(int d);
	int adjVS(int d);
	int adjIAS(int d);

	float getAilerons();
	float getElevator();
	float getRudder();
	float getThrottle(float thrtl, float dt);

	void gpws_update(float spawnheight);

	void getRadioFix(TerrainObjectManager::localizer_t *localizers, int free_localizer, float *vdev, float *hdev);

private:

	int mode_heading;
	int mode_alt;
	bool mode_ias;
	bool mode_gpws;
	int alt;
	int vs;
	int ias;
	node_t *ref_l;
	node_t *ref_r;
	node_t *ref_b;
	node_t *ref_c;
	float ref_span;
	float last_elevator;
	float last_aileron;
	float last_rudder;
	float last_gpws_height;
	float last_pullup_height;
	float lastradiov;
	float lastradioh;
	float lastradiorwh;
	float lastradiorwd;
	float last_closest_hdist;

	int trucknum;
};

#endif // __AutoPilot_H_
