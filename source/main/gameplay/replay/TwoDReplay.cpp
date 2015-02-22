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
#include "TwoDReplay.h"

#include <Ogre.h>

#include "Utils.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "BeamFactory.h"

#include "Language.h"

using namespace Ogre;

TwoDReplay::TwoDReplay() : file(0), time(0)
{
}


TwoDReplay::~TwoDReplay()
{
	if (file) fclose(file);
}

void TwoDReplay::update(float &dt)
{
	// frame update rate: every 0.25 milliseconds
	time += dt;
	if (time > 0.25f)
	{
		recordFrame();
		time -= 0.25f;
	}

}

void TwoDReplay::recordFrame()
{
	if (!file)
		start();

	Beam *v = BeamFactory::getSingleton().getCurrentTruck();
	if (!v) return;

	// write entry first
	r2d_file_entry_t entry;
	memset(&entry, 0, sizeof(entry));
	entry.type      = R2DTYPE_FRAME;
	entry.rignum    = v->trucknum;
	entry.time_ms   = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
	entry.physframe = BeamFactory::getSingleton().getPhysFrame();
	fwrite(&entry, 1, sizeof(entry), file);

	// then the actual data
	r2d_file_frame_t frame;
	memset(&frame, 0, sizeof(frame));

	// input things first
	frame.brake    = v->brake;
	frame.steering = v->hydrodircommand;
	frame.pbrake   = (v->parkingbrake > 0);
	if (v->engine)
	{
		frame.accel  = v->engine->getAcc();
		frame.clutch = v->engine->getClutch();
		frame.gear   = v->engine->getGear();
		frame.gearMode = v->engine->getAutoMode();
	}
	fwrite(&frame, 1, sizeof(frame), file);

	// then the wheels
	r2d_file_wheel_frame_t wf;
	for (int w=0; w < v->free_wheel; w++)
	{
		memset(&wf, 0, sizeof(wf));
		Vector3 idir = v->wheels[w].refnode0->AbsPosition - v->wheels[w].refnode1->AbsPosition;
		Vector3 mpos = v->wheels[w].refnode1->AbsPosition + idir * 0.5f;
		float angle = atan2(idir.dotProduct(Vector3::UNIT_X), idir.dotProduct(-Vector3::UNIT_Z));
		wf.wheel_pos[0] = mpos.x;
		wf.wheel_pos[1] = mpos.y;
		wf.wheel_pos[2] = mpos.z;
		wf.wheel_direction = angle;
		wf.wheel_slip   = v->wheels[w].lastSlip;
		wf.wheel_speed  = v->wheels[w].speed;
		fwrite(&wf, 1, sizeof(wf), file);
	}

	// TODO: actually record any coupled/locked trucks

}

void TwoDReplay::start()
{
	if (file) return;
	file = fopen("data.tdr", "w");

	// first: write header
	r2d_file_header_t header;
	memset(&header, 0, sizeof(header));
	strncpy(header.fileformat, R2DFILEFORMAT, 8);
	header.num_rigs = BeamFactory::getSingleton().getTruckCount();
	fwrite(&header, 1, sizeof(header), file);

	// second: write entry for every truck
	for (int t = 0; t < header.num_rigs; t++)
	{
		Beam *v = BeamFactory::getSingleton().getTruck(t);
		r2d_file_rig_t rig;
		memset(&rig, 0, sizeof(rig));
		rig.num = t;
		if (v)
		{
			strncpy(rig.name, v->getTruckName().c_str(), std::min<int>(v->getTruckName().size(), 255));
			strncpy(rig.filename, v->getTruckFileName().c_str(), std::min<int>(v->getTruckFileName().size(), 255));
			strncpy(rig.hash, v->getTruckHash().c_str(), std::min<int>(v->getTruckHash().size(), 255));
			rig.num_wheels = v->free_wheel;
		}
		fwrite(&rig, 1, sizeof(rig), file);

		// write wheel setup
		if (v && rig.num_wheels > 0)
		{
			r2d_file_wheel_setup_t ws;
			for (int w=0; w < rig.num_wheels; w++)
			{
				memset(&ws, 0, sizeof(ws));
				ws.width  = v->wheels[w].width;
				ws.radius = v->wheels[w].radius;
				ws.type   = v->wheels[w].propulsed;
				fwrite(&ws, 1, sizeof(ws), file);
			}
		}
	}

	// third: header done, now the data
}


