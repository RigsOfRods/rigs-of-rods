/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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
// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 13th of August 2009
#ifdef USE_MYGUI
#ifndef _GUI_Menu_H_
#define _GUI_Menu_H_

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

	int getMenuHeight() { return menuHeight; };

	void updatePositionUponMousePosition(int x, int y);
	Ogre::UTFString getUserString(user_info_t &user, int num_vehicles);
	void addUserToMenu(user_info_t &user);

	void triggerUpdateVehicleList();

protected:

	MyGUI::MenuBarPtr mainmenu;
	int menuWidth, menuHeight;

	pthread_mutex_t updateLock;

	bool vehicleListNeedsUpdate;
	
	void onVehicleMenu(MyGUI::MenuControl* _sender, MyGUI::MenuItem* _item);
	void onMenuBtn(MyGUI::MenuCtrlPtr _sender, MyGUI::MenuItemPtr _item);
	void vehiclesListUpdate();

	std::vector<MyGUI::PopupMenuPtr> pop;
	MyGUI::PopupMenuPtr vehiclesMenu;
};

#endif // _GUI_Menu_H_
#endif // USE_MYGUI
