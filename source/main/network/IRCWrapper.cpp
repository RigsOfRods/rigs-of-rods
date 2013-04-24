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
// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 7th of August 2009
#ifdef USE_SOCKETW

#include "IRCWrapper.h"

#include "ErrorUtils.h"
#include "ImprovedConfigFile.h"
#include "rornet.h"
#include "RoRVersion.h"
#include "Settings.h"
#include "Utils.h"

#ifdef USE_CURL
#define CURL_STATICLIB
#include <stdio.h>
#include <curl/curl.h>
#include <curl/easy.h>
#endif //USE_CURL

// some function forward declarations
void *s_ircthreadstart(void* arg);

using namespace Ogre;
// some utils
// helps to fill our struct
message_t constructMessage(int type, const char *channel, const char *nick, const char *message, const char *arg = 0)
{
	message_t t;
	t.type    = type;
	t.channel = String();
	t.nick    = String();
	t.message = String();
	t.arg     = String();

	if (channel) t.channel = String(channel);
	if (nick)    t.nick    = String(nick);
	if (message) t.message = String(message);
	if (arg)     t.arg     = String(arg);
	return t;
}

// and the class implementation

IRCWrapper::IRCWrapper() : irc_session(0)
{
	// set default values
	serverName     = "irc.rigsofrods.org";
	serverPassword = "";
	nick           = "foouser";
	userName       = nick;
	realName       = nick;
	channel        = "#RigsOfRods";
	channelKey     = "";
	serverPort     = 6667; // 6667 = default IRC port
	reJoin         = true; // rejoin if we got kicked
	reConnect      = true; // reconnect if we got disconnected
	retryCounter   = 0;    // Counter that counts up connection retries
	wasConnected   = false; // already connected once
}

IRCWrapper::~IRCWrapper()
{
	if (irc_session && irc_is_connected(irc_session))
	{
		irc_disconnect(irc_session);
	}
}

void IRCWrapper::initIRC()
{
	// authenticate before doing anything else
	pthread_create(&ircthread, NULL, s_ircthreadstart, (void *)this);
}

void IRCWrapper::process()
{
	if (reConnect && wasConnected)
	{
		// check if we are still connected
		if (irc_session && !irc_is_connected(irc_session))
		{
			push(constructMessage(MT_StatusUpdate, 0, 0, "Disconnected, reconnecting ...", "1"));
			// disconnected, reconnect now
			irc_disconnect(irc_session); // be sure to let the old thread end ...

			initIRC();
		}
	}
	std::vector<message_t> messages;
	int num = pull(messages);
	if (num == 0)
		return;

	// there is something, lets investigate

	std::vector<message_t>::iterator it;
	for (it = messages.begin(); it != messages.end(); it++)
	{
		processIRCEvent(*it);
	}
}

int IRCWrapper::sendMessage(String msg, String channelOrNick)
{
	if (!irc_session) return 1;
	if (channelOrNick.empty())
		channelOrNick = channel;

	return irc_cmd_msg(irc_session, channelOrNick.c_str(), msg.c_str());
}

int IRCWrapper::sendMeMessage(String msg, String channelOrNick)
{
	if (!irc_session) return 1;
	if (channelOrNick.empty())
		channelOrNick = channel;

	return irc_cmd_me(irc_session, channelOrNick.c_str(), msg.c_str());
}

int IRCWrapper::changeNick(String newNick)
{
	if (!irc_session) return 1;
	return irc_cmd_nick(irc_session, newNick.c_str());
}

int IRCWrapper::joinChannel(String channel, String channelKeyStr)
{
	if (!irc_session) return 1;
	const char *channelKey = channelKeyStr.c_str();
	if (channelKeyStr.empty()) channelKey = 0;
	return irc_cmd_join (irc_session, channel.c_str(), channelKey);
}

int IRCWrapper::leaveChannel(String channel)
{
	if (!irc_session) return 1;
	return irc_cmd_part (irc_session, channel.c_str());
}

int IRCWrapper::quit(String reason)
{
	if (!irc_session) return 1;
	return irc_cmd_quit (irc_session, reason.c_str());
}

String IRCWrapper::getLastErrorMessage()
{
	if (!irc_session) return "not connected";
	return String(irc_strerror(irc_errno(irc_session)));
}


#ifdef USE_CURL
struct curlMemoryStruct {
	char *memory;
	size_t size;
};

//hacky hack to fill memory with data for curl
// from: http://curl.haxx.se/libcurl/c/getinmemory.html
static size_t curlWriteMemoryCallback2(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t realsize = size * nmemb;
	struct curlMemoryStruct *mem = (struct curlMemoryStruct *)data;
	char* new_mem;

	new_mem = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if (new_mem == NULL) {
		free(mem->memory);
		printf("Error (re)allocating memory\n");
		exit(EXIT_FAILURE);
	}
	mem->memory = new_mem;

	memcpy(&(mem->memory[mem->size]), ptr, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}
#endif //USE_CURL

int IRCWrapper::authenticate()
{
#ifdef USE_CURL
	struct curlMemoryStruct chunk;

	chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	// construct post fields
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;
	curl_global_init(CURL_GLOBAL_ALL);

	// add some hard coded values
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_NickName", CURLFORM_COPYCONTENTS, SSETTING("Nickname", "Anonymous").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_Language", CURLFORM_COPYCONTENTS, SSETTING("Language", "English").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_Token", CURLFORM_COPYCONTENTS, SSETTING("User Token Hash", "-").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionString", CURLFORM_COPYCONTENTS, ROR_VERSION_STRING, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionSVN", CURLFORM_COPYCONTENTS, SVN_REVISION, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionSVNID", CURLFORM_COPYCONTENTS, SVN_ID, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_ProtocolVersion", CURLFORM_COPYCONTENTS, RORNET_VERSION, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_BinaryHash", CURLFORM_COPYCONTENTS, SSETTING("BinaryHash", "-").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_GUID", CURLFORM_COPYCONTENTS, SSETTING("GUID", "-").c_str(), CURLFORM_END);


	CURLcode res;
	CURL *curl = curl_easy_init();
	if (!curl)
	{
		push(constructMessage(MT_ErrorAuth, 0, 0, "Error: failed to init CURL"));
		return 1;
	}

	char *curl_err_str[CURL_ERROR_SIZE];
	memset(curl_err_str, 0, CURL_ERROR_SIZE);

	String url = "http://" + String(REPO_SERVER) + "/auth_lobby/";
	curl_easy_setopt(curl, CURLOPT_URL,              url.c_str());

	/* send all data to this function  */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteMemoryCallback2);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	// set post options
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

	// logging stuff
	//curl_easy_setopt(curl, CURLOPT_STDERR,           LogManager::getsin InstallerLog::getSingleton()->getLogFilePtr());
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER,      curl_err_str[0]);

	// http related settings
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,   1); // follow redirects
	curl_easy_setopt(curl, CURLOPT_AUTOREFERER,      1); // set the Referrer: field in requests where it follows a Location: redirect.
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS,        20);
	curl_easy_setopt(curl, CURLOPT_USERAGENT,        "RoR");
	curl_easy_setopt(curl, CURLOPT_FILETIME,         1);

	// TO BE DONE: ADD SSL
	// see: http://curl.haxx.se/libcurl/c/simplessl.html
	// curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);

	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	//printf("%lu bytes retrieved\n", (long)chunk.size);

	String result;

	curl_formfree(formpost);

	if (chunk.memory)
	{
		// convert memory into String now
		result = String(chunk.memory);

		// then free
		free(chunk.memory);
	}

	/* we're done with libcurl, so clean it up */
	curl_global_cleanup();

	if (res != CURLE_OK)
	{
		const char *errstr = curl_easy_strerror(res);
		push(constructMessage(MT_ErrorAuth, 0, 0, errstr));
		return 1;
	}

	return processAuthenticationResults(result);
#else
	push(constructMessage(MT_ErrorAuth, 0, 0, "you have not compiled RoR with CURL support, thus authentication and the lobby are not available to you."));
	return 1;
#endif //USE_CURL
}

int IRCWrapper::processAuthenticationResults(String &results)
{
	ImprovedConfigFile cfg;
	cfg.loadFromString(results, "=", true);

	// TODO: improve the checks if the config is valid or not ...

	// fatal error?
	if (cfg.hasSetting("fatalError"))
	{
		showOgreWebError(cfg.getSetting("fatalErrorTitle"), cfg.getSetting("fatalError"), cfg.getSetting("fatalErrorURL"));
		return 1;
	}

	// non-fatal error?
	if (cfg.hasSetting("error"))
	{
		push(constructMessage(MT_ErrorAuth, 0, 0, cfg.getSetting("error").c_str()));
		return 1;
	}

	if (!cfg.hasSetting("serverName") || !cfg.hasSetting("serverPort"))
		return 1;

	serverName     = cfg.getSetting("serverName");
	serverPort     = cfg.getSettingInt("serverPort");
	serverPassword = cfg.getSetting("serverPassword");
	nick           = cfg.getSetting("nick");
	userName       = cfg.getSetting("userName");
	realName       = cfg.getSetting("realName");
	channel        = cfg.getSetting("channel");
	reJoin         = cfg.getSettingBool("reJoin");
	reConnect      = cfg.getSettingBool("reConnect");
	
	// TODO:
	//userAuth = cfg.getSetting("userAuth");
	//userGroups = cfg.getSetting("userGroups");

	return 0;
}

// C from libIRCClient BELOW

// detects if we are meant
bool isSelf(IRCWrapper * ctx, const char *origin)
{
	if (!origin) return false;

	// we store the nickname without the host information, so cut off the host to find out if its us
	String org = String(origin);
	size_t s = org.find("!");
	if (s != org.npos)
	{
		org = org.substr(0, s);
	}

	return (org == ctx->nick);
}

void addlog (const char * fmt, ...)
{
	char buf[1024];
	va_list va_alist;

	va_start (va_alist, fmt);
#if defined (WIN32)
	_vsnprintf (buf, sizeof(buf), fmt, va_alist);
#else
	vsnprintf (buf, sizeof(buf), fmt, va_alist);
#endif
	va_end (va_alist);

	LOG("IRC| " + String(buf));
}

void dump_event (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	char buf[512] = {};
	unsigned int cnt;

	buf[0] = '\0';

	for ( cnt = 0; cnt < count; cnt++ )
	{
		if ( cnt )
			strcat (buf, "|");

		strcat (buf, params[cnt]);
	}

	addlog ("Event \"%s\", origin: \"%s\", params: %d [%s]", event, origin ? origin : "NULL", cnt, buf);
}

void event_connect (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	// no params
	IRCWrapper * ctx = (IRCWrapper *) irc_get_ctx (session);
	
	ctx->push(constructMessage(MT_StatusUpdate, 0, 0, "Joining Channels ...", "1"));
	
	// join our preset channel
	const char *channelKey = ctx->channelKey.c_str();
	if (ctx->channelKey.empty()) channelKey = 0;
	irc_cmd_join (session, ctx->channel.c_str(), channelKey);
	// tell the main that we are connected now
	ctx->wasConnected = true;
}

void event_nick (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	/*
		origin 	the person, who changes the nick. Note that it can be you!
		params[0] 	mandatory, contains the new nick.
	*/
	if ( count != 1 ) return;
	IRCWrapper * ctx = (IRCWrapper *) irc_get_ctx (session);
	
	bool myself = isSelf(ctx, origin);
	ctx->push(constructMessage(myself ? MT_NickChangeSelf : MT_NickChangeOther, 0, origin, params[0]));
}

void event_quit (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	/*
		origin 	the person, who is disconnected
		params[0] 	optional, contains the reason message (user-specified).
	*/
	if ( count != 1 ) return;

	IRCWrapper * ctx = (IRCWrapper *) irc_get_ctx (session);
	ctx->push(constructMessage(MT_QuitOther, 0, origin, params[0]));
}

void event_join (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	/*
		origin 	the person, who joins the channel. By comparing it with your own nickname, you can check whether your JOIN command succeed.
		params[0] 	mandatory, contains the channel name.
	*/
	
	// set some user modes
	
	// i - if set, marks a user as 'invisible' - that is, not seen by lookups if the user is not in a channel.
	// s - if set, marks a user for receipt of server notices.
	irc_cmd_user_mode (session, "+is");

	IRCWrapper * ctx = (IRCWrapper *) irc_get_ctx (session);
	bool myself = isSelf(ctx, origin);
	ctx->push(constructMessage(myself ? MT_JoinChannelSelf : MT_JoinChannelOther, params[0], origin, 0));

	if (myself)
	{
		// clear status
		ctx->push(constructMessage(MT_StatusUpdate, 0, 0, "", ""));
	}
}

void event_part (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	/*
		origin 	the person, who leaves the channel. By comparing it with your own nickname, you can check whether your PART command succeed.
		params[0] 	mandatory, contains the channel name.
		params[1] 	optional, contains the reason message (user-defined).
	*/
	if ( count != 2 ) return;
	IRCWrapper * ctx = (IRCWrapper *) irc_get_ctx (session);
	bool myself = isSelf(ctx, origin);
	ctx->push(constructMessage(myself ? MT_PartChannelSelf : MT_PartChannelOther, params[0], origin, params[1]));
}

void event_mode (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	/*
		origin 	the person, who changed the channel mode.
		params[0] 	mandatory, contains the channel name.
		params[1] 	mandatory, contains the changed channel mode, like '+t', '-i' and so on.
		params[2] 	optional, contains the mode argument (for example, a key for +k mode, or user who got the channel operator status for +o mode)
	*/
	if ( count != 3 ) return;
	IRCWrapper * ctx = (IRCWrapper *) irc_get_ctx (session);
	ctx->push(constructMessage(MT_ChangedChannelMode, params[0], params[1], 0, params[2]));
}

void event_umode (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	/*
		origin 	the person, who changed the user mode.
		params[0] 	mandatory, contains the user changed mode, like '+t', '-i' and so on.
	*/
	if ( count != 1 ) return;
	IRCWrapper * ctx = (IRCWrapper *) irc_get_ctx (session);
	ctx->push(constructMessage(MT_ChangedUserModeSelf, 0, 0, params[0]));
}

void event_topic (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	/*
		origin 	the person, who changes the channel topic.
		params[0] 	mandatory, contains the channel name.
		params[1] 	optional, contains the new topic.
	*/
	if (count != 2) return;
	IRCWrapper * ctx = (IRCWrapper *) irc_get_ctx (session);
	ctx->push(constructMessage(MT_ChangedChannelTopic, params[0], origin, params[1]));
}

void event_kick (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	/*
		origin 	the person, who kicked the poor.
		params[0] 	mandatory, contains the channel name.
		params[1] 	optional, contains the nick of kicked person.
		params[2] 	optional, contains the kick text
	*/
	if (count != 3) return;
	IRCWrapper * ctx = (IRCWrapper *) irc_get_ctx (session);
	bool myself = isSelf(ctx, params[1]);
	ctx->push(constructMessage(myself ? MT_WeGotKicked : MT_SomeoneGotKicked, params[0], origin, params[2], params[1]));

	if (myself && ctx->reJoin)
	{
		// try to rejoin the channel
		const char *channelKey = ctx->channelKey.c_str();
		if (ctx->channelKey.empty()) channelKey = 0;
		irc_cmd_join (session, ctx->channel.c_str(), channelKey);	
	}

}

void event_channel (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	/*
		origin 	the person, who generates the message.
		params[0] 	mandatory, contains the channel name.
		params[1] 	optional, contains the message text
	*/
	if ( count != 2 ) return;
	IRCWrapper * ctx = (IRCWrapper *) irc_get_ctx (session);
	ctx->push(constructMessage(MT_Channel, params[0], origin, params[1]));
}

void event_privmsg (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	/*
		origin 	the person, who generates the message.
		params[0] 	mandatory, contains your nick.
		params[1] 	optional, contains the message text
	*/
	if ( count != 2 ) return;
	IRCWrapper * ctx = (IRCWrapper *) irc_get_ctx (session);
	ctx->push(constructMessage(MT_GotPrivateMessage, 0, origin, params[1], params[0]));
}

void event_notice (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	/*
		origin 	the person, who generates the message.
		params[0] 	mandatory, contains the target nick name.
		params[1] 	optional, contains the message text
	*/
	if ( count != 2 ) return;
	IRCWrapper * ctx = (IRCWrapper *) irc_get_ctx (session);
	ctx->push(constructMessage(MT_GotNotice, 0, origin, params[1], params[0]));
}

void event_invite (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	/*
		origin 	the person, who INVITEs you.
		params[0] 	mandatory, contains your nick.
		params[1] 	mandatory, contains the channel name you're invited into.
	*/
	if ( count != 2 ) return;
	IRCWrapper * ctx = (IRCWrapper *) irc_get_ctx (session);
	ctx->push(constructMessage(MT_GotInvitation, params[1], origin, 0, params[0]));
}



void event_numeric (irc_session_t * session, unsigned int eventNum, const char * origin, const char ** params, unsigned int count)
{
	IRCWrapper * ctx = (IRCWrapper *) irc_get_ctx (session);

	char buf[24];
	sprintf (buf, "%d", eventNum);

	if (eventNum == 433)
	{
		// 433 = uername already used, try to take another one
		sleepMilliSeconds(500);


		//irc_ctx_t * ctx = (irc_ctx_t *) irc_get_ctx (session);
		//ctx->retryCounter++;
		ctx->nick += "_";

		// now change the nick
		irc_cmd_nick(session, ctx->nick.c_str());
		sleepMilliSeconds(500);

		// and rejoin the channels
		irc_cmd_join (session, ctx->channel.c_str(), 0);
	} else if (eventNum == 251)
	{
		// 251 = There are <integer> users and <integer> services on <integer> servers
		if (count != 2) return;
		ctx->push(constructMessage(MT_VerboseMessage, 0, origin, params[1], params[0]));
	} else if (eventNum == 372)
	{
		// 372 = RPL_MOTD
		if (count != 2) return;
		ctx->push(constructMessage(MT_VerboseMessage, 0, origin, params[1], params[0]));
	} else if (eventNum == 332)
	{
		// 332 = RPL_TOPIC / When sending a TOPIC message to determine the channel topic, one of two replies is sent. If the topic is set, RPL_TOPIC is sent back else RPL_NOTOPIC.
		if (count != 3) return;
		ctx->push(constructMessage(MT_TopicInfo, params[1], origin, params[2], params[0]));
	} else if (eventNum == 353)
	{
		// 353 = RPL_NAMREPLY
		if (count != 4) return;
		ctx->push(constructMessage(MT_NameList, params[2], origin, params[3], params[1	]));

	} else
	{
		dump_event (session, buf, origin, params, count);
	}
}


void *s_ircthreadstart(void* arg)
{
	IRCWrapper *ctx = (IRCWrapper *)arg;

	ctx->push(constructMessage(MT_StatusUpdate, 0, 0, "Authenticating ...", "1"));
	
	// authenticate before doing anything else
	// we are doing it in this thread so the userinteface will not lag
	if (ctx->authenticate())
	{
		
		ctx->push(constructMessage(MT_ErrorAuth, 0, 0, "Could not authenticate, please try again later"));
		// clear status
		ctx->push(constructMessage(MT_StatusUpdate, 0, 0, "", ""));
		// end thread
		return 0;
	}

#ifdef WIN32
	// winsock startup, required
	WORD wVersionRequested = MAKEWORD (1, 1);
	WSADATA wsaData;

	if ( WSAStartup (wVersionRequested, &wsaData) != 0 )
	{
		LOG("IRC| failed to init IRC socket");
		return 0;
	}
#endif //WIN32

	// setup the callbacks
	irc_callbacks_t	callbacks;
	memset (&callbacks, 0, sizeof(callbacks));

	// The "on_connect" event is triggered when the client successfully connects to the server, and could send commands to the server. No extra params supplied; params is 0.
	callbacks.event_connect = event_connect;
	// The "nick" event is triggered when the client receives a NICK message, meaning that someone (including you) on a channel with the client has changed their nickname.
	callbacks.event_nick = event_nick;
	// The "quit" event is triggered upon receipt of a QUIT message, which means that someone on a channel with the client has disconnected.
	callbacks.event_quit = event_quit;
	// The "join" event is triggered upon receipt of a JOIN message, which means that someone has entered a channel that the client is on.
	callbacks.event_join = event_join;
	// The "part" event is triggered upon receipt of a PART message, which means that someone has left a channel that the client is on.
	callbacks.event_part = event_part;
	// The "mode" event is triggered upon receipt of a channel MODE message, which means that someone on a channel with the client has changed the channel's parameters.
	callbacks.event_mode = event_mode;
	// The "umode" event is triggered upon receipt of a user MODE message, which means that your user mode has been changed.
	callbacks.event_umode = event_umode;
	// The "topic" event is triggered upon receipt of a TOPIC message, which means that someone on a channel with the client has changed the channel's topic.
	callbacks.event_topic = event_topic;
	// The "kick" event is triggered upon receipt of a KICK message, which means that someone on a channel with the client (or possibly the client itself!) has been forcibly ejected.
	callbacks.event_kick = event_kick;
	// The "channel" event is triggered upon receipt of a PRIVMSG message to an entire channel, which means that someone on a channel with the client has said something aloud. Your own messages don't trigger PRIVMSG event.
	callbacks.event_channel = event_channel;
	// The "privmsg" event is triggered upon receipt of a PRIVMSG message which is addressed to one or more clients, which means that someone is sending the client a private message.
	callbacks.event_privmsg = event_privmsg;
	// The "notice" event is triggered upon receipt of a NOTICE message which means that someone has sent the client a public or private notice. According to RFC 1459, the only difference between NOTICE and PRIVMSG is that you should NEVER automatically reply to NOTICE messages. Unfortunately, this rule is frequently violated by IRC servers itself - for example, NICKSERV messages require reply, and are NOTICEs.
	callbacks.event_notice = event_notice;
	// The "channel_notice" event is triggered upon receipt of a NOTICE message which means that someone has sent the client a public notice. According to RFC 1459, the only difference between NOTICE and PRIVMSG is that you should NEVER automatically reply to NOTICE messages. Unfortunately, this rule is frequently violated by IRC servers itself - for example, NICKSERV messages require reply, and are NOTICEs.
	//callbacks.event_channel_notice = dump_event;
	// The "invite" event is triggered upon receipt of an INVITE message, which means that someone is permitting the client's entry into a +i channel.
	callbacks.event_invite = event_invite;
	// The "ctcp" event is triggered when the client receives the CTCP request. By default, the built-in CTCP request handler is used. The build-in handler automatically replies on most CTCP messages, so you will rarely need to override it.
	//callbacks.event_ctcp_req = dump_event;
	// The "ctcp" event is triggered when the client receives the CTCP reply.
	//callbacks.event_ctcp_rep = dump_event;
	// The "action" event is triggered when the client receives the CTCP ACTION message.
	//callbacks.event_ctcp_action = dump_event;
	// The "unknown" event is triggered upon receipt of any number of unclassifiable miscellaneous messages, which aren't handled by the library.
	callbacks.event_unknown = dump_event;
	// The "numeric" event is triggered upon receipt of any numeric response from the server. There is a lot of such responses, see the full list here: Numeric reply codes from RFC1459.
	callbacks.event_numeric = event_numeric;
	// The "dcc chat" event is triggered when someone requests a DCC CHAT from you.
	//callbacks.event_dcc_chat_req = dump_event;
	// The "dcc chat" event is triggered when someone wants to send a file to you via DCC SEND request.
	//callbacks.event_dcc_send_req = dump_event;

	ctx->push(constructMessage(MT_StatusUpdate, 0, 0, "Connecting ...", "1"));

	ctx->irc_session = irc_create_session (&callbacks);
	if (!ctx->irc_session)
	{
		LOG ("Could not create session");
		// clear status
		ctx->push(constructMessage(MT_StatusUpdate, 0, 0, "", ""));
		return 0;
	}

	ctx->retryCounter++;
	if (ctx->retryCounter > 1)
	{
		// sleep longer the more often we try to connect - security measure
		// gives the old connection the chance to time-out
		sleepMilliSeconds(2000*ctx->retryCounter);
	}

	irc_set_ctx (ctx->irc_session, ctx);

	// allows to strip origins automatically
	irc_option_reset(ctx->irc_session, LIBIRC_OPTION_STRIPNICKS);

	// some c++ -> c mapping
	const char *serverName     = ctx->serverName.c_str();
	const char *serverPassword = ctx->serverPassword.c_str();
	if (ctx->serverPassword.empty()) serverPassword = 0;
	const char *nick           = ctx->nick.c_str();
	if (ctx->nick.empty()) nick = 0;
	const char *userName       = ctx->userName.c_str();
	if (ctx->userName.empty()) userName = 0;
	const char *realName       = ctx->realName.c_str();
	if (ctx->realName.empty()) realName = 0;

	if (irc_connect (ctx->irc_session, serverName, ctx->serverPort, serverPassword, nick, userName, realName))
	{
		LOG ("Could not connect: " + String(irc_strerror (irc_errno(ctx->irc_session))));
		return 0;
	}


	// start the main loop in this thread
	irc_run (ctx->irc_session);
	irc_destroy_session(ctx->irc_session);
	return 0;
}

#endif // USE_SOCKETW
