/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   
	@author Petr Ohlidal
	@date   12/2014
*/

#include "GUI_RigEditorNodePanel.h"
#include "RigDef_File.h" // For RigDef::Node::FLAGS
#include "RigEditor_Config.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

struct NodeFlagCheckboxUserData
{
	NodeFlagCheckboxUserData(unsigned int flag, const char* tooltip):
		flag(flag),
		tooltip(tooltip)
	{}

	unsigned int flag;
	const char * tooltip;
};

#define SETUP_CHECKBOX(WIDGET, FLAG, TOOLTIP) \
	WIDGET->setUserData(NodeFlagCheckboxUserData(FLAG, TOOLTIP)); \
	WIDGET->eventMouseButtonClick += MyGUI::newDelegate(this, &RigEditorNodePanel::NodeFlagCheckboxClicked); \
	WIDGET->eventMouseLostFocus   += MyGUI::newDelegate(this, &RigEditorNodePanel::NodeFlagCheckboxFocusLost); \
	WIDGET->eventMouseSetFocus    += MyGUI::newDelegate(this, &RigEditorNodePanel::NodeFlagCheckboxFocusGained);

#define CONVERT_COLOR_OGRE_TO_MYGUI(OUT, IN) OUT.red = IN.r; OUT.green = IN.g; OUT.blue = IN.b; OUT.alpha = IN.a;

RigEditorNodePanel::RigEditorNodePanel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config):
	GuiPanelBase(m_node_panel)
{
	Hide();

	// Setup attributes
	m_rig_editor_interface = rig_editor_interface;
	CONVERT_COLOR_OGRE_TO_MYGUI(m_config_checkbox_mixvalues_color, config->gui_node_panel_flag_checkbox_mixvalues_color);
	m_config_checkbox_default_color = m_flag_m_checkbox->getTextColour();

	// Setup GUI
	MyGUI::Colour flag_tooltip_text_color;
	CONVERT_COLOR_OGRE_TO_MYGUI(flag_tooltip_text_color,  config->gui_node_panel_flag_tooltip_text_color);
	m_flag_tooltip_label->setTextColour(flag_tooltip_text_color);
	
	// Node flag checkboxes - set user data
	SETUP_CHECKBOX( m_flag_m_checkbox,   RigDef::Node::OPTION_m_NO_MOUSE_GRAB,     "Mouse grab disabled");
	SETUP_CHECKBOX( m_flag_l_checkbox,   RigDef::Node::OPTION_l_LOAD_WEIGHT,       "Loaded with cargo");
	SETUP_CHECKBOX( m_flag_p_checkbox,   RigDef::Node::OPTION_p_NO_PARTICLES,      "Particle gfx disabled");
	SETUP_CHECKBOX( m_flag_b_checkbox,   RigDef::Node::OPTION_b_EXTRA_BUOYANCY,    "Extra buoyancy");
	SETUP_CHECKBOX( m_flag_h_checkbox,   RigDef::Node::OPTION_h_HOOK_POINT,        "Hook point [legacy]");
	SETUP_CHECKBOX( m_flag_c_checkbox,   RigDef::Node::OPTION_c_NO_GROUND_CONTACT, "Ground contact disabled");
	SETUP_CHECKBOX( m_flag_y_checkbox,   RigDef::Node::OPTION_y_EXHAUST_DIRECTION, "Exhaust direction [legacy]");
	SETUP_CHECKBOX( m_flag_x_checkbox,   RigDef::Node::OPTION_x_EXHAUST_POINT,     "Exhaust point [legacy]");
	SETUP_CHECKBOX( m_flag_f_checkbox,   RigDef::Node::OPTION_f_NO_SPARKS,         "Sparks disabled");
	SETUP_CHECKBOX( m_flag_L_checkbox,   RigDef::Node::OPTION_L_LOG,               "Logging enabled");
	
	PositionOnScreen(config);
}

void RigEditorNodePanel::PositionOnScreen(RigEditor::Config* config)
{
	MyGUI::IntSize screenSize = m_panel_widget->getParentSize();
	int x = config->gui_node_panel_margin_x_px; // Anchor: left
	int y = config->gui_node_panel_margin_y_px; // Anchor: top
	if (config->gui_node_panel_anchor_right)
	{
		x = screenSize.width - GetWidthPixels() - config->gui_node_panel_margin_x_px;
	}
	if (config->gui_node_panel_anchor_bottom)
	{
		y = screenSize.height - GetHeightPixels() - config->gui_node_panel_margin_y_px;
	}
	SetPosition(x, y);
}

void RigEditorNodePanel::NodeFlagCheckboxClicked(MyGUI::Widget* sender)
{
	MyGUI::Button* checkbox = sender->castType<MyGUI::Button>();
	assert(checkbox != nullptr);
	unsigned int flag = 0;

	// Switch checkbox state
	bool selected = !checkbox->getStateSelected();
	checkbox->setStateSelected(selected);

	// Enable/disable "load weight override" editbox
	if (checkbox == m_flag_l_checkbox)
	{
		m_loadweight_label->setEnabled(selected);
		m_loadweight_editbox->setEnabled(selected);
	}

	// Update node(s)
	NodeFlagCheckboxUserData* userdata_ptr = checkbox->getUserData<NodeFlagCheckboxUserData>();
	assert(userdata_ptr != nullptr);
	m_rig_editor_interface->CommandSelectedNodesUpdateFlag(selected, userdata_ptr->flag);
}

void RigEditorNodePanel::UpdateNodeData(
	int node_count, 
	Ogre::String node_name,
	float* unique_load_weight_ptr,
	int* unique_detacher_group_id_ptr,
	int* unique_preset_id_ptr
	)
{
	// "Node name" field
	if (node_count == 1 && !node_name.empty())
	{
		m_node_name_editbox->setCaption(node_name);
		SetNodeNameFieldVisible(true);
	}
	else
	{
		m_node_name_editbox->setCaption("");
		SetNodeNameFieldVisible(false);
	}
	// Fields
	if (unique_load_weight_ptr != nullptr)
	{
		m_loadweight_editbox->setCaption(Ogre::StringConverter::toString(*unique_load_weight_ptr));
	}
	if (unique_detacher_group_id_ptr != nullptr)
	{
		m_detacher_group_editbox->setCaption(Ogre::StringConverter::toString(*unique_detacher_group_id_ptr));
	}
	if (unique_preset_id_ptr != nullptr)
	{
		m_preset_selector_combobox->setCaption(Ogre::StringConverter::toString(*unique_preset_id_ptr));
	}
	if (node_count == 1)
	{
		m_node_panel->setCaption("Node");
	}
	else
	{
		char panel_caption[30] = "";
		sprintf(panel_caption, "Nodes [%d]", node_count);
		m_node_panel->setCaption(panel_caption);
	}
}

void RigEditorNodePanel::UpdateFlagCheckbox(
	MyGUI::Button* checkbox,
	unsigned int flags_all_nodes,
	unsigned int flags_any_node
	)
{
	NodeFlagCheckboxUserData* userdata_ptr = checkbox->getUserData<NodeFlagCheckboxUserData>();
	assert(userdata_ptr != nullptr);
	unsigned int checkbox_flag = userdata_ptr->flag;
	bool all_nodes_match = BITMASK_IS_1(flags_all_nodes, checkbox_flag);
	bool any_node_match = BITMASK_IS_1(flags_any_node, checkbox_flag);
	if (all_nodes_match) // Flag enabled on all nodes
	{
		checkbox->setStateSelected(true);
		checkbox->setTextColour(m_config_checkbox_default_color);
	}
	else if (!all_nodes_match && !any_node_match) // Flag disabled on all nodes
	{
		checkbox->setStateSelected(false);
		checkbox->setTextColour(m_config_checkbox_default_color);
	}
	else // Mixed values
	{
		checkbox->setStateSelected(false);
		checkbox->setTextColour(m_config_checkbox_mixvalues_color);
	}
}

void RigEditorNodePanel::UpdateNodeFlags(
	unsigned int flags_all_nodes,
	unsigned int flags_any_node
	)
{
	UpdateFlagCheckbox(m_flag_m_checkbox, flags_all_nodes, flags_any_node);
	UpdateFlagCheckbox(m_flag_l_checkbox, flags_all_nodes, flags_any_node);
	UpdateFlagCheckbox(m_flag_p_checkbox, flags_all_nodes, flags_any_node);
	UpdateFlagCheckbox(m_flag_b_checkbox, flags_all_nodes, flags_any_node);
	UpdateFlagCheckbox(m_flag_h_checkbox, flags_all_nodes, flags_any_node);
	UpdateFlagCheckbox(m_flag_c_checkbox, flags_all_nodes, flags_any_node);
	UpdateFlagCheckbox(m_flag_y_checkbox, flags_all_nodes, flags_any_node);
	UpdateFlagCheckbox(m_flag_x_checkbox, flags_all_nodes, flags_any_node);
	UpdateFlagCheckbox(m_flag_f_checkbox, flags_all_nodes, flags_any_node);
	UpdateFlagCheckbox(m_flag_L_checkbox, flags_all_nodes, flags_any_node);
}

void RigEditorNodePanel::NodeFlagCheckboxFocusGained(MyGUI::Widget* old_widget, MyGUI::Widget* new_widget)
{
	NodeFlagCheckboxUserData* userdata_ptr = old_widget->getUserData<NodeFlagCheckboxUserData>();
	if (userdata_ptr == nullptr)
	{
		return;
	}
	m_flag_tooltip_label->setCaption(userdata_ptr->tooltip);
}

void RigEditorNodePanel::NodeFlagCheckboxFocusLost(MyGUI::Widget* old_widget, MyGUI::Widget* new_widget)
{
	m_flag_tooltip_label->setCaption("");
}

void RigEditorNodePanel::SetNodeNameFieldVisible(bool show)
{
	m_node_name_label->setVisible(show);
	m_node_name_editbox->setVisible(show);
}
