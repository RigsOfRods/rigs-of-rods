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
#include "CharacterFactory.h"
#include "RaceSystem.h"
#include "GUI_SceneMouse.h"

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

/// RoR's gameplay is quite simple in structure, it consists of:
///  - static terrain:  static elevation map, managed by `TerrainManager`.
///                     this includes static collision objects (or intrusion detection objects), managed by `TerrainObjectManager`.
///  - softbody actors: a.k.a "trucks" or "vehicles" (local or remote), managed by `ActorManager`. They collide with static terrain and each other.
///                     this includes 'fixes' - actors with partially fixed position.
///  - characters:      player-controlled avatars (local or remote), managed by `CharacterFactory`.
///                     they have simplified physics and can climb objects.
/// For convenience and to help manage interactions, this class provides methods to manipulate these elements.
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
    void                RespawnLastActor();

    Actor*              GetPlayerActor() { return m_player_actor; }
    Actor*              GetPrevPlayerActor() { return m_prev_player_actor; }
    void                SetPrevPlayerActor(Actor* actor) { m_prev_player_actor = actor; }
    void                ChangePlayerActor(Actor* actor);

    void                ShowLoaderGUI(int type, const Ogre::String& instance, const Ogre::String& box);
    void                OnLoaderGuiCancel(); ///< GUI callback
    void                OnLoaderGuiApply(RoR::LoaderType type, CacheEntry* entry, std::string sectionconfig);  ///< GUI callback

    // ----------------------------
    // Characters

    Character*          GetPlayerCharacter();
    CharacterFactory*   GetCharacterFactory() { return &m_character_factory; }

    // ----------------------------
    // Savegames (defined in Savegame.cpp)

    void                LoadScene(std::string const& filename); ///< Matching terrain must be already loaded
    void                SaveScene(std::string const& filename);
    std::string         GetQuicksaveFilename(); ///< For currently loaded terrain (cvar sim_terrain_name)
    std::string         ExtractSceneName(std::string const& filename);
    std::string         ExtractSceneTerrain(std::string const& filename); ///< Returns terrain filename

    // ----------------------------
    // Gameplay feats (misc.)

    RaceSystem&         GetRaceSystem() { return m_race_system; }
    void                TeleportPlayer(float x, float z);
    
private:
    // Message queue
    GameMsgQueue        m_msg_queue;
    std::mutex          m_msg_mutex;

    // Actors (physics and netcode)
    ActorManager        m_actor_manager;
    Actor*              m_player_actor = nullptr;           ///< Actor (vehicle or machine) mounted and controlled by player
    Actor*              m_prev_player_actor = nullptr;      ///< Previous actor (vehicle or machine) mounted and controlled by player
    
    CacheEntry*         m_last_cache_selection = nullptr;   ///< Vehicle/load
    CacheEntry*         m_last_skin_selection = nullptr;
    Ogre::String        m_last_section_config;
    ActorSpawnRequest   m_current_selection;                ///< Context of the loader UI

    // Characters (simplified physics and netcode)
    CharacterFactory    m_character_factory;

    // Gameplay feats (misc.)
    RaceSystem          m_race_system;
};

} // namespace RoR
