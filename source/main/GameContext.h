/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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
/// @author Petr Ohlidal
/// @brief  Game state manager and message-queue provider

#pragma once

#include "Beam.h" // class Actor
#include "BeamData.h" // Physics structs
#include "BeamFactory.h" // class ActorManager

#include <list>
#include <mutex>
#include <queue>
#include <string>

namespace RoR {

/// Unified game event system - all requests and state changes are reported using a message (work in progress)
struct Message
{
    Message(MsgType _type): type(_type) {}
    Message(MsgType _type, std::string const& _desc): type(_type), description(_desc) {}
    Message(MsgType _type, void* _data): type(_type), payload(_data) {}

    MsgType     type         = MSG_INVALID;
    std::string description;
    void*       payload      = nullptr;
};

typedef std::queue < Message, std::list<Message>> GameMsgQueue;

class GameContext
{
public:

    // ----------------------------
    // Message queue

    void                PushMessage(Message m);
    bool                HasMessages();
    Message             PopMessage();

    // ----------------------------
    // Actors

    Actor*              SpawnActor(ActorSpawnRequest& rq);
    void                ModifyActor(ActorModifyRequest& rq);
    void                DeleteActor(Actor* actor);
    void                UpdateActors(float dt_sec);
    ActorManager*       GetActorManager() { return &m_actor_manager; }
    Actor*              FetchPrevVehicleOnList();
    Actor*              FetchNextVehicleOnList();
    Actor*              FindActorByCollisionBox(std::string const & ev_src_instance_name, std::string const & box_name);

    Actor*              GetPlayerActor() { return m_player_actor; }
    Actor*              GetPrevPlayerActor() { return m_prev_player_actor; }
    void                SetPrevPlayerActor(Actor* actor) { m_prev_player_actor = actor; }
    void                ChangePlayerActor(Actor* actor);

    // ----------------------------
    // Savegames (defined in Savegame.cpp)

    void                LoadScene(std::string const& filename); ///< Matching terrain must be already loaded
    void                SaveScene(std::string const& filename);
    std::string         GetQuicksaveFilename(); ///< For currently loaded terrain (cvar sim_terrain_name)
    std::string         ExtractSceneName(std::string const& filename);
    std::string         ExtractSceneTerrain(std::string const& filename); ///< Returns terrain filename

private:
    // Message queue
    GameMsgQueue        m_msg_queue;
    std::mutex          m_msg_mutex;

    // Actors (physics and netcode)
    ActorManager        m_actor_manager;
    Actor*              m_player_actor = nullptr;           ///< Actor (vehicle or machine) mounted and controlled by player
    Actor*              m_prev_player_actor = nullptr;      ///< Previous actor (vehicle or machine) mounted and controlled by player
};

} // namespace RoR
