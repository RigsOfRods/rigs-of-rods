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
#ifdef USE_MYGUI

#ifndef LOBBYGUI_H__
#define LOBBYGUI_H__

#include "Singleton.h"
#include "IRCWrapper.h"
#include "mygui/BaseLayout.h"

ATTRIBUTE_CLASS_LAYOUT(LobbyGUI, "Lobby.layout");
class LobbyGUI :
	public wraps::BaseLayout,
	public RoRSingleton<LobbyGUI>,
	public IRCWrapper,
	public ZeroedMemoryAllocator
{
	friend class RoRSingleton<LobbyGUI>;
	LobbyGUI();
	~LobbyGUI();
public:

	void setVisible(bool _visible);
	bool getVisible();

	void select();

	// print waiting messages
	void frameEntered(float _frame);

	void update(float dt);

protected:
	typedef struct tabctx_t {
		Ogre::String name;
		Ogre::String realName;

		MyGUI::TabItemPtr tab;
		MyGUI::EditPtr txt;

		int mHistoryPosition;
		std::vector<MyGUI::UString> mHistory;
	} tabctx_t;

	ATTRIBUTE_FIELD_WIDGET_NAME(LobbyGUI, mainWindow, "_Main");
	MyGUI::Window* mainWindow;

	ATTRIBUTE_FIELD_WIDGET_NAME(LobbyGUI, tabControl, "tabControl");
	MyGUI::TabControl* tabControl;

	ATTRIBUTE_FIELD_WIDGET_NAME(LobbyGUI, commandBox, "commandBox");
	MyGUI::EditBox* commandBox;

	ATTRIBUTE_FIELD_WIDGET_NAME(LobbyGUI, statusText, "statusText");
	MyGUI::TextBox* statusText;

	ATTRIBUTE_FIELD_WIDGET_NAME(LobbyGUI, playerList, "playerList");
	MyGUI::ScrollView* playerList;

	ATTRIBUTE_FIELD_WIDGET_NAME(LobbyGUI, waitDisplay, "waitDisplay");
	MyGUI::ImageBox* waitDisplay;
	MyGUI::RotatingSkin *rotatingWait;
	
	std::map<Ogre::String , tabctx_t > tabs;
	tabctx_t *current_tab;
	bool waitingAnimation;

	void processIRCEvent(message_t &msg);
	void addTextToChatWindow(Ogre::String, Ogre::String channel);
	void addTab(Ogre::String name);

	void eventCommandAccept(MyGUI::Edit* _sender);
	void eventChangeTab(MyGUI::TabControl* _sender, size_t _index);
	void eventButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char);
};

#endif // LOBBYGUI_H__

#endif // USE_MYGUI
