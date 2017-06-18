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
#include "BeamFactory.h"
#include "GUIManager.h"
#include "GUI_MainSelector.h"
#include "MainMenu.h"
#include "RoRFrameListener.h"
#include "Network.h"

void RoR::GUI::TopMenubar::Update()
{
    // ## ImGui's 'menubar' and 'menuitem' features won't quite cut it...
    // ## Let's do our own menus and menuitems using buttons and coloring tricks.

    const char* sim_title = "Simulation"; // TODO: Localize all!
    const char* actors_title = "Vehicles (2)"; // TODO: Dummy menu!
    const char* tools_title = "Tools";

    float panel_target_width = 
        (ImGui::GetStyle().WindowPadding.x * 2) + (ImGui::GetStyle().FramePadding.x * 2) + // Left+right window padding
        ImGui::CalcTextSize(sim_title).x + ImGui::CalcTextSize(actors_title).x + ImGui::CalcTextSize(tools_title).x +  // Items
        (ImGui::GetStyle().ItemSpacing.x * 3); // Item spacing

    ImVec2 window_target_pos = ImVec2((ImGui::GetIO().DisplaySize.x/2.f) - (panel_target_width / 2.f), ImGui::GetStyle().WindowPadding.y);
    if (!this->ShouldDisplay(window_target_pos))
    {
        m_open_menu = TopMenu::TOPMENU_NONE;
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_WindowBg, PANEL_BG_COLOR);
    ImGui::PushStyleColor(ImGuiCol_Button,   TRANSPARENT_COLOR);

    // The panel
    int flags = ImGuiWindowFlags_NoCollapse  | ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoMove
              | ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoScrollbar;
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
    Beam* current_actor = App::GetSimController()->GetBeamFactory()->getCurrentTruck();
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
                if (current_actor != nullptr)
                {
                    App::GetSimController()->ReloadCurrentActor();
                    App::GetGuiManager()->UnfocusGui();
                }
            }

            if (ImGui::Button("Remove current vehicle")) // TODO: make button disabled (fake it!) when no active vehicle
            {
                App::GetSimController()->GetBeamFactory()->removeCurrentTruck();
            }

            if (App::mp_state.GetActive() != MpState::CONNECTED) // Singleplayer only!
            {
                if (ImGui::Button("Activate all vehicles"))
                {
                    App::GetSimController()->GetBeamFactory()->activateAllTrucks();
                }

                bool force_trucks_active = App::GetSimController()->GetBeamFactory()->AreTrucksForcedActive();
                if (ImGui::Checkbox("Activated vehicles never sleep", &force_trucks_active))
                {
                    App::GetSimController()->GetBeamFactory()->setTrucksForcedActive(force_trucks_active);
                }

                if (ImGui::Button("Send all vehicles to sleep"))
                {
                    if (current_actor != nullptr) // Get out first
                    {
                        App::GetSimController()->GetBeamFactory()->setCurrentTruck(-1);
                    }
                    App::GetSimController()->GetBeamFactory()->sendAllTrucksSleeping();
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

            if (ImGui::Button("Debug Options"))
            {
                m_open_menu = TopMenu::TOPMENU_NONE;
                App::GetGuiManager()->SetVisible_DebugOptions(true);
            }

            // TODO: Make the radio buttons visible only when there's active actor
            // NOTE: Currently there seems to be a bug in IMGUI - if the window is displayed first without the radiobuttons, 
            //       it remembers the size and the radiobuttons never become visible - they get clipped out.
            ImGui::Separator();

            int debug_view_type = -1;
            if (current_actor != nullptr)
            {
                debug_view_type = current_actor->debugVisuals;
            }
            ImGui::RadioButton("no visual debug"       , &debug_view_type,  0);
            ImGui::RadioButton("show node numbers"     , &debug_view_type,  1);
            ImGui::RadioButton("show beam numbers"     , &debug_view_type,  2);
            ImGui::RadioButton("show node&beam numbers", &debug_view_type,  3);
            ImGui::RadioButton("show node mass"        , &debug_view_type,  4);
            ImGui::RadioButton("show node locked"      , &debug_view_type,  5);
            ImGui::RadioButton("show beam compression" , &debug_view_type,  6);
            ImGui::RadioButton("show beam broken"      , &debug_view_type,  7);
            ImGui::RadioButton("show beam stress"      , &debug_view_type,  8);
            ImGui::RadioButton("show beam strength"    , &debug_view_type,  9);
            ImGui::RadioButton("show beam hydros"      , &debug_view_type, 10);
            ImGui::RadioButton("show beam commands"    , &debug_view_type, 11);
            if ((current_actor != nullptr) && (debug_view_type != current_actor->debugVisuals))
            {
                current_actor->setDebugOverlayState(debug_view_type);
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
    size_t num_actors_total = App::GetSimController()->GetNumActors();
    size_t num_actors_player = 0;
    for (size_t i = 0; i < num_actors_total; ++i)
    {
        Beam* actor = App::GetSimController()->GetActorById(static_cast<int>(i));
        if ((actor != nullptr) && (actor->m_source_id == user.uniqueid))
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
    Ogre::ColourValue player_color = RoR::Networking::GetPlayerColor(user.colournum);
    ImVec4 player_gui_color(player_color.r, player_color.g, player_color.b, 1.f);
    ImGui::PushStyleColor(ImGuiCol_Text, player_gui_color);
    ImGui::Text(usertext_buf);
    ImGui::PopStyleColor();

    // Display actor list
    for (size_t i = 0; i < num_actors_total; ++i)
    {
        Beam* actor = App::GetSimController()->GetActorById(static_cast<int>(i));
        if ((actor != nullptr) && (!actor->hideInChooser) && (actor->m_source_id == user.uniqueid))
        {
            char actortext_buf[400];
            snprintf(actortext_buf, 400, "  + %s (%s)", actor->realtruckname.c_str(), actor->realtruckfilename.c_str());
            if (ImGui::Button(actortext_buf)) // Button clicked?
            {
                App::GetSimController()->SetPlayerActor(actor);
            }
        }
    }
}

void RoR::GUI::TopMenubar::DrawActorListSinglePlayer()
{
    size_t num_actors = App::GetSimController()->GetNumActors();
    if (num_actors == 0)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ACTORLIST_GRAY_TEXT);
        ImGui::Text("None spawned yet");
        ImGui::Text("Use [Simulation] menu");
        ImGui::PopStyleColor();
        return;
    }

    for (size_t i = 0; i < num_actors; ++i)
    {
        Beam* actor = App::GetSimController()->GetActorById(i);
        if ((actor != nullptr) && (!actor->hideInChooser))
        {
            char text_buf[200];
            snprintf(text_buf, 200, "[%d] %s", i, actor->realtruckname.c_str());
            if (ImGui::Button(text_buf)) // Button clicked?
            {
                App::GetSimController()->SetPlayerActor(actor);
            }
        }
    }
}
