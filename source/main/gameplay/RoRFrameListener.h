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

#include "RoRPrerequisites.h"
#include "ForceFeedback.h"

#include "CharacterFactory.h"
#include "BeamFactory.h"

#include <Ogre.h>

namespace RoR { struct Terrn2Telepoint; } // Forward decl.

/// The simulation controller object (TODO: Rename this class to SimController)
/// It's lifetime is tied to single gameplay session. When user returns to main menu, it's destroyed.
///
/// RoR's gameplay is quite simple in structure, it consists of:
///  - static terrain:  static elevation map, managed by `TerrainManager` singleton.
///                     this includes static collision objects (or intrusion detection objects), managed by `TerrainObjectManager`.
///  - softbody actors: a.k.a "trucks" or "vehicles", managed by `BeamFactory`. They collide with static terrain and each other.
///                     this includes 'fixes' - actors with partially fixed position.
///  - characters:      player-controlled human figures with their own primitive physics, managed by `CharacterFactory`
///                     these only collide with static terrain, not actors.
/// For convenience and to help manage interactions, this class provides methods to manipulate these elements.
class RoRFrameListener: public Ogre::FrameListener, public Ogre::WindowEventListener, public ZeroedMemoryAllocator
{
    friend class RoR::BeamFactory; // Needed for function `ChangedCurrentVehicle()`. TODO: Eliminate ~ only_a_ptr, 06/2017
public:

    RoRFrameListener(RoR::ForceFeedback* ff, RoR::SkidmarkConfig* skid_conf);

    // Ogre::FrameListener public interface
    bool   frameStarted          (const Ogre::FrameEvent& evt) override;
    bool   frameEnded            (const Ogre::FrameEvent& evt) override;

    // Actor management interface
    size_t GetNumActors          () const                  { return m_beam_factory.getTruckCount(); }
    Beam*  GetActorById          (int actor_id)            { return m_beam_factory.getTruck(actor_id); }
    void   SetPlayerActorById    (int actor_id)            { m_beam_factory.setCurrentTruck(actor_id); } // TODO: Eliminate, use pointers ~ only_a_ptr, 06/2017
    void   SetPlayerActor        (Beam* actor)             { m_beam_factory.setCurrentTruck(actor->trucknum); }
    void   ReloadCurrentActor    ();

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

    RoR::BeamFactory*           GetBeamFactory  () { return &m_beam_factory; } // TODO: Eliminate this. All operations upon actors should be done through above methods. ~ only_a_ptr, 06/2017
    RoR::SkidmarkConfig*        GetSkidmarkConf () { return m_skidmark_conf; }

private:

    // Ogre::WindowEventListener interface
    void   windowMoved             (Ogre::RenderWindow* rw);
    void   windowClosed            (Ogre::RenderWindow* rw);
    void   windowFocusChange       (Ogre::RenderWindow* rw);
    void   windowResized           (Ogre::RenderWindow* rw);

    void   UpdateForceFeedback     (float dt);
    bool   UpdateInputEvents       (float dt);
    void   UpdateRacingGui         ();
    void   FinalizeTruckSpawning   (Beam* local_truck, Beam* previous_truck);
    void   HideGUI                 (bool hidden);
    void   CleanupAfterSimulation  (); /// Unloads all data

    // BeamFactory callback, requires a `friend` declaration above.
    //TODO: Eliminate this. This is one of the reasons for the "BeamFactory should not be accessible directly" guideline above ~ only_a_ptr, 06/2017
    void   ChangedCurrentVehicle (Beam* previous_vehicle, Beam* current_vehicle);

    RoR::BeamFactory         m_beam_factory;
    RoR::CharacterFactory    m_character_factory;
    HeatHaze*                m_heathaze;
    RoR::SkidmarkConfig*     m_skidmark_conf;
    Ogre::Real               m_time_until_next_toggle; ///< just to stop toggles flipping too fast
    float                    m_last_simulation_speed;  ///< previously used time ratio between real time (evt.timeSinceLastFrame) and physics time ('dt' used in calcPhysics)
    bool                     m_is_pace_reset_pressed;
    int                      m_stats_on;
    float                    m_netcheck_gui_timer;
    collision_box_t*         m_reload_box;
    double                   m_time;
    RoR::ForceFeedback*      m_force_feedback;
    bool                     m_hide_gui;
    bool                     m_was_app_window_closed;
    bool                     m_truck_info_on;
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

    bool                     m_advanced_truck_repair;
    float                    m_advanced_truck_repair_timer;

    Ogre::Vector3            m_reload_pos;
    Ogre::Quaternion         m_reload_dir;
};
