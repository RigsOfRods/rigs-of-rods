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
    @date   03/2015
*/

#include "GUI_RigEditorMeshWheels2Panel.h"
#include "RigDef_File.h" // For wheel propulsion/braking specs
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
        m_data.GetFlagsPtr(), RigEditor::MeshWheel2AggregateData::UNIFLAG, ((void*) &(SRC)), EditboxFieldSpec::SRC_TYPE);

#define SETUP_COMBOBOX(NAME, UNIFLAG, SRC, SRC_TYPE) \
    SetupGenericField(&NAME##_field, NAME##_label, NAME##_combobox, \
        m_data.GetFlagsPtr(), RigEditor::MeshWheel2AggregateData::UNIFLAG, ((void*) &(SRC)), GenericFieldSpec::SRC_TYPE);

#define SETUP_NODE_EDITBOX(NAME, SRC) \
    SetupEditboxField(&NAME##_field, NAME##_label, NAME##_editbox, \
        nullptr, 0, ((void*) &(SRC)), EditboxFieldSpec::SOURCE_DATATYPE_OBJECT_NODE, false);

RigEditorMeshWheels2Panel::RigEditorMeshWheels2Panel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config):
    RigEditor::RigElementGuiPanelBase(rig_editor_interface, config, m_meshwheels2_panel, nullptr)
{
    SETUP_EDITBOX  ( m_num_rays              , FIELD_RAY_COUNT_IS_UNIFORM            , m_data.num_rays               , SOURCE_DATATYPE_INT);
    SETUP_EDITBOX  ( m_mass	                 , FIELD_MASS_IS_UNIFORM                 , m_data.mass                   , SOURCE_DATATYPE_FLOAT);
    SETUP_COMBOBOX ( m_propulsion            , FIELD_PROPULSION_MODE_IS_UNIFORM      , m_data.propulsion             , SOURCE_DATATYPE_INT); // ENUM
    SETUP_COMBOBOX ( m_braking               , FIELD_BRAKING_MODE_IS_UNIFORM         , m_data.braking                , SOURCE_DATATYPE_INT); // ENUM
    SETUP_EDITBOX  ( m_rim_mesh_name         , FIELD_RIM_MESH_NAME_IS_UNIFORM        , m_data.rim_mesh_name          , SOURCE_DATATYPE_STRING);
    SETUP_EDITBOX  ( m_tyre_material_name    , FIELD_TYRE_MATERIAL_NAME_IS_UNIFORM   , m_data.tyre_material_name     , SOURCE_DATATYPE_STRING);
    SETUP_COMBOBOX ( m_side	                 , FIELD_SIDE_IS_UNIFORM                 , m_data.is_right_side          , SOURCE_DATATYPE_BOOL);
    SETUP_EDITBOX  ( m_rim_radius            , FIELD_RIM_RADIUS_IS_UNIFORM           , m_data.rim_radius             , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX  ( m_rim_spring            , FIELD_RIM_SPRING_IS_UNIFORM           , m_data.rim_spring             , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX  ( m_rim_damping           , FIELD_RIM_DAMPING_IS_UNIFORM          , m_data.rim_damping            , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX  ( m_rim_deform_threshold  , FIELD_RIM_DEFORM_THRESHOLD_IS_UNIFORM , m_data.rim_deform_threshold   , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX  ( m_rim_break_threshold   , FIELD_RIM_BREAK_THRESHOLD_IS_UNIFORM  , m_data.rim_breaking_threshold , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX  ( m_tyre_radius           , FIELD_TYRE_RADIUS_IS_UNIFORM          , m_data.tyre_radius            , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX  ( m_tyre_spring           , FIELD_TYRE_SPRING_IS_UNIFORM          , m_data.tyre_spring            , SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX  ( m_tyre_damping          , FIELD_TYRE_DAMPING_IS_UNIFORM         , m_data.tyre_damping           , SOURCE_DATATYPE_FLOAT);
    
    SETUP_NODE_EDITBOX  ( m_node_axis_a        , m_data.axis_nodes[0]      );
    SETUP_NODE_EDITBOX  ( m_node_axis_b        , m_data.axis_nodes[1]      );
    SETUP_NODE_EDITBOX  ( m_rigidity_node      , m_data.rigidity_node      );
    SETUP_NODE_EDITBOX  ( m_arm_node           , m_data.reference_arm_node );
    
    this->AlignToScreen(&config->gui_meshwheels2_panel_position);
    this->SetDefaultTextColor(m_tyre_damping_label->getTextColour());
}

// ----------------------------------------------------------------------------
// Data manipulation
// ----------------------------------------------------------------------------

void RigEditorMeshWheels2Panel::UpdateMeshWheels2Data(RigEditor::MeshWheel2AggregateData* data)
{
    m_data = *data;

    // Panel name
    if (data->num_elements == 1)
    {
        m_panel_widget->setCaption("Mesh wheel v2");
    }
    else
    {
        char caption[30];
        sprintf(caption, "Mesh wheels v2 [%d]", data->num_elements);
        m_panel_widget->setCaption(caption);
    }

    // Fields
    EditboxRestoreValue ( &m_num_rays_field             );
    EditboxRestoreValue ( &m_mass_field	                );
    ComboboxRestoreValue( &m_propulsion_field           );
    ComboboxRestoreValue( &m_braking_field              );
    ComboboxRestoreValue( &m_side_field                 );
    EditboxRestoreValue ( &m_rim_mesh_name_field        );
    EditboxRestoreValue ( &m_tyre_material_name_field   );
    EditboxRestoreValue ( &m_rim_radius_field           );
    EditboxRestoreValue ( &m_rim_spring_field           );
    EditboxRestoreValue ( &m_rim_damping_field          );
    EditboxRestoreValue ( &m_rim_deform_threshold_field );
    EditboxRestoreValue ( &m_rim_break_threshold_field  );
    EditboxRestoreValue ( &m_tyre_radius_field          );
    EditboxRestoreValue ( &m_tyre_spring_field          );
    EditboxRestoreValue ( &m_tyre_damping_field         );

    // Node fields
    EditboxRestoreValue ( &m_node_axis_a_field    );
    EditboxRestoreValue ( &m_node_axis_b_field    );
    EditboxRestoreValue ( &m_rigidity_node_field  );
    EditboxRestoreValue ( &m_arm_node_field       );
    this->SetNodeFieldsVisible(data->num_elements == 1);
}

// ----------------------------------------------------------------------------
// Other
// ----------------------------------------------------------------------------

void RigEditorMeshWheels2Panel::SetNodeFieldsVisible(bool visible)
{
    m_node_axis_a_field  .SetWidgetsVisible(visible);
    m_node_axis_b_field  .SetWidgetsVisible(visible);
    m_rigidity_node_field.SetWidgetsVisible(visible);
    m_arm_node_field     .SetWidgetsVisible(visible);

    m_nodes_section_label->setVisible(visible);
}

void RigEditorMeshWheels2Panel::OnFieldValueChanged(GenericFieldSpec* field)
{
    if (field == &m_num_rays_field || field == &m_tyre_radius_field)
    {
        this->SetIsImmediateRigUpdateNeeded(true);
    }
}
