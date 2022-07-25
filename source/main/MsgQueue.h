/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2022 Petr Ohlidal

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

/// @file

#pragma once

#include <queue>
#include <string>
#include <vector>

namespace RoR {

/// @addtogroup MsgQueue
/// @{

/// Global gameplay message loop, see `struct Message` in GameContext.h
enum MsgType
{
    MSG_INVALID,
    // Application
    MSG_APP_SHUTDOWN_REQUESTED,
    MSG_APP_SCREENSHOT_REQUESTED,
    MSG_APP_DISPLAY_FULLSCREEN_REQUESTED,
    MSG_APP_DISPLAY_WINDOWED_REQUESTED,
    MSG_APP_MODCACHE_LOAD_REQUESTED,
    MSG_APP_MODCACHE_UPDATE_REQUESTED,
    MSG_APP_MODCACHE_PURGE_REQUESTED,
    // Networking
    MSG_NET_CONNECT_REQUESTED,
    MSG_NET_CONNECT_STARTED,
    MSG_NET_CONNECT_PROGRESS,
    MSG_NET_CONNECT_SUCCESS,
    MSG_NET_CONNECT_FAILURE,
    MSG_NET_SERVER_KICK,
    MSG_NET_DISCONNECT_REQUESTED,
    MSG_NET_USER_DISCONNECT,
    MSG_NET_RECV_ERROR,
    MSG_NET_REFRESH_SERVERLIST_SUCCESS,    //!< Payload = GUI::MpServerInfoVec* (owner)
    MSG_NET_REFRESH_SERVERLIST_FAILURE,
    MSG_NET_REFRESH_REPOLIST_SUCCESS,    //!< Payload = GUI::ResourcesCollection* (owner)
    MSG_NET_OPEN_RESOURCE_SUCCESS,    //!< Payload = GUI::ResourcesCollection* (owner)
    MSG_NET_REFRESH_REPOLIST_FAILURE,
    // Simulation
    MSG_SIM_PAUSE_REQUESTED,
    MSG_SIM_UNPAUSE_REQUESTED,
    MSG_SIM_LOAD_TERRN_REQUESTED,
    MSG_SIM_LOAD_SAVEGAME_REQUESTED,
    MSG_SIM_UNLOAD_TERRN_REQUESTED,
    MSG_SIM_SPAWN_ACTOR_REQUESTED,         //!< Payload = RoR::ActorSpawnRequest* (owner)
    MSG_SIM_MODIFY_ACTOR_REQUESTED,        //!< Payload = RoR::ActorModifyRequest* (owner)
    MSG_SIM_DELETE_ACTOR_REQUESTED,        //!< Payload = RoR::Actor* (weak)
    MSG_SIM_SEAT_PLAYER_REQUESTED,         //!< Payload = RoR::Actor* (weak) | nullptr
    MSG_SIM_TELEPORT_PLAYER_REQUESTED,     //!< Payload = Ogre::Vector3* (owner)
    MSG_SIM_HIDE_NET_ACTOR_REQUESTED,      //!< Payload = Actor* (weak)
    MSG_SIM_UNHIDE_NET_ACTOR_REQUESTED,    //!< Payload = Actor* (weak)
    // GUI
    MSG_GUI_OPEN_MENU_REQUESTED,
    MSG_GUI_CLOSE_MENU_REQUESTED,
    MSG_GUI_OPEN_SELECTOR_REQUESTED,       //!< Payload = LoaderType* (owner), Description = GUID | empty
    MSG_GUI_CLOSE_SELECTOR_REQUESTED,
    MSG_GUI_MP_CLIENTS_REFRESH,
    MSG_GUI_SHOW_MESSAGE_BOX_REQUESTED,    //!< Payload = MessageBoxConfig* (owner)
    MSG_GUI_DOWNLOAD_PROGRESS,
    MSG_GUI_DOWNLOAD_FINISHED,
    // Editing
    MSG_EDI_MODIFY_GROUNDMODEL_REQUESTED,  //!< Payload = RoR::ground_model_t* (weak)
    MSG_EDI_ENTER_TERRN_EDITOR_REQUESTED,
    MSG_EDI_LEAVE_TERRN_EDITOR_REQUESTED,
    MSG_EDI_RELOAD_BUNDLE_REQUESTED,       //!< Payload = RoR::CacheEntry* (weak)
};

/// Unified game event system - all requests and state changes are reported using a message.
struct Message
{
    Message(MsgType _type) : type(_type) {}
    Message(MsgType _type, std::string const& _desc) : type(_type), description(_desc) {}
    Message(MsgType _type, void* _data) : type(_type), payload(_data) {}

    MsgType     type = MSG_INVALID;
    std::string description;
    void* payload = nullptr;
    std::vector<Message> chain; //!< Posted after the message is processed
};

typedef std::queue < Message, std::list<Message>> GameMsgQueue;

struct MessageListener
{
    virtual bool ProcessMessage(Message& m) = 0;
};

/// @} // addtogroup MsgQueue

} // namespace RoR