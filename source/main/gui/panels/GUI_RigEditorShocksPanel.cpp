/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors.

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

#include "GUI_RigEditorShocksPanel.h"
#include "RigEditor_Config.h"
#include "RigEditor_RigElementsAggregateData.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

#define SUPERCLASS RigEditor::RigElementGuiPanelBase

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------

#define SETUP_EDITBOX(NAME, UNIFLAG, SRC, SRC_TYPE) \
    SetupEditboxField(&NAME##_field, NAME##_label, NAME##_editbox, \
        m_data.GetFlagsPtr(), RigAggregateShocksData::UNIFLAG, ((void*) &(SRC)), EditboxFieldSpec::SRC_TYPE);

#define SETUP_FLAG_CHECKBOX(FLAG_LETTER, TOOLTIP)                                        \
    SetupFlagCheckbox(                                                                   \
        m_flag_##FLAG_LETTER##_checkbox,                                                 \
        m_data.GetFlagsPtr(),                                                            \
        RigEditor::RigAggregateShocksData::QUERY_FLAG_##FLAG_LETTER##_IS_UNIFORM,  \
        RigEditor::RigAggregateShocksData::VALUE_FLAG_##FLAG_LETTER,               \
        m_flags_tooltip_label,                                                           \
        TOOLTIP                                                                          \
    );

RigEditorShocksPanel::RigEditorShocksPanel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config):
    SUPERCLASS(rig_editor_interface, config, m_shocks_panel, m_flag_i_checkbox)
{
    using namespace RigEditor;

    m_flags_tooltip_label->setTextColour(m_text_color_tooltip);
    
    // Node flag checkboxes - set user data
    SETUP_FLAG_CHECKBOX( i, "Invisible beam");
    SETUP_FLAG_CHECKBOX( R, "Stability active, left");
    SETUP_FLAG_CHECKBOX( L, "Stability active, right");

    SETUP_EDITBOX(m_detacher_group   , QUERY_DETACHER_GROUP_IS_UNIFORM   , m_data.detacher_group   , SOURCE_DATATYPE_INT  );
    SETUP_EDITBOX(m_spring           , QUERY_SPRING_IS_UNIFORM           , m_data.spring_rate      , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX(m_damping          , QUERY_DAMPING_IS_UNIFORM          , m_data.damping          , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX(m_contraction_limit, QUERY_CONTRACTION_LIMIT_IS_UNIFORM, m_data.contraction_limit, SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX(m_extension_limit  , QUERY_EXTENSION_LIMIT_IS_UNIFORM  , m_data.extension_limit  , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX(m_precompression   , QUERY_PRECOMPRESSION_IS_UNIFORM   , m_data.precompression   , SOURCE_DATATYPE_FLOAT);

    m_beam_type_combobox->eventKeySetFocus += MyGUI::newDelegate(this, &RigEditorShocksPanel::Shocks_CallbackKeyFocusGained_RestorePreviousFieldValue);
    
    AlignToScreen(&config->gui_shocks_panel_position);
}

// ----------------------------------------------------------------------------
// GUI events
// ----------------------------------------------------------------------------

void RigEditorShocksPanel::Shocks_CallbackKeyFocusGained_RestorePreviousFieldValue(MyGUI::Widget* new_widget, MyGUI::Widget* old_widget)
{
    CallbackKeyFocusGained_RestorePreviousFieldValue(new_widget, old_widget);
}

// ----------------------------------------------------------------------------
// Data manipulation
// ----------------------------------------------------------------------------

void RigEditorShocksPanel::UpdateShockData(RigEditor::RigAggregateShocksData* data)
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
    UpdateFlagCheckbox(m_flag_R_checkbox, data->HasFlag_R(), !data->IsFlagUniform_R());
    UpdateFlagCheckbox(m_flag_L_checkbox, data->HasFlag_L(), !data->IsFlagUniform_L());
    
    // Fields
    EditboxRestoreValue(&m_detacher_group_field);
    EditboxRestoreValue(&m_spring_field);
    EditboxRestoreValue(&m_damping_field);
    EditboxRestoreValue(&m_contraction_limit_field);
    EditboxRestoreValue(&m_extension_limit_field);
    EditboxRestoreValue(&m_precompression_field);
}
