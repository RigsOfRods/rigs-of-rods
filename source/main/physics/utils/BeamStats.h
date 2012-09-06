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
// thomas, 11th of March 2008

#ifndef __BeamStats_H_
#define __BeamStats_H_

#include "RoRPrerequisites.h"
#include <Ogre.h>

// BES = Beam Engine Statistics

#ifdef FEAT_TIMING

#define BES_START(x)     statistics->queryStart(x)
#define BES_STOP(x)      statistics->queryStop (x)
#define BES_GFX_START(x) statistics_gfx->queryStart(x)
#define BES_GFX_STOP(x)  statistics_gfx->queryStop (x)


#else //FEAT_TIMING

#define BES_START(x)
#define BES_STOP(x)
#define BES_GFX_START(x)
#define BES_GFX_STOP(x)

#endif //FEAT_TIMING

#ifdef FEAT_TIMING

// Beam engine stats type
enum {
	BES_CORE,
	BES_GFX
};

// stats items
#define MAX_TIMINGS 60 // this number must fit with the amount of enum items below
enum {
	BES_CORE_WholeTruckCalc=0,
	BES_CORE_Beams,
	BES_CORE_Nodes,
	BES_CORE_TruckEngine,
	BES_CORE_Rigidifiers,
	BES_CORE_Ropes,
	BES_CORE_Turboprop,
	BES_CORE_Screwprop,
	BES_CORE_Wing,
	BES_CORE_FuseDrag,
	BES_CORE_Airbrakes,
	BES_CORE_Buoyance,
	BES_CORE_Contacters,
	BES_CORE_CollisionCab,
	BES_CORE_Wheels,
	BES_CORE_Shocks,
	BES_CORE_Hydros,
	BES_CORE_Commands,
	BES_CORE_AnimatedProps,
	BES_CORE_SkeletonColouring,
	BES_CORE_Hooks,
	BES_CORE_SlideNodes,
	BES_CORE_Axles,
	BES_CORE_Replay,
	BES_CORE_Skidmarks
	// if you change this, change MAX_TIMINGS as well
};

enum {
	BES_GFX_framestep=0,
	BES_GFX_UpdateSkeleton,
	BES_GFX_ScaleTruck,
	BES_GFX_checkBeamMaterial,
	BES_GFX_pushNetwork,
	BES_GFX_calcNetwork,
	BES_GFX_calc_masses2,
	BES_GFX_calcBox,
	BES_GFX_calcNodeConnectivityGraph,
	BES_GFX_calculateDriverPos,
	BES_GFX_sendStreamData,
	BES_GFX_receiveStreamData,
	BES_GFX_calcAnimators,
	BES_GFX_updateFlares,
	BES_GFX_updateProps,
	BES_GFX_updateSoundSources,
	BES_GFX_updateVisual,
	BES_GFX_updateFlexBodies,
	BES_GFX_updateNetworkInfo
	// if you change this, change MAX_TIMINGS as well
};


#define BES BeamEngineStats::getInstance()
class BeamThreadStats : public ZeroedMemoryAllocator
{
public:
	BeamThreadStats(int type);
	~BeamThreadStats();
	void queryStart(int type);
	void queryStop(int type);
	void frameStep(float ds);

	unsigned int getFramecount();
	unsigned int getPhysFrameCount();
	double getTiming(int type);

private:
	PrecisionTimer *timings_start[MAX_TIMINGS];
	double timings[MAX_TIMINGS];
	double savedTimings[MAX_TIMINGS];
	Ogre::String stattext;
	unsigned int framecounter;
	unsigned int physcounter;
	float updateTime;
	int stype;
};

class BeamEngineStats : public ZeroedMemoryAllocator
{
public:
	bool updateGUI(float dt);
	void setup(bool enabled);
	BeamThreadStats *getClient(int number, int type);
	static BeamEngineStats & getInstance();
	~BeamEngineStats();
protected:
	BeamEngineStats();
	BeamEngineStats(const BeamEngineStats&);
	BeamEngineStats& operator= (const BeamEngineStats&);
	static Ogre::String typeDescriptions[MAX_TIMINGS];
	static Ogre::String typeDescriptions_gfx[MAX_TIMINGS];

private:

	typedef struct stats_entry_t {
		int trucknum;
		BeamThreadStats *stat;
	} stats_entry_t;
	bool enabled;
	static BeamEngineStats *myInstance;
	std::vector<stats_entry_t> statClients;
	std::vector<stats_entry_t> statClientsGFX;
	Ogre::OverlayElement *stats;
	float updateTimeGUI;
};

#endif // FEAT_TIMING

#endif //__BeamStats_H_
