/*
    This source file is part of Rigs of Rods
    Copyright 2020 tritonas00
    Copyright 2021 - 2026 Petr Ohlidal

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

    void SetVisible(bool visible);
    bool IsVisible() const { return m_is_visible; }
    bool IsHovered() const { return m_is_hovered; }

    bool IsInteractiveKeyBindingActive() { return m_interactive_keybinding_active; }
    void Draw(float dt);

private:
    void DrawEventEditBox();             //!< Only the editing UI, embeddable.
    void DrawEvent(RoR::events ev_code); //!< One line in table
    void DrawControlsTab(const char* prefix); //!< Draws table with events matching prefix.
    void DrawControlsTabItem(const char* name, const char* prefix); //!< Wraps `DrawControlsTab()` with scrollbar and tabs-bar logic.
    void DrawMenubar();
    void DrawPreviewControls();
    void DrawPovPreview(int pov_index, int pov_direction);
    void DrawInteractiveKeybindDigital();
    void DrawInteractiveButtonBinding();

    // Edit bindings (used for both expert and interactive modes)
    void UpdateInteractiveKeybinding();
    void ApplyChanges();
    void CancelChanges();

    void SaveMapFile();
    void ReloadMapFile();

    bool ShouldDisplay(event_trigger_t& trig);
    void UpdateFlashingColor(float dt);

    bool m_is_visible = false;
    bool m_is_hovered = false;
    float m_colum_widths[3] = {}; //!< body->header width sync
    Ogre::ColourValue m_flashing_color = Ogre::ColourValue(1.f, 0.5f, 0.5f, 1.f);
    float m_flashing_timer = 0.f;

    // Mode/config file selection
    int m_active_mapping_deviceid = InputEngine::DEFAULT_MAPFILE_DEVICEID;
    bool m_unsaved_changes = false;
    bool m_preview_controls = false;

    // Editing context
    RoR::events      m_active_event = events::EV_MODE_LAST; // Invalid
    event_trigger_t* m_active_trigger = nullptr;
    eventtypes       m_selected_evtype = eventtypes::ET_NONE;
    Str<1000>        m_active_buffer;
    bool             m_interactive_keybinding_active = false;
    bool             m_interactive_keybinding_expl = true;
    bool             m_interactive_keybinding_analog = false;
    bool             m_interactive_keybinding_delete_on_cancel = false;
};

} // namespace GUI
} // namespace RoR
