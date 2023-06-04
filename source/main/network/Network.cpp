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
#include "GameContext.h"
#include "GUIManager.h"
#include "GUI_TopMenubar.h"
#include "Language.h"
#include "RoRVersion.h"
#include "ScriptEngine.h"
#include "Utils.h"

#include <Ogre.h>
#include <SocketW.h>

#include <algorithm>
#include <chrono>
#include <cstring>

using namespace RoR;

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

static const unsigned int m_packet_buffer_size = 20;

#define LOG_THREAD(_MSG_) { std::stringstream s; s << _MSG_ << " (Thread ID: " << std::this_thread::get_id() << ")"; LOG(s.str()); }
#define LOGSTREAM         Ogre::LogManager().getSingleton().stream()

void Network::PushNetMessage(MsgType type, std::string const & message)
{
    Message m(type);
    m.description = message;
    App::GetGameContext()->PushMessage(m);
}

Ogre::ColourValue Network::GetPlayerColor(int color_num)
{
    int numColours = sizeof(MP_COLORS) / sizeof(Ogre::ColourValue);
    if (color_num < 0 || color_num >= numColours)
        return Ogre::ColourValue::White;

    return MP_COLORS[color_num];
}

// Internal helper
void DebugPacket(const char *name, RoRnet::Header *header, char *buffer)
{
    std::stringstream msg;
    msg << "++ " << name << ": " << header->source << ", " << header->streamid
        << ", "<< header->command << ", " << header->size << ", hash: " << HashData(buffer, header->size);
    LOG(msg.str());
}

void Network::SetNetQuality(int quality)
{
    m_net_quality = quality;
}

int Network::GetNetQuality()
{
    return m_net_quality;
}

int Network::GetUID()
{
    return m_uid;
}

bool Network::SendMessageRaw(char *buffer, int msgsize)
{
    SWBaseSocket::SWBaseError error;

    if (m_socket.fsend(buffer, msgsize, &error) < msgsize)
    {
        LOG("NET send error: " + error.get_error());
        return false;
    }

    return true;
}

bool Network::SendNetMessage(int type, unsigned int streamid, int len, char* content)
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

void Network::QueueStreamData(RoRnet::Header &header, char *buffer, size_t buffer_len)
{
    NetRecvPacket packet;
    packet.header = header;
    memcpy(packet.buffer, buffer, std::min(buffer_len, size_t(RORNET_MAX_MESSAGE_LENGTH)));

    std::lock_guard<std::mutex> lock(m_recv_packetqueue_mutex);
    m_recv_packet_buffer.push_back(packet);
}

int Network::ReceiveMessage(RoRnet::Header *head, char* content, int bufferlen)
{
    SWBaseSocket::SWBaseError error;

#ifdef DEBUG
	LOG_THREAD("[RoR|Networking] ReceiveMessage() waiting...");
#endif //DEBUG

    if (m_socket.frecv((char*)head, sizeof(RoRnet::Header), &error) < sizeof(RoRnet::Header))
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
        if (m_socket.frecv(content, head->size, &error) < static_cast<int>(head->size))
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

void Network::SendThread()
{
    LOG("[RoR|Networking] SendThread started");
    while (!m_shutdown)
    {
        NetSendPacket packet;
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

void Network::RecvThread()
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
            PushNetMessage(MSG_NET_RECV_ERROR, _LC("Network", "Error receiving data from network"));
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

                bool was_kick = (std::strstr(buffer, "disconnected on request") == nullptr); // FIXME: Add a reason code to MSG2_USER_LEAVE, this is ugly!
                PushNetMessage((was_kick) ? MSG_NET_SERVER_KICK : MSG_NET_USER_DISCONNECT, msg.str());
                App::GetGameContext()->PushMessage(Message(MSG_GUI_MP_CLIENTS_REFRESH));

                Message m((was_kick) ? MSG_NET_USER_DISCONNECT : MSG_NET_SERVER_KICK);
            }
            else
            {
                std::lock_guard<std::mutex> lock(m_users_mutex);
                auto user = std::find_if(m_users.begin(), m_users.end(), [header](const RoRnet::UserInfo& u) { return static_cast<int>(u.uniqueid) == header.source; });
                if (user != m_users.end())
                {
                    // Console is now threadsafe, no need to send fake chatmessages to ourselves
                    Str<300> text;
                    text << _L("left the game");
                    App::GetConsole()->putNetMessage(user->uniqueid, Console::CONSOLE_SYSTEM_NOTICE, text.ToCStr());
                    App::GetGameContext()->PushMessage(Message(MSG_GUI_MP_CLIENTS_REFRESH));
                    LOG_THREAD(text);

                    m_disconnected_users.push_back(*user); // Copy
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
                App::GetGameContext()->PushMessage(Message(MSG_GUI_MP_CLIENTS_REFRESH));
                // TODO: Update the global variable 'mp_player_name' in a threadsafe way.
            }
            else
            {
                RoRnet::UserInfo user_info;
                if (!GetUserInfo(header.source, user_info)) // Check that user doesn't exist yet.
                {
                    memcpy(&user_info, buffer, sizeof(RoRnet::UserInfo));
                    Str<300> text;
                    if (user_info.authstatus != 0) // Show nothing for guests (no special authorization)
                    {
                        text << "(" << UserAuthToStringShort(user_info) << ") ";
                    }
                    text << _L("joined the game");
                    
                    // NB: Console is threadsafe
                    App::GetConsole()->putNetMessage(
                        user_info.uniqueid, Console::CONSOLE_SYSTEM_NOTICE, text.ToCStr());
                    App::GetGameContext()->PushMessage(Message(MSG_GUI_MP_CLIENTS_REFRESH));
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
            App::GetScriptEngine()->queueStringForExecution(Ogre::String(buffer));
#endif // USE_ANGELSCRIPT
            continue;
        }
        //DebugPacket("recv", &header, buffer);

        QueueStreamData(header, buffer, RORNET_MAX_MESSAGE_LENGTH);
    }

    LOG_THREAD("[RoR|Networking] RecvThread stopped");
}


void Network::CouldNotConnect(std::string const & msg, bool close_socket /*= true*/)
{
    RoR::LogFormat("[RoR|Networking] Failed to connect to server [%s:%d], message: %s", m_net_host.c_str(), m_net_port, msg.c_str());
    PushNetMessage(MSG_NET_CONNECT_FAILURE, msg);

    if (close_socket)
    {
        m_socket.set_timeout(1, 0);
        m_socket.disconnect();
    }
}

bool Network::StartConnecting()
{
    // Shadow vars for threaded access
    m_username = App::mp_player_name->getStr();
    m_token    = App::mp_player_token->getStr();
    m_net_host = App::mp_server_host->getStr();
    m_net_port = App::mp_server_port->getInt();
    m_password = App::mp_server_password->getStr();

    try
    {
        m_connect_thread = std::thread(&Network::ConnectThread, this);
        App::mp_state->setVal((int)MpState::CONNECTING); // Mark connect thread as started
        PushNetMessage(MSG_NET_CONNECT_STARTED, _LC("Network", "Starting..."));
        return true;
    }
    catch (std::exception& e)
    {
        App::mp_state->setVal((int)MpState::DISABLED);
        PushNetMessage(MSG_NET_CONNECT_FAILURE, _L("Failed to launch connection thread"));
        RoR::LogFormat("[RoR|Networking] Failed to launch connection thread, message: %s", e.what());
        return false;
    }
}

void Network::StopConnecting()
{
    if (m_connect_thread.joinable())
        m_connect_thread.join(); // Clean up
}

bool Network::ConnectThread()
{
    RoR::LogFormat("[RoR|Networking] Trying to join server '%s' on port '%d' ...", m_net_host.c_str(), m_net_port);

    SWBaseSocket::SWBaseError error;

    PushNetMessage(MSG_NET_CONNECT_PROGRESS, _LC("Network", "Estabilishing connection..."));
    m_socket = SWInetSocket();
    m_socket.set_timeout(10, 0);
    m_socket.connect(m_net_port, m_net_host, &error);
    if (error != SWBaseSocket::ok)
    {
        CouldNotConnect(_L("Could not create connection"), false);
        return false;
    }

    PushNetMessage(MSG_NET_CONNECT_PROGRESS, _LC("Network", "Getting server info..."));
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

    PushNetMessage(MSG_NET_CONNECT_PROGRESS, _LC("Network", "Authorizing..."));

    // First handshake done, increase the timeout, important!
    m_socket.set_timeout(0, 0);

    // Construct user credentials
    // Beware of the wchar_t converted to UTF8 for networking
    RoRnet::UserInfo c;
    memset(&c, 0, sizeof(RoRnet::UserInfo));
    // Cut off the UTF string on the highest level, otherwise you will break UTF info
    strncpy((char *)c.username, m_username.substr(0, RORNET_MAX_USERNAME_LEN * 0.5f).asUTF8_c_str(), RORNET_MAX_USERNAME_LEN);
    strncpy(c.serverpassword, Sha1Hash(m_password).c_str(), size_t(40));
    strncpy(c.usertoken, Sha1Hash(m_token).c_str(), size_t(40));
    strncpy(c.clientversion, ROR_VERSION_STRING, strnlen(ROR_VERSION_STRING, 25));
    strncpy(c.clientname, "RoR", 10);
    std::string language = App::app_language->getStr().substr(0, 2);
    std::string country = App::app_country->getStr().substr(0, 2);
    strncpy(c.language, (language + std::string("_") + country).c_str(), 5);
    strcpy(c.sessiontype, "normal");
    // determine character to use
    if (App::mp_override_character->getStr() != "")
    {
        strncpy(c.characterfile, App::mp_override_character->getStr().c_str(), RORNET_MAX_CHARACTERFILE_LEN - 1);
    }
    else
    {
        strncpy(c.characterfile, App::sim_player_character->getStr().c_str(), RORNET_MAX_CHARACTERFILE_LEN - 1);
    }

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
        // Do NOT `disconnect()` the m_socket in this case - causes SocketW to terminate RoR.
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

    PushNetMessage(MSG_NET_CONNECT_PROGRESS, _LC("Network", "Finishing..."));

    m_uid = header.source;

    // we get our userdata back
    memcpy(&m_userdata, buffer, std::min<int>(sizeof(RoRnet::UserInfo), header.size));

    m_shutdown = false;

    LOG("[RoR|Networking] Connect(): Creating Send/Recv threads");
    m_send_thread = std::thread(&Network::SendThread, this);
    m_recv_thread = std::thread(&Network::RecvThread, this);
    PushNetMessage(MSG_NET_CONNECT_SUCCESS, "");

    return true;
}

void Network::Disconnect()
{
    LOG("[RoR|Networking] Disconnect() disconnecting...");
    bool is_clean_disconnect = !m_shutdown; // Hacky detection of invalid network state

    m_shutdown = true; // Instruct Send/Recv threads to shut down.

    m_send_packet_available_cv.notify_one();

    m_send_thread.join();
    LOG("[RoR|Networking] Disconnect() sender thread cleaned up");

    m_socket.set_timeout(1, 0);

    if (is_clean_disconnect)
    {
        SendNetMessage(MSG2_USER_LEAVE, 0, 0, 0);
    }

    m_recv_thread.join();
    LOG("[RoR|Networking] Disconnect() receiver thread cleaned up");

    if (is_clean_disconnect)
    {
        m_socket.disconnect();
    }
    else
    {
        m_socket.close_fd();
    }

    SetNetQuality(0);
    m_users.clear();
    m_disconnected_users.clear();
    m_recv_packet_buffer.clear();
    m_send_packet_buffer.clear();
    App::GetConsole()->doCommand("clear net");

    m_shutdown = false;
    App::mp_state->setVal((int)MpState::DISABLED);

    LOG("[RoR|Networking] Disconnect() done");
}

void Network::AddPacket(int streamid, int type, int len, const char *content)
{
    const auto max_len = RORNET_MAX_MESSAGE_LENGTH - sizeof(RoRnet::Header);
    if (len > max_len)
    {
        LOGSTREAM << "[RoR|Networking] Discarding network packet (StreamID: "
            <<streamid<<", Type: "<<type<<"), length is " << len << ", max is " << max_len;
        return;
    }

    NetSendPacket packet;
    memset(&packet, 0, sizeof(NetSendPacket));

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
                    [&](const NetSendPacket& p) { return !memcmp(packet.buffer, p.buffer, sizeof(RoRnet::Header)); });
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

void Network::AddLocalStream(RoRnet::StreamRegister *reg, int size)
{
    reg->origin_sourceid = m_uid;
    reg->origin_streamid = m_stream_id;
    reg->status = 0;

    AddPacket(m_stream_id, MSG2_STREAM_REGISTER, size, (char*)reg);
    LOG("adding local stream: " + TOSTRING(m_uid) + ":"+ TOSTRING(m_stream_id) + ", type: " + TOSTRING(reg->type));

    m_stream_id++;
}

std::vector<NetRecvPacket> Network::GetIncomingStreamData()
{
    std::lock_guard<std::mutex> lock(m_recv_packetqueue_mutex);
    std::vector<NetRecvPacket> buf_copy = m_recv_packet_buffer;
    m_recv_packet_buffer.clear();
    return buf_copy;
}

Ogre::String Network::GetTerrainName()
{
    return m_server_settings.terrain;
}

int Network::GetUserColor()
{
    std::lock_guard<std::mutex> lock(m_userdata_mutex);
    return m_userdata.colournum;
}

Ogre::UTFString Network::GetUsername()
{
    std::lock_guard<std::mutex> lock(m_userdata_mutex);
    return m_username;
}

RoRnet::UserInfo Network::GetLocalUserData()
{
    std::lock_guard<std::mutex> lock(m_userdata_mutex);
    return m_userdata;
}

std::vector<RoRnet::UserInfo> Network::GetUserInfos()
{
    std::lock_guard<std::mutex> lock(m_users_mutex);
    return m_users;
}

bool Network::GetUserInfo(int uid, RoRnet::UserInfo &result)
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

bool Network::GetDisconnectedUserInfo(int uid, RoRnet::UserInfo &result)
{
    std::lock_guard<std::mutex> lock(m_users_mutex);
    for (RoRnet::UserInfo user : m_disconnected_users)
    {
        if ((int)user.uniqueid == uid)
        {
            result = user;
            return true;
        }
    }
    return false;
}

bool Network::GetAnyUserInfo(int uid, RoRnet::UserInfo &result)
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

bool Network::FindUserInfo(std::string const& username, RoRnet::UserInfo &result)
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

void Network::BroadcastChatMsg(const char* msg)
{
    AddPacket(m_stream_id, RoRnet::MSG2_UTF8_CHAT, (int)std::strlen(msg), msg);
}

void Network::WhisperChatMsg(RoRnet::UserInfo const& user, const char* msg)
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
    AddPacket(m_stream_id, RoRnet::MSG2_UTF8_PRIVCHAT, (int)payload_len, msg);
}

std::string Network::UserAuthToStringShort(RoRnet::UserInfo const &user)
{
         if (user.authstatus & AUTH_ADMIN)    { return _LC("NetUserAuth", "Admin");  }
    else if (user.authstatus & AUTH_MOD)      { return _LC("NetUserAuth", "Mod");    }
    else if (user.authstatus & AUTH_BOT)      { return _LC("NetUserAuth", "Bot");    }
    else if (user.authstatus & AUTH_RANKED)   { return _LC("NetUserAuth", "Ranked"); }
    else if (user.authstatus & AUTH_BANNED)   { return _LC("NetUserAuth", "Banned"); }
    else                                      { return _LC("NetUserAuth", "Guest");  }
}

std::string Network::UserAuthToStringLong(RoRnet::UserInfo const &user)
{
         if (user.authstatus & AUTH_ADMIN)    { return _LC("NetUserAuth","Server Administrator");   }
    else if (user.authstatus & AUTH_MOD)      { return _LC("NetUserAuth","Server Moderator");       }
    else if (user.authstatus & AUTH_BOT)      { return _LC("NetUserAuth","Bot");                    }
    else if (user.authstatus & AUTH_RANKED)   { return _LC("NetUserAuth","Ranked user");            }
    else if (user.authstatus & AUTH_BANNED)   { return _LC("NetUserAuth","Banned user");            }
    else                                      { return _LC("NetUserAuth","Guest");                  }
}

#endif // USE_SOCKETW
