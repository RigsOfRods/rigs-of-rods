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

#pragma once

#ifdef USE_SOCKETW

#include "Application.h"
#include "RoRnet.h"

#include <SocketW.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <list>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <OgreUTFString.h>

namespace RoR {

/// @addtogroup Network
/// @{

struct CurlFailInfo
{
    std::string title;
    CURLcode curl_result = CURLE_OK;
    long http_response = 0;
};

// ----------------------- Network messages (packed) -------------------------

#pragma pack(push, 1)

enum NetCharacterCmd
{
    CHARACTER_CMD_INVALID,
    CHARACTER_CMD_POSITION,
    CHARACTER_CMD_ATTACH,
    CHARACTER_CMD_DETACH
};

struct NetCharacterMsgGeneric
{
    int32_t command;
};

struct NetCharacterMsgPos
{
    int32_t command;
    float   pos_x, pos_y, pos_z;
    float   rot_angle;
    uint32_t action_flags;
    uint32_t situation_flags;
};

struct NetCharacterMsgAttach
{
    int32_t command;
    int32_t source_id;
    int32_t stream_id;
    int32_t position;
};

struct NetSendPacket
{
    char buffer[RORNET_MAX_MESSAGE_LENGTH];
    int size;
};

struct NetRecvPacket
{
    RoRnet::Header header;
    char buffer[RORNET_MAX_MESSAGE_LENGTH];
};

#pragma pack(pop)

// ------------------------ End of network messages --------------------------

class Network
{
public:
    bool                 StartConnecting();    //!< Launches connecting on background.
    void                 StopConnecting();
    void                 Disconnect();

    void                 AddPacket(int streamid, int type, int len, const char *content);
    void                 AddLocalStream(RoRnet::StreamRegister *reg, int size);

    std::vector<NetRecvPacket> GetIncomingStreamData();

    int                  GetUID();
    int                  GetNetQuality();

    Ogre::String         GetTerrainName();

    int                  GetUserColor();
    Ogre::UTFString      GetUsername();
    RoRnet::UserInfo     GetLocalUserData();
    std::vector<RoRnet::UserInfo> GetUserInfos();
    bool                 GetUserInfo(int uid, RoRnet::UserInfo &result);
    bool                 GetDisconnectedUserInfo(int uid, RoRnet::UserInfo &result);
    bool                 GetAnyUserInfo(int uid, RoRnet::UserInfo &result); //!< Also considers local client
    bool                 FindUserInfo(std::string const& username, RoRnet::UserInfo &result);
    Ogre::ColourValue    GetPlayerColor(int color_num);

    void                 BroadcastChatMsg(const char* msg);
    void                 WhisperChatMsg(RoRnet::UserInfo const& user, const char* msg);

    std::string          UserAuthToStringShort(RoRnet::UserInfo const &user);
    std::string          UserAuthToStringLong(RoRnet::UserInfo const &user);

private:
    void                 PushNetMessage(MsgType type, std::string const & message);
    void                 SetNetQuality(int quality);
    bool                 SendMessageRaw(char *buffer, int msgsize);
    bool                 SendNetMessage(int type, unsigned int streamid, int len, char* content);
    void                 QueueStreamData(RoRnet::Header &header, char *buffer, size_t buffer_len);
    int                  ReceiveMessage(RoRnet::Header *head, char* content, int bufferlen);
    void                 CouldNotConnect(std::string const & msg, bool close_socket = true);

    bool                 ConnectThread();
    void                 SendThread();
    void                 RecvThread();

    // Variables

    SWInetSocket         m_socket;

    RoRnet::ServerInfo   m_server_settings;
    RoRnet::UserInfo     m_userdata;
    std::vector<RoRnet::UserInfo> m_users;
    std::vector<RoRnet::UserInfo> m_disconnected_users;

    Ogre::UTFString      m_username; // Shadows GVar 'mp_player_name' for multithreaded access.
    std::string          m_net_host; // Shadows GVar 'mp_server_host' for multithreaded access.
    std::string          m_password; // Shadows GVar 'mp_server_password' for multithreaded access.
    std::string          m_token;    // Shadows GVar 'mp_player_token' for multithreaded access.
    int                  m_net_port; // Shadows GVar 'mp_server_port' for multithreaded access.
    int                  m_uid;
    int                  m_authlevel;

    std::thread          m_send_thread;
    std::thread          m_recv_thread;
    std::thread          m_connect_thread;

    std::string          m_status_message;
    std::atomic<bool>    m_shutdown;
    std::atomic<int>     m_net_quality;
    int                  m_stream_id = 10; //!< Counter

    std::mutex           m_users_mutex;
    std::mutex           m_userdata_mutex;
    std::mutex           m_recv_packetqueue_mutex;
    std::mutex           m_send_packetqueue_mutex;

    std::condition_variable m_send_packet_available_cv;

    std::vector<NetRecvPacket> m_recv_packet_buffer;
    std::deque <NetSendPacket> m_send_packet_buffer;
};

/// @}   //addtogroup Network

} // namespace RoR

#endif // USE_SOCKETW
