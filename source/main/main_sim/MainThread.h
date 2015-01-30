/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

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

/** 
	@file   MainThread.h
	@author Petr Ohlidal
	@date   05/2014
	@brief  Main thread logic. Manages startup and shutdown etc...
*/

#pragma once

#include "Application.h"
#include "RoRPrerequisites.h"
#include "RigEditor_ForwardDeclarations.h"

#include <pthread.h>

class GameScript;

namespace RoR
{

class MainThread
{
	friend class ::GameScript; // Needs LoadTerrain()

public:

	MainThread();
	
	void Go();

	void Exit();

	void RequestShutdown();

	void RequestRestart();

	void RequestExitCurrentLoop();

	void StartRaceTimer();

	float StopRaceTimer();

	void UpdateRacingGui();

	bool IsRaceInProgress()
	{
		return m_race_in_progress;
	}

	void SetNextApplicationState(Application::State next_app_state)
	{
		m_next_application_state = next_app_state;
	}

	static void ChangedCurrentVehicle(Beam *previous_vehicle, Beam *current_vehicle);
	
	void RegenCache();

	void BackToMenu();

	void UnloadTerrain(); //just unload terrain, no back to menu
protected:

	void EnterMainMenuLoop();

	void MainMenuLoopUpdate(float seconds_since_last_frame);
	
	void MainMenuLoopUpdateEvents(float seconds_since_last_frame);

	/**
	* @return True if everything was prepared OK and simulation may start.
	*/
	bool SetupGameplayLoop(bool enable_network, Ogre::String preselected_map);

	void EnterGameplayLoop();

	void LoadTerrain(Ogre::String const & terrain_file);

	void ShowSurveyMap(bool hide);

	bool               m_no_rendering;
	bool               m_exit_loop_requested;
	bool               m_shutdown_requested;
	bool               m_restart_requested;
	pthread_mutex_t    m_lock;
	unsigned long      m_start_time;
	unsigned long      m_race_start_time;
	bool               m_race_in_progress;
	Application::State m_next_application_state;
	Application::State m_application_state;
	RigEditor::Main*   m_rig_editor;
	bool			   m_base_resource_load;

};

} // namespace RoR
