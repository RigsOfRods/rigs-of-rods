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

#include "GUI_ThemeEditor.h"

#include "Application.h"
#include "Actor.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GUITheme.h"
#include "InputEngine.h"
#include "Language.h"

#include <fmt/format.h>
#include <imgui.h>

using namespace RoR;
using namespace GUI;

void ThemeEditor::Draw()
{
    ImGui::SetNextWindowPosCenter(ImGuiCond_Appearing);
    
    if (!ImGui::Begin("ThemeEditor", &m_is_visible))
    {
        ImGui::End(); // The window is collapsed
        return;
    }

    GUITheme& theme = App::GetGuiManager()->GetTheme();

    if (ImGui::CollapsingHeader(_LC("ThemeEditor", "Text"), ImGuiTreeNodeFlags_DefaultOpen))
    {
        this->EditColor(theme.in_progress_text_color    , _LC("UITheme", "In progress"));
        this->EditColor(theme.no_entries_text_color     , _LC("UITheme", "No entries"));
        this->EditColor(theme.error_text_color          , _LC("UITheme", "Error"));
        this->EditColor(theme.selected_entry_text_color , _LC("UITheme", "Selected entry"));
        this->EditColor(theme.value_red_text_color      , _LC("UITheme", "Red"));
        this->EditColor(theme.value_blue_text_color     , _LC("UITheme", "Blue"));
        this->EditColor(theme.highlight_text_color      , _LC("UITheme", "Highlighted"));
        this->EditColor(theme.success_text_color        , _LC("UITheme", "Success"));
        this->EditColor(theme.warning_text_color        , _LC("UITheme", "Warning"));
        this->EditColor(theme.help_text_color           , _LC("UITheme", "Help"));
    }

    if (ImGui::CollapsingHeader(_LC("ThemeEditor", "Windows"), ImGuiTreeNodeFlags_DefaultOpen))
    {
        this->EditColor(theme.semitransparent_window_bg , _LC("UITheme", "Semitrans BG"));
        this->EditColor(theme.semitrans_text_bg_color   , _LC("UITheme", "Semitrans text BG"));
        ImGui::TextDisabled(_LC("ThemeEditor", "If all RGB components are darker than this, text is auto-lightened."));
        this->EditColor(theme.color_mark_max_darkness   , _LC("UITheme", "Max text darkness"));

    }

    if (ImGui::CollapsingHeader(_LC("ThemeEditor", "Mouse scene interaction")))
    {
        // Mouse pick of nodes
        this->EditFloat(theme.node_circle_num_segments           , _LC("UITheme", "Circle num segments"));   // float  
        this->EditColor(theme.mouse_minnode_color                , _LC("UITheme", "Hovered node color"));   // ImVec4 
        this->EditFloat(theme.mouse_minnode_thickness            , _LC("UITheme", "Hovered node thickness"));   // float  
        this->EditColor(theme.mouse_highlighted_node_color       , _LC("UITheme", "Highlighted node color"));   // ImVec4 
        this->EditFloat(theme.mouse_node_highlight_ref_distance  , _LC("UITheme", "Node highlight ref distance"));   // float  
        this->EditFloat(theme.mouse_node_highlight_aabb_padding  , _LC("UITheme", "Node highlight AABB padding"));   // float  
        this->EditFloat(theme.mouse_highlighted_node_radius_max  , _LC("UITheme", "Highlighted node radius max"));   // float  
        this->EditFloat(theme.mouse_highlighted_node_radius_min  , _LC("UITheme", "Highlighted node radius min"));   // float  
        this->EditColor(theme.mouse_beam_close_color             , _LC("UITheme", "Beam color close"));   // ImVec4 
        this->EditColor(theme.mouse_beam_far_color               , _LC("UITheme", "Beam color far"));   // ImVec4 
        this->EditFloat(theme.mouse_beam_thickness               , _LC("UITheme", "Beam thickness"));   // float  
        this->EditFloat(theme.mouse_beam_traversal_length        , _LC("UITheme", "Beam traversal length"));   // float  

        // Node effects
        this->EditColor(theme.node_effect_force_line_color       , _LC("UITheme", "Force line color"));   // ImVec4 
        this->EditFloat(theme.node_effect_force_line_thickness   , _LC("UITheme", "Force line thickness"));   // float  
        this->EditColor(theme.node_effect_force_circle_color     , _LC("UITheme", "Affector circle color"));   // ImVec4 
        this->EditFloat(theme.node_effect_force_circle_radius    , _LC("UITheme", "Affector circle radius"));   // float  
        this->EditColor(theme.node_effect_highlight_line_color   , _LC("UITheme", "Affector highlight color"));   // ImVec4 
    }

    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();
}

void ThemeEditor::EditColor(ImVec4& color, const char* desc)
{
    ImGui::ColorEdit4(desc, (float*)&color);
}

void ThemeEditor::EditFloat(float& val, const char* desc)
{
    ImGui::InputFloat(desc, &val);
}
