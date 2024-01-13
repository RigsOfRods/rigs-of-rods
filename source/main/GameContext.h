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

#include "ActorManager.h"
#include "CacheSystem.h"
#include "CharacterFactory.h"
#include "RaceSystem.h"
#include "RepairMode.h"
#include "SceneMouse.h"
#include "SimData.h"
#include "Terrain.h"

#include <list>
#include <mutex>
#include <queue>
#include <string>

namespace RoR {

/// @addtogroup GameState
/// @{

/// @addtogroup MsgQueue
/// @{

/// Unified game event system - all requests and state changes are reported using a message.
struct Message
{
    Message(MsgType _type): type(_type) {}
    Message(MsgType _type, std::string const& _desc): type(_type), description(_desc) {}
    Message(MsgType _type, void* _data): type(_type), payload(_data) {}

    MsgType     type         = MSG_INVALID;
    std::string description;
    void*       payload      = nullptr;
    std::vector<Message> chain; //!< Posted after the message is processed
};

typedef std::queue < Message, std::list<Message>> GameMsgQueue;

/// @} // addtogroup MsgQueue

/// Central game state manager.
/// RoR's gameplay is quite simple in structure, it consists of:
///  - static terrain:  static elevation map, managed by `Terrain`.
///                     this includes static collision objects (or intrusion detection objects), managed by `TerrainObjectManager`.
///  - softbody actors: a.k.a "trucks" or "vehicles" (local or remote), managed by `ActorManager`. They collide with static terrain and each other.
///                     this includes 'fixes' - actors with partially fixed position.
///  - characters:      player-controlled avatars (local or remote), managed by `CharacterFactory`.
///                     they have simplified physics and can climb objects.
/// For convenience and to help manage interactions, this class provides methods to manipulate these elements.
///
/// CAUTION:
/// PushMessage() doesn't guarantee order of processing: if message handler generates additional messages, it moves the original message at the end of the queue.
/// To illustrate:
/// 1. Start with empty queue.
/// 2. push A.
/// 3. push B which needs A.
/// 4. Process the queue.
/// 5. pop A, which needs C done first. It pushes C and re-pushes A.
/// 6. Queue is now BCA. B fails because A wasn't done.
/// 
/// Message chaining is for situations where message order must be preserved.
/// 1. Start with empty queue
/// 2. push A.
/// 3. chain B which needs A. - it's added to A's chain: A{B}.
/// 4. Process the queue.
/// 5. pop A, which needs C done first. It pushes C and re-pushes A{B}.
/// 6. Queue is now C, A{B}. B succeeds because A gets done first.

class GameContext
{
    friend class AppContext;
public:

    GameContext();
    ~GameContext();

    /// @name Message queue
    /// @{

    void                PushMessage(Message m);  //!< Doesn't guarantee order! Use ChainMessage() if order matters.
    void                ChainMessage(Message m); //!< Add to last pushed message's chain
    bool                HasMessages();
    Message             PopMessage();

    /// @}
    /// @name Terrain
    /// @{

    bool                LoadTerrain(std::string const& filename_part);
    void                UnloadTerrain();
    const TerrainPtr&   GetTerrain() { return m_terrain; }

    /// @}
    /// @name Actors
    /// @{

    ActorPtr            SpawnActor(ActorSpawnRequest& rq);
    void                ModifyActor(ActorModifyRequest& rq);
    void                DeleteActor(ActorPtr actor);
    void                UpdateActors();
    ActorManager*       GetActorManager() { return &m_actor_manager; }
    const ActorPtr&     FetchPrevVehicleOnList();
    const ActorPtr&     FetchNextVehicleOnList();
    ActorPtr            FindActorByCollisionBox(std::string const & ev_src_instance_name, std::string const & box_name);
    void                RespawnLastActor();
    void                SpawnPreselectedActor(std::string const& preset_vehicle, std::string const& preset_veh_config); //!< needs `Character` to exist

    const ActorPtr&     GetPlayerActor() { return m_player_actor; }
    const ActorPtr&     GetPrevPlayerActor() { return m_prev_player_actor; }
    const ActorPtr&     GetLastSpawnedActor() { return m_last_spawned_actor; } //!< Last actor spawned by user and still alive.
    void                SetPrevPlayerActor(ActorPtr actor);
    void                ChangePlayerActor(ActorPtr actor);

    void                ShowLoaderGUI(int type, const Ogre::String& instance, const Ogre::String& box);
    void                OnLoaderGuiCancel(); //!< GUI callback
    void                OnLoaderGuiApply(RoR::LoaderType type, CacheEntryPtr entry, std::string sectionconfig);  //!< GUI callback

    /// @}
    /// @name Characters
    /// @{ 

    void                CreatePlayerCharacter(); //!< Terrain must be loaded
    Character*          GetPlayerCharacter();
    CharacterFactory*   GetCharacterFactory() { return &m_character_factory; }

    /// @}
    /// @name Savegames
    /// @{
    // (defined in Savegame.cpp)

    void                LoadScene(std::string const& filename); //!< Matching terrain must be already loaded
    void                SaveScene(std::string const& filename);
    std::string         GetQuicksaveFilename(); //!< For currently loaded terrain (cvar 'sim_terrain_name')
    std::string         ExtractSceneName(std::string const& filename);
    std::string         ExtractSceneTerrain(std::string const& filename); //!< Returns terrain filename
    void                HandleSavegameHotkeys();

    /// @}
    /// @name Gameplay feats (misc.)
    /// @{

    RaceSystem&         GetRaceSystem() { return m_race_system; }
    RepairMode&         GetRepairMode() { return m_recovery_mode; }
    SceneMouse&         GetSceneMouse() { return m_scene_mouse; }
    void                TeleportPlayer(float x, float z);
    void                UpdateGlobalInputEvents();
    void                UpdateSimInputEvents(float dt);
    void                UpdateSkyInputEvents(float dt);
    void                UpdateCommonInputEvents(float dt);
    void                UpdateAirplaneInputEvents(float dt);
    void                UpdateBoatInputEvents(float dt);
    void                UpdateTruckInputEvents(float dt);

    /// @}

private:
    // Message queue
    GameMsgQueue        m_msg_queue;
    Message*            m_msg_chain_end = nullptr;
    std::mutex          m_msg_mutex;

    // Terrain
    TerrainPtr          m_terrain;

    // Actors (physics and netcode)
    ActorManager        m_actor_manager;
    ActorPtr            m_player_actor;           //!< Actor (vehicle or machine) mounted and controlled by player
    ActorPtr            m_prev_player_actor;      //!< Previous actor (vehicle or machine) mounted and controlled by player
    ActorPtr            m_last_spawned_actor;     //!< Last actor spawned by user and still alive.
    
    CacheEntryPtr       m_last_cache_selection;   //!< Vehicle/load
    CacheEntryPtr       m_last_skin_selection;
    CacheEntryPtr       m_last_tuneup_selection;
    Ogre::String        m_last_section_config;
    ActorSpawnRequest   m_current_selection;                //!< Context of the loader UI
    CacheEntryPtr       m_dummy_cache_selection;

    // Characters (simplified physics and netcode)
    CharacterFactory    m_character_factory;

    // Gameplay feats (misc.)
    RaceSystem          m_race_system;
    RepairMode          m_recovery_mode;                     //!< Aka 'advanced repair' or 'interactive reset'
    SceneMouse          m_scene_mouse;                       //!< Mouse interaction with scene
    Ogre::Timer         m_timer;
    Ogre::Vector3       prev_pos = Ogre::Vector3::ZERO;
};

/// @} // addtogroup GameState

} // namespace RoR
