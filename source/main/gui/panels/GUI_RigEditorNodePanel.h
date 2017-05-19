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
    @date   12/2014
*/

#include "ForwardDeclarations.h"
#include "GUI_RigEditorNodePanelLayout.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_RigElementGuiPanelBase.h"
#include "RigEditor_RigElementsAggregateData.h"

namespace RoR
{

namespace GUI
{

class RigEditorNodePanel: public RigEditorNodePanelLayout, public RigEditor::RigElementGuiPanelBase
{

public:

    RigEditorNodePanel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config);

    void UpdateNodeData(RigEditor::RigAggregateNodesData* query);

    inline void HideAndReset()
    {
        Hide();
        m_data.Reset();
    }

    inline const RigEditor::RigAggregateNodesData* GetData() { return &m_data; }

private:

    void SetNodeNameFieldVisible(bool show);

    // Aggregate node data
    RigEditor::RigAggregateNodesData m_data;

    // Form fields
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_loadweight_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_detacher_group_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_node_name_field;
};

} // namespace GUI

} // namespace RoR
