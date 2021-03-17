/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2020 Petr Ohlidal

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
/// @date   06/2017

#include "GUI_TopMenubar.h"

#include "Application.h"
#include "Actor.h"
#include "ActorManager.h"
#include "CameraManager.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "GUI_MainSelector.h"
#include "InputEngine.h"
#include "Language.h"
#include "Network.h"
#include "PlatformUtils.h"
#include "Replay.h"
#include "SkyManager.h"
#include "TerrainManager.h"

#include <algorithm>
#include <fmt/format.h>

using namespace RoR;
using namespace GUI;

void TopMenubar::Update()
{
    // ## ImGui's 'menubar' and 'menuitem' features won't quite cut it...
    // ## Let's do our own menus and menuitems using buttons and coloring tricks.

    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

    auto actors = App::GetGameContext()->GetActorManager()->GetActors();
    int num_playable_actors = std::count_if(actors.begin(), actors.end(), [](Actor* a) {return !a->ar_hide_in_actor_list;});

    std::string sim_title =         _LC("TopMenubar", "Simulation");
    std::string actors_title = fmt::format("{} ({})", _LC("TopMenubar", "Vehicles"), num_playable_actors);
    std::string savegames_title =   _LC("TopMenubar", "Saves");
    std::string settings_title =    _LC("TopMenubar", "Settings");
    std::string tools_title =       _LC("TopMenubar", "Tools");
    const int NUM_BUTTONS = 5;

    float menubar_content_width =
        (ImGui::GetStyle().ItemSpacing.x * (NUM_BUTTONS - 1)) +
        (ImGui::GetStyle().FramePadding.x * (NUM_BUTTONS * 2)) +
        ImGui::CalcTextSize(sim_title.c_str()).x +
        ImGui::CalcTextSize(actors_title.c_str()).x +
        ImGui::CalcTextSize(savegames_title.c_str()).x +
        ImGui::CalcTextSize(settings_title.c_str()).x +
        ImGui::CalcTextSize(tools_title.c_str()).x;

    ImVec2 window_target_pos = ImVec2((ImGui::GetIO().DisplaySize.x/2.f) - (menubar_content_width / 2.f), theme.screen_edge_padding.y);
    if (!this->ShouldDisplay(window_target_pos))
    {
        m_open_menu = TopMenu::TOPMENU_NONE;
        m_confirm_remove_all = false;
        this->DrawSpecialStateBox(10.f);
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.semitransparent_window_bg);
    ImGui::PushStyleColor(ImGuiCol_Button,   ImVec4(0,0,0,0)); // Fully transparent

    // The panel
    int flags = ImGuiWindowFlags_NoCollapse  | ImGuiWindowFlags_NoResize    | ImGuiWindowFlags_NoMove
              | ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::SetNextWindowContentSize(ImVec2(menubar_content_width, 0.f));
    ImGui::SetNextWindowPos(window_target_pos);
    ImGui::Begin("Top menubar", nullptr, flags);

    // The 'simulation' button
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 sim_cursor = ImGui::GetCursorPos();
    ImGui::Button(sim_title.c_str());
    if ((m_open_menu != TopMenu::TOPMENU_SIM) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_SIM;
    }

    ImGui::SameLine();

    // The 'vehicles' button
    ImVec2 actors_cursor = ImGui::GetCursorPos();
    ImGui::Button(actors_title.c_str());
    if ((m_open_menu != TopMenu::TOPMENU_ACTORS) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_ACTORS;
    }

    ImGui::SameLine();

    // The 'savegames' button
    ImVec2 savegames_cursor = ImGui::GetCursorPos();
    ImGui::Button(savegames_title.c_str());
    if ((m_open_menu != TopMenu::TOPMENU_SAVEGAMES) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_SAVEGAMES;
        m_quicksave_name = App::GetGameContext()->GetQuicksaveFilename();
        m_quickload = FileExists(PathCombine(App::sys_savegames_dir->GetStr(), m_quicksave_name));
        m_savegame_names.clear();
        for (int i = 0; i <= 9; i++)
        {
            Ogre::String filename = Ogre::StringUtil::format("quicksave-%d.sav", i);
            m_savegame_names.push_back(App::GetGameContext()->ExtractSceneName(filename));
        }
    }

    ImGui::SameLine();

    // The 'settings' button
    ImVec2 settings_cursor = ImGui::GetCursorPos();
    ImGui::Button(settings_title.c_str());
    if ((m_open_menu != TopMenu::TOPMENU_SETTINGS) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_SETTINGS;
#ifdef USE_CAELUM
        if (App::gfx_sky_mode->GetEnum<GfxSkyMode>() == GfxSkyMode::CAELUM)
            m_daytime = App::GetSimTerrain()->getSkyManager()->GetTime();
#endif // USE_CAELUM
    }

    ImGui::SameLine();

    // The 'tools' button
    ImVec2 tools_cursor = ImGui::GetCursorPos();
    ImGui::Button(tools_title.c_str());
    if ((m_open_menu != TopMenu::TOPMENU_TOOLS) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_TOOLS;
    }

    ImVec2 topmenu_final_size = ImGui::GetWindowSize();
    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();

    this->DrawSpecialStateBox(window_target_pos.y + topmenu_final_size.y + 10.f);

    ImVec2 menu_pos;
    Actor* current_actor = App::GetGameContext()->GetPlayerActor();
    switch (m_open_menu)
    {
    case TopMenu::TOPMENU_SIM:
        menu_pos.y = window_pos.y + sim_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = sim_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin(_LC("TopMenubar", "Sim menu"), nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            // TODO: Display hotkeys on the right side of the menu (with different text color)

            if (ImGui::Button(_LC("TopMenubar", "Get new vehicle")))
            {
                m_open_menu = TopMenu::TOPMENU_NONE;

                RoR::Message m(MSG_GUI_OPEN_SELECTOR_REQUESTED);
                m.payload = reinterpret_cast<void*>(new LoaderType(LT_AllBeam));
                App::GetGameContext()->PushMessage(m);
            }

            if (current_actor != nullptr)
            {
                if (ImGui::Button(_LC("TopMenubar", "Show vehicle description")))
                {
                    App::GetGuiManager()->SetVisible_VehicleDescription(true);
                }

                if (current_actor->ar_sim_state != Actor::SimState::NETWORKED_OK)
                {
                    if (ImGui::Button(_LC("TopMenubar", "Reload current vehicle")))
                    {
                        ActorModifyRequest* rq = new ActorModifyRequest;
                        rq->amr_type = ActorModifyRequest::Type::RELOAD;
                        rq->amr_actor = current_actor;
                        App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
                    }

                    if (ImGui::Button(_LC("TopMenubar", "Remove current vehicle")))
                    {
                        App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, (void*)current_actor));
                    }
                }
            }

            if (!App::GetGameContext()->GetActorManager()->GetLocalActors().empty())
            {
                if (ImGui::Button(_LC("TopMenubar", "Remove all vehicles")))
                {
                    m_confirm_remove_all = true;
                }
                if (m_confirm_remove_all)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ORANGE_TEXT);
                    if (ImGui::Button(_LC("TopMenubar", " [!] Confirm removal")))
                    {
                        for (auto actor : App::GetGameContext()->GetActorManager()->GetLocalActors())
                        {
                            if (!actor->ar_hide_in_actor_list && !actor->isPreloadedWithTerrain() && 
                                    actor->ar_sim_state != Actor::SimState::NETWORKED_OK)
                            {
                                App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, (void*)actor));
                            }
                        }
                        m_confirm_remove_all = false;
                    }
                    ImGui::PopStyleColor();
                }

                if (ImGui::Button(_LC("TopMenubar", "Activate all vehicles")))
                {
                    App::GetGameContext()->GetActorManager()->WakeUpAllActors();
                }

                bool force_trucks_active = App::GetGameContext()->GetActorManager()->AreTrucksForcedAwake();
                if (ImGui::Checkbox(_LC("TopMenubar", "Activated vehicles never sleep"), &force_trucks_active))
                {
                    App::GetGameContext()->GetActorManager()->SetTrucksForcedAwake(force_trucks_active);
                }

                if (ImGui::Button(_LC("TopMenubar", "Send all vehicles to sleep")))
                {
                    App::GetGameContext()->GetActorManager()->SendAllActorsSleeping();
                }
            }

            ImGui::Separator();

            if (ImGui::Button(_LC("TopMenubar", "Back to menu")))
            {
                if (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED)
                {
                    App::GetGameContext()->PushMessage(Message(MSG_NET_DISCONNECT_REQUESTED));
                }
                App::GetGameContext()->PushMessage(Message(MSG_SIM_UNLOAD_TERRN_REQUESTED));
                App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
            }

            if (ImGui::Button(_LC("TopMenubar", "Exit")))
            {
                App::GetGameContext()->PushMessage(Message(MSG_APP_SHUTDOWN_REQUESTED));
            }

            m_open_menu_hoverbox_min = menu_pos;
            m_open_menu_hoverbox_max.x = menu_pos.x + ImGui::GetWindowWidth();
            m_open_menu_hoverbox_max.y = menu_pos.y + ImGui::GetWindowHeight();
            App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
            ImGui::End();
        }
        break;

    case TopMenu::TOPMENU_ACTORS:
        menu_pos.y = window_pos.y + actors_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = actors_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin(_LC("TopMenubar", "Actors menu"), nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            if (App::mp_state->GetEnum<MpState>() != MpState::CONNECTED)
            {
                this->DrawActorListSinglePlayer();
            }
            else
            {
#ifdef USE_SOCKETW
                RoRnet::UserInfo net_user_info = App::GetNetwork()->GetLocalUserData();
                this->DrawMpUserToActorList(net_user_info);

                std::vector<RoRnet::UserInfo> remote_users = App::GetNetwork()->GetUserInfos();
                for (auto& user: remote_users)
                {
                    this->DrawMpUserToActorList(user);
                }
#endif // USE_SOCKETW
            }
            m_open_menu_hoverbox_min = menu_pos;
            m_open_menu_hoverbox_max.x = menu_pos.x + ImGui::GetWindowWidth();
            m_open_menu_hoverbox_max.y = menu_pos.y + ImGui::GetWindowHeight();
            App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
            ImGui::End();
        }
        break;

    case TopMenu::TOPMENU_SAVEGAMES:
        menu_pos.y = window_pos.y + savegames_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = savegames_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin(_LC("TopMenubar", "Savegames"), nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            if (ImGui::Button(_LC("TopMenubar", "Quicksave")))
            {
                App::GetGameContext()->GetActorManager()->SaveScene(m_quicksave_name);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }
            ImGui::SameLine();
            ImGui::TextColored(GRAY_HINT_TEXT, "('NUMPAD: /')");

            if (m_quickload)
            {
                if (ImGui::Button(_LC("TopMenubar", "Quickload")))
                {
                    App::GetGameContext()->GetActorManager()->LoadScene(m_quicksave_name);
                    m_open_menu = TopMenu::TOPMENU_NONE;
                }
                ImGui::SameLine();
                ImGui::TextColored(GRAY_HINT_TEXT, "('NUMPAD: *')");
            }

            ImGui::Separator();

            ImGui::TextColored(GRAY_HINT_TEXT, _LC("TopMenubar", "(Save with 'CTRL+ALT+1..5')"));
            for (int i = 1; i <= 5; i++)
            {
                Ogre::String name = _LC("TopMenubar", "Empty Slot");
                if (!m_savegame_names[i].empty())
                {
                    name = m_savegame_names[i];
                }
                Ogre::String caption = Ogre::StringUtil::format("%d. %s##Save", i, name.c_str());
                if (ImGui::Button(caption.c_str()))
                {
                    Ogre::String filename = Ogre::StringUtil::format("quicksave-%d.sav", i);
                    App::GetGameContext()->GetActorManager()->SaveScene(filename);
                    m_open_menu = TopMenu::TOPMENU_NONE;
                }
            }

            ImGui::TextColored(GRAY_HINT_TEXT, _LC("TopMenubar", "(Load with 'ALT+1..5')"));
            for (int i = 1; i <= 5; i++)
            {
                if (!m_savegame_names[i].empty())
                {
                    Ogre::String name = m_savegame_names[i];
                    Ogre::String caption = Ogre::StringUtil::format("%d. %s##Load", i, name.c_str());
                    if (ImGui::Button(caption.c_str()))
                    {
                        Ogre::String filename = Ogre::StringUtil::format("quicksave-%d.sav", i);
                        App::GetGameContext()->PushMessage(RoR::Message(MSG_SIM_LOAD_SAVEGAME_REQUESTED, filename));
                        m_open_menu = TopMenu::TOPMENU_NONE;
                    }
                }
            }

            m_open_menu_hoverbox_min = menu_pos;
            m_open_menu_hoverbox_max.x = menu_pos.x + ImGui::GetWindowWidth();
            m_open_menu_hoverbox_max.y = menu_pos.y + ImGui::GetWindowHeight() * 2;
            App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
            ImGui::End();
        }
        break;

    case TopMenu::TOPMENU_SETTINGS:
        menu_pos.y = window_pos.y + settings_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = settings_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin(_LC("TopMenubar", "Settings menu"), nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            ImGui::PushItemWidth(125.f); // Width includes [+/-] buttons
            ImGui::TextColored(GRAY_HINT_TEXT,_LC("TopMenubar",  "Audio:"));
            DrawGFloatSlider(App::audio_master_volume, _LC("TopMenubar", "Volume"), 0, 1);
            ImGui::Separator();
            ImGui::TextColored(GRAY_HINT_TEXT, _LC("TopMenubar", "Frames per second:"));
            if (App::gfx_envmap_enabled->GetBool())
            {
                DrawGIntSlider(App::gfx_envmap_rate, _LC("TopMenubar", "Reflections"), 0, 6);
            }
            DrawGIntSlider(App::gfx_fps_limit, _LC("TopMenubar", "Game"), 0, 240);

            /*   //  Uncomment if you want to tweak the physics, it has no other use, really.
                 //  Do not publish - players tend to max it out and then complain about performance. :|
            int physics_fps = std::round(1.0f / App::diag_physics_dt->GetFloat());
            if (ImGui::SliderInt("Physics", &physics_fps, 2000, 10000))
            {
                App::diag_physics_dt->SetVal(Ogre::Math::Clamp(1.0f / physics_fps, 0.0001f, 0.0005f));
            }
            */
            ImGui::Separator();
            ImGui::TextColored(GRAY_HINT_TEXT, _LC("TopMenubar", "Simulation:"));
            float slowmotion = std::min(App::GetGameContext()->GetActorManager()->GetSimulationSpeed(), 1.0f);
            if (ImGui::SliderFloat(_LC("TopMenubar", "Slow motion"), &slowmotion, 0.01f, 1.0f))
            {
                App::GetGameContext()->GetActorManager()->SetSimulationSpeed(slowmotion);
            }
            float timelapse = std::max(App::GetGameContext()->GetActorManager()->GetSimulationSpeed(), 1.0f);
            if (ImGui::SliderFloat(_LC("TopMenubar", "Time lapse"), &timelapse, 1.0f, 10.0f))
            {
                App::GetGameContext()->GetActorManager()->SetSimulationSpeed(timelapse);
            }
            if (App::GetCameraManager()->GetCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_STATIC)
            {
                ImGui::Separator();
                ImGui::TextColored(GRAY_HINT_TEXT, _LC("TopMenubar", "Camera:"));
                DrawGFloatSlider(App::gfx_static_cam_fov_exp, _LC("TopMenubar", "FOV"), 0.8f, 1.5f);
                DrawGIntSlider(App::gfx_camera_height, _LC("TopMenubar", "Height"), 1, 50);
            }
            else
            {
                ImGui::Separator();
                ImGui::TextColored(GRAY_HINT_TEXT, _LC("TopMenubar", "Camera:"));
                if (App::GetCameraManager()->GetCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)
                {
                    int fov = App::gfx_fov_internal->GetInt();
                    if (ImGui::SliderInt(_LC("TopMenubar", "FOV"), &fov, 10, 120))
                    {
                        App::gfx_fov_internal->SetVal(fov);
                    }
                }
                else
                {
                    int fov = App::gfx_fov_external->GetInt();
                    if (ImGui::SliderInt(_LC("TopMenubar", "FOV"), &fov, 10, 120))
                    {
                        App::gfx_fov_external->SetVal(fov);
                    }
                }
                if (App::GetCameraManager()->GetCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_FIXED)
                {
                    DrawGCheckbox(App::gfx_fixed_cam_tracking, _LC("TopMenubar", "Tracking"));
                }
            }
#ifdef USE_CAELUM
            if (App::gfx_sky_mode->GetEnum<GfxSkyMode>() == GfxSkyMode::CAELUM)
            {
                ImGui::Separator();
                ImGui::TextColored(GRAY_HINT_TEXT, _LC("TopMenubar", "Time of day:"));
                float time = App::GetSimTerrain()->getSkyManager()->GetTime();
                if (ImGui::SliderFloat("", &time, m_daytime - 0.5f, m_daytime + 0.5f, ""))
                {
                    App::GetSimTerrain()->getSkyManager()->SetTime(time);
                }
            }       
#endif // USE_CAELUM
            
            if (current_actor != nullptr)
            {
                ImGui::Separator();
                ImGui::TextColored(GRAY_HINT_TEXT,_LC("TopMenubar",  "Vehicle control options:"));
                DrawGCheckbox(App::io_hydro_coupling, _LC("TopMenubar", "Keyboard steering speed coupling"));
            }
            if (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED)
            {
                ImGui::Separator();
                ImGui::TextColored(GRAY_HINT_TEXT,       _LC("TopMenubar", "Multiplayer:"));
                DrawGCheckbox(App::mp_pseudo_collisions, _LC("TopMenubar", "Collisions"));
                DrawGCheckbox(App::mp_hide_net_labels,   _LC("TopMenubar", "Hide labels"));
            }
            ImGui::PopItemWidth();
            m_open_menu_hoverbox_min = menu_pos;
            m_open_menu_hoverbox_max.x = menu_pos.x + ImGui::GetWindowWidth();
            m_open_menu_hoverbox_max.y = menu_pos.y + ImGui::GetWindowHeight();
            App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
            ImGui::End();
        }
        break;

    case TopMenu::TOPMENU_TOOLS:
        menu_pos.y = window_pos.y + tools_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = tools_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin(_LC("TopMenubar", "Tools menu"), nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            if (ImGui::Button(_LC("TopMenubar", "Friction settings")))
            {
                App::GetGuiManager()->SetVisible_FrictionSettings(true);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (ImGui::Button(_LC("TopMenubar", "Show console")))
            {
                App::GetGuiManager()->SetVisible_Console(! App::GetGuiManager()->IsVisible_Console());
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (ImGui::Button(_LC("TopMenubar", "Texture tool")))
            {
                App::GetGuiManager()->SetVisible_TextureToolWindow(true);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (ImGui::Button(_LC("TopMenubar", "Game controls")))
            {
                App::GetGuiManager()->SetVisible_GameControls(true);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (current_actor != nullptr)
            {
                if (ImGui::Button(_LC("TopMenubar", "Node / Beam utility")))
                {
                    App::GetGuiManager()->SetVisible_NodeBeamUtils(true);
                    m_open_menu = TopMenu::TOPMENU_NONE;
                }

                if (ImGui::Button(_LC("TopMenubar", "Export current vehicle")))
                {
                    App::GetGameContext()->PushMessage(Message(MSG_EDI_EXPORT_TRUCK_REQUESTED, (void*)current_actor->GetCacheEntry()));
                }
            }

            ImGui::Separator();
            ImGui::TextColored(GRAY_HINT_TEXT, _LC("TopMenubar", "Pre-spawn diag. options:"));

            bool diag_mass = App::diag_truck_mass->GetBool();
            if (ImGui::Checkbox(_LC("TopMenubar", "Node mass recalc. logging"), &diag_mass))
            {
                App::diag_truck_mass->SetVal(diag_mass);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(_LC("TopMenubar", "Extra logging on runtime - mass recalculation"));
                ImGui::EndTooltip();
            }

            bool diag_break = App::diag_log_beam_break->GetBool();
            if (ImGui::Checkbox(_LC("TopMenubar", "Beam break logging"), &diag_break))
            {
                App::diag_log_beam_break->SetVal(diag_break);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(_LC("TopMenubar", "Extra logging on runtime"));
                ImGui::EndTooltip();
            }

            bool diag_deform = App::diag_log_beam_deform->GetBool();
            if (ImGui::Checkbox(_LC("TopMenubar", "Beam deform. logging"), &diag_deform))
            {
                App::diag_log_beam_deform->SetVal(diag_deform);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(_LC("TopMenubar", "Extra logging on runtime"));
                ImGui::EndTooltip();
            }

            bool diag_trig = App::diag_log_beam_trigger->GetBool();
            if (ImGui::Checkbox(_LC("TopMenubar", "Trigger logging"), &diag_trig))
            {
                App::diag_log_beam_trigger->SetVal(diag_trig);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(_LC("TopMenubar", "Extra logging on runtime - trigger beams activity"));
                ImGui::EndTooltip();
            }

            bool diag_vcam = App::diag_videocameras->GetBool();
            if (ImGui::Checkbox(_LC("TopMenubar", "VideoCamera direction marker"), &diag_vcam))
            {
                App::diag_videocameras->SetVal(diag_vcam);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(_LC("TopMenubar", "Visual marker of VideoCameras direction"));
                ImGui::EndTooltip();
            }

            if (current_actor != nullptr)
            {
                ImGui::Separator();

                ImGui::TextColored(GRAY_HINT_TEXT, _LC("TopMenubar", "Live diagnostic views:"));
                ImGui::TextColored(GRAY_HINT_TEXT, _LC("TopMenubar", "(Toggle with '%s')"), App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_DEBUG_VIEW).c_str());
                ImGui::TextColored(GRAY_HINT_TEXT, _LC("TopMenubar", "(Cycle with '%s')"), App::GetInputEngine()->getEventCommand(EV_COMMON_CYCLE_DEBUG_VIEWS).c_str());

                int debug_view_type = static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_NONE);
                if (current_actor != nullptr)
                {
                    debug_view_type = static_cast<int>(current_actor->GetGfxActor()->GetDebugView());
                }
                ImGui::RadioButton(_LC("TopMenubar", "Normal view"),     &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_NONE));
                ImGui::RadioButton(_LC("TopMenubar", "Skeleton view"),   &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_SKELETON));
                ImGui::RadioButton(_LC("TopMenubar", "Node details"),    &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_NODES));
                ImGui::RadioButton(_LC("TopMenubar", "Beam details"),    &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_BEAMS));
                if (current_actor->ar_num_wheels > 0)
                {
                    ImGui::RadioButton(_LC("TopMenubar", "Wheel details"),   &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_WHEELS));
                }
                if (current_actor->ar_num_shocks > 0)
                {
                    ImGui::RadioButton(_LC("TopMenubar", "Shock details"),   &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_SHOCKS));
                }
                if (current_actor->ar_num_rotators > 0)
                {
                    ImGui::RadioButton(_LC("TopMenubar", "Rotator details"), &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_ROTATORS));
                }
                if (current_actor->hasSlidenodes())
                {
                    ImGui::RadioButton(_LC("TopMenubar", "Slidenode details"), &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_SLIDENODES));
                }
                if (current_actor->ar_num_cabs > 0)
                {
                    ImGui::RadioButton(_LC("TopMenubar", "Submesh details"), &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_SUBMESH));
                }

                if ((current_actor != nullptr) && (debug_view_type != static_cast<int>(current_actor->GetGfxActor()->GetDebugView())))
                {
                    current_actor->GetGfxActor()->SetDebugView(static_cast<GfxActor::DebugViewType>(debug_view_type));
                }

                if (debug_view_type >= 1 && debug_view_type <= static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_BEAMS)) 
                {
                    ImGui::Separator();
                    ImGui::TextColored(GRAY_HINT_TEXT,           _LC("TopMenubar", "Settings:"));
                    DrawGCheckbox(App::diag_hide_broken_beams,   _LC("TopMenubar", "Hide broken beams"));
                    DrawGCheckbox(App::diag_hide_beam_stress,    _LC("TopMenubar", "Hide beam stress"));
                    DrawGCheckbox(App::diag_hide_wheels,         _LC("TopMenubar", "Hide wheels"));
                    DrawGCheckbox(App::diag_hide_nodes,          _LC("TopMenubar", "Hide nodes"));
                    if (debug_view_type >= 2)
                    {
                        DrawGCheckbox(App::diag_hide_wheel_info, _LC("TopMenubar", "Hide wheel info"));
                    }
                }
            }

            m_open_menu_hoverbox_min = menu_pos;
            m_open_menu_hoverbox_max.x = menu_pos.x + ImGui::GetWindowWidth();
            m_open_menu_hoverbox_max.y = menu_pos.y + ImGui::GetWindowHeight();
            App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
            ImGui::End();
        }
        break;

    default:
        m_open_menu_hoverbox_min = ImVec2(0,0);
        m_open_menu_hoverbox_max = ImVec2(0,0);
    }

    ImGui::PopStyleColor(2); // WindowBg, Button
}

bool TopMenubar::ShouldDisplay(ImVec2 window_pos)
{
    ImVec2 box_min(0,0);
    ImVec2 box_max(ImGui::GetIO().DisplaySize.x, ImGui::GetStyle().WindowPadding.y + PANEL_HOVERBOX_HEIGHT);
    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
    bool window_hovered ((mouse_pos.x >= box_min.x) && (mouse_pos.x <= box_max.x) &&
                            (mouse_pos.y >= box_min.y) && (mouse_pos.y <= box_max.y));

    if (m_open_menu == TopMenu::TOPMENU_NONE)
    {
        return window_hovered;
    }

    bool menu_hovered ((mouse_pos.x >= m_open_menu_hoverbox_min.x) && (mouse_pos.x <= m_open_menu_hoverbox_max.x) &&
                        (mouse_pos.y >= m_open_menu_hoverbox_min.y) && (mouse_pos.y <= m_open_menu_hoverbox_max.y));
    return (menu_hovered || window_hovered);
}

void TopMenubar::DrawMpUserToActorList(RoRnet::UserInfo &user)
{
    // Count actors owned by the player
    unsigned int num_actors_player = 0;
    for (Actor* actor : App::GetGameContext()->GetActorManager()->GetActors())
    {
        if (actor->ar_net_source_id == user.uniqueid)
        {
            ++num_actors_player;
        }
    }

    // Display user in list
#ifdef USE_SOCKETW
    const Ogre::ColourValue player_color = App::GetNetwork()->GetPlayerColor(user.colournum);
    ImVec4 player_gui_color(player_color.r, player_color.g, player_color.b, 1.f);
    ImGui::PushStyleColor(ImGuiCol_Text, player_gui_color);
    ImGui::Text("%s: %u (%s, Ver: %s, Lang: %s)",
                user.username, num_actors_player,
                App::GetNetwork()->UserAuthToStringShort(user).c_str(),
                user.clientversion, user.language);
    ImGui::PopStyleColor();
#endif // USE_SOCKETW

    // Display actor list
    int i = 0;
    for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
    {
        if ((!actor->ar_hide_in_actor_list) && (actor->ar_net_source_id == user.uniqueid))
        {
            std::string actortext_buf = fmt::format("  + {} ({}) ##[{}:{}]", actor->ar_design_name.c_str(), actor->ar_filename.c_str(), i++, user.uniqueid);
            if (ImGui::Button(actortext_buf.c_str())) // Button clicked?
            {
                App::GetGameContext()->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, (void*)actor));
            }
        }
    }
}

void TopMenubar::DrawActorListSinglePlayer()
{
    std::vector<Actor*> actor_list;
    for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
    {
        if (!actor->ar_hide_in_actor_list)
        {
            actor_list.emplace_back(actor);
        }
    }
    if (actor_list.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GRAY_HINT_TEXT);
        ImGui::Text("%s", _LC("TopMenubar", "None spawned yet"));
        ImGui::Text("%s", _LC("TopMenubar", "Use [Simulation] menu"));
        ImGui::PopStyleColor();
    }
    else
    {
        Actor* player_actor = App::GetGameContext()->GetPlayerActor();
        int i = 0;
        for (auto actor : actor_list)
        {
            std::string text_buf_rem = fmt::format("X ##[{}]", i);
            ImGui::PushStyleColor(ImGuiCol_Text, RED_TEXT);
            if (ImGui::Button(text_buf_rem.c_str()))
            {
                App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, (void*)actor));
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();

            std::string text_buf = fmt::format( "[{}] {}", i++, actor->ar_design_name.c_str());
            auto linked_actors = actor->GetAllLinkedActors();
            if (actor == player_actor)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, GREEN_TEXT);
            }
            else if (std::find(linked_actors.begin(), linked_actors.end(), player_actor) != linked_actors.end())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ORANGE_TEXT);
            }
            else if (actor->ar_sim_state == Actor::SimState::LOCAL_SIMULATED)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, WHITE_TEXT);
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, GRAY_HINT_TEXT);
            }
            if (ImGui::Button(text_buf.c_str())) // Button clicked?
            {
                App::GetGameContext()->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, (void*)actor));
            }
            ImGui::PopStyleColor();
        }
    }
}

void TopMenubar::DrawSpecialStateBox(float top_offset)
{
    std::string special_text;
    ImVec4 special_color = ImGui::GetStyle().Colors[ImGuiCol_Text]; // Regular color
    float content_width = 0.f;
    bool replay_box = false;

    // Gather state info
    if (App::GetGameContext()->GetActorManager()->IsSimulationPaused() && !App::GetGuiManager()->IsGuiHidden())
    {
        special_color = ORANGE_TEXT;
        special_text = fmt::format(_LC("TopMenubar", "All physics paused, press '{}' to resume"),
                                   App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_PHYSICS));
        content_width = ImGui::CalcTextSize(special_text.c_str()).x;
    }
    else if (App::GetGameContext()->GetPlayerActor() &&
             App::GetGameContext()->GetPlayerActor()->ar_physics_paused &&
             !App::GetGuiManager()->IsGuiHidden())
    {
        special_color = GREEN_TEXT;
        special_text = fmt::format(_LC("TopMenubar", "Vehicle physics paused, press '{}' to resume"),
                                   App::GetInputEngine()->getEventCommand(EV_TRUCK_TOGGLE_PHYSICS));
        content_width = ImGui::CalcTextSize(special_text.c_str()).x;
    }
    else if (App::GetGameContext()->GetPlayerActor() &&
            App::GetGameContext()->GetPlayerActor()->ar_sim_state == Actor::SimState::LOCAL_REPLAY)
    {
        content_width = 300;
        replay_box = true;
        special_text = _LC("TopMenubar", "Replay");
    }

    // Draw box if needed
    if (!special_text.empty())
    {
        ImVec2 box_pos;
        box_pos.y = top_offset;
        box_pos.x = (ImGui::GetIO().DisplaySize.x / 2) - ((content_width / 2) + ImGui::GetStyle().FramePadding.x);
        ImGui::SetNextWindowPos(box_pos);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize    | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, App::GetGuiManager()->GetTheme().semitransparent_window_bg);
        if (ImGui::Begin("Special state box", nullptr, flags))
        {
            ImGui::TextColored(special_color, "%s", special_text.c_str());
            if (replay_box)
            {
                ImGui::SameLine();
                
                // Progress bar with frame index/count
                Replay* replay = App::GetGameContext()->GetPlayerActor()->GetReplay();
                float fraction = (float)std::abs(replay->getCurrentFrame())/(float)replay->getNumFrames();
                Str<100> pbar_text; pbar_text << replay->getCurrentFrame() << "/" << replay->getNumFrames();
                float pbar_width = content_width - (ImGui::GetStyle().ItemSpacing.x + ImGui::CalcTextSize(special_text.c_str()).x);
                ImGui::ProgressBar(fraction, ImVec2(pbar_width, ImGui::GetTextLineHeight()), pbar_text.ToCStr());

                // Game time text
                float time_sec = replay->getLastReadTime() / 1000000.0;
                char str[200];
                int str_pos = 0;
                if (time_sec > 60)
                {
                    int min = (int)time_sec / 60;
                    str_pos = snprintf(str, 200, "%dmin ", min);
                    time_sec -= (float)min * 60.f;
                }
                snprintf(str+str_pos, 200-str_pos, "%.2fsec", time_sec);
                ImGui::TextDisabled("%s: %s", _LC("TopMenubar", "Time"), str);
                
            }
            ImGui::End();
        }
        ImGui::PopStyleColor(1); // WindowBg
    }
}
