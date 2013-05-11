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

#include "BeamStats.h"

#ifdef FEAT_TIMING

// thomas, 11th of March 2008

#include "Beam.h"
#include "Timer.h"
#include "BeamFactory.h"

using namespace std;
using namespace Ogre;

BeamEngineStats *BeamEngineStats::myInstance = 0;
Ogre::String BeamEngineStats::typeDescriptions[MAX_TIMINGS];
Ogre::String BeamEngineStats::typeDescriptions_gfx[MAX_TIMINGS];

BeamEngineStats::BeamEngineStats()
{
	enabled=true;
	updateTimeGUI=0;
	stats = OverlayManager::getSingleton().getOverlayElement("tracks/DebugBeamTiming/Text");

	// setup descriptions
	typeDescriptions[BES_CORE_WholeTruckCalc]    = "Sum";
	typeDescriptions[BES_CORE_Beams]             = "Beams";
	typeDescriptions[BES_CORE_Nodes]             = "Nodes";
	typeDescriptions[BES_CORE_TruckEngine]       = "TruckEngine";
	typeDescriptions[BES_CORE_Rigidifiers]       = "Rigidifiers";
	typeDescriptions[BES_CORE_Ropes]             = "Ropes";
	typeDescriptions[BES_CORE_Turboprop]         = "Turboprop";
	typeDescriptions[BES_CORE_Screwprop]         = "Screwprop";
	typeDescriptions[BES_CORE_Wing]              = "Wing";
	typeDescriptions[BES_CORE_FuseDrag]          = "FuseDrag";
	typeDescriptions[BES_CORE_Airbrakes]         = "Airbrakes";
	typeDescriptions[BES_CORE_Buoyance]          = "Buoyance";
	typeDescriptions[BES_CORE_Contacters]        = "Contacters";
	typeDescriptions[BES_CORE_CollisionCab]      = "CollisionCab";
	typeDescriptions[BES_CORE_Wheels]            = "Wheels";
	typeDescriptions[BES_CORE_Shocks]            = "Shocks";
	typeDescriptions[BES_CORE_Hydros]            = "Hydros";
	typeDescriptions[BES_CORE_Commands]          = "Commands";
	typeDescriptions[BES_CORE_AnimatedProps]     = "AnimatedProps";
	typeDescriptions[BES_CORE_SkeletonColouring] = "SkeletonColouring";
	typeDescriptions[BES_CORE_Hooks]             = "Hooks";
	typeDescriptions[BES_CORE_SlideNodes]        = "SlideNodes";
	typeDescriptions[BES_CORE_Axles]             = "Axles";
	typeDescriptions[BES_CORE_Replay]            = "Replay";
	typeDescriptions[BES_CORE_Skidmarks]         = "Skidmarks";

	typeDescriptions_gfx[BES_GFX_UpdateSkeleton]            = "UpdateSkeleton";
	typeDescriptions_gfx[BES_GFX_ScaleTruck]                = "ScaleTruck";
	typeDescriptions_gfx[BES_GFX_checkBeamMaterial]         = "checkBeamMaterial";
	typeDescriptions_gfx[BES_GFX_pushNetwork]               = "pushNetwork";
	typeDescriptions_gfx[BES_GFX_calcNetwork]               = "calcNetwork";
	typeDescriptions_gfx[BES_GFX_calc_masses2]              = "calc_masses2";
	typeDescriptions_gfx[BES_GFX_calcBox]                   = "calcBox";
	typeDescriptions_gfx[BES_GFX_calcNodeConnectivityGraph] = "calcNodeConGraph";
	typeDescriptions_gfx[BES_GFX_calculateDriverPos]        = "calculateDriverPos";
	typeDescriptions_gfx[BES_GFX_framestep]                 = "framestep";
	typeDescriptions_gfx[BES_GFX_sendStreamData]            = "sendStreamData";
	typeDescriptions_gfx[BES_GFX_receiveStreamData]         = "receiveStreamData";
	typeDescriptions_gfx[BES_GFX_calcAnimators]             = "calcAnimators";
	typeDescriptions_gfx[BES_GFX_updateFlares]              = "updateFlares";
	typeDescriptions_gfx[BES_GFX_updateProps]               = "updateProps";
	typeDescriptions_gfx[BES_GFX_updateSoundSources]        = "updateSoundSources";
	typeDescriptions_gfx[BES_GFX_updateVisual]              = "updateVisual";
	typeDescriptions_gfx[BES_GFX_updateFlexBodies]          = "updateFlexBodies";
	typeDescriptions_gfx[BES_GFX_updateNetworkInfo]         = "updateNetworkInfo";

	stats->setCaption("calculating ...");
}

BeamEngineStats::~BeamEngineStats()
{
}

BeamEngineStats &BeamEngineStats::getInstance()
{
	if (myInstance == 0) {
		myInstance = new BeamEngineStats();
	}
	return *myInstance;
}

BeamThreadStats *BeamEngineStats::getClient(int number, int type)
{
	if (!enabled)
		return 0;
	BeamThreadStats *bts = new BeamThreadStats(type);

	// TODO: check for duplicates
	//if (statClients[number]!=0)
	//	delete statClients[number];

	stats_entry_t e;
	e.trucknum = number;
	e.stat = bts;

	if (type == BES_CORE)
		statClients.push_back(e);
	else if (type == BES_GFX)
		statClientsGFX.push_back(e);

	return bts;
}

void BeamEngineStats::setup(bool enabled)
{
	this->enabled = enabled;
}

bool BeamEngineStats::updateGUI(float dt)
{
	int current_truck = BeamFactory::getSingleton().getCurrentTruckNumber();
	Beam **trucks = BeamFactory::getSingleton().getTrucks();
	
	updateTimeGUI += dt;
	if (updateTimeGUI < 5.0f)
		return true;

	updateTimeGUI = 0;

	if (statClients.size() == 0)
		return true;

	if (!stats->isVisible())
		return true;

	String msg = "";

	char tmp[1024]="";
	char tmp_gfx[1024]="";
	char line[1024]="";

	for (unsigned int c = 0; c < statClients.size(); c++)
	{
		//if (it->first != current_truck || trucks[it->first]->state == SLEEPING)
		if (!trucks[statClients[c].trucknum]) continue;
		if (trucks[statClients[c].trucknum]->state == SLEEPING)
			continue;

		BeamThreadStats *core = statClients[c].stat;
		BeamThreadStats *gfx  = statClientsGFX[c].stat;

		double sum=0, sum_gfx=0, t=0, perc=0;
		msg += "Stats Timestep: 5 Seconds\n";
		msg += "Truck "+TOSTRING(statClients[c].trucknum) + "\n";
		msg += "  Graphic Frames: " + TOSTRING(core->getFramecount()) + "\n";
		msg += "  Physic Frames:  " + TOSTRING(core->getPhysFrameCount()) + "\n";

		sprintf(line, "%-46s  |  %-46s\n", "============== PHYSICS ==============", "================ GFX ================");
		msg += String(line);

		sum = core->getTiming(BES_CORE_WholeTruckCalc);
		sum_gfx = core->getTiming(BES_CORE_WholeTruckCalc);
		if (sum==0)
			continue;

		sprintf(tmp, " %20s: % 10.4f%% : %1.6fs", "Whole truck", 100.0f, sum);
		sprintf(tmp_gfx, " %20s: % 10.4f%% : %1.6fs", "FrameTime", 100.0f, sum_gfx);
		sprintf(line, "%-46s  |  %-46s\n", tmp, tmp_gfx);
		msg += String(line);

		float sum_real = 0, sum_real_gfx = 0;
		for (int i=1;i<MAX_TIMINGS;i++)
		{
			sprintf(tmp, "");
			sprintf(tmp_gfx, "");

			bool addLine=false;
			if (!typeDescriptions[i].empty())
			{
				t = core->getTiming(i);
				sum_real += t;
				perc = (100.0 * t / sum);
				sprintf(tmp, " %20s: % 10.4f%% : %1.6fs", typeDescriptions[i].c_str(), perc, t);
				addLine=true;
			}
			if (!typeDescriptions_gfx[i].empty())
			{
				t = gfx->getTiming(i);
				sum_real_gfx += t;
				perc = (100.0 * t / sum_gfx);
				sprintf(tmp_gfx, " %20s: % 10.4f%% : %1.6fs", typeDescriptions_gfx[i].c_str(), perc, t);
				addLine=true;
			}
			if (!addLine) break;
			sprintf(line, "%-46s  |  %-46s\n", tmp, tmp_gfx);
			msg += String(line);
		}
		// find out the rest time
		{
			float unknown_time = sum - sum_real;
			perc=(int)(100.0 * unknown_time / sum);
			sprintf(tmp, " %20s: % 10.4f%% : %1.6fs", "Unknown", perc, unknown_time);

			float unknown_time_gfx = sum_gfx - sum_real_gfx;
			perc=(int)(100.0 * unknown_time_gfx / sum_gfx);
			sprintf(tmp_gfx, " %20s: % 10.4f%% : %1.6fs", "Unknown", perc, unknown_time_gfx);
			sprintf(line, "%-46s  |  %-46s\n", tmp, tmp_gfx);
			msg += String(line);
		}
	}
	stats->setCaption(msg);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BeamThreadStats::BeamThreadStats(int _type) : stype(_type)
{
	for (int i=0; i<MAX_TIMINGS; i++)
	{
		timings[i]=0;
		savedTimings[i]=0;
		timings_start[i]=0;
	}
	framecounter=0;
	physcounter=0;
	updateTime=0;
}

BeamThreadStats::~BeamThreadStats()
{
}

void BeamThreadStats::frameStep(float ds)
{
	updateTime += ds;
	if (updateTime > 5.0f)
	{
		for (int i=0; i<MAX_TIMINGS; i++)
			timings[i]=0;
		updateTime = 0;
	}
	framecounter++;
}

void BeamThreadStats::queryStart(int type)
{
	timings_start[type] = new PrecisionTimer();
}

void BeamThreadStats::queryStop(int type)
{
	if (!timings_start[type]) return;

	timings[type] += (timings_start[type]->elapsed());

	if (stype == BES_CORE && type == BES_CORE_WholeTruckCalc)
	{
		// secure timings
		physcounter++;
		memcpy(savedTimings, timings, sizeof(double) * MAX_TIMINGS);
	}
	else if (stype == BES_GFX)
	{
		// secure timings
		memcpy(savedTimings, timings, sizeof(double) * MAX_TIMINGS);
	}
}

unsigned int BeamThreadStats::getPhysFrameCount()
{
	return physcounter;
}

unsigned int BeamThreadStats::getFramecount()
{
	return framecounter;
}

double BeamThreadStats::getTiming(int type)
{
	return savedTimings[type];
}

#endif //FEAT_TIMING