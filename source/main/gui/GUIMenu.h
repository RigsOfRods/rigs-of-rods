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
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   GUIMenu.h
	@date   13th of August 2009
	@author Thomas Fischer thomas{AT}thomasfischer{DOT}biz
*/

#ifdef USE_MYGUI

#pragma once

#include "RigDefPrerequisites.h"
#include "RoRPrerequisites.h"
#include "BeamData.h"
#include "Singleton.h"

#include <MyGUI.h>
#include <pthread.h>

class GUI_MainMenu : public RoRSingletonNoCreation< GUI_MainMenu >, public ZeroedMemoryAllocator
{
public:

	GUI_MainMenu();

	~GUI_MainMenu();

	bool getVisible();

	void setVisible(bool value);

	int getMenuHeight()
	{ 
		return m_menu_height; 
	};

	void updatePositionUponMousePosition(int x, int y);

	Ogre::UTFString getUserString(user_info_t &user, int num_vehicles);

	void addUserToMenu(user_info_t &user);

	void triggerUpdateVehicleList();

protected:

	void onVehicleMenu(MyGUI::MenuControl* _sender, MyGUI::MenuItem* _item);

	void onMenuBtn(MyGUI::MenuCtrlPtr _sender, MyGUI::MenuItemPtr _item);

	void vehiclesListUpdate();

	std::vector<MyGUI::PopupMenuPtr> m_popup_menus;
	MyGUI::PopupMenuPtr              m_vehicles_menu_widget;
	MyGUI::MenuBarPtr                m_menubar_widget;
	int                              m_menu_width;
	int                              m_menu_height;
	pthread_mutex_t                  m_update_lock;
	bool                             m_vehicle_list_needs_update;
};

#endif // USE_MYGUI
