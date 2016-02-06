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
#pragma once
#ifndef __RoRFrameListener_H_
#define __RoRFrameListener_H_

#include "RoRPrerequisites.h"

#include <Ogre.h>
#include <pthread.h>

class RoRFrameListener: public Ogre::FrameListener, public Ogre::WindowEventListener, public ZeroedMemoryAllocator
{
	friend class RoR::MainThread; // Temporary hack

public:

	RoRFrameListener();
	virtual ~RoRFrameListener();

	ChatSystem *netChat;

	bool freeTruckPosition; // Used for initial truck loading

	float netcheckGUITimer;

	int loading_state;
	
	Ogre::Vector3 reload_pos;

	void setSimPaused(bool state);

	void StartRaceTimer();

	float StopRaceTimer();

	void UpdateRacingGui();

	bool IsRaceInProgress()
	{
		return m_race_in_progress;
	}

protected:

#ifdef USE_MPLATFORM
	MPlatform_Base *mplatform;
#endif //USE_MPLATFORM

	Dashboard *dashboard;
	DOFManager *dof;
	ForceFeedback *forcefeedback;
	HeatHaze *heathaze;

	Ogre::Quaternion reload_dir;
	Ogre::Real mTimeUntilNextToggle; // just to stop toggles flipping too fast
	Ogre::Vector3 dirArrowPointed;

	float mLastSimulationSpeed; // previously used time ratio between real time (evt.timeSinceLastFrame) and physics time ('dt' used in calcPhysics)

	int mLastScreenShotID;
	Ogre::String mLastScreenShotDate;

	unsigned long      m_race_start_time;
	bool               m_race_in_progress;
	float			   m_race_bestlap_time;

	bool dirvisible;
	bool enablePosStor;
	bool flipflop;
	bool hidegui;
	bool mTruckInfoOn;
	bool pressure_pressed;

	bool m_is_sim_paused;

	char screenshotformat[256];
	
	collision_box_t *reload_box;
	double rtime;

	bool m_advanced_truck_repair;
	float m_advanced_truck_repair_timer;

	int mStatsOn;
	int netPointToUID;
	int raceStartTime;

	bool updateTruckMirrors(float dt);

	void updateIO(float dt);

	// WindowEventListener
	void windowMoved(Ogre::RenderWindow* rw);
	void windowClosed(Ogre::RenderWindow* rw);
	void windowFocusChange(Ogre::RenderWindow* rw);

public: // public methods

	bool RTSSgenerateShadersForMaterial(Ogre::String curMaterialName, Ogre::String normalTextureName);
	bool frameEnded(const Ogre::FrameEvent& evt);
	bool frameStarted(const Ogre::FrameEvent& evt); // Override frameStarted event to process that (don't care about frameEnded)

	bool updateEvents(float dt);
	double getTime() { return rtime; }
	int getNetPointToUID() { return netPointToUID; }

	void checkRemoteStreamResultsChanged();
	void hideGUI(bool hidden);
	void hideMap();
	void InitTrucks(
        bool loadmanual, 
        std::string const & selected, 
        int cache_entry_number = -1,
        std::string const & selectedExtension = "",
        const std::vector<Ogre::String> *truckconfig = nullptr,
        bool enterTruck = false,
        Skin *skin = nullptr
        );

	void netDisconnectTruck(int number);
	void pauseSim(bool value);
	void reloadCurrentTruck();

	void RTSSgenerateShaders(Ogre::Entity *entity, Ogre::String normalTextureName);
	void setDirectionArrow(char *text, Ogre::Vector3 position);

	void setNetPointToUID(int uid);
	void showLoad(int type, const Ogre::String &instance, const Ogre::String &box);
	void shutdown_final();
	void Restart();
	void windowResized(Ogre::RenderWindow* rw); // TODO: make this private, it's public for legacy reasons.
};

#endif // __RoRFrameListener_H_
