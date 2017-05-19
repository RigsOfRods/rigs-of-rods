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
#include "GUI_RigEditorBeamsPanelLayout.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_RigElementGuiPanelBase.h"
#include "RigEditor_IMain.h"
#include "RigEditor_RigElementsAggregateData.h"

namespace RoR
{

namespace GUI
{

class RigEditorBeamsPanel: public RigEditorBeamsPanelLayout, public RigEditor::RigElementGuiPanelBase
{

public:

    RigEditorBeamsPanel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config);

    void UpdatePlainBeamData(RigEditor::RigAggregatePlainBeamsData* query_result);

    void UpdateMixedBeamData(RigEditor::RigAggregateBeams2Data* query_result);

    inline const RigEditor::RigAggregatePlainBeamsData* GetPlainBeamsData() { return &m_data; }
    inline void GetMixedBeamsData(RigEditor::MixedBeamsAggregateData* data)
    {
        assert(m_data_mixed == true);
        data->SetDetacherGroupIsUniform(m_data.IsDetacherGroupUniform());
        data->detacher_group = m_data.detacher_group;
    }
    inline bool HasMixedBeamTypes() { return m_data_mixed; }

    inline void HideAndReset()
    {
        m_data.Reset();
        m_data_mixed = false;
        Hide();
    }

private:

    // Visibility
    void SetFlagFieldsVisible(bool visible);
    void SetExtBreaklimitFieldVisible(bool visible);

    // Beam combobox
    void SetBeamTypeFieldMixedValueMode(MyGUI::Widget* label_widget, MyGUI::Widget* combobox_widget, const char* beam_type, bool is_mixed);

    // Aggregate rig data
    RigEditor::RigAggregatePlainBeamsData m_data;
    bool                                        m_data_mixed;

    // GUI form fields
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_detacher_group_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_ext_breaklimit_field;
};

} // namespace GUI

} // namespace RoR
