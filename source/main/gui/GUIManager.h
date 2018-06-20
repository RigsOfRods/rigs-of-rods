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
    void SetVisible_GamePauseMenu       (bool visible);
    void SetVisible_GameSettings        (bool visible);
    void SetVisible_DebugOptions        (bool visible);
    void SetVisible_MultiplayerSelector (bool visible);
    void SetVisible_ChatBox             (bool visible);
    void SetVisible_SpawnerReport       (bool visible);
    void SetVisible_VehicleDescription  (bool visible);
    void SetVisible_MpClientList        (bool visible);
    void SetVisible_FrictionSettings    (bool visible);
    void SetVisible_TextureToolWindow   (bool visible);
    void SetVisible_TeleportWindow      (bool visible);
    void SetVisible_LoadingWindow       (bool visible);
    void SetVisible_TopMenubar          (bool visible);
    void SetVisible_Console             (bool visible);

    // GUI IsVisible*()
    bool IsVisible_GameMainMenu         ();
    bool IsVisible_GameAbout            ();
    bool IsVisible_GamePauseMenu        ();
    bool IsVisible_GameSettings         ();
    bool IsVisible_DebugOptions         ();
    bool IsVisible_MessageBox           ();
    bool IsVisible_MultiplayerSelector  ();
    bool IsVisible_MpClientList         ();
    bool IsVisible_MainSelector         ();
    bool IsVisible_ChatBox              ();
    bool IsVisible_SpawnerReport        ();
    bool IsVisible_VehicleDescription   ();
    bool IsVisible_FrictionSettings     ();
    bool IsVisible_TextureToolWindow    ();
    bool IsVisible_TeleportWindow       ();
    bool IsVisible_LoadingWindow        ();
    bool IsVisible_TopMenubar           ();
    bool IsVisible_Console              ();

    // GUI GetInstance*()
    Console* GetConsole();
    GUI::MainSelector* GetMainSelector();
    GUI::LoadingWindow* GetLoadingWindow();
    GUI::MpClientList* GetMpClientList();
    GUI::MultiplayerSelector* GetMpSelector();
    GUI::FrictionSettings* GetFrictionSettings();
    GUI::SimUtils* GetSimUtils();
    GUI::TopMenubar* GetTopMenubar();
    GUI::TeleportWindow* GetTeleport();

    // GUI manipulation
    void pushMessageChatBox(Ogre::String txt);
    void ShowMessageBox(Ogre::String mTitle, Ogre::String mText, bool button1, Ogre::String mButton1, bool AllowClose, bool button2, Ogre::String mButton2);
    void UpdateMessageBox(Ogre::String mTitle, Ogre::String mText, bool button1, Ogre::String mButton1, bool AllowClose, bool button2, Ogre::String mButton2, bool IsVisible);
    void UnfocusGui();
    void PushNotification(Ogre::String Title, Ogre::UTFString text);
    void HideNotification();
    void CenterSpawnerReportWindow();
    void AdjustPauseMenuPosition();
    void AdjustMainMenuPosition();

    void UpdateSimUtils(float dt, Actor* truck);
    void FrameStepGui(float dt);
    void NewImGuiFrame(float dt);
    void DrawMainMenuGui();

    int getMessageBoxResult(); //TODO
    void DrawMpConnectingStatusBox();
    void InitMainSelector(RoR::SkinManager* skin_manager);

    void hideGUI(bool visible);

    void destroy();

    void windowResized(Ogre::RenderWindow* rw);

    void SetSceneManagerForGuiRendering(Ogre::SceneManager* scene_manager);

    void FrictionSettingsUpdateCollisions();
    void ShutdownMyGUI();
    void ReflectGameState();
    void SetMouseCursorVisibility(MouseCursorVisibility visi);

    virtual void AddRigLoadingReport(std::string const& vehicle_name, std::string const& text, int num_errors, int num_warnings, int num_other);

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
};

} // namespace RoR
