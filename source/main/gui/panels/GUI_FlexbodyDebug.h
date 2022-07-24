/*
    This source file is part of Rigs of Rods
    Copyright 2022 Petr Ohlidal

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

#pragma once

#include "Application.h"

#include "SimData.h"

namespace RoR {
namespace GUI {

class FlexbodyDebug
{
public:
    bool IsVisible() const { return m_is_visible; }
    bool IsHovered() const { return m_is_hovered; }
    void SetVisible(bool value) { m_is_visible = value; m_is_hovered = false; }
    void Draw();

    void AnalyzeFlexbodies(); //!< populates the combobox
    void DrawDebugView();

    // Display options
    bool draw_mesh_wireframe = false;
    bool show_base_nodes = false;
    bool show_forset_nodes = false;
    bool show_vertices = false;
    bool hide_other_flexbodies = false;
    std::vector<bool> show_locator;

private:

    void UpdateVisibility();

    // Flexbody selection combobox
    std::string m_combo_items;
    int m_combo_selection = 0;

    // Window state
    bool m_is_visible = false;
    bool m_is_hovered = false;
};

} // namespace GUI
} // namespace RoR