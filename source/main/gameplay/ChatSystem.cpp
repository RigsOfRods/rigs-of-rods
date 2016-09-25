/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2016 Petr Ohlidal

	For more information, see http://www.rigsofrods.org/

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
#include "GUIManager.h"
#include "Language.h"
#include "PlayerColours.h"
#include "Utils.h"

namespace RoR {
namespace ChatSystem {

static int m_stream_id;

#ifdef USE_SOCKETW
void SendStreamSetup()
{
	stream_register_t reg;
	memset(&reg, 0, sizeof(stream_register_t));
	reg.status = 1;
	strcpy(reg.name, "chat");
	reg.type = 3;

	RoR::Networking::AddLocalStream(&reg, sizeof(stream_register_t));

	m_stream_id = reg.origin_streamid;

}
#endif // USE_SOCKETW

Ogre::UTFString GetColouredName(Ogre::UTFString nick, int colour_number)
{
	Ogre::ColourValue col_val = PlayerColours::getSingleton().getColour(colour_number);
	char tmp[255] = {0};
	sprintf(tmp, "#%02X%02X%02X", (unsigned int)(col_val.r * 255.0f), (unsigned int)(col_val.g * 255.0f), (unsigned int)(col_val.b * 255.0f));

	// replace # with X in nickname so the user cannot fake the colour
	for (unsigned int i=0; i<nick.size(); i++)
		if (nick[i] == '#') nick[i] = 'X';

	return tryConvertUTF(tmp) + nick;
}

#ifdef USE_SOCKETW
void ReceiveStreamData(unsigned int type, int source, char *buffer)
{
	if (type != MSG2_UTF_CHAT && type != MSG2_UTF_PRIVCHAT) return;

	Ogre::UTFString msg = tryConvertUTF(buffer);

	if (type == MSG2_UTF_CHAT)
	{
		if (source == -1)
		{
			// server said something
		} else if (source == RoR::Networking::GetUID())
		{
			// our message bounced back :D
			Ogre::UTFString local_username = GetColouredName(RoR::Networking::GetUsername(), RoR::Networking::GetUserColor());
			msg = local_username + RoR::Color::NormalColour + ": " + msg;
		} else
		{
			user_info_t user;
			if (RoR::Networking::GetUserInfo(source, user))
			{
				msg = GetColouredName(user.username, user.colournum) + RoR::Color::NormalColour + ": " + msg;
			}
		}
	} else if (type == MSG2_UTF_PRIVCHAT)
	{
		if (source == -1)
		{
			// server said something
			msg = RoR::Color::WhisperColour + _L(" [whispered] ") + RoR::Color::NormalColour + msg;
		} else
		{
			user_info_t user;
			if (RoR::Networking::GetUserInfo(source, user))
			{
				msg = GetColouredName(user.username, user.colournum) + _L(" [whispered] ") + RoR::Color::NormalColour + ": " + msg;
			}
		}
	}
#ifdef USE_MYGUI
	RoR::Application::GetGuiManager()->pushMessageChatBox(msg);
#endif //USE_MYGUI
}
#endif // USE_SOCKETW

#ifdef USE_SOCKETW
void HandleStreamData(std::vector<RoR::Networking::recv_packet_t> packet_buffer)
{
	for (auto packet : packet_buffer)
	{
		ReceiveStreamData(packet.header.command, packet.header.source, packet.buffer);
	}
}
#endif // USE_SOCKETW

void SendChat(Ogre::UTFString chatline)
{
#ifdef USE_SOCKETW
	const char *utf8_line = chatline.asUTF8_c_str();
	RoR::Networking::AddPacket(m_stream_id, MSG2_UTF_CHAT, (unsigned int)strlen(utf8_line), (char *)utf8_line);
#endif // USE_SOCKETW
}

void SendPrivateChat(int target_uid, Ogre::UTFString chatline, Ogre::UTFString target_username)
{
#ifdef USE_SOCKETW
	char buffer[MAX_MESSAGE_LENGTH] = {0};
	
	const char *chat_msg = (const char *)chatline.asUTF8_c_str();

	// format: int of UID, then chat message
	memcpy(buffer, &target_uid, sizeof(int));
	strncpy(buffer + sizeof(int), chat_msg, MAX_MESSAGE_LENGTH - sizeof(int));

	size_t len = sizeof(int) + chatline.size() * sizeof(wchar_t);
	buffer[len] = 0;

	RoR::Networking::AddPacket(m_stream_id, MSG2_UTF_PRIVCHAT, (unsigned int)len, buffer);

	if (target_username.empty())
	{
		user_info_t user;
		if (RoR::Networking::GetUserInfo(target_uid, user))
		{
			target_username = GetColouredName(user.username, user.colournum);
		}
	}

	// add local visual
	Ogre::UTFString local_username = GetColouredName(RoR::Networking::GetUsername(), RoR::Networking::GetUserColor());
	Ogre::UTFString nmsg = local_username + RoR::Color::WhisperColour + _L(" [whispered to ") + RoR::Color::NormalColour + target_username + RoR::Color::WhisperColour + "]" + RoR::Color::NormalColour + ": " + chatline;
#ifdef USE_MYGUI
	RoR::Application::GetGuiManager()->pushMessageChatBox(nmsg);
#endif // USE_MYGUI
#endif // USE_SOCKETW
}

void SendPrivateChat(Ogre::UTFString target_username, Ogre::UTFString chatline)
{
#ifdef USE_SOCKETW
	// first: find id to username:
	user_info_t target_user;
	std::vector<user_info_t> users = RoR::Networking::GetUserInfos();

	bool found_target = false;
	for (auto user : users)
	{
		if (Ogre::UTFString(user.username) == target_username)
		{
			found_target = true;
			target_user = user;
			break;
		}
	}

	if (!found_target)
	{
#ifdef USE_MYGUI
		RoR::Application::GetGuiManager()->pushMessageChatBox(RoR::Color::CommandColour + _L("user not found: ") + target_username);
#endif // USE_MYGUI
		return;
	}

	SendPrivateChat(target_user.uniqueid, chatline, GetColouredName(target_user.username, target_user.colournum));
#endif // USE_SOCKETW
}

} // namespace ChatSystem
} // namespace RoR
