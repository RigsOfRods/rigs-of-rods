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
// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 18th of July 2010
#ifdef USE_MYGUI
#ifdef USE_SOCKETW
#ifndef __GUI_Mp_H_
#define __GUI_Mp_H_

#include "RoRPrerequisites.h"

#include "rornet.h"
#include "Singleton.h"

#include <MyGUI.h>

class GUI_Multiplayer : public RoRSingletonNoCreation< GUI_Multiplayer >
{
public:

	GUI_Multiplayer();
	~GUI_Multiplayer();

	int update();
	void setVisible(bool value);
	bool getVisible();

protected:

	typedef struct player_row_t {
		MyGUI::StaticImagePtr flagimg;
		MyGUI::StaticImagePtr statimg;
		MyGUI::StaticImagePtr usergoimg;
		MyGUI::StaticImagePtr userTruckOKImg;
		MyGUI::StaticImagePtr userTruckOKRemoteImg;
		MyGUI::StaticTextPtr  playername;
	} player_row_t;

	MyGUI::EditPtr msgtext;
	MyGUI::StaticTextPtr tooltipText;
	MyGUI::WidgetPtr tooltipPanel, mpPanel;
	MyGUI::WindowPtr msgwin;

	player_row_t player_rows[MAX_PEERS + 1];

	void clickInfoIcon(MyGUI::WidgetPtr sender);
	void clickUserGoIcon(MyGUI::WidgetPtr sender);
	void openToolTip(MyGUI::WidgetPtr sender, const MyGUI::ToolTipInfo &t);

	MyGUI::WindowPtr netmsgwin;
	MyGUI::StaticTextPtr netmsgtext;
	
	void updateSlot(player_row_t *row, user_info_t *c, bool self);

	client_t *clients;
	int lineheight;

	static const int sidebarWidth = 250;
};

#endif // __GUI_Mp_H_
#endif // USE_SOCKETW
#endif // USE_MYGUI
