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

#include "RoRPrerequisites.h"

#include "RoRnet.h"

namespace RoR {
namespace Networking {

// ----------------------- Network messages (packed) -------------------------

#ifdef _MSC_VER          // MSVC: set global packing and make PACKED an empty macro
#   pragma pack(push, 1)
#   define PACKED
#else                    // GCC/Clang: use standard attribute
#   define PACKED __attribute__((packed))
#endif

enum CharacterCmd
{
    CHARACTER_CMD_INVALID,
    CHARACTER_CMD_POSITION,
    CHARACTER_CMD_ATTACH,
    CHARACTER_CMD_DETACH
};

PACKED struct CharacterMsgGeneric
{
    int32_t command;
};

PACKED struct CharacterMsgPos
{
    int32_t command;
    float   pos_x, pos_y, pos_z;
    float   rot_angle;
    float   anim_time;
    char    anim_name[CHARACTER_ANIM_NAME_LEN];
};

PACKED struct CharacterMsgAttach
{
    int32_t command;
    int32_t source_id;
    int32_t stream_id;
    int32_t position;
};

PACKED struct recv_packet_t
{
    RoRnet::Header header;
    char buffer[RORNET_MAX_MESSAGE_LENGTH];
};

#ifdef _MSC_VER
#   pragma pack(pop)
#endif
#undef PACKED

// ------------------------ End of network messages --------------------------

bool Connect();
void Disconnect();

void AddPacket(int streamid, int type, int len, char *content);
void AddLocalStream(RoRnet::StreamRegister *reg, int size);

void HandleStreamData();

int GetUID();
int GetNetQuality();

Ogre::String GetTerrainName();

int GetUserColor();
Ogre::UTFString GetUsername();
RoRnet::UserInfo GetLocalUserData();
std::vector<RoRnet::UserInfo> GetUserInfos();
bool GetUserInfo(int uid, RoRnet::UserInfo &result);

Ogre::UTFString GetErrorMessage();
bool CheckError();

} // namespace Networking
} // namespace RoR

#endif // USE_SOCKETW
