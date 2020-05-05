/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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
    
    @author Petr Ohlidal
    @date   05/2014
    @brief  Main menu logic. Also helps with multiplayer setup.
*/

#pragma once

#include "RoRPrerequisites.h"

#include <OgreFrameListener.h>

namespace RoR {

class MainMenu: public Ogre::WindowEventListener, public Ogre::FrameListener
{
public:

    MainMenu();

    void LeaveMultiplayerServer();

    void EnterMainMenuLoop();

private:

    void MainMenuLoopUpdate(float seconds_since_last_frame);

    void MainMenuLoopUpdateEvents(float seconds_since_last_frame);

    void HandleSavegameShortcuts();

    // From Ogre::FrameListener
    bool frameStarted(const Ogre::FrameEvent & evt) override;

    // Ogre::WindowEventListener
    void windowResized    (Ogre::RenderWindow* rw) override;
    void windowFocusChange(Ogre::RenderWindow* rw) override;
};

} // namespace RoR
