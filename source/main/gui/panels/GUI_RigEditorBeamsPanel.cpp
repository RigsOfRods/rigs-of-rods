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

#include "GUI_RigEditorBeamsPanel.h"
#include "RigEditor_Config.h"
#include "RigEditor_RigElementsAggregateData.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------

#define SETUP_FLAG_CHECKBOX(FLAG_LETTER, TOOLTIP)                                      \
    SetupFlagCheckbox(                                                                 \
        m_flag_##FLAG_LETTER##_checkbox,                                               \
        m_data.GetFlagsPtr(),                                                          \
        RigEditor::RigAggregatePlainBeamsData::QUERY_FLAG_##FLAG_LETTER##_IS_UNIFORM,  \
        RigEditor::RigAggregatePlainBeamsData::VALUE_FLAG_##FLAG_LETTER,               \
        m_flags_tooltip_label,                                                         \
        TOOLTIP                                                                        \
    );

#define SETUP_EDITBOX_FIELD(NAME, UNIFLAG, SRC, SRC_TYPE) \
    SetupEditboxField(&(NAME##_field), NAME##_label, NAME##_editbox, \
        m_data.GetFlagsPtr(), RigEditor::RigAggregatePlainBeamsData::UNIFLAG, ((void*) &(SRC)), EditboxFieldSpec::SRC_TYPE);

RigEditorBeamsPanel::RigEditorBeamsPanel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config):
    RigEditor::RigElementGuiPanelBase(rig_editor_interface, config, m_beams_panel, m_flag_i_checkbox)
{
    m_flags_tooltip_label->setTextColour(m_text_color_tooltip);
    
    // Node flag checkboxes - set user data
    SETUP_FLAG_CHECKBOX( i, "Invisible beam");
    SETUP_FLAG_CHECKBOX( r, "Rope (no compress resist)");
    SETUP_FLAG_CHECKBOX( s, "Support (no extend resist)");

    SETUP_EDITBOX_FIELD(m_detacher_group, QUERY_DETACHER_GROUP_IS_UNIFORM,       m_data.detacher_group,        SOURCE_DATATYPE_INT);
    SETUP_EDITBOX_FIELD(m_ext_breaklimit, QUERY_EXTENSION_BREAKLIMIT_IS_UNIFORM, m_data.extension_break_limit, SOURCE_DATATYPE_FLOAT);

    AlignToScreen(&config->gui_beams_panel_position);
}

// ----------------------------------------------------------------------------
// GUI utility
// ----------------------------------------------------------------------------

void RigEditorBeamsPanel::SetFlagFieldsVisible(bool visible)
{
    m_flags_label->setVisible(visible);
    m_flag_i_checkbox->setVisible(visible);
    m_flag_r_checkbox->setVisible(visible);
    m_flag_s_checkbox->setVisible(visible);
}

void RigEditorBeamsPanel::SetExtBreaklimitFieldVisible(bool visible)
{
    m_ext_breaklimit_label->setVisible(visible);
    m_ext_breaklimit_editbox->setVisible(visible);
}

void RigEditorBeamsPanel::SetBeamTypeFieldMixedValueMode(MyGUI::Widget* label_widget, MyGUI::Widget* combobox_widget, const char* beam_type, bool is_mixed)
{
    // Process input
    MyGUI::TextBox* label = nullptr;
    if (label_widget != nullptr) 
    { 
        label = label_widget->castType<MyGUI::TextBox>(false); 
    }
    MyGUI::EditBox* combobox = combobox_widget->castType<MyGUI::EditBox>(false);
    assert(combobox != nullptr);

    // Update GUI
    if (label != nullptr)
    {
        label->setTextColour( (is_mixed) ? m_text_color_mixvalues : m_text_color_default);
    }
    combobox->setCaption( (is_mixed) ? "" : beam_type);
}

// ----------------------------------------------------------------------------
// Data manipulation
// ----------------------------------------------------------------------------

void RigEditorBeamsPanel::UpdatePlainBeamData(RigEditor::RigAggregatePlainBeamsData* data)
{
    m_data = *data;
    m_data_mixed = false;

    // Beam type
    SetBeamTypeFieldMixedValueMode(m_beam_type_label, m_beam_type_combobox, "Plain beam", false);

    // Panel name
    if (data->num_selected == 1)
    {
        m_panel_widget->setCaption("Beam");
    }
    else
    {
        char caption[30];
        sprintf(caption, "Beams [%d]", data->num_selected);
        m_panel_widget->setCaption(caption);
    }

    // Flags
    UpdateFlagCheckbox(m_flag_i_checkbox, data->HasFlag_i(), !data->IsFlagUniform_i());
    UpdateFlagCheckbox(m_flag_r_checkbox, data->HasFlag_r(), !data->IsFlagUniform_r());
    UpdateFlagCheckbox(m_flag_s_checkbox, data->HasFlag_s(), !data->IsFlagUniform_s());
    SetFlagFieldsVisible(true);

    // Fields
    SetExtBreaklimitFieldVisible(data->HasFlag_s());
    EditboxRestoreValue(&m_detacher_group_field);
    EditboxRestoreValue(&m_ext_breaklimit_field);
}

void RigEditorBeamsPanel::UpdateMixedBeamData(RigEditor::RigAggregateBeams2Data* query_result)
{
    m_data_mixed = true;
    m_data.SetDetacherGroupIsUniform(query_result->IsDetacherGroupUniform());
    m_data.detacher_group = query_result->plain_beams.detacher_group;

    assert(query_result->HasMixedBeamTypes());
    SetFlagFieldsVisible(false);
    SetExtBreaklimitFieldVisible(false);
    SetBeamTypeFieldMixedValueMode(m_beam_type_label, m_beam_type_combobox, "", true);

    // Title
    char title[100] = "";
    sprintf(title, "Beams (mixed) [%d]", query_result->GetTotalNumSelectedBeams());
    m_panel_widget->setCaption(title);

    // Fields
    EditboxRestoreValue(&m_detacher_group_field);
}
