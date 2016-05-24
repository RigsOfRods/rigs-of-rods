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
#ifdef USE_SOCKETW

#pragma once
#ifndef __Network_H_
#define __Network_H_

#include "RoRPrerequisites.h"

#include "rornet.h"
#include "SocketW.h"

#include <atomic>
#include <condition_variable>
#include <mutex>

class Network : public ZeroedMemoryAllocator
{
public:

	Network(Ogre::String servername, long server_port);
	~Network();

	// messaging functions
	int sendMessageRaw(char *content, unsigned int msgsize);
	int sendmessage(int type, unsigned int streamid, unsigned int len, char* content);
	int sendScriptMessage(char* content, unsigned int len);
	int receivemessage(header_t *header, char* content, unsigned int bufferlen);

	// methods
	bool connect();
	void disconnect();
	void netFatalError(Ogre::UTFString error, bool exit=true);

	void sendthreadstart();
	void receivethreadstart();

	Ogre::UTFString getNickname(bool colour=false);
	char *getTerrainName() { return server_settings.terrain; };
	client_t *getClientInfo(unsigned int uid);
	int getClientInfos(client_t c[MAX_PEERS]);
	int getNetQuality();
	int getSpeedDown();
	int getSpeedUp();
	static unsigned long getNetTime();
	unsigned int getUID() { return myuid; };
	user_info_t *getLocalUserData() { return &userdata; };

	static void debugPacket(const char *name, header_t *header, char *buffer);

private:

	Ogre::UTFString m_server_name;
	Ogre::UTFString nickname;
	SWInetSocket socket;
	bool initiated;
	bool shutdown;
	char sendthreadstart_buffer[MAX_MESSAGE_LENGTH];
	char* send_buffer;
	client_t clients[MAX_PEERS];
	int last_time;
	int myauthlevel;
	int rconauthed;
	int send_buffer_len;
	int speed_bytes_sent, speed_bytes_sent_tmp, speed_bytes_recv, speed_bytes_recv_tmp;
	int speed_time;
	long m_server_port;
	std::condition_variable send_work_cv;
	std::mutex clients_mutex;
	std::mutex dl_data_mutex;
	std::mutex msgsend_mutex;
	std::mutex send_work_mutex;
	server_info_t server_settings;
	static Ogre::Timer timer;
	unsigned int myuid;
	std::map<int, float> lagDataClients;
	user_info_t userdata;

	void calcSpeed();
	void updatePlayerList();

	Ogre::UTFString getUserChatName(client_t *c);

	std::atomic<int> net_quality;

	void setNetQuality(int q);
};

#endif // __Network_H_

#endif // USE_SOCKETW
