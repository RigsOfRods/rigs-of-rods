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
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   
	@author Petr Ohlidal
	@date   11/2014
*/

#include "ContentManager.h"
#include "GUI_RigEditorHelpWindow.h"
#include "RigEditor_IMain.h"
#include "RoRPrerequisites.h"
#include "Utils.h"

#include <MyGUI.h>
#include <OgreResourceGroupManager.h>

using namespace RoR;
using namespace GUI;

#define CLASS        RigEditorHelpWindow
#define MAIN_WIDGET  m_rig_editor_help_window

CLASS::CLASS(RigEditor::IMain* rig_editor_interface):
	m_helpfile_loaded(false)
{
	m_rig_editor_interface = rig_editor_interface;

	// Close window [X] button
	MyGUI::Window* main_window = MAIN_WIDGET->castType<MyGUI::Window>();
	main_window->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::WindowButtonClicked);

	Hide();
}

void CLASS::Show()
{
	// Load helpfile
	if (!m_helpfile_loaded)
	{
		Ogre::DataStreamPtr helpfile_stream = Ogre::ResourceGroupManager::getSingleton().openResource(
			"rig_editor_helpfile_english.txt", // TODO: Localization
			RoR::ContentManager::ResourcePack::RIG_EDITOR.resource_group_name,
			false);

		if (!helpfile_stream.isNull())
		{
			m_help_view->setCaption(helpfile_stream->getAsString());
			m_helpfile_loaded = true;
			m_help_view->setVScrollPosition(0);
		}
	}

	MAIN_WIDGET->setVisible(true);
}

void CLASS::Hide()
{
	MAIN_WIDGET->setVisible(false);
}

void CLASS::CenterToScreen()
{
	MyGUI::IntSize windowSize = MAIN_WIDGET->getSize();
	MyGUI::IntSize parentSize = MAIN_WIDGET->getParentSize();

	MAIN_WIDGET->setPosition((parentSize.width - windowSize.width) / 2, (parentSize.height - windowSize.height) / 2);
}

bool CLASS::IsVisible()
{
	return MAIN_WIDGET->isVisible();
}

void CLASS::WindowButtonClicked(MyGUI::Widget* sender, const std::string& name)
{
	Hide(); // There's only close [X] button -> hide window.
}
