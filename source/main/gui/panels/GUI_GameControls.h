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
    const ImVec4      GRAY_HINT_TEXT = ImVec4(0.62f, 0.62f, 0.61f, 1.f);
    const std::string MAPFILE_COMBO_ALL = "<All files>";
    const int         MAPFILE_ID_ALL = -2;
    const int         MAPFILE_ID_DEFAULT = -1;

    void SetVisible(bool vis);
    bool IsVisible() const { return m_is_visible; }
    void Draw();

private:
    void DrawEventEditBox();             //!< Only the editing UI, embeddable.
    void DrawEvent(RoR::events ev_code); //!< One line in table
    void DrawControlsTab(const char* prefix); //!< Draws table with events matching prefix.
    void DrawControlsTabItem(const char* name, const char* prefix); //!< Wraps `DrawControlsTab()` with scrollbar and tabs-bar logic.
    void DrawToolbar();

    void ApplyChanges();
    void CancelChanges();
    void SaveMapFile();
    void ReloadMapFile();

    std::string const& GetFileComboLabel(int file_id);
    bool               ShouldDisplay(event_trigger_t& trig);

    bool m_is_visible = false;
    bool m_show_empty = true;

    // Config file selection
    int m_active_mapping_file = -2; // -2 = all, -1 = default (input.map), 0+ = device specific map file.

    // Editing context
    RoR::events      m_active_event = events::EV_MODE_LAST; // Invalid
    event_trigger_t* m_active_trigger = nullptr;
    eventtypes       m_selected_evtype = eventtypes::ET_NONE;
    Str<1000>        m_active_buffer;
};

} // namespace GUI
} // namespace RoR
