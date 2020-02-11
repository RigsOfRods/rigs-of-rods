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
#include "Console.h"
#include "ErrorUtils.h"
#include "GUIManager.h"
#include "GUI_TopMenubar.h"
#include "Language.h"
#include "RoRVersion.h"
#include "ScriptEngine.h"
#include "Utils.h"

#include <Ogre.h>
#include <SocketW.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
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
    //Ogre::ColourValue(0.2,            0.0,            0.6),
    //Ogre::ColourValue(0.6,            0.0,            0.6),
    Ogre::ColourValue(0.8,            1.0,            0.0),
    Ogre::ColourValue(1.0,            0.0,            0.0),
    Ogre::ColourValue(0.501960784314, 0.501960784314, 0.501960784314),
    Ogre::ColourValue(0.0,            0.560784313725, 0.0),
    //Ogre::ColourValue(0.0,            0.282352941176, 0.490196078431),
    Ogre::ColourValue(0.701960784314, 0.352941176471, 0.0),
    Ogre::ColourValue(0.701960784314, 0.560784313725, 0.0),
    //Ogre::ColourValue(0.419607843137, 0.0,            0.419607843137),
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

static NetEventQueue     m_event_queue;
static std::string       m_status_message;
static std::atomic<bool> m_shutdown;

static std::mutex m_users_mutex;
static std::mutex m_userdata_mutex;
static std::mutex m_recv_packetqueue_mutex;
static std::mutex m_send_packetqueue_mutex;
static std::mutex m_event_queue_mutex;

static std::condition_variable m_send_packet_available_cv;

static std::vector<recv_packet_t> m_recv_packet_buffer;
static std::deque <send_packet_t> m_send_packet_buffer;

static const unsigned int m_packet_buffer_size = 20;

#define LOG_THREAD(_MSG_) { std::stringstream s; s << _MSG_ << " (Thread ID: " << std::this_thread::get_id() << ")"; LOG(s.str()); }
#define LOGSTREAM         Ogre::LogManager().getSingleton().stream()

bool ConnectThread(); // Declaration.

Ogre::ColourValue GetPlayerColor(int color_num)
{
    int numColours = sizeof(MP_COLORS) / sizeof(Ogre::ColourValue);
    if (color_num < 0 || color_num >= numColours)
        return Ogre::ColourValue::ZERO;

    return MP_COLORS[color_num];
}

void FireNetEvent(NetEvent::Type type, std::string const & message)
{
    std::lock_guard<std::mutex> lock(m_event_queue_mutex);
    m_event_queue.emplace(type, message);
}

void DebugPacket(const char *name, RoRnet::Header *header, char *buffer)
{
    std::stringstream msg;
    msg << "++ " << name << ": " << header->source << ", " << header->streamid
        << ", "<< header->command << ", " << header->size << ", hash: " << HashData(buffer, header->size);
    LOG(msg.str());
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

    if (socket.fsend(buffer, msgsize, &error) < msgsize)
    {
        LOG("NET send error: " + error.get_error());
        return false;
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

#ifdef DEBUG
	LOG_THREAD("[RoR|Networking] ReceiveMessage() waiting...");
#endif //DEBUG

    if (socket.frecv((char*)head, sizeof(RoRnet::Header), &error) < sizeof(RoRnet::Header))
    {
        LOG("NET receive error 1: " + error.get_error());
        return -1;
    }

#ifdef DEBUG
    LOG_THREAD("[RoR|Networking] ReceiveMessage() header received");
#endif //DEBUG

    if (head->size > uint32_t(bufferlen))
    {
        return -3;
    }

    if (head->size > 0)
    {
        // Read the packet content
        std::memset(content, 0, bufferlen);
        if (socket.frecv(content, head->size, &error) < static_cast<int>(head->size))
        {
            LOG_THREAD("NET receive error 2: "+ error.get_error());
            return -1;
        }
    }

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
        send_packet_t packet;
        {
            std::unique_lock<std::mutex> queue_lock(m_send_packetqueue_mutex);
            while (m_send_packet_buffer.empty() && !m_shutdown)
            {
                m_send_packet_available_cv.wait(queue_lock);
            }
            if (m_shutdown)
            {
                break;
            }
            packet = m_send_packet_buffer.front();
            m_send_packet_buffer.pop_front();
        }
        SendMessageRaw(packet.buffer, packet.size);
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
            LOG_THREAD("[RoR|Networking] RecvThread: Error while receiving data: " + TOSTRING(err));
            m_shutdown = true; // Atomic; instruct sender thread to stop
            FireNetEvent(NetEvent::Type::RECV_ERROR, _LC("Network", "Error receiving data from network"));
            continue; // Stop receiving data
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
                m_shutdown = true; // Atomic; stop sending/receiving data - server closes the connection quickly after kick.

                std::stringstream msg;
                msg << _L("disconnected: remote side closed the connection");
                msg << " ** ";
                msg << buffer;
                if (std::strstr(buffer, "disconnected on request") != nullptr) // FIXME: Add a reason code to MSG2_USER_LEAVE, this is ugly!
                    FireNetEvent(NetEvent::Type::USER_DISCONNECT, msg.str());
                else
                    FireNetEvent(NetEvent::Type::SERVER_KICK, msg.str());
            }
            else
            {
                std::lock_guard<std::mutex> lock(m_users_mutex);
                auto user = std::find_if(m_users.begin(), m_users.end(), [header](const RoRnet::UserInfo& u) { return static_cast<int>(u.uniqueid) == header.source; });
                if (user != m_users.end())
                {
                    // Console is now threadsafe, no need to send fake chatmessages to ourselves
                    Str<300> text;
                    text << user->username << _L(" left the game");
                    App::GetConsole()->putMessage( // NOTE: we can't use `putNetMessage()` since the user info gets deleted.
                        Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, text.ToCStr());
                    LOG_THREAD(text);

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
                // NB: Console is threadsafe
                RoRnet::UserInfo user_info;
                if (GetUserInfo(header.source, user_info)) // Check duplicate user ID
                {
                    Str<300> txt;
                    txt << "Duplicate net userID: " << header.source << ", orig. user: '" << user_info.username << "'";
                    App::GetConsole()->putMessage(
                        Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, txt.ToCStr());
                }
                else
                {
                    memcpy(&user_info, buffer, sizeof(RoRnet::UserInfo));
                    Str<300> text;
                    text << user_info.username;
                    if (user_info.authstatus != 0) // Show nothing for guests (no special authorization)
                    {
                        text << " (" << UserAuthToStringShort(user_info) << ")";
                    }
                    text << _L(" joined the game");
                    App::GetConsole()->putNetMessage(
                        user_info.uniqueid, Console::CONSOLE_SYSTEM_NOTICE, text.ToCStr());
                    // Lock and update userlist
                    std::lock_guard<std::mutex> lock(m_users_mutex);
                    m_users.push_back(user_info);
                } // End of lock scope
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


void CouldNotConnect(std::string const & msg, bool close_socket = true)
{
    RoR::LogFormat("[RoR|Networking] Failed to connect to server [%s:%d], message: %s", m_net_host.GetBuffer(), m_net_port, msg.c_str());
    FireNetEvent(NetEvent::Type::CONNECT_FAILURE, msg);

    if (close_socket)
    {
        socket.set_timeout(1, 0);
        socket.disconnect();
    }
}

bool StartConnecting()
{
    // Shadow vars for threaded access
    m_username = App::mp_player_name->GetActiveStr();
    m_token    = App::mp_player_token->GetActiveStr();
    m_net_host = App::mp_server_host->GetActiveStr();
    m_net_port = App::mp_server_port->GetActiveVal<int>();
    m_password = App::mp_server_password->GetActiveStr();

    try
    {
        m_connect_thread = std::thread(ConnectThread);
        App::mp_state->SetActiveVal((int)MpState::CONNECTING); // Mark connect thread as started
        FireNetEvent(NetEvent::Type::CONNECT_STARTED, _LC("Network", "Starting..."));
        return true;
    }
    catch (std::exception& e)
    {
        App::mp_state->SetActiveVal((int)MpState::DISABLED);
        FireNetEvent(NetEvent::Type::CONNECT_FAILURE, _L("Failed to launch connection thread"));
        RoR::LogFormat("[RoR|Networking] Failed to launch connection thread, message: %s", e.what());
        return false;
    }
}

NetEventQueue CheckEvents()
{
    std::lock_guard<std::mutex> lock(m_event_queue_mutex);

    NetEventQueue qcopy;
    while (!m_event_queue.empty())
    {
        switch (m_event_queue.front().type)
        {
        case NetEvent::Type::CONNECT_FAILURE:
        case NetEvent::Type::CONNECT_SUCCESS:
            if (m_connect_thread.joinable())
                m_connect_thread.join(); // Clean up
            break;
        default:;
        }
        qcopy.push(m_event_queue.front());
        m_event_queue.pop();
    }
    return qcopy;
}

bool ConnectThread()
{
    RoR::LogFormat("[RoR|Networking] Trying to join server '%s' on port '%d' ...", m_net_host.GetBuffer(), m_net_port);

    SWBaseSocket::SWBaseError error;

    FireNetEvent(NetEvent::Type::CONNECT_PROGRESS, _LC("Network", "Estabilishing connection..."));
    socket = SWInetSocket();
    socket.set_timeout(10, 0);
    socket.connect(App::mp_server_port->GetActiveVal<int>(), App::mp_server_host->GetActiveStr(), &error);
    if (error != SWBaseSocket::ok)
    {
        CouldNotConnect(_L("Could not create connection"), false);
        return false;
    }

    FireNetEvent(NetEvent::Type::CONNECT_PROGRESS, _LC("Network", "Getting server info..."));
    if (!SendNetMessage(MSG2_HELLO, 0, (int)strlen(RORNET_VERSION), (char *)RORNET_VERSION))
    {
        CouldNotConnect(_L("Establishing network session: error sending hello"));
        return false;
    }

    RoRnet::Header header;
    char buffer[RORNET_MAX_MESSAGE_LENGTH] = {0};

    // Receive server (rornet protocol) version
    if (ReceiveMessage(&header, buffer, RORNET_MAX_MESSAGE_LENGTH))
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

    FireNetEvent(NetEvent::Type::CONNECT_PROGRESS, _LC("Network", "Authorizing..."));

    // First handshake done, increase the timeout, important!
    socket.set_timeout(0, 0);

    // Construct user credentials
    // Beware of the wchar_t converted to UTF8 for networking
    RoRnet::UserInfo c;
    memset(&c, 0, sizeof(RoRnet::UserInfo));
    // Cut off the UTF string on the highest level, otherwise you will break UTF info
    strncpy((char *)c.username, m_username.substr(0, RORNET_MAX_USERNAME_LEN * 0.5f).asUTF8_c_str(), RORNET_MAX_USERNAME_LEN);
    strncpy(c.serverpassword, Utils::Sha1Hash(m_password.GetBuffer()).c_str(), size_t(40));
    strncpy(c.usertoken, Utils::Sha1Hash(m_token.GetBuffer()).c_str(), size_t(40));
    strncpy(c.clientversion, ROR_VERSION_STRING, strnlen(ROR_VERSION_STRING, 25));
    strncpy(c.clientname, "RoR", 10);
    std::string language = App::app_language->GetActiveStr().substr(0, 2);
    std::string country = App::app_country->GetActiveStr().substr(0, 2);
    strncpy(c.language, (language + std::string("_") + country).c_str(), 5);
    strcpy(c.sessiontype, "normal");
    if (!SendNetMessage(MSG2_USER_INFO, 0, sizeof(RoRnet::UserInfo), (char*)&c))
    {
        CouldNotConnect(_L("Establishing network session: error sending user info"));
        return false;
    }

    // Getting authorization
    if (ReceiveMessage(&header, buffer, RORNET_MAX_MESSAGE_LENGTH))
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
        // Do NOT `disconnect()` the socket in this case - causes SocketW to terminate RoR.
        CouldNotConnect(_L("Establishing network session: sorry, you are banned!"), /*close_socket=*/false);
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

    FireNetEvent(NetEvent::Type::CONNECT_PROGRESS, _LC("Network", "Finishing..."));

    m_uid = header.source;

    // we get our userdata back
    memcpy(&m_userdata, buffer, std::min<int>(sizeof(RoRnet::UserInfo), header.size));

    m_shutdown = false;

    LOG("[RoR|Networking] Connect(): Creating Send/Recv threads");
    m_send_thread = std::thread(SendThread);
    m_recv_thread = std::thread(RecvThread);
    FireNetEvent(NetEvent::Type::CONNECT_SUCCESS, "");

    return true;
}

void Disconnect()
{
    LOG("[RoR|Networking] Disconnect() disconnecting...");
    bool is_clean_disconnect = !m_shutdown; // Hacky detection of invalid network state

    m_shutdown = true; // Instruct Send/Recv threads to shut down.

    m_send_packet_available_cv.notify_one();

    m_send_thread.join();
    LOG("[RoR|Networking] Disconnect() sender thread cleaned up");

    socket.set_timeout(1, 0);

    if (is_clean_disconnect)
    {
        SendNetMessage(MSG2_USER_LEAVE, 0, 0, 0);
    }

    m_recv_thread.join();
    LOG("[RoR|Networking] Disconnect() receiver thread cleaned up");

    if (is_clean_disconnect)
    {
        socket.disconnect();
    }
    else
    {
        socket.close_fd();
    }

    m_users.clear();
    m_recv_packet_buffer.clear();
    m_send_packet_buffer.clear();

    m_shutdown = false;
    App::mp_state->SetActiveVal((int)MpState::DISABLED);

    LOG("[RoR|Networking] Disconnect() done");
}

void AddPacket(int streamid, int type, int len, const char *content)
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
    head->command     = type;
    head->source      = m_uid;
    head->size        = len;
    head->streamid    = streamid;

    // then copy the contents
    char *bufferContent = (char *)(buffer + sizeof(RoRnet::Header));
    memcpy(bufferContent, content, len);

    // record the packet size
    packet.size = len + sizeof(RoRnet::Header);

    { // Lock scope
        std::lock_guard<std::mutex> lock(m_send_packetqueue_mutex);
        if (type == MSG2_STREAM_DATA_DISCARDABLE)
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
                // Found outdated discardable streamdata -> replace it
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

bool GetAnyUserInfo(int uid, RoRnet::UserInfo &result)
{
    RoRnet::UserInfo tmp;

    // Try remote users
    if (GetUserInfo(uid, tmp))
    {
        result = tmp;
        return true;
    }

    // Try local user
    tmp = GetLocalUserData();
    if (tmp.uniqueid == uid)
    {
        result = tmp;
        return true;
    }

    return false;
}

bool FindUserInfo(std::string const& username, RoRnet::UserInfo &result)
{
    std::lock_guard<std::mutex> lock(m_users_mutex);
    for (RoRnet::UserInfo user : m_users)
    {
        if (user.username == username)
        {
            result = user;
            return true;
        }
    }
    return false;
}

void BroadcastChatMsg(const char* msg)
{
    Networking::AddPacket(m_stream_id, RoRnet::MSG2_UTF8_CHAT, (int)std::strlen(msg), msg);
}

void WhisperChatMsg(RoRnet::UserInfo const& user, const char* msg)
{
    // Prepare buffer
    char payload[RORNET_MAX_MESSAGE_LENGTH - sizeof(RoRnet::Header)];
    size_t payload_len = 0;

    // Write client ID
    std::memcpy(payload, &user.uniqueid, sizeof(user.uniqueid));
    payload_len += sizeof(user.uniqueid);

    // Write text
    std::strncpy(payload + payload_len, msg, sizeof(payload) - payload_len);
    payload_len += std::strlen(msg);

    // Queue packet
    Networking::AddPacket(m_stream_id, RoRnet::MSG2_UTF8_PRIVCHAT, (int)payload_len, msg);
}

std::string UserAuthToStringShort(RoRnet::UserInfo const &user)
{
    // TODO: currently we must allocate & return std::string, see _LC
         if (user.authstatus & AUTH_ADMIN)    { return _LC("NetUserAuth", "Admin");  }
    else if (user.authstatus & AUTH_MOD)      { return _LC("NetUserAuth", "Mod");    }
    else if (user.authstatus & AUTH_BOT)      { return _LC("NetUserAuth", "Bot");    }
    else if (user.authstatus & AUTH_RANKED)   { return _LC("NetUserAuth", "Ranked"); }
    else if (user.authstatus & AUTH_BANNED)   { return _LC("NetUserAuth", "Banned"); }
    else                                      { return _LC("NetUserAuth", "Guest");  }
}

std::string UserAuthToStringLong(RoRnet::UserInfo const &user)
{
    // TODO: currently we must allocate & return std::string, see _L
         if (user.authstatus & AUTH_ADMIN)    { return _L("Server Administrator");   }
    else if (user.authstatus & AUTH_MOD)      { return _L("Server Moderator");       }
    else if (user.authstatus & AUTH_BOT)      { return _L("Bot");                    }
    else if (user.authstatus & AUTH_RANKED)   { return _L("ranked user");            }
    else if (user.authstatus & AUTH_BANNED)   { return _L("Banned user");            }
    else                                      { return _L("Guest");                  }
}

} // namespace Networking
} // namespace RoR

#endif // USE_SOCKETW
