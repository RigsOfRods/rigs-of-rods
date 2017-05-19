/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.com/

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

/** 
    @file   
    @author Petr Ohlidal
    @date   12/2014
*/

#include "GUI_RigEditorNodePanel.h"
#include "RigEditor_Config.h"
#include "RigEditor_RigElementsAggregateData.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------

#define SETUP_FLAG_CHECKBOX(FLAG_LETTER, TOOLTIP)                                 \
    SetupFlagCheckbox(                                                            \
        m_flag_##FLAG_LETTER##_checkbox,                                          \
        m_data.GetFlagsPtr(),                                                     \
        RigEditor::RigAggregateNodesData::QUERY_FLAG_##FLAG_LETTER##_IS_UNIFORM,  \
        RigEditor::RigAggregateNodesData::VALUE_FLAG_##FLAG_LETTER,               \
        m_flag_tooltip_label,                                                     \
        TOOLTIP                                                                   \
    );

#define SETUP_EDITBOX(NAME, UNIFLAG, SRC, SRC_TYPE) \
    SetupEditboxField(&NAME##_field, NAME##_label, NAME##_editbox, \
        m_data.GetFlagsPtr(), RigEditor::RigAggregateNodesData::UNIFLAG, ((void*) &(SRC)), EditboxFieldSpec::SRC_TYPE);

RigEditorNodePanel::RigEditorNodePanel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config):
    RigEditor::RigElementGuiPanelBase(rig_editor_interface, config, m_node_panel, m_flag_m_checkbox)
{
    m_flag_tooltip_label->setTextColour(m_text_color_tooltip);
    
    // Node flag checkboxes - set user data
    SETUP_FLAG_CHECKBOX( m, "Mouse grab disabled");
    SETUP_FLAG_CHECKBOX( l, "Loaded with cargo");
    SETUP_FLAG_CHECKBOX( p, "Particle gfx disabled");
    SETUP_FLAG_CHECKBOX( b, "Extra buoyancy");
    SETUP_FLAG_CHECKBOX( h, "Hook point [legacy]");
    SETUP_FLAG_CHECKBOX( c, "Ground contact disabled");
    SETUP_FLAG_CHECKBOX( y, "Exhaust direction [legacy]");
    SETUP_FLAG_CHECKBOX( x, "Exhaust point [legacy]");
    SETUP_FLAG_CHECKBOX( f, "Sparks disabled");
    SETUP_FLAG_CHECKBOX( L, "Logging enabled");

    // Fields
    SETUP_EDITBOX(m_detacher_group , QUERY_DETACHER_GROUP_IS_UNIFORM , m_data.detacher_group_id , SOURCE_DATATYPE_INT  );
    SETUP_EDITBOX(m_loadweight     , QUERY_LOAD_WEIGHT_IS_UNIFORM    , m_data.load_weight       , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX(m_node_name      , RESERVED_NODENAME_UNIFORM       , m_data.node_name         , SOURCE_DATATYPE_STRING);
    
    AlignToScreen(&config->gui_nodes_panel_position);
}

// ----------------------------------------------------------------------------
// Data manipulation
// ----------------------------------------------------------------------------

void RigEditorNodePanel::UpdateNodeData(RigEditor::RigAggregateNodesData* query)
{
    m_data = *query;

    // Panel name & node name
    if (query->num_selected == 1)
    {
        m_node_panel->setCaption("Node");
        m_node_name_editbox->setCaption(query->node_name);
        SetNodeNameFieldVisible(true);
    }
    else
    {
        char caption[30] = "";
        sprintf(caption, "Nodes [%d]", query->num_selected);
        m_node_panel->setCaption(caption);
        m_node_name_editbox->setCaption("");
        SetNodeNameFieldVisible(false);
    }

    // Fields
    EditboxRestoreValue(&m_detacher_group_field);
    EditboxRestoreValue(&m_loadweight_field);
    
    // Preset selector
    if (query->IsPresetUniform())
    {
        m_preset_selector_combobox->setCaption(Ogre::StringConverter::toString(query->preset_id));
        m_preset_selector_label->setTextColour(m_text_color_default);
    }
    else
    {
        m_preset_selector_combobox->setCaption("");
        m_preset_selector_label->setTextColour(m_text_color_mixvalues);
    }

    // Flags
    UpdateFlagCheckbox(m_flag_m_checkbox, m_data.HasFlag_m(), !m_data.IsFlagUniform_m());
    UpdateFlagCheckbox(m_flag_l_checkbox, m_data.HasFlag_l(), !m_data.IsFlagUniform_l());
    UpdateFlagCheckbox(m_flag_p_checkbox, m_data.HasFlag_p(), !m_data.IsFlagUniform_p());
    UpdateFlagCheckbox(m_flag_b_checkbox, m_data.HasFlag_b(), !m_data.IsFlagUniform_b());
    UpdateFlagCheckbox(m_flag_h_checkbox, m_data.HasFlag_h(), !m_data.IsFlagUniform_h());
    UpdateFlagCheckbox(m_flag_c_checkbox, m_data.HasFlag_c(), !m_data.IsFlagUniform_c());
    UpdateFlagCheckbox(m_flag_y_checkbox, m_data.HasFlag_y(), !m_data.IsFlagUniform_y());
    UpdateFlagCheckbox(m_flag_x_checkbox, m_data.HasFlag_x(), !m_data.IsFlagUniform_x());
    UpdateFlagCheckbox(m_flag_f_checkbox, m_data.HasFlag_f(), !m_data.IsFlagUniform_f());
    UpdateFlagCheckbox(m_flag_L_checkbox, m_data.HasFlag_L(), !m_data.IsFlagUniform_L());
}

// ----------------------------------------------------------------------------
// GUI utility
// ----------------------------------------------------------------------------

void RigEditorNodePanel::SetNodeNameFieldVisible(bool show)
{
    m_node_name_label->setVisible(show);
    m_node_name_editbox->setVisible(show);
}
