/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2014-2017 Petr Ohlidal & contributors

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

#include <imgui.h>

namespace RoR {
namespace GUI {

class GameMainMenu
{
public:
    // This class implements hand-made keyboard focus - button count must be known for wrapping
    const int     NUM_BUTTONS           = 5; // Buttons: SinglePlayer, MultiPlayer, Settings, About, Exit
    const float   WINDOW_WIDTH          = 200.f;
    const ImVec4  WINDOW_BG_COLOR       = ImVec4(0.1f, 0.1f, 0.1f, 0.8f);
    const ImVec4  BUTTON_BG_COLOR       = ImVec4(0.25f, 0.25f, 0.24f, 0.6f); // Drawn on top of a transparent panel; make it just a shade
    const ImVec2  BUTTON_PADDING        = ImVec2(4.f, 6.f);

    GameMainMenu();

    // Keyboard updates - move up/down and wrap on top/bottom. Initial index is '-1' which means "no focus"
    inline void   KeyUpPressed()                   { m_kb_focus_index = (m_kb_focus_index <= 0) ? (NUM_BUTTONS-1) : (m_kb_focus_index - 1); }
    inline void   KeyDownPressed()                 { m_kb_focus_index = (m_kb_focus_index < (NUM_BUTTONS - 1)) ? (m_kb_focus_index + 1) : 0; }
    void          EnterKeyPressed()                { m_kb_enter_index = m_kb_focus_index; }

    inline bool   IsVisible() const                { return m_is_visible; }
    inline void   SetVisible(bool v)               { m_is_visible = v; m_kb_focus_index = -1; }
    void          Draw();

private:
    bool   m_is_visible;
    int    m_kb_focus_index; // -1 = no focus; 0+ = button index
    int    m_kb_enter_index; // Focus positon when enter key was pressed.
};

} // namespace GUI
} // namespace RoR
