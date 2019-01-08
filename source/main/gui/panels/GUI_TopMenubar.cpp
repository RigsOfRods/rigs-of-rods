/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2017 Petr Ohlidal & contributors

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
#include "Beam.h"
#include "BeamFactory.h"
#include "GUIManager.h"
#include "GUI_MainSelector.h"
#include "MainMenu.h"
#include "RoRFrameListener.h"
#include "Network.h"

#include <algorithm>

void RoR::GUI::TopMenubar::Update()
{
    // ## ImGui's 'menubar' and 'menuitem' features won't quite cut it...
    // ## Let's do our own menus and menuitems using buttons and coloring tricks.

    const char* sim_title = "Simulation"; // TODO: Localize all!
    Str<50> actors_title;
    auto actors = App::GetSimController()->GetActors();
    int num_playable_actors = std::count_if(actors.begin(), actors.end(), [](Actor* a) {return !a->ar_hide_in_actor_list;});
    actors_title << "Vehicles (" << num_playable_actors << ")";
    const char* tools_title = "Tools";

    float panel_target_width = 
        (ImGui::GetStyle().WindowPadding.x * 2) + (ImGui::GetStyle().FramePadding.x * 2) + // Left+right window padding
        ImGui::CalcTextSize(sim_title).x + ImGui::CalcTextSize(actors_title.GetBuffer()).x + ImGui::CalcTextSize(tools_title).x +  // Items
        (ImGui::GetStyle().ItemSpacing.x * 3); // Item spacing

    ImVec2 window_target_pos = ImVec2((ImGui::GetIO().DisplaySize.x/2.f) - (panel_target_width / 2.f), ImGui::GetStyle().WindowPadding.y);
    if (!this->ShouldDisplay(window_target_pos))
    {
        m_open_menu = TopMenu::TOPMENU_NONE;
        m_confirm_remove_all = false;
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_WindowBg, PANEL_BG_COLOR);
    ImGui::PushStyleColor(ImGuiCol_Button,   TRANSPARENT_COLOR);

    // The panel
    int flags = ImGuiWindowFlags_NoCollapse  | ImGuiWindowFlags_NoResize    | ImGuiWindowFlags_NoMove
              | ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::SetNextWindowContentSize(ImVec2(panel_target_width, 0.f));
    ImGui::SetNextWindowPos(window_target_pos);
    if (!ImGui::Begin("Top menubar", nullptr, static_cast<ImGuiWindowFlags_>(flags)))
    {
        ImGui::PopStyleColor(2);
        return;
    }

    // The 'simulation' button
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 sim_cursor = ImGui::GetCursorPos();
    ImGui::Button(sim_title);
    if ((m_open_menu != TopMenu::TOPMENU_SIM) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_SIM;
    }

    ImGui::SameLine();

    // The 'vehicles' button
    ImVec2 actors_cursor = ImGui::GetCursorPos();
    ImGui::Button(actors_title);
    if ((m_open_menu != TopMenu::TOPMENU_ACTORS) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_ACTORS;
    }

    ImGui::SameLine();

    // The 'tools' button
    ImVec2 tools_cursor = ImGui::GetCursorPos();
    ImGui::Button(tools_title);
    if ((m_open_menu != TopMenu::TOPMENU_TOOLS) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_TOOLS;
    }

    ImGui::End();

    ImVec2 menu_pos;
    Actor* current_actor = App::GetSimController()->GetPlayerActor();
    switch (m_open_menu)
    {
    case TopMenu::TOPMENU_SIM:
        menu_pos.y = window_pos.y + sim_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = sim_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin("Sim menu", nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            // TODO: Display hotkeys on the right side of the menu (with different text color)

            if (ImGui::Button("Get new vehicle"))
            {
                App::sim_state.SetActive(SimState::SELECTING); // TODO: use 'pending' mechanism
                App::GetGuiManager()->GetMainSelector()->Show(LT_AllBeam);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (ImGui::Button("Show vehicle description")) // TODO: make button disabled (fake it!) when no active vehicle
            {
                if (current_actor != nullptr)
                {
                    App::GetGuiManager()->SetVisible_VehicleDescription(true);
                }
            }

            if (ImGui::Button("View spawner log")) // TODO: display num. warnings/errors
            {
                App::GetGuiManager()->SetVisible_SpawnerReport(true);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (ImGui::Button("Reload current vehicle")) // TODO: make button disabled (fake it!) when no active vehicle
            {
                if ((current_actor != nullptr) && (current_actor->ar_sim_state != Actor::SimState::NETWORKED_OK))
                {
                    ActorModifyRequest rq;
                    rq.amr_type = ActorModifyRequest::Type::RELOAD;
                    rq.amr_actor = current_actor;
                    App::GetSimController()->QueueActorModify(rq);

                    App::GetGuiManager()->UnfocusGui();
                }
            }

            if (ImGui::Button("Remove current vehicle")) // TODO: make button disabled (fake it!) when no active vehicle
            {
                if ((current_actor != nullptr) && (current_actor->ar_sim_state != Actor::SimState::NETWORKED_OK))
                {
                    App::GetSimController()->QueueActorRemove(current_actor);
                }
            }

            if (ImGui::Button("Remove all vehicles")) // TODO: make button disabled (fake it!) when no active vehicle
            {
                m_confirm_remove_all = true;
            }
            if (m_confirm_remove_all)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ORANGE_TEXT);
                if (ImGui::Button(" [!] Confirm removal"))
                {
                    for (auto actor : App::GetSimController()->GetActors())
                    {
                        if (!actor->ar_hide_in_actor_list && !actor->isPreloadedWithTerrain() && 
                                actor->ar_sim_state != Actor::SimState::NETWORKED_OK)
                        {
                            App::GetSimController()->QueueActorRemove(actor);
                        }
                    }
                    m_confirm_remove_all = false;
                }
                ImGui::PopStyleColor();
            }

            if (App::mp_state.GetActive() != MpState::CONNECTED) // Singleplayer only!
            {
                if (ImGui::Button("Activate all vehicles"))
                {
                    App::GetSimController()->GetBeamFactory()->WakeUpAllActors();
                }

                bool force_trucks_active = App::GetSimController()->GetBeamFactory()->AreTrucksForcedAwake();
                if (ImGui::Checkbox("Activated vehicles never sleep", &force_trucks_active))
                {
                    App::GetSimController()->GetBeamFactory()->SetTrucksForcedAwake(force_trucks_active);
                }

                if (ImGui::Button("Send all vehicles to sleep"))
                {
                    if (current_actor != nullptr) // Get out first
                    {
                        App::GetSimController()->SetPendingPlayerActor(nullptr);
                    }
                    App::GetSimController()->GetBeamFactory()->SendAllActorsSleeping();
                }
            }

            ImGui::Separator();

            if (ImGui::Button("Back to menu"))
            {
                App::app_state.SetPending(RoR::AppState::MAIN_MENU);
            }

            if (ImGui::Button("Exit"))
            {
                App::app_state.SetPending(RoR::AppState::SHUTDOWN);
            }

            m_open_menu_hoverbox_min = menu_pos;
            m_open_menu_hoverbox_max.x = menu_pos.x + ImGui::GetWindowWidth();
            m_open_menu_hoverbox_max.y = menu_pos.y + ImGui::GetWindowHeight();
            ImGui::End();
        }
        break;

    case TopMenu::TOPMENU_ACTORS:
        menu_pos.y = window_pos.y + actors_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = actors_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin("Actors menu", nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            if (App::mp_state.GetActive() != MpState::CONNECTED)
            {
                this->DrawActorListSinglePlayer();
            }
            else
            {
#ifdef USE_SOCKETW
                RoRnet::UserInfo net_user_info = RoR::Networking::GetLocalUserData();
                this->DrawMpUserToActorList(net_user_info);

                std::vector<RoRnet::UserInfo> remote_users = RoR::Networking::GetUserInfos();
                for (auto& user: remote_users)
                {
                    this->DrawMpUserToActorList(user);
                }
#endif // USE_SOCKETW
            }
            m_open_menu_hoverbox_min = menu_pos;
            m_open_menu_hoverbox_max.x = menu_pos.x + ImGui::GetWindowWidth();
            m_open_menu_hoverbox_max.y = menu_pos.y + ImGui::GetWindowHeight();
            ImGui::End();
        }
        break;

    case TopMenu::TOPMENU_TOOLS:
        menu_pos.y = window_pos.y + tools_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = tools_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin("Tools menu", nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            if (ImGui::Button("Friction settings"))
            {
                App::GetGuiManager()->SetVisible_FrictionSettings(true);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (ImGui::Button("Show Console"))
            {
                App::GetGuiManager()->SetVisible_Console(! App::GetGuiManager()->IsVisible_Console());
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (ImGui::Button("Texture Tool"))
            {
                App::GetGuiManager()->SetVisible_TextureToolWindow(true);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            ImGui::Separator();
            ImGui::TextColored(GRAY_HINT_TEXT, "Pre-spawn diag. options:");

            bool diag_mass = App::diag_truck_mass.GetActive();
            if (ImGui::Checkbox("Node mass recalc. logging", &diag_mass))
            {
                App::diag_truck_mass.SetActive(diag_mass);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Extra logging on runtime - mass recalculation (config: \"Debug Truck Mass\"; GVar: \"diag_truck_mass\")");
                ImGui::EndTooltip();
            }

            bool diag_break = App::diag_log_beam_break.GetActive();
            if (ImGui::Checkbox("Beam break logging", &diag_break))
            {
                App::diag_log_beam_break.SetActive(diag_break);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Extra logging on runtime (config: \"Beam Break Debug\"; GVar: \"diag_log_beam_break\")");
                ImGui::EndTooltip();
            }

            bool diag_deform = App::diag_log_beam_deform.GetActive();
            if (ImGui::Checkbox("Beam deform. logging", &diag_deform))
            {
                App::diag_log_beam_deform.SetActive(diag_deform);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Extra logging on runtime (config: \"Beam Deform Debug\"; GVar: \"diag_log_beam_deform\")");
                ImGui::EndTooltip();
            }

            bool diag_trig = App::diag_log_beam_trigger.GetActive();
            if (ImGui::Checkbox("Trigger logging", &diag_trig))
            {
                App::diag_log_beam_trigger.SetActive(diag_trig);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Extra logging on runtime - trigger beams activity (config: \"Trigger Debug\"; GVar: \"diag_log_beam_trigger\")");
                ImGui::EndTooltip();
            }

            bool diag_vcam = App::diag_videocameras.GetActive();
            if (ImGui::Checkbox("VideoCamera direction marker", &diag_vcam))
            {
                App::diag_videocameras.SetActive(diag_vcam);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Visual marker of VideoCameras direction (config: \"VideoCameraDebug\"; GVar: \"diag_videocameras\")");
                ImGui::EndTooltip();
            }

            if (current_actor != nullptr)
            {
                ImGui::Separator();

                ImGui::TextColored(GRAY_HINT_TEXT, "Live diagnostic views:");
                ImGui::TextColored(GRAY_HINT_TEXT, "(Toggle with 'K')"); // !!TODO!! - display actual configured hotkey (EV_COMMON_TOGGLE_DEBUG_VIEW)
                ImGui::TextColored(GRAY_HINT_TEXT, "(Cycle with 'CTRL+K')"); // !!TODO!! - display actual configured hotkey (EV_COMMON_CYCLE_DEBUG_VIEWS)

                int debug_view_type = static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_NONE);
                if (current_actor != nullptr)
                {
                    debug_view_type = static_cast<int>(current_actor->GetGfxActor()->GetDebugView());
                }
                ImGui::RadioButton("Normal view",     &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_NONE));
                ImGui::RadioButton("Skeleton view",   &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_SKELETON));
                ImGui::RadioButton("Node details",    &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_NODES));
                ImGui::RadioButton("Beam details",    &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_BEAMS));
                if (current_actor->ar_num_wheels > 0)
                {
                    ImGui::RadioButton("Wheel details",   &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_WHEELS));
                }
                if (current_actor->ar_num_shocks > 0)
                {
                    ImGui::RadioButton("Shock details",   &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_SHOCKS));
                }
                if (current_actor->ar_num_rotators > 0)
                {
                    ImGui::RadioButton("Rotator details", &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_ROTATORS));
                }
                if (current_actor->ar_num_cabs > 0)
                {
                    ImGui::RadioButton("Submesh details", &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_SUBMESH));
                }

                if ((current_actor != nullptr) && (debug_view_type != static_cast<int>(current_actor->GetGfxActor()->GetDebugView())))
                {
                    current_actor->GetGfxActor()->SetDebugView(static_cast<GfxActor::DebugViewType>(debug_view_type));
                }
            }

            m_open_menu_hoverbox_min = menu_pos;
            m_open_menu_hoverbox_max.x = menu_pos.x + ImGui::GetWindowWidth();
            m_open_menu_hoverbox_max.y = menu_pos.y + ImGui::GetWindowHeight();
            ImGui::End();
        }
        break;

    default:
        m_open_menu_hoverbox_min = ImVec2(0,0);
        m_open_menu_hoverbox_max = ImVec2(0,0);
    }

    ImGui::PopStyleColor(2);
}

bool RoR::GUI::TopMenubar::ShouldDisplay(ImVec2 window_pos)
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

void RoR::GUI::TopMenubar::DrawMpUserToActorList(RoRnet::UserInfo &user)
{
    // Count actors owned by the player
    unsigned int num_actors_player = 0;
    for (Actor* actor : App::GetSimController()->GetActors())
    {
        if (actor->ar_net_source_id == user.uniqueid)
        {
            ++num_actors_player;
        }
    }

    // Prepare user info text
    const char* user_type_str = "";
         if (user.authstatus & RoRnet::AUTH_BOT)     { user_type_str = "Bot, ";       } // Old coloring: #0000c9
    else if (user.authstatus & RoRnet::AUTH_BANNED)  { user_type_str = "Banned, ";    } // Old coloring: none
    else if (user.authstatus & RoRnet::AUTH_RANKED)  { user_type_str = "Ranked, ";    } // Old coloring: #00c900
    else if (user.authstatus & RoRnet::AUTH_MOD)     { user_type_str = "Moderator, "; } // Old coloring: #c90000
    else if (user.authstatus & RoRnet::AUTH_ADMIN)   { user_type_str = "Admin, ";     } // Old coloring: #c97100

    char usertext_buf[400];
    snprintf(usertext_buf, 400, "%s: %u (%sVer: %s, Lang: %s)",
        user.username, num_actors_player, user_type_str, user.clientversion, user.language);

    // Display user in list
    Ogre::ColourValue player_color;
#ifdef USE_SOCKETW
    player_color = RoR::Networking::GetPlayerColor(user.colournum);
#endif
    ImVec4 player_gui_color(player_color.r, player_color.g, player_color.b, 1.f);
    ImGui::PushStyleColor(ImGuiCol_Text, player_gui_color);
    ImGui::Text("%s", usertext_buf);
    ImGui::PopStyleColor();

    // Display actor list
    for (auto actor : App::GetSimController()->GetActors())
    {
        if ((!actor->ar_hide_in_actor_list) && (actor->ar_net_source_id == user.uniqueid))
        {
            char actortext_buf[400];
            snprintf(actortext_buf, 400, "  + %s (%s)", actor->ar_design_name.c_str(), actor->ar_filename.c_str());
            if (ImGui::Button(actortext_buf)) // Button clicked?
            {
                App::GetSimController()->SetPendingPlayerActor(actor);
            }
        }
    }
}

void RoR::GUI::TopMenubar::DrawActorListSinglePlayer()
{
    std::vector<Actor*> actor_list;
    for (auto actor : App::GetSimController()->GetActors())
    {
        if (!actor->ar_hide_in_actor_list)
        {
            actor_list.emplace_back(actor);
        }
    }
    if (actor_list.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GRAY_HINT_TEXT);
        ImGui::Text("None spawned yet");
        ImGui::Text("Use [Simulation] menu");
        ImGui::PopStyleColor();
    }
    else
    {
        Actor* player_actor = App::GetSimController()->GetPlayerActor();
        int i = 0;
        for (auto actor : actor_list)
        {
            char text_buf[200];
            snprintf(text_buf, 200, "[%d] %s", i++, actor->ar_design_name.c_str());
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
            if (ImGui::Button(text_buf)) // Button clicked?
            {
                App::GetSimController()->SetPendingPlayerActor(actor);
            }
            ImGui::PopStyleColor();
        }
    }
}
