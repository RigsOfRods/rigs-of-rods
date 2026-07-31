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

bool Network::SendMessageTcp(int type, unsigned int streamid, int len, char* content)
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

int Network::ReceiveMessageTcp(RoRnet::Header *head, char* content, int bufferlen)
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

void Network::OnPacketReceived(ENetPacket* packet)
{
    // Sanity check
    if (packet->dataLength < sizeof(RoRnet::Header))
    {
        this->PushNetMessage(MSG_NET_RECV_ERROR, _LC("Network", "Error receiving data from network"));
        this->DisconnectENet();
        return;
    }

    // Analyze packet
    RoRnet::Header header;
    char buffer[RORNET_MAX_MESSAGE_LENGTH] = {0};
    std::memcpy(&header, packet->data, sizeof(RoRnet::Header));
    if (header.size > 0)
    {
        std::memcpy(buffer, packet->data + sizeof(RoRnet::Header), header.size);
    }

    // Update client state
    if (m_progress == NetProgress::AWAITING_HELLO_RESPONSE)
    {
        LOG("[RoR|Networking|ENet] received response to RoRnet HELLO");

        if (header.command == MSG2_WRONG_VER)
        {
            this->PushNetMessage(MSG_NET_CONNECT_FAILURE, _L("server uses a different protocol version"));
            this->DisconnectENet();
        }
        
        if (header.command == MSG2_SERVER_SETTINGS)
        {
            // Save server settings
            std::memcpy(&m_server_settings, buffer, sizeof(RoRnet::ServerInfo));

            // Construct user credentials
            RoRnet::UserInfo c;
            memset(&c, 0, sizeof(RoRnet::UserInfo));
            strncpy(c.username, m_username.c_str(), RORNET_MAX_USERNAME_LEN-1);
            strncpy(c.serverpassword, Sha1Hash(m_password).c_str(), size_t(40));
            strncpy(c.usertoken, Sha1Hash(m_token).c_str(), size_t(40));
            strncpy(c.clientversion, ROR_VERSION_STRING, strnlen(ROR_VERSION_STRING, 25));
            strncpy(c.clientname, "RoR", 10);
            std::string language = App::app_language->getStr().substr(0, 2);
            std::string country = App::app_country->getStr().substr(0, 2);
            strncpy(c.language, (language + std::string("_") + country).c_str(), 5);
            strcpy(c.sessiontype, "normal");
            this->AddPacket(0, MSG2_USER_INFO, sizeof(RoRnet::UserInfo), (char*)&c);

            m_progress = NetProgress::AWAITING_USER_AUTH_RESPONSE;
        }
        else
        {
            this->PushNetMessage(MSG_NET_CONNECT_FAILURE, _L("Establishing network session: sorry, unknown server response"));
            this->DisconnectENet();
        }
        return;
    }

    if (m_progress == NetProgress::AWAITING_USER_AUTH_RESPONSE)
    {
        LOG("[RoR|Networking|ENet] received response to RoRnet USER_INFO");

        if (header.command==MSG2_FULL)
        {
            this->PushNetMessage(MSG_NET_CONNECT_FAILURE, _L("Establishing network session: sorry, server has too many players"));
            LOG("[RoR|Networking|ENet] server responded FULL, disconnecting peer.");
            this->DisconnectENet();
        }
        else if (header.command==MSG2_BANNED)
        {
            this->PushNetMessage(MSG_NET_CONNECT_FAILURE, _L("Establishing network session: sorry, you are banned!"));
            LOG("[RoR|Networking|ENet] server responded BANNED, disconnecting peer.");
            this->DisconnectENet();
        }
        else if (header.command==MSG2_WRONG_PW)
        {
            this->PushNetMessage(MSG_NET_CONNECT_FAILURE, _L("Establishing network session: sorry, wrong password!"));
            LOG("[RoR|Networking|ENet] server responded WRONG_PW, disconnecting peer.");
            this->DisconnectENet();
        }
        else if (header.command==MSG2_WRONG_VER)
        {
            this->PushNetMessage(MSG_NET_CONNECT_FAILURE, _L("Establishing network session: sorry, wrong protocol version!"));
            LOG("[RoR|Networking|ENet] server responded WRONG_VER, disconnecting peer.");
            this->DisconnectENet();
        }
        else if (header.command==MSG2_NO_RANK)
        {
            this->PushNetMessage(MSG_NET_CONNECT_FAILURE, _L("Establishing network session: sorry, this server requires a user token!"));
            // ChainMessage() ensures the MP settings request is handled _after_ the MSG_NET_CONNECT_FAILURE with the text above.
            App::GetGameContext()->ChainMessage(Message(MSG_GUI_OPEN_MP_SETTINGS_REQUESTED));
            this->DisconnectENet();
        }

        if (header.command==MSG2_WELCOME)
        {
            LOG("[RoR|Networking|ENet] server said WELCOME");
            m_uid = header.source;

            // we get our userdata back
            std::memcpy(&m_userdata, buffer, std::min<int>(sizeof(RoRnet::UserInfo), header.size));

            this->PushNetMessage(MSG_NET_CONNECT_SUCCESS, "");
            m_progress = NetProgress::PLAYING;
        }
        else
        {
            LOG("[RoR|Networking|ENet] unexpected server response, disconnecting peer.");
            this->PushNetMessage(MSG_NET_CONNECT_FAILURE, _L("Establishing network session: sorry, unknown server response"));
            this->DisconnectENet();
        }
        return;
    }

    // Process the packet

    if (header.command == MSG2_STREAM_REGISTER)
    {
        if (header.source == m_uid)
            return;

        RoRnet::StreamRegister *reg = (RoRnet::StreamRegister *)buffer;

        LOG(" * received stream registration: " + TOSTRING(header.source) + ": " + TOSTRING(header.streamid) + ", type: " + TOSTRING(reg->type));
        App::GetGameContext()->PushMessage(Message(MSG_NET_BCAST_PACKET_DISPATCHED, packet));
        return;
    }
    else if (header.command == MSG2_STREAM_REGISTER_RESULT)
    {
        RoRnet::StreamRegister *reg = (RoRnet::StreamRegister *)buffer;
        LOG(" * received stream registration result: " + TOSTRING(header.source) + ": " + TOSTRING(header.streamid) + ", status: " + TOSTRING(reg->status));
        App::GetGameContext()->PushMessage(Message(MSG_NET_BCAST_PACKET_DISPATCHED, packet));
        return;
    }
    else if (header.command == MSG2_STREAM_UNREGISTER)
    {
        LOG(" * received stream deregistration: " + TOSTRING(header.source) + ": " + TOSTRING(header.streamid));
        App::GetGameContext()->PushMessage(Message(MSG_NET_BCAST_PACKET_DISPATCHED, packet));
        return;
    }
    else if (header.command == MSG2_UTF8_CHAT || header.command == MSG2_UTF8_PRIVCHAT)
    {
        // Console is threadsafe
        ChatSystem::ReceiveStreamData(header.command, header.source, buffer);
        return;
    }
    else if (header.command == MSG2_NETQUALITY && header.source == -1)
    {
        if (header.size != sizeof(int))
        {
            return;
        }
        int quality = *(int *)buffer;
        SetNetQuality(quality);
        return;
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

                // Erase matching peer options
                int peeropt_offset = (int)std::distance(m_users.begin(), user);
                auto peeropt_itor = m_users_peeropts.begin() + peeropt_offset;
                m_users_peeropts.erase(peeropt_itor);

                m_disconnected_users.push_back(*user); // Copy
                m_users.erase(user);
            }
        }
        App::GetGameContext()->PushMessage(Message(MSG_NET_BCAST_PACKET_DISPATCHED, packet));
        return;
    }
    else if (header.command == MSG2_USER_INFO || header.command == MSG2_USER_JOIN)
    {
        if (header.source == m_uid)
        {
            std::lock_guard<std::mutex> lock(m_userdata_mutex);
            memcpy(&m_userdata, buffer, sizeof(RoRnet::UserInfo));
            m_authlevel = m_userdata.authstatus;
            m_username = std::string(m_userdata.username);
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
                m_users_peeropts.push_back(BitMask_t(0));
            } // End of lock scope
        }
        return;
    }
    else if (header.command == MSG2_GAME_CMD)
    {
#ifdef USE_ANGELSCRIPT
        App::GetScriptEngine()->queueStringForExecution(Ogre::String(buffer));
#endif // USE_ANGELSCRIPT
        return;
    }
    else if (header.command == MSG2_STREAM_DATA_CHARACTER)
    {
        App::GetGameContext()->GetCharacterFactory()->recv_character_packets.Push(packet);
    }
    else if (header.command == MSG2_STREAM_DATA_ACTOR)
    {
        App::GetGameContext()->GetActorManager()->recv_actor_packets.Push(packet);
    }
    else if (header.command == MSG2_STREAM_DATA_FORCES)
    {
        App::GetGameContext()->GetActorManager()->recv_forces_packets.Push(packet);
    }

    // Update stats
    { // Begin lock scope
        std::lock_guard<std::mutex> lock(m_recv_client_stats_mutex);
        NetClientStats& stats = m_recv_client_stats[header.source];
        stats.combined_ping.AddSample(header.server2source_ping + m_peer->roundTripTime);
    } // End lock scope
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

bool Network::Connect()
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

    PushNetMessage(MSG_NET_CONNECT_PROGRESS, _LC("Network", "Checking server version..."));
    if (!SendMessageTcp(MSG2_HELLO, 0, (int)strlen(RORNET_VERSION), (char *)RORNET_VERSION))
    {
        CouldNotConnect(_L("Establishing network session: error sending hello"));
        return false;
    }

    RoRnet::Header header;
    char buffer[RORNET_MAX_MESSAGE_LENGTH] = {0};

    // Receive server (rornet protocol) version
    if (ReceiveMessageTcp(&header, buffer, RORNET_MAX_MESSAGE_LENGTH))
    {
        CouldNotConnect(_L("Establishing network session: error getting server version"));
        return false;
    }
    if (header.command == MSG2_WRONG_VER_LEGACY) // Pre-RoRnet_2.40 server
    {
        RoRnet::LegacyServerInfo info;
        memcpy(&info, buffer, sizeof(RoRnet::LegacyServerInfo));
        std::string format_str = _L("Establishing network session: wrong server version, you are using version '%s' and the server is using '%s'");
        const char* server_ver = (info.protocolversion[0] != 0) ? info.protocolversion : "~ RoRnet_2.38 or earlier (not detected) ~";
        CouldNotConnect(fmt::format(format_str.c_str(), RORNET_VERSION, server_ver));
        return false;
    }
    if (header.command == MSG2_WRONG_VER)
    {
        CouldNotConnect(_L("server uses a different protocol version"));
        return false;
    }
    if (header.command != MSG2_VERSION)
    {
        this->CouldNotConnect(_L("Establishing network session: unexpected response from server"));
        return false;
    }

    this->PushNetMessage(MSG_NET_CONNECT_PROGRESS, _LC("Network", "Switching to UDP..."));
    m_socket.disconnect();

    { // Lock scope
        std::lock_guard<std::mutex> enet_lock(m_enet_mutex);

        m_host = enet_host_create (nullptr /* create a client host */,
                1 /* only allow 1 outgoing connection */,
                2 /* allow up 2 channels to be used, 0 and 1 */,
                0 /* assume any amount of incoming bandwidth */,
                0 /* assume any amount of outgoing bandwidth */);
        if (m_host == nullptr)
        {
            this->PushNetMessage(MSG_NET_CONNECT_FAILURE, _L("Establishing network session: could not create ENet client"));
            return false;
        }

        ENetAddress address;
        if (enet_address_set_host(&address, m_net_host.c_str()) != 0)
        {
            this->PushNetMessage(MSG_NET_CONNECT_FAILURE, _L("Establishing network session: ENet rejected the server IP"));
            return false;
        }
        address.port = m_net_port + 1;

        this->PushNetMessage(MSG_NET_CONNECT_PROGRESS, _LC("Network", "Connecting via UDP..."));
        size_t num_channels = 1;
        m_peer = enet_host_connect(m_host, &address, num_channels, 0);
        if (m_peer == nullptr)
        {
            this->PushNetMessage(MSG_NET_CONNECT_FAILURE, _L("Establishing network session: could not connect to server using ENet"));
            enet_host_destroy(m_host);
            m_host = nullptr;
            return false;
        }
    
    } // lock scope

    // Spin the ENet dispatch loop
    LOG("[RoR|Networking|ENet] dispatch loop is running...");
    while (!m_shutdown)
    {
        NetSendPacket q_packet;
        { // packet queue lock scope
            std::unique_lock<std::mutex> queue_lock(m_send_packetqueue_mutex);
            while (!m_send_packet_buffer.empty())
            {
                q_packet = m_send_packet_buffer.front();
                
                { // enet lock scope
                    std::lock_guard<std::mutex> enet_lock(m_enet_mutex);
                    enet_uint32 packet_flags = (IsRoRnetDiscardable(q_packet.type)) ? 0 : ENET_PACKET_FLAG_RELIABLE;
                    ENetPacket* packet = enet_packet_create(q_packet.buffer, q_packet.size, packet_flags);
                    if (packet == nullptr)
                    {
                        LOG("NET send error: could not create ENet packet");
                        enet_peer_disconnect(m_peer, 0);
                    }
                    int result = enet_peer_send(m_peer, 0, packet);
                    if (result != 0)
                    {
                        LOG(fmt::format("NET send error: could not send ENet packet, result: {}", result));
                        enet_peer_disconnect(m_peer, 0);
                    }
                } // END enet lock scope

                m_send_packet_buffer.pop_front();
            }
        } // END packet queue lock scope

        ENetEvent ev;
        enet_uint32 timeout_milisec = 0;
        { // enet lock scope
            std::lock_guard<std::mutex> enet_lock(m_enet_mutex);
            int result = enet_host_service(m_host, &ev, timeout_milisec);
            if (result < 0)
            {
                m_shutdown = true; // Atomic; instruct dispatch loop to quit.
                this->PushNetMessage(
                    (m_progress == NetProgress::PLAYING) ? MSG_NET_RECV_ERROR : MSG_NET_CONNECT_FAILURE,
                    _L("connection error"));
                continue;
            }
        } // END enet lock scope

        switch (ev.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
                LOG("[RoR|Networking|ENet] received event CONNECT");
                // Send HELLO again, this time using ENet
                this->AddPacket(0, MSG2_HELLO, (int)strlen(RORNET_VERSION), (char *)RORNET_VERSION);
                { // enet lock scope
                    std::lock_guard<std::mutex> enet_lock(m_enet_mutex);
                    m_progress = NetProgress::AWAITING_HELLO_RESPONSE;
                }
                this->PushNetMessage(MSG_NET_CONNECT_PROGRESS, _LC("Network", "Getting server info..."));
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
            {
                LOG("[RoR|Networking|ENet] received event DISCONNECT");
                m_shutdown = true; // Atomic; instruct dispatch loop to quit.
                NetProgress progress = this->GetProgress();
                if (progress == NetProgress::AWAITING_DISCONNECT)
                {
                    LOG("[RoR|Networking|ENet] Disconnected on player request.");
                    this->PushNetMessage(MSG_NET_USER_DISCONNECT,
                        "");
                }
                else if (progress == NetProgress::PLAYING)
                {
                    LOG("[RoR|Networking|ENet] Unexpected disconnect (connection error or kick)");
                    this->PushNetMessage(MSG_NET_CONNECT_FAILURE,
                        _L("disconnected: error"));
                }
                else
                {
                    LOG("[RoR|Networking|ENet] Player rejected by server.");
                    this->PushNetMessage(MSG_NET_CONNECT_FAILURE,
                        _L("disconnected: remote side closed the connection"));                    
                }
                m_peer = nullptr;
            }
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                this->OnPacketReceived(ev.packet);
                break;

            default:;
        }
    }

    // Clean up
    //   DO NOT reset 'mp_state' cvar and `m_progress` member here - should be done by main thread after cleanup.
    enet_host_destroy(m_host);
    m_host = nullptr;
    this->SetNetQuality(0);
    m_users.clear();
    m_disconnected_users.clear();
    App::GetConsole()->doCommand("clear net");
    m_shutdown = false;    

    return true;
}

NetProgress Network::GetProgress()
{
    std::lock_guard<std::mutex> enet_lock(m_enet_mutex);
    return m_progress;
}

void Network::DisconnectENet()
{
    std::lock_guard<std::mutex> enet_lock(m_enet_mutex);
    enet_peer_disconnect(m_peer, 0);
}

void Network::Disconnect()
{
    NetProgress progress = this->GetProgress();

    if (progress == NetProgress::PLAYING)
    {
        // Mark our state as DISCONNECTING.
        std::unique_lock<std::mutex> manual_lock(m_enet_mutex);
        m_progress = NetProgress::AWAITING_DISCONNECT;
        manual_lock.unlock();

        // Tell server to disconnect us.
        this->AddPacket(0, MSG2_USER_LEAVE, 0, 0);
    }

    if (m_connect_thread.joinable())
    {
        m_connect_thread.join(); // thread will exit on ENET_EVENT_TYPE_DISCONNECT
    }
    App::mp_state->setVal((int)MpState::DISABLED);
}

void Network::AddPacket(int streamid, int type, int len, const char *content)
{
    NetSendPacket packet;
    memset(&packet, 0, sizeof(NetSendPacket));

    char *buffer = (char*)(packet.buffer);

    RoRnet::Header *head = (RoRnet::Header *)buffer;
    head->command     = type;
    head->source      = m_uid;
    head->size        = (uint32_t)len;
    head->streamid    = streamid;

    // then copy the contents
    char *bufferContent = (char *)(buffer + sizeof(RoRnet::Header));
    memcpy(bufferContent, content, len);

    // record the packet size
    packet.size = len + sizeof(RoRnet::Header);

    // record also packet type (redundant hack)
    packet.type = type;

    std::lock_guard<std::mutex> lock(m_send_packetqueue_mutex);
    if (IsRoRnetDiscardable(type))
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
            return;
        }
    }
    //DebugPacket("send", head, buffer);
    m_send_packet_buffer.push_back(packet);
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

Ogre::String Network::GetTerrainName()
{
    return m_server_settings.terrain;
}

int Network::GetUserColor()
{
    std::lock_guard<std::mutex> lock(m_userdata_mutex);
    return m_userdata.colournum;
}

std::string Network::GetUsername()
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

std::vector<BitMask_t> Network::GetAllUsersPeerOpts()
{
    std::lock_guard<std::mutex> lock(m_users_mutex);
    return m_users_peeropts;
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

bool Network::GetUserPeerOpts(int uid, BitMask_t& result)
{
    std::lock_guard<std::mutex> lock(m_users_mutex);
    for (size_t i = 0; i < m_users.size(); i++)
    {
        if (static_cast<int>(m_users[i].uniqueid) == uid)
        {
            result = m_users_peeropts[i];
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


void Network::AddPeerOptions(PeerOptionsRequest* rq)
{
    std::lock_guard<std::mutex> lock(m_users_mutex);

    const bool peeropts_sane = m_users.size() == m_users_peeropts.size();
    ROR_ASSERT(peeropts_sane);
    if (!peeropts_sane) return;

    for (size_t i = 0; i < m_users.size(); i++)
    {
        if (static_cast<int>(m_users[i].uniqueid) == rq->por_uid)
        {
            BITMASK_SET_1(m_users_peeropts[i], rq->por_peeropts);
        }
    }
}

void Network::RemovePeerOptions(PeerOptionsRequest* rq)
{
    std::lock_guard<std::mutex> lock(m_users_mutex);

    const bool peeropts_sane = m_users.size() == m_users_peeropts.size();
    ROR_ASSERT(peeropts_sane);
    if (!peeropts_sane) return;

    for (size_t i = 0; i < m_users.size(); i++)
    {
        if (static_cast<int>(m_users[i].uniqueid) == rq->por_uid)
        {
            BITMASK_SET_0(m_users_peeropts[i], rq->por_peeropts);
        }
    }
}

bool Network::GetUserStats(int uid, NetClientStats& result)
{
    std::lock_guard<std::mutex> lock(m_recv_client_stats_mutex);
    const auto it = m_recv_client_stats.find(uid);
    const bool found = it != m_recv_client_stats.end();
    if (found)
    {
        result = it->second;
    }
    return found;
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
