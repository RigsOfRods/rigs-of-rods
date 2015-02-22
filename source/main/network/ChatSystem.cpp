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
#include "ChatSystem.h"

#include "Application.h"
#include "Collisions.h"
#include "Console.h"
#include "Language.h"
#include "Network.h"
#include "PlayerColours.h"
#include "Utils.h"
#include "GUIManager.h"

#ifdef USE_MYGUI
#include "GUIMp.h"
#endif  // USE_MYGUI

using namespace Ogre;
using namespace RoR;

///////////////////////////////////
// ChatSystemFactory

template<> ChatSystemFactory *StreamableFactory < ChatSystemFactory, ChatSystem >::_instance = 0;

ChatSystemFactory::ChatSystemFactory()
{
}

ChatSystemFactory::~ChatSystemFactory()
{
}

ChatSystem *ChatSystemFactory::createLocal(int playerColour)
{
	lockStreams();
	std::map < int, std::map < unsigned int, ChatSystem *> > &streamables = getStreams();
	ChatSystem *ch = new ChatSystem(-1, 0, playerColour, false);
	streamables[-1][0] = ch;
	unlockStreams();
	return ch;
}

ChatSystem *ChatSystemFactory::createRemoteInstance(stream_reg_t *reg)
{
	// NO LOCKS IN HERE, already locked

	LOG(" new chat system for " + TOSTRING(reg->sourceid) + ":" + TOSTRING(reg->streamid) + ", colour: " + TOSTRING(reg->colour));
	ChatSystem *ch = new ChatSystem(reg->sourceid, reg->streamid, reg->colour, true);

	// already locked
	//lockStreams();
	std::map < int, std::map < unsigned int, ChatSystem *> > &streamables = getStreams();
	streamables[reg->sourceid][reg->streamid] = ch;
	//unlockStreams();
	return ch;
}

bool ChatSystemFactory::syncRemoteStreams()
{
	// we override this here, so we know if something changed and could update the player list
	bool changes = StreamableFactory <ChatSystemFactory, ChatSystem>::syncRemoteStreams();

#ifdef USE_MYGUI
#ifdef USE_SOCKETW
	if (changes)
		GUI_Multiplayer::getSingleton().update();
#endif // USE_SOCKETW
#endif // USE_MYGUI	
	return changes;
}

ChatSystem *ChatSystemFactory::getFirstChatSystem()
{
	lockStreams();
	std::map < int, std::map < unsigned int, ChatSystem *> > &streamables = getStreams();
	if (streamables.empty() || streamables.begin()->second.empty()) return 0;
	ChatSystem *r = streamables.begin()->second.begin()->second;
	unlockStreams();
	return r;
}

///////////////////////////////////
// ChatSystem

const UTFString ChatSystem::commandColour       = U("#00FF00");
const UTFString ChatSystem::normalColour        = U("#FFFFFF");
const UTFString ChatSystem::whisperColour       = U("#FFCC00");
const UTFString ChatSystem::scriptCommandColour = U("#0099FF");




ChatSystem::ChatSystem(int source, unsigned int streamid, int colourNumber, bool remote) :
	source(source),
	streamid(streamid),
	colourNumber(colourNumber),
	remote(remote),
	username("unknown"),
	mNickColour("")
{
	sendStreamSetup();
#ifdef USE_SOCKETW
	if (remote)
	{
		client_t *c = gEnv->network->getClientInfo(source);
		if (c)
		{
			username = getColouredName(*c);
		}

#ifdef USE_MYGUI
		String msg = username + commandColour + _L(" joined the game");
		//RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_NETWORK, Console::CONSOLE_LOGMESSAGE, msg, "user_add.png");
		RoR::Application::GetGuiManager()->PushNotification("Server info:", msg);
#endif //USE_MYGUI
	}
#endif //SOCKETW
}

ChatSystem::~ChatSystem()
{
#ifdef USE_MYGUI
	if (remote)
	{
		String msg = username + commandColour + _L(" left the game");
		//RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_NETWORK, Console::CONSOLE_LOGMESSAGE, msg, "user_delete.png");
		RoR::Application::GetGuiManager()->PushNotification("Server info:", msg);
	}
#endif //USE_MYGUI
}

void ChatSystem::sendStreamSetup()
{
	if (remote) return;

	stream_register_t reg;
	reg.status = 1;
	strcpy(reg.name, "chat");
	reg.type = 3;
	NetworkStreamManager::getSingleton().addLocalStream(this, &reg);
}

void ChatSystem::sendStreamData()
{
    // we send data synchronously to prevent lag
}

void ChatSystem::receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len)
{
#ifdef USE_MYGUI
	if (type == MSG2_UTF_CHAT)
	{
		// some chat code
		if (source == -1)
		{
			// server said something
			UTFString msg = tryConvertUTF(buffer);
			RoR::Application::GetGuiManager()->pushMessageChatBox(msg);
		} else if (source == (int)this->source && (int)streamid == this->streamid)
		{
			UTFString msg = username + normalColour + ": " + tryConvertUTF(buffer);
			RoR::Application::GetGuiManager()->pushMessageChatBox(msg);
		}
#ifdef USE_SOCKETW
		else if (source == (int)gEnv->network->getUID())
		{
			// our message bounced back :D
			UTFString msg = gEnv->network->getNickname(true) + normalColour + ": " + tryConvertUTF(buffer);
			RoR::Application::GetGuiManager()->pushMessageChatBox(msg);
		}
#endif // USE_SOCKETW
	} else if (type == MSG2_UTF_PRIVCHAT)
	{
		// some private chat message
		if (source == -1)
		{
			// server said something
			String msg = whisperColour + _L(" [whispered] ") + normalColour +  tryConvertUTF(buffer);
			RoR::Application::GetGuiManager()->pushMessageChatBox(msg);
		} else if (source == (int)this->source && (int)streamid == this->streamid)
		{
			UTFString msg = username + _L(" [whispered] ") + normalColour + ": " + tryConvertUTF(buffer);
			RoR::Application::GetGuiManager()->pushMessageChatBox(msg);
		}
	}
#endif //USE_MYGUI
}

void ChatSystem::sendChat(UTFString chatline)
{
	const char *utf8_line = chatline.asUTF8_c_str();
	this->addPacket(MSG2_UTF_CHAT, (unsigned int)strlen(utf8_line), (char *)utf8_line);
}

int ChatSystem::getChatUserNames(std::vector<UTFString> &names)
{
#ifdef USE_SOCKETW
	client_t c[MAX_PEERS];
	if (gEnv->network->getClientInfos(c)) return 0;

	for (int i = 0; i < MAX_PEERS; i++)
	{
		names.push_back(c[i].user.username);
	}
	return (int)names.size();
#else
	return 0;
#endif // USE_SOCKETW
}

void ChatSystem::sendPrivateChat(UTFString targetUsername, UTFString chatline)
{
#ifdef USE_SOCKETW
	// first: find id to username:
	client_t c[MAX_PEERS];
	if (gEnv->network->getClientInfos(c))
		return;
	int target_uid = -1, target_index = -1;
	for (int i = 0; i < MAX_PEERS; i++)
	{
		if (UTFString(c[i].user.username) == targetUsername)
		{
			// found it :)
			target_uid = c[i].user.uniqueid;
			target_index = i;
			break;
		}
	}

	if (target_uid < 0)
	{
#ifdef USE_MYGUI
		RoR::Application::GetGuiManager()->pushMessageChatBox(ChatSystem::commandColour + _L("user not found: ") + targetUsername);
		//RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ChatSystem::commandColour + _L("user not found: ") + targetUsername, "error.png");
#endif // USE_MYGUI
		return;
	}

	sendPrivateChat(target_uid, chatline, getColouredName(c[target_index]));
#endif // USE_SOCKETW
}



void ChatSystem::sendPrivateChat(int target_uid, UTFString chatline, UTFString username)
{
#ifdef USE_SOCKETW
	char buffer[MAX_MESSAGE_LENGTH] = "";
	
	const char *chat_msg = (const char *)chatline.asUTF8_c_str();

	// format: int of UID, then chat message
	memcpy(buffer, &target_uid, sizeof(int));
	strncpy(buffer + sizeof(int), chat_msg, MAX_MESSAGE_LENGTH - sizeof(int));

	size_t len = sizeof(int) + chatline.size() * sizeof(wchar_t);
	buffer[len] = 0;

	this->addPacket(MSG2_UTF_PRIVCHAT, (unsigned int)len, buffer);

	if (username.empty())
	{
		client_t *c = gEnv->network->getClientInfo(target_uid);
		if (c) username = getColouredName(*c);
	}

	// add local visual
#ifdef USE_MYGUI
	UTFString nmsg = gEnv->network->getNickname(true) + normalColour + whisperColour + _L(" [whispered to ") + normalColour + username + whisperColour + "]" + normalColour + ": " + chatline;
	//RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_NETWORK, Console::CONSOLE_LOGMESSAGE, nmsg, "script_key.png");
	RoR::Application::GetGuiManager()->pushMessageChatBox(nmsg);
#endif // USE_MYGUI
#endif // USE_SOCKETW
}

UTFString ChatSystem::getColouredName(client_t &c)
{
	return getColouredName(c.user);
}

UTFString ChatSystem::getColouredName(user_info_t &u)
{
	return ChatSystem::getColouredName(UTFString(u.username), u.authstatus, u.colournum);
}

UTFString ChatSystem::getColouredName(UTFString nick, int auth, int colourNumber)
{
	ColourValue col_val = PlayerColours::getSingleton().getColour(colourNumber);
	char tmp[255] = "";
	sprintf(tmp, "#%02X%02X%02X", (unsigned int)(col_val.r * 255.0f), (unsigned int)(col_val.g * 255.0f), (unsigned int)(col_val.b * 255.0f));

	// replace # with X in nickname so the user cannot fake the colour
	for (unsigned int i=0; i<nick.size(); i++)
		if (nick[i] == '#') nick[i] = 'X';

	return tryConvertUTF(tmp) + nick;

#if 0
	// old code: colour not depending on auth status anymore ...

	if (auth == AUTH_NONE)  col = "#c9c9c9"; // grey
	if (auth & AUTH_BOT )   col = "#0000c9"; // blue
	if (auth & AUTH_RANKED) col = "#00c900"; // green
	if (auth & AUTH_MOD)    col = "#c90000"; // red
	if (auth & AUTH_ADMIN)  col = "#c97100"; // orange

	return col + nick;
#endif //0
}
