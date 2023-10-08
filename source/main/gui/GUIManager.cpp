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

/// @file   GUIManager.h
/// @author based on the basemanager code from mygui common

#include "GUIManager.h"

#include "AppContext.h"
#include "ActorManager.h"
#include "CameraManager.h"
#include "ContentManager.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "GUIUtils.h"
#include "InputEngine.h"
#include "Language.h"
#include "OgreImGui.h"
#include "OverlayWrapper.h"
#include "PlatformUtils.h"
#include "RTTLayer.h"
#include "Terrain.h"

#include <OgreOverlayElement.h>
#include <OgreOverlayContainer.h>
#include <OgreOverlayManager.h>

#define RESOURCE_FILENAME "MyGUI_Core.xml"

using namespace RoR;

/// Global list of UI Presets, selectable via Settings menu in TopMenubar
UiPresetEntry RoR::UiPresets[] =
{
    // Cvar name                      | NOVICE, REGULAR, EXPERT, MINIMALLIST
    { "gfx_surveymap_icons",          {"true",  "true",  "true",  "false"} },
    { "gfx_declutter_map",            {"false", "true",  "false", "true"} },
    { "ui_show_live_repair_controls", {"true",  "false", "false", "false"} },

    // List closure
    { nullptr, {} }
};

GUIManager::GUIManager()
{
    std::string gui_logpath = PathCombine(App::sys_logs_dir->getStr(), "MyGUI.log");
    auto mygui_platform = new MyGUI::OgrePlatform();
    mygui_platform->initialise(
        App::GetAppContext()->GetRenderWindow(), 
        App::GetGfxScene()->GetSceneManager(),
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

    m_mygui_platform = mygui_platform;
    m_mygui = mygui;
    MyGUI::PointerManager::getInstance().setVisible(false); // RoR is using mouse cursor drawn by DearIMGUI.

#ifdef _WIN32
    MyGUI::LanguageManager::getInstance().eventRequestTag = MyGUI::newDelegate(this, &GUIManager::eventRequestTag);
#endif // _WIN32

    this->SetupImGui();

    // Configure the chatbox console view
    this->ChatBox.GetConsoleView().cvw_background_color   = m_theme.semitrans_text_bg_color;
    this->ChatBox.GetConsoleView().cvw_background_padding = m_theme.semitrans_text_bg_padding;
}

GUIManager::~GUIManager()
{
}

void GUIManager::ShutdownMyGUI()
{
    if (m_mygui)
    {
        m_mygui->shutdown();
        delete m_mygui;
        m_mygui = nullptr;
    }

    if (m_mygui_platform)
    {
        m_mygui_platform->shutdown();
        delete m_mygui_platform;
        m_mygui_platform = nullptr;
    }
}

void GUIManager::ApplyGuiCaptureKeyboard()
{
    m_gui_kb_capture_requested = m_gui_kb_capture_queued;
};

bool GUIManager::AreStaticMenusAllowed() //!< i.e. top menubar / vehicle UI buttons
{
    return (App::GetCameraManager()->GetCurrentBehavior() != CameraManager::CAMERA_BEHAVIOR_FREE &&
            !this->ConsoleWindow.IsHovered() &&
            !this->GameControls.IsHovered() &&
            !this->FrictionSettings.IsHovered() &&
            !this->TextureToolWindow.IsHovered() &&
            !this->NodeBeamUtils.IsHovered() &&
            !this->CollisionsDebug.IsHovered() &&
            !this->MainSelector.IsHovered() &&
            !this->SurveyMap.IsHovered() &&
            !this->FlexbodyDebug.IsHovered());
}

void GUIManager::ApplyUiPreset() //!< reads cvar 'ui_preset'
{
    const int preset = App::ui_preset->getInt();
    int i = 0;
    while (UiPresets[i].uip_cvar != nullptr)
    {
        App::GetConsole()->cVarSet(
            UiPresets[i].uip_cvar,
            UiPresets[i].uip_values[preset]);
        i++;
    }
}

void GUIManager::DrawSimulationGui(float dt)
{
    if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
    {
        this->TopMenubar.Update();

        if (this->GameMainMenu.IsVisible())
        {
            this->GameMainMenu.Draw();
        }
    }

    if (this->NodeBeamUtils.IsVisible())
    {
        this->NodeBeamUtils.Draw();
    }

    if (this->CollisionsDebug.IsVisible())
    {
        this->CollisionsDebug.Draw();
    }

    if (this->MessageBoxDialog.IsVisible())
    {
        this->MessageBoxDialog.Draw();
    }

    if (this->FlexbodyDebug.IsVisible())
    {
        this->FlexbodyDebug.Draw();
    }
};

void GUIManager::DrawSimGuiBuffered(GfxActor* player_gfx_actor)
{
    this->DrawCommonGui();

    if (player_gfx_actor && this->SimActorStats.IsVisible())
    {
        this->SimActorStats.Draw(player_gfx_actor);
    }

    if (player_gfx_actor && player_gfx_actor->GetActor()->ar_state == ActorState::LOCAL_SIMULATED && 
        !this->SimActorStats.IsVisible() && !this->SimPerfStats.IsVisible() && !this->GameMainMenu.IsVisible())
    {
        this->VehicleButtons.Draw(player_gfx_actor);
    }

    if (!this->ConsoleWindow.IsVisible())
    {
        if (!App::ui_hide_gui->getBool())
        {
            this->ChatBox.Draw(); // Messages must be always visible
        }
    }

    if (this->LoadingWindow.IsVisible())
    {
        this->LoadingWindow.Draw();
    }

    if (this->FrictionSettings.IsVisible())
    {
        this->FrictionSettings.Draw();
    }

    if (this->VehicleDescription.IsVisible())
    {
        this->VehicleDescription.Draw();
    }

    if (this->SimPerfStats.IsVisible())
    {
        this->SimPerfStats.Draw();
    }

    if (this->TextureToolWindow.IsVisible())
    {
        this->TextureToolWindow.Draw();
    }

    if (this->SurveyMap.IsVisible())
    {
        this->SurveyMap.Draw();
    }

    this->DirectionArrow.Update(player_gfx_actor);
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
    this->MenuWallpaper = Ogre::OverlayManager::getSingleton().create("rigsofrods/WallpaperOverlay");
    this->MenuWallpaper->add2D(static_cast<Ogre::OverlayContainer*>(wp_panel)); // 'Panel' inherits from 'Container'
    this->MenuWallpaper->setZOrder(0);
    this->MenuWallpaper->show();
}

void GUIManager::SetSceneManagerForGuiRendering(Ogre::SceneManager* scene_manager)
{
    m_mygui_platform->getRenderManagerPtr()->setSceneManager(scene_manager);
}

void GUIManager::SetGuiHidden(bool hidden)
{
    App::ui_hide_gui->setVal(hidden);
    App::GetOverlayWrapper()->showDashboardOverlays(!hidden, App::GetGameContext()->GetPlayerActor());
    if (hidden)
    {
        this->SimPerfStats.SetVisible(false);
        this->ChatBox.SetVisible(false);
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

void GUIManager::UpdateMouseCursorVisibility()
{
    if (m_last_mousemove_time.getMilliseconds() > 5000)
    {
        this->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::HIDDEN);
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

    App::GetGfxScene()->GetSceneManager()->addRenderQueueListener(&m_imgui);
}

void GUIManager::DrawCommonGui()
{
    if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED && !App::ui_hide_gui->getBool())
    {
        this->MpClientList.Draw();
    }

    if (this->MainSelector.IsVisible())
    {
        this->MainSelector.Draw();
    }

    if (this->ConsoleWindow.IsVisible())
    {
        this->ConsoleWindow.Draw();
    }

    if (this->GameControls.IsVisible())
    {
        this->GameControls.Draw();
    }
}

void GUIManager::DrawMainMenuGui()
{
    this->DrawCommonGui();

    if (this->MultiplayerSelector.IsVisible())
    {
        this->MultiplayerSelector.Draw();
    }

    if (this->GameMainMenu.IsVisible())
    {
        this->GameMainMenu.Draw();
    }

    if (this->GameSettings.IsVisible())
    {
        this->GameSettings.Draw();
    }

    if (this->MessageBoxDialog.IsVisible())
    {
        this->MessageBoxDialog.Draw();
    }

    if (this->LoadingWindow.IsVisible())
    {
        this->LoadingWindow.Draw();
    }

    if (this->GameAbout.IsVisible())
    {
        this->GameAbout.Draw();
    }

    if (this->RepositorySelector.IsVisible())
    {
        this->RepositorySelector.Draw();
    }
}

void GUIManager::ShowMessageBox(const char* title, const char* text, bool allow_close, const char* btn1_text, const char* btn2_text)
{
    this->MessageBoxDialog.Show(title, text, allow_close, btn1_text, btn2_text);
}

void GUIManager::ShowMessageBox(GUI::MessageBoxConfig const& conf)
{
    this->MessageBoxDialog.Show(conf);
}

void GUIManager::RequestGuiCaptureKeyboard(bool val) 
{ 
    m_gui_kb_capture_queued = m_gui_kb_capture_queued || val;
}

void GUIManager::WakeUpGUI()
{
    m_last_mousemove_time.reset();
    if (!m_is_cursor_supressed)
    {
        this->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::VISIBLE);
    }
}

void GUIManager::SupressCursor(bool do_supress)
{
    m_is_cursor_supressed = do_supress;
}

void GUIManager::UpdateInputEvents(float dt)
{
    // EV_COMMON_CONSOLE_TOGGLE - display console GUI (anytime)
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_CONSOLE_TOGGLE))
    {
        this->ConsoleWindow.SetVisible(!this->ConsoleWindow.IsVisible());
    }

    if (App::app_state->getEnum<AppState>() == AppState::SIMULATION)
    {
        // EV_COMMON_HIDE_GUI
        if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_HIDE_GUI))
        {
            this->SetGuiHidden(!this->IsGuiHidden());
        }

        // EV_COMMON_ENTER_CHATMODE
        if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_CHATMODE, 0.5f) &&
            App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
        {
            this->ChatBox.SetVisible(!this->ChatBox.IsVisible());
        }

        // EV_COMMON_TRUCK_INFO - Vehicle status panel
        if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TRUCK_INFO) && App::GetGameContext()->GetPlayerActor())
        {
            this->SimActorStats.SetVisible(!this->SimActorStats.IsVisible());
        }

        // EV_COMMON_TRUCK_DESCRIPTION - Vehicle controls and details
        if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TRUCK_DESCRIPTION) && App::GetGameContext()->GetPlayerActor())
        {
            this->VehicleDescription.SetVisible(!this->VehicleDescription.IsVisible());
        }

        // EV_COMMON_TOGGLE_DASHBOARD
        if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_DASHBOARD))
        {
            App::GetOverlayWrapper()->ToggleDashboardOverlays(App::GetGameContext()->GetPlayerActor());
        }

        // EV_COMMON_TOGGLE_STATS - FPS, draw batch count etc...
        if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_STATS))
        {
            this->SimPerfStats.SetVisible(!this->SimPerfStats.IsVisible());
        }

        if (App::GetCameraManager()->GetCurrentBehavior() != CameraManager::CAMERA_BEHAVIOR_FREE)
        {
            // EV_SURVEY_MAP_CYCLE
            if (App::GetInputEngine()->getEventBoolValueBounce(EV_SURVEY_MAP_CYCLE))
            {
                this->SurveyMap.CycleMode();
            }

            // EV_SURVEY_MAP_TOGGLE
            if (App::GetInputEngine()->getEventBoolValueBounce(EV_SURVEY_MAP_TOGGLE))
            {
                this->SurveyMap.ToggleMode();
            }
        }
    }
}
