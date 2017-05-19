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
#include "GUI_RigEditorShocks2PanelLayout.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_RigElementGuiPanelBase.h"
#include "RigEditor_IMain.h"
#include "RigEditor_RigElementsAggregateData.h"

namespace RoR
{

namespace GUI
{

class RigEditorShocks2Panel: public RigEditorShocks2PanelLayout, public RigEditor::RigElementGuiPanelBase
{

public:

    RigEditorShocks2Panel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config);

    void UpdateShock2Data(RigEditor::RigAggregateShocks2Data* data);

    inline const RigEditor::RigAggregateShocks2Data* GetShocks2Data() { return &m_data; }

    inline void HideAndReset()
    {
        Hide();
        m_data.Reset();
    }

private:

    // Aggregate rig data
    RigEditor::RigAggregateShocks2Data m_data;

    // GUI form fields
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_detacher_group_field;     
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_spring_in_field;          
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_damp_in_field;            
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_spring_out_field;         
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_damp_out_field;           
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_spring_in_progress_field; 
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_damp_in_progress_field;   
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_spring_out_progress_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_damp_out_progress_field;  
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_contraction_limit_field;  
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_extension_limit_field;    
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_precompression_field;     
};

} // namespace GUI

} // namespace RoR
