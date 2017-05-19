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

#pragma once

/** 
    @file   
    @author Petr Ohlidal
    @date   01/2015
*/

#include "ForwardDeclarations.h"
#include "GUI_RigEditorCommands2PanelLayout.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_RigElementGuiPanelBase.h"
#include "RigEditor_RigElementsAggregateData.h"

namespace RoR
{

namespace GUI
{

class RigEditorCommands2Panel: public RigEditorCommands2PanelLayout, public RigEditor::RigElementGuiPanelBase
{

public:

    RigEditorCommands2Panel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config);

    void UpdateCommand2Data(RigEditor::RigAggregateCommands2Data* query_result);

    inline const RigEditor::RigAggregateCommands2Data* GetCommands2Data() { return &m_data; }

    inline void HideAndReset()
    {
        Hide();
        m_data.Reset();
    }

private:

    void CallbackClick_NeedsEngineChekckbox(MyGUI::Widget* sender);

        // Aggregate rig data
    RigEditor::RigAggregateCommands2Data m_data;

    // GUI form fields
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_detacher_group_field;   
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_contraction_rate_field; 
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_extension_rate_field;   
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_contraction_limit_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_extension_limit_field;  
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_contract_key_field;     
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_extend_key_field;                   
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_start_delay_field;      
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_stop_delay_field;       
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_affect_engine_field;    
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_start_function_field;   
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_stop_function_field; 
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_description_field;
};

} // namespace GUI

} // namespace RoR
