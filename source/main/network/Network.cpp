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
#include "ChatSystem.h"
#include "ErrorUtils.h"
#include "GUIManager.h"
#include "GUI_TopMenubar.h"
#include "Language.h"
#include "RoRVersion.h"
#include "SHA1.h"
#include "ScriptEngine.h"
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

static Ogre::ColourValue MP_COLORS[] = // Classic RoR multiplayer colors
{
    Ogre::ColourValue(0.0,            0.8,            0.0),
    Ogre::ColourValue(0.0,            0.4,            0.701960784314),
    Ogre::ColourValue(1.0,            0.501960784314, 0.0),
    Ogre::ColourValue(1.0,            0.8,            0.0),
    Ogre::ColourValue(0.2,            0.0,            0.6),
    Ogre::ColourValue(0.6,            0.0,            0.6),
    Ogre::ColourValue(0.8,            1.0,            0.0),
    Ogre::ColourValue(1.0,            0.0,            0.0),
    Ogre::ColourValue(0.501960784314, 0.501960784314, 0.501960784314),
    Ogre::ColourValue(0.0,            0.560784313725, 0.0),
    Ogre::ColourValue(0.0,            0.282352941176, 0.490196078431),
    Ogre::ColourValue(0.701960784314, 0.352941176471, 0.0),
    Ogre::ColourValue(0.701960784314, 0.560784313725, 0.0),
    Ogre::ColourValue(0.419607843137, 0.0,            0.419607843137),
    Ogre::ColourValue(0.560784313725, 0.701960784314, 0.0),
    Ogre::ColourValue(0.701960784314, 0.0,            0.0),
    Ogre::ColourValue(0.745098039216, 0.745098039216, 0.745098039216),
    Ogre::ColourValue(0.501960784314, 1.0,            0.501960784314),
    Ogre::ColourValue(0.501960784314, 0.788235294118, 1.0),
    Ogre::ColourValue(1.0,            0.752941176471, 0.501960784314),
    Ogre::ColourValue(1.0,            0.901960784314, 0.501960784314),
    Ogre::ColourValue(0.666666666667, 0.501960784314, 1.0),
    Ogre::ColourValue(0.933333333333, 0.0,            0.8),
    Ogre::ColourValue(1.0,            0.501960784314, 0.501960784314),
    Ogre::ColourValue(0.4,            0.4,            0.0),
    Ogre::ColourValue(1.0,            0.749019607843, 1.0),
    Ogre::ColourValue(0.0,            1.0,            0.8),
    Ogre::ColourValue(0.8,            0.4,            0.6),
    Ogre::ColourValue(0.6,            0.6,            0.0),
};

using namespace RoRnet;

struct send_packet_t
{
    char buffer[RORNET_MAX_MESSAGE_LENGTH];
    int size;
};

static RoRnet::ServerInfo m_server_settings;

static Ogre::UTFString m_username; // Shadows GVar 'mp_player_name' for multithreaded access.
static Str<400> m_net_host; // Shadows GVar 'mp_server_host' for multithreaded access.
static Str<100> m_password; // Shadows GVar 'mp_server_password' for multithreaded access.
static Str<100> m_token; // Shadows GVar 'mp_player_token' for multithreaded access.
static int      m_net_port; // Shadows GVar 'mp_server_port' for multithreaded access.
static int m_uid;
static int m_authlevel;
static RoRnet::UserInfo m_userdata;

static int m_stream_id = 10;

static std::atomic<int> m_net_quality;

static std::vector<RoRnet::UserInfo> m_users;

static SWInetSocket socket;

static std::thread m_send_thread;
static std::thread m_recv_thread;
static std::thread m_connect_thread;

static std::atomic<ConnectState> m_connecting_status(ConnectState::IDLE);
static std::atomic<bool> m_shutdown;

static std::mutex m_users_mutex;
static std::mutex m_userdata_mutex;
static std::mutex m_error_message_mutex;
static std::mutex m_status_message_mutex;
static std::mutex m_recv_packetqueue_mutex;
static std::mutex m_send_packetqueue_mutex;

static std::condition_variable m_send_packet_available_cv;

static std::vector<recv_packet_t> m_recv_packet_buffer;
static std::deque <send_packet_t> m_send_packet_buffer;

static const unsigned int m_packet_buffer_size = 20;

static std::atomic<bool> m_net_fatal_error;
static std::atomic<bool> m_socket_broken;
static Ogre::UTFString   m_error_message;
static StatusStr         m_status_message;

#define LOG_THREAD(_MSG_) { std::stringstream s; s << _MSG_ << " (Thread ID: " << std::this_thread::get_id() << ")"; LOG(s.str()); }
#define LOGSTREAM         Ogre::LogManager().getSingleton().stream()

static const int RECVMESSAGE_RETVAL_SHUTDOWN = -43;

bool ConnectThread(); // Declaration.

Ogre::ColourValue GetPlayerColor(int color_num)
{
    int numColours = sizeof(MP_COLORS) / sizeof(Ogre::ColourValue);
    if (color_num < 0 || color_num >= numColours)
        return Ogre::ColourValue::ZERO;

    return MP_COLORS[color_num];
}

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

StatusStr GetStatusMessage()
{
    std::lock_guard<std::mutex> lock(m_status_message_mutex);
    return m_status_message;
}

void SetStatusMessage(const char* msg)
{
    std::lock_guard<std::mutex> lock(m_status_message_mutex);
    m_status_message = msg;
}

bool CheckError()
{
    if (m_net_fatal_error)
    {
        App::mp_state.SetActive(RoR::MpState::DISABLED);
        return true;
    }
    return false;
}

void DebugPacket(const char *name, RoRnet::Header *header, char *buffer)
{
    std::stringstream msg;
    msg << "++ " << name << ": " << header->source << ", " << header->streamid
        << ", "<< header->command << ", " << header->size << ", hash: " << HashData(buffer, header->size);
    LOG(msg.str());
}

void NetFatalError(Ogre::UTFString errormsg)
{
    LOG("[RoR|Networking] NetFatalError(): " + errormsg.asUTF8());

    SetErrorMessage(errormsg);
    m_net_fatal_error = true;

    m_shutdown = true;

    // IMPORTANT: Disconnecting here terminates the application for some reason
    // Workaround: leave the socket in broken state -> impossible to re-connect. Fix later.
    //socket.set_timeout(1, 0);
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
    RoRnet::Header head;
    memset(&head, 0, sizeof(RoRnet::Header));
    head.command = type;
    head.source = m_uid;
    head.size = len;
    head.streamid = streamid;

    const int msgsize = sizeof(RoRnet::Header) + len;

    if (msgsize >= RORNET_MAX_MESSAGE_LENGTH)
    {
        return false;
    }

    char buffer[RORNET_MAX_MESSAGE_LENGTH] = {0};
    memcpy(buffer, (char *)&head, sizeof(RoRnet::Header));
    memcpy(buffer + sizeof(RoRnet::Header), content, len);

    return SendMessageRaw(buffer, msgsize);
}

void QueueStreamData(RoRnet::Header &header, char *buffer, size_t buffer_len)
{
    recv_packet_t packet;
    packet.header = header;
    memcpy(packet.buffer, buffer, std::min(buffer_len, size_t(RORNET_MAX_MESSAGE_LENGTH)));

    std::lock_guard<std::mutex> lock(m_recv_packetqueue_mutex);
    m_recv_packet_buffer.push_back(packet);
}

int ReceiveMessage(RoRnet::Header *head, char* content, int bufferlen)
{
    SWBaseSocket::SWBaseError error;

    char buffer[RORNET_MAX_MESSAGE_LENGTH] = {0};

#ifdef DEBUG
	LOG_THREAD("[RoR|Networking] ReceiveMessage() waiting...");
#endif //DEBUG

    int hlen = 0;
    while (hlen < (int)sizeof(RoRnet::Header))
    {
        int recvnum = socket.recv(buffer + hlen, sizeof(RoRnet::Header) - hlen, &error);
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

    memcpy(head, buffer, sizeof(RoRnet::Header));

#ifdef DEBUG
    LOG_THREAD("[RoR|Networking] ReceiveMessage() header received");
#endif //DEBUG

    if (head->size >= RORNET_MAX_MESSAGE_LENGTH)
    {
        return -3;
    }

    if (head->size > 0)
    {
        // Read the packet content
        while (hlen < (int)sizeof(RoRnet::Header) + (int)head->size)
        {
            int recvnum = socket.recv(buffer + hlen, (head->size + sizeof(RoRnet::Header)) - hlen, &error);
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

    memcpy(content, buffer + sizeof(RoRnet::Header), bufferlen);

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

    RoRnet::Header header;

    char buffer[RORNET_MAX_MESSAGE_LENGTH] = {0};

    while (!m_shutdown)
    {
        int err = ReceiveMessage(&header, buffer, RORNET_MAX_MESSAGE_LENGTH);
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

            RoRnet::StreamRegister *reg = (RoRnet::StreamRegister *)buffer;

            LOG(" * received stream registration: " + TOSTRING(header.source) + ": " + TOSTRING(header.streamid) + ", type: " + TOSTRING(reg->type));
        }
        else if (header.command == MSG2_STREAM_REGISTER_RESULT)
        {
            RoRnet::StreamRegister *reg = (RoRnet::StreamRegister *)buffer;
            LOG(" * received stream registration result: " + TOSTRING(header.source) + ": " + TOSTRING(header.streamid) + ", status: " + TOSTRING(reg->status));
        }
        else if (header.command == MSG2_STREAM_UNREGISTER)
        {
            LOG(" * received stream deregistration: " + TOSTRING(header.source) + ": " + TOSTRING(header.streamid));
        }
        else if (header.command == MSG2_UTF8_CHAT || header.command == MSG2_UTF8_PRIVCHAT)
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
                auto user = std::find_if(m_users.begin(), m_users.end(), [header](const RoRnet::UserInfo& u) { return static_cast<int>(u.uniqueid) == header.source; });
                if (user != m_users.end())
                {
                    Ogre::UTFString msg = RoR::ChatSystem::GetColouredName(user->username, user->colournum) + RoR::Color::CommandColour + _L(" left the game");
                    const char *utf8_line = msg.asUTF8_c_str();
                    RoRnet::Header head;
                    head.command = MSG2_UTF8_CHAT;
                    head.source  = -1;
                    head.size    = (int)strlen(utf8_line);
                    QueueStreamData(head, (char *)utf8_line, strlen(utf8_line) + 1);
                    LOG_THREAD(Ogre::UTFString(user->username) + _L(" left the game"));
                    m_users.erase(user);
                }
            }
        }
        else if (header.command == MSG2_USER_INFO || header.command == MSG2_USER_JOIN)
        {
            if (header.source == m_uid)
            {
                std::lock_guard<std::mutex> lock(m_userdata_mutex);
                memcpy(&m_userdata, buffer, sizeof(RoRnet::UserInfo));
                m_authlevel = m_userdata.authstatus;
                m_username = Ogre::UTFString(m_userdata.username);
                // TODO: Update the global variable 'mp_player_name' in a threadsafe way.
            }
            else
            {
                RoRnet::UserInfo user_info;
                memcpy(&user_info, buffer, sizeof(RoRnet::UserInfo));

                bool user_exists = false;
                {
                    std::lock_guard<std::mutex> lock(m_users_mutex);
                    for (RoRnet::UserInfo &user : m_users)
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
                        RoRnet::Header head;
                        head.command = MSG2_UTF8_CHAT;
                        head.source  = -1;
                        head.size    = (int)strlen(utf8_line);
                        QueueStreamData(head, (char *)utf8_line, strlen(utf8_line) + 1);
                        LOG(Ogre::UTFString(user_info.username) + _L(" joined the game"));
                    }
                }
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
        //DebugPacket("recv", &header, buffer);

        QueueStreamData(header, buffer, RORNET_MAX_MESSAGE_LENGTH);
    }

    LOG_THREAD("[RoR|Networking] RecvThread stopped");
}


void CouldNotConnect(Ogre::UTFString const & msg, bool close_socket = true)
{
    RoR::LogFormat("[RoR|Networking] Failed to connect to server [%s:%d], message: %s", m_net_host.GetBuffer(), m_net_port, msg.asUTF8_c_str());
    SetErrorMessage(msg);
    m_connecting_status = ConnectState::FAILURE;

    if (close_socket)
    {
        socket.set_timeout(1, 0);
        socket.disconnect();
    }
}

bool StartConnecting()
{
    SetStatusMessage("Starting...");

    // Temporary workaround for unrecoverable error
    if (m_socket_broken)
    {
        App::mp_state.SetActive(MpState::DISABLED);
        Ogre::UTFString msg = "Connection failed."
            "\n\nNetworking is unable to recover from previous error (abrupt disconnect)."
            "\n\nPlease restart Rigs of Rods.";
        SetErrorMessage(msg);
        m_connecting_status = ConnectState::FAILURE;
        return false;
    }

    // Reset errors
    SetErrorMessage("");
    m_net_fatal_error = false;

    // Shadow vars for threaded access
    m_username = App::mp_player_name.GetActive();
    m_token    = App::mp_player_token.GetActive();
    m_net_host = App::mp_server_host.GetActive();
    m_net_port = App::mp_server_port.GetActive();
    m_password = App::mp_server_password.GetActive();

    try
    {
        m_connect_thread = std::thread(ConnectThread);
        m_connecting_status = ConnectState::WORKING;
        return true;
    }
    catch (std::exception& e)
    {
        App::mp_state.SetActive(MpState::DISABLED);
        m_connecting_status = ConnectState::FAILURE;
        RoR::LogFormat("[RoR|Networking] Failed to launch connection thread, message: %s", e.what());
        SetErrorMessage(_L("Failed to launch connection thread"));
        SetStatusMessage("Failed");
        return false;
    }
}

ConnectState CheckConnectingState()
{
    switch (m_connecting_status)
    {
    case ConnectState::SUCCESS:
        if (m_connect_thread.joinable())
            m_connect_thread.join(); // Clean up
        m_connecting_status = ConnectState::IDLE;
        return ConnectState::SUCCESS;

    case ConnectState::FAILURE:
        if (m_connect_thread.joinable())
            m_connect_thread.join(); // Clean up
        m_connecting_status = ConnectState::IDLE;
        return ConnectState::FAILURE;

    default:
        return m_connecting_status;
    }
}

bool ConnectThread()
{
    RoR::LogFormat("[RoR|Networking] Trying to join server '%s' on port '%d' ...", m_net_host.GetBuffer(), m_net_port);

    SWBaseSocket::SWBaseError error;

    SetStatusMessage("Estabilishing connection...");
    socket.set_timeout(10, 0);
    socket.connect(App::mp_server_port.GetActive(), App::mp_server_host.GetActive(), &error);
    if (error != SWBaseSocket::ok)
    {
        CouldNotConnect(_L("Could not create connection"), false);
        return false;
    }

    SetStatusMessage("Getting server info...");
    if (!SendNetMessage(MSG2_HELLO, 0, (int)strlen(RORNET_VERSION), (char *)RORNET_VERSION))
    {
        CouldNotConnect(_L("Establishing network session: error sending hello"));
        return false;
    }

    RoRnet::Header header;
    char buffer[RORNET_MAX_MESSAGE_LENGTH] = {0};

    // Receive server (rornet protocol) version
    if (ReceiveMessage(&header, buffer, 255))
    {
        CouldNotConnect(_L("Establishing network session: error getting server version"));
        return false;
    }
    if (header.command == MSG2_WRONG_VER_LEGACY) // Pre-RoRnet_2.40 server
    {
        RoRnet::LegacyServerInfo info;
        memcpy(&info, buffer, sizeof(RoRnet::LegacyServerInfo));
        Ogre::UTFString format_wstr = _L("Establishing network session: wrong server version, you are using version '%s' and the server is using '%s'");
        const char* server_ver = (info.protocolversion[0] != 0) ? info.protocolversion : "~ RoRnet_2.38 or earlier (not detected) ~";
        char msg_buf[500];
        snprintf(msg_buf, 500, format_wstr.asUTF8_c_str(), RORNET_VERSION, server_ver);
        CouldNotConnect(msg_buf);
        return false;
    }
    if (header.command == MSG2_WRONG_VER)
    {
        CouldNotConnect(_L("server uses a different protocol version"));
        return false;
    }
    if (header.command != MSG2_HELLO)
    {
        CouldNotConnect(_L("Establishing network session: error getting server hello"));
        return false;
    }

    // Save server settings
    memcpy(&m_server_settings, buffer, sizeof(RoRnet::ServerInfo));

    if (strncmp(m_server_settings.protocolversion, RORNET_VERSION, strlen(RORNET_VERSION)))
    {
        wchar_t tmp[512] = L"";
        Ogre::UTFString tmp2 = _L("Establishing network session: wrong server version, you are using version '%s' and the server is using '%s'");
        swprintf(tmp, 512, tmp2.asWStr_c_str(), RORNET_VERSION, m_server_settings.protocolversion);
        CouldNotConnect(MyGUI::UString(tmp).asUTF8_c_str());
        return false;
    }

    SetStatusMessage("Authorizing...");

    // First handshake done, increase the timeout, important!
    socket.set_timeout(0, 0);

    // Send credentials
    char sha1pwresult[250] = {0};
    if (!m_password.IsEmpty())
    {
        RoR::CSHA1 sha1;
        sha1.UpdateHash((uint8_t*)m_password.GetBuffer(), static_cast<uint32_t>(m_password.GetLength()));
        sha1.Final();
        sha1.ReportHash(sha1pwresult, RoR::CSHA1::REPORT_HEX_SHORT);
    }

    // Construct user credentials
    // Beware of the wchar_t converted to UTF8 for networking
    RoRnet::UserInfo c;
    memset(&c, 0, sizeof(RoRnet::UserInfo));
    // Cut off the UTF string on the highest level, otherwise you will break UTF info
    strncpy((char *)c.username, m_username.substr(0, RORNET_MAX_USERNAME_LEN * 0.5f).asUTF8_c_str(), RORNET_MAX_USERNAME_LEN);
    strncpy(c.serverpassword, sha1pwresult, 40);
    {
        RoR::CSHA1 sha1;
        uint8_t* input_text = (uint8_t*)m_token.GetBuffer();
        uint32_t input_size = (uint32_t)m_token.GetLength();
        sha1.UpdateHash(input_text, input_size);
        sha1.Final();
        Str<100> hash;
        sha1.ReportHash(hash.GetBuffer(), RoR::CSHA1::REPORT_HEX_SHORT);
        strncpy(c.usertoken, hash.ToCStr(), 40);
    }
    strncpy(c.clientversion, ROR_VERSION_STRING, strnlen(ROR_VERSION_STRING, 25));
    strncpy(c.clientname, "RoR", 10);
    std::string language = std::string(App::app_language.GetActive()).substr(0, 2);
    std::string country = std::string(App::app_country.GetActive()).substr(0, 2);
    strncpy(c.language, (language + std::string("_") + country).c_str(), 5);
    strcpy(c.sessiontype, "normal");
    if (!SendNetMessage(MSG2_USER_INFO, 0, sizeof(RoRnet::UserInfo), (char*)&c))
    {
        CouldNotConnect(_L("Establishing network session: error sending user info"));
        return false;
    }

    // Getting authorization
    if (ReceiveMessage(&header, buffer, 255))
    {
        CouldNotConnect(_L("Establishing network session: error getting server authorization"));
        return false;
    }

    if (header.command==MSG2_FULL)
    {
        CouldNotConnect(_L("Establishing network session: sorry, server has too many players"));
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
            CouldNotConnect(Ogre::UTFString(tmp));
        }
        else
        {
            CouldNotConnect(_L("Establishing network session: sorry, you are banned!"));
        }
        return false;
    }
    else if (header.command==MSG2_WRONG_PW)
    {
        CouldNotConnect(_L("Establishing network session: sorry, wrong password!"));
        return false;
    }
    else if (header.command==MSG2_WRONG_VER)
    {
        CouldNotConnect(_L("Establishing network session: sorry, wrong protocol version!"));
        return false;
    }

    if (header.command!=MSG2_WELCOME)
    {
        CouldNotConnect(_L("Establishing network session: sorry, unknown server response"));
        return false;
    }

    SetStatusMessage("Finishing...");

    m_uid = header.source;

    // we get our userdata back
    memcpy(&m_userdata, buffer, std::min<int>(sizeof(RoRnet::UserInfo), header.size));

    m_shutdown = false;

    LOG("[RoR|Networking] Connect(): Creating Send/Recv threads");
    m_send_thread = std::thread(SendThread);
    m_recv_thread = std::thread(RecvThread);
    m_connecting_status = ConnectState::SUCCESS;

    return true;
}

void Disconnect()
{
    LOG("[RoR|Networking] Disconnect() disconnecting...");

    m_shutdown = true; // Instruct Send/Recv threads to shut down.

    m_send_packet_available_cv.notify_one();

    m_send_thread.join();
    LOG("[RoR|Networking] Disconnect() sender thread stopped...");

    socket.set_timeout(1, 0);

    SendNetMessage(MSG2_USER_LEAVE, 0, 0, 0);

    m_recv_thread.join();
    LOG("[RoR|Networking] Disconnect() receiver thread stopped...");

    socket.disconnect();

    m_users.clear();
    m_recv_packet_buffer.clear();
    m_send_packet_buffer.clear();

    m_shutdown = false;
    App::mp_state.SetActive(RoR::MpState::DISABLED);

    LOG("[RoR|Networking] Disconnect() done");
}

void AddPacket(int streamid, int type, int len, char *content)
{
    const auto max_len = RORNET_MAX_MESSAGE_LENGTH - sizeof(RoRnet::Header);
    if (len > max_len)
    {
        LOGSTREAM << "[RoR|Networking] Discarding network packet (StreamID: "
            <<streamid<<", Type: "<<type<<"), length is " << len << ", max is " << max_len;
        return;
    }

    send_packet_t packet;
    memset(&packet, 0, sizeof(send_packet_t));

    char *buffer = (char*)(packet.buffer);

    RoRnet::Header *head = (RoRnet::Header *)buffer;
    head->command  = type;
    head->source   = m_uid;
    head->size     = len;
    head->streamid = streamid;

    // then copy the contents
    char *bufferContent = (char *)(buffer + sizeof(RoRnet::Header));
    memcpy(bufferContent, content, len);

    // record the packet size
    packet.size = len + sizeof(RoRnet::Header);

    { // Lock scope
        std::lock_guard<std::mutex> lock(m_send_packetqueue_mutex);
        if (type == MSG2_STREAM_DATA)
        {
            if (m_send_packet_buffer.size() > m_packet_buffer_size)
            {
                // buffer full, discard unimportant data packets
                return;
            }
            auto search = std::find_if(m_send_packet_buffer.begin(), m_send_packet_buffer.end(),
                    [&](const send_packet_t& p) { return !memcmp(packet.buffer, p.buffer, sizeof(RoRnet::Header)); });
            if (search != m_send_packet_buffer.end())
            {
                // Found an older packet with the same header -> replace it
                (*search) = packet;
                m_send_packet_available_cv.notify_one();
                return;
            }
        }
        //DebugPacket("send", head, buffer);
        m_send_packet_buffer.push_back(packet);
    }

    m_send_packet_available_cv.notify_one();
}

void AddLocalStream(RoRnet::StreamRegister *reg, int size)
{
    reg->origin_sourceid = m_uid;
    reg->origin_streamid = m_stream_id;
    reg->status = 0;

    AddPacket(m_stream_id, MSG2_STREAM_REGISTER, size, (char*)reg);
    LOG("adding local stream: " + TOSTRING(m_uid) + ":"+ TOSTRING(m_stream_id) + ", type: " + TOSTRING(reg->type));

    m_stream_id++;
}

std::vector<recv_packet_t> GetIncomingStreamData()
{
    std::lock_guard<std::mutex> lock(m_recv_packetqueue_mutex);
    std::vector<recv_packet_t> buf_copy = m_recv_packet_buffer;
    m_recv_packet_buffer.clear();
    return buf_copy;
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

RoRnet::UserInfo GetLocalUserData()
{
    std::lock_guard<std::mutex> lock(m_userdata_mutex);
    return m_userdata;
}

std::vector<RoRnet::UserInfo> GetUserInfos()
{
    std::lock_guard<std::mutex> lock(m_users_mutex);
    return m_users;
}

bool GetUserInfo(int uid, RoRnet::UserInfo &result)
{
    std::lock_guard<std::mutex> lock(m_users_mutex);
    for (RoRnet::UserInfo user : m_users)
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
