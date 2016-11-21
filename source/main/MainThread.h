/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013+     Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
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

#include <map>

class GameScript;

namespace RoR {

class MainThread
{
    friend class ::GameScript; // Needs LoadTerrain()

public:

    MainThread();

    void Go();

    void Exit();

    static void ChangedCurrentVehicle(Beam* previous_vehicle, Beam* current_vehicle);

    void RegenCache();

    void JoinMultiplayerServer();
    void LeaveMultiplayerServer();

    /// @return True if everything was prepared OK and simulation may start.
    bool SetupGameplayLoop();

    void UnloadTerrain();

    RoRFrameListener* GetFrameListener() { return m_frame_listener; }

protected:

    void EnterMainMenuLoop();

    void MainMenuLoopUpdate(float seconds_since_last_frame);

    void MainMenuLoopUpdateEvents(float seconds_since_last_frame);

    void EnterGameplayLoop();

    bool LoadTerrain(); ///< Reads GVar 'sim_terrain_pending'

    void ShowSurveyMap(bool hide);

    bool              m_no_rendering;
    bool              m_restart_requested;
    unsigned long     m_start_time;
    bool              m_base_resource_loaded;
    bool              m_is_mumble_created;
    RoRFrameListener* m_frame_listener;

    std::map<std::string, bool> isLoadedMap;
};

} // namespace RoR
