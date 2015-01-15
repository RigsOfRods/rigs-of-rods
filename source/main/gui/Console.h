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
	@file   Console.h
*/

#pragma once

#ifdef USE_MYGUI

#define CONSOLE_PUTMESSAGE(a,b,c,d,e,f) while(0) { Console *console = RoR::Application::GetConsole(); if (console) console->putMessage(a,b,c,d,e,f); }
#define CONSOLE_PUTMESSAGE_SHORT(a,b,c) while(0) { Console *console = RoR::Application::GetConsole(); if (console) console->putMessage(a,b,c); }

#include "RoRPrerequisites.h"
#include "InterThreadStoreVector.h"

#include "mygui/BaseLayout.h"
#include "panels\GUI_GameConsoleLayout.h"

namespace RoR
{

struct ConsoleMessage
{
	//Kept this for comptability rather than remplacing hunderd of lines.
	char type; 
	int sender_uid; //Not used
	unsigned long time; //Not used !< post time in milliseconds since RoR start
	unsigned long ttl;  //Not used !< in milliseconds
	Ogre::UTFString txt; //!< not POD, beware...
	char icon[50]; //Not used
	bool forcevisible; //Not used
};

class Console :
	public Ogre::LogListener,
	public InterThreadStoreVector<ConsoleMessage>,
	public GUI::GameConsoleLayout,
	public ZeroedMemoryAllocator
{
	friend class Application;

public:

	Console();
	~Console();

	void setVisible(bool _visible);
	bool getVisible();

	enum { CONSOLE_MSGTYPE_LOG, CONSOLE_MSGTYPE_INFO, CONSOLE_MSGTYPE_SCRIPT, CONSOLE_MSGTYPE_NETWORK, CONSOLE_MSGTYPE_FLASHMESSAGE, CONSOLE_MSGTYPE_HIGHSCORE };

	enum {
		// detailed message type identifier, mostly used for message filtering
		CONSOLE_HELP,
		CONSOLE_TITLE,

		CONSOLE_LOCAL_SCRIPT, // script self

		CONSOLE_SYSTEM_NOTICE,
		CONSOLE_SYSTEM_ERROR,
		CONSOLE_SYSTEM_REPLY, // reply to a commands

		CONSOLE_SYSTEM_DEBUG,

		CONSOLE_LOGMESSAGE,
		CONSOLE_LOGMESSAGE_SCRIPT,

		MSG_CUSTOM,
	};

	void putMessage(int type, int uid, Ogre::UTFString msg, Ogre::String icon = "bullet_black.png", unsigned long ttl = 30000, bool forcevisible = false);

protected:
	void notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name);
	void showMessage(ConsoleMessage msg);

	Ogre::String ConsoleText;
	Ogre::UTFString TextCol;
	bool angelscriptMode;

#if OGRE_VERSION < ((1 << 16) | (8 << 8 ) | 0)
	void messageLogged(const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName);
#else
	void messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName, bool& skipThisMessage);
#endif // OGRE_VERSION

	void eventCommandAccept(MyGUI::Edit* _sender);
};

} // namespace RoR

#else // #ifdef USE_MYGUI

#define CONSOLE_PUTMESSAGE(a,b,c,d,e,f)
#define CONSOLE_PUTMESSAGE_SHORT(a,b,c)

#endif 
