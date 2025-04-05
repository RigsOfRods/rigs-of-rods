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

/// Flexbody and prop diagnostic
class FlexbodyDebug
{
public:
    bool IsVisible() const { return m_is_visible; }
    bool IsHovered() const { return m_is_hovered; }
    void SetVisible(bool value) { m_is_visible = value; m_is_hovered = false; }
    void Draw();
    bool IsHideOtherElementsModeActive() const { return hide_other_elements; }

    void AnalyzeFlexbodies(); //!< populates the combobox
    void DrawDebugView(FlexBody* flexbody, Prop* prop, NodeNum_t node_ref, NodeNum_t node_x, NodeNum_t node_y);

private:

    void UpdateVisibility();
    void DrawMemoryOrderGraph(FlexBody* flexbody);
    void DrawLocatorsTable(FlexBody* flexbody, bool& locators_visible);
    void DrawMeshInfo(FlexBody* flexbody);
    void DrawMeshInfo(Prop* prop);

    // Display options
    bool draw_mesh_wireframe = false;
    bool show_base_nodes = false;
    bool show_forset_nodes = false;
    bool show_vertices = false;
    bool hide_other_elements = false;
    std::vector<bool> show_locator;
    int hovered_vert = -1;

    // Flexbody and prop selection combobox
    std::string m_combo_items; //!< Flexbodies come first, props second
    int m_combo_props_start = -1; //!< Index of first prop in the combobox. -1 means no props.
    int m_combo_selection = 0;

    // Window state
    bool m_is_visible = false;
    bool m_is_hovered = false;
};

} // namespace GUI
} // namespace RoR
