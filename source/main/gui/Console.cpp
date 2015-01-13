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