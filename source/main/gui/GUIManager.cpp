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

/// @file   GUIManager.h
/// @author based on the basemanager code from mygui common

#include "GUIManager.h"

#include "Application.h"
#include "BeamFactory.h"
#include "ContentManager.h"
#include "GUIUtils.h"
#include "InputEngine.h"
#include "Language.h"
#include "OgreImGui.h"
#include "OgreSubsystem.h"
#include "PlatformUtils.h"
#include "RTTLayer.h"
#include "TerrainManager.h"

//Managed GUI panels
#include "GUI_DebugOptions.h"
#include "GUI_FrictionSettings.h"
#include "GUI_GameMainMenu.h"
#include "GUI_GameAbout.h"
#include "GUI_GameConsole.h"
#include "GUI_GamePauseMenu.h"
#include "GUI_GameChatBox.h"
#include "GUI_GameSettings.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MessageBox.h"
#include "GUI_MultiplayerSelector.h"
#include "GUI_MultiplayerClientList.h"
#include "GUI_MainSelector.h"
#include "GUI_RigSpawnerReportWindow.h"
#include "GUI_SimUtils.h"
#include "GUI_TextureToolWindow.h"
#include "GUI_TopMenubar.h"
#include "GUI_VehicleDescription.h"

#include <MyGUI.h>
#include <MyGUI_OgrePlatform.h>

#define RESOURCE_FILENAME "MyGUI_Core.xml"

namespace RoR {

struct GuiManagerImpl
{
    GuiManagerImpl():
        mygui(nullptr),
        mygui_platform(nullptr)
    {}

    GUI::GameMainMenu           panel_GameMainMenu;
    GUI::GameAbout              panel_GameAbout;
    GUI::GamePauseMenu          panel_GamePauseMenu;
    GUI::GameSettings           panel_GameSettings;
    GUI::DebugOptions           panel_DebugOptions;
    GUI::SimUtils               panel_SimUtils;
    GUI::MessageBoxDialog       panel_MessageBox;
    GUI::MultiplayerSelector    panel_MultiplayerSelector;
    GUI::MainSelector           panel_MainSelector;
    GUI::GameChatBox            panel_ChatBox;
    GUI::ActorSpawnerReportWindow panel_SpawnerReport;
    GUI::VehicleDescription     panel_VehicleDescription;
    GUI::MpClientList           panel_MpClientList;
    GUI::FrictionSettings       panel_FrictionSettings;
    GUI::TextureToolWindow      panel_TextureToolWindow;
    GUI::LoadingWindow          panel_LoadingWindow;
    GUI::TopMenubar             panel_TopMenubar;
    RoR::Console                panel_GameConsole;

    MyGUI::Gui*                 mygui;
    MyGUI::OgrePlatform*        mygui_platform;
};

GUIManager::GuiTheme::GuiTheme():
    in_progress_text_color(1.f, 0.832031f, 0.f, 1.f),
    no_entries_text_color(0.7f, 0.7f, 0.7f, 1.f),
    error_text_color(1.f, 0.175439f, 0.175439f, 1.f),
    selected_entry_text_color(.9f, 0.7f, 0.05f, 1.f)
{
    try
    {
        //Setup custom font
        Str<500> font_path;
        font_path << App::sys_process_dir.GetActive() << PATH_SLASH << "languages" << PATH_SLASH << "Roboto-Medium.ttf";
        ImFontConfig font_config;
        font_config.OversampleH = 1;
        font_config.OversampleV = 1;
        font_config.PixelSnapH = true;
        default_font = ImGui::GetIO().Fonts->AddFontFromFileTTF(font_path, 16, &font_config);
        assert(default_font);
    }
    catch (...)
    {
        LOG("Failed to load font: Roboto-Medium.ttf");
    }

}

void GUIManager::SetVisible_GameMainMenu        (bool v) { m_impl->panel_GameMainMenu       .SetVisible(v); }
void GUIManager::SetVisible_GameAbout           (bool v) { m_impl->panel_GameAbout          .SetVisible(v); }
void GUIManager::SetVisible_DebugOptions        (bool v) { m_impl->panel_DebugOptions       .SetVisible(v); }
void GUIManager::SetVisible_MultiplayerSelector (bool v) { m_impl->panel_MultiplayerSelector.SetVisible(v); }
void GUIManager::SetVisible_ChatBox             (bool v) { m_impl->panel_ChatBox            .SetVisible(v); }
void GUIManager::SetVisible_SpawnerReport       (bool v) { m_impl->panel_SpawnerReport      .SetVisible(v); }
void GUIManager::SetVisible_VehicleDescription  (bool v) { m_impl->panel_VehicleDescription .SetVisible(v); }
void GUIManager::SetVisible_MpClientList        (bool v) { m_impl->panel_MpClientList       .SetVisible(v); }
void GUIManager::SetVisible_FrictionSettings    (bool v) { m_impl->panel_FrictionSettings   .SetVisible(v); }
void GUIManager::SetVisible_TextureToolWindow   (bool v) { m_impl->panel_TextureToolWindow  .SetVisible(v); }
void GUIManager::SetVisible_LoadingWindow       (bool v) { m_impl->panel_LoadingWindow      .SetVisible(v); }
void GUIManager::SetVisible_Console             (bool v) { m_impl->panel_GameConsole        .SetVisible(v); }
void GUIManager::SetVisible_GameSettings        (bool v) { m_impl->panel_GameSettings       .SetVisible(v); }

bool GUIManager::IsVisible_GameMainMenu         () { return m_impl->panel_GameMainMenu       .IsVisible(); }
bool GUIManager::IsVisible_GameAbout            () { return m_impl->panel_GameAbout          .IsVisible(); }
bool GUIManager::IsVisible_DebugOptions         () { return m_impl->panel_DebugOptions       .IsVisible(); }
bool GUIManager::IsVisible_MultiplayerSelector  () { return m_impl->panel_MultiplayerSelector.IsVisible(); }
bool GUIManager::IsVisible_MainSelector         () { return m_impl->panel_MainSelector       .IsVisible(); }
bool GUIManager::IsVisible_ChatBox              () { return m_impl->panel_ChatBox            .IsVisible(); }
bool GUIManager::IsVisible_SpawnerReport        () { return m_impl->panel_SpawnerReport      .IsVisible(); }
bool GUIManager::IsVisible_VehicleDescription   () { return m_impl->panel_VehicleDescription .IsVisible(); }
bool GUIManager::IsVisible_MpClientList         () { return m_impl->panel_MpClientList       .IsVisible(); }
bool GUIManager::IsVisible_FrictionSettings     () { return m_impl->panel_FrictionSettings   .IsVisible(); }
bool GUIManager::IsVisible_TextureToolWindow    () { return m_impl->panel_TextureToolWindow  .IsVisible(); }
bool GUIManager::IsVisible_LoadingWindow        () { return m_impl->panel_LoadingWindow      .IsVisible(); }
bool GUIManager::IsVisible_Console              () { return m_impl->panel_GameConsole        .IsVisible(); }
bool GUIManager::IsVisible_GameSettings         () { return m_impl->panel_GameSettings       .IsVisible(); }

// GUI GetInstance*()
Console*                    GUIManager::GetConsole()           { return &m_impl->panel_GameConsole         ; }
GUI::MainSelector*          GUIManager::GetMainSelector()      { return &m_impl->panel_MainSelector        ; }
GUI::GameMainMenu*          GUIManager::GetMainMenu()          { return &m_impl->panel_GameMainMenu        ; }
GUI::GamePauseMenu*         GUIManager::GetPauseMenu()         { return &m_impl->panel_GamePauseMenu       ; }
GUI::LoadingWindow*         GUIManager::GetLoadingWindow()     { return &m_impl->panel_LoadingWindow       ; }
GUI::MpClientList*          GUIManager::GetMpClientList()      { return &m_impl->panel_MpClientList        ; }
GUI::MultiplayerSelector*   GUIManager::GetMpSelector()        { return &m_impl->panel_MultiplayerSelector ; }
GUI::FrictionSettings*      GUIManager::GetFrictionSettings()  { return &m_impl->panel_FrictionSettings    ; }
GUI::SimUtils*              GUIManager::GetSimUtils()          { return &m_impl->panel_SimUtils            ; }
GUI::TopMenubar*            GUIManager::GetTopMenubar()        { return &m_impl->panel_TopMenubar          ; }

GUIManager::GUIManager() :
    m_renderwindow_closed(false),
    m_impl(nullptr)
{
    RoR::App::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(this);
    OgreBites::WindowEventUtilities::addWindowEventListener(RoR::App::GetOgreSubsystem()->GetRenderWindow(), this);

    std::string gui_logpath = PathCombine(App::sys_logs_dir.GetActive(), "MyGUI.log");
    auto mygui_platform = new MyGUI::OgrePlatform();
    mygui_platform->initialise(
        RoR::App::GetOgreSubsystem()->GetRenderWindow(), 
        gEnv->sceneManager,
        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
        gui_logpath); // use cache resource group so preview images are working
    auto mygui = new MyGUI::Gui();

    // empty init
    mygui->initialise("");

    // add layer factory
    MyGUI::FactoryManager::getInstance().registerFactory<MyGUI::RTTLayer>("Layer");

    // then load the actual config
    MyGUI::ResourceManager::getInstance().load(RESOURCE_FILENAME);

#ifdef NOLANG
    MyGUI::ResourceManager::getInstance().load("MyGUI_FontsEnglish.xml");
#else
    MyGUI::ResourceManager::getInstance().load(LanguageEngine::getSingleton().getMyGUIFontConfigFilename());
#endif // NOLANG

    m_impl = new GuiManagerImpl();
    m_impl->mygui_platform = mygui_platform;
    m_impl->mygui = mygui;
    MyGUI::PointerManager::getInstance().setVisible(false); // RoR is using mouse cursor drawn by DearIMGUI.

#ifdef _WIN32
    MyGUI::LanguageManager::getInstance().eventRequestTag = MyGUI::newDelegate(this, &GUIManager::eventRequestTag);
#endif // _WIN32
    windowResized(RoR::App::GetOgreSubsystem()->GetRenderWindow());

    this->SetupImGui();
}

GUIManager::~GUIManager()
{
    delete m_impl;
}

void GUIManager::UnfocusGui()
{
    MyGUI::InputManager::getInstance().resetKeyFocusWidget();
    MyGUI::InputManager::getInstance().resetMouseCaptureWidget();
}

void GUIManager::ShutdownMyGUI()
{
    if (m_impl->mygui)
    {
        m_impl->mygui->shutdown();
        delete m_impl->mygui;
        m_impl->mygui = nullptr;
    }

    if (m_impl->mygui_platform)
    {
        m_impl->mygui_platform->shutdown();
        delete m_impl->mygui_platform;
        m_impl->mygui_platform = nullptr;
    }

    delete m_impl;
}

bool GUIManager::frameStarted(const Ogre::FrameEvent& evt)
{
    if (m_renderwindow_closed) return false;
    if (!m_impl->mygui) return true;


    // now hide the mouse cursor if not used since a long time
    if (getLastMouseMoveTime() > 5000)
    {
        App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::HIDDEN);
    }

    return true;
}

bool GUIManager::frameEnded(const Ogre::FrameEvent& evt)
{
    return true;
};

void GUIManager::DrawSimulationGui(float dt)
{
    m_impl->panel_SimUtils.FrameStepSimGui(dt);
    if (App::app_state.GetActive() == AppState::SIMULATION)
    {
        m_impl->panel_TopMenubar.Update();

        if (App::sim_state.GetActive() == SimState::PAUSED)
        {
            m_impl->panel_GamePauseMenu.Draw();
        }
    }

    if (m_impl->panel_MessageBox.IsVisible())
    {
        m_impl->panel_MessageBox.Draw();
    }
};

void GUIManager::PushNotification(Ogre::String Title, Ogre::UTFString text)
{
    m_impl->panel_SimUtils.PushNotification(Title, text);
}

void GUIManager::HideNotification()
{
    m_impl->panel_SimUtils.HideNotificationBox();
}

void GUIManager::windowResized(Ogre::RenderWindow* rw)
{
    int width = (int)rw->getWidth();
    int height = (int)rw->getHeight();
    setInputViewSize(width, height);

}

void GUIManager::windowClosed(Ogre::RenderWindow* rw)
{
    m_renderwindow_closed = true;
}

void GUIManager::eventRequestTag(const MyGUI::UString& _tag, MyGUI::UString& _result)
{
    _result = MyGUI::LanguageManager::getInstance().getTag(_tag);
}

Ogre::String GUIManager::getRandomWallpaperImage()
{
    using namespace Ogre;
    FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo("Wallpapers", "*.jpg", false);
    if (files.isNull() || files->empty())
    {
        files = ResourceGroupManager::getSingleton().findResourceFileInfo("Wallpapers", "*.png", false);
        if (files.isNull() || files->empty())
            return "";
    }
    srand ( time(NULL) );

    int num = 0;
    for (int i = 0; i<Math::RangeRandom(0, 10); i++)
        num = Math::RangeRandom(0, files->size());

    return files->at(num).filename;
}

void GUIManager::SetSceneManagerForGuiRendering(Ogre::SceneManager* scene_manager)
{
    m_impl->mygui_platform->getRenderManagerPtr()->setSceneManager(scene_manager);
}

void GUIManager::UpdateSimUtils(float dt, Actor *truck)
{
    if (m_impl->panel_SimUtils.IsBaseVisible()) //Better to update only when it's visible.
    {
        m_impl->panel_SimUtils.UpdateStats(dt, truck);
    }
}

void GUIManager::AddRigLoadingReport(std::string const & vehicle_name, std::string const & text, int num_errors, int num_warnings, int num_other)
{
    m_impl->panel_SpawnerReport.SetRigLoadingReport(vehicle_name, text, num_errors, num_warnings, num_other);
}

void GUIManager::CenterSpawnerReportWindow()
{
    m_impl->panel_SpawnerReport.CenterToScreen();
}

void GUIManager::pushMessageChatBox(Ogre::String txt)
{
    m_impl->panel_ChatBox.pushMsg(txt);
}

void GUIManager::hideGUI(bool hidden)
{
    if (hidden)
    {
        m_impl->panel_SimUtils.HideNotificationBox();
        m_impl->panel_SimUtils.SetFPSBoxVisible(false);
        m_impl->panel_SimUtils.SetActorInfoBoxVisible(false);
        m_impl->panel_ChatBox.Hide();
    }
    m_impl->panel_SimUtils.DisableNotifications(hidden);
}

void GUIManager::FrictionSettingsUpdateCollisions()
{
    App::GetGuiManager()->GetFrictionSettings()->setCollisions(gEnv->collisions);
}

void GUIManager::SetMouseCursorVisibility(MouseCursorVisibility visi)
{
    switch (visi)
    {
    case MouseCursorVisibility::VISIBLE:
        ImGui::GetIO().MouseDrawCursor = true;
        this->SupressCursor(false);
        return;

    case MouseCursorVisibility::HIDDEN:
        ImGui::GetIO().MouseDrawCursor = false;
        return;

    case MouseCursorVisibility::SUPRESSED:
        ImGui::GetIO().MouseDrawCursor = false;
        this->SupressCursor(true);
        return;
    }
}

void GUIManager::ReflectGameState()
{
    const auto app_state = App::app_state.GetActive();
    const auto mp_state  = App::mp_state.GetActive();
    if (app_state == AppState::MAIN_MENU)
    {
        m_impl->panel_GameMainMenu       .SetVisible(!m_impl->panel_MainSelector.IsVisible());

        m_impl->panel_ChatBox            .SetVisible(false);
        m_impl->panel_DebugOptions       .SetVisible(false);
        m_impl->panel_FrictionSettings   .SetVisible(false);
        m_impl->panel_TextureToolWindow  .SetVisible(false);
        m_impl->panel_VehicleDescription .SetVisible(false);
        m_impl->panel_SpawnerReport      .SetVisible(false);
        m_impl->panel_SimUtils           .SetBaseVisible(false);
        m_impl->panel_MpClientList       .SetVisible(mp_state == MpState::CONNECTED);
        return;
    }
    if (app_state == AppState::SIMULATION)
    {
        m_impl->panel_SimUtils           .SetBaseVisible(true);
        m_impl->panel_GameMainMenu       .SetVisible(false);
        return;
    }
}

void GUIManager::NewImGuiFrame(float dt)
{
    // Update screen size
    int left, top, width, height;
    gEnv->mainCamera->getViewport()->getActualDimensions(left, top, width, height); // output params

     // Read keyboard modifiers inputs
    OIS::Keyboard* kb = App::GetInputEngine()->GetOisKeyboard();
    bool ctrl  = kb->isKeyDown(OIS::KC_LCONTROL);
    bool shift = kb->isKeyDown(OIS::KC_LSHIFT);
    bool alt   = kb->isKeyDown(OIS::KC_LMENU);

    // Call IMGUI
    m_imgui.NewFrame(dt, static_cast<float>(width), static_cast<float>(height), ctrl, alt, shift);
}

void GUIManager::SetupImGui()
{
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // Disable 'imgui.ini' - we don't need to persist window positions.

    m_imgui.Init(gEnv->sceneManager);
    // Colors
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.05f, 0.05f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.78f, 0.39f, 0.00f, 0.99f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.90f, 0.65f, 0.65f, 0.98f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.57f, 0.31f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.40f, 0.40f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.74f, 0.44f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.16f, 0.16f, 0.16f, 0.99f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.30f, 0.30f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.78f, 0.39f, 0.00f, 0.99f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(1.00f, 0.50f, 0.00f, 0.99f);
    style.Colors[ImGuiCol_ComboBg]               = ImVec4(0.20f, 0.20f, 0.20f, 0.99f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.00f, 0.48f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.26f, 0.26f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.78f, 0.39f, 0.00f, 0.98f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(1.00f, 0.48f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.57f, 0.30f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.78f, 0.39f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_Column]                = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.70f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.90f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.22f, 0.22f, 0.21f, 1.00f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.78f, 0.39f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.00f, 0.48f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_CloseButton]           = ImVec4(0.55f, 0.27f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.86f, 0.43f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(1.00f, 0.48f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.00f, 0.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    // Styles
    style.WindowPadding         = ImVec2(10.f, 10.f);
    style.FrameRounding         = 2.f;
    style.WindowRounding        = 4.f;
    style.WindowTitleAlign      = ImVec2(0.5f, 0.5f);
    style.ItemSpacing           = ImVec2(5.f, 5.f);
    style.GrabRounding          = 3.f;
    style.ChildWindowRounding   = 4.f;
}

void GUIManager::DrawMainMenuGui()
{
    if (m_impl->panel_MultiplayerSelector.IsVisible())
    {
        m_impl->panel_MultiplayerSelector.Draw();
    }

    if (m_impl->panel_GameMainMenu.IsVisible())
    {
        m_impl->panel_GameMainMenu.Draw();
    }

    if ((App::mp_state.GetActive() != MpState::CONNECTED) && (App::mp_state.GetPending() == MpState::CONNECTED))
    {
        this->DrawMpConnectingStatusBox();
    }

    if (m_impl->panel_GameSettings.IsVisible())
    {
        m_impl->panel_GameSettings.Draw();
    }

    if (m_impl->panel_MessageBox.IsVisible())
    {
        m_impl->panel_MessageBox.Draw();
    }
}

void GUIManager::DrawMpConnectingStatusBox()
{
    static float spin_counter=0.f;

    const ImVec2 spin_size(20.f, 20.f);
    const float spin_column_w(50.f);
    const int win_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoInputs
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;

    ImGui::SetNextWindowPosCenter();
    ImGui::Begin("Connecting to MP server...", nullptr, win_flags);
    ImGui::Columns(2);
    ImGui::SetColumnOffset(1, spin_column_w);

    ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(5.f, 7.f)); // NOTE: Hand aligned; I failed calculate the positioning here ~ only_a_ptr, 10/2017
    DrawImGuiSpinner(spin_counter, spin_size);

    ImGui::NextColumn();
    // HACK: The trailing space is a workaround for a scissoring issue in OGRE/DearIMGUI integration. ~ only_a_ptr, 10/2017
    ImGui::Text("Joining [%s:%d] ", App::mp_server_host.GetActive(), App::mp_server_port.GetActive());
#ifdef USE_SOCKETW
    ImGui::TextDisabled("%s", Networking::GetStatusMessage().GetBuffer());
#endif
    ImGui::End();
}

void GUIManager::ShowMessageBox(const char* title, const char* text, bool allow_close, const char* btn1_text, const char* btn2_text)
{
    m_impl->panel_MessageBox.Show(title, text, allow_close, btn1_text, btn2_text);
}

} // namespace RoR
