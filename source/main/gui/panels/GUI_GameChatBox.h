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
	@file   GUI_GameChatBox.h
	@author Moncef Ben Slimane
	@date   2/2015
*/

#include "ForwardDeclarations.h"
#include "GUI_GameChatBoxLayout.h"

namespace RoR
{

namespace GUI
{

class GameChatBox: public GameChatBoxLayout
{

public:
	GameChatBox();
	~GameChatBox();

	void Show();
	void Hide();
	bool IsVisible();
	void pushMsg(Ogre::String txt);
	void Update(float dt);
	void setNetChat(ChatSystem *c);

private:
	void eventCommandAccept(MyGUI::Edit* _sender);

	ChatSystem *netChat;

	Ogre::String mHistory;
	bool newMsg;
	bool isTyping;

	// logic
	float alpha;
	long pushTime;
};

} // namespace GUI

} // namespace RoR
