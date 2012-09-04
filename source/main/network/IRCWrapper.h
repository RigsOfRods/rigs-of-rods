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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 12th of May 2011

#ifdef USE_SOCKETW

#ifndef __IRCWrapper_H_
#define __IRCWrapper_H_

#include "RoRPrerequisites.h"

#include "InterThreadStoreVector.h"

#include <libircclient.h>
#include <pthread.h>

enum message_types {
	MT_Channel,             // someone said something in a channel
	MT_NickChangeOther,     // someone changed his nick
	MT_NickChangeSelf,      // we changed our nick
	MT_QuitOther,           // someone quit
	MT_JoinChannelOther,    // someone joined a channel
	MT_JoinChannelSelf,     // we joined a channel
	MT_PartChannelOther,    // someone left a channel
	MT_PartChannelSelf,     // we left a channel
	MT_ChangedChannelMode , // someone changed the channelmode
	MT_ChangedUserModeSelf, // our usermode got changed
	MT_ChangedChannelTopic, // someone changed the channel topic
	MT_WeGotKicked,         // we got kicked from a channel
	MT_SomeoneGotKicked,    // someone else got kicked from a channel
	MT_GotPrivateMessage,   // we got a private message
	MT_GotNotice,           // we got a notice
	MT_GotInvitation,       // we got an invitation to a channel
	MT_VerboseMessage,      // a verbose message used for various things coming directly from the server
	MT_TopicInfo,           // got info about a channel topic
	MT_NameList,            // got the name list of a channel
	MT_ErrorAuth,           // if the authentication failed horribly
	MT_StatusUpdate         // to update the status message on the bottom right of the screen
};

typedef struct message_t
{
	int type;
	Ogre::String channel;
	Ogre::String nick;
	Ogre::String message;
	Ogre::String arg;
} message_t;

class IRCWrapper : public InterThreadStoreVector<message_t>
{
public:
	IRCWrapper();
	~IRCWrapper();
	
	void initIRC();

	void process(); // update function
	virtual void processIRCEvent(message_t &msg) = 0; // single event

	int sendMessage(Ogre::String msg, Ogre::String channelOrNick = "");
	int sendMeMessage(Ogre::String msg, Ogre::String channelOrNick = "");
	int changeNick(Ogre::String newNick);
	int joinChannel(Ogre::String channel, Ogre::String channelKey);
	int leaveChannel(Ogre::String channel);
	int quit(Ogre::String reason);

	Ogre::String getLastErrorMessage();

	// TODO:
	// inviteChannel
	// NAMES
	// LIST
	// TOPIC
	// channel mode
	// kick
	// send notice
	// WHOIS

	int authenticate();

	Ogre::String serverName, serverPassword, nick, userName, realName, channel, channelKey;
	unsigned int serverPort, retryCounter;
	bool reJoin, reConnect, wasConnected;

	irc_session_t *irc_session;
protected:
	pthread_t ircthread;

	int processAuthenticationResults(Ogre::String &results);
};

#endif // __IRCWrapper_H_

#endif // USE_SOCKETW
