/*
    This source file is part of Rigs of Rods
    Copyright 2020 tritonas00

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

#pragma once

#include "InputEngine.h"

namespace RoR {
namespace GUI {

class GameControls
{
public:
    const ImVec4 GRAY_HINT_TEXT = ImVec4(0.62f, 0.62f, 0.61f, 1.f);

    void SetVisible(bool vis);
    bool IsVisible() const { return m_is_visible; }
    void Draw();

private:
    void DrawEvent(RoR::events ev_code); //!< One line in table
    void DrawControlsTab(const char* prefix); //!< Draws table with events matching prefix.
    void DrawControlsTabItem(const char* name, const char* prefix); //!< Wraps `DrawControlsTab()` with scrollbar and tabs-bar logic.

    void ApplyChanges();
    void CancelChanges();

    bool m_is_visible = false;

    // Editing context
    RoR::events m_active_event = events::EV_MODE_LAST; // Invalid
    Str<1000> m_active_buffer;
};

} // namespace GUI
} // namespace RoR
