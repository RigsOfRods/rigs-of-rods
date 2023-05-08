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

/// @file
/// @author based on the basemanager code from mygui common

#pragma once

#include "OgreImGui.h"
#include "Application.h"
#include "GUI_MessageBox.h"

// GUI panels
#include "GUI_CollisionsDebug.h"
#include "GUI_ConsoleWindow.h"
#include "GUI_FlexbodyDebug.h"
#include "GUI_FrictionSettings.h"
#include "GUI_RepositorySelector.h"
#include "GUI_GameMainMenu.h"
#include "GUI_GameAbout.h"
#include "GUI_GameChatBox.h"
#include "GUI_GameSettings.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MessageBox.h"
#include "GUI_MultiplayerSelector.h"
#include "GUI_MultiplayerClientList.h"
#include "GUI_MainSelector.h"
#include "GUI_NodeBeamUtils.h"
#include "GUI_DirectionArrow.h"
#include "GUI_SimActorStats.h"
#include "GUI_SimPerfStats.h"
#include "GUI_SurveyMap.h"
#include "GUI_TextureToolWindow.h"
#include "GUI_GameControls.h"
#include "GUI_TopMenubar.h"
#include "GUI_VehicleDescription.h"
#include "GUI_VehicleButtons.h"

// Deps
#include <Bites/OgreWindowEventUtilities.h>
#include <MyGUI.h>
#include <MyGUI_OgrePlatform.h>
#include <MyGUI_UString.h>
#include <OgreOverlay.h>

namespace RoR {

class GUIManager
{
public:

    struct GuiTheme
    {
        ImVec4 in_progress_text_color    = ImVec4(1.f, 0.832031f, 0.f, 1.f);
        ImVec4 no_entries_text_color     = ImVec4(0.7f, 0.7f, 0.7f, 1.f);
        ImVec4 error_text_color          = ImVec4(1.f, 0.175439f, 0.175439f, 1.f);
        ImVec4 selected_entry_text_color = ImVec4(.9f, 0.7f, 0.05f, 1.f);
        ImVec4 value_red_text_color      = ImVec4(.9f, .1f, .1f, 1.f);
        ImVec4 value_blue_text_color     = ImVec4(0.34f, 0.67f, 0.84f, 1.f);
        ImVec4 highlight_text_color      = ImVec4(0.78f, 0.39f, 0.f, 1.f);
        ImVec4 success_text_color        = ImVec4(0.f, 0.8f, 0.f, 1.f);
        ImVec4 warning_text_color        = ImVec4(0.9f, 0.8f, 0.1f, 1.f);
        ImVec4 help_text_color           = ImVec4(0.5f, 0.7f, 1.f, 1.f);

        ImVec4 semitransparent_window_bg = ImVec4(0.1f, 0.1f, 0.1f, 0.8f);
        ImVec4 semitrans_text_bg_color   = ImVec4(0.1f, 0.1f, 0.1f, 0.6f);
        ImVec4 color_mark_max_darkness   = ImVec4(0.2, 0.2, 0.2, 0.0); //!< If all RGB components are darker than this, text is auto-lightened.

        ImVec2 screen_edge_padding       = ImVec2(10.f, 10.f);
        ImVec2 semitrans_text_bg_padding = ImVec2(4.f, 2.f);

        ImFont* default_font = nullptr;
    };

    // NOTE: RoR's mouse cursor management is a mess - cursor is hidden/revealed ad-hoc in the code (originally by calling `MyGUI::PointerManager::setVisible()`); this enum+API cleans it up a bit ~ only_a_ptr, 09/2017
    enum class MouseCursorVisibility
    {
        VISIBLE,   //!< Visible, will be auto-hidden if not moving for a while.
        HIDDEN,    //!< Hidden as inactive, will re-appear the moment user moves mouse.
        SUPRESSED, //!< Hidden manually, will not re-appear until explicitly set VISIBLE.
    };

    GUIManager();
    ~GUIManager();


    GUI::CollisionsDebug        CollisionsDebug;
    GUI::GameMainMenu           GameMainMenu;
    GUI::GameAbout              GameAbout;
    GUI::GameSettings           GameSettings;
    GUI::SimActorStats          SimActorStats;
    GUI::SimPerfStats           SimPerfStats;
    GUI::MessageBoxDialog       MessageBoxDialog;
    GUI::MultiplayerSelector    MultiplayerSelector;
    GUI::MainSelector           MainSelector;
    GUI::GameChatBox            ChatBox;
    GUI::VehicleDescription     VehicleDescription;
    GUI::MpClientList           MpClientList;
    GUI::FrictionSettings       FrictionSettings;
    GUI::TextureToolWindow      TextureToolWindow;
    GUI::GameControls           GameControls;
    GUI::RepositorySelector     RepositorySelector;
    GUI::NodeBeamUtils          NodeBeamUtils;
    GUI::LoadingWindow          LoadingWindow;
    GUI::TopMenubar             TopMenubar;
    GUI::ConsoleWindow          ConsoleWindow;
    GUI::SurveyMap              SurveyMap;
    GUI::DirectionArrow         DirectionArrow;
    GUI::VehicleButtons         VehicleButtons;
    GUI::FlexbodyDebug          FlexbodyDebug;
    Ogre::Overlay*              MenuWallpaper = nullptr;

    // GUI manipulation
    void ShowMessageBox(const char* title, const char* text, bool allow_close = true, const char* btn1_text = "OK", const char* btn2_text = nullptr);
    void ShowMessageBox(GUI::MessageBoxConfig const& conf);
    void RequestGuiCaptureKeyboard(bool val); //!< Pass true during frame to prevent input passing to application
    bool IsGuiCaptureKeyboardRequested() const { return m_gui_kb_capture_requested; }
    void ApplyGuiCaptureKeyboard(); //!< Call after rendered frame to apply queued value
    bool AreStaticMenusAllowed(); //!< i.e. top menubar / vehicle UI buttons

    void NewImGuiFrame(float dt);
    void DrawMainMenuGui();
    void DrawSimulationGui(float dt); //!< Touches live data; must be called in sync with sim. thread
    void DrawSimGuiBuffered(GfxActor* player_gfx_actor); //!< Reads data from simbuffer
    void DrawCommonGui();

    void SetGuiHidden(bool visible);
    bool IsGuiHidden() const { return m_hide_gui; }

    void SetSceneManagerForGuiRendering(Ogre::SceneManager* scene_manager);

    void ShutdownMyGUI();
    void SetMouseCursorVisibility(MouseCursorVisibility visi);
    void UpdateMouseCursorVisibility();
    void SupressCursor(bool do_supress);

    void SetUpMenuWallpaper();

    inline OgreImGui& GetImGui() { return m_imgui; }
    inline GuiTheme&  GetTheme() { return m_theme; }

    void WakeUpGUI();

    void UpdateInputEvents(float dt);

private:
    void SetupImGui();

    void eventRequestTag(const MyGUI::UString& _tag, MyGUI::UString& _result);

    MyGUI::Gui*          m_mygui                    = nullptr;
    MyGUI::OgrePlatform* m_mygui_platform           = nullptr;
    bool                 m_hide_gui                 = false;
    OgreImGui            m_imgui;
    GuiTheme             m_theme;
    bool                 m_gui_kb_capture_queued    = false; //!< Resets and accumulates every frame
    bool                 m_gui_kb_capture_requested = false; //!< Effective value, persistent
    Ogre::Timer          m_last_mousemove_time;
    bool                 m_is_cursor_supressed      = false; //!< True if cursor was manually hidden.
};

} // namespace RoR
