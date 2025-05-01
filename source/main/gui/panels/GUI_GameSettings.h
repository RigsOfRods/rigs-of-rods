/*
    This source file is part of Rigs of Rods
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

#pragma once

#include "Application.h"
#include "OgreImGui.h"

namespace RoR {
namespace GUI {

class GameSettings
{
public:
    void Draw();

    bool IsVisible() const { return m_is_visible; }
    void SetVisible(bool v);

    CacheCategoryId default_dash_being_selected = CID_None;

private:
    void DrawRenderSystemSettings();
    void DrawGeneralSettings();
    void DrawGameplaySettings();
    void DrawUiSettings();
    void DrawGraphicsSettings();
    void DrawAudioSettings();
    void DrawControlSettings();
    void DrawDiagSettings();

    // UI settings
    const float UI_SELECTOR_WIDTH = 275.0f;
    void DrawUiPresetCombo();
    void DrawUiDefaultDashboard(CacheEntryPtr& entry, CVar* cvar, CacheCategoryId category_id, const std::string& label);
    CacheEntryPtr m_ui_known_dash_truck;
    CacheEntryPtr m_ui_known_dash_boat;

    // GUI state
    bool m_is_visible = false;
    ImVec2 m_window_size = ImVec2(0, 0);

    // Buffers for text input boxes
    Str<1000> m_buf_diag_preset_terrain;
    Str<1000> m_buf_diag_preset_vehicle;
    Str<1000> m_buf_diag_preset_veh_config;
    Str<1000> m_buf_app_extra_mod_dir;
    Str<1000> m_buf_io_outgauge_ip;
    Str<1000> m_buf_audio_default_efx_preset;
    Str<1000> m_buf_audio_force_listener_efx_preset;

    // Pre-formatted combobox items
    std::string m_combo_items_gearbox_mode;
    std::string m_combo_items_light_sources;
    std::string m_combo_items_shadow_type;
    std::string m_combo_items_sky_mode;
    std::string m_combo_items_tex_filter;
    std::string m_combo_items_vegetation;
    std::string m_combo_items_water_mode;
    std::string m_combo_items_extcam_mode;
    std::string m_combo_items_input_grab;
    std::string m_combo_items_efx_reverb_engine;
    std::string m_cached_uipreset_combo_string;

    // Render settings
    bool m_render_must_restart = false;
    float m_bump_height = 0.0f;
};

} // namespace GUI
} // namespace RoR
