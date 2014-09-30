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
	@file   GUI_RigEditorFilePropertiesWindow.cpp
	@author Petr Ohlidal
	@date   09/2014
*/

#include "GUI_RigEditorFilePropertiesWindow.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

#define CLASS RigEditorFilePropertiesWindow

CLASS::CLASS(RigEditor::IMain* rig_editor_interface)
{
	m_rig_editor_interface = rig_editor_interface;

	// [Cancel] button
	m_button_cancel->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CancelButtonClicked);

	// Close window [X] button
	MyGUI::Window* main_window = m_rig_properties_window->castType<MyGUI::Window>();
	main_window->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::WindowButtonClicked);

	Hide();
}

void CLASS::Show()
{
	m_rig_properties_window->setVisible(true);
}

void CLASS::Hide()
{
	m_rig_properties_window->setVisible(false);
}

void CLASS::CenterToScreen()
{
	MyGUI::IntSize windowSize = m_rig_properties_window->getSize();
	MyGUI::IntSize parentSize = m_rig_properties_window->getParentSize();

	m_rig_properties_window->setPosition((parentSize.width - windowSize.width) / 2, (parentSize.height - windowSize.height) / 2);
}

bool CLASS::IsVisible()
{
	return m_rig_properties_window->isVisible();
}

void CLASS::WindowButtonClicked(MyGUI::Widget* sender, const std::string& name)
{
	Hide(); // There's only close [X] button -> hide window.
}

void CLASS::CancelButtonClicked(MyGUI::Widget* sender)
{
	Hide();
}
