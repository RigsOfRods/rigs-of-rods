/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#include <Ogre.h>

class RoRFrameListener: public Ogre::FrameListener, public Ogre::WindowEventListener, public ZeroedMemoryAllocator
{
    friend class RoR::MainThread; // Temporary hack

public:

    RoRFrameListener();
    virtual ~RoRFrameListener();

    bool frameEnded(const Ogre::FrameEvent& evt);
    bool frameStarted(const Ogre::FrameEvent& evt); // Override frameStarted event to process that (don't care about frameEnded)

    bool updateEvents(float dt);
    double getTime() { return m_time; }

    void hideGUI(bool hidden);

    void reloadCurrentTruck();

    void setDirectionArrow(char* text, Ogre::Vector3 position);

    void showLoad(int type, const Ogre::String& instance, const Ogre::String& box);
    void windowResized(Ogre::RenderWindow* rw); // TODO: make this private, it's public for legacy reasons.

    void StartRaceTimer();

    float StopRaceTimer();

    void UpdateRacingGui();

    bool IsRaceInProgress() { return m_race_in_progress; }

    void SetReloadPos(Ogre::Vector3 position) { m_reload_pos = position; }

protected:

    // WindowEventListener
    void windowMoved(Ogre::RenderWindow* rw);
    void windowClosed(Ogre::RenderWindow* rw);
    void windowFocusChange(Ogre::RenderWindow* rw);

    void updateForceFeedback(float dt);

    RoR::ForceFeedback m_forcefeedback;
    HeatHaze* m_heathaze;

    CacheEntry* m_last_cache_selection;
    Skin* m_last_skin_selection;
    std::vector<Ogre::String> m_last_vehicle_configs;

    Ogre::Real m_time_until_next_toggle; // just to stop toggles flipping too fast

    bool m_is_dir_arrow_visible;
    Ogre::Vector3 m_dir_arrow_pointed;

    float m_last_simulation_speed; // previously used time ratio between real time (evt.timeSinceLastFrame) and physics time ('dt' used in calcPhysics)

    int m_last_screenshot_id;
    Ogre::String m_last_screenshot_date;

    unsigned long m_race_start_time;
    bool m_race_in_progress;
    float m_race_bestlap_time;

    double m_time;

    bool m_hide_gui;
    bool m_truck_info_on;
    bool m_pressure_pressed;

    float m_netcheck_gui_timer;

    collision_box_t* m_reload_box;

    bool m_advanced_truck_repair;
    float m_advanced_truck_repair_timer;

    bool m_is_pace_reset_pressed;

    int m_stats_on;

    Ogre::Vector3 m_reload_pos;
    Ogre::Quaternion m_reload_dir;

    void finalizeTruckSpawning(Beam* local_truck, Beam* previous_truck);
};
