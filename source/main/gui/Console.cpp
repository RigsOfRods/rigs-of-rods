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
#include "Settings.h"
#include "TerrainManager.h"
#include "Utils.h"
#include "RoRVersion.h"
#include "IWater.h"
#include "Scripting.h"

#include <boost/lexical_cast.hpp>

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
	((MyGUI::Window*)mMainWidget)->setVisible(false);

	ConsoleText = "";
	iText = 0;
	HistoryCursor = 0;

	m_Console_TextBox->eventEditSelectAccept += MyGUI::newDelegate(this, &Console::eventCommandAccept);
	m_Console_TextBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &Console::eventButtonPressed);

	m_Console_Send->eventMouseButtonClick += MyGUI::newDelegate(this, &Console::eventMouseButtonClickSendButton);

#ifdef USE_ANGELSCRIPT
	//ScriptEngine::getSingleton().loadScript("Console.as");
#endif //USE_ANGELSCRIPT

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
	((MyGUI::Window*)mMainWidget)->setVisibleSmooth(_visible);
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
	//TextCol = "";
	TextCol = "#FFFFFF";
	/*
	if (msg.type == CONSOLE_MSGTYPE_LOG)
		TextCol = "#FFFFFF";
	else if (msg.type == CONSOLE_MSGTYPE_INFO)
		TextCol = "#00FF00";
	else if (msg.type == CONSOLE_MSGTYPE_SCRIPT)
		TextCol = "#3399FF";
	else if (msg.type == CONSOLE_MSGTYPE_NETWORK)
		TextCol = "#9900FF";
	else
		TextCol = "#FFFFFF";*/

	if (msg.sender_uid == CONSOLE_TITLE)
		TextCol = "#FF8100"; //Orange
	else if (msg.sender_uid == CONSOLE_SYSTEM_ERROR)
		TextCol = "#FF0000"; //Red
	else if (msg.sender_uid == CONSOLE_SYSTEM_REPLY)
		TextCol = "#00FF00"; //Green
	else if (msg.sender_uid == CONSOLE_HELP)
		TextCol = "#72C0E0"; //Light blue

	ConsoleText += TextCol + msg.txt + "\n";

	/*if (msg.forcevisible)
		setVisible(true);*/

	m_Console_MainBox->setMaxTextLength(ConsoleText.length() + 1);

	//if (getVisible())
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

void Console::eventMouseButtonClickSendButton(MyGUI::WidgetPtr _sender)
{
	m_Console_TextBox->_riseKeyButtonPressed(MyGUI::KeyCode::Return, ' ');
}

void Console::eventButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char)
{
	switch (_key.toValue())
	{
		case MyGUI::KeyCode::ArrowUp:
		{
			if (HistoryCursor > iText)
				HistoryCursor = 0;

			if (sTextHistory[HistoryCursor] != "")
				m_Console_TextBox->setCaption(sTextHistory[HistoryCursor]);

			HistoryCursor++;
		}
		break;

		case MyGUI::KeyCode::ArrowDown:
		{
			if (HistoryCursor < 0)
				HistoryCursor = iText;

			if (sTextHistory[HistoryCursor] != "")
				m_Console_TextBox->setCaption(sTextHistory[HistoryCursor]);	

			HistoryCursor--;
		}
		break;
	}
}

/*
All commands are here
*/
void Console::eventCommandAccept(MyGUI::Edit* _sender)
{
	UTFString msg = convertFromMyGUIString(m_Console_TextBox->getCaption());

	// we did not autoComplete, so try to handle the message
	m_Console_TextBox->setCaption("");

	if (msg.empty())
	{
		// discard the empty message
		return;
	}
	if (msg[0] == '/' || msg[0] == '\\')
	{
		Ogre::StringVector args = StringUtil::split(msg, " ");
		sTextHistory[iText] = msg;
		iText++; //Used for text history
		if (args[0] == "/help")
		{
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_TITLE, _L("Available commands:"), "help.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("/help - this help information"), "help.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("/ver - shows the Rigs of Rods version"), "information.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("/pos - outputs the current position"), "world.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("/goto <x> <y> <z> - jumps to the mentioned position"), "world.png");


			//if (gEnv->terrainManager->getHeightFinder()) //Not needed imo -max98
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("/terrainheight - get height of terrain at current position"), "world.png");

			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("/log - toggles log output on the console"), "table_save.png");

			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("/quit - exits"), "table_save.png");

#ifdef USE_ANGELSCRIPT	
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("/as <code here> - interpret angel code using console"), "script_go.png");
#endif // USE_ANGELSCRIPT

			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("/setgravity <real> or <text string> - changes terrain's gravity"), "script_go.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("Possible values: \n earth \n moon \n jupiter \n A random number (use negative)"), "script_go.png");

			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("/setwaterlevel <real> - changes water's level"), "script_go.png");

			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_TITLE, _L("Tips:"), "help.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("- use Arrow Up/Down Keys in the InputBox to reuse old messages"), "information.png");
			return;
		} else if (args[0] == "/setgravity")
		{
			float gValue;
			if (args[1] == "earth")
				gValue = -9.81;
			else if (args[1] == "moon")
				gValue = -1.6;
			else if (args[1] == "jupiter")
				gValue = -50;
			else
				gValue = boost::lexical_cast<float>(args[1].c_str());

			gEnv->terrainManager->setGravity(gValue);
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Gravity set to: ") + StringConverter::toString(gValue), "information.png");
			gValue = NULL;
			return;
		} else if (args[0] == "/setwaterlevel")
		{
			if (gEnv->terrainManager && gEnv->terrainManager->getWater())
			{
				IWater* water = gEnv->terrainManager->getWater();
				water->setCamera(gEnv->mainCamera);
				water->setHeight(boost::lexical_cast<float>(args[1].c_str()));
				water->update();
			}
			return;
		}
		else if (args[0] == "/pos" && (gEnv->frameListener->loading_state == TERRAIN_LOADED || gEnv->frameListener->loading_state == ALL_LOADED))
		{
			Beam *b = BeamFactory::getSingleton().getCurrentTruck();
			if (!b && gEnv->player)
			{
				Vector3 pos = gEnv->player->getPosition();
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Character position: ") + String("x: ") + TOSTRING(pos.x) + String(" y: ") + TOSTRING(pos.y) + String(" z: ") + TOSTRING(pos.z), "world.png");
			}
			else if (b)
			{
				Vector3 pos = b->getPosition();
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Vehicle position: ") + String("x: ") + TOSTRING(pos.x) + String(" y: ") + TOSTRING(pos.y) + String(" z: ") + TOSTRING(pos.z), "world.png");
			}

			return;
		}
		else if (args[0] == "/goto" && (gEnv->frameListener->loading_state == TERRAIN_LOADED || gEnv->frameListener->loading_state == ALL_LOADED))
		{
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
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Character position set to: ") + String("x: ") + TOSTRING(pos.x) + String(" y: ") + TOSTRING(pos.y) + String(" z: ") + TOSTRING(pos.z), "world.png");
			}
			else if (b)
			{
				b->resetPosition(pos, false);
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Vehicle position set to: ") + String("x: ") + TOSTRING(pos.x) + String(" y: ") + TOSTRING(pos.y) + String(" z: ") + TOSTRING(pos.z), "world.png");
			}

			return;
		}
		else if (args[0] == "/terrainheight" && (gEnv->frameListener->loading_state == TERRAIN_LOADED || gEnv->frameListener->loading_state == ALL_LOADED))
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
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Terrain height at position: ") + String("x: ") + TOSTRING(pos.x) + String("z: ") + TOSTRING(pos.z) + _L(" = ")  + TOSTRING(h), "world.png");

			return;
		}
		else if (args[0] == "/ver")
		{
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_TITLE, "Rigs of Rods:", "information.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, " Version: " + String(ROR_VERSION_STRING), "information.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, " Protocol version: " + String(RORNET_VERSION), "information.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, " build time: " + String(__DATE__) + ", " + String(__TIME__), "information.png");
			return;

		}
		else if (args[0] == "/quit")
		{
			gEnv->frameListener->shutdown_final();
			return;

		}
#ifdef USE_ANGELSCRIPT
		else if (args[0] == "/as" && (gEnv->frameListener->loading_state == TERRAIN_LOADED || gEnv->frameListener->loading_state == ALL_LOADED))
		{
			// we want to notify any running scripts that we might change something (prevent cheating)
			ScriptEngine::getSingleton().triggerEvent(SE_ANGELSCRIPT_MANIPULATIONS);

			String command = msg.substr(args[0].length());

			StringUtil::trim(command);
			if (command.empty()) return;

			String nmsg = ChatSystem::scriptCommandColour + ">>> " + ChatSystem::normalColour + command;
			putMessage(CONSOLE_MSGTYPE_SCRIPT, CONSOLE_LOCAL_SCRIPT, nmsg, "script_go.png");
			int res = ScriptEngine::getSingleton().executeString(command);
			return;
		}
#endif //ANGELSCRIPT
		else if (args[0] == "/log")
		{
			// switch to console logging
			bool logging = BSETTING("Enable Ingame Console", false);
			if (!logging)
			{
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_NOTICE, _L(" logging to console enabled"), "information.png");
				SETTINGS.setSetting("Enable Ingame Console", "Yes");
			}
			else
			{
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_NOTICE, _L(" logging to console disabled"), "information.png");
				SETTINGS.setSetting("Enable Ingame Console", "No");
			}
			return;
		}
		else
		{
			//TODO: Angelscript here
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_ERROR, _L("unknown command: ") + msg, "error.png");
		}
	}

}