/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2015-2017 Petr Ohlidal & contributors

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

#include "BeamFactory.h"
#include "CharacterFactory.h"
#include "EnvironmentMap.h"
#include "ForceFeedback.h"
#include "RoRPrerequisites.h"


#include <Ogre.h>

/// The simulation controller object
/// It's lifetime is tied to single gameplay session. When user returns to main menu, it's destroyed.
///
/// RoR's gameplay is quite simple in structure, it consists of:
///  - static terrain:  static elevation map, managed by `TerrainManager` singleton.
///                     this includes static collision objects (or intrusion detection objects), managed by `TerrainObjectManager`.
///  - softbody actors: a.k.a "trucks" or "vehicles", managed by `ActorManager`. They collide with static terrain and each other.
///                     this includes 'fixes' - actors with partially fixed position.
///  - characters:      player-controlled human figures with their own primitive physics, managed by `CharacterFactory`
///                     these only collide with static terrain, not actors.
/// For convenience and to help manage interactions, this class provides methods to manipulate these elements.
class SimController: public Ogre::FrameListener, public Ogre::WindowEventListener, public ZeroedMemoryAllocator
{
public:
    SimController(RoR::ForceFeedback* ff, RoR::SkidmarkConfig* skid_conf);

    // Ogre::FrameListener public interface
    bool   frameStarted          (const Ogre::FrameEvent& evt) override;
    bool   frameRenderingQueued  (const Ogre::FrameEvent& evt) override;
    bool   frameEnded            (const Ogre::FrameEvent& evt) override;

    // Actor management interface
    Actor* GetActorById          (int actor_id)            { return m_actor_manager.GetActorByIdInternal(actor_id); }
    void   SetPlayerActorById    (int actor_id);                                                          // TODO: Eliminate, use pointers ~ only_a_ptr, 06/2017
    void   SetPlayerActor        (Actor* actor);
    Actor* GetPlayerActor        ()                        { return m_player_actor; }
    void   ReloadPlayerActor     ();
    void   RemovePlayerActor     ();
    void   RemoveActorByCollisionBox(std::string const & ev_src_instance_name, std::string const & box_name); ///< Scripting utility. TODO: Does anybody use it? ~ only_a_ptr, 08/2017

    // Scripting interface
    double getTime               () { return m_time; }
    void   UpdateDirectionArrow  (char* text, Ogre::Vector3 position);
    void   ShowLoaderGUI         (int type, const Ogre::String& instance, const Ogre::String& box);
    void   StartRaceTimer        ();
    float  StopRaceTimer         ();
    bool   LoadTerrain           (); ///< Reads GVar 'sim_terrain_pending'

    // GUI interface
    void   TeleportPlayer        (RoR::Terrn2Telepoint* telepoint); // Teleport UI
    void   TeleportPlayerXZ      (float x, float y); // Teleport UI

    /// @return True if everything was prepared OK and simulation may start.
    bool   SetupGameplayLoop     ();
    void   EnterGameplayLoop     ();

    RoR::ActorManager*           GetBeamFactory  ()         { return &m_actor_manager; } // TODO: Eliminate this. All operations upon actors should be done through above methods. ~ only_a_ptr, 06/2017
    RoR::SkidmarkConfig*        GetSkidmarkConf ()         { return m_skidmark_conf; }

private:

    // Ogre::WindowEventListener interface
    void   windowMoved             (Ogre::RenderWindow* rw);
    void   windowClosed            (Ogre::RenderWindow* rw);
    void   windowFocusChange       (Ogre::RenderWindow* rw);
    void   windowResized           (Ogre::RenderWindow* rw);

    int PreDefineSkyXExemple; //For predefined skyx weathers

    void   UpdateForceFeedback     (float dt);
    bool   UpdateInputEvents       (float dt);
    void   UpdateRacingGui         ();
    void   FinalizeActorSpawning   (Actor* local_actor, Actor* previous_actor);
    void   HideGUI                 (bool hidden);
    void   CleanupAfterSimulation  (); /// Unloads all data

    Actor*                   m_player_actor;           //!< Actor (vehicle or machine) mounted and controlled by player
    Actor*                   m_prev_player_actor;      //!< Previous actor (vehicle or machine) mounted and controlled by player
    RoR::ActorManager        m_actor_manager;
    RoR::CharacterFactory    m_character_factory;
    RoR::GfxEnvmap           m_gfx_envmap;
    HeatHaze*                m_heathaze;
    RoR::SkidmarkConfig*     m_skidmark_conf;
    Ogre::Real               m_time_until_next_toggle; //!< just to stop toggles flipping too fast
    float                    m_last_simulation_speed;  //!< previously used time ratio between real time (evt.timeSinceLastFrame) and physics time ('dt' used in calcPhysics)
    bool                     m_is_pace_reset_pressed;
    int                      m_stats_on;
    float                    m_netcheck_gui_timer;
    collision_box_t*         m_reload_box;
    double                   m_time;
    RoR::ForceFeedback*      m_force_feedback;
    bool                     m_hide_gui;
    bool                     m_was_app_window_closed;
    bool                     m_actor_info_gui_visible;
    bool                     m_pressure_pressed;

    CacheEntry*              m_last_cache_selection;
    RoR::SkinDef*            m_last_skin_selection;
    std::vector<std::string> m_last_vehicle_configs;

    bool                     m_is_dir_arrow_visible;
    Ogre::Vector3            m_dir_arrow_pointed;

    int                      m_last_screenshot_id;
    Ogre::String             m_last_screenshot_date;

    unsigned long            m_race_start_time;
    bool                     m_race_in_progress;
    float                    m_race_bestlap_time;

    bool                     m_advanced_vehicle_repair;
    float                    m_advanced_vehicle_repair_timer;

    Ogre::Vector3            m_reload_pos;
    Ogre::Quaternion         m_reload_dir;
};
