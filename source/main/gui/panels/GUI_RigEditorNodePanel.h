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

#pragma once

/** 
	@file   
	@author Petr Ohlidal
	@date   12/2014
*/

#include "ForwardDeclarations.h"
#include "GUI_RigEditorNodePanelLayout.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_IMain.h"
#include "GuiPanelBase.h"

namespace RoR
{

namespace GUI
{

class RigEditorNodePanel: public RigEditorNodePanelLayout, public GuiPanelBase
{

public:

	RigEditorNodePanel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config);

	/**
	* @param node_name                     nullptr if multiple nodes selected
	* @param unique_load_weight_ptr        nullptr if not unique
	* @param unique_detacher_group_id_ptr  nullptr if not unique
	* @param unique_preset_id_ptr          nullptr if not unique
	*/
	void UpdateNodeData(
		int node_count, 
		Ogre::String node_name,
		float* unique_load_weight_ptr,
		int* unique_detacher_group_id_ptr,
		int* unique_preset_id_ptr
		);

	void UpdateNodeFlags(
		unsigned int flags_all_nodes,
		unsigned int flags_any_node
		);

	void PositionOnScreen(RigEditor::Config* config);

private:

	void SetNodeNameFieldVisible(bool show);

	void UpdateFlagCheckbox(
		MyGUI::Button* checkbox,
		unsigned int flags_all_nodes,
		unsigned int flags_any_node
		);

	void NodeFlagCheckboxClicked(MyGUI::Widget* sender);

	void NodeFlagCheckboxFocusGained(MyGUI::Widget* old_widget, MyGUI::Widget* new_widget);

	void NodeFlagCheckboxFocusLost(MyGUI::Widget* old_widget, MyGUI::Widget* new_widget);

	RigEditor::IMain* m_rig_editor_interface;

	MyGUI::Colour m_config_checkbox_mixvalues_color;
	MyGUI::Colour m_config_checkbox_default_color;

};

} // namespace GUI

} // namespace RoR
