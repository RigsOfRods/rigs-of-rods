/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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

namespace RoR {

inline ImVec4 RGBAv4(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
    return ImVec4(static_cast<float>(r) / 255.f, static_cast<float>(g) / 255.f, static_cast<float>(b) / 255.f, static_cast<float>(b) / 255.f);
}

struct GUITheme
{
    ImVec4 in_progress_text_color    = ImVec4(1.f, 0.832031f, 0.f, 1.f);
    ImVec4 no_entries_text_color     = ImVec4(0.7f, 0.7f, 0.7f, 1.f);
    ImVec4 error_text_color          = ImVec4(1.f, 0.175439f, 0.175439f, 1.f);
    ImVec4 selected_entry_text_color = ImVec4(.9f, 0.7f, 0.05f, 1.f);
    ImVec4 value_red_text_color      = ImVec4(.9f, .1f, .1f, 1.f);
    ImVec4 value_blue_text_color     = ImVec4(0.34f, 0.67f, 0.84f, 1.f);
    ImVec4 highlight_text_color      = ImVec4(0.78f, 0.39f, 0.f, 1.f);
    ImVec4 success_text_color        = ImVec4(0.f, 0.8f, 0.f, 1.f);
    ImVec4 warning_text_color        = ImVec4(0.9f, 0.8f, 0.1f, 1.f);
    ImVec4 help_text_color           = ImVec4(0.5f, 0.7f, 1.f, 1.f);

    ImVec4 semitransparent_window_bg = ImVec4(0.1f, 0.1f, 0.1f, 0.8f);
    ImVec4 semitrans_text_bg_color = ImVec4(0.1f, 0.1f, 0.1f, 0.6f);
    ImVec4 color_mark_max_darkness = ImVec4(0.2, 0.2, 0.2, 0.0); //!< If all RGB components are darker than this, text is auto-lightened.

    ImVec2 screen_edge_padding = ImVec2(10.f, 10.f);
    ImVec2 semitrans_text_bg_padding = ImVec2(4.f, 2.f);

    ImFont* default_font = nullptr;

    // Mouse pick of nodes
    float  node_circle_num_segments          = 10.f;
    ImVec4 mouse_minnode_color               = RGBAv4(25, 59, 255, 255);
    float  mouse_minnode_thickness           = 3.f; //!< in pixels
    ImVec4 mouse_highlighted_node_color      = ImVec4(0.7f, 1.f, 0.4f, 1.f);
    float  mouse_node_highlight_ref_distance = 5.f;
    float  mouse_node_highlight_aabb_padding = 2.f; //!< in meters, for all directions; Inflates actor AABB in test so highlights show earlier
    float  mouse_highlighted_node_radius_max = 5; //!< in pixels
    float  mouse_highlighted_node_radius_min = 1; //!< in pixels
    ImVec4 mouse_beam_close_color            = RGBAv4(3, 148, 109, 255);
    ImVec4 mouse_beam_far_color              = RGBAv4(38, 54, 51, 180);
    float  mouse_beam_thickness              = 2.f; //!< in pixels
    float  mouse_beam_traversal_length       = 0.8f; //!< in meters

    // Node effects
    ImVec4 node_effect_force_line_color = ImVec4(0.3f, 0.2f, 1.f, 1.f);
    float  node_effect_force_line_thickness = 2.f;
    ImVec4 node_effect_force_circle_color = ImVec4(0.15f, 0.2f, 1.f, 1.f);
    float  node_effect_force_circle_radius = 4.f;
    ImVec4 node_effect_highlight_line_color = ImVec4(1.f, 1.f, 0.f, 1.f);

};

} // namespace RoR
