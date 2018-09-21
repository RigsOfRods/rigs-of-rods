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


/// @file
/// @author Petr Ohlidal
/// @date   05/2014


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
    unsigned long fpsLimit = App::gfx_fps_limit.GetActive(); // TOD: use GVar directly without copying

    if (fpsLimit < 10 || fpsLimit >= 200)
    {
        fpsLimit = 0;
    }

    if (fpsLimit)
    {
        minTimePerFrame = 1000 / fpsLimit;
    }

    Ogre::Timer timer;
    App::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(this);
    
    while (App::app_state.GetPending() == AppState::MAIN_MENU)
    {
        startTime = timer.getMilliseconds();

        this->MainMenuLoopUpdate(static_cast<float>(timeSinceLastFrame)/1000);

        if (RoR::App::GetGuiManager()->GetMainSelector()->IsFinishedSelecting())
        {
            CacheEntry* selected_map = RoR::App::GetGuiManager()->GetMainSelector()->GetSelectedEntry();
            if (selected_map != nullptr)
            {
                App::GetGuiManager()->GetMainSelector()->Reset(); // TODO: Eliminate this mechanism ~ only_a_ptr 09/2016
                App::app_state.SetPending(AppState::SIMULATION);
                App::sim_terrain_name.SetPending(selected_map->fname.c_str());
            }
        }

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        RoRWindowEventUtilities::messagePump();
#endif
        Ogre::RenderWindow* rw = RoR::App::GetOgreSubsystem()->GetRenderWindow();
        if (rw->isClosed())
        {
            App::app_state.SetPending(AppState::SHUTDOWN);
            continue;
        }

        App::GetGuiManager()->NewImGuiFrame(static_cast<float>(timeSinceLastFrame)/1000);
        App::GetGuiManager()->DrawMainMenuGui();

        App::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame();

#ifdef USE_SOCKETW
        if ((App::mp_state.GetActive() == MpState::CONNECTED) && RoR::Networking::CheckError())
        {
            const char* title = LanguageEngine::getSingleton().lookUp("Network fatal error: ").asUTF8_c_str();
            const char* text = RoR::Networking::GetErrorMessage().asUTF8_c_str();
            App::GetGuiManager()->ShowMessageBox(title, text);
            App::app_state.SetPending(AppState::MAIN_MENU);

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

        timeSinceLastFrame = timer.getMilliseconds() - startTime;
    }
    RoRWindowEventUtilities::removeWindowEventListener(App::GetOgreSubsystem()->GetRenderWindow(), this);
    App::GetOgreSubsystem()->GetOgreRoot()->removeFrameListener(this);
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
        App::app_state.SetPending(AppState::SHUTDOWN);
        return;
    }

    auto gui = App::GetGuiManager();

#ifdef USE_SOCKETW
    if (App::mp_state.GetActive() == MpState::CONNECTED)
    {
        gui->GetMpClientList()->update();
    }
    else if (App::mp_state.GetPending() == MpState::CONNECTED)
    {
        Networking::ConnectState con_state = Networking::CheckConnectingState();
        if (con_state == Networking::ConnectState::IDLE) // Not connecting yet
        {
            gui->SetVisible_MultiplayerSelector(false);
            bool connect_started = Networking::StartConnecting();
            gui->SetVisible_GameMainMenu(!connect_started);
            if (!connect_started)
            {
                App::GetGuiManager()->ShowMessageBox("Multiplayer: connection failed", Networking::GetErrorMessage().asUTF8_c_str());
                App::mp_state.SetActive(RoR::MpState::DISABLED);
            }
        }
        else if (con_state == Networking::ConnectState::FAILURE) // Just failed (only returned once)
        {
            App::mp_state.SetActive(RoR::MpState::DISABLED);
            App::GetGuiManager()->ShowMessageBox("Multiplayer: connection failed", Networking::GetErrorMessage().asUTF8_c_str());
            App::GetGuiManager()->SetVisible_GameMainMenu(true);
        }
        else if (con_state == Networking::ConnectState::SUCCESS) // Just succeeded (only returned once)
        {
            App::mp_state.SetActive(RoR::MpState::CONNECTED);
            gui->SetVisible_MpClientList(true);
            ChatSystem::SendStreamSetup();
            App::CheckAndCreateMumble();
            String terrain_name = Networking::GetTerrainName();
            if (terrain_name != "any")
            {
                App::sim_terrain_name.SetPending(terrain_name.c_str());
                App::app_state.SetPending(AppState::SIMULATION);
            }
            else
            {
                // Connected -> go directly to map selector
                if (App::diag_preset_terrain.IsActiveEmpty())
                {
                    gui->GetMainSelector()->Reset();
                    gui->GetMainSelector()->Show(LT_Terrain);
                }
                else
                {
                    App::app_state.SetPending(AppState::SIMULATION);
                }
            }
        }
    }

    if (App::GetGuiManager()->GetMpSelector()->IsRefreshThreadRunning())
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

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUIT_GAME)) // TODO: EV_COMMON_QUIT_GAME should really be EV_COMMON_ESCAPE ~ only_a_ptr, 12/2017
    {
        if (App::GetGuiManager()->IsVisible_GameAbout())
        {
            App::GetGuiManager()->SetVisible_GameAbout(false);
            App::GetGuiManager()->SetVisible_GameMainMenu(true);
        }
        else if (App::GetGuiManager()->IsVisible_MainSelector())
        {
            App::GetGuiManager()->GetMainSelector()->Cancel();
            App::GetGuiManager()->SetVisible_GameMainMenu(true);
        }
        else if (App::GetGuiManager()->IsVisible_GameSettings())
        {
            App::GetGuiManager()->SetVisible_GameSettings(false);
            App::GetGuiManager()->SetVisible_GameMainMenu(true);
        }
        else if (App::GetGuiManager()->IsVisible_MultiplayerSelector())
        {
            App::GetGuiManager()->SetVisible_MultiplayerSelector(false);
            App::GetGuiManager()->SetVisible_GameMainMenu(true);
        }
        else
        {
            App::app_state.SetPending(AppState::SHUTDOWN);
        }
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

void MainMenu::LeaveMultiplayerServer()
{
#ifdef USE_SOCKETW
    if (App::mp_state.GetActive() == MpState::CONNECTED)
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

bool MainMenu::frameRenderingQueued(const Ogre::FrameEvent & evt)
{
    App::GetGuiManager()->GetImGui().Render();
    return true;
}

} // namespace RoR


