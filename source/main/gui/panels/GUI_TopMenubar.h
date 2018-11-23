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

#pragma once

#include "RoRnet.h"

#include <imgui.h>

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

    enum class TopMenu { TOPMENU_NONE, TOPMENU_SIM, TOPMENU_ACTORS, TOPMENU_SAVEGAMES, TOPMENU_PROJECTS, TOPMENU_SETTINGS, TOPMENU_TOOLS };

    TopMenubar(): m_open_menu(TopMenu::TOPMENU_NONE), m_daytime(0), m_quickload(false), m_confirm_remove_all(false) {}

    void Update();
    bool ShouldDisplay(ImVec2 window_pos);

    bool IsVisible() { return m_open_menu != TopMenu::TOPMENU_NONE; };

private:
    void DrawActorListSinglePlayer();
    void DrawMpUserToActorList(RoRnet::UserInfo &user); // Multiplayer
    void DrawSpecialStateBox(float top_offset);

    ImVec2  m_open_menu_hoverbox_min;
    ImVec2  m_open_menu_hoverbox_max;
    TopMenu m_open_menu;
    bool    m_confirm_remove_all;

    float   m_daytime;
    bool    m_quickload;
    std::string m_quicksave_name;
    std::vector<std::string> m_savegame_names;
};

} // namespace GUI
} // namespace RoR
