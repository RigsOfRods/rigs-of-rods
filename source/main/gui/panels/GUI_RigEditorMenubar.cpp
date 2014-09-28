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
	@file   GUI_RigEditorMenubar.cpp
	@author Petr Ohlidal
	@date   08/2014
*/

#include "GUI_RigEditorMenubar.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

RigEditorMenubar::RigEditorMenubar(RigEditor::IMain* rig_editor_interface)
{
	m_rig_editor_interface = rig_editor_interface;

	m_file_popup_item_open->eventMouseButtonClick += MyGUI::newDelegate(this, &RigEditorMenubar::OpenFileItemClicked);
	m_file_popup_item_close->eventMouseButtonClick += MyGUI::newDelegate(this, &RigEditorMenubar::CloseRigItemClicked);
	m_file_popup_item_quit->eventMouseButtonClick += MyGUI::newDelegate(this, &RigEditorMenubar::QuitEditorItemClicked);
}

void RigEditorMenubar::Show()
{
	m_rig_editor_menubar->setVisible(true);
}

void RigEditorMenubar::Hide()
{
	m_rig_editor_menubar->setVisible(false);
}

void RigEditorMenubar::SetWidth(int width_pixels)
{
	m_rig_editor_menubar->setSize(width_pixels, m_rig_editor_menubar->getHeight());
}

// ============================================================================
// Event handlers
// ============================================================================

void RigEditorMenubar::OpenFileItemClicked(MyGUI::Widget* sender)
{
	m_rig_editor_interface->CommandShowDialogOpenRigFile();
}

void RigEditorMenubar::SaveFileItemClicked(MyGUI::Widget* sender)
{
	m_rig_editor_interface->CommandSaveRigFile();
}

void RigEditorMenubar::CloseRigItemClicked(MyGUI::Widget* sender)
{
	m_rig_editor_interface->CommandCloseCurrentRig();
}

void RigEditorMenubar::QuitEditorItemClicked(MyGUI::Widget* sender)
{
	m_rig_editor_interface->CommandQuitRigEditor();
}
