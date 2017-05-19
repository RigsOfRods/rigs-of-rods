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
    @date   03/2015
*/

#include "ForwardDeclarations.h"
#include "GUI_RigEditorMeshWheels2PanelLayout.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_RigElementGuiPanelBase.h"
#include "RigEditor_IMain.h"
#include "RigEditor_RigWheelsAggregateData.h"

namespace RoR
{

namespace GUI
{

class RigEditorMeshWheels2Panel: public RigEditorMeshWheels2PanelLayout, public RigEditor::RigElementGuiPanelBase
{

public:

    RigEditorMeshWheels2Panel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config);

    void UpdateMeshWheels2Data(RigEditor::MeshWheel2AggregateData* data);

    inline const RigEditor::MeshWheel2AggregateData* GetMeshWheel2Data() { return &m_data; }

    inline void HideAndReset()
    {
        Hide();
        m_data.Reset();
    }

private:

    // Notification calback
    // Override
    virtual void OnFieldValueChanged(GenericFieldSpec* field);

    void CallbackClick_RadioButton(MyGUI::Widget* sender);

    void SetNodeFieldsVisible(bool visible);

    // Aggregate rig data
    RigEditor::MeshWheel2AggregateData m_data;

    // Fields
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_num_rays_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_mass_field;
    RigEditor::RigElementGuiPanelBase::GenericFieldSpec m_propulsion_field;
    RigEditor::RigElementGuiPanelBase::GenericFieldSpec m_braking_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_rim_mesh_name_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_tyre_material_name_field;
    RigEditor::RigElementGuiPanelBase::GenericFieldSpec m_side_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_node_axis_a_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_node_axis_b_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_rigidity_node_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_arm_node_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_rim_radius_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_rim_spring_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_rim_damping_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_rim_deform_threshold_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_rim_break_threshold_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_tyre_radius_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_tyre_spring_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_tyre_damping_field;
};

} // namespace GUI

} // namespace RoR
