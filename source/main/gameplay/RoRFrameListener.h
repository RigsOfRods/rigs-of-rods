/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2015-2020 Petr Ohlidal

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

#include "Application.h"

#include "BeamData.h" // RoR::ActorSpawnRequest
#include "BeamFactory.h"
#include "CameraManager.h" // CameraManager::CameraBehaviors
#include "CharacterFactory.h"
#include "GfxScene.h"
#include "OutGauge.h"
#include "GUI_SceneMouse.h"

/// The simulation controller object
/// It's lifetime is tied to single gameplay session. When user returns to main menu, it's destroyed.
///
///
/// Architecture is currently undergoing a refactor.
///  OLD: We use threadpool; rendering loop runs on main thread, simulation is updated in between frames via 'Ogre::FrameListener' interface which also provides timing.
///   1. SimController::EnterGameplayLoop() invokes Ogre::Root::renderOneFrame() which invokes SimController::frameStarted()
///   2. Wait for simulation task to finish.
///   3. Gather user inputs; send/receive network data
///   3. Update OGRE scene; Put flexbody tasks to threadpool and wait for them to finish.
///   4. Run new sim. task: split time since last frame into constant size chunks and advance simulation
///   5. While sim. task is running, let main thread do the rendering
///
///  FUTURE: Use threadpool, run simulation loop on main thread and render in background.
///   1. Check time since last sim. update and sleep if we're up-to-date.
///   2. Check time since last render; if necessary, copy sim. data and put an "update scene and render" task to threadpool.
///   2. Gather user inputs; send/receive network data
///   3. Update simulation
class SimController: public ZeroedMemoryAllocator
{
public:
    SimController();

    // Actor management interface

    void   RemoveActorByCollisionBox(std::string const & ev_src_instance_name, std::string const & box_name); ///< Scripting utility. TODO: Does anybody use it? ~ only_a_ptr, 08/2017

    // Scripting interface
    float  getTime               () { return m_time; }

    bool                         IsPressurizingTyres() const { return m_pressure_pressed; }
    bool                         AreControlsLocked() const;
    bool                         IsGUIHidden()              { return m_hide_gui; }

    bool GetPhysicsPaused()                                 { return m_physics_simulation_paused; }
    void SetPhysicsPausedInternal(bool paused)              { m_physics_simulation_paused = paused; }

    void   UpdateSimulation(float dt);

private:

    void   UpdateInputEvents       (float dt);
    void   HideGUI                 (bool hidden);

    Ogre::Real               m_time_until_next_toggle; //!< just to stop toggles flipping too fast
    float                    m_last_simulation_speed;  //!< previously used time ratio between real time (evt.timeSinceLastFrame) and physics time ('dt' used in calcPhysics)
    bool                     m_is_pace_reset_pressed;
    float                    m_physics_simulation_time; //!< Amount of time the physics simulation is going to be advanced
    bool                     m_physics_simulation_paused;
    float                    m_time;
    bool                     m_hide_gui;
    bool                     m_pressure_pressed;

    bool                     m_advanced_vehicle_repair;
    float                    m_advanced_vehicle_repair_timer;

    float                    m_pressure_pressed_timer;
};
