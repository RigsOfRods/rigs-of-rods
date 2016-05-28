/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2016 Petr Ohlidal

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

#include "Network.h"

#include "BeamFactory.h"
#include "CharacterFactory.h"
#include "ChatSystem.h"
#include "ErrorUtils.h"
#include "Language.h"
#include "RoRVersion.h"
#include "SHA1.h"
#include "ScriptEngine.h"
#include "Settings.h"
#include "Utils.h"

#include <Ogre.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <deque>
#include <thread>

namespace RoR {
namespace Networking {

typedef struct send_packet_t
{
	char buffer[MAX_MESSAGE_LENGTH];
	int size;
} send_packet_t;

static Ogre::UTFString m_server_name;
static int m_server_port;
static server_info_t m_server_settings;

static Ogre::UTFString m_username;
static int m_uid;
static int m_authlevel;
static user_info_t m_userdata;

static int m_stream_id = 10;

static std::atomic<int> m_net_quality;

static std::vector<user_info_t> m_users;

static SWInetSocket socket;

static std::thread m_send_thread;
static std::thread m_recv_thread;

static std::atomic<bool> m_shutdown;

static std::mutex m_users_mutex;
static std::mutex m_userdata_mutex;
static std::mutex m_recv_packetqueue_mutex;
static std::mutex m_send_packetqueue_mutex;

static std::condition_variable m_send_packet_available_cv;

static std::vector<recv_packet_t> m_recv_packet_buffer;
static std::deque <send_packet_t> m_send_packet_buffer;

static const unsigned int m_packet_buffer_size = 20;

void DebugPacket(const char *name, header_t *header, char *buffer)
{
	char sha1result[250] = {0};
	if (buffer)
	{
		RoR::CSHA1 sha1;
		sha1.UpdateHash((uint8_t *)buffer, header->size);
		sha1.Final();
		sha1.ReportHash(sha1result, RoR::CSHA1::REPORT_HEX_SHORT);
	}
	char tmp[256] = {0};
	sprintf(tmp, "++ %s: %d:%d, %d, %d, hash: %s", name, header->source, header->streamid, header->command, header->size, sha1result);
	LOG(tmp);
}

void NetFatalError(Ogre::UTFString errormsg, bool exit_program)
{
	if (m_shutdown)
		return;

	socket.set_timeout(1, 1000);
	ErrorUtils::ShowError(_L("Network Connection Problem"), _L("Network fatal error: ") + errormsg);

	socket.disconnect();

	if (exit_program)
		exit(124);
}

void SetNetQuality(int quality)
{
	m_net_quality = quality;
}

int GetNetQuality()
{
	return m_net_quality;
}

int GetUID()
{
	return m_uid;
}

int SendMessageRaw(char *buffer, int msgsize)
{
	SWBaseSocket::SWBaseError error;

	int rlen = 0;
	while (rlen < msgsize)
	{
		int sendnum = socket.send(buffer + rlen, msgsize - rlen, &error);
		if (sendnum < 0)
		{
			LOG("NET send error: " + TOSTRING(sendnum));
			return -1;
		}
		rlen += sendnum;
	}

	// TODO: Think about sleeping here to avoid network congestion
	// The sleep time could be based on how much time was spent in socket.send()

	return 0;
}

int SendMessage(int type, unsigned int streamid, int len, char* content)
{
	header_t head;
	memset(&head, 0, sizeof(header_t));
	head.command = type;
	head.source = m_uid;
	head.size = len;
	head.streamid = streamid;

	const int msgsize = sizeof(header_t) + len;

	if (msgsize >= MAX_MESSAGE_LENGTH)
	{
    	return -1;
	}

	char buffer[MAX_MESSAGE_LENGTH] = {0};
	memset(buffer, 0, MAX_MESSAGE_LENGTH);
	memcpy(buffer, (char *)&head, sizeof(header_t));
	memcpy(buffer + sizeof(header_t), content, len);

	return SendMessageRaw(buffer, msgsize);
}

void QueueStreamData(header_t &header, char *buffer)
{
	recv_packet_t packet;
	packet.header = header;
	memcpy(packet.buffer, buffer, MAX_MESSAGE_LENGTH);

	std::lock_guard<std::mutex> lock(m_recv_packetqueue_mutex);
	m_recv_packet_buffer.push_back(packet);
}

int ReceiveMessage(header_t *head, char* content, int bufferlen)
{
	SWBaseSocket::SWBaseError error;

	char buffer[MAX_MESSAGE_LENGTH] = {0};

	int hlen = 0;
	while (hlen < (int)sizeof(header_t))
	{
		int recvnum = socket.recv(buffer + hlen, sizeof(header_t) - hlen, &error);
		if (recvnum < 0 && !m_shutdown)
		{
			LOG("NET receive error 1: " + TOSTRING(recvnum));
			return -1;
		}
		hlen += recvnum;
	}

	memcpy(head, buffer, sizeof(header_t));

	if (head->size >= MAX_MESSAGE_LENGTH)
	{
    	return -3;
	}

	if (head->size > 0)
	{
		// Read the packet content
		while (hlen < (int)sizeof(header_t) + (int)head->size)
		{
			int recvnum = socket.recv(buffer + hlen, (head->size + sizeof(header_t)) - hlen, &error);
			if (recvnum < 0 && !m_shutdown)
			{
				LOG("NET receive error 2: "+ TOSTRING(recvnum));
				return -1;
			}
			hlen += recvnum;
		}
	}

	memcpy(content, buffer + sizeof(header_t), bufferlen);

	return 0;
}

void SendThread()
{
	LOG("SendThread started");
	while (!m_shutdown)
	{
		std::unique_lock<std::mutex> queue_lock(m_send_packetqueue_mutex);
		while (m_send_packet_buffer.empty())
		{
			if (m_shutdown) { return; }
			m_send_packet_available_cv.wait(queue_lock);
		}

		while (!m_send_packet_buffer.empty())
		{
			send_packet_t packet = m_send_packet_buffer.front();
			m_send_packet_buffer.pop_front();

			queue_lock.unlock();
			SendMessageRaw(packet.buffer, packet.size);
			queue_lock.lock();
		}
	}
	LOG("SendThread stopped");
}

void RecvThread()
{
	LOG("RecvThread started");

	header_t header;

	char *buffer = (char*)malloc(MAX_MESSAGE_LENGTH);

	while (!m_shutdown)
	{
		int err = ReceiveMessage(&header, buffer, MAX_MESSAGE_LENGTH);
		//LOG("Received data: " + TOSTRING(header.command) + ", source: " + TOSTRING(header.source) + ":" + TOSTRING(header.streamid) + ", size: " + TOSTRING(header.size));
		if (err)
		{
			char errmsg[256] = {0};
			sprintf(errmsg, "Error %i while receiving data", err);
			NetFatalError(errmsg, true);
			return;
		}

		if (header.command == MSG2_STREAM_REGISTER)
		{
			if (header.source == m_uid)
				continue;

			stream_register_t *reg = (stream_register_t *)buffer;

			LOG(" * received stream registration: " + TOSTRING(header.source) + ": " + TOSTRING(header.streamid) + ", type: " + TOSTRING(reg->type));
		} else if (header.command == MSG2_STREAM_REGISTER_RESULT)
		{
			stream_register_t *reg = (stream_register_t *)buffer;
			LOG(" * received stream registration result: " + TOSTRING(header.source) + ": " + TOSTRING(header.streamid) + ", status: " + TOSTRING(reg->status));
		} else if (header.command == MSG2_STREAM_UNREGISTER)
		{
			LOG(" * received stream deregistration: " + TOSTRING(header.source) + ": " + TOSTRING(header.streamid));
		} else if (header.command == MSG2_UTF_CHAT || header.command == MSG2_UTF_PRIVCHAT)
		{
			// Chat message
		} else if (header.command == MSG2_NETQUALITY && header.source == -1)
		{
			if (header.size != sizeof(int))
				continue;
			int quality = *(int *)buffer;
			SetNetQuality(quality);
			continue;
		} else if (header.command == MSG2_USER_LEAVE)
		{
			if (header.source == m_uid)
			{
				NetFatalError(_L("disconnected: remote side closed the connection"), false);
				return;
			}

			{
				std::lock_guard<std::mutex> lock(m_users_mutex);
				for (auto it = m_users.begin(); it != m_users.end(); it++)
				{
					if ((int)(*it).uniqueid == header.source)
					{
						Ogre::UTFString msg = RoR::ChatSystem::GetColouredName((*it).username, (*it).colournum) + RoR::Color::CommandColour + _L(" left the game");
						const char *utf8_line = msg.asUTF8_c_str();
						header_t head;
						head.command = MSG2_UTF_CHAT;
						head.source  = -1;
						head.size    = (int)strlen(utf8_line);
						QueueStreamData(head, (char *)utf8_line);
						LOG(Ogre::UTFString((*it).username) + _L(" left the game"));
						m_users.erase(it);
						break;
					}
				}
			}
		} else if (header.command == MSG2_USER_INFO || header.command == MSG2_USER_JOIN)
		{
			if (header.source == m_uid)
			{
				std::lock_guard<std::mutex> lock(m_userdata_mutex);
				memcpy(&m_userdata, buffer, sizeof(user_info_t));
				m_authlevel = m_userdata.authstatus;
				m_username = Ogre::UTFString(m_userdata.username);
				SETTINGS.setUTFSetting(L"Nickname", m_username);
			} else
			{
				user_info_t user_info;
				memcpy(&user_info, buffer, sizeof(user_info_t));

				bool user_exists = false;
				{
					std::lock_guard<std::mutex> lock(m_users_mutex);
					for (user_info_t &user : m_users)
					{
						if ((int)user.uniqueid == header.source)
						{
							user = user_info;
							user_exists = true;
							break;
						}
					}
					if (!user_exists)
					{
						m_users.push_back(user_info);
						Ogre::UTFString msg = RoR::ChatSystem::GetColouredName(user_info.username, user_info.colournum) + RoR::Color::CommandColour + _L(" joined the game");
						const char *utf8_line = msg.asUTF8_c_str();
						header_t head;
						head.command = MSG2_UTF_CHAT;
						head.source  = -1;
						head.size    = (int)strlen(utf8_line);
						QueueStreamData(head, (char *)utf8_line);
						LOG(Ogre::UTFString(user_info.username) + _L(" joined the game"));
					}
				}
			}
			continue;
		} else if (header.command == MSG2_GAME_CMD)
		{
#ifdef USE_ANGELSCRIPT
			ScriptEngine::getSingleton().queueStringForExecution(Ogre::String(buffer));
#endif // USE_ANGELSCRIPT
			continue;
		}
		//DebugPacket("receive-1", &header, buffer);

		QueueStreamData(header, buffer);
	}
	LOG("RecvThread stopped");
}

bool Connect()
{
	m_server_name = SSETTING("Server name", "");
	m_server_port = ISETTING("Server port", 0);
	m_username = SSETTING("Nickname", "Anonymous");

	LOG("Trying to join server '" + m_server_name + "' on port " + TOSTRING(m_server_port) + "'...");

	SWBaseSocket::SWBaseError error;

	socket.set_timeout(10, 10000);
	socket.connect(m_server_port, m_server_name, &error);
	if (error != SWBaseSocket::ok)
	{
		NetFatalError(_L("Establishing network session: "), false);
		return false;
	}
	if (SendMessage(MSG2_HELLO, 0, (int)strlen(RORNET_VERSION), (char *)RORNET_VERSION))
	{
		NetFatalError(_L("Establishing network session: error sending hello"), false);
		return false;
	}

	header_t header;
	char buffer[MAX_MESSAGE_LENGTH] = {0};

	// Receive server (rornet protocol) version
	if (ReceiveMessage(&header, buffer, 255))
	{
		NetFatalError(_L("Establishing network session: error getting server version"), false);
		return false;
	}
	if (header.command == MSG2_WRONG_VER)
	{
		NetFatalError(_L("server uses a different protocol version"), true);
		return false;
	}
	if (header.command != MSG2_HELLO)
	{
		NetFatalError(_L("Establishing network session: error getting server hello"), true);
		return false;
	}

	// Save server settings
	memcpy(&m_server_settings, buffer, sizeof(server_info_t));

	if (strncmp(m_server_settings.protocolversion, RORNET_VERSION, strlen(RORNET_VERSION)))
	{
		wchar_t tmp[512] = L"";
		Ogre::UTFString tmp2 = _L("Establishing network session: wrong server version, you are using version '%s' and the server is using '%s'");
		swprintf(tmp, 512, tmp2.asWStr_c_str(), RORNET_VERSION, m_server_settings.protocolversion);
		NetFatalError(Ogre::UTFString(tmp), true);
		return false;
	}

	// First handshake done, increase the timeout, important!
	socket.set_timeout(0, 0);

	// Send credentials
	char pwbuffer[250] = {0};
	strncpy(pwbuffer, SSETTING("Server password", "").c_str(), 250);

	char sha1pwresult[250] = {0};
	if (strnlen(pwbuffer, 250) > 0)
	{
		RoR::CSHA1 sha1;
		sha1.UpdateHash((uint8_t *)pwbuffer, (uint32_t)strnlen(pwbuffer, 250));
		sha1.Final();
		sha1.ReportHash(sha1pwresult, RoR::CSHA1::REPORT_HEX_SHORT);
	}

	Ogre::String usertokenhash = SSETTING("User Token Hash", "");

	// Construct user credentials
	// Beware of the wchar_t converted to UTF8 for networking
	user_info_t c;
	memset(&c, 0, sizeof(user_info_t));
	// Cut off the UTF string on the highest level, otherwise you will break UTF info
	strncpy((char *)c.username, m_username.substr(0, MAX_USERNAME_LEN * 0.5f).asUTF8_c_str(), MAX_USERNAME_LEN);
	strncpy(c.serverpassword, sha1pwresult, 40);
	strncpy(c.usertoken, usertokenhash.c_str(), 40);
	strncpy(c.clientversion, ROR_VERSION_STRING, strnlen(ROR_VERSION_STRING, 25));
	strcpy(c.clientname, "RoR");
	Ogre::String lang = SSETTING("Language Short", "en");
	strncpy(c.language, lang.c_str(), std::min<int>((int)lang.size(), 10));
	Ogre::String guid = SSETTING("GUID", "");
	strncpy(c.clientGUID, guid.c_str(), std::min<int>((int)guid.size(), 10));
	strcpy(c.sessiontype, "normal");
	if (SendMessage(MSG2_USER_INFO, 0, sizeof(user_info_t), (char*)&c))
	{
		NetFatalError(_L("Establishing network session: error sending user info"), false);
		return false;
	}

	// Getting authorization
	if (ReceiveMessage(&header, buffer, 255))
	{
		NetFatalError(_L("Establishing network session: error getting server authorization"), false);
		return false;
	}
	if (header.command==MSG2_FULL)
	{
		NetFatalError(_L("Establishing network session: sorry, server has too many players"), false);
		return false;
	} else if (header.command==MSG2_BANNED)
	{
		wchar_t tmp[512];
		memset(tmp, 0, 512);
		if (strnlen(buffer, 20) > 0)
		{
			buffer[header.size] = {0};
			Ogre::UTFString tmp2 = _L("Establishing network session: sorry, you are banned:\n%s");
			swprintf(tmp, 512, tmp2.asWStr_c_str(), buffer);
			NetFatalError(Ogre::UTFString(tmp), true);
		} else
		{
			NetFatalError(_L("Establishing network session: sorry, you are banned!"), false);
		}
		return false;
	} else if (header.command==MSG2_WRONG_PW)
	{
		NetFatalError(_L("Establishing network session: sorry, wrong password!"), false);
		return false;
	} else if (header.command==MSG2_WRONG_VER)
	{
		NetFatalError(_L("Establishing network session: sorry, wrong protocol version!"), false);
		return false;
	}
	if (header.command!=MSG2_WELCOME)
	{
		NetFatalError(_L("Establishing network session: sorry, unknown server response"), false);
		return false;
	}

	m_uid = header.source;

	// we get our userdata back
	memcpy(&m_userdata, buffer, std::min<int>(sizeof(user_info_t), header.size));

	m_shutdown = false;

	m_send_thread = std::thread(SendThread);
	m_recv_thread = std::thread(RecvThread);

	return true;
}

void Disconnect()
{
	m_shutdown = true;
	m_send_packet_available_cv.notify_one();
	m_send_thread.join();
	m_recv_thread.detach();

	SendMessage(MSG2_USER_LEAVE, 0, 0, 0);
	socket.set_timeout(1, 1000);
	socket.disconnect();
}

void AddPacket(int streamid, int type, int len, char *content)
{
	if (len > MAX_MESSAGE_LENGTH)
		// packet too big, discarded
		return;

	if (type == MSG2_STREAM_DATA && m_send_packet_buffer.size() > m_packet_buffer_size)
		// buffer full, discard unimportant data packets
		return;

	send_packet_t packet;
	memset(&packet, 0, sizeof(send_packet_t));

	char *buffer = (char*)(packet.buffer);

	header_t *head = (header_t *)buffer;
	head->command  = type;
	head->source   = m_uid;
	head->size     = len;
	head->streamid = streamid;

	// then copy the contents
	char *bufferContent = (char *)(buffer + sizeof(header_t));
	memcpy(bufferContent, content, len);

	// record the packet size
	packet.size = len + sizeof(header_t);

	{
		std::lock_guard<std::mutex> lock(m_send_packetqueue_mutex);
		if (type == MSG2_STREAM_DATA)
		{
			auto search = std::find_if(m_send_packet_buffer.begin(), m_send_packet_buffer.end(), [packet](const send_packet_t& p) { return memcmp(packet.buffer, p.buffer, sizeof(header_t)) == 0; });
			if (search != m_send_packet_buffer.end())
			{
				// Found an older packet with the same header -> replace it
				(*search) = packet;
				return;
			}
		}
		m_send_packet_buffer.push_back(packet);
	}
	
	m_send_packet_available_cv.notify_one();
}

void AddLocalStream(stream_register_t *reg, int size)
{
	reg->origin_sourceid = m_uid;
	reg->origin_streamid = m_stream_id;
	reg->status = 0;

	AddPacket(m_stream_id, MSG2_STREAM_REGISTER, size, (char*)reg);
	LOG("adding local stream: " + TOSTRING(m_uid) + ":"+ TOSTRING(m_stream_id) + ", type: " + TOSTRING(reg->type));

	m_stream_id++;
}

void HandleStreamData()
{
	std::vector<recv_packet_t> packet_buffer;

	{
		std::lock_guard<std::mutex> lock(m_recv_packetqueue_mutex);
		packet_buffer = m_recv_packet_buffer;
		m_recv_packet_buffer.clear();
	}

	RoR::ChatSystem::HandleStreamData(packet_buffer);
	BeamFactory::getSingleton().handleStreamData(packet_buffer);
	// Updates characters last (or else beam coupling might fail)
	CharacterFactory::getSingleton().handleStreamData(packet_buffer);
}

Ogre::String GetTerrainName()
{
	return m_server_settings.terrain;
}

int GetUserColor()
{
	std::lock_guard<std::mutex> lock(m_userdata_mutex);
	return m_userdata.colournum;
}

Ogre::UTFString GetUsername()
{
	std::lock_guard<std::mutex> lock(m_userdata_mutex);
	return m_username;
}

user_info_t GetLocalUserData()
{
	std::lock_guard<std::mutex> lock(m_userdata_mutex);
	return m_userdata;
}

std::vector<user_info_t> GetUserInfos()
{
	std::lock_guard<std::mutex> lock(m_users_mutex);
	return m_users;
}

bool GetUserInfo(int uid, user_info_t &result)
{
	std::lock_guard<std::mutex> lock(m_users_mutex);
	for (user_info_t user : m_users)
	{
		if ((int)user.uniqueid == uid)
		{
			result = user;
			return true;
		}
	}
	return false;
}

} // namespace Networking
} // namespace RoR

#endif // USE_SOCKETW
