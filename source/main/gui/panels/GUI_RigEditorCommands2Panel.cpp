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

#include "GUI_RigEditorCommands2Panel.h"
#include "RigEditor_Config.h"
#include "RigEditor_RigElementsAggregateData.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------

#define SETUP_WIDGET_ALL(FIELD, LABEL, WIDGET, UNIFLAG, SRC, SRC_TYPE) \
    SetupEditboxField(&(FIELD), LABEL, WIDGET, \
        m_data.GetFlagsPtr(), RigEditor::RigAggregateCommands2Data::UNIFLAG, ((void*) &(SRC)), EditboxFieldSpec::SRC_TYPE);

#define SETUP_EDITBOX(NAME, UNIFLAG, SRC, SRC_TYPE) \
    SETUP_WIDGET_ALL(NAME##_field, NAME##_label, NAME##_editbox, UNIFLAG, SRC, SRC_TYPE)

#define SETUP_COMBOBOX(NAME, UNIFLAG, SRC, SRC_TYPE) \
    SETUP_WIDGET_ALL(NAME##_field, NAME##_label, NAME##_combobox, UNIFLAG, SRC, SRC_TYPE)

#define SETUP_FLAG_CHECKBOX(FLAG_LETTER, TOOLTIP)                                    \
    SetupFlagCheckbox(                                                               \
        m_flag_##FLAG_LETTER##_checkbox,                                             \
        m_data.GetFlagsPtr(),                                                        \
        RigEditor::RigAggregateCommands2Data::QUERY_FLAG_IS_UNIFORM_##FLAG_LETTER,   \
        RigEditor::RigAggregateCommands2Data::VALUE_FLAG_##FLAG_LETTER,              \
        m_flags_tooltip_label,                                                       \
        TOOLTIP                                                                      \
    );

RigEditorCommands2Panel::RigEditorCommands2Panel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config):
    RigEditor::RigElementGuiPanelBase(rig_editor_interface, config, m_commands2_panel, m_flag_i_checkbox)
{
    m_flags_tooltip_label->setTextColour(m_text_color_tooltip);
    
    // Node flag checkboxes - set user data
    SETUP_FLAG_CHECKBOX( i, "Invisible beam");
    SETUP_FLAG_CHECKBOX( r, "Rope/winch");
    SETUP_FLAG_CHECKBOX( c, "Auto-centering");
    SETUP_FLAG_CHECKBOX( f, "No speedup from engine");
    SETUP_FLAG_CHECKBOX( p, "Press-once");
    SETUP_FLAG_CHECKBOX( o, "Press-once, stop at center");

    // Fields
    SETUP_EDITBOX ( m_detacher_group,    QUERY_DETACHER_GROUP_IS_UNIFORM        , m_data.detacher_group,         SOURCE_DATATYPE_INT)
    SETUP_EDITBOX ( m_contraction_rate,  QUERY_CONTRACT_RATE_IS_UNIFORM         , m_data.contraction_rate,       SOURCE_DATATYPE_FLOAT)      
    SETUP_EDITBOX ( m_extension_rate,    QUERY_EXTEND_RATE_IS_UNIFORM           , m_data.extension_rate,         SOURCE_DATATYPE_FLOAT)      
    SETUP_EDITBOX ( m_contraction_limit, QUERY_CONTRACT_LIMIT_IS_UNIFORM        , m_data.max_contraction,        SOURCE_DATATYPE_FLOAT)      
    SETUP_EDITBOX ( m_extension_limit,   QUERY_EXTEND_LIMIT_IS_UNIFORM          , m_data.max_extension,          SOURCE_DATATYPE_FLOAT)      
    SETUP_COMBOBOX( m_contract_key,      QUERY_CONTRACT_KEY_IS_UNIFORM          , m_data.contract_key,           SOURCE_DATATYPE_INT)      
    SETUP_COMBOBOX( m_extend_key,        QUERY_EXTEND_KEY_IS_UNIFORM            , m_data.extend_key,             SOURCE_DATATYPE_INT)      
    SETUP_EDITBOX ( m_start_delay,       QUERY_INERTIA_START_DELAY_IS_UNIFORM   , m_data.inertia_start_delay,    SOURCE_DATATYPE_FLOAT)    
    SETUP_EDITBOX ( m_stop_delay,        QUERY_INERTIA_STOP_DELAY_IS_UNIFORM    , m_data.inertia_stop_delay,     SOURCE_DATATYPE_FLOAT)  
    SETUP_EDITBOX ( m_affect_engine,     QUERY_AFFECT_ENGINE_IS_UNIFORM         , m_data.affect_engine,          SOURCE_DATATYPE_FLOAT)         
    SETUP_EDITBOX ( m_start_function,    QUERY_INERTIA_START_FUNCTION_IS_UNIFORM, m_data.inertia_start_function, SOURCE_DATATYPE_STRING)
    SETUP_EDITBOX ( m_stop_function,     QUERY_INERTIA_STOP_FUNCTION_IS_UNIFORM , m_data.inertia_stop_function,  SOURCE_DATATYPE_STRING)

    SETUP_WIDGET_ALL ( m_description_field, nullptr, m_description_editbox, QUERY_DESCRIPTION_IS_UNIFORM   ,m_data.description,     SOURCE_DATATYPE_STRING)
    
    // Checkbox "needs engine"
    m_needs_engine_checkbox->eventMouseButtonClick += MyGUI::newDelegate(this, &RigEditorCommands2Panel::CallbackClick_NeedsEngineChekckbox);

    AlignToScreen(&config->gui_commands2_panel_position);
}

void RigEditorCommands2Panel::CallbackClick_NeedsEngineChekckbox(MyGUI::Widget* sender)
{
    // Clear "mixed values" state
    m_needs_engine_checkbox->setTextColour(m_text_color_default);
    m_data.SetNeedsEngineIsUniform(true);

    // Update data
    MyGUI::Button* button = sender->castType<MyGUI::Button>();
    m_data.SetBoolNeedsEngine(button->getStateSelected());
}

void RigEditorCommands2Panel::UpdateCommand2Data(RigEditor::RigAggregateCommands2Data* data)
{
    m_data = *data;

    // Panel name
    if (data->num_selected == 1)
    {
        m_panel_widget->setCaption("Command2");
    }
    else
    {
        char caption[50];
        sprintf(caption, "Commands2 [%d]", data->num_selected);
        m_panel_widget->setCaption(caption);
    }

    // Flags
    UpdateFlagCheckbox(m_flag_i_checkbox, data->HasFlag_i(), !data->IsFlagUniform_i());
    UpdateFlagCheckbox(m_flag_r_checkbox, data->HasFlag_r(), !data->IsFlagUniform_r());
    UpdateFlagCheckbox(m_flag_c_checkbox, data->HasFlag_c(), !data->IsFlagUniform_c());
    UpdateFlagCheckbox(m_flag_f_checkbox, data->HasFlag_f(), !data->IsFlagUniform_f());
    UpdateFlagCheckbox(m_flag_p_checkbox, data->HasFlag_p(), !data->IsFlagUniform_p());
    UpdateFlagCheckbox(m_flag_o_checkbox, data->HasFlag_o(), !data->IsFlagUniform_o());

    // Fields
    EditboxRestoreValue(&m_detacher_group_field    );
    EditboxRestoreValue(&m_contraction_rate_field  );
    EditboxRestoreValue(&m_extension_rate_field    );
    EditboxRestoreValue(&m_contraction_limit_field );
    EditboxRestoreValue(&m_extension_limit_field   );
    EditboxRestoreValue(&m_contract_key_field      );
    EditboxRestoreValue(&m_extend_key_field        );
    EditboxRestoreValue(&m_description_field       );
    EditboxRestoreValue(&m_start_delay_field       );
    EditboxRestoreValue(&m_stop_delay_field        );
    EditboxRestoreValue(&m_affect_engine_field     );
    EditboxRestoreValue(&m_start_function_field    );
    EditboxRestoreValue(&m_stop_function_field     );

    // "needs engine" checkbox field
    m_needs_engine_checkbox->setStateSelected(data->IsNeedsEngineUniform() ? data->GetBoolNeedsEngine() : false);
    m_needs_engine_checkbox->setTextColour(data->IsNeedsEngineUniform() ? m_text_color_default : m_text_color_mixvalues);
}
