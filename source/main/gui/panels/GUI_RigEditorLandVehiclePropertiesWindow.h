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
	@file   GUI_RigEditorLandVehiclePropertiesWindow.h
	@author Petr Ohlidal
	@date   10/2014
*/

#include "ForwardDeclarations.h"
#include "GUI_RigEditorLandVehiclePropertiesWindowLayout.h"
#include "RigDef_File.h"
#include "RigDef_Prerequisites.h"
#include "RigEditor_ForwardDeclarations.h"

namespace RoR
{

namespace GUI
{

class RigEditorLandVehiclePropertiesWindow: public RigEditorLandVehiclePropertiesWindowLayout
{

public:

	RigEditorLandVehiclePropertiesWindow(RigEditor::IMain* rig_editor_interface);

	void Show();
	
	void Hide();

	void CenterToScreen();

	bool IsVisible();

	void Import(
			boost::shared_ptr<RigDef::Engine> engine,
			boost::shared_ptr<RigDef::Engoption> engoption
		);

	void Export(
			boost::shared_ptr<RigDef::Engine> engine,
			boost::shared_ptr<RigDef::Engoption> engoption
		);

private:

	void SetEngineType(RigDef::Engoption::EngineType type);

	/* Event handlers */

	void WindowButtonClicked(MyGUI::Widget* sender, const std::string& name);

	void CancelButtonClicked(MyGUI::Widget* sender);

	void ForwardGearsEditboxKeyFocusGained(MyGUI::Widget* sender, MyGUI::Widget* _);

	void ForwardGearsEditboxKeyFocusLost(MyGUI::Widget* sender, MyGUI::Widget* _);

	void EngineTypeRadioClicked(MyGUI::Widget* sender);

private:

	RigEditor::IMain* m_rig_editor_interface;

	RigEditor::RigProperties* m_rig_properties;

	// Forward gears editbox
	bool m_forward_gears_textbox_empty;
	MyGUI::UString m_gears_textbox_placeholder_text;
	MyGUI::Colour m_editbox_placeholder_color;

};

} // namespace GUI

} // namespace RoR
