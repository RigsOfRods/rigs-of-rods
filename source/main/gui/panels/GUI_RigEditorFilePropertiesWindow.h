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
	@file   GUI_RigEditorFilePropertiesWindow.h
	@author Petr Ohlidal
	@date   09/2014
*/

#include "ForwardDeclarations.h"
#include "GUI_RigEditorFilePropertiesWindowLayout.h"
#include "RigEditor_ForwardDeclarations.h"

namespace RoR
{

namespace GUI
{

class RigEditorFilePropertiesWindow: public RigEditorFilePropertiesWindowLayout
{

public:

	RigEditorFilePropertiesWindow(RigEditor::IMain* rig_editor_interface);

	void Show();
	
	void Hide();

	void CenterToScreen();

	bool IsVisible();

	void Import(RigEditor::RigProperties* rig_properties);

	void Export(RigEditor::RigProperties* rig_properties);

private:

	/* Event handlers */

	void WindowButtonClicked(MyGUI::Widget* sender, const std::string& name);

	void CancelButtonClicked(MyGUI::Widget* sender);

private:

	RigEditor::IMain* m_rig_editor_interface;

};

} // namespace GUI

} // namespace RoR
