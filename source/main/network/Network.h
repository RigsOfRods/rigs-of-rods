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
#include "RoRPrerequisites.h"

#include <list>
#include <queue>
#include <string>
#include <vector>

namespace RoR {
namespace Networking {

// ----------------------- Network messages (packed) -------------------------

#pragma pack(push, 1)

enum CharacterCmd
{
    CHARACTER_CMD_INVALID,
    CHARACTER_CMD_POSITION,
    CHARACTER_CMD_ATTACH,
    CHARACTER_CMD_DETACH
};

struct CharacterMsgGeneric
{
    int32_t command;
};

struct CharacterMsgPos
{
    int32_t command;
    float   pos_x, pos_y, pos_z;
    float   rot_angle;
    float   anim_time;
    char    anim_name[CHARACTER_ANIM_NAME_LEN];
};

struct CharacterMsgAttach
{
    int32_t command;
    int32_t source_id;
    int32_t stream_id;
    int32_t position;
};

struct recv_packet_t
{
    RoRnet::Header header;
    char buffer[RORNET_MAX_MESSAGE_LENGTH];
};

#pragma pack(pop)

// ------------------------ End of network messages --------------------------

struct NetEvent
{
    enum class Type
    {
        INVALID,
        CONNECT_STARTED,
        CONNECT_PROGRESS,
        CONNECT_SUCCESS,
        CONNECT_FAILURE,
        SERVER_KICK,
        USER_DISCONNECT,
        RECV_ERROR,
    };

    NetEvent(Type t, std::string const& msg) :type(t), message(msg) {}

    Type type;
    std::string message;
};

typedef std::queue < NetEvent, std::list<NetEvent>> NetEventQueue;

bool                 StartConnecting();    ///< Launches connecting on background.
NetEventQueue        CheckEvents();        ///< Processes and returns the event queue.
void                 Disconnect();

void                 AddPacket(int streamid, int type, int len, const char *content);
void                 AddLocalStream(RoRnet::StreamRegister *reg, int size);

std::vector<recv_packet_t> GetIncomingStreamData();

int                  GetUID();
int                  GetNetQuality();

Ogre::String         GetTerrainName();

int                  GetUserColor();
Ogre::UTFString      GetUsername();
RoRnet::UserInfo     GetLocalUserData();
std::vector<RoRnet::UserInfo> GetUserInfos();
bool                 GetUserInfo(int uid, RoRnet::UserInfo &result);
bool                 FindUserInfo(std::string const& username, RoRnet::UserInfo &result);
Ogre::ColourValue    GetPlayerColor(int color_num);

void                 BroadcastChatMsg(const char* msg);
void                 WhisperChatMsg(RoRnet::UserInfo const& user, const char* msg);

std::string          UserAuthToStringShort(RoRnet::UserInfo const &user);
std::string          UserAuthToStringLong(RoRnet::UserInfo const &user);

} // namespace Networking
} // namespace RoR

#endif // USE_SOCKETW
