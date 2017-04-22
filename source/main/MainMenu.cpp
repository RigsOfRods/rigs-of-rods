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
    @file   
    @author Petr Ohlidal
    @date   05/2014
*/

#include "MainMenu.h"

#include "Application.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "BeamFactory.h"
#include "CacheSystem.h"
#include "CameraManager.h"
#include "Character.h"
#include "CharacterFactory.h"
#include "ChatSystem.h"
#include "ContentManager.h"
#include "DashBoardManager.h"
#include "DustManager.h"
#include "ErrorUtils.h"
#include "ForceFeedback.h"
#include "GlobalEnvironment.h"
#include "GUIManager.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MainSelector.h"
#include "GUI_MultiplayerClientList.h"
#include "GUI_MultiplayerSelector.h"
#include "Heathaze.h"
#include "InputEngine.h"
#include "Language.h"
#include "MumbleIntegration.h"

#include "Network.h"
#include "OgreSubsystem.h"
#include "OverlayWrapper.h"
#include "OutProtocol.h"

#include "RoRFrameListener.h"
#include "Scripting.h"
#include "Settings.h"
#include "SoundScriptManager.h"
#include "SurveyMapManager.h"
#include "TerrainManager.h"
#include "Utils.h"
#include "SkyManager.h"

#include <OgreRoot.h>
#include <OgreString.h>

#ifdef USE_ANGELSCRIPT
#    include "ScriptEngine.h"
#endif

#include <chrono>
#include <thread>

using namespace Ogre; // The _L() macro won't compile without.

namespace RoR {

MainMenu::MainMenu()
{
    RoR::App::SetMainMenu(this);
}

void MainMenu::EnterMainMenuLoop()
{
    RoRWindowEventUtilities::addWindowEventListener(App::GetOgreSubsystem()->GetRenderWindow(), this);

    // ==== FPS-limiter ====
    // TODO: Is this necessary in menu?

    unsigned long timeSinceLastFrame = 1;
    unsigned long startTime = 0;
    unsigned long minTimePerFrame = 0;
    unsigned long fpsLimit = App::GetGfxFpsLimit();

    if (fpsLimit < 10 || fpsLimit >= 200)
    {
        fpsLimit = 0;
    }

    if (fpsLimit)
    {
        minTimePerFrame = 1000 / fpsLimit;
    }

    while (App::GetPendingAppState() == App::APP_STATE_NONE)
    {
        startTime = App::GetOgreSubsystem()->GetTimer()->getMilliseconds();


        MainMenuLoopUpdate(timeSinceLastFrame);

        if (RoR::App::GetGuiManager()->GetMainSelector()->IsFinishedSelecting())
        {
            CacheEntry* selected_map = RoR::App::GetGuiManager()->GetMainSelector()->GetSelectedEntry();
            if (selected_map != nullptr)
            {
                App::GetGuiManager()->GetMainSelector()->Reset(); // TODO: Eliminate this mechanism ~ only_a_ptr 09/2016
                App::SetPendingAppState(App::APP_STATE_SIMULATION);
                App::SetSimNextTerrain(selected_map->fname);
            }
        }

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        RoRWindowEventUtilities::messagePump();
#endif
        Ogre::RenderWindow* rw = RoR::App::GetOgreSubsystem()->GetRenderWindow();
        if (rw->isClosed())
        {
            App::SetPendingAppState(App::APP_STATE_SHUTDOWN);
            continue;
        }

        RoR::App::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame();

#ifdef USE_SOCKETW
        if ((App::GetActiveMpState() == App::MP_STATE_CONNECTED) && RoR::Networking::CheckError())
        {
            Ogre::String title = Ogre::UTFString(_L("Network fatal error: ")).asUTF8();
            Ogre::String msg = RoR::Networking::GetErrorMessage().asUTF8();
            App::GetGuiManager()->ShowMessageBox(title, msg, true, "OK", true, false, "");
            App::SetPendingAppState(App::APP_STATE_MAIN_MENU);

            RoR::App::GetGuiManager()->GetMainSelector()->Hide();
            RoR::App::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
        }
#endif

        if (!rw->isActive() && rw->isVisible())
            rw->update(); // update even when in background !

        // FPS-limiter. TODO: Is this necessary in menu?
        if (fpsLimit && timeSinceLastFrame < minTimePerFrame)
        {
            // Sleep twice as long as we were too fast.
            int ms = static_cast<int>((minTimePerFrame - timeSinceLastFrame) << 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        }

        timeSinceLastFrame = RoR::App::GetOgreSubsystem()->GetTimer()->getMilliseconds() - startTime;
    }

    RoRWindowEventUtilities::removeWindowEventListener(App::GetOgreSubsystem()->GetRenderWindow(), this);
}

void MainMenu::MainMenuLoopUpdate(float seconds_since_last_frame)
{
    if (seconds_since_last_frame == 0)
    {
        return;
    }
    if (seconds_since_last_frame > 1.0 / 20.0) // TODO: Does this have any sense for menu?
    {
        seconds_since_last_frame = 1.0 / 20.0;
    }

    if (RoR::App::GetOgreSubsystem()->GetRenderWindow()->isClosed())
    {
        App::SetPendingAppState(App::APP_STATE_SHUTDOWN);
        return;
    }

#ifdef USE_SOCKETW
    if (App::GetActiveMpState() == App::MP_STATE_CONNECTED)
    {
        App::GetGuiManager()->GetMpClientList()->update();
    }
    else if (App::GetGuiManager()->GetMpSelector()->IsRefreshThreadRunning())
    {
        App::GetGuiManager()->GetMpSelector()->CheckAndProcessRefreshResult();
    }
#endif // USE_SOCKETW

    RoR::App::GetInputEngine()->Capture();

    MainMenuLoopUpdateEvents(seconds_since_last_frame);

#ifdef USE_ANGELSCRIPT
    ScriptEngine::getSingleton().framestep(seconds_since_last_frame);
#endif
}

void MainMenu::MainMenuLoopUpdateEvents(float seconds_since_last_frame)
{
    if (seconds_since_last_frame == 0.0f)
    {
        return;
    }

    RoR::App::GetInputEngine()->updateKeyBounces(seconds_since_last_frame);

    if (! RoR::App::GetInputEngine()->getInputsChanged())
    {
        return;
    }

    if (RoR::App::GetOverlayWrapper() != nullptr)
    {
        RoR::App::GetOverlayWrapper()->update(seconds_since_last_frame); // TODO: What's the meaning of this? It only updates some internal timer. ~ only_a_ptr
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUIT_GAME))
    {
        //TODO: Go back to menu 
        App::SetPendingAppState(App::APP_STATE_SHUTDOWN);
        return;
    }

    auto gui_man = App::GetGuiManager();
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_CONSOLE_TOGGLE, 5.f))
    {
        gui_man->SetVisible_Console(!gui_man->IsVisible_Console());
    }

    // TODO: screenshot

    // FOV settings disabled in menu

    // FIXME: full screen/windowed screen switching
    //if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FULLSCREEN_TOGGLE, 2.0f)) {}
}

void MainMenu::JoinMultiplayerServer()
{
#ifdef USE_SOCKETW

    auto gui = App::GetGuiManager();
    gui->SetVisible_MultiplayerSelector(false);
    gui->SetVisible_GameMainMenu(false);

    gui->GetLoadingWindow()->setAutotrack(_L("Connecting to server ..."));

    if (!Networking::Connect())
    {
        LOG("connection failed. server down?");
        gui->SetVisible_LoadingWindow(false);
        gui->SetVisible_GameMainMenu(true);

        gui->ShowMessageBox("Connection failed", Networking::GetErrorMessage().asUTF8_c_str(), true, "OK", true, false, "");
        return;
    }

    gui->SetVisible_LoadingWindow(false);
    gui->SetVisible_MpClientList(true);
    gui->GetMpClientList()->update();

    ChatSystem::SendStreamSetup();

#ifdef USE_MUMBLE
    SoundScriptManager::getSingleton().CheckAndCreateMumble();
#endif // USE_MUMBLE

    String terrain_name = Networking::GetTerrainName();
    if (terrain_name != "any")
    {
        App::SetSimNextTerrain(terrain_name);
        App::SetPendingAppState(App::APP_STATE_SIMULATION);
    }
    else
    {
        // Connected -> go directly to map selector
        if (App::GetDiagPreselectedTerrain() == "")
        {
            gui->GetMainSelector()->Reset();
            gui->GetMainSelector()->Show(LT_Terrain);
        }
        else
        {
            App::SetPendingAppState(App::APP_STATE_SIMULATION);
        }
    }
#endif //SOCKETW
}

void MainMenu::LeaveMultiplayerServer()
{
#ifdef USE_SOCKETW
    if (App::GetActiveMpState() == App::MP_STATE_CONNECTED)
    {
        App::GetGuiManager()->GetLoadingWindow()->setAutotrack(_L("Disconnecting, wait 10 seconds ..."));
        RoR::Networking::Disconnect();
        App::GetGuiManager()->SetVisible_MpClientList(false);
        App::GetGuiManager()->SetVisible_LoadingWindow(false);
    }
#endif //SOCKETW
}

void MainMenu::windowResized(Ogre::RenderWindow* rw)
{
    App::GetInputEngine()->windowResized(rw); // Update mouse area
}

void MainMenu::windowFocusChange(Ogre::RenderWindow* rw)
{
    App::GetInputEngine()->resetKeys();
}

} // namespace RoR


