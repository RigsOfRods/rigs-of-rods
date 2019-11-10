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
/// @author based on the basemanager code from mygui common

#pragma once

#include "GUIInputManager.h"
#include "OgreImGui.h"
#include "RoRPrerequisites.h"

#include <Bites/OgreWindowEventUtilities.h>
#include <OgreFrameListener.h>
#include <MyGUI_UString.h>

namespace RoR {

class GUIManager :
    public GUIInputManager
    , public Ogre::FrameListener
    , public Ogre::WindowEventListener
    , public ZeroedMemoryAllocator
{
public:

    struct GuiTheme
    {
        GuiTheme();

        ImVec4 in_progress_text_color;
        ImVec4 no_entries_text_color;
        ImVec4 error_text_color;
        ImVec4 selected_entry_text_color;
        ImVec4 highlight_text_color;
        ImVec4 value_red_text_color;
        ImVec4 value_blue_text_color;
        ImVec4 success_text_color;
        ImVec4 warning_text_color;
        ImVec4 help_text_color;

        ImFont* default_font;
    };

    // NOTE: RoR's mouse cursor management is a mess - cursor is hidden/revealed ad-hoc in the code (originally by calling `MyGUI::PointerManager::setVisible()`); this enum+API cleans it up a bit ~ only_a_ptr, 09/2017
    enum class MouseCursorVisibility
    {
        VISIBLE,   ///< Visible, will be auto-hidden if not moving for a while.
        HIDDEN,    ///< Hidden as inactive, will re-appear the moment user moves mouse.
        SUPRESSED, ///< Hidden manually, will not re-appear until explicitly set VISIBLE.
    };

    GUIManager();
    ~GUIManager();

    // GUI SetVisible*()
    void SetVisible_GameMainMenu        (bool visible);
    void SetVisible_GameAbout           (bool visible);
    void SetVisible_GameSettings        (bool visible);
    void SetVisible_MultiplayerSelector (bool visible);
    void SetVisible_ChatBox             (bool visible);
    void SetVisible_VehicleDescription  (bool visible);
    void SetVisible_MpClientList        (bool visible);
    void SetVisible_FrictionSettings    (bool visible);
    void SetVisible_TextureToolWindow   (bool visible);
    void SetVisible_NodeBeamUtils       (bool visible);
    void SetVisible_LoadingWindow       (bool visible);
    void SetVisible_Console             (bool visible);
    void SetVisible_SimActorStats       (bool visible);

    // GUI IsVisible*()
    bool IsVisible_GameMainMenu         ();
    bool IsVisible_GameAbout            ();
    bool IsVisible_GameSettings         ();
    bool IsVisible_TopMenubar           ();
    bool IsVisible_MessageBox           ();
    bool IsVisible_MultiplayerSelector  ();
    bool IsVisible_MpClientList         ();
    bool IsVisible_MainSelector         ();
    bool IsVisible_ChatBox              ();
    bool IsVisible_VehicleDescription   ();
    bool IsVisible_FrictionSettings     ();
    bool IsVisible_TextureToolWindow    ();
    bool IsVisible_NodeBeamUtils        ();
    bool IsVisible_LoadingWindow        ();
    bool IsVisible_Console              ();
    bool IsVisible_SimActorStats        ();

    // GUI GetInstance*()
    GUI::MainSelector* GetMainSelector();
    GUI::GameMainMenu* GetMainMenu();
    GUI::GamePauseMenu* GetPauseMenu();
    GUI::LoadingWindow* GetLoadingWindow();
    GUI::MpClientList* GetMpClientList();
    GUI::MultiplayerSelector* GetMpSelector();
    GUI::FrictionSettings* GetFrictionSettings();
    GUI::SimUtils* GetSimUtils();
    GUI::TopMenubar* GetTopMenubar();

    // GUI manipulation
    void pushMessageChatBox(Ogre::String txt);
    void ShowMessageBox(const char* title, const char* text, bool allow_close = true, const char* btn1_text = "OK", const char* btn2_text = nullptr);
    void UnfocusGui();
    void PushNotification(Ogre::String Title, Ogre::UTFString text);
    void HideNotification();

    void UpdateSimUtils(float dt, Actor* truck);
    void NewImGuiFrame(float dt);
    void DrawMainMenuGui();
    void DrawSimulationGui(float dt); //!< Touches live data; must be called in sync with sim. thread
    void DrawSimGuiBuffered(GfxActor* player_gfx_actor); //!< Reads data from simbuffer

    void SetMpConnectingStatusMsg(std::string const & msg) { m_net_connect_status = msg; }
    void DrawMpConnectingStatusBox();
    void hideGUI(bool visible);

    void windowResized(Ogre::RenderWindow* rw);

    void SetSceneManagerForGuiRendering(Ogre::SceneManager* scene_manager);

    void ShutdownMyGUI();
    void ReflectGameState();
    void SetMouseCursorVisibility(MouseCursorVisibility visi);

    static Ogre::String getRandomWallpaperImage();

    inline OgreImGui& GetImGui() { return m_imgui; }
    inline GuiTheme&  GetTheme() { return m_theme; }


private:
    void SetupImGui();

    virtual bool frameStarted(const Ogre::FrameEvent& _evt);
    virtual bool frameEnded(const Ogre::FrameEvent& _evt);
    virtual void windowClosed(Ogre::RenderWindow* rw);

    void eventRequestTag(const MyGUI::UString& _tag, MyGUI::UString& _result);

    GuiManagerImpl*    m_impl;
    bool               m_renderwindow_closed;
    OgreImGui          m_imgui;
    GuiTheme           m_theme;
    std::string        m_net_connect_status;
};

} // namespace RoR
