/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2021 Petr Ohlidal

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

#include "AutoPilot.h"
#include "CameraManager.h"
#include "Differentials.h"
#include "SimData.h"
#include <Ogre.h>

/*
    @file
    @brief  Sim buffers are snapshots of simulation state, which are used for updating visual scene.

    HOW IT WORKS AND WHY
    --------------------
    This game uses quite a lot of CPU-intensive computations when updating visuals.
    The biggest issue are flexbodies, submeshes and wheels, but also props and beams.
    To avoid halting simulation until all these slow updates are done, the process is divided:
    1. Simulation runs on it's own thread, at it's own pace (see PHYSICS_DT).
       It only updates positons and forces, it doesn't deal with graphics at all.
    2. When time comes for rendering, simulation is halted and all data relevant to graphics
       are copied-out to simbuffers. Then, simulation is resumed.
    3. The rendering thread processes the simbuffers and updates visual objects.

    OVERVIEW OF GAMEPLAY OBJECTS
    ----------------------------
    Following listing shows {SIMULATION / SIMBUFFER / GRAPHICAL} object triplets
    and filenames where the objects live (to be cleaned up).
    Note that for historical reasons, many simulation bits are actually done
    on main thread, but the goal is to do all on sim. thread.

        GameContext (gamecontext.h)  /  GameContextSB  /  GfxScene    (gfxscene.h)
        Actor       (actor.h)        /  ActorSB        /  GfxActor    (gfxactor.h)
        node_t      (simdata.h)      /  NodeSB         /  NodeGfx     (gfxdata.h)
        beam_t      (simdata.h)      /  -              /  BeamGfx         (gfxdata.h)
        command_t   (simdata.h)      /  CommandKeySB   /  -
        wheel_t     (simdata.h)      /  -              /  WheelGfx    (gfxdata.h)
        wing_t      (simdata.h)      /  -              /  -
        Turbojet    (turbojet.h)     /  AeroEngineSB   /  TurbojetVisual (turbojet.h)
        Turboprop   (turboprop.h)    /  AeroEngineSB   /  -
        Airbrake    (airbrake.h)     /  AirbrakeSB     /  AirbrakeGfx (gfxdata.h)
        Screwprop   (screwprop.h)    /  ScrewpropSB    /  -
*/

namespace RoR {

struct NodeSB
{
    Ogre::Vector3     AbsPosition; // classic name
    bool              nd_has_contact:1;
    bool              nd_is_wet:1;
};

struct ScrewpropSB
{
    float             simbuf_sp_rudder;
    float             simbuf_sp_throttle;
};

struct CommandKeySB
{
    float             simbuf_cmd_value;
};

struct AeroEngineSB
{
    AeroEngineType    simbuf_ae_type;
    float             simbuf_ae_rpm;
    float             simbuf_ae_rpmpc;
    float             simbuf_ae_throttle;
    float             simbuf_tp_aetorque; //!< Turboprop torque, used by animation "aetorque"
    float             simbuf_tp_aepitch;  //!< Turboprop pitch, used by animation "aepitch"
    float             simbuf_tj_ab_thrust; //! Turbojet afterburner
    float             simbuf_tj_exhaust_velo; //! Turbojet
    bool              simbuf_ae_ignition:1;
    bool              simbuf_ae_failed:1;
    bool              simbuf_tj_afterburn:1; //! Turbojet afterburner
};

struct AirbrakeSB
{
    float             simbuf_ab_ratio;
};

struct ActorSB
{
    // Attributes (not changing during gameplay).
    float             xa_speedo_highest_kph;
    bool              xa_speedo_use_engine_max_rpm;
    int               xa_num_gears; //!< Gearbox
    float             xa_engine_max_rpm;
    NodeNum_t         xa_camera0_pos_node;
    NodeNum_t         xa_camera0_roll_node;
    int               xa_driveable;
    bool              xa_has_autopilot;
    bool              xa_has_engine;
    Ogre::MaterialPtr xa_help_mat;
    Ogre::TexturePtr  xa_help_tex;

    // Gameplay state
    ActorState        simbuf_actor_state              = ActorState::LOCAL_SLEEPING;
    bool              simbuf_physics_paused           = false;
    int               simbuf_cur_cinecam              = 0;
    std::string       simbuf_net_username;
    int               simbuf_net_colornum;

    // Movement
    Ogre::Vector3     simbuf_pos                      = Ogre::Vector3::ZERO;
    Ogre::Vector3     simbuf_node0_velo               = Ogre::Vector3::ZERO;
    float             simbuf_rotation                 = 0;
    Ogre::Vector3     simbuf_direction                = Ogre::Vector3::ZERO; //!< Output of `Actor::getDirection()`
    float             simbuf_wheel_speed              = 0;
    float             simbuf_top_speed                = 0;
    Ogre::AxisAlignedBox simbuf_aabb                  = Ogre::AxisAlignedBox::BOX_NULL;

    // Elements
    std::vector<NodeSB>       simbuf_nodes;
    std::vector<ScrewpropSB>  simbuf_screwprops;
    std::vector<CommandKeySB> simbuf_commandkey;
    std::vector<AeroEngineSB> simbuf_aeroengines;
    std::vector<AirbrakeSB>   simbuf_airbrakes;

    // Drivetrain
    int               simbuf_gear                     = 0;
    int               simbuf_autoshift                = 0;    
    float             simbuf_engine_rpm               = 0;
    float             simbuf_engine_crankfactor       = 0;
    float             simbuf_engine_turbo_psi         = 0;
    float             simbuf_engine_accel             = 0;
    float             simbuf_engine_torque            = 0;
    float             simbuf_inputshaft_rpm           = 0;     // Land vehicle only
    float             simbuf_drive_ratio              = 0;     // Land vehicle only
    float             simbuf_hydro_dir_state          = 0;     // State of steering actuator ('hydro'), for steeringwheel display
    DiffType          simbuf_diff_type                = DiffType::SPLIT_DIFF;
    float             simbuf_brake                    = 0;
    float             simbuf_clutch                   = 0;

    // Tyre pressure
    float             simbuf_tyre_pressure            = 0;
    bool              simbuf_tyre_pressurizing        = false;  

    // Effects
    bool              simbuf_headlight_on             = 0;
    bool              simbuf_beaconlight_active       = false;
    bool              simbuf_smoke_enabled            = false;
    bool              simbuf_parking_brake            = false;

    // Aerial
    float             simbuf_hydro_aileron_state      = 0;
    float             simbuf_hydro_elevator_state     = 0;
    float             simbuf_hydro_aero_rudder_state  = 0;
    int               simbuf_aero_flap_state          = 0;
    int               simbuf_airbrake_state           = 0;
    float             simbuf_wing4_aoa                = 0;

    // Autopilot
    int               simbuf_ap_heading_mode          = Autopilot::HEADING_NONE;
    int               simbuf_ap_heading_value         = 0;
    int               simbuf_ap_alt_mode              = Autopilot::ALT_NONE;
    int               simbuf_ap_alt_value             = 1000; // from AutoPilot::reset()
    bool              simbuf_ap_ias_mode              = false;
    int               simbuf_ap_ias_value             = 150;  // from AutoPilot::reset()
    bool              simbuf_ap_gpws_mode             = false;
    bool              simbuf_ap_ils_available         = false;
    float             simbuf_ap_ils_vdev              = 0;
    float             simbuf_ap_ils_hdev              = 0;
    int               simbuf_ap_vs_value              = 0;
};

struct GameContextSB
{
    Actor*            simbuf_player_actor             = nullptr;
    Ogre::Vector3     simbuf_character_pos            = Ogre::Vector3::ZERO;
    bool              simbuf_sim_paused               = false;
    float             simbuf_sim_speed                = 1.f;
    CameraManager::CameraBehaviors
                      simbuf_camera_behavior          = CameraManager::CAMERA_BEHAVIOR_INVALID;

    // Race system
    float             simbuf_race_time                = 0.f;
    float             simbuf_race_best_time           = 0.f;
    float             simbuf_race_time_diff           = 0.f;
    bool              simbuf_race_in_progress         = false;
    bool              simbuf_race_in_progress_prev    = false;
    Ogre::Vector3     simbuf_dir_arrow_target         = Ogre::Vector3::ZERO;
    std::string       simbuf_dir_arrow_text;
    bool              simbuf_dir_arrow_visible        = false;
};

} // namespace RoR

