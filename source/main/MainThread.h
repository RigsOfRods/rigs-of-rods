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

    MainThread(RoRFrameListener* fl);

    void JoinMultiplayerServer();
    void LeaveMultiplayerServer();

    RoRFrameListener* GetFrameListener() { return m_frame_listener; }

    void EnterMainMenuLoop();

    void MainMenuLoopUpdate(float seconds_since_last_frame);

    void MainMenuLoopUpdateEvents(float seconds_since_last_frame);

private:
    
    bool              m_is_mumble_created;
    RoRFrameListener* m_frame_listener;

    
};

} // namespace RoR
