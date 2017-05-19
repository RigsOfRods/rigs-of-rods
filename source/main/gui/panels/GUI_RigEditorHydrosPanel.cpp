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

#include "GUI_RigEditorHydrosPanel.h"
#include "RigEditor_Config.h"
#include "RigEditor_RigElementsAggregateData.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

#define SETUP_FLAG_CHECKBOX(FLAG_LETTER, TOOLTIP)                                  \
    SetupFlagCheckbox(                                                             \
        m_flag_##FLAG_LETTER##_checkbox,                                           \
        m_data.GetFlagsPtr(),                                                      \
        RigEditor::RigAggregateHydrosData::QUERY_FLAG_##FLAG_LETTER##_IS_UNIFORM,  \
        RigEditor::RigAggregateHydrosData::VALUE_FLAG_##FLAG_LETTER,               \
        m_flags_tooltip_label,                                                     \
        TOOLTIP                                                                    \
    );

#define SETUP_EDITBOX(NAME, UNIFLAG, SRC, SRC_TYPE) \
    SetupEditboxField(&NAME##_field, NAME##_label, NAME##_editbox, \
        m_data.GetFlagsPtr(), RigEditor::RigAggregateHydrosData::UNIFLAG, ((void*) &(SRC)), EditboxFieldSpec::SRC_TYPE);

RigEditorHydrosPanel::RigEditorHydrosPanel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config):
    RigEditor::RigElementGuiPanelBase(rig_editor_interface, config, m_beams_panel, m_flag_i_checkbox)
{
    m_flags_tooltip_label->setTextColour(m_text_color_tooltip);
    
    // Node flag checkboxes - set user data
    SETUP_FLAG_CHECKBOX( i, "Invisible hydro");
    SETUP_FLAG_CHECKBOX( s, "Disable at high speed");
    SETUP_FLAG_CHECKBOX( a, "Aileron");
    SETUP_FLAG_CHECKBOX( r, "Rudder");
    SETUP_FLAG_CHECKBOX( e, "Elevator");
    SETUP_FLAG_CHECKBOX( u, "Aileron + elevator");
    SETUP_FLAG_CHECKBOX( v, "Inv-Aileron + elevator");
    SETUP_FLAG_CHECKBOX( x, "Aileron + rudder");
    SETUP_FLAG_CHECKBOX( y, "Inv-Aileron + rudder");
    SETUP_FLAG_CHECKBOX( g, "Elevator + rudder");
    SETUP_FLAG_CHECKBOX( h, "Inv-Elevator + rudder");

    // Fields
    SETUP_EDITBOX(m_extension_factor, QUERY_EXTENSION_FACTOR_IS_UNIFORM      , m_data.extension_factor        , SOURCE_DATATYPE_FLOAT  );
    SETUP_EDITBOX(m_detacher_group  , QUERY_DETACHER_GROUP_IS_UNIFORM        , m_data.detacher_group          , SOURCE_DATATYPE_INT    );
    SETUP_EDITBOX(m_start_delay     , QUERY_INERTIA_START_DELAY_IS_UNIFORM   , m_data.inertia_start_delay     , SOURCE_DATATYPE_FLOAT  );
    SETUP_EDITBOX(m_stop_delay      , QUERY_INERTIA_STOP_DELAY_IS_UNIFORM    , m_data.inertia_stop_delay      , SOURCE_DATATYPE_FLOAT  );
    SETUP_EDITBOX(m_start_function  , QUERY_INERTIA_START_FUNCTION_IS_UNIFORM, m_data.inertia_start_function  , SOURCE_DATATYPE_STRING ); 
    SETUP_EDITBOX(m_stop_function   , QUERY_INERTIA_STOP_FUNCTION_IS_UNIFORM , m_data.inertia_stop_function   , SOURCE_DATATYPE_STRING );  
    
    AlignToScreen(&config->gui_hydros_panel_position);
}

void RigEditorHydrosPanel::UpdateHydrosData(RigEditor::RigAggregateHydrosData* data)
{
    m_data = *data;

    // Panel name
    if (data->num_selected == 1)
    {
        m_panel_widget->setCaption("Steering hydro");
    }
    else
    {
        char caption[100];
        sprintf(caption, "Steering hydros [%d]", data->num_selected);
        m_panel_widget->setCaption(caption);
    }

    // Flags
    UpdateFlagCheckbox(m_flag_i_checkbox, data->HasFlag_i(), data->IsFlagUniform_i());
    UpdateFlagCheckbox(m_flag_s_checkbox, data->HasFlag_s(), data->IsFlagUniform_s());
    UpdateFlagCheckbox(m_flag_a_checkbox, data->HasFlag_a(), data->IsFlagUniform_a());
    UpdateFlagCheckbox(m_flag_r_checkbox, data->HasFlag_r(), data->IsFlagUniform_r());
    UpdateFlagCheckbox(m_flag_e_checkbox, data->HasFlag_e(), data->IsFlagUniform_e());
    UpdateFlagCheckbox(m_flag_u_checkbox, data->HasFlag_u(), data->IsFlagUniform_u());
    UpdateFlagCheckbox(m_flag_v_checkbox, data->HasFlag_v(), data->IsFlagUniform_v());
    UpdateFlagCheckbox(m_flag_x_checkbox, data->HasFlag_x(), data->IsFlagUniform_x());
    UpdateFlagCheckbox(m_flag_y_checkbox, data->HasFlag_y(), data->IsFlagUniform_y());
    UpdateFlagCheckbox(m_flag_g_checkbox, data->HasFlag_g(), data->IsFlagUniform_g());
    UpdateFlagCheckbox(m_flag_h_checkbox, data->HasFlag_h(), data->IsFlagUniform_h());

    // Fields
    EditboxRestoreValue(&m_extension_factor_field);
    EditboxRestoreValue(&m_start_delay_field);
    EditboxRestoreValue(&m_stop_delay_field);
    EditboxRestoreValue(&m_start_function_field); 
    EditboxRestoreValue(&m_stop_function_field);  
    EditboxRestoreValue(&m_detacher_group_field);
}
