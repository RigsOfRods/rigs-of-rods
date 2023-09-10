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

#pragma once

#include "RoRnet.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include <rapidjson/document.h>

struct ai_events
{
    Ogre::Vector3 position;
    int speed = -1;
};

namespace RoR {
namespace GUI {

class TopMenubar
{
public:
    const float   MENU_Y_OFFSET         = 40.f;
    const float   PANEL_HOVERBOX_HEIGHT = 50.f;
    const ImVec4  GRAY_HINT_TEXT        = ImVec4(0.62f, 0.62f, 0.61f, 1.f);
    const ImVec4  WHITE_TEXT            = ImVec4(0.9f, 0.9f, 0.9f, 1.f);
    const ImVec4  GREEN_TEXT            = ImVec4(0.0f, 0.9f, 0.0f, 1.f);
    const ImVec4  ORANGE_TEXT           = ImVec4(0.9f, 0.6f, 0.0f, 1.f);
    const ImVec4  RED_TEXT              = ImVec4(1.00f, 0.00f, 0.00f, 1.f);

    enum class TopMenu { TOPMENU_NONE, TOPMENU_SIM, TOPMENU_ACTORS, TOPMENU_SAVEGAMES, TOPMENU_SETTINGS, TOPMENU_TOOLS, TOPMENU_AI };
    enum class StateBox { STATEBOX_NONE, STATEBOX_REPLAY, STATEBOX_RACE, STATEBOX_LIVE_REPAIR, STATEBOX_QUICK_REPAIR };

    TopMenubar(): m_open_menu(), m_daytime(0), m_quickload(false), m_confirm_remove_all(false) {}

    void Update();
    bool ShouldDisplay(ImVec2 window_pos);

    bool IsVisible() { return m_open_menu != TopMenu::TOPMENU_NONE; };

    // Vehicle AI
    std::vector<ai_events> ai_waypoints;
    int ai_num = 1;
    int ai_speed = 50;
    int ai_times = 1;
    int ai_altitude = 1000;
    int ai_distance = 20;
    int ai_position_scheme = 0;
    Ogre::String ai_fname = "95bbUID-agoras.truck";
    Ogre::String ai_dname = "Bus RVI Agora S";
    Ogre::String ai_sectionconfig = "";
    std::string ai_skin = "";
    bool ai_select = false;
    bool ai_rec = false;
    int ai_mode = 0;
    bool ai_menu = false;

    // Secondary selections for Drag Race and Crash driving modes
    bool ai_select2 = false;
    Ogre::String ai_fname2 = "95bbUID-agoras.truck";
    Ogre::String ai_dname2 = "Bus RVI Agora S";
    Ogre::String ai_sectionconfig2 = "";
    std::string ai_skin2 = "";

    int ai_num_prev = 1;
    int ai_speed_prev = 50;
    int ai_position_scheme_prev = 0;
    int ai_times_prev = 1;
    int ai_mode_prev = 0;

    void Refresh(std::string payload);

private:
    void DrawActorListSinglePlayer();
    void DrawMpUserToActorList(RoRnet::UserInfo &user); // Multiplayer
    void DrawSpecialStateBox(float top_offset);

    ImVec2  m_open_menu_hoverbox_min;
    ImVec2  m_open_menu_hoverbox_max;
    TopMenu m_open_menu = TopMenu::TOPMENU_NONE;

    ImVec2  m_state_box_hoverbox_min;
    ImVec2  m_state_box_hoverbox_max;
    StateBox m_state_box = StateBox::STATEBOX_NONE;

    bool    m_confirm_remove_all;

    float   m_daytime;
    float   m_waves_height;
    bool    m_quickload;
    std::string m_quicksave_name;
    std::vector<std::string> m_savegame_names;

    void GetPresets();
    rapidjson::Document j_doc;
};

} // namespace GUI
} // namespace RoR
