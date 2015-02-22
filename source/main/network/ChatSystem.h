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
#ifndef __ChatSystem_H_
#define __ChatSystem_H_

#include "RoRPrerequisites.h"

#include "Streamable.h"
#include "StreamableFactory.h"

class ChatSystem : public Streamable, public ZeroedMemoryAllocator
{
	friend class ChatSystemFactory;
	friend class Network;

public:

	ChatSystem(int source=-1, unsigned int streamid=0, int colourNumber=0, bool remote=true);
	~ChatSystem();

	void sendChat(Ogre::UTFString chatline);
	void sendPrivateChat(Ogre::UTFString targetUsername, Ogre::UTFString chatline);
	void sendPrivateChat(int targetUID, Ogre::UTFString chatline, Ogre::UTFString username = "");

	static Ogre::UTFString getColouredName(Ogre::UTFString nick, int auth, int colourNumber);
	static Ogre::UTFString getColouredName(client_t &c);
	static Ogre::UTFString getColouredName(user_info_t &u);

	int getChatUserNames(std::vector<Ogre::UTFString> &names);

	static const Ogre::UTFString commandColour, normalColour, whisperColour, scriptCommandColour;

protected:
	int source;
	int streamid;
	int colourNumber;
	bool remote;
	Ogre::UTFString username;
	Ogre::String mNickColour;
	void sendStreamSetup();
	void sendStreamData();
	void receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len);
};

class ChatSystemFactory : public StreamableFactory < ChatSystemFactory, ChatSystem >, public ZeroedMemoryAllocator
{
	friend class Network;

public:

	ChatSystemFactory();
	~ChatSystemFactory();

	ChatSystem *createLocal(int playerColour);
	ChatSystem *createRemoteInstance(stream_reg_t *reg);

	ChatSystem *getFirstChatSystem();

protected:
	// functions used by friends
	void netUserAttributesChanged(int source, int streamid) {};
	void localUserAttributesChanged(int newid) {};

	bool syncRemoteStreams();
};

#endif // __ChatSystem_H_
