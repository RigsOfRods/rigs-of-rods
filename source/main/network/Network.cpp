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

#include "Network.h"

#include "BeamFactory.h"
#include "CharacterFactory.h"
#include "ChatSystem.h"
#include "Console.h"
#include "ErrorUtils.h"
#include "GUIMenu.h"
#include "GUIMp.h"
#include "Language.h"
#include "RoRVersion.h"
#include "Settings.h"
#include "SHA1.h"
#include "Utils.h"
#include "Scripting.h"

#ifdef USE_CRASHRPT
# include "crashrpt.h"
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
//#include <CFUserNotification.h>
#endif

using namespace Ogre;

Network *net_instance;

void *s_sendthreadstart(void* vid)
{
#ifdef USE_CRASHRPT
	if (!BSETTING("NoCrashRpt"))
	{
		// add the crash handler for this thread
		CrThreadAutoInstallHelper cr_thread_install_helper;
		assert(cr_thread_install_helper.m_nInstallStatus==0);
	}
#endif //USE_CRASHRPT
	net_instance->sendthreadstart();
	return NULL;
}

void *s_receivethreadstart(void* vid)
{
#ifdef USE_CRASHRPT
	if (!BSETTING("NoCrashRpt"))
	{
		// add the crash handler for this thread
		CrThreadAutoInstallHelper cr_thread_install_helper(0);
		assert(cr_thread_install_helper.m_nInstallStatus==0);
	}
#endif //USE_CRASHRPT
	net_instance->receivethreadstart();
	return NULL;
}

Timer Network::timer = Ogre::Timer();
unsigned int Network::myuid=0;

Network::Network(String servername, long server_port) :
	  lagDataClients()
	, initiated(false)
	, net_quality(0)
	, net_quality_changed(false)
{

	pthread_mutex_init(&mutex_data, NULL);

	// update factories network objects

	memset(&server_settings, 0, sizeof(server_info_t));
	memset(&userdata, 0, sizeof(user_info_t));
	shutdown=false;

	m_server_name = servername;
	m_server_port = server_port;
	myauthlevel = AUTH_NONE;
	net_instance=this;
	nickname = "";
	myuid=0;

	speed_time=0;
	speed_bytes_sent = speed_bytes_sent_tmp = speed_bytes_recv = speed_bytes_recv_tmp = 0;

	rconauthed=0;
	last_time=0;
	send_buffer=0;
	pthread_cond_init(&send_work_cv, NULL);
	pthread_mutex_init(&msgsend_mutex, NULL);
	pthread_mutex_init(&send_work_mutex, NULL);
	pthread_mutex_init(&dl_data_mutex, NULL);
	pthread_mutex_init(&clients_mutex, NULL);

	// reset client list
	MUTEX_LOCK(&clients_mutex);
	for (int i=0; i<MAX_PEERS; i++)
	{
		clients[i].used=false;
		memset(&clients[i].user, 0, sizeof(user_info_t));
	}
	MUTEX_UNLOCK(&clients_mutex);

	// direct start, no vehicle required
	initiated = true;
}

Network::~Network()
{
	shutdown=true;
	pthread_mutex_destroy(&clients_mutex);
	pthread_mutex_destroy(&send_work_mutex);
	pthread_mutex_destroy(&dl_data_mutex);
	pthread_cond_destroy(&send_work_cv);
}

void Network::netFatalError(UTFString errormsg, bool exitProgram)
{
	if (shutdown)
		return;

	SWBaseSocket::SWBaseError error;
	socket.set_timeout(1, 1000);
	socket.disconnect(&error);
	ErrorUtils::ShowError(_L("Network Connection Problem"), _L("Network fatal error: ") + errormsg);
	if (exitProgram)
		exit(124);
}

bool Network::connect()
{
	//here we go
	//begin setup with the caller thread
	SWBaseSocket::SWBaseError error;

	//manage the server socket
	socket.set_timeout(10, 10000); // 10 seconds timeout set as default
	socket.connect(m_server_port, m_server_name, &error);
	if (error!=SWBaseSocket::ok)
	{
		//this is an error!
		netFatalError(_L("Establishing network session: "), false);
		return false;
	}
	//say hello to the server
	if (sendmessage(&socket, MSG2_HELLO, 0, (unsigned int)strlen(RORNET_VERSION), (char *)RORNET_VERSION))
	{
		//this is an error!
		netFatalError(_L("Establishing network session: error sending hello"), false);
		return false;
	}

	header_t header;
	char buffer[MAX_MESSAGE_LENGTH];
	//get server version
	if (receivemessage(&socket, &header, buffer, 255))
	{
		//this is an error!
		netFatalError(_L("Establishing network session: error getting server version"), false);
		return false;
	}
	if (header.command == MSG2_WRONG_VER)
	{
		netFatalError(_L("server uses a different protocol version"));
		return false;
	}
	
	if (header.command != MSG2_HELLO)
	{
		netFatalError(_L("Establishing network session: error getting server hello"));
		return false;
	}

	// save server settings
	memcpy(&server_settings, buffer, sizeof(server_info_t));

	if (strncmp(server_settings.protocolversion, RORNET_VERSION, strlen(RORNET_VERSION)))
	{
		wchar_t tmp[512] = L"";
		UTFString tmp2 = _L("Establishing network session: wrong server version, you are using version '%s' and the server is using '%s'");
		swprintf(tmp, 512, tmp2.asWStr_c_str(), RORNET_VERSION, server_settings.protocolversion);
		netFatalError(UTFString(tmp));
		return false;
	}
	// first handshake done, increase the timeout, important!
	socket.set_timeout(0, 0);

	//send credentials
	nickname = SSETTING("Nickname", "Anonymous");
	String nick = nickname;
	StringUtil::toLowerCase(nick);
	if (nick==String("pricorde") || nick==String("thomas") || nick == String("tdev"))
	{
		nickname = "Anonymous";
		SETTINGS.setSetting("Nickname", nickname);
	}

	char pwbuffer[250];
	memset(pwbuffer, 0, 250);
	strncpy(pwbuffer, SSETTING("Server password", "").c_str(), 250);

	char sha1pwresult[250];
	memset(sha1pwresult, 0, 250);
	if (strnlen(pwbuffer, 250)>0)
	{
		RoR::CSHA1 sha1;
		sha1.UpdateHash((uint8_t *)pwbuffer, (uint32_t)strnlen(pwbuffer, 250));
		sha1.Final();
		sha1.ReportHash(sha1pwresult, RoR::CSHA1::REPORT_HEX_SHORT);
	}

	String usertokenhash = SSETTING("User Token Hash", "");

	// construct user credentials
	// beware of the wchar_t converted to UTF8 for networking
	user_info_t c;
	memset(&c, 0, sizeof(user_info_t));
	// cut off the UTF string on the highest level, otherwise you will break UTF info
	strncpy((char *)c.username, nickname.substr(0, MAX_USERNAME_LEN * 0.5f).asUTF8_c_str(), MAX_USERNAME_LEN);
	strncpy(c.serverpassword, sha1pwresult, 40);
	strncpy(c.usertoken, usertokenhash.c_str(), 40);
	strncpy(c.clientversion, ROR_VERSION_STRING, strnlen(ROR_VERSION_STRING, 25));
	strcpy(c.clientname, "RoR");
	String lang = SSETTING("Language Short", "en");
	strncpy(c.language, lang.c_str(), std::min<int>((int)lang.size(), 10));
	String guid = SSETTING("GUID", "");
	strncpy(c.clientGUID, guid.c_str(), std::min<int>((int)guid.size(), 10));
	strcpy(c.sessiontype, "normal");
	if (sendmessage(&socket, MSG2_USER_INFO, 0, sizeof(user_info_t), (char*)&c))
	{
		//this is an error!
		netFatalError(_L("Establishing network session: error sending user info"), false);
		return false;
	}
	//now this is important, getting authorization
	if (receivemessage(&socket, &header, buffer, 255))
	{
		//this is an error!
		netFatalError(_L("Establishing network session: error getting server authorization"), false);
		return false;
	}
	if (header.command==MSG2_FULL)
	{
		//this is an error!
		netFatalError(_L("Establishing network session: sorry, server has too many players"), false);
		return false;
	}
	else if (header.command==MSG2_BANNED)
	{
		wchar_t tmp[512];
		memset(tmp, 0, 512);
		if (buffer && strnlen(buffer, 20)>0)
		{
			buffer[header.size]=0;
			UTFString tmp2 = _L("Establishing network session: sorry, you are banned:\n%s");
			swprintf(tmp, 512, tmp2.asWStr_c_str(), buffer);
			netFatalError(UTFString(tmp));
		} else
		{
			netFatalError(_L("Establishing network session: sorry, you are banned!"), false);
		}

		return false;
	}
	else if (header.command==MSG2_WRONG_PW)
	{
		//this is an error!
		netFatalError(_L("Establishing network session: sorry, wrong password!"), false);
		return false;
	}
	else if (header.command==MSG2_WRONG_VER)
	{
		//this is an error!
		netFatalError(_L("Establishing network session: sorry, wrong protocol version!"), false);
		return false;
	}
	if (header.command!=MSG2_WELCOME)
	{
		//this is an error!
		netFatalError(_L("Establishing network session: sorry, unknown server response"), false);
		return false;
	}
	//okay keep our uid
	myuid = header.source;

	// we get our userdata back
	memcpy(&userdata, buffer, std::min<int>(sizeof(user_info_t), header.size));

	//start the handling threads
	pthread_create(&sendthread, NULL, s_sendthreadstart, (void*)(0));
	pthread_create(&receivethread, NULL, s_receivethreadstart, (void*)(0));

	return true;
}

Ogre::UTFString Network::getNickname(bool colour)
{
	if (colour)
		return ChatSystem::getColouredName(nickname, myauthlevel, userdata.colournum);
	else
		return nickname;
}

int Network::sendMessageRaw(SWInetSocket *socket, char *buffer, unsigned int msgsize)
{
	//LOG("* sending raw message: " + TOSTRING(msgsize));

	MUTEX_LOCK(&msgsend_mutex); //we use a mutex because a chat message can be sent asynchronously
	SWBaseSocket::SWBaseError error;

	int rlen=0;
	while (rlen<(int)msgsize)
	{
		int sendnum=socket->send(buffer+rlen, msgsize-rlen, &error);
		if (sendnum<0)
		{
			LOG("NET send error: " + TOSTRING(sendnum));
			return -1;
		}
		rlen+=sendnum;
	}
	MUTEX_UNLOCK(&msgsend_mutex);
	return 0;
}

int Network::sendmessage(SWInetSocket *socket, int type, unsigned int streamid, unsigned int len, char* content)
{
	MUTEX_LOCK(&msgsend_mutex); //we use a mutex because a chat message can be sent asynchronously
	SWBaseSocket::SWBaseError error;
	header_t head;
	memset(&head, 0, sizeof(header_t));
	head.command=type;
	head.source=myuid;
	head.size=len;
	head.streamid=streamid;
	//int hlen=0;

	// construct buffer
	const int msgsize = sizeof(header_t) + len;

	if (msgsize >= MAX_MESSAGE_LENGTH)
	{
    	return -2;
	}

	char buffer[MAX_MESSAGE_LENGTH];
	memset(buffer, 0, MAX_MESSAGE_LENGTH);
	memcpy(buffer, (char *)&head, sizeof(header_t));
	memcpy(buffer+sizeof(header_t), content, len);

	int rlen=0;
	speed_bytes_sent_tmp += msgsize;
	while (rlen<(int)msgsize)
	{
		int sendnum=socket->send(buffer+rlen, msgsize-rlen, &error);
		if (sendnum<0)
		{
			LOG("NET send error: " + TOSTRING(sendnum));
			return -1;
		}
		rlen+=sendnum;
	}
	MUTEX_UNLOCK(&msgsend_mutex);
	calcSpeed();
	return 0;
}

int Network::receivemessage(SWInetSocket *socket, header_t *head, char* content, unsigned int bufferlen)
{
	SWBaseSocket::SWBaseError error;

	char buffer[MAX_MESSAGE_LENGTH];
	//ensure that the buffer is clean after each received message!
	memset(buffer, 0, MAX_MESSAGE_LENGTH);

	int hlen=0;
	while (hlen<(int)sizeof(header_t))
	{
		int recvnum=socket->recv(buffer+hlen, sizeof(header_t)-hlen,&error);
		if (recvnum<0)
		{
			LOG("NET receive error 1: " + TOSTRING(recvnum));
			return -1;
		}
		hlen+=recvnum;
	}

	memcpy(head, buffer, sizeof(header_t));

	if (head->size >= MAX_MESSAGE_LENGTH)
	{
    	return -3;
	}

	if (head->size>0)
	{
		if ((int)sizeof(header_t) > 0)
		{
			//read the rest
			while (hlen<(int)sizeof(header_t)+(int)head->size)
			{
				int recvnum=socket->recv(buffer+hlen, (head->size+sizeof(header_t))-hlen,&error);
				if (recvnum<0)
				{
					LOG("NET receive error 2: "+ TOSTRING(recvnum));
					return -1;
				}
				hlen+=recvnum;
			}
		}
	}
	speed_bytes_recv_tmp += head->size + sizeof(header_t);

	memcpy(content, buffer+sizeof(header_t), bufferlen);
	calcSpeed();
	return 0;
}


int Network::getSpeedUp()
{
	return speed_bytes_sent;
}

int Network::getSpeedDown()
{
	return speed_bytes_recv;
}

void Network::calcSpeed()
{
	int t = timer.getMilliseconds();
	if (t - speed_time > 1000)
	{
		// we measure bytes / second
		speed_bytes_sent = speed_bytes_sent_tmp;
		speed_bytes_sent_tmp = 0;
		speed_bytes_recv = speed_bytes_recv_tmp;
		speed_bytes_recv_tmp = 0;
		speed_time = t;
	}
}

void Network::sendthreadstart()
{
	LOG("Sendthread starting");
	while (!shutdown)
	{
		// wait for data...
		NetworkStreamManager::getSingleton().sendStreams(this, &socket);

	}
}

void Network::disconnect()
{
	shutdown=true;
	sendmessage(&socket, MSG2_USER_LEAVE, 0, 0, 0);
	SWBaseSocket::SWBaseError error;
	socket.set_timeout(1, 1000);
	socket.disconnect(&error);
	LOG("Network error while disconnecting: ");
}

int Network::sendScriptMessage(char* content, unsigned int len)
{
	int result = sendmessage(&socket, MSG2_GAME_CMD, 0, len, content);
	if (result<0) LOG("An error occurred while sending a script message to the server.");
	return result;
}

unsigned long Network::getNetTime()
{
	return timer.getMilliseconds();
}

void Network::receivethreadstart()
{
	header_t header;

	char *buffer=(char*)malloc(MAX_MESSAGE_LENGTH);
	//bool autoDl = (BSETTING("AutoDownload", false));
	std::deque < stream_reg_t > streamCreationResults;
	LOG("Receivethread starting");
	// unlimited timeout, important!

	// wait for beamfactory to be existant before doing anything
	// otherwise you can get runtime conditions
	while(!BeamFactory::getSingletonPtr())
	{
		sleepMilliSeconds(1000);
	};


	socket.set_timeout(0,0);
	while (!shutdown)
	{
		//get one message
		int err=receivemessage(&socket, &header, buffer, MAX_MESSAGE_LENGTH);
		//LOG("received data: " + TOSTRING(header.command) + ", source: "+TOSTRING(header.source) + ":"+TOSTRING(header.streamid) + ", size: "+TOSTRING(header.size));
		if (err)
		{
			//this is an error!
			char errmsg[256];
			sprintf(errmsg, "Error %i while receiving data", err);
			netFatalError(errmsg);
			return;
		}

		// check for stream registration errors and notify the remote client
		if (BeamFactory::getSingletonPtr() && BeamFactory::getSingletonPtr()->getStreamRegistrationResults(&streamCreationResults))
		{
			while (!streamCreationResults.empty())
			{
				stream_reg_t r = streamCreationResults.front();
				stream_register_t reg = r.reg;
				sendmessage(&socket, MSG2_STREAM_REGISTER_RESULT, 0, sizeof(stream_register_t), (char *)&reg);
				streamCreationResults.pop_front();
			}
		}

		// TODO: produce new streamable classes when required
		if (header.command == MSG2_STREAM_REGISTER)
		{
			if (header.source == (int)myuid)
				// our own stream, ignore
				continue;
			stream_register_t *reg = (stream_register_t *)buffer;
			client_t *client = getClientInfo(header.source);
			int playerColour = 0;
			if (client) playerColour = client->user.colournum;

			String typeStr = "unknown";
			switch(reg->type)
			{
				case 0: typeStr="truck"; break;
				case 1: typeStr="character"; break;
				case 3: typeStr="chat"; break;
			};
			LOG(" * received stream registration: " + TOSTRING(header.source) + ": "+TOSTRING(header.streamid) + ", type: "+typeStr);

			if (reg->type == 0)
			{
				// truck
				BeamFactory::getSingleton().createRemote(header.source, header.streamid, reg, playerColour);
			} else if (reg->type == 1)
			{
				// person
				CharacterFactory::getSingleton().createRemote(header.source, header.streamid, reg, playerColour);
			} else if (reg->type == 2)
			{
				// previously AITRAFFIC, unused for now
			} else if (reg->type == 3)
			{
				// chat stream
				ChatSystemFactory::getSingleton().createRemote(header.source, header.streamid, reg, playerColour);
			}
			continue;
		}
		else if (header.command == MSG2_STREAM_REGISTER_RESULT)
		{
			stream_register_t *reg = (stream_register_t *)buffer;
			BeamFactory::getSingleton().addStreamRegistrationResults(header.source, reg);
			LOG(" * received stream registration result: " + TOSTRING(header.source) + ": "+TOSTRING(header.streamid));
		}
		else if (header.source == -1 && (header.command == MSG2_UTF_CHAT || header.command == MSG2_UTF_PRIVCHAT))
		{
			// NOTE: this is only a shortcut for server messages, other UIDs propagate over the standard way
			ChatSystem *cs = ChatSystemFactory::getSingleton().getFirstChatSystem();
			if (cs) cs->addReceivedPacket(header, buffer);
			continue;
		}
		else if (header.command == MSG2_NETQUALITY && header.source == -1)
		{
			if (header.size != sizeof(int))
				continue;
			int quality = *(int *)buffer;
			setNetQuality(quality);
			continue;
		}
		else if (header.command == MSG2_USER_LEAVE)
		{
			if (header.source == (int)myuid)
			{
				netFatalError(_L("disconnected: remote side closed the connection"), false);
				return;
			}

			// remove all things that belong to that user
			client_t *client = getClientInfo(header.source);
			if (client)
				client->used = false;

			// now remove all possible streams
			NetworkStreamManager::getSingleton().removeUser(header.source);

#ifdef USE_MYGUI
			if (GUI_MainMenu::getSingletonPtr() != nullptr) // Does menubar exist yet?
				// Executed in main menu, but menubar isn't created until simulation starts.
			{
				// we can trigger this in the network thread as the function is thread safe.
				GUI_MainMenu::getSingleton().triggerUpdateVehicleList();
			}
#endif // USE_MYGUI
			continue;
		}
		else if (header.command == MSG2_USER_INFO || header.command == MSG2_USER_JOIN)
		{
			if (header.source == (int)myuid)
			{
				// we got data about ourself!
				memcpy(&userdata, buffer, sizeof(user_info_t));
				CharacterFactory::getSingleton().localUserAttributesChanged(myuid);
				// update our nickname
				nickname = UTFString(userdata.username);
				// save it in the Settings as well
				SETTINGS.setUTFSetting(L"Nickname", nickname);
				// update auth status
				myauthlevel = userdata.authstatus;
			} else
			{
				user_info_t *cinfo = (user_info_t*) buffer;
				// data about someone else, try to update the array
				bool found = false; // whether to add a new client
				client_t *client = getClientInfo(header.source);
				if (client)
				{
					memcpy(&client->user, cinfo, sizeof(user_info_t));

					// inform the streamfactories of a attribute change
					CharacterFactory::getSingleton().netUserAttributesChanged(header.source, -1);
					BeamFactory::getSingleton().netUserAttributesChanged(header.source, -1);
					found = true;
				} else
				{
					// find a free entry
					MUTEX_LOCK(&clients_mutex);
					for (int i=0; i<MAX_PEERS; i++)
					{
						if (clients[i].used)
							continue;
						clients[i].used = true;
						memcpy(&clients[i].user, cinfo, sizeof(user_info_t));

						// inform the streamfactories of a attribute change
						CharacterFactory::getSingleton().netUserAttributesChanged(header.source, -1);
						BeamFactory::getSingleton().netUserAttributesChanged(header.source, -1);
						break;
					}
					MUTEX_UNLOCK(&clients_mutex);
				}
			}
#ifdef USE_MYGUI
			if (GUI_MainMenu::getSingletonPtr() != nullptr) // Does menubar exist yet? 
				// This code is executed in selector already, but Menubar isn't created until simulation launches.
			{
				// we can trigger this in the network thread as the function is thread safe.
				GUI_MainMenu::getSingleton().triggerUpdateVehicleList();
			}
#endif // USE_MYGUI

			continue;
		}
		else if (header.command == MSG2_GAME_CMD)
		{
#ifdef USE_ANGELSCRIPT
			ScriptEngine::getSingleton().queueStringForExecution(Ogre::String(buffer));
#endif // USE_ANGELSCRIPT
			continue;
		}
		//debugPacket("receive-1", &header, buffer);
		NetworkStreamManager::getSingleton().pushReceivedStreamMessage(header, buffer);
	}
}

int Network::getClientInfos(client_t c[MAX_PEERS])
{
	if (!initiated) return 1;
	MUTEX_LOCK(&clients_mutex);
	for (int i=0;i<MAX_PEERS;i++)
		c[i] = clients[i]; // copy the whole client list
	MUTEX_UNLOCK(&clients_mutex);
	return 0;
}

client_t *Network::getClientInfo(unsigned int uid)
{
// this is a deadlock here
//	MUTEX_LOCK(&clients_mutex);
	client_t *c = 0;
	for (int i=0; i<MAX_PEERS; i++)
	{
		if (clients[i].user.uniqueid == uid)
			c = &clients[i];
	}
//	MUTEX_UNLOCK(&clients_mutex);
	return c;
}

void Network::debugPacket(const char *name, header_t *header, char *buffer)
{
	char sha1result[250];
	memset(sha1result, 0, 250);
	if (buffer)
	{
		RoR::CSHA1 sha1;
		sha1.UpdateHash((uint8_t *)buffer, header->size);
		sha1.Final();
		sha1.ReportHash(sha1result, RoR::CSHA1::REPORT_HEX_SHORT);
	}

	char tmp[256]="";
	sprintf(tmp, "++ %s: %d:%d, %d, %d, hash: %s", name, header->source, header->streamid, header->command, header->size, sha1result);
	LOG(tmp);
	//String hex = hexdump(buffer, header->size);
	//LOG(hex);
}

void Network::setNetQuality(int q)
{
	MUTEX_LOCK(&mutex_data);
	net_quality = q;
	net_quality_changed = true;
	MUTEX_UNLOCK(&mutex_data);
}

int Network::getNetQuality(bool ack)
{
	int res = 0;
	MUTEX_LOCK(&mutex_data);
	res = net_quality;
	if (ack) net_quality_changed=false;
	MUTEX_UNLOCK(&mutex_data);
	return res;
}

bool Network::getNetQualityChanged()
{
	bool res = false;
	MUTEX_LOCK(&mutex_data);
	res = net_quality_changed;
	MUTEX_UNLOCK(&mutex_data);
	return res;
}

#endif // USE_SOCKETW
