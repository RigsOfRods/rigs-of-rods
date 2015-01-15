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

#ifdef USE_MYGUI

#include "Console.h"

#include "Application.h"
#include "Beam.h"
#include "BeamFactory.h"
#include "Character.h"
#include "ChatSystem.h"
#include "GUIManager.h"
#include "GUIMenu.h"
#include "IHeightFinder.h"
#include "HighScoreWindow.h"
#include "Language.h"
#include "Network.h"
#include "OverlayWrapper.h"
#include "RoRFrameListener.h"
#include "Scripting.h"
#include "Settings.h"
#include "TerrainManager.h"
#include "Utils.h"

#if MYGUI_PLATFORM == MYGUI_PLATFORM_LINUX
#include <iconv.h>
#endif // LINUX

using namespace Ogre;
using namespace RoR;
using namespace GUI;

//Console layout manager

// the delimiters that decide where a word is finished
const UTFString wordDelimiters = " \\\"\'|.,`!;<>~{}()+&%$@"; //TODO
const char *builtInCommands[] = {"/help", "/log", "/pos", "/goto", "/terrainheight", "/ver", "/save", "/whisper", "/as", NULL};

// class
Console::Console()
{
	MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);
	win->eventWindowButtonPressed += MyGUI::newDelegate(this, &Console::notifyWindowButtonPressed); //The "X" button thing

	//((MyGUI::Window*)mMainWidget)
	MyGUI::IntSize windowSize = ((MyGUI::Window*)mMainWidget)->getSize();
	MyGUI::IntSize parentSize = ((MyGUI::Window*)mMainWidget)->getParentSize();

	((MyGUI::Window*)mMainWidget)->setPosition((parentSize.width - windowSize.width) / 2, (parentSize.height - windowSize.height) / 2);

	if (BSETTING("Enable Ingame Console", false))
		setVisible(true);

	ConsoleText = "";

	m_Console_TextBox->eventEditSelectAccept += MyGUI::newDelegate(this, &Console::eventCommandAccept);

	LogManager::getSingleton().getDefaultLog()->addListener(this);
}

Console::~Console()
{
	
}

void Console::notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name)
{
	if (_name == "close")
		setVisible(false);
}

void Console::setVisible(bool _visible)
{
	((MyGUI::Window*)mMainWidget)->setVisible(_visible);
}

bool Console::getVisible()
{
	return ((MyGUI::Window*)mMainWidget)->getVisible();
}

void Console::putMessage( int type, int sender_uid, UTFString txt, String icon, unsigned long ttl, bool forcevisible )
{
	ConsoleMessage t;

	t.type       = type;
	t.sender_uid = sender_uid;
	t.time       = Root::getSingleton().getTimer()->getMilliseconds();
	t.ttl        = ttl;
	t.forcevisible = forcevisible;
	//strncpy(t.txt,  txt.c_str(), 2048);
	t.txt        = txt;
	strncpy(t.icon, icon.c_str(), 50);
	//t.channel = "default";

	showMessage(t);
}

void Console::showMessage(ConsoleMessage msg)
{
	TextCol = "";
	if (msg.type == CONSOLE_MSGTYPE_LOG)
		TextCol = "#FFFFFF";
	else if (msg.type == CONSOLE_MSGTYPE_INFO)
		TextCol = "#00FF00";
	else if (msg.type == CONSOLE_MSGTYPE_SCRIPT)
		TextCol = "#3399FF";
	else if (msg.type == CONSOLE_MSGTYPE_NETWORK)
		TextCol = "#9900FF";
	else if (msg.type == CONSOLE_MSGTYPE_FLASHMESSAGE)
		TextCol = "#00FFFF";
	else if (msg.type == CONSOLE_MSGTYPE_HIGHSCORE)
		TextCol = "#FFFFFF";
	else
		TextCol = "#FFFFFF";

	ConsoleText += TextCol + msg.txt + "\n";

	if (msg.forcevisible)
		setVisible(true);

	m_Console_MainBox->setMaxTextLength(ConsoleText.length() + 1);

	if (getVisible())
		m_Console_MainBox->setCaptionWithReplacing(ConsoleText);
}

#if OGRE_VERSION < ((1 << 16) | (8 << 8 ) | 0)
void Console::messageLogged(const String& message, LogMessageLevel lml, bool maskDebug, const String &logName)
#else
void Console::messageLogged(const String& message, LogMessageLevel lml, bool maskDebug, const String &logName, bool& skipThisMessage)
#endif // OGRE_VERSION
{
	String msg = message;
	//this->print(logName+": "+message);
	// strip script engine things
	if (message.substr(0, 4) == "SE| ")
	{
		msg = message.substr(4);
		putMessage(CONSOLE_MSGTYPE_SCRIPT, CONSOLE_LOGMESSAGE, UTFString("#FFFFFF") + (msg), "page_white_code.png");
	}
	else
	{
		if (BSETTING("Enable Ingame Console", false))
		{
			if (lml == LML_NORMAL)
				putMessage(CONSOLE_MSGTYPE_LOG, CONSOLE_LOGMESSAGE, UTFString("#FFFFFF") + (msg), "script_error.png");
			else if (lml == LML_TRIVIAL)
				putMessage(CONSOLE_MSGTYPE_LOG, CONSOLE_LOGMESSAGE, UTFString("#FF9900") + (msg), "script.png");
			else if (lml == LML_CRITICAL)
				putMessage(CONSOLE_MSGTYPE_LOG, CONSOLE_LOGMESSAGE, UTFString("#FF0000") + (msg), "script_lightning.png");
		}
	}
}

#endif //MYGUI

void Console::eventCommandAccept(MyGUI::Edit* _sender)
{
	UTFString msg = convertFromMyGUIString(_sender->getCaption());

	// we did not autoComplete, so try to handle the message
	_sender->setCaption("");

	if (msg.empty())
	{
		// discard the empty message
		return;
	}

	if (msg[0] == '/')
	{
		if (msg == "/help")
		{
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, ChatSystem::commandColour + _L("possible commands:"), "help.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("#dd0000/help#000000 - this help information"), "help.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("#dd0000/ver#000000  - shows the Rigs of Rods version"), "information.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("#dd0000/pos#000000  - outputs the current position"), "world.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("#dd0000/goto <x> <y> <z>#000000  - jumps to the mentioned position"), "world.png");

			if (gEnv->terrainManager->getHeightFinder())
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("#dd0000/terrainheight#000000  - get height of terrain at current position"), "world.png");

			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("#dd0000/log#000000  - toggles log output on the console"), "table_save.png");

			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("#dd0000/quit#000000 - exits"), "table_save.png");

			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, ChatSystem::commandColour + _L("tips:"), "help.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("- use #dd0000Arrow Up/Down Keys#000000 in the InputBox to reuse old messages"), "information.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("- use #dd0000Page Up/Down Keys#000000 in the InputBox to scroll through the history"), "information.png");

#ifdef USE_ANGELSCRIPT
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("- use #dd0000\\game.log(\"hello world!\");#000000 - if first character of a line is as backslash, the line is interpreted as AngelScript code"), "information.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("#dd0000/as#000000 - toggle AngelScript Mode: no need to put the backslash before script commands"), "script_go.png");
#endif // USE_ANGELSCRIPT
			return;
		}
		else if (msg == "/pos")
		{
			Beam *b = BeamFactory::getSingleton().getCurrentTruck();
			if (!b && gEnv->player)
			{
				Vector3 pos = gEnv->player->getPosition();
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Character position: ") + String("#dd0000") + TOSTRING(pos.x) + String("#000000, #00dd00") + TOSTRING(pos.y) + String("#000000, #0000dd") + TOSTRING(pos.z), "world.png");
			}
			else if (b)
			{
				Vector3 pos = b->getPosition();
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Vehicle position: ") + String("#dd0000") + TOSTRING(pos.x) + String("#000000, #00dd00") + TOSTRING(pos.y) + String("#000000, #0000dd") + TOSTRING(pos.z), "world.png");
			}

			return;
		}
		else if(msg.substr(0, 5) == "/goto")
		{
			StringVector args = StringUtil::split(msg, " ");

			if (args.size() != 4)
			{
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, ChatSystem::commandColour + _L("usage: /goto x y z"), "information.png");
				return;
			}

			Vector3 pos = Vector3(PARSEREAL(args[1]), PARSEREAL(args[2]), PARSEREAL(args[3]));

			Beam *b = BeamFactory::getSingleton().getCurrentTruck();
			if (!b && gEnv->player)
			{
				gEnv->player->setPosition(pos);
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Character position set to: ") + String("#dd0000") + TOSTRING(pos.x) + String("#000000, #00dd00") + TOSTRING(pos.y) + String("#000000, #0000dd") + TOSTRING(pos.z), "world.png");
			}
			else if (b)
			{
				b->resetPosition(pos, false);
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Vehicle position set to: ") + String("#dd0000") + TOSTRING(pos.x) + String("#000000, #00dd00") + TOSTRING(pos.y) + String("#000000, #0000dd") + TOSTRING(pos.z), "world.png");
			}

			return;
		} 
		else if (msg == "/terrainheight")
		{
			if (!gEnv->terrainManager->getHeightFinder()) return;
			Vector3 pos = Vector3::ZERO;

			Beam *b = BeamFactory::getSingleton().getCurrentTruck();
			if (!b && gEnv->player)
			{
				pos = gEnv->player->getPosition();
			}
			else if (b)
			{
				pos = b->getPosition();
			}

			Real h = gEnv->terrainManager->getHeightFinder()->getHeightAt(pos.x, pos.z);
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Terrain height at position: ") + String("#dd0000") + TOSTRING(pos.x) + String("#000000, #0000dd") + TOSTRING(pos.z) + String("#000000 = #00dd00") + TOSTRING(h), "world.png");

			return;
		}
		else if (msg == "/ver")
		{
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, ChatSystem::commandColour + getVersionString(false), "information.png");
			return;

		} 
		else if (msg == "/quit")
		{
			gEnv->frameListener->shutdown_final();
			return;

		}
		else if (msg == "/as")
		{
			//TODO
			angelscriptMode = !angelscriptMode;
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_NOTICE, ChatSystem::commandColour + _L("AngelScript Mode ") + (angelscriptMode ? _L("enabled") : _L("disabled")), "information.png");
			return;

		}
		else if (msg == "/log")
		{
			// switch to console logging
			bool logging = BSETTING("Enable Ingame Console", false);
			if (!logging)
			{
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_NOTICE, ChatSystem::commandColour + _L(" logging to console enabled"), "information.png");
				SETTINGS.setSetting("Enable Ingame Console", "Yes");
			}
			else
			{
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_NOTICE, ChatSystem::commandColour + _L(" logging to console disabled"), "information.png");
				SETTINGS.setSetting("Enable Ingame Console", "No");
			}
			return;
		}
		else
		{
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_NOTICE, ChatSystem::commandColour + _L(" unknown command: ") + msg, "error.png");
		}

		// scripting
#ifdef USE_ANGELSCRIPT
		if (angelscriptMode || msg[0] == '\\')
		{
			// we want to notify any running scripts that we might change something (prevent cheating)
			ScriptEngine::getSingleton().triggerEvent(SE_ANGELSCRIPT_MANIPULATIONS);

			String command = (angelscriptMode ? msg : msg.substr(1));

			StringUtil::trim(command);
			if (command.empty()) return;

			String nmsg = ChatSystem::scriptCommandColour + ">>> " + ChatSystem::normalColour + command;
			putMessage(CONSOLE_MSGTYPE_SCRIPT, CONSOLE_LOCAL_SCRIPT, nmsg, "script_go.png");
			int res = ScriptEngine::getSingleton().executeString(command);
			return;
		}
#endif //ANGELSCRIPT

	}
}