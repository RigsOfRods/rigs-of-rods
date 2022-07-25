/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

// -------------------------------------------
// It turns out Ogre::Root is dumb and doesn't tolerate user creating individual singletons early by hand.
// The singletons are also reason you can't split OGRE usage into frontend/backend.
// For example, backend should not process .material files, 
//     but there's only one singleton controlling it.
// Using Ogre::Root without initializing rendering is still possible, 
//     but only if there's completely different renderer.
// 
// For this backend, I give up on separating OGRE tasks, I let the backend load everything.
// I'll only move rendering-related message processing and simbuffer processing here
//     so that future frontend can use it as tutorial/template.
// -------------------------------------------

#include "MessageHandler.h"

// Includes from backend
#include "Application.h"
#include "PlatformUtils.h"

// Dependencies
#include <Bites/OgreWindowEventUtilities.h>
#include <Ogre.h>

// System includes
#include <string>
#include <thread>

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char *argv[])
{
    // First create fallback ogre log
    OGRE_NEW Ogre::LogManager();
    Ogre::Log* safe_log = Ogre::LogManager::getSingleton().createLog("Frontend.log", /*defaultLog:*/false, /*debuggerOutput:*/true);
    safe_log->stream() << "This is Rigs of Rods frontend, proof of concept\n";

    // Initialize the backend
    int start_res = RoR::StartApplication(argc, argv);
    safe_log->stream() << "RoR::StartApplication() finished, result:" << start_res << "\n";

    // Run the backend message loop
    MessageHandler msg_handler;
    std::thread msgloop_thread = std::thread(&RoR::RunApplicationMessageLoop, &msg_handler);

    while (!msg_handler.WasExitRequested())
    {
        OgreBites::WindowEventUtilities::messagePump();
        // TODO: request simbuffers, process simbuffers, invoke rendering.
    }

    // Wait until the loop exits
    msgloop_thread.join();
    safe_log->stream() << "RoR::RunApplicationMessageLoop() ended, exit!\n";
}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <Windows.h>
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT)
{
    return main(__argc, __argv);
}
#endif

#ifdef __cplusplus
}
#endif