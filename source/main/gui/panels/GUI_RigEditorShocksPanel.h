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
#include "GUI_RigEditorShocksPanelLayout.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_RigElementGuiPanelBase.h"
#include "RigEditor_RigElementsAggregateData.h"
#include "RigEditor_IMain.h"

namespace RoR
{

namespace GUI
{

class RigEditorShocksPanel: public RigEditorShocksPanelLayout, public RigEditor::RigElementGuiPanelBase
{

public:

    RigEditorShocksPanel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config);

    void UpdateShockData(RigEditor::RigAggregateShocksData* data);

    inline void Hide()
    {
        m_data.Reset();
        RigElementGuiPanelBase::Hide();
    }

    inline RigEditor::RigAggregateShocksData* GetShocksData() { return &m_data; }

    inline void HideAndReset()
    {
        Hide();
        m_data.Reset();
    }

private:

    /// Wrapper around Base's callback - needed to bind
    void Shocks_CallbackKeyFocusGained_RestorePreviousFieldValue(MyGUI::Widget* new_widget, MyGUI::Widget* old_widget);

    // Aggregate rig data
    RigEditor::RigAggregateShocksData m_data;

    // GUI form fields
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_detacher_group_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_spring_field;        
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_damping_field;       
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_contraction_limit_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_extension_limit_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_precompression_field;

};

} // namespace GUI

} // namespace RoR
