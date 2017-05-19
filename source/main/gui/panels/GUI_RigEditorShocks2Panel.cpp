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
    @date   01/2015
*/

#include "GUI_RigEditorShocks2Panel.h"
#include "RigDef_File.h" // For shock flags
#include "RigEditor_Config.h"
#include "RigEditor_RigElementsAggregateData.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------

#define SETUP_EDITBOX(NAME, UNIFLAG, SRC, SRC_TYPE) \
    SetupEditboxField(&NAME##_field, NAME##_label, NAME##_editbox, \
        m_data.GetFlagsPtr(), RigEditor::RigAggregateShocks2Data::UNIFLAG, ((void*) &(SRC)), EditboxFieldSpec::SRC_TYPE);

#define SETUP_FLAG_CHECKBOX(FLAG_LETTER, TOOLTIP)                                        \
    SetupFlagCheckbox(                                                                   \
        m_flag_##FLAG_LETTER##_checkbox,                                                 \
        m_data.GetFlagsPtr(),                                                            \
        RigEditor::RigAggregateShocks2Data::QUERY_FLAG_##FLAG_LETTER##_IS_UNIFORM, \
        RigEditor::RigAggregateShocks2Data::VALUE_FLAG_##FLAG_LETTER,              \
        m_flags_tooltip_label,                                                           \
        TOOLTIP                                                                          \
    );

RigEditorShocks2Panel::RigEditorShocks2Panel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config):
    RigEditor::RigElementGuiPanelBase(rig_editor_interface, config, m_shocks2_panel, m_flag_i_checkbox)
{
    m_flags_tooltip_label->setTextColour(m_text_color_tooltip);
    
    // Node flag checkboxes - set user data
    SETUP_FLAG_CHECKBOX( i,  "Invisible beam");
    SETUP_FLAG_CHECKBOX( s,  "Soft bump boundaries");
    SETUP_FLAG_CHECKBOX( m,  "Metric units ~extension");
    SETUP_FLAG_CHECKBOX( M,  "Abs. metric units ~extension");

    // Fields
    SETUP_EDITBOX( m_detacher_group     , QUERY_DETACHER_GROUP_IS_UNIFORM	  , m_data.detacher_group     , SOURCE_DATATYPE_INT);
    SETUP_EDITBOX( m_spring_in          , QUERY_SPRING_IN_RATE_IS_UNIFORM	  , m_data.spring_in_rate     , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX( m_damp_in            , QUERY_DAMP_IN_RATE_IS_UNIFORM		  , m_data.damp_in_rate       , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX( m_spring_out         , QUERY_SPRING_OUT_RATE_IS_UNIFORM	  , m_data.spring_out_rate    , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX( m_damp_out           , QUERY_DAMP_OUT_RATE_IS_UNIFORM	  , m_data.damp_out_rate      , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX( m_spring_in_progress , QUERY_SPRING_IN_PROGRESS_IS_UNIFORM , m_data.spring_in_progress , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX( m_damp_in_progress   , QUERY_DAMP_IN_PROGRESS_IS_UNIFORM	  , m_data.damp_in_progress   , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX( m_spring_out_progress, QUERY_SPRING_OUT_PROGRESS_IS_UNIFORM, m_data.spring_out_progress, SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX( m_damp_out_progress  , QUERY_DAMP_OUT_PROGRESS_IS_UNIFORM  , m_data.damp_out_progress  , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX( m_contraction_limit  , QUERY_CONTRACTION_LIMIT_IS_UNIFORM  , m_data.contraction_limit  , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX( m_extension_limit    , QUERY_EXTENSION_LIMIT_IS_UNIFORM	  , m_data.extension_limit    , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX( m_precompression     , QUERY_PRECOMPRESSION_IS_UNIFORM     , m_data.precompression     , SOURCE_DATATYPE_FLOAT); 
    
    AlignToScreen(&config->gui_shocks2_panel_position);
}

// ----------------------------------------------------------------------------
// Data manipulation
// ----------------------------------------------------------------------------

void RigEditorShocks2Panel::UpdateShock2Data(RigEditor::RigAggregateShocks2Data* data)
{
    m_data = *data;

    // Panel name
    if (data->num_selected == 1)
    {
        m_panel_widget->setCaption("Shock");
    }
    else
    {
        char caption[30];
        sprintf(caption, "Shocks [%d]", data->num_selected);
        m_panel_widget->setCaption(caption);
    }

    // Flags
    UpdateFlagCheckbox(m_flag_i_checkbox, data->HasFlag_i(), !data->IsFlagUniform_i());
    UpdateFlagCheckbox(m_flag_s_checkbox, data->HasFlag_s(), !data->IsFlagUniform_s());
    UpdateFlagCheckbox(m_flag_m_checkbox, data->HasFlag_m(), !data->IsFlagUniform_m());
    UpdateFlagCheckbox(m_flag_M_checkbox, data->HasFlag_M(), !data->IsFlagUniform_M());
    
    // Fields
    EditboxRestoreValue(&m_detacher_group_field);     
    EditboxRestoreValue(&m_spring_in_field);          
    EditboxRestoreValue(&m_damp_in_field);            
    EditboxRestoreValue(&m_spring_out_field);         
    EditboxRestoreValue(&m_damp_out_field);           
    EditboxRestoreValue(&m_spring_in_progress_field); 
    EditboxRestoreValue(&m_damp_in_progress_field);   
    EditboxRestoreValue(&m_spring_out_progress_field);
    EditboxRestoreValue(&m_damp_out_progress_field);  
    EditboxRestoreValue(&m_contraction_limit_field);  
    EditboxRestoreValue(&m_extension_limit_field);    
    EditboxRestoreValue(&m_precompression_field);     
}
