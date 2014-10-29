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

#include "Console.h"

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

// the delimiters that decide where a word is finished
const UTFString Console::wordDelimiters = " \\\"\'|.,`!;<>~{}()+&%$@";
const char *builtInCommands[] = {"/help", "/log", "/pos", "/goto", "/terrainheight", "/ver", "/save", "/whisper", "/as", NULL};

// class
Console::Console() : netChat(0), top_border(20), bottom_border(100), message_counter(0), mHistory(), mHistoryPosition(0), inputMode(false), linesChanged(false), scrollOffset(0), autoCompleteIndex(-1), linecount(10), scroll_size(5), angelscriptMode(false)
{
	setSingleton(this);
	mMainWidget = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("default", 0, 0, 400, 300,  MyGUI::Align::Center, "Back", "Console");
	mMainWidget->setCaption(_L("Console"));
	mMainWidget->setAlpha(0.9f);

	memset(&lines, 0, sizeof(lines));

	mHistory.push_back(UTFString());

	// and the textbox inside
	mCommandEdit = mMainWidget->createWidget<MyGUI::EditBox>("EditBoxChat", 0, 0, 304, lineheight * 1.2f,  MyGUI::Align::Default, "ConsoleInput");
	mCommandEdit->setProperty("WordWrap", "false");
	mCommandEdit->setProperty("TextAlign", "Left Bottom");
	mCommandEdit->setProperty("ReadOnly", "false");
	mCommandEdit->setProperty("OverflowToTheLeft", "true");
	mCommandEdit->setProperty("MaxTextLength", "8192");
	mCommandEdit->setWidgetStyle(MyGUI::WidgetStyle::Child);
	mCommandEdit->setCaption("");
	mCommandEdit->setTabPrinting(false);
	mCommandEdit->setEnabled(false);
	mCommandEdit->setVisible(false);

	mCommandEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &Console::eventButtonPressed);
	mCommandEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &Console::eventCommandAccept);

	// auto complete list, one layer higher so its visible above the chat
	mAutoCompleteList = MyGUI::Gui::getInstance().createWidget<MyGUI::ListBox>("ListBox", 0, 0, 304, lineheight * 1.2f,  MyGUI::Align::Default, "Main", "ConsoleInput");
	//mAutoCompleteList->setProperty("WordWrap", "false");
	mAutoCompleteList->setWidgetStyle(MyGUI::WidgetStyle::Child);
	mAutoCompleteList->removeAllItems();
	mAutoCompleteList->setVisible(false);


	popMenu = MyGUI::Gui::getInstance().createWidget<MyGUI::PopupMenu>("PopupMenu", 0, 0, 300, 143, MyGUI::Align::Default, "Main", "PopupMenu");
	popMenu->removeAllItems();
	//w->addItem("#ffffffExpandable Item  >", MyGUI::MenuItemType::Popup);
	popMenu->eventMenuCtrlAccept += MyGUI::newDelegate(this, &Console::onPopUpBtn);
	popMenu->setVisible(false);

	// scroll icons
	scrollImgUp = mMainWidget->createWidget<MyGUI::ImageBox>("ChatIcon", 0, 0, lineheight, lineheight * 2,  MyGUI::Align::Default, "ConsoleIconScrollUp");
	scrollImgUp->setProperty("ImageTexture", "arrow_up.png");
	scrollImgUp->setWidgetStyle(MyGUI::WidgetStyle::Child);
	scrollImgUp->setVisible(false);
	MyGUI::RotatingSkin *rotatingIcon = scrollImgUp->getSubWidgetMain()->castType<MyGUI::RotatingSkin>();
	rotatingIcon->setCenter(MyGUI::IntPoint(lineheight*0.5f,lineheight*0.5f));
	//rotatingIcon->setAngle(Degree(90).valueRadians());

	scrollImgDown = mMainWidget->createWidget<MyGUI::ImageBox>("ChatIcon", 0, 0, lineheight, lineheight * 2,  MyGUI::Align::Default, "ConsoleIconScrollDown");
	scrollImgDown->setProperty("ImageTexture", "arrow_down.png");
	scrollImgDown->setWidgetStyle(MyGUI::WidgetStyle::Child);
	scrollImgDown->setVisible(false);
	rotatingIcon = scrollImgDown->getSubWidgetMain()->castType<MyGUI::RotatingSkin>();
	rotatingIcon->setCenter(MyGUI::IntPoint(lineheight*0.5f,lineheight*0.5f));
	//rotatingIcon->setAngle(Degree(90).valueRadians());


	// the rest
	setVisible(false);

	MyGUI::Gui::getInstance().eventFrameStart += MyGUI::newDelegate( this, &Console::frameEntered );

	LogManager::getSingleton().getDefaultLog()->addListener(this);
}

Console::~Console()
{
	LogManager::getSingleton().getDefaultLog()->removeListener(this);
	MyGUI::Gui::getInstance().eventFrameStart -= MyGUI::newDelegate( this, &Console::frameEntered );
}


void Console::setVisible(bool _visible)
{
	mMainWidget->setEnabledSilent(_visible);
	mMainWidget->setVisible(_visible);

	// DO NOT change focus here

	if (!_visible)
	{
		inputMode = false;
	}
}

bool Console::getVisible()
{
	return mMainWidget->getVisible();
}


void Console::select(UTFString start)
{
	MyGUI::InputManager::getInstance().setKeyFocusWidget(mCommandEdit);
	mCommandEdit->setEnabled(true);
	mCommandEdit->setVisible(true);
	inputMode = true;
	linesChanged = true;

	if (!start.empty())
	{
		MyGUI::UString s = convertToMyGUIString(start);
		mCommandEdit->setCaption(s);
		mCommandEdit->setTextCursor(s.size());
	}
}

void Console::startPrivateChat(int target_uid)
{
	if (!gEnv->network) return;

	client_t *c = gEnv->network->getClientInfo(target_uid);
	if (!c) return;

	Console::getSingleton().setVisible(true);
	Console::getSingleton().select("/whisper " + UTFString(c->user.username) + " ");
}

void Console::unselect()
{
	MyGUI::InputManager::getInstance().resetKeyFocusWidget();
	GUIManager::getSingleton().unfocus();
}


void Console::frameEntered(float dt)
{
	messageUpdate(dt);

	updateGUIVisual(dt);
}

void Console::eventButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char)
{
	// any delimiter will abort the auto completion
	if (autoCompleteIndex != -1 && wordDelimiters.find(_char) != wordDelimiters.npos)
		abortAutoCompletion();

	switch(_key.toValue())
	{
	case MyGUI::KeyCode::Delete:
		{
			if (autoCompleteIndex != -1)
				abortAutoCompletion();
		}
		break;
	case MyGUI::KeyCode::ArrowUp:
		{
			if (autoCompleteIndex != -1)
			{
				walkAutoCompletion(false);
				// fixing cursor
				mCommandEdit->setTextCursor(autoCompletionCursor);
			} else if (mHistoryPosition > 0)
			{
				if (mHistoryPosition == (int)mHistory.size() - 1)
					mHistory[mHistoryPosition] = convertFromMyGUIString(mCommandEdit->getCaption());
				mHistoryPosition--;
				mCommandEdit->setCaption(convertToMyGUIString(mHistory[mHistoryPosition]));
			}
		}
		break;
	case MyGUI::KeyCode::ArrowDown:
		{
			if (autoCompleteIndex != -1)
			{
				walkAutoCompletion();
				// fixing cursor
				mCommandEdit->setTextCursor(autoCompletionCursor);
			} if (mHistoryPosition < (int)mHistory.size() - 1)
			{
				mHistoryPosition++;
				mCommandEdit->setCaption(convertToMyGUIString(mHistory[mHistoryPosition]));
			}
		}
		break;
	case MyGUI::KeyCode::PageUp:
		{
			if (scrollOffset + scroll_size <= (message_counter - linecount))
			{
				scrollOffset += scroll_size;
				linesChanged = true;
			}
		}
		break;
	case MyGUI::KeyCode::PageDown:
		{
			scrollOffset -= scroll_size;
			if (scrollOffset < 0) scrollOffset = 0;
			linesChanged = true;
		}
		break;
	case MyGUI::KeyCode::Tab:
		{
			initOrWalkAutoCompletion();
		}
		break;
	case MyGUI::KeyCode::Space:
		{
			if (MyGUI::InputManager::getInstance().isControlPressed())
				initOrWalkAutoCompletion();
		}
		break;
	case MyGUI::KeyCode::Return:
		{
			// IGNORE HERE
			// we handle it in accept()
		}
		break;
	default:
		{
			// re-dice with possible new characters on the word?
			bool wasAutocompleting = (autoCompleteIndex != -1);
			abortAutoCompletion();
			// restart again
			if (wasAutocompleting)
				initOrWalkAutoCompletion();
		}
		break;
	}
}

void Console::findCurrentWord()
{
	UTFString line = convertFromMyGUIString(mCommandEdit->getCaption());
	autoCompletionCursor = (int)mCommandEdit->getTextCursor();

	// look for word start
	// now search the current word we are working with
	autoCompletionWordStart = autoCompletionCursor - 1;
	// enforce limits
	autoCompletionWordStart = std::min<int>(autoCompletionWordStart, (int)line.size() - 1);
	autoCompletionWordStart = std::max<int>(autoCompletionWordStart, 0);
	for (int counter = 0; autoCompletionWordStart > 0; autoCompletionWordStart--)
	{
		if (wordDelimiters.find(line[autoCompletionWordStart]) != wordDelimiters.npos)
		{
			if (counter > 0) break;
		} else
		{
			counter++;
		}
	}
	autoCompletionWordStart++;
	// enforce limits
	autoCompletionWordStart = std::min<int>(autoCompletionWordStart, (int)line.size() - 1);
	autoCompletionWordStart = std::max<int>(autoCompletionWordStart, 0);

	// find end
	autoCompletionWordEnd   = autoCompletionWordStart;
	autoCompletionWordEnd   = std::min<int>(autoCompletionWordEnd, (int)line.size() - 1);
	autoCompletionWordEnd   = std::max<int>(autoCompletionWordEnd, 0);
	for (int counter = 0; autoCompletionWordEnd < (int)line.size(); autoCompletionWordEnd++)
	{
		if (wordDelimiters.find(line[autoCompletionWordEnd]) != wordDelimiters.npos)
		{
			if (counter > 0) break;
		} else
		{
			counter++;
		}
	}
	autoCompletionWordEnd--;
	autoCompletionWordEnd   = std::min<int>(autoCompletionWordEnd, (int)line.size() - 1);
	autoCompletionWordEnd   = std::max<int>(autoCompletionWordEnd, 0);


	// extract the word
	autoCompletionWord = line.substr(autoCompletionWordStart, autoCompletionWordEnd - autoCompletionWordStart + 1);
}

// init the auto-completion or walk it when it is already initialized
void Console::initOrWalkAutoCompletion()
{
	if (autoCompleteIndex == -1)
	{
		// auto-complete just triggered, so get choices and show window
		autoCompleteChoices.clear();
		mAutoCompleteList->removeAllItems();

		findCurrentWord();

		// word invalid? then abort auto-completion
		if (autoCompletionWord.empty()) return;

		// add the incomplete version as first entry
		//autoCompleteChoices.push_back(autoCompletionWord);
		//mAutoCompleteList->addItem(autoCompletionWord);

		// Auto-completion for the network usernames
		if (gEnv->network && netChat)
		{
			std::vector<UTFString> names;
			int res = netChat->getChatUserNames(names);

			for (unsigned int i = 0; i < names.size(); i++)
			{
				// TODO: case insensitive comparison between UTFString

				UTFString a = names[i].substr(0, autoCompletionWord.size());
				//std::transform(a.begin(), a.end(), a.begin(), tolower);

				UTFString b = autoCompletionWord;
				//std::transform(b.begin(), b.end(), b.begin(), tolower);

				if (a == b)
				{
					autoCompleteChoices.push_back(names[i]);
					mAutoCompleteList->addItem(convertToMyGUIString(names[i]));
				}
			}
		}

		// auto-completion phrases for the built-in commands, ONLY at the line start
		if (autoCompletionWordStart == 0)
		{
			for (int i = 0; builtInCommands[i]; i++)
			{
				UTFString us = UTFString(builtInCommands[i]);
				if (us.substr(0, autoCompletionWord.size()) == autoCompletionWord)
				{
					autoCompleteChoices.push_back(us);
					mAutoCompleteList->addItem(convertToMyGUIString(us));
				}
			}
		}

#ifdef USE_ANGELSCRIPT
		// TODO: add auto-completion for AngelScript
#endif // USE_ANGELSCRIPT

		// no valid completions found, abort, we added the word to it as well ...
		if (autoCompleteChoices.size() < 1) return;

		autoCompleteIndex = 0;
		mAutoCompleteList->setIndexSelected(autoCompleteIndex);

		// idea: if there is only one possible completion, chose that?

		// now try to position the auto-completion window in the best possible way
		const int height = mAutoCompleteList->getOptimalHeight();
		const int width  = 250; // TODO: use dynamic text width?

		// TODO: the left position is not precise
		mAutoCompleteList->setCoord(
			mCommandEdit->getLeft() + mCommandEdit->getTextSize().width,
			mCommandEdit->getTop() - height,
			width,
			height
			);
		mAutoCompleteList->setVisible(true);

	} else
	{
		if (autoCompleteChoices.size() == 1)
		{
			// just one choice, just select that entry on 2nd tab press
			finalizeAutoCompletion();
		} else
		{
			walkAutoCompletion();
		}
	}
}

// walk the auto-completion
void Console::walkAutoCompletion(bool direction)
{
	if (direction)
	{
		autoCompleteIndex++;
		// + 1 due to first entry = incomplete word
		if (autoCompleteIndex >= (int)autoCompleteChoices.size()) autoCompleteIndex = 0;
	} else
	{
		autoCompleteIndex--;
		// + 1 due to first entry = incomplete word
		if (autoCompleteIndex < 0) autoCompleteIndex = (int)autoCompleteChoices.size() - 1;
	}

	mAutoCompleteList->setIndexSelected(autoCompleteIndex);
}

// abort auto-completion
void Console::abortAutoCompletion()
{
	// reset everything
	autoCompleteIndex = -1;
	autoCompleteChoices.clear();
	mAutoCompleteList->removeAllItems();
	mAutoCompleteList->setVisible(false);
	autoCompletionWord = UTFString();
}

void Console::finalizeAutoCompletion()
{
	// now add the word we chose
	UTFString line = convertFromMyGUIString(mCommandEdit->getCaption());

	// construct final string
	UTFString strA = line.substr(0, autoCompletionWordStart) + autoCompleteChoices[autoCompleteIndex];
	UTFString strB = line.substr(autoCompletionWordEnd + 1);
	UTFString str  = strA + strB;
	// and set the text
	mCommandEdit->setCaption(convertToMyGUIString(str));

	// and put the cursor to a useful position
	mCommandEdit->setTextCursor(strA.size());	


	// auto completion done
	abortAutoCompletion();
}

void Console::eventCommandAccept(MyGUI::Edit* _sender)
{
	UTFString msg = convertFromMyGUIString(_sender->getCaption());

	// did we do auto completion?!
	if (autoCompleteIndex != -1)
	{
		// yes, find out what the user selected
		finalizeAutoCompletion();
		return;
	}

	// we did not autoComplete, so try to handle the message
	_sender->setCaption("");

	// unfocus, so we return to the main game for the keyboard focus
	inputMode = false;
	linesChanged = true;
	scrollOffset = 0; // reset offset
	unselect();
	mCommandEdit->setEnabled(false);
	mCommandEdit->setVisible(false);

	if (msg.empty())
	{
		// discard the empty message
		return;
	}

	// record the history
	*mHistory.rbegin() = msg;
	mHistory.push_back(""); // new, empty last entry
	mHistoryPosition = (int)mHistory.size() - 1; // switch to the new line
	mCommandEdit->setCaption(convertToMyGUIString(mHistory[mHistoryPosition]));

	// some specials
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
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("#dd0000/save#000000 - saves the chat history to a file"), "table_save.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("#dd0000/log#000000  - toggles log output on the console"), "table_save.png");
			if (gEnv->network)
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("#dd0000/whisper <username> <message>#000000 - send someone a private message"), "script_key.png");
	#ifdef USE_ANGELSCRIPT
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("#dd0000/as#000000 - toggle AngelScript Mode: no need to put the backslash before script commands"), "script_go.png");
	#endif // USE_ANGELSCRIPT
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("#dd0000/quit#000000 - exits"), "table_save.png");

			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, ChatSystem::commandColour + _L("tips:"), "help.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("- use #dd0000Arrow Up/Down Keys#000000 in the InputBox to reuse old messages"), "information.png");
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("- use #dd0000Page Up/Down Keys#000000 in the InputBox to scroll through the history"), "information.png");
			if (gEnv->network)
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("- click on a username in the vehicle menu to start a private chat"), "information.png");
	#ifdef USE_ANGELSCRIPT
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, _L("- use #dd0000\\game.log(\"hello world!\");#000000 - if first character of a line is as backslash, the line is interpreted as AngelScript code"), "information.png");
	#endif // USE_ANGELSCRIPT
			return;
		} else if (msg == "/pos")
		{
			outputCurrentPosition();
			return;

		} else if (msg.substr(0, 5) == "/goto")
		{
			StringVector args = StringUtil::split(msg, " ");

			if (args.size() != 4)
			{
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_HELP, ChatSystem::commandColour + _L("usage: /goto x y z"), "information.png");
				return;
			}

			Vector3 pos = Vector3(PARSEREAL(args[1]), PARSEREAL(args[2]), PARSEREAL(args[3]));

			jumpToPosition(pos);
			return;

		} else if (msg == "/terrainheight")
		{
			outputCurrentTerrainHeight();
			return;

		} else if (msg == "/ver")
		{
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, ChatSystem::commandColour + getVersionString(false), "information.png");
			return;

		} else if (msg == "/quit")
		{
			gEnv->frameListener->shutdown_final();
			return;

		} else if (msg == "/save")
		{
			saveChat(SSETTING("Log Path", "") + "chat-log.txt");
			return;

		} else if (msg.substr(0, 8) == "/whisper")
		{
			StringVector args = StringUtil::split(msg, " ", 2);

			if (args.size() != 3)
			{
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_NOTICE, ChatSystem::commandColour + _L("usage: /whisper username message"), "information.png");
				return;
			}
			netChat->sendPrivateChat(args[1], args[2]);
			return;

		} else if (msg == "/as")
		{
			angelscriptMode = !angelscriptMode;
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_NOTICE, ChatSystem::commandColour + _L("AngelScript Mode ") + (angelscriptMode ? _L("enabled") : _L("disabled")), "information.png");
			return;

		} else if (msg == "/log")
		{
			// switch to console logging
			bool logging = BSETTING("Enable Ingame Console", false);
			if (!logging)
			{
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_NOTICE, ChatSystem::commandColour + _L(" logging to console enabled"), "information.png");
				SETTINGS.setSetting("Enable Ingame Console", "Yes");
			} else
			{
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_NOTICE, ChatSystem::commandColour + _L(" logging to console disabled"), "information.png");
				SETTINGS.setSetting("Enable Ingame Console", "No");
			}
			return;
		
		// some debugging things below ;)
		} else if (msg == "/test1")
		{
			for (int i=0; i<600; i++)
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_DEBUG, "TEST " + TOSTRING(i), "cog.png");
			return;

		} else if (msg == "/test2")
		{
			for (int i=0; i<MESSAGES_MAX*3; i++)
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_DEBUG, "OVERFLOW_TEST " + TOSTRING(i) + " / size: " + TOSTRING(size()), "cog.png");
			return;

		} else if (msg == "/fadetest")
		{
			for (int i=0; i<10; i++)
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_DEBUG, "FADE-TEST: down " + TOSTRING(i), "cog.png", i*1000);
			for (int i=0; i<10; i++)
				putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_DEBUG, "FADE-TEST: up " + TOSTRING(10-i), "cog.png", (10-i)*1000);
			return;

		} else
		{
			putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_NOTICE, ChatSystem::commandColour + _L(" unknown command: ") + msg, "error.png");
		}
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

	// normal network chat as last chance to use this message
	if (gEnv->network && netChat)
	{
		// TODO: trim it before sending ...
		netChat->sendChat(msg.c_str());
		return;
	}
}

void Console::setNetChat(ChatSystem *c)
{
	netChat = c;
}

#if OGRE_VERSION < ((1 << 16) | (8 << 8 ) | 0)
void Console::messageLogged( const String& message, LogMessageLevel lml, bool maskDebug, const String &logName)
#else
void Console::messageLogged( const String& message, LogMessageLevel lml, bool maskDebug, const String &logName, bool& skipThisMessage)
#endif // OGRE_VERSION
{
	String msg = message;
	//this->print(logName+": "+message);
	// strip script engine things
	if (message.substr(0,4) == "SE| ")
	{
		msg = message.substr(4);
		putMessage(CONSOLE_MSGTYPE_SCRIPT, CONSOLE_LOGMESSAGE, UTFString("#988310") + (msg), "page_white_code.png");
	} else
	{
		if (BSETTING("Enable Ingame Console", false))
		{
			if (lml == LML_NORMAL)
				putMessage(CONSOLE_MSGTYPE_LOG, CONSOLE_LOGMESSAGE, UTFString("#988310") + (msg), "script_error.png");
			else if (lml == LML_TRIVIAL)
				putMessage(CONSOLE_MSGTYPE_LOG, CONSOLE_LOGMESSAGE, UTFString("#988310") + (msg), "script.png");
			else if (lml == LML_CRITICAL)
				putMessage(CONSOLE_MSGTYPE_LOG, CONSOLE_LOGMESSAGE, UTFString("#988310") + (msg), "script_lightning.png");
		}
	}
}

void Console::resized()
{
	GUI_MainMenu *menu = GUI_MainMenu::getSingletonPtr();
	if (menu) top_border = menu->getMenuHeight();

	MyGUI::IntSize size = MyGUI::RenderManager::getInstance().getViewSize();
	
	// 15% of the window height is the overlay
	OverlayWrapper *ow = OverlayWrapper::getSingletonPtr();
	if (ow) bottom_border = size.height - ow->getDashBoardHeight() + 20;

	int height = size.height - bottom_border - top_border;
	int width  = size.width;
	mMainWidget->setCoord(0, top_border, width, size.height - bottom_border);
	
	linecount = height / (float)lineheight;
	if (linecount >= LINES_MAX)
		linecount = LINES_MAX - 1;

	scroll_size = linecount * 0.5f;

	// add controls
	for (unsigned int i = 0; i < linecount; i++)
	{
		if (lines[i].txtctrl)
		{
			// do not set visibility here, we do that in updateGUILines()
			lines[i].txtctrl->setSize(width, lines[i].txtctrl->getHeight());
			continue;
		}
		mygui_console_line_t line;
		memset(&line, 0, sizeof(line));
		line.number = i;

		line.txtctrl = mMainWidget->createWidget<MyGUI::TextBox>("TextBoxChat", lineheight + 2, i*lineheight, width, lineheight,  MyGUI::Align::Left | MyGUI::Align::Right | MyGUI::Align::Top, "ConsoleLine" + TOSTRING(i));
		line.txtctrl->setWidgetStyle(MyGUI::WidgetStyle::Child);
		line.txtctrl->setCaption("> LINE " + TOSTRING(i) + " >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
		line.txtctrl->setVisible(false);
		//line.txtctrl->eventMouseButtonClick += MyGUI::newDelegate(this, &Console::onLineClicked);

		line.iconctrl = mMainWidget->createWidget<MyGUI::ImageBox>("ChatIcon", 0, i*lineheight, lineheight, lineheight,  MyGUI::Align::Default, "ConsoleIcon"+TOSTRING(i));
		line.iconctrl->setProperty("ImageTexture", "arrow_left.png");
		line.iconctrl->setWidgetStyle(MyGUI::WidgetStyle::Child);
		line.iconctrl->setVisible(false);
		line.iconctrl->eventMouseButtonClick += MyGUI::newDelegate(this, &Console::onLineClicked);
		line.iconctrl->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &Console::onLineClicked);
		MyGUI::ISubWidget* waitDisplaySub = line.iconctrl->getSubWidgetMain();
		MyGUI::RotatingSkin *rotatingIcon = waitDisplaySub->castType<MyGUI::RotatingSkin>();
		rotatingIcon->setCenter(MyGUI::IntPoint(lineheight*0.5f,lineheight*0.5f));
		
		// funny but resource hungry
		//rotatingIcon->setAngle(Degree(90).valueRadians());
		//rotatingIcon->setAngle(Degree(Math::RangeRandom(0, 360)).valueRadians());
		lines[i] = line;
	}
	// hide the rest of the lines which are not visible anyways
	for (unsigned int i = linecount; i < LINES_MAX; i++)
	{
		if (!lines[i].txtctrl) break;
		lines[i].txtctrl->setVisible(false);
		lines[i].iconctrl->setVisible(false);
	}

	// resize the rest
	mCommandEdit->setCoord(0, linecount*lineheight, width, lineheight * 1.2f);
	scrollImgUp->setPosition(width * 0.3f, 2);
	scrollImgDown->setPosition(width * 0.3f, linecount * lineheight);

	// trigger lines update
	linesChanged = true;
}

void Console::updateGUILines( float dt )
{
	// update GUI
	// bottom line is last entry always
	int msgi = 0;
	int ctrli = linecount - 1;

	while(ctrli >= 0)
	{
		if (!lines[ctrli].txtctrl) break; // text control missing?!

		int msgid = message_counter - msgi - 1 - scrollOffset;
		if (msgid < 0)
		{
			// hide this entry, its empty
			lines[ctrli].txtctrl->setVisible(false);
			lines[ctrli].iconctrl->setVisible(false);
			ctrli--;
			msgi++;
			continue;
		}
		if (msgid >= (int)message_counter)
		{
			// no messages left
			break;
		}

		msg_t &m = messages[msgid];

		// check if TTL expired
		unsigned long t = Root::getSingleton().getTimer()->getMilliseconds() - m.time;
		if (t > m.ttl && !inputMode)
		{
			// expired, take the next message instead, when not in input mode
			msgi++;
			continue;
		} else
		{
			// look if its enforced to be visible
			if (m.forcevisible && !getVisible())
				setVisible(true);
		}

		// not empty, not expired, add content
		lines[ctrli].txtctrl->setVisible(true);
		lines[ctrli].iconctrl->setVisible(true);

		// set the controls data
		lines[ctrli].txtctrl->setUserData(msgid);
		lines[ctrli].iconctrl->setUserData(msgid);

		lines[ctrli].txtctrl->setCaption(convertToMyGUIString(m.txt));
		lines[ctrli].iconctrl->setProperty("ImageTexture", std::string(m.icon));
		lines[ctrli].expired = false;

		lines[ctrli].msg = &messages[msgid];
		
		ctrli--;
		msgi++;
	}
	
	linesChanged = false;
}

void Console::updateGUIVisual( float dt )
{
	for (unsigned int i = 0; i < linecount; i++)
	{
		if (lines[i].expired) continue;
		if (lines[i].iconctrl && lines[i].msg)
		{
			
			// rotating icons - funny but resource hungry
			//MyGUI::RotatingSkin* rotatingIcon = lines[i].iconctrl->getSubWidgetMain()->castType<MyGUI::RotatingSkin>();
			//rotatingIcon->setAngle(rotatingIcon->getAngle() + Math::PI / 360.0f);

			unsigned long ot = Root::getSingleton().getTimer()->getMilliseconds();
			unsigned long t = ot - lines[i].msg->time;
			if (t > lines[i].msg->ttl && !inputMode)
			{
				// expired
				lines[i].txtctrl->setVisible(false);
				lines[i].iconctrl->setVisible(false);
				lines[i].expired = true;
				lines[i].txtctrl->setCaption(lines[i].txtctrl->getCaption() + " EXPIRED");
				linesChanged = true;
				continue;
			}

			float alpha = 1.0f;
			if (!inputMode)
			{
				// logic
				unsigned long endTime   = lines[i].msg->time + lines[i].msg->ttl;
				const float fadeTime    = 2000.0f;
				unsigned long startTime = endTime - (long)fadeTime;

				if (ot < startTime)
				{
					alpha = 1.0f;
				} else
				{
					alpha = 1 - ((ot - startTime) / fadeTime);
				}
			} else
			{
				// different logic in input mode: display all messages
				alpha = 0.9f;
			}

			// set the alpha
			lines[i].txtctrl->setAlpha (alpha);
			lines[i].iconctrl->setAlpha(alpha);
		}
	}

	// show/hide the scroll icons
	scrollImgDown->setVisible(scrollOffset > 0 && inputMode);
	scrollImgUp->setVisible(scrollOffset < (int)(message_counter - linecount) && (message_counter - linecount) > 0  && inputMode);
}


int Console::messageUpdate( float dt )
{
	// collect the waiting messages and handle them
	std::vector<msg_t> tmpWaitingMessages;
	int results = pull(tmpWaitingMessages);

	// nothing to add?
	int r = 0;
	if (results > 0)
	{
		for (int i = 0; i < results; i++, r++)
		{
			if (tmpWaitingMessages[i].type == CONSOLE_MSGTYPE_HIGHSCORE)
			{
				// special :)
				HighScoreWindow::getSingleton().show(tmpWaitingMessages[i].txt);
				continue;
			}

			// copy over to our storage
			messages[message_counter] = tmpWaitingMessages[i];
		
			// increase pointer and overwrite oldest if overflown
			message_counter++;
			if (message_counter >= MESSAGES_MAX)
				message_counter = 0;
		}
		// new lines, update them
		updateGUILines(dt);
	}
	else if (results == 0 && linesChanged)
	{
		// in inputmode we display the last lines, so change them
		updateGUILines(dt);
	}

	return r;
}

void Console::putMessage( int type, int sender_uid, UTFString txt, String icon, unsigned long ttl, bool forcevisible )
{
	msg_t t;

	t.type       = type;
	t.sender_uid = sender_uid;
	t.time       = Root::getSingleton().getTimer()->getMilliseconds();
	t.ttl        = ttl;
	t.forcevisible = forcevisible;
	//strncpy(t.txt,  txt.c_str(), 2048);
	t.txt        = txt;
	strncpy(t.icon, icon.c_str(), 50);
	//t.channel = "default";

	push(t);
}

void Console::saveChat(String filename)
{
	// use C++ for easier wstring usage ... :-/
	std::ofstream f(filename.c_str(), std::ios_base::app|std::ios_base::out); // in append mode
	if (!f.is_open())
	{
		putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_NOTICE, ChatSystem::commandColour + _L("Unable to open file ") + filename, "error.png");
		return;
	}

	// now save the chat
	f << " ==== " << std::endl;
	for (unsigned int i = 0; i < message_counter; i++)
	{
		f << messages[i].time << " | " << messages[i].txt << std::endl;
	}
	f.close();
	
	putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, ChatSystem::commandColour + _L("History saved as ") + filename, "table_save.png");
}

void Console::outputCurrentPosition()
{
	Beam *b = BeamFactory::getSingleton().getCurrentTruck();
	if (!b && gEnv->player)
	{
		Vector3 pos = gEnv->player->getPosition();
		putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Character position: ") + String("#dd0000") + TOSTRING(pos.x) +  String("#000000, #00dd00") + TOSTRING(pos.y) + String("#000000, #0000dd") + TOSTRING(pos.z), "world.png");
	}
	else if (b)
	{
		Vector3 pos = b->getPosition();
		putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Vehicle position: ") + String("#dd0000") + TOSTRING(pos.x) +  String("#000000, #00dd00") + TOSTRING(pos.y) + String("#000000, #0000dd") + TOSTRING(pos.z), "world.png");
	}
}

void Console::outputCurrentTerrainHeight()
{
	if (!gEnv->terrainManager->getHeightFinder()) return;
	Vector3 pos  = Vector3::ZERO;

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
	putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Terrain height at position: ") + String("#dd0000") + TOSTRING(pos.x) +  String("#000000, #0000dd") + TOSTRING(pos.z) + String("#000000 = #00dd00") + TOSTRING(h), "world.png");
}

void Console::jumpToPosition( Vector3 pos )
{
	Beam *b = BeamFactory::getSingleton().getCurrentTruck();
	if (!b && gEnv->player)
	{
		gEnv->player->setPosition(pos);
		putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Character position set to: ") + String("#dd0000") + TOSTRING(pos.x) +  String("#000000, #00dd00") + TOSTRING(pos.y) + String("#000000, #0000dd") + TOSTRING(pos.z), "world.png");
	}
	else if (b)
	{
		b->resetPosition(pos, false);
		putMessage(CONSOLE_MSGTYPE_INFO, CONSOLE_SYSTEM_REPLY, _L("Vehicle position set to: ") + String("#dd0000") + TOSTRING(pos.x) +  String("#000000, #00dd00") + TOSTRING(pos.y) + String("#000000, #0000dd") + TOSTRING(pos.z), "world.png");
	}
}

void Console::onPopUpBtn( MyGUI::MenuCtrlPtr _sender, MyGUI::MenuItemPtr _item )
{
	//popUpContext
}

void Console::onLineClicked( MyGUI::Widget* _sender )
{
	int msg_id = -1;
	try
	{
		msg_id = *(_sender->getUserData<int>());
	}
	catch (...)
	{
		return;
	}
	if (msg_id < 0) return;

	popUpContext = messages[msg_id];
	popUpContextNumber = msg_id;

	// show popup control here
	popMenu->removeAllItems();
	popMenu->addItem(_L("Filter messages like this"), MyGUI::MenuItemType::Normal, "filter_this");
	popMenu->addItem(_L("Remove all Filters"), MyGUI::MenuItemType::Normal, "filter_clear");

	MyGUI::IntPoint point = MyGUI::InputManager::getInstance().getLastPressedPosition(MyGUI::MouseButton::Left);
	popMenu->setPosition(point);
	popMenu->setVisible(true);
}

#endif //MYGUI

