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
	@file   MenuWindow.h
	@author Moncef Ben Slimane
	@date   08/2014
	@brief  Main menu
*/

#include "RoRPrerequisites.h"
#include "Singleton.h"
#include "mygui/BaseLayout.h"

ATTRIBUTE_CLASS_LAYOUT(MenuWindow, "MainMenu.layout");

class MenuWindow :
	public wraps::BaseLayout,
	public RoRSingleton<MenuWindow>,
	public ZeroedMemoryAllocator
{
	friend class RoRSingleton<MenuWindow>;
	MenuWindow();
	~MenuWindow();

public:
	void Show();
	void Hide();
	void SetPosition(int pixels_left, int pixels_top);
	int GetHeight();

private:
	void eventMouseButtonClickSelectButton(MyGUI::WidgetPtr _sender);
	void eventMouseButtonClickRigEditorButton(MyGUI::WidgetPtr _sender);
	void eventMouseButtonClickSettingButton(MyGUI::WidgetPtr _sender);
	void eventMouseButtonClickAboutButton(MyGUI::WidgetPtr _sender);
	void eventMouseButtonClickExitButton(MyGUI::WidgetPtr _sender);

	ATTRIBUTE_FIELD_WIDGET_NAME(MenuWindow, mSelTerrButton, "single_player");
	MyGUI::Button* mSelTerrButton;

	ATTRIBUTE_FIELD_WIDGET_NAME(MenuWindow, mRigEditorButton, "rig_editor");
	MyGUI::Button* mRigEditorButton;

	ATTRIBUTE_FIELD_WIDGET_NAME(MenuWindow, mSettingsButton, "settings");
	MyGUI::Button* mSettingsButton;
	
	ATTRIBUTE_FIELD_WIDGET_NAME(MenuWindow, mAboutButton, "about");
	MyGUI::Button* mAboutButton;
	
	ATTRIBUTE_FIELD_WIDGET_NAME(MenuWindow, mCloseButton, "exit");
	MyGUI::Button* mCloseButton;
};
