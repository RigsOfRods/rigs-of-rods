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
// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 7th of September 2009
#ifdef USE_MYGUI
#ifndef __GUI_Friction_H_
#define __GUI_Friction_H_

#include "RoRPrerequisites.h"
#include "Singleton.h"

#include <MyGUI.h>

class GUI_Friction : public RoRSingleton < GUI_Friction >, public ZeroedMemoryAllocator
{
public:

	GUI_Friction();
	~GUI_Friction();

	bool getVisible();
	void setVisible(bool value);

	void setShaded(bool value);
	
	void setCollisions(Collisions *_col) { col = _col; };
	void setActiveCol(ground_model_t *gm);

private:

	Collisions *col;
	MyGUI::WindowPtr msgwin;
	MyGUI::WindowPtr win;
	ground_model_t *active_gm;
	ground_model_t *selected_gm;

	std::map < Ogre::UTFString , std::pair < Ogre::UTFString , Ogre::UTFString > > helpTexts;
	std::map < Ogre::UTFString , std::pair < Ogre::Real , Ogre::Real > > minMaxs;

	void updateControls(ground_model_t *gm, bool setCombo=true);
	
	void event_combo_grounds_eventComboAccept(MyGUI::ComboBoxPtr _sender, size_t _index);
	void event_btn_MouseButtonClick(MyGUI::WidgetPtr _sender);
	void event_edit_TextChange(MyGUI::EditPtr _sender);
	void event_scroll_value(MyGUI::ScrollBar *_sender, size_t _value);
	void notifyWindowButtonPressed(MyGUI::WindowPtr _sender, const std::string& _name);
	void notifyHelpWindowButtonPressed(MyGUI::WindowPtr _sender, const std::string& _name);
	void applyChanges();
	void showHelp(Ogre::UTFString title, Ogre::UTFString msg, int x, int y);
};

#endif // __GUI_Friction_H_
#endif // USE_MYGUI
