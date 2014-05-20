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
#ifdef USE_SOCKETW

#include "GUIMp.h"

#include "BeamFactory.h"
#include "GUIManager.h"
#include "Language.h"
#include "Network.h"
#include "PlayerColours.h"
#include "RoRFrameListener.h"

using namespace Ogre;

GUI_Multiplayer::GUI_Multiplayer() :
	  clients(0)
	, lineheight(16)
	, msgwin(0)
{
	setSingleton(this);
	
	// allocate some buffers
	clients = (client_t *)calloc(MAX_PEERS,sizeof(client_t));
	

	// tooltip window
	tooltipPanel = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>("PanelSkin", 0, 0, 200, 20,  MyGUI::Align::Default, "ToolTip");
	tooltipText = tooltipPanel->createWidget<MyGUI::TextBox>("TextBox", 4, 2, 200, 16,  MyGUI::Align::Default);
	tooltipText->setFontName("VeraMono");
	//tooltipPanel->setAlpha(0.9f);
	tooltipText->setFontHeight(16);
	tooltipPanel->setVisible(false);
	
	// message window
	msgwin = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("WindowCSX", 0, 0, 400, 300,  MyGUI::Align::Center, "Overlapped");
	msgwin->setCaption(_L("Player Information"));
	msgtext = msgwin->createWidget<MyGUI::Edit>("EditStretch", 0, 0, 400, 300,  MyGUI::Align::Default, "helptext");
	msgtext->setCaption("");
	msgtext->setEditWordWrap(true);
	msgtext->setEditStatic(true);
	msgwin->setVisible(false);


	// network quality warning
	netmsgwin = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("FlowContainer", 5, 30, 300, 40,  MyGUI::Align::Default, "Main");
	netmsgwin->setAlpha(0.8f);
	MyGUI::ImageBox *nimg = netmsgwin->createWidget<MyGUI::ImageBox>("ImageBox", 0, 0, 16, 16,  MyGUI::Align::Default, "Main");
	nimg->setImageTexture("error.png");
	netmsgtext = netmsgwin->createWidget<MyGUI::TextBox>("TextBox", 18, 2, 300, 40,  MyGUI::Align::Default, "helptext");
	netmsgtext->setCaption(_L("Slow  Network  Download"));
	netmsgtext->setFontName("DefaultBig");
	netmsgtext->setTextColour(MyGUI::Colour::Red);
	netmsgtext->setFontHeight(lineheight);
	netmsgwin->setVisible(false);


	// now the main GUI
	MyGUI::IntSize gui_area = MyGUI::RenderManager::getInstance().getViewSize();
	int x=gui_area.width - 300, y=30;

	MyGUI::ImageBox *ib = MyGUI::Gui::getInstance().createWidget<MyGUI::ImageBox>("ImageBox", x, y, sidebarWidth, gui_area.height,  MyGUI::Align::Default, "Main");
	ib->setImageTexture("mpbg.png");

	mpPanel = ib; //->createWidget<MyGUI::Widget>("FlowContainer", x, y, sidebarWidth, gui_area.height,  MyGUI::Align::Default, "Main");
	mpPanel->setVisible(true);

	y=5;
	UTFString tmp;
	for (int i = 0; i < MAX_PEERS + 1; i++) // plus 1 for local entry
	{
		x=100; // space for icons
		player_row_t *row = &player_rows[i];
		row->playername = mpPanel->createWidget<MyGUI::TextBox>("TextBox", x, y+1, sidebarWidth, lineheight,  MyGUI::Align::Default, "Main");
		row->playername->setCaption("Player " + TOSTRING(i));
		row->playername->setFontName("DefaultBig");
		tmp = _L("user name");
		row->playername->setUserString("tooltip", tmp.asUTF8());
		row->playername->eventToolTip += MyGUI::newDelegate(this, &GUI_Multiplayer::openToolTip);
		row->playername->setNeedToolTip(true);
		row->playername->setVisible(false);
		row->playername->setFontHeight(lineheight);
		row->playername->setAlpha(1);

		x -= 18;
		row->flagimg = mpPanel->createWidget<MyGUI::ImageBox>("ImageBox", x, y + 3, 16, 11,  MyGUI::Align::Default, "Main");
		tmp = _L("user country");
		row->flagimg->setUserString("tooltip", tmp.asUTF8());
		row->flagimg->eventToolTip += MyGUI::newDelegate(this, &GUI_Multiplayer::openToolTip);
		row->flagimg->setNeedToolTip(true);
		row->flagimg->setVisible(false);

		x -= 18;
		row->statimg = mpPanel->createWidget<MyGUI::ImageBox>("ImageBox", x, y, 16, 16,  MyGUI::Align::Default, "Main");
		tmp = _L("user authentication level");
		row->statimg->setUserString("tooltip", tmp.asUTF8());
		row->statimg->eventToolTip += MyGUI::newDelegate(this, &GUI_Multiplayer::openToolTip);
		row->statimg->setNeedToolTip(true);
		row->statimg->setVisible(false);

		x -= 18;
		row->userTruckOKImg = mpPanel->createWidget<MyGUI::ImageBox>("ImageBox", x, y, 16, 16,  MyGUI::Align::Default, "Main");
		tmp = _L("truck loading state");
		row->userTruckOKImg->setUserString("tooltip", tmp.asUTF8());
		row->userTruckOKImg->eventToolTip += MyGUI::newDelegate(this, &GUI_Multiplayer::openToolTip);
		row->userTruckOKImg->setNeedToolTip(true);
		row->userTruckOKImg->setVisible(false);
		row->userTruckOKImg->eventMouseButtonClick += MyGUI::newDelegate(this, &GUI_Multiplayer::clickInfoIcon);

		x -= 18;
		row->userTruckOKRemoteImg = mpPanel->createWidget<MyGUI::ImageBox>("ImageBox", x, y, 16, 16,  MyGUI::Align::Default, "Main");
		tmp = _L("remote truck loading state");
		row->userTruckOKRemoteImg->setUserString("tooltip", tmp.asUTF8());
		row->userTruckOKRemoteImg->eventToolTip += MyGUI::newDelegate(this, &GUI_Multiplayer::openToolTip);
		row->userTruckOKRemoteImg->setNeedToolTip(true);
		row->userTruckOKRemoteImg->setVisible(false);
		row->userTruckOKRemoteImg->eventMouseButtonClick += MyGUI::newDelegate(this, &GUI_Multiplayer::clickInfoIcon);
		
		x -= 18;
		row->usergoimg = mpPanel->createWidget<MyGUI::ImageBox>("ImageBox", x, y, 16, 16,  MyGUI::Align::Default, "Main");
		row->usergoimg->setUserString("num", TOSTRING(i));
		tmp = _L("go to user");
		row->usergoimg->setUserString("tooltip", tmp.asUTF8());
		row->usergoimg->setImageTexture("user_go.png");
		row->usergoimg->eventToolTip += MyGUI::newDelegate(this, &GUI_Multiplayer::openToolTip);
		row->usergoimg->setNeedToolTip(true);
		row->usergoimg->setVisible(false);
		row->usergoimg->eventMouseButtonClick += MyGUI::newDelegate(this, &GUI_Multiplayer::clickUserGoIcon);

		/*
		img = MyGUI::Gui::getInstance().createWidget<MyGUI::ImageBox>("ImageBox", x-36, y, 16, 16,  MyGUI::Align::Default, "Overlapped");
		img->setImageTexture("information.png");
		img->eventMouseButtonClick += MyGUI::newDelegate(this, &GUI_Multiplayer::clickInfoIcon);
		img->eventToolTip += MyGUI::newDelegate(this, &GUI_Multiplayer::openToolTip);
		img->setNeedToolTip(true);
		img->setUserString("info", TOSTRING(i));
		img->setUserString("tooltip", _L("information about the user"));
		*/

		y += lineheight;
	}
}

GUI_Multiplayer::~GUI_Multiplayer()
{
	if (clients)
	{
		free(clients);
		clients = 0;
	}
}

void GUI_Multiplayer::updateSlot(player_row_t *row, user_info_t *c, bool self)
{
	if (!row || !c) return;

	int x = 100;
	int y = row->playername->getPosition().top;
	// name
	row->playername->setCaption(c->username);
	ColourValue col = PlayerColours::getSingleton().getColour(c->colournum);
	row->playername->setTextColour(MyGUI::Colour(col.r, col.g, col.b, col.a));
	row->playername->setVisible(true);
	x -= 18;
	
	// flag
	StringVector parts = StringUtil::split(String(c->language), "_");
	if (parts.size() == 2)
	{
		String lang = parts[1];
		StringUtil::toLowerCase(lang);
		row->flagimg->setImageTexture(lang + ".png");
		row->flagimg->setUserString("tooltip", _L("user language: ") + parts[0] + _L(" user country: ") + parts[1]);
		row->flagimg->setVisible(true);
		row->flagimg->setPosition(x, y);
		x -= 18;
	} else
	{
		row->flagimg->setVisible(false);
	}
	
	UTFString tmp;
	// auth
	if (c->authstatus == AUTH_NONE)
	{
		row->statimg->setVisible(false);
	} else if (c->authstatus & AUTH_ADMIN)
	{
		row->statimg->setVisible(true);
		row->statimg->setImageTexture("flag_red.png");
		tmp = _L("Server Administrator");
		row->statimg->setUserString("tooltip", tmp.asUTF8());
		row->statimg->setPosition(x, y);
		x -= 18;
	} else if (c->authstatus & AUTH_MOD)
	{
		row->statimg->setVisible(true);
		row->statimg->setImageTexture("flag_blue.png");
		tmp = _L("Server Moderator");
		row->statimg->setUserString("tooltip", tmp.asUTF8());
		row->statimg->setPosition(x, y);
		x -= 18;
	} else if (c->authstatus & AUTH_RANKED)
	{
		row->statimg->setVisible(true);
		row->statimg->setImageTexture("flag_green.png");
		tmp = _L("ranked user");
		row->statimg->setUserString("tooltip", tmp.asUTF8());
		row->statimg->setPosition(x, y);
		x -= 18;
	}

	// truck ok image
	if (!self)
	{
		row->userTruckOKImg->setVisible(true);
		row->userTruckOKRemoteImg->setVisible(true);
		row->userTruckOKImg->setUserString("uid", TOSTRING(c->uniqueid));
		row->userTruckOKRemoteImg->setUserString("uid", TOSTRING(c->uniqueid));
		row->userTruckOKImg->setPosition(x, y);
		x -= 10;
		row->userTruckOKRemoteImg->setPosition(x, y);
		x -= 10;

		int ok = BeamFactory::getSingleton().checkStreamsOK(c->uniqueid);
		if (ok == 0)
		{
			row->userTruckOKImg->setImageTexture("arrow_down_red.png");
			tmp = _L("Truck loading errors");
			row->userTruckOKImg->setUserString("tooltip", tmp.asUTF8());
		} else if (ok == 1)
		{
			row->userTruckOKImg->setImageTexture("arrow_down.png");
			tmp = _L("Truck loaded correctly, no errors");
			row->userTruckOKImg->setUserString("tooltip", tmp.asUTF8());
		} else if (ok == 2)
		{
			row->userTruckOKImg->setImageTexture("arrow_down_grey.png");
			tmp = _L("no truck loaded");
			row->userTruckOKImg->setUserString("tooltip", tmp.asUTF8());
		}

		int rok = BeamFactory::getSingleton().checkStreamsRemoteOK(c->uniqueid);
		if (rok == 0)
		{
			row->userTruckOKRemoteImg->setImageTexture("arrow_up_red.png");
			tmp = _L("Remote Truck loading errors");
			row->userTruckOKRemoteImg->setUserString("tooltip", tmp.asUTF8());
		} else if (rok == 1)
		{
			row->userTruckOKRemoteImg->setImageTexture("arrow_up.png");
			tmp = _L("Remote Truck loaded correctly, no errors");
			row->userTruckOKRemoteImg->setUserString("tooltip", tmp.asUTF8());
		} else if (rok == 2)
		{
			row->userTruckOKRemoteImg->setImageTexture("arrow_up_grey.png");
			tmp = _L("No Trucks loaded");
			row->userTruckOKRemoteImg->setUserString("tooltip", tmp.asUTF8());
		}
	} else
	{
		row->userTruckOKImg->setVisible(false);
		row->userTruckOKRemoteImg->setVisible(false);
	}
	
	// user go img
	row->usergoimg->setVisible(false);
	/*
	// disabled for now, since no use
	if (!self)
	{
		row->usergoimg->setVisible(true);
		row->usergoimg->setPosition(x, y);
		x -= 18;
		row->usergoimg->setUserString("uid", TOSTRING(c->uniqueid));
		if (eflsingleton && eflsingleton->getNetPointToUID() == c->uniqueid)
		{
			// active for this user
			row->usergoimg->setAlpha(1.0f);
		} else
		{
			// inactive for this user
			row->usergoimg->setAlpha(0.4f);
		}
	} else
	{
		row->usergoimg->setVisible(false);
	}
	*/
}

int GUI_Multiplayer::update()
{
	int slotid = 0;
	
	MyGUI::IntSize gui_area = MyGUI::RenderManager::getInstance().getViewSize();
	int x=gui_area.width - sidebarWidth, y=30;
	mpPanel->setPosition(x,y);

	// add local player to first slot always
	user_info_t *lu = gEnv->network->getLocalUserData();
	updateSlot(&player_rows[slotid], lu, true);
	slotid++;

	// add remote players
	int res = gEnv->network->getClientInfos(clients);
	if (res) return 1;
	for (int i = 0; i < MAX_PEERS; i++)
	{
		client_t *c = &clients[i];
		player_row_t *row = &player_rows[slotid];
		// only count up slotid for used slots, so there are no gap in the list
		if (c->used)
		{
			// used
			slotid++;
			try
			{
				updateSlot(row, &c->user, false);
			} catch(...)
			{
			}
		} else
		{
			// not used, hide everything
			row->flagimg->setVisible(false);
			row->playername->setVisible(false);
			row->statimg->setVisible(false);
			row->usergoimg->setVisible(false);
			row->userTruckOKImg->setVisible(false);
			row->userTruckOKRemoteImg->setVisible(false);
		}
	}

	int height = lineheight * (slotid + 1);
	mpPanel->setSize(sidebarWidth, height);
	
	if (gEnv->frameListener && gEnv->network->getNetQuality(true) != 0)
	{
		netmsgwin->setVisible(true);
	} else if (gEnv->frameListener && gEnv->network->getNetQuality(true) == 0)
	{
		netmsgwin->setVisible(false);
	}
	return 0;
}


void GUI_Multiplayer::clickUserGoIcon(MyGUI::WidgetPtr sender)
{
	int uid = StringConverter::parseInt(sender->getUserString("uid"));

	if (!gEnv->frameListener) return;

	if (gEnv->frameListener->getNetPointToUID() == uid)
		gEnv->frameListener->setNetPointToUID(-1);
	else
		gEnv->frameListener->setNetPointToUID(uid);
}

void GUI_Multiplayer::clickInfoIcon(MyGUI::WidgetPtr sender)
{
	//msgtext->setCaption("FOOBAR: "+sender->getUserString("info"));
	//msgwin->setVisible(true);
}

void GUI_Multiplayer::openToolTip(MyGUI::WidgetPtr sender, const MyGUI::ToolTipInfo &t)
{
	if (t.type == MyGUI::ToolTipInfo::Show)
	{
		String txt = sender->getUserString("tooltip");
		if (!txt.empty())
		{
			tooltipText->setCaption(txt);
			MyGUI::IntSize s = tooltipText->getTextSize();
			int newWidth = s.width + 10;
			tooltipPanel->setPosition(t.point - MyGUI::IntPoint(newWidth + 10, 10));
			tooltipPanel->setSize(newWidth, 20);
			tooltipText->setSize(newWidth, 16);
			tooltipPanel->setVisible(true);
		}
	} else if (t.type == MyGUI::ToolTipInfo::Hide)
	{
		tooltipPanel->setVisible(false);
	}
}

void GUI_Multiplayer::setVisible(bool value)
{
	mpPanel->setVisible(value);
}

bool GUI_Multiplayer::getVisible()
{
	return mpPanel->getVisible();
}

#endif // USE_SOCKETW
#endif // USE_MYGUI
