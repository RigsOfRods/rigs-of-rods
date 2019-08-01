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

#include "ChatSystem.h"
#include "ContentManager.h"
#include "GUIManager.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MainSelector.h"
#include "GUI_MultiplayerClientList.h"
#include "GUI_MultiplayerSelector.h"
#include "InputEngine.h"
#include "Language.h"

#include "OgreSubsystem.h"
#include "OverlayWrapper.h"

#ifdef USE_ANGELSCRIPT
#    include "ScriptEngine.h"
#endif

#include <chrono>

namespace RoR {

MainMenu::MainMenu()
{
    RoR::App::SetMainMenu(this);
}

void MainMenu::EnterMainMenuLoop()
{
    OgreBites::WindowEventUtilities::addWindowEventListener(App::GetOgreSubsystem()->GetRenderWindow(), this);

    App::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(this);
    
    auto start_time = std::chrono::high_resolution_clock::now();

    while (App::app_state.GetPending() == AppState::MAIN_MENU)
    {
        // Check FPS limit
        if (App::gfx_fps_limit.GetActive() > 0)
        {
            const float min_frame_time = 1.0f / Ogre::Math::Clamp(App::gfx_fps_limit.GetActive(), 60, 240);
            float dt = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start_time).count();
            while (dt < min_frame_time)
            {
                int ms = static_cast<int>((min_frame_time - dt) * 1000.0f);
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                dt = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start_time).count();
            }
        }

        const auto now = std::chrono::high_resolution_clock::now();
        const float dt_sec = std::chrono::duration<float>(now - start_time).count();
        start_time = now;

        this->MainMenuLoopUpdate(dt_sec);

        if (RoR::App::GetGuiManager()->GetMainSelector()->IsFinishedSelecting())
        {
            if (RoR::App::GetGuiManager()->GetMainSelector()->GetSelectedEntry() != nullptr)
            {
                App::app_state.SetPending(AppState::SIMULATION);
                App::sim_terrain_name.SetPending("");
            }
        }

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        OgreBites::WindowEventUtilities::messagePump();
#endif
        Ogre::RenderWindow* rw = RoR::App::GetOgreSubsystem()->GetRenderWindow();
        if (rw->isClosed())
        {
            App::app_state.SetPending(AppState::SHUTDOWN);
            continue;
        }

        App::GetGuiManager()->NewImGuiFrame(dt_sec);
        App::GetGuiManager()->DrawMainMenuGui();

        App::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame();

        if (!rw->isActive() && rw->isVisible())
            rw->update(); // update even when in background !
    }
    OgreBites::WindowEventUtilities::removeWindowEventListener(App::GetOgreSubsystem()->GetRenderWindow(), this);
    App::GetOgreSubsystem()->GetOgreRoot()->removeFrameListener(this);
}

void MainMenu::MainMenuLoopUpdate(float seconds_since_last_frame)
{
    if (RoR::App::GetOgreSubsystem()->GetRenderWindow()->isClosed())
    {
        App::app_state.SetPending(AppState::SHUTDOWN);
        return;
    }

#ifdef USE_SOCKETW
    Networking::ConnectState con_state = Networking::CheckConnectingState();
    if (App::mp_state.GetActive() == MpState::CONNECTED)
    {
        if (con_state == Networking::ConnectState::KICKED) // Just kicked by server (only returned once)
        {
            this->LeaveMultiplayerServer();
            App::GetGuiManager()->ShowMessageBox(
                _LC("Network", "Multiplayer: disconnected"),
                Networking::GetStatusMessage().c_str());
            RoR::Networking::ResetStatusMessage();
        }
        else if (con_state == Networking::ConnectState::RECV_ERROR) // Receive error (only returned once)
        {
            this->LeaveMultiplayerServer();
            App::GetGuiManager()->ShowMessageBox(
                _L("Network fatal error: "),
                Networking::GetStatusMessage().c_str());
            RoR::Networking::ResetStatusMessage();
        }
        else
        {
            App::GetGuiManager()->GetMpClientList()->update();
        }
    }
    else if (App::mp_state.GetPending() == MpState::CONNECTED)
    {
        if (con_state == Networking::ConnectState::IDLE) // Not connecting yet
        {
            App::GetGuiManager()->SetVisible_MultiplayerSelector(false);
            bool connect_started = Networking::StartConnecting();
            App::GetGuiManager()->SetVisible_GameMainMenu(!connect_started);
            if (!connect_started)
            {
                App::GetGuiManager()->ShowMessageBox(
                    _LC("Network", "Multiplayer: connection failed"),
                    Networking::GetStatusMessage().c_str());
                App::mp_state.SetActive(RoR::MpState::DISABLED);
            }
        }
        else if (con_state == Networking::ConnectState::FAILURE) // Just failed (only returned once)
        {
            App::mp_state.SetActive(RoR::MpState::DISABLED);
            App::GetGuiManager()->ShowMessageBox(
                _LC("Network", "Multiplayer: connection failed"),
                Networking::GetStatusMessage().c_str());
            App::GetGuiManager()->SetVisible_GameMainMenu(true);
        }
        else if (con_state == Networking::ConnectState::SUCCESS) // Just succeeded (only returned once)
        {
            App::mp_state.SetActive(RoR::MpState::CONNECTED);
            App::GetGuiManager()->SetVisible_MpClientList(true);
            App::GetGuiManager()->GetMpClientList()->update();
            ChatSystem::SendStreamSetup();
            App::CheckAndCreateMumble();
            Ogre::String terrain_name = Networking::GetTerrainName();
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
                    App::GetGuiManager()->GetMainSelector()->Reset();
                    App::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
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

    if (App::app_force_cache_udpate.GetActive() || App::app_force_cache_purge.GetActive())
    {
        if (App::GetGuiManager()->IsVisible_GameSettings())
        {
            App::GetGuiManager()->SetVisible_GameSettings(false);
            App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::HIDDEN);
        }
        else
        {
            App::GetContentManager()->InitModCache();
            App::GetGuiManager()->SetVisible_GameMainMenu(true);
        }
    }

    RoR::App::GetInputEngine()->Capture();

    MainMenuLoopUpdateEvents(seconds_since_last_frame);

#ifdef USE_ANGELSCRIPT
    ScriptEngine::getSingleton().framestep(seconds_since_last_frame);
#endif
}

void MainMenu::MainMenuLoopUpdateEvents(float seconds_since_last_frame)
{
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

    this->HandleSavegameShortcuts();

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_CONSOLE_TOGGLE, 5.f))
    {
        App::GetGuiManager()->SetVisible_Console(!App::GetGuiManager()->IsVisible_Console());
    }

    // TODO: screenshot

    // FOV settings disabled in menu

    // FIXME: full screen/windowed screen switching
    //if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FULLSCREEN_TOGGLE, 2.0f)) {}
}

void MainMenu::HandleSavegameShortcuts()
{
    int slot = -1;
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_01, 1.0f))
    {
        slot = 1;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_02, 1.0f))
    {
        slot = 2;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_03, 1.0f))
    {
        slot = 3;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_04, 1.0f))
    {
        slot = 4;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_05, 1.0f))
    {
        slot = 5;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_06, 1.0f))
    {
        slot = 6;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_07, 1.0f))
    {
        slot = 7;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_08, 1.0f))
    {
        slot = 8;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_09, 1.0f))
    {
        slot = 9;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUICKLOAD_10, 1.0f))
    {
        slot = 0;
    }
    if (slot != -1)
    {
        Ogre::String filename = Ogre::StringUtil::format("quicksave-%d.sav", slot);
        App::sim_savegame.SetActive(filename.c_str());
        App::sim_load_savegame.SetActive(true);
        App::app_state.SetPending(AppState::SIMULATION);
    }
}

void MainMenu::LeaveMultiplayerServer()
{
#ifdef USE_SOCKETW
    if (App::mp_state.GetActive() == MpState::CONNECTED)
    {
        RoR::Networking::Disconnect();
        App::GetGuiManager()->SetVisible_MpClientList(false);
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


