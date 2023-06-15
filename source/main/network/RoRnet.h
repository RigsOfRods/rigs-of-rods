/*
    This file is part of Rigs of Rods

    Copyright 2007  Pierre-Michel Ricordel
    Copyright 2014+ Petr Ohlidal & contributors.

    Rigs of Rods is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 3 of the License.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "BitFlags.h"

/// @file
/// @addtogroup Network
/// @{

namespace RoRnet {

#define RORNET_MAX_PEERS            64     //!< maximum clients connected at the same time
#define RORNET_MAX_MESSAGE_LENGTH   8192   //!< maximum size of a RoR message. 8192 bytes = 8 kibibytes
#define RORNET_LAN_BROADCAST_PORT   13000  //!< port used to send the broadcast announcement in LAN mode
#define RORNET_MAX_USERNAME_LEN     40     //!< bytes.

#define RORNET_VERSION              "RoRnet_2.44"

enum MessageType
{
    MSG2_HELLO  = 1025,                //!< client sends its version as first message
    
    // Hello responses
    MSG2_FULL,                         //!< no more slots for us
    MSG2_WRONG_PW,                     //!< server send that on wrong pw
    MSG2_WRONG_VER,                    //!< wrong version
    MSG2_BANNED,                       //!< client not allowed to join
    MSG2_WELCOME,                      //!< we can proceed

    // Technical
    MSG2_VERSION,                      //!< server responds with its version
    MSG2_SERVER_SETTINGS,              //!< server send client the terrain name: server_info_t
    MSG2_USER_INFO,                    //!< user data that is sent from the server to the clients
    MSG2_MASTERINFO,                   //!< master information response
    MSG2_NETQUALITY,                   //!< network quality information

    // Gameplay
    MSG2_GAME_CMD,                     //!< Script message. Can be sent in both directions.
    MSG2_USER_JOIN,                    //!< new user joined
    MSG2_USER_LEAVE,                   //!< user leaves
    MSG2_UTF8_CHAT,                    //!< broadcast chat line in UTF8 encoding; Payload: const char*(text)
    MSG2_UTF8_PRIVCHAT,                //!< private chat line in UTF8 encoding; Payload: uint32_t(uniqueid), const char*(text)

    // Stream functions
    MSG2_STREAM_REGISTER,              //!< create new stream
    MSG2_STREAM_REGISTER_RESULT,       //!< result of a stream creation
    MSG2_STREAM_UNREGISTER,            //!< remove stream
    MSG2_STREAM_DATA,                  //!< stream data
    MSG2_STREAM_DATA_DISCARDABLE,      //!< stream data that is allowed to be discarded

    // Legacy values (RoRnet_2.38 and earlier)
    MSG2_WRONG_VER_LEGACY = 1003       //!< Wrong version
};

enum UserAuth
{
    AUTH_NONE   = 0,                   //!< no authentication
    AUTH_ADMIN  = BITMASK(1),          //!< admin on the server
    AUTH_RANKED = BITMASK(2),          //!< ranked status
    AUTH_MOD    = BITMASK(3),          //!< moderator status
    AUTH_BOT    = BITMASK(4),          //!< bot status
    AUTH_BANNED = BITMASK(5)           //!< banned
};

enum Netmask
{
    NETMASK_HORN         = BITMASK(1), //!< horn is in use
    NETMASK_POLICEAUDIO  = BITMASK(2), //!< police siren on
    NETMASK_PARTICLE     = BITMASK(3), //!< custom particles on
    NETMASK_PBRAKE       = BITMASK(4), //!< parking brake
    NETMASK_TC_ACTIVE    = BITMASK(5), //!< traction control light on?
    NETMASK_ALB_ACTIVE   = BITMASK(6), //!< anti lock brake light on?
    NETMASK_ENGINE_CONT  = BITMASK(7), //!< ignition on?
    NETMASK_ENGINE_RUN   = BITMASK(8), //!< engine running?

    NETMASK_ENGINE_MODE_AUTOMATIC     = BITMASK(9), //!< engine mode
    NETMASK_ENGINE_MODE_SEMIAUTO      = BITMASK(10), //!< engine mode
    NETMASK_ENGINE_MODE_MANUAL        = BITMASK(11), //!< engine mode
    NETMASK_ENGINE_MODE_MANUAL_STICK  = BITMASK(12), //!< engine mode
    NETMASK_ENGINE_MODE_MANUAL_RANGES = BITMASK(13), //!< engine mode
};

enum Lightmask
{
    LIGHTMASK_CUSTOM1     = BITMASK(1),  //!< custom light 1 on
    LIGHTMASK_CUSTOM2     = BITMASK(2),  //!< custom light 2 on
    LIGHTMASK_CUSTOM3     = BITMASK(3),  //!< custom light 3 on
    LIGHTMASK_CUSTOM4     = BITMASK(4),  //!< custom light 4 on
    LIGHTMASK_CUSTOM5     = BITMASK(5),  //!< custom light 5 on
    LIGHTMASK_CUSTOM6     = BITMASK(6),  //!< custom light 6 on
    LIGHTMASK_CUSTOM7     = BITMASK(7),  //!< custom light 7 on
    LIGHTMASK_CUSTOM8     = BITMASK(8),  //!< custom light 8 on
    LIGHTMASK_CUSTOM9     = BITMASK(9),  //!< custom light 9 on
    LIGHTMASK_CUSTOM10    = BITMASK(10), //!< custom light 10 on

    LIGHTMASK_HEADLIGHT   = BITMASK(11),
    LIGHTMASK_HIGHBEAMS   = BITMASK(12),
    LIGHTMASK_FOGLIGHTS   = BITMASK(13),
    LIGHTMASK_SIDELIGHTS  = BITMASK(14),
    LIGHTMASK_BRAKES      = BITMASK(15), //!< brake lights on
    LIGHTMASK_REVERSE     = BITMASK(16), //!< reverse light on
    LIGHTMASK_BEACONS     = BITMASK(17), //!< beacons on
    LIGHTMASK_BLINK_LEFT  = BITMASK(18), //!< left blinker on
    LIGHTMASK_BLINK_RIGHT = BITMASK(19), //!< right blinker on
    LIGHTMASK_BLINK_WARN  = BITMASK(20), //!< warn blinker on
};

// -------------------------------- structs -----------------------------------
// Only use datatypes with defined binary sizes (avoid bool, int, wchar_t...)
// Prefer alignment to 4 or 2 bytes (put int32/float/etc. fields on top)

#pragma pack(push, 1)

struct Header                      //!< Common header for every packet
{
    uint32_t command;              //!< the command of this packet: MSG2_*
    int32_t  source;               //!< source of this command: 0 = server
    uint32_t streamid;             //!< streamid for this command
    uint32_t size;                 //!< size of the attached data block
};

struct StreamRegister              //!< Sent from the client to server and vice versa, to broadcast a new stream
{
    int32_t type;                  //!< 0 = Actor, 1 = Character, 3 = ChatSystem
    int32_t status;                //!< initial stream status
    int32_t origin_sourceid;       //!< origin sourceid
    int32_t origin_streamid;       //!< origin streamid
    char    name[128];             //!< file name
    char    data[128];             //!< data used for stream setup
};

struct ActorStreamRegister         //!< Must preserve mem. layout of RoRnet::StreamRegister
{
    // RoRnet::StreamRegister: Common
    int32_t type;                  //!< 0
    int32_t status;                //!< initial stream status
    int32_t origin_sourceid;       //!< origin sourceid
    int32_t origin_streamid;       //!< origin streamid
    char    name[128];             //!< truck file name
    // RoRnet::StreamRegister: Data buffer (128B)
    int32_t bufferSize;            //!< initial stream status
    int32_t time;                  //!< initial time stamp
    char    skin[60];              //!< skin
    char    sectionconfig[60];     //!< section configuration
};

struct StreamUnRegister            //< sent to remove a stream
{
    uint32_t streamid;
};

struct UserInfo
{
    uint32_t uniqueid;             //!< user unique id
    int32_t  authstatus;           //!< auth status set by server: AUTH_*
    int32_t  slotnum;              //!< slot number set by server
    int32_t  colournum;            //!< colour set by server

    char     username[RORNET_MAX_USERNAME_LEN]; //!< the nickname of the user (UTF-8)
    char     usertoken[40];        //!< user token
    char     serverpassword[40];   //!< server password
    char     language[10];         //!< user's language. For example "de-DE" or "en-US"
    char     clientname[10];       //!< the name and version of the client. For exmaple: "ror" or "gamebot"
    char     clientversion[25];    //!< a version number of the client. For example 1 for RoR 0.35
    char     clientGUID[40];       //!< the clients GUID
    char     sessiontype[10];      //!< the requested session type. For example "normal", "bot", "rcon"
    char     sessionoptions[128];  //!< reserved for future options
};

struct VehicleState                  //!< Formerly `oob_t`
{
    int32_t  time;                 //!< time data
    float    engine_speed;         //!< engine RPM
    float    engine_force;         //!< engine acceleration
    float    engine_clutch;        //!< the clutch value
    int32_t  engine_gear;          //!< engine gear
    float    hydrodirstate;        //!< the turning direction status
    float    brake;                //!< the brake value
    float    wheelspeed;           //!< the wheel speed value
    BitMask_t flagmask;             //!< flagmask: NETMASK_*
    BitMask_t lightmask;            //!< flagmask: LIGHTMASK_*
};

struct ServerInfo
{
    char    protocolversion[20];   //!< protocol version being used
    char    terrain[128];          //!< terrain name
    char    servername[128];       //!< name of the server
    uint8_t has_password;          //!< passworded server?
    char    info[4096];            //!< info text
};

struct LegacyServerInfo
{
    char    protocolversion[20];   //!< protocol version being used
};

} // namespace RoRnet

/// @}   //addtogroup Network

#pragma pack(pop)
