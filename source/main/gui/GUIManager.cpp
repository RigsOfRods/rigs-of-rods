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
#include "GUI_ConsoleWindow.h"
#include "GUI_FrictionSettings.h"
#include "GUI_GameMainMenu.h"
#include "GUI_GameAbout.h"
#include "GUI_GamePauseMenu.h"
#include "GUI_GameChatBox.h"
#include "GUI_GameSettings.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MessageBox.h"
#include "GUI_MultiplayerSelector.h"
#include "GUI_MultiplayerClientList.h"
#include "GUI_MainSelector.h"
#include "GUI_NodeBeamUtils.h"
#include "GUI_SimActorStats.h"
#include "GUI_SimPerfStats.h"
#include "GUI_SurveyMap.h"
#include "GUI_TextureToolWindow.h"
#include "GUI_GameControls.h"
#include "GUI_TopMenubar.h"
#include "GUI_VehicleDescription.h"

#include <MyGUI.h>
#include <MyGUI_OgrePlatform.h>
#include <OgreOverlay.h>
#include <OgreOverlayElement.h>
#include <OgreOverlayContainer.h>

#define RESOURCE_FILENAME "MyGUI_Core.xml"

namespace RoR {

struct GuiManagerImpl
{
    GUI::GameMainMenu           panel_GameMainMenu;
    GUI::GameAbout              panel_GameAbout;
    GUI::GamePauseMenu          panel_GamePauseMenu;
    GUI::GameSettings           panel_GameSettings;
    GUI::SimActorStats          panel_SimActorStats;
    GUI::SimPerfStats           panel_SimPerfStats;
    GUI::MessageBoxDialog       panel_MessageBox;
    GUI::MultiplayerSelector    panel_MultiplayerSelector;
    GUI::MainSelector           panel_MainSelector;
    GUI::GameChatBox            panel_ChatBox;
    GUI::VehicleDescription     panel_VehicleDescription;
    GUI::MpClientList           panel_MpClientList;
    GUI::FrictionSettings       panel_FrictionSettings;
    GUI::TextureToolWindow      panel_TextureToolWindow;
    GUI::GameControls           panel_GameControls;
    GUI::NodeBeamUtils          panel_NodeBeamUtils;
    GUI::LoadingWindow          panel_LoadingWindow;
    GUI::TopMenubar             panel_TopMenubar;
    GUI::ConsoleWindow          panel_ConsoleWindow;
    GUI::SurveyMap              panel_SurveyMap;
    Ogre::Overlay*              overlay_Wallpaper = nullptr;

    MyGUI::Gui*                 mygui = nullptr;
    MyGUI::OgrePlatform*        mygui_platform = nullptr;
};

void GUIManager::SetVisible_GameMainMenu        (bool v) { m_impl->panel_GameMainMenu       .SetVisible(v); }
void GUIManager::SetVisible_GameAbout           (bool v) { m_impl->panel_GameAbout          .SetVisible(v); }
void GUIManager::SetVisible_MultiplayerSelector (bool v) { m_impl->panel_MultiplayerSelector.SetVisible(v); }
void GUIManager::SetVisible_ChatBox             (bool v) { m_impl->panel_ChatBox            .SetVisible(v); }
void GUIManager::SetVisible_VehicleDescription  (bool v) { m_impl->panel_VehicleDescription .SetVisible(v); }
void GUIManager::SetVisible_FrictionSettings    (bool v) { m_impl->panel_FrictionSettings   .SetVisible(v); }
void GUIManager::SetVisible_TextureToolWindow   (bool v) { m_impl->panel_TextureToolWindow  .SetVisible(v); }
void GUIManager::SetVisible_GameControls        (bool v) { m_impl->panel_GameControls       .SetVisible(v); }
void GUIManager::SetVisible_LoadingWindow       (bool v) { m_impl->panel_LoadingWindow      .SetVisible(v); }
void GUIManager::SetVisible_Console             (bool v) { m_impl->panel_ConsoleWindow      .SetVisible(v); }
void GUIManager::SetVisible_GameSettings        (bool v) { m_impl->panel_GameSettings       .SetVisible(v); }
void GUIManager::SetVisible_NodeBeamUtils       (bool v) { m_impl->panel_NodeBeamUtils      .SetVisible(v); }
void GUIManager::SetVisible_SimActorStats       (bool v) { m_impl->panel_SimActorStats      .SetVisible(v); }
void GUIManager::SetVisible_SimPerfStats        (bool v) { m_impl->panel_SimPerfStats       .SetVisible(v); }

bool GUIManager::IsVisible_GameMainMenu         () { return m_impl->panel_GameMainMenu       .IsVisible(); }
bool GUIManager::IsVisible_GameAbout            () { return m_impl->panel_GameAbout          .IsVisible(); }
bool GUIManager::IsVisible_MultiplayerSelector  () { return m_impl->panel_MultiplayerSelector.IsVisible(); }
bool GUIManager::IsVisible_MainSelector         () { return m_impl->panel_MainSelector       .IsVisible(); }
bool GUIManager::IsVisible_ChatBox              () { return m_impl->panel_ChatBox            .IsVisible(); }
bool GUIManager::IsVisible_VehicleDescription   () { return m_impl->panel_VehicleDescription .IsVisible(); }
bool GUIManager::IsVisible_FrictionSettings     () { return m_impl->panel_FrictionSettings   .IsVisible(); }
bool GUIManager::IsVisible_TextureToolWindow    () { return m_impl->panel_TextureToolWindow  .IsVisible(); }
bool GUIManager::IsVisible_GameControls         () { return m_impl->panel_GameControls       .IsVisible(); }
bool GUIManager::IsVisible_LoadingWindow        () { return m_impl->panel_LoadingWindow      .IsVisible(); }
bool GUIManager::IsVisible_Console              () { return m_impl->panel_ConsoleWindow      .IsVisible(); }
bool GUIManager::IsVisible_GameSettings         () { return m_impl->panel_GameSettings       .IsVisible(); }
bool GUIManager::IsVisible_TopMenubar           () { return m_impl->panel_TopMenubar         .IsVisible(); }
bool GUIManager::IsVisible_NodeBeamUtils        () { return m_impl->panel_NodeBeamUtils      .IsVisible(); }
bool GUIManager::IsVisible_SimActorStats        () { return m_impl->panel_SimActorStats      .IsVisible(); }
bool GUIManager::IsVisible_SimPerfStats         () { return m_impl->panel_SimPerfStats       .IsVisible(); }
bool GUIManager::IsVisible_SurveyMap            () { return m_impl->panel_SurveyMap          .IsVisible(); }

// GUI GetInstance*()
GUI::MainSelector*          GUIManager::GetMainSelector()      { return &m_impl->panel_MainSelector        ; }
GUI::GameMainMenu*          GUIManager::GetMainMenu()          { return &m_impl->panel_GameMainMenu        ; }
GUI::GamePauseMenu*         GUIManager::GetPauseMenu()         { return &m_impl->panel_GamePauseMenu       ; }
GUI::LoadingWindow*         GUIManager::GetLoadingWindow()     { return &m_impl->panel_LoadingWindow       ; }
GUI::MultiplayerSelector*   GUIManager::GetMpSelector()        { return &m_impl->panel_MultiplayerSelector ; }
GUI::FrictionSettings*      GUIManager::GetFrictionSettings()  { return &m_impl->panel_FrictionSettings    ; }
GUI::TopMenubar*            GUIManager::GetTopMenubar()        { return &m_impl->panel_TopMenubar          ; }
GUI::SurveyMap*             GUIManager::GetSurveyMap()         { return &m_impl->panel_SurveyMap           ; }
GUI::SimActorStats*         GUIManager::GetSimActorStats()     { return &m_impl->panel_SimActorStats       ; }

GUIManager::GUIManager() :
    m_renderwindow_closed(false),
    m_impl(nullptr)
{
    RoR::App::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(this);
    OgreBites::WindowEventUtilities::addWindowEventListener(RoR::App::GetOgreSubsystem()->GetRenderWindow(), this);

    std::string gui_logpath = PathCombine(App::sys_logs_dir->GetActiveStr(), "MyGUI.log");
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

    MyGUI::ResourceManager::getInstance().load("MyGUI_FontsEnglish.xml");

    m_impl = new GuiManagerImpl();
    m_impl->mygui_platform = mygui_platform;
    m_impl->mygui = mygui;
    MyGUI::PointerManager::getInstance().setVisible(false); // RoR is using mouse cursor drawn by DearIMGUI.

#ifdef _WIN32
    MyGUI::LanguageManager::getInstance().eventRequestTag = MyGUI::newDelegate(this, &GUIManager::eventRequestTag);
#endif // _WIN32
    windowResized(RoR::App::GetOgreSubsystem()->GetRenderWindow());

    this->SetupImGui();

    // Configure the chatbox console view
    m_impl->panel_ChatBox.GetConsoleView().cvw_background_color   = m_theme.semitrans_text_bg_color;
    m_impl->panel_ChatBox.GetConsoleView().cvw_background_padding = m_theme.semitrans_text_bg_padding;
}

GUIManager::~GUIManager()
{
    delete m_impl;
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
    m_gui_kb_capture_requested = m_gui_kb_capture_queued;
    return true;
};

void GUIManager::DrawSimulationGui(float dt)
{
    if (App::app_state->GetActiveEnum<AppState>() == AppState::SIMULATION)
    {
        m_impl->panel_TopMenubar.Update();

        if (App::sim_state->GetActiveEnum<SimState>() == SimState::PAUSED)
        {
            m_impl->panel_GamePauseMenu.Draw();
        }
    }

    if (m_impl->panel_NodeBeamUtils.IsVisible())
    {
        m_impl->panel_NodeBeamUtils.Draw();
    }

    if (m_impl->panel_MessageBox.IsVisible())
    {
        m_impl->panel_MessageBox.Draw();
    }
};

void GUIManager::DrawSimGuiBuffered(GfxActor* player_gfx_actor)
{
    this->DrawCommonGui();

    if (player_gfx_actor && this->IsVisible_SimActorStats())
    {
        m_impl->panel_SimActorStats.Draw(player_gfx_actor);
    }

    if (this->IsVisible_Console())
    {
        m_impl->panel_ConsoleWindow.Draw();
    }
    else
    {
        if (!m_hide_gui)
        {
            m_impl->panel_ChatBox.Draw(); // Messages must be always visible
        }
    }

    if (this->IsVisible_LoadingWindow())
    {
        m_impl->panel_LoadingWindow.Draw();
    }

    if (this->IsVisible_FrictionSettings())
    {
        m_impl->panel_FrictionSettings.Draw();
    }

    if (this->IsVisible_VehicleDescription())
    {
        m_impl->panel_VehicleDescription.Draw();
    }

    if (this->IsVisible_SimPerfStats())
    {
        m_impl->panel_SimPerfStats.Draw();
    }

    if (this->IsVisible_TextureToolWindow())
    {
        m_impl->panel_TextureToolWindow.Draw();
    }

    if (this->IsVisible_GameControls())
    {
        m_impl->panel_GameControls.Draw();
    }

    if (this->IsVisible_SurveyMap())
    {
        m_impl->panel_SurveyMap.Draw();
    }
}

void GUIManager::windowClosed(Ogre::RenderWindow* rw)
{
    m_renderwindow_closed = true;
}

void GUIManager::eventRequestTag(const MyGUI::UString& _tag, MyGUI::UString& _result)
{
    _result = MyGUI::LanguageManager::getInstance().getTag(_tag);
}

void GUIManager::SetUpMenuWallpaper()
{
    // Determine image filename
    using namespace Ogre;
    FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo("Wallpapers", "*.jpg", false);
    if (files.isNull() || files->empty())
    {
        files = ResourceGroupManager::getSingleton().findResourceFileInfo("Wallpapers", "*.png", false);
    }
    srand ( time(NULL) );

    int num = 0;
    for (int i = 0; i<Math::RangeRandom(0, 10); i++)
        num = Math::RangeRandom(0, files->size());

    // Set up wallpaper
    // ...texture...
    Ogre::ResourceManager::ResourceCreateOrRetrieveResult wp_tex_result
        = Ogre::TextureManager::getSingleton().createOrRetrieve(files->at(num).filename, "Wallpapers");
    Ogre::TexturePtr wp_tex = wp_tex_result.first.staticCast<Ogre::Texture>();
    // ...material...
    Ogre::MaterialPtr wp_mat = Ogre::MaterialManager::getSingleton().create("rigsofrods/WallpaperMat", Ogre::RGN_DEFAULT);
    Ogre::TextureUnitState* wp_tus = wp_mat->getTechnique(0)->getPass(0)->createTextureUnitState();
    wp_tus->setTexture(wp_tex);
    wp_mat->compile();
    // ...panel...
    Ogre::OverlayElement* wp_panel = Ogre::OverlayManager::getSingleton()
        .createOverlayElement("Panel", "rigsofrods/WallpaperPanel", /*isTemplate=*/false);
    wp_panel->setMaterial(wp_mat);
    wp_panel->setDimensions(1.f, 1.f);
    // ...overlay...
    m_impl->overlay_Wallpaper = Ogre::OverlayManager::getSingleton().create("rigsofrods/WallpaperOverlay");
    m_impl->overlay_Wallpaper->add2D(static_cast<Ogre::OverlayContainer*>(wp_panel)); // 'Panel' inherits from 'Container'
    m_impl->overlay_Wallpaper->setZOrder(0);
    m_impl->overlay_Wallpaper->show();
}

void GUIManager::SetSceneManagerForGuiRendering(Ogre::SceneManager* scene_manager)
{
    m_impl->mygui_platform->getRenderManagerPtr()->setSceneManager(scene_manager);
}

void GUIManager::hideGUI(bool hidden)
{
    m_hide_gui = hidden;
    if (hidden)
    {
        m_impl->panel_SimPerfStats.SetVisible(false);
        m_impl->panel_ChatBox.SetVisible(false);
    }
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
    if (App::app_state->GetActiveEnum<AppState>() == AppState::MAIN_MENU)
    {
        m_impl->overlay_Wallpaper       ->show();
        m_impl->panel_GameMainMenu       .SetVisible(!m_impl->panel_MainSelector.IsVisible());

        m_impl->panel_ChatBox            .SetVisible(false);
        m_impl->panel_FrictionSettings   .SetVisible(false);
        m_impl->panel_TextureToolWindow  .SetVisible(false);
        m_impl->panel_GameControls       .SetVisible(false);
        m_impl->panel_VehicleDescription .SetVisible(false);
        m_impl->panel_SimActorStats      .SetVisible(false);
        m_impl->panel_SimPerfStats       .SetVisible(false);
    }
    else if (App::app_state->GetActiveEnum<AppState>() == AppState::SIMULATION)
    {
        m_impl->panel_GameMainMenu       .SetVisible(false);
        m_impl->overlay_Wallpaper       ->hide();
    }
}

void GUIManager::NewImGuiFrame(float dt)
{
    ImGuiIO& io = ImGui::GetIO();
    OIS::Keyboard* kb = App::GetInputEngine()->GetOisKeyboard();

     // Read keyboard modifiers inputs
    io.KeyCtrl = kb->isKeyDown(OIS::KC_LCONTROL);
    io.KeyShift = kb->isKeyDown(OIS::KC_LSHIFT);
    io.KeyAlt = kb->isKeyDown(OIS::KC_LMENU);
    io.KeySuper = false;

    // Call IMGUI
    Ogre::FrameEvent ev;
    ev.timeSinceLastFrame = dt;
    Ogre::ImGuiOverlay::NewFrame(ev);

    // Reset state
    m_gui_kb_capture_queued = false;
}

void GUIManager::SetupImGui()
{
    m_imgui.Init();
    // Colors
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.06f, 0.06f, 0.06f, 0.90f);
    style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.1f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.05f, 0.05f, 0.10f, 0.90f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.20f, 0.20f, 0.20f, 0.90f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.90f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.20f, 0.20f, 0.90f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.13f, 0.40f, 0.60f, 0.90f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.18f, 0.53f, 0.79f, 0.90f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.11f, 0.33f, 0.49f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.13f, 0.40f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.13f, 0.40f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.20f, 0.20f, 0.20f, 0.90f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.16f, 0.16f, 0.16f, 0.90f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.30f, 0.30f, 0.29f, 0.90f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.13f, 0.40f, 0.60f, 0.90f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.18f, 0.53f, 0.79f, 0.90f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.90f, 0.90f, 0.90f, 0.90f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.39f, 0.39f, 0.39f, 0.90f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.18f, 0.53f, 0.79f, 0.90f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.26f, 0.26f, 0.25f, 0.90f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.13f, 0.40f, 0.60f, 0.90f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.18f, 0.53f, 0.79f, 0.90f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.20f, 0.20f, 0.20f, 0.90f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.13f, 0.40f, 0.60f, 0.90f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.18f, 0.53f, 0.79f, 0.90f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.22f, 0.22f, 0.21f, 0.90f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.13f, 0.40f, 0.60f, 0.90f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.18f, 0.53f, 0.79f, 0.90f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.13f, 0.40f, 0.60f, 0.90f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.18f, 0.53f, 0.79f, 0.90f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.13f, 0.40f, 0.60f, 0.90f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.00f, 0.00f, 1.00f, 0.90f);
    style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.20f, 0.20f, 0.20f, 0.90f);
    // Styles
    style.WindowPadding         = ImVec2(10.f, 10.f);
    style.FrameRounding         = 2.f;
    style.WindowRounding        = 4.f;
    style.WindowTitleAlign      = ImVec2(0.5f, 0.5f);
    style.ItemSpacing           = ImVec2(5.f, 5.f);
    style.GrabRounding          = 3.f;
    style.WindowBorderSize      = 0.f;
}

void GUIManager::DrawCommonGui()
{
    switch (App::mp_state->GetActiveEnum<MpState>())
    {
        case MpState::CONNECTING:
            this->DrawMpConnectingStatusBox();
            break;

        case MpState::CONNECTED:
            if (!m_hide_gui)
            {
                m_impl->panel_MpClientList.Draw();
            }
            break;

        default:
            break;
    }

    if (m_impl->panel_MainSelector.IsVisible())
    {
        m_impl->panel_MainSelector.Draw();
    }
}

void GUIManager::DrawMainMenuGui()
{
    this->DrawCommonGui();

    if (m_impl->panel_MultiplayerSelector.IsVisible())
    {
        m_impl->panel_MultiplayerSelector.Draw();
    }

    if (m_impl->panel_GameMainMenu.IsVisible())
    {
        m_impl->panel_GameMainMenu.Draw();
    }

    if (m_impl->panel_GameSettings.IsVisible())
    {
        m_impl->panel_GameSettings.Draw();
    }

    if (m_impl->panel_MessageBox.IsVisible())
    {
        m_impl->panel_MessageBox.Draw();
    }

    if (m_impl->panel_LoadingWindow.IsVisible())
    {
        m_impl->panel_LoadingWindow.Draw();
    }

    if (m_impl->panel_GameAbout.IsVisible())
    {
        m_impl->panel_GameAbout.Draw();
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
    ImGui::Text("Joining [%s:%d] ", App::mp_server_host->GetActiveStr().c_str(), App::mp_server_port->GetActiveVal<int>());
#ifdef USE_SOCKETW
    ImGui::TextDisabled("%s", m_net_connect_status.c_str());
#endif
    ImGui::End();
}

void GUIManager::ShowMessageBox(const char* title, const char* text, bool allow_close, const char* btn1_text, const char* btn2_text)
{
    m_impl->panel_MessageBox.Show(title, text, allow_close, btn1_text, btn2_text);
}

void GUIManager::RequestGuiCaptureKeyboard(bool val) 
{ 
    m_gui_kb_capture_queued = m_gui_kb_capture_queued || val;
}

} // namespace RoR
