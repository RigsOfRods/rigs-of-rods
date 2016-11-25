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
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_SOCKETW

#include "Network.h"

#include "Application.h"
#include "BeamFactory.h"
#include "CharacterFactory.h"
#include "ChatSystem.h"
#include "ErrorUtils.h"
#include "GUIManager.h"
#include "GUI_TopMenubar.h"
#include "Language.h"
#include "RoRVersion.h"
#include "SHA1.h"
#include "ScriptEngine.h"
#include "Settings.h"
#include "Utils.h"

#include <Ogre.h>
#include <SocketW.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <deque>
#include <thread>

namespace RoR {
namespace Networking {

struct send_packet_t
{
    char buffer[MAX_MESSAGE_LENGTH];
    int size;
};

static server_info_t m_server_settings;

static Ogre::UTFString m_username; // Shadows gEnv->mp_player_name for multithreaded access.
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
static std::atomic<bool> m_recv_stopped;

static std::mutex m_users_mutex;
static std::mutex m_userdata_mutex;
static std::mutex m_error_message_mutex;
static std::mutex m_recv_packetqueue_mutex;
static std::mutex m_send_packetqueue_mutex;

static std::condition_variable m_send_packet_available_cv;

static std::vector<recv_packet_t> m_recv_packet_buffer;
static std::deque <send_packet_t> m_send_packet_buffer;

static const unsigned int m_packet_buffer_size = 20;

static std::atomic<bool> m_net_fatal_error;
static std::atomic<bool> m_socket_broken;
static Ogre::UTFString   m_error_message;

#define LOG_THREAD(_MSG_) { std::stringstream s; s << _MSG_ << " (Thread ID: " << std::this_thread::get_id() << ")"; LOG(s.str()); }

static const int RECVMESSAGE_RETVAL_SHUTDOWN = -43;

Ogre::UTFString GetErrorMessage()
{
    std::lock_guard<std::mutex> lock(m_error_message_mutex);
    return m_error_message;
}

void SetErrorMessage(Ogre::UTFString msg)
{
    std::lock_guard<std::mutex> lock(m_error_message_mutex);
    m_error_message = msg;
}

bool CheckError()
{
    if (m_net_fatal_error)
    {
        RoR::App::SetActiveMpState(RoR::App::MP_STATE_DISABLED);
        return true;
    }
    return false;
}

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

void NetFatalError(Ogre::UTFString errormsg)
{
    LOG("[RoR|Networking] NetFatalError(): " + errormsg.asUTF8());

    SetErrorMessage(errormsg);
    m_net_fatal_error = true;

    m_shutdown = true;

    // IMPORTANT: Disconnecting here terminates the application for some reason
    // Workaround: leave the socket in broken state -> impossible to re-connect. Fix later.
    //socket.set_timeout(1, 1000);
    //socket.disconnect();
    m_socket_broken = true; // Flag to enable "Please restart RoR" error dialog.
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

bool SendMessageRaw(char *buffer, int msgsize)
{
    SWBaseSocket::SWBaseError error;

    int rlen = 0;
    while (rlen < msgsize)
    {
        int sendnum = socket.send(buffer + rlen, msgsize - rlen, &error);
        if (sendnum < 0)
        {
            LOG("NET send error: " + error.get_error());
            return false;
        }
        rlen += sendnum;
    }

    return true;
}

bool SendNetMessage(int type, unsigned int streamid, int len, char* content)
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
        return false;
    }

    char buffer[MAX_MESSAGE_LENGTH] = {0};
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

#ifdef DEBUG
	LOG_THREAD("[RoR|Networking] ReceiveMessage() waiting...");
#endif //DEBUG

    int hlen = 0;
    while (hlen < (int)sizeof(header_t))
    {
        int recvnum = socket.recv(buffer + hlen, sizeof(header_t) - hlen, &error);
        if (recvnum < 0 && !m_shutdown)
        {
            LOG("NET receive error 1: " + error.get_error());
            return -1;
        }
        else if (m_shutdown)
        {
            break;
        }
        hlen += recvnum;
    }

    if (m_shutdown)
    {
        return RECVMESSAGE_RETVAL_SHUTDOWN;
    }

    memcpy(head, buffer, sizeof(header_t));

#ifdef DEBUG
    LOG_THREAD("[RoR|Networking] ReceiveMessage() header received");
#endif //DEBUG

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
                LOG_THREAD("NET receive error 2: "+ error.get_error());
                return -1;
            }
            else if (m_shutdown)
            {
                break;
            }
            hlen += recvnum;
        }
    }

    memcpy(content, buffer + sizeof(header_t), bufferlen);

#ifdef DEBUG
    LOG_THREAD("[RoR|Networking] ReceiveMessage() body received");
#endif //DEBUG

    return 0;
}

void SendThread()
{
    LOG("[RoR|Networking] SendThread started");
    while (!m_shutdown)
    {
        std::unique_lock<std::mutex> queue_lock(m_send_packetqueue_mutex);
        while (!m_shutdown && m_send_packet_buffer.empty())
        {
            m_send_packet_available_cv.wait(queue_lock);
        }

        while (!m_shutdown && !m_send_packet_buffer.empty())
        {
            send_packet_t packet = m_send_packet_buffer.front();
            m_send_packet_buffer.pop_front();

            queue_lock.unlock();
            SendMessageRaw(packet.buffer, packet.size);
            queue_lock.lock();
        }
        queue_lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        queue_lock.lock();
    }
    LOG("[RoR|Networking] SendThread stopped");
}

void RecvThread()
{
    LOG_THREAD("[RoR|Networking] RecvThread starting...");

    header_t header;

    char *buffer = (char*)malloc(MAX_MESSAGE_LENGTH);

    while (!m_shutdown)
    {
        int err = ReceiveMessage(&header, buffer, MAX_MESSAGE_LENGTH);
        //LOG("Received data: " + TOSTRING(header.command) + ", source: " + TOSTRING(header.source) + ":" + TOSTRING(header.streamid) + ", size: " + TOSTRING(header.size));
        if (err != 0)
        {
            if (err != RECVMESSAGE_RETVAL_SHUTDOWN)
            {
                std::stringstream s;
                s << "[RoR|Networking] RecvThread: Error while receiving data: " << err << ", tid: " <<std::this_thread::get_id();
                NetFatalError(s.str());
            }
            break;
        }

        if (header.command == MSG2_STREAM_REGISTER)
        {
            if (header.source == m_uid)
                continue;

            stream_register_t *reg = (stream_register_t *)buffer;

            LOG(" * received stream registration: " + TOSTRING(header.source) + ": " + TOSTRING(header.streamid) + ", type: " + TOSTRING(reg->type));
        }
        else if (header.command == MSG2_STREAM_REGISTER_RESULT)
        {
            stream_register_t *reg = (stream_register_t *)buffer;
            LOG(" * received stream registration result: " + TOSTRING(header.source) + ": " + TOSTRING(header.streamid) + ", status: " + TOSTRING(reg->status));
        }
        else if (header.command == MSG2_STREAM_UNREGISTER)
        {
            LOG(" * received stream deregistration: " + TOSTRING(header.source) + ": " + TOSTRING(header.streamid));
        }
        else if (header.command == MSG2_UTF_CHAT || header.command == MSG2_UTF_PRIVCHAT)
        {
            // Chat message
        }
        else if (header.command == MSG2_NETQUALITY && header.source == -1)
        {
            if (header.size != sizeof(int))
            {
                continue;
            }
            int quality = *(int *)buffer;
            SetNetQuality(quality);
            continue;
        }
        else if (header.command == MSG2_USER_LEAVE)
        {
            if (header.source == m_uid)
            {
                NetFatalError(_L("disconnected: remote side closed the connection"));
                return;
            }

            { // Lock scope
                std::lock_guard<std::mutex> lock(m_users_mutex);
                auto user = std::find_if(m_users.begin(), m_users.end(), [header](const user_info_t u) { return static_cast<int>(u.uniqueid) == header.source; });
                if (user != m_users.end())
                {
                    Ogre::UTFString msg = RoR::ChatSystem::GetColouredName(user->username, user->colournum) + RoR::Color::CommandColour + _L(" left the game");
                    const char *utf8_line = msg.asUTF8_c_str();
                    header_t head;
                    head.command = MSG2_UTF_CHAT;
                    head.source  = -1;
                    head.size    = (int)strlen(utf8_line);
                    QueueStreamData(head, (char *)utf8_line);
                    LOG_THREAD(Ogre::UTFString(user->username) + _L(" left the game"));
                    m_users.erase(user);
                }
            }
#ifdef USE_MYGUI
            RoR::App::GetGuiManager()->GetTopMenubar()->triggerUpdateVehicleList();
#endif // USE_MYGUI
        }
        else if (header.command == MSG2_USER_INFO || header.command == MSG2_USER_JOIN)
        {
            if (header.source == m_uid)
            {
                std::lock_guard<std::mutex> lock(m_userdata_mutex);
                memcpy(&m_userdata, buffer, sizeof(user_info_t));
                m_authlevel = m_userdata.authstatus;
                m_username = Ogre::UTFString(m_userdata.username);
                // TODO: Update the global variable 'mp_player_name' in a threadsafe way.
            }
            else
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
#ifdef USE_MYGUI
                RoR::App::GetGuiManager()->GetTopMenubar()->triggerUpdateVehicleList();
#endif // USE_MYGUI
            }
            continue;
        }
        else if (header.command == MSG2_GAME_CMD)
        {
#ifdef USE_ANGELSCRIPT
            ScriptEngine::getSingleton().queueStringForExecution(Ogre::String(buffer));
#endif // USE_ANGELSCRIPT
            continue;
        }
        //DebugPacket("receive-1", &header, buffer);

        QueueStreamData(header, buffer);
    }

    m_recv_stopped = true;

    LOG_THREAD("[RoR|Networking] RecvThread stopped");
}


void ConnectionFailed(Ogre::UTFString const & msg)
{
    Ogre::UTFString message = "Error connecting to server: [" + App::GetMpServerHost() 
                            + ":" + TOSTRING(App::GetMpServerPort()) + "]\n\n" + msg.asUTF8();
    SetErrorMessage(message);
    RoR::App::SetActiveMpState(App::MP_STATE_DISABLED);
    RoR::App::SetPendingMpState(App::MP_STATE_NONE);
}

bool Connect()
{
    // Temporary workaround for unrecoverable error
    if (m_socket_broken)
    {
        RoR::App::SetActiveMpState(App::MP_STATE_DISABLED);
        RoR::App::SetPendingMpState(App::MP_STATE_NONE);
        Ogre::UTFString msg = "Connection failed."
            "\n\nNetworking is unable to recover from previous error (abrupt disconnect)."
            "\n\nPlease restart Rigs of Rods.";
        SetErrorMessage(msg);
        return false;
    }

    // Reset errors
    SetErrorMessage("");
    m_net_fatal_error = false;

    m_username = App::GetMpPlayerName();

    LOG("[RoR|Networking] Trying to join server '" + App::GetMpServerHost() + "' on port " + TOSTRING(App::GetMpServerPort()) + "'...");

    SWBaseSocket::SWBaseError error;

    socket.set_timeout(10, 10000);
    socket.connect(App::GetMpServerPort(), App::GetMpServerHost(), &error);
    if (error != SWBaseSocket::ok)
    {
        ConnectionFailed("Could not create connection");
        return false;
    }
    if (!SendNetMessage(MSG2_HELLO, 0, (int)strlen(RORNET_VERSION), (char *)RORNET_VERSION))
    {
        ConnectionFailed(_L("Establishing network session: error sending hello"));
        return false;
    }

    header_t header;
    char buffer[MAX_MESSAGE_LENGTH] = {0};

    // Receive server (rornet protocol) version
    if (ReceiveMessage(&header, buffer, 255))
    {
        ConnectionFailed(_L("Establishing network session: error getting server version"));
        return false;
    }
    if (header.command == MSG2_WRONG_VER)
    {
        ConnectionFailed(_L("server uses a different protocol version"));
        return false;
    }
    if (header.command != MSG2_HELLO)
    {
        ConnectionFailed(_L("Establishing network session: error getting server hello"));
        return false;
    }

    // Save server settings
    memcpy(&m_server_settings, buffer, sizeof(server_info_t));

    if (strncmp(m_server_settings.protocolversion, RORNET_VERSION, strlen(RORNET_VERSION)))
    {
        wchar_t tmp[512] = L"";
        Ogre::UTFString tmp2 = _L("Establishing network session: wrong server version, you are using version '%s' and the server is using '%s'");
        swprintf(tmp, 512, tmp2.asWStr_c_str(), RORNET_VERSION, m_server_settings.protocolversion);
        ConnectionFailed(MyGUI::UString(tmp).asUTF8_c_str());
        return false;
    }

    // First handshake done, increase the timeout, important!
    socket.set_timeout(0, 0);

    // Send credentials
    char pwbuffer[250] = {0};
    strncpy(pwbuffer, App::GetMpServerPassword().c_str(), 250);

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
    Ogre::String lang = App::GetAppLocale();
    strncpy(c.language, lang.c_str(), std::min<int>((int)lang.size(), 10));
    strcpy(c.sessiontype, "normal");
    if (!SendNetMessage(MSG2_USER_INFO, 0, sizeof(user_info_t), (char*)&c))
    {
        ConnectionFailed(_L("Establishing network session: error sending user info"));
        return false;
    }

    // Getting authorization
    if (ReceiveMessage(&header, buffer, 255))
    {
        ConnectionFailed(_L("Establishing network session: error getting server authorization"));
        return false;
    }

    if (header.command==MSG2_FULL)
    {
        ConnectionFailed(_L("Establishing network session: sorry, server has too many players"));
        return false;
    }
    else if (header.command==MSG2_BANNED)
    {
        wchar_t tmp[512];
        memset(tmp, 0, 512);
        if (strnlen(buffer, 20) > 0)
        {
            buffer[header.size] = {0};
            Ogre::UTFString tmp2 = _L("Establishing network session: sorry, you are banned:\n%s");
            swprintf(tmp, 512, tmp2.asWStr_c_str(), buffer);
            ConnectionFailed(Ogre::UTFString(tmp));
        }
        else
        {
            ConnectionFailed(_L("Establishing network session: sorry, you are banned!"));
        }
        return false;
    }
    else if (header.command==MSG2_WRONG_PW)
    {
        ConnectionFailed(_L("Establishing network session: sorry, wrong password!"));
        return false;
    }
    else if (header.command==MSG2_WRONG_VER)
    {
        ConnectionFailed(_L("Establishing network session: sorry, wrong protocol version!"));
        return false;
    }

    if (header.command!=MSG2_WELCOME)
    {
        ConnectionFailed(_L("Establishing network session: sorry, unknown server response"));
        return false;
    }

    m_uid = header.source;

    // we get our userdata back
    memcpy(&m_userdata, buffer, std::min<int>(sizeof(user_info_t), header.size));

    m_shutdown = false;

    LOG("[RoR|Networking] Connect(): Creating Send/Recv threads");
    m_send_thread = std::thread(SendThread);
    m_recv_thread = std::thread(RecvThread);
    RoR::App::SetActiveMpState(RoR::App::MP_STATE_CONNECTED);
    RoR::App::SetPendingMpState(RoR::App::MP_STATE_NONE);

    return true;
}

void Disconnect()
{
    LOG("[RoR|Networking] Disconnect() disconnecting...");

    m_shutdown = true; // Instruct Send/Recv threads to shut down.
    m_recv_stopped = false;

    m_send_packet_available_cv.notify_one();

    m_send_thread.join();
    LOG("[RoR|Networking] Disconnect() sender thread stopped...");

    while (!m_recv_stopped)
    {
        SendNetMessage(MSG2_USER_LEAVE, 0, 0, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    m_recv_thread.join();
    LOG("[RoR|Networking] Disconnect() receiver thread stopped...");

    socket.set_timeout(1, 1000);
    socket.disconnect();

    m_users.clear();
    m_recv_packet_buffer.clear();
    m_send_packet_buffer.clear();

    m_shutdown = false;
    RoR::App::SetActiveMpState(RoR::App::MP_STATE_DISABLED);
    RoR::App::SetPendingMpState(RoR::App::MP_STATE_NONE);

    LOG("[RoR|Networking] Disconnect() done");
}

void AddPacket(int streamid, int type, int len, char *content)
{
    if (len > MAX_MESSAGE_LENGTH)
    {
        // packet too big, discarded
        return;
    }

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

    { // Lock scope
        std::lock_guard<std::mutex> lock(m_send_packetqueue_mutex);
        if (type == MSG2_STREAM_DATA)
        {
            if (m_send_packet_buffer.size() > m_packet_buffer_size)
            {
                // buffer full, discard unimportant data packets
                return;
            }
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

    { // Lock scope
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
