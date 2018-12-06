/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2017-2018 Petr Ohlidal & contributors

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

#include "CameraManager.h"

#include "Application.h"
#include "ApproxMath.h"
#include "Beam.h"
#include "BeamFactory.h"
#include "Character.h"
#include "Collisions.h"
#include "GUI_GameConsole.h"
#include "InputEngine.h"
#include "Language.h"
#include "OverlayWrapper.h"
#include "RoRFrameListener.h"
#include "TerrainManager.h"
#include "GUIManager.h"
#include "PerVehicleCameraContext.h"

// ========== Project 'SimCam' (started June 2018) ==========
// - Eliminate 'gEnv->mainCamera' (pointer to Ogre::Camera)
//       because it shouldn't be updated and read from externally throughout the simulation cycle;
//       instead, it should only be updated internally prior to actual rendering.
// - [Done] Eliminate 'GlobalEnvironment::cameraManager' (RoR object)
//       because it serves ad-hoc camera updates throughout the simulation cycle
//       instead, camera should be updated at the end of sim. cycle from the resulting state;
//       locking controls based on camera mode (free camera) should be governed by SimController instead
// - Put camera manager under SimController
//       because camera state is part of sim. state,
//       also it will help future refactors - current CameraManager does many things it shouldn't
//       (it's own input processing, manipulating controls...)

// ==== gEnv->mainCamera RESEARCH ====
//   Character.cpp:                   getPosition()
//   SceneMouse.cpp:                  getViewport()
//   DepthOfFieldEffect.cpp:          getFOVy, setFOVy, getPosition, getViewport
//   EnvironmentMap.cpp:              setFarClipDistance, getFarClipDistance, getBackgroundColour
//   GfxActor.cpp:                    getPosition(), 
//   Heathaze.cpp:                    get/addViewport()
//   HydraxWater.cpp, skyxManager:    getDerivedPosition()
//   Scripting:                        setPosition  setDirection  setOrientation yaw  pitch  roll  getPosition  getDirection  getOrientation  lookAt
//   SimController:                   getUp()

using namespace Ogre;
using namespace RoR;

static const Ogre::Vector3 CHARACTERCAM_OFFSET_1ST_PERSON(0.0f, 1.82f, 0.0f);
static const Ogre::Vector3 CHARACTERCAM_OFFSET_3RD_PERSON(0.0f, 1.1f, 0.0f);
static const int           SPLINECAM_DRAW_RESOLUTION = 200;
static const int           DEFAULT_INTERNAL_CAM_PITCH = -15;
static const float         TRANS_SPEED = 50.f;
static const float         ROTATE_SPEED = 100.f;

bool intersectsTerrain(Vector3 a, Vector3 b) // internal helper
{
    int steps = std::max(10.0f, a.distance(b));
    for (int i = 0; i < steps; i++)
    {
        Vector3 pos = a + (b - a) * (float)i / steps;
        float y = a.y + (b.y - a.y) * (float)i / steps;
        float h = App::GetSimTerrain()->GetHeightAt(pos.x, pos.z);
        if (h > y)
        {
            return true;
        }
    }
    return false;
}

CameraManager::CameraManager() :
      m_current_behavior(CAMERA_BEHAVIOR_INVALID)
    , m_cct_dt(0.0f)
    , m_cct_trans_scale(1.0f)
    , m_cct_sim_speed(1.0f)
    , m_cam_before_toggled(CAMERA_BEHAVIOR_INVALID)
    , m_prev_toggled_cam(CAMERA_BEHAVIOR_INVALID)
    , m_charactercam_is_3rdperson(true)
    , m_splinecam_num_linked_beams(0)
    , m_splinecam_auto_tracking(false)
    , m_splinecam_spline(new SimpleSpline())
    , m_splinecam_spline_closed(false)
    , m_splinecam_spline_len(1.0f)
    , m_splinecam_mo(0)
    , m_splinecam_spline_pos(0.5f)
    , m_cam_rot_x(0.0f)
    , m_cam_rot_y(0.3f)
    , m_cam_dist(5.f)
    , m_cam_dist_min(0.f)
    , m_cam_dist_max(0.f)
    , m_cam_target_direction(0.f)
    , m_cam_target_pitch(0.f)
    , m_cam_ratio (11.f)
    , m_cam_look_at(Ogre::Vector3::ZERO)
    , m_cam_look_at_last(Ogre::Vector3::ZERO)
    , m_cam_look_at_smooth(Ogre::Vector3::ZERO)
    , m_cam_look_at_smooth_last(Ogre::Vector3::ZERO)
    , m_cam_limit_movement(true)
    , m_camera_ready(false)
{
    m_cct_player_actor = nullptr;

    m_staticcam_update_timer.reset();
}

CameraManager::~CameraManager()
{
    if (m_splinecam_spline)
        delete m_splinecam_spline;
    if (m_splinecam_mo)
        delete m_splinecam_mo;
}

bool CameraManager::EvaluateSwitchBehavior()
{
    switch(m_current_behavior)
    {
    case CAMERA_BEHAVIOR_CHARACTER: {
        if (m_charactercam_is_3rdperson)
        {
            m_charactercam_is_3rdperson = false;
            this->ResetCurrentBehavior();
            return false;
        }
        else // first person
        {
            m_charactercam_is_3rdperson = true;
            return true;
        }
    }
    case CAMERA_BEHAVIOR_STATIC:          return true;
    case CAMERA_BEHAVIOR_VEHICLE:         return true;
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  return true;
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: {
        if ( (m_cct_player_actor != nullptr)
            && (m_cct_player_actor->ar_current_cinecam) < (m_cct_player_actor->ar_num_cinecams-1) )
        {
            m_cct_player_actor->ar_current_cinecam++;
            m_cct_player_actor->NotifyActorCameraChanged();
            return false;
        }
        return true;
    }
    case CAMERA_BEHAVIOR_FREE:            return true;
    case CAMERA_BEHAVIOR_FIXED:           return true;
    case CAMERA_BEHAVIOR_ISOMETRIC:       return true;
    case CAMERA_BEHAVIOR_INVALID:         return true;
    default:                              return false;
    }
}

void CameraManager::UpdateCurrentBehavior()
{
    switch(m_current_behavior)
    {
    case CAMERA_BEHAVIOR_CHARACTER: {
        if (!gEnv->player)
            return;
        m_cam_target_direction = -gEnv->player->getRotation() - Radian(Math::HALF_PI);
        Ogre::Vector3 offset = (!m_charactercam_is_3rdperson) ? CHARACTERCAM_OFFSET_1ST_PERSON : CHARACTERCAM_OFFSET_3RD_PERSON;
        m_cam_look_at = gEnv->player->getPosition() + offset;

        CameraManager::CameraBehaviorOrbitUpdate();
        return;
    }

    case CAMERA_BEHAVIOR_STATIC:
        this->UpdateCameraBehaviorStatic();
        return;

    case CAMERA_BEHAVIOR_VEHICLE:         this->UpdateCameraBehaviorVehicle();  return;
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  this->CameraBehaviorVehicleSplineUpdate();  return;
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: {
        CameraManager::CameraBehaviorOrbitUpdate();

        int pos_node  = m_cct_player_actor->ar_camera_node_pos [m_cct_player_actor->ar_current_cinecam];
        int dir_node  = m_cct_player_actor->ar_camera_node_dir [m_cct_player_actor->ar_current_cinecam];
        int roll_node = m_cct_player_actor->ar_camera_node_roll[m_cct_player_actor->ar_current_cinecam];

        Vector3 dir  = (m_cct_player_actor->ar_nodes[pos_node].AbsPosition
                - m_cct_player_actor->ar_nodes[dir_node].AbsPosition).normalisedCopy();
        Vector3 roll = (m_cct_player_actor->ar_nodes[pos_node].AbsPosition
                - m_cct_player_actor->ar_nodes[roll_node].AbsPosition).normalisedCopy();

        if ( m_cct_player_actor->ar_camera_node_roll_inv[m_cct_player_actor->ar_current_cinecam] )
        {
            roll = -roll;
        }

        Vector3 up = dir.crossProduct(roll);
        roll = up.crossProduct(dir);

        Quaternion orientation = Quaternion(m_cam_rot_x, up) * Quaternion(Degree(180.0) + m_cam_rot_y, roll) * Quaternion(roll, up, dir);

        gEnv->mainCamera->setPosition(m_cct_player_actor->ar_nodes[m_cct_player_actor->ar_cinecam_node[m_cct_player_actor->ar_current_cinecam]].AbsPosition);
        gEnv->mainCamera->setOrientation(orientation);
        return;
    }
    case CAMERA_BEHAVIOR_FREE:            this->UpdateCameraBehaviorFree(); return;
    case CAMERA_BEHAVIOR_FIXED:           return;
    case CAMERA_BEHAVIOR_ISOMETRIC:       return;
    case CAMERA_BEHAVIOR_INVALID:         return;
    default:                              return;
    }
}

bool CameraManager::Update(float dt, Actor* player_vehicle, float sim_speed) // Called every frame
{
    if (RoR::App::sim_state.GetActive() == RoR::SimState::PAUSED) { return true; } // Do nothing when paused
    const float trans_scale = TRANS_SPEED  * dt;
    const float rot_scale   = ROTATE_SPEED * dt;

    m_cct_player_actor = player_vehicle;
    m_cct_sim_speed    = sim_speed;
    m_cct_dt           = dt;
    m_cct_rot_scale    = Degree(rot_scale);
    m_cct_trans_scale  = trans_scale;
    m_cct_fov_interior = Degree(App::gfx_fov_internal.GetActive()); // TODO: don't copy Gvar!
    m_cct_fov_exterior = Degree(App::gfx_fov_external.GetActive());

    if ( m_current_behavior < CAMERA_BEHAVIOR_END && RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_CHANGE) )
    {
        if ( (m_current_behavior == CAMERA_BEHAVIOR_INVALID) || this->EvaluateSwitchBehavior() )
        {
            this->switchToNextBehavior();
        }
    }

    if ( RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_FREE_MODE_FIX) )
    {
        this->ToggleCameraBehavior(CAMERA_BEHAVIOR_FIXED);
    }

    if ( RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_FREE_MODE) )
    {
        this->ToggleCameraBehavior(CAMERA_BEHAVIOR_FREE);
    }

    if (m_current_behavior != CAMERA_BEHAVIOR_INVALID)
    {
        this->UpdateCurrentBehavior();
    }
    else
    {
        switchBehavior(CAMERA_BEHAVIOR_CHARACTER);
    }

    return true;
}

void CameraManager::switchToNextBehavior()
{
    int i = (static_cast<int>(m_current_behavior) + 1) % CAMERA_BEHAVIOR_END;
    this->switchBehavior(static_cast<CameraBehaviors>(i));
}

void CameraManager::ResetCurrentBehavior()
{
    switch(m_current_behavior)
    {
    case CAMERA_BEHAVIOR_CHARACTER: {
        CameraManager::CameraBehaviorOrbitReset();

        // Vars from CameraBehaviorOrbit
        if (!m_charactercam_is_3rdperson)
        {
            m_cam_rot_y = 0.1f;
            m_cam_dist = 0.1f;
            m_cam_ratio = 0.0f;
        }
        else
        {
            m_cam_rot_y = 0.3f;
            m_cam_dist = 5.0f;
            m_cam_ratio = 11.0f;
        }
        m_cam_dist_min = 0;
	m_cam_target_pitch = 0.0f;
        return;
    }

    case CAMERA_BEHAVIOR_STATIC:
        return;

    case CAMERA_BEHAVIOR_VEHICLE:
        this->CameraBehaviorVehicleReset();
        return;

    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  this->CameraBehaviorVehicleSplineReset();  return;
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM:
        CameraManager::CameraBehaviorOrbitReset();
        m_cam_rot_y = Degree(DEFAULT_INTERNAL_CAM_PITCH);
        gEnv->mainCamera->setFOVy(m_cct_fov_interior);
        return;

    case CAMERA_BEHAVIOR_FREE:            return;
    case CAMERA_BEHAVIOR_FIXED:           return;
    case CAMERA_BEHAVIOR_ISOMETRIC:       return;
    case CAMERA_BEHAVIOR_INVALID:         return;
    default:                              return;
    }
}

void CameraManager::ActivateNewBehavior(CameraBehaviors new_behavior, bool reset)
{
    // Assign new behavior
    m_current_behavior = new_behavior;

    // Resolve per-behavior constraints and actions
    switch (new_behavior)
    {
    case CAMERA_BEHAVIOR_FREE:
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Free camera"), "camera_go.png", 3000);
        RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Free camera"));
        break;

    case CAMERA_BEHAVIOR_ISOMETRIC:
    case CAMERA_BEHAVIOR_FIXED:
        RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_INFO, RoR::Console::CONSOLE_SYSTEM_NOTICE, _L("Fixed camera"), "camera_link.png", 3000);
        RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Fixed camera"));
        break;

    case CAMERA_BEHAVIOR_STATIC:
        m_staticcam_previous_fov = gEnv->mainCamera->getFOVy();
        break;

    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:
        if ( (m_cct_player_actor == nullptr) || m_cct_player_actor->ar_num_camera_rails <= 0)
        {
            this->switchToNextBehavior();
            return;
        }
        else if (reset)
        {
            this->CameraBehaviorVehicleSplineReset();
            this->CameraBehaviorVehicleSplineCreateSpline();
        }
        m_cct_player_actor->GetCameraContext()->behavior = RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_SPLINE;
        break;

    case CAMERA_BEHAVIOR_VEHICLE_CINECAM:
        if ((m_cct_player_actor == nullptr) || (m_cct_player_actor->ar_num_cinecams <= 0))
        {
            this->switchToNextBehavior();
            return;
        }
        else if ( reset )
        {
            this->ResetCurrentBehavior();
        }

        gEnv->mainCamera->setFOVy(m_cct_fov_interior);

        m_cct_player_actor->prepareInside(true);

        if ( RoR::App::GetOverlayWrapper() != nullptr )
        {
            bool visible = m_cct_player_actor->ar_driveable == AIRPLANE && !RoR::App::GetSimController()->IsGUIHidden();
            RoR::App::GetOverlayWrapper()->showDashboardOverlays(visible, m_cct_player_actor);
        }

        m_cct_player_actor->ar_current_cinecam = std::max(0, m_cct_player_actor->ar_current_cinecam);
        m_cct_player_actor->NotifyActorCameraChanged();

        m_cct_player_actor->GetCameraContext()->behavior = RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_CINECAM;
        break;

    case CAMERA_BEHAVIOR_VEHICLE:
        if ( m_cct_player_actor == nullptr )
        {
            this->switchToNextBehavior();
            return;
        }
        else if ( reset )
        {
            this->ResetCurrentBehavior();
        }
        m_cct_player_actor->GetCameraContext()->behavior = RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_3rdPERSON;
        break;

    case CAMERA_BEHAVIOR_CHARACTER:
        if (m_cct_player_actor != nullptr)
        {
            this->switchToNextBehavior();
            return;
        }
        else if (reset)
        {
            this->ResetCurrentBehavior();
        }
        break;

    default:;
    }
}

void CameraManager::DeactivateCurrentBehavior()
{
    if (m_current_behavior == CAMERA_BEHAVIOR_STATIC)
    {
        gEnv->mainCamera->setFOVy(m_staticcam_previous_fov);
    }
    else if (m_current_behavior == CAMERA_BEHAVIOR_VEHICLE_CINECAM)
    {
        if ( m_cct_player_actor != nullptr )
        {
            gEnv->mainCamera->setFOVy(m_cct_fov_exterior);
            m_cct_player_actor->prepareInside(false);
            m_cct_player_actor->NotifyActorCameraChanged();
        }
    }
}

void CameraManager::switchBehavior(CameraBehaviors new_behavior)
{
    if (new_behavior == m_current_behavior)
    {
        return;
    }

    this->DeactivateCurrentBehavior();

    if (m_cct_player_actor != nullptr)
    {
        m_cct_player_actor->GetCameraContext()->behavior = RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_EXTERNAL;
        if ( RoR::App::GetOverlayWrapper() != nullptr && !RoR::App::GetSimController()->IsGUIHidden() )
        {
            RoR::App::GetOverlayWrapper()->showDashboardOverlays(true, m_cct_player_actor);
        }
        if (m_current_behavior == CAMERA_BEHAVIOR_VEHICLE_CINECAM)
        {
            m_cct_player_actor->ar_current_cinecam = -1;
        }
    }

    this->ActivateNewBehavior(new_behavior, true);
}

void CameraManager::SwitchBehaviorOnVehicleChange(CameraBehaviors new_behavior, Actor* new_vehicle)
{
    if (new_behavior == m_current_behavior)
    {
        this->NotifyContextChange();
    }

    this->DeactivateCurrentBehavior();

    m_cct_player_actor = new_vehicle;

    this->ActivateNewBehavior(new_behavior, new_behavior != m_current_behavior);
}

bool CameraManager::hasActiveBehavior()
{
    return m_current_behavior != CAMERA_BEHAVIOR_INVALID;
}

int CameraManager::getCurrentBehavior()
{
    return static_cast<int>(m_current_behavior);
}

bool CameraManager::mouseMoved(const OIS::MouseEvent& _arg)
{
    switch(m_current_behavior)
    {
    case CAMERA_BEHAVIOR_CHARACTER: {
        if (!gEnv->player)
            return false;
        if (!m_charactercam_is_3rdperson)
        {
            const OIS::MouseState ms = _arg.state;
            Radian angle = gEnv->player->getRotation();

            m_cam_rot_y += Degree(ms.Y.rel * 0.13f);
            angle += Degree(ms.X.rel * 0.13f);

            m_cam_rot_y = Radian(std::min(+Math::HALF_PI * 0.65f, m_cam_rot_y.valueRadians()));
            m_cam_rot_y = Radian(std::max(m_cam_rot_y.valueRadians(), -Math::HALF_PI * 0.9f));

            gEnv->player->setRotation(angle);

            RoR::App::GetGuiManager()->SetMouseCursorVisibility(RoR::GUIManager::MouseCursorVisibility::HIDDEN);

            return true;
        }

        return CameraManager::CameraBehaviorOrbitMouseMoved(_arg);
    }
    case CAMERA_BEHAVIOR_STATIC:          return false;
    case CAMERA_BEHAVIOR_VEHICLE:         return CameraBehaviorOrbitMouseMoved(_arg);
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  return this->CameraBehaviorVehicleSplineMouseMoved(_arg);
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: return CameraBehaviorOrbitMouseMoved(_arg);
    case CAMERA_BEHAVIOR_FREE: {
        const OIS::MouseState ms = _arg.state;

        gEnv->mainCamera->yaw(Degree(-ms.X.rel * 0.13f));
        gEnv->mainCamera->pitch(Degree(-ms.Y.rel * 0.13f));

        App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::HIDDEN);

        return true;
    }

    case CAMERA_BEHAVIOR_FIXED:           return false;
    case CAMERA_BEHAVIOR_ISOMETRIC:       return false;
    case CAMERA_BEHAVIOR_INVALID:         return false;
    default:                              return false;
    }
}

bool CameraManager::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    switch(m_current_behavior)
    {
    case CAMERA_BEHAVIOR_CHARACTER:       return false;
    case CAMERA_BEHAVIOR_STATIC:          return false;
    case CAMERA_BEHAVIOR_VEHICLE:         return this->CameraBehaviorVehicleMousePressed(_arg, _id);
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  return this->CameraBehaviorVehicleMousePressed(_arg, _id);
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: return this->CameraBehaviorVehicleMousePressed(_arg, _id);
    case CAMERA_BEHAVIOR_FREE:            return false;
    case CAMERA_BEHAVIOR_FIXED:           return false;
    case CAMERA_BEHAVIOR_ISOMETRIC:       return false;
    case CAMERA_BEHAVIOR_INVALID:         return false;
    default:                              return false;
    }
}

bool CameraManager::gameControlsLocked() const
{
    // game controls are only disabled in free camera mode for now
    return (m_current_behavior == CAMERA_BEHAVIOR_FREE);
}

void CameraManager::NotifyContextChange()
{
    switch(m_current_behavior)
    {
    case CAMERA_BEHAVIOR_CHARACTER:
    case CAMERA_BEHAVIOR_VEHICLE:
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM:
        m_cam_look_at_last = Vector3::ZERO;
        return;

    default:
        return;
    }
}

void CameraManager::NotifyVehicleChanged(Actor* old_vehicle, Actor* new_vehicle)
{
    // Getting out of vehicle
    if (new_vehicle == nullptr)
    {
        m_cct_player_actor = nullptr;
        if (this->m_current_behavior != CAMERA_BEHAVIOR_FIXED)
        {
            this->switchBehavior(CAMERA_BEHAVIOR_CHARACTER);
        }
        return;
    }

    // Getting in vehicle
    if (this->m_current_behavior != CAMERA_BEHAVIOR_FIXED)
    {
        // Change camera
        switch (new_vehicle->GetCameraContext()->behavior)
        {
        case RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_3rdPERSON:
            this->SwitchBehaviorOnVehicleChange(CAMERA_BEHAVIOR_VEHICLE, new_vehicle);
            break;

        case RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_SPLINE:
            this->SwitchBehaviorOnVehicleChange(CAMERA_BEHAVIOR_VEHICLE_SPLINE, new_vehicle);
            break;

        case RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_CINECAM:
            this->SwitchBehaviorOnVehicleChange(CAMERA_BEHAVIOR_VEHICLE_CINECAM, new_vehicle);
            break;

        default:
            this->SwitchBehaviorOnVehicleChange(CAMERA_BEHAVIOR_VEHICLE, new_vehicle);
        }
    }
}

void CameraManager::ToggleCameraBehavior(CameraBehaviors new_behavior) // Only accepts FREE and FREEFIX modes
{
    assert(new_behavior == CAMERA_BEHAVIOR_FIXED || new_behavior == CAMERA_BEHAVIOR_FREE);

    if (m_current_behavior == new_behavior) // Leaving toggled mode?
    {
        if (m_prev_toggled_cam != CAMERA_BEHAVIOR_INVALID)
        {
            this->switchBehavior(m_prev_toggled_cam);
            m_prev_toggled_cam = CAMERA_BEHAVIOR_INVALID;
        }
        else if (m_cam_before_toggled != CAMERA_BEHAVIOR_INVALID)
        {
            this->switchBehavior(m_cam_before_toggled);
            m_cam_before_toggled = CAMERA_BEHAVIOR_INVALID;
        }
    }
    else // Entering toggled mode
    {
        if (m_cam_before_toggled == CAMERA_BEHAVIOR_INVALID)
        {
            m_cam_before_toggled = m_current_behavior;
        }
        else
        {
            m_prev_toggled_cam = m_current_behavior;
        }
        this->switchBehavior(new_behavior);
    }
}

void CameraManager::UpdateCameraBehaviorStatic()
{
    Vector3 lookAt = Vector3::ZERO;
    Vector3 velocity = Vector3::ZERO;
    Radian angle = Degree(90);
    float rotation = 0.0f;
    float speed = 0.0f;

    if (m_cct_player_actor)
    {
        lookAt = m_cct_player_actor->getPosition();
        rotation = m_cct_player_actor->getRotation();
        velocity = m_cct_player_actor->ar_nodes[0].Velocity * m_cct_sim_speed;
        angle = (lookAt - m_staticcam_position).angleBetween(velocity);
        speed = velocity.length();
        if (m_cct_player_actor->ar_replay_mode)
        {
            speed *= m_cct_player_actor->ar_replay_precision;
        }
    }
    else
    {
        lookAt = gEnv->player->getPosition();
        rotation = gEnv->player->getRotation().valueRadians();
    }

    bool forceUpdate = RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_RESET, 2.0f);
    forceUpdate = forceUpdate || (m_staticcam_position.distance(lookAt) > 200.0f && speed < 1.0f);

    if (forceUpdate || m_staticcam_update_timer.getMilliseconds() > 2000)
    {
        float distance = m_staticcam_position.distance(lookAt);
        bool terrainIntersection = intersectsTerrain(m_staticcam_position, lookAt + Vector3::UNIT_Y) || intersectsTerrain(m_staticcam_position, lookAt + velocity + Vector3::UNIT_Y);

        if (forceUpdate || terrainIntersection || distance > std::max(75.0f, speed * 3.5f) || (distance > 25.0f && angle < Degree(30)))
        {
            if (speed < 0.1f)
            {
                velocity = Vector3(cos(rotation), 0.0f, sin(rotation));
            }
            speed = std::max(5.0f, speed);
            m_staticcam_position = lookAt + velocity.normalisedCopy() * speed * 3.0f;
            Vector3 offset = (velocity.crossProduct(Vector3::UNIT_Y)).normalisedCopy() * speed;
            for (int i = 0; i < 100; i++)
            {
                Vector3 pos = m_staticcam_position + offset * frand_11();
                float h = App::GetSimTerrain()->GetHeightAt(pos.x, pos.z);
                pos.y = std::max(h, pos.y);
                if (!intersectsTerrain(pos, lookAt + Vector3::UNIT_Y))
                {
                    m_staticcam_position = pos;
                    break;
                }
            }
            m_staticcam_position += offset * frand_11();

            float h = App::GetSimTerrain()->GetHeightAt(m_staticcam_position.x, m_staticcam_position.z);
            m_staticcam_position.y = std::max(h, m_staticcam_position.y) + 5.0f;

            m_staticcam_update_timer.reset();
        }
    }

    float camDist = m_staticcam_position.distance(lookAt);
    float fov = atan2(20.0f, camDist);

    gEnv->mainCamera->setPosition(m_staticcam_position);
    gEnv->mainCamera->lookAt(lookAt);
    gEnv->mainCamera->setFOVy(Radian(fov));
}

void CameraManager::CameraBehaviorOrbitUpdate()
{
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_LOOKBACK))
    {
        if (m_cam_rot_x > Degree(0))
        {
            m_cam_rot_x = Degree(0);
        }
        else
        {
            m_cam_rot_x = Degree(180);
        }
    }

    m_cam_rot_x += (RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_RIGHT) - RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_LEFT)) * m_cct_rot_scale;
    m_cam_rot_y += (RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_UP) - RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_DOWN)) * m_cct_rot_scale;

    m_cam_rot_y = std::max((Radian)Degree(-80), m_cam_rot_y);
    m_cam_rot_y = std::min(m_cam_rot_y, (Radian)Degree(88));

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_IN) && m_cam_dist > 1)
    {
        m_cam_dist -= m_cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_IN_FAST) && m_cam_dist > 1)
    {
        m_cam_dist -= m_cct_trans_scale * 10;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_OUT))
    {
        m_cam_dist += m_cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_OUT_FAST))
    {
        m_cam_dist += m_cct_trans_scale * 10;
    }

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_RESET))
    {
        ResetCurrentBehavior();
    }

    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT) && RoR::App::GetInputEngine()->isKeyDownValueBounce(OIS::KC_SPACE))
    {
        m_cam_limit_movement = !m_cam_limit_movement;
        if (m_cam_limit_movement)
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Limited camera movement enabled"), "camera_go.png", 3000);
            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Limited camera movement enabled"));
        }
        else
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Limited camera movement disabled"), "camera_go.png", 3000);
            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Limited camera movement disabled"));
        }
    }

    if (m_cam_limit_movement && m_cam_dist_min > 0.0f)
    {
        m_cam_dist = std::max(m_cam_dist_min, m_cam_dist);
    }
    if (m_cam_limit_movement && m_cam_dist_max > 0.0f)
    {
        m_cam_dist = std::min(m_cam_dist, m_cam_dist_max);
    }

    m_cam_dist = std::max(0.0f, m_cam_dist);

    Vector3 desiredPosition = m_cam_look_at + m_cam_dist * 0.5f * Vector3(
        sin(m_cam_target_direction.valueRadians() + m_cam_rot_x.valueRadians()) * cos(m_cam_target_pitch.valueRadians() + m_cam_rot_y.valueRadians())
        , sin(m_cam_target_pitch.valueRadians() + m_cam_rot_y.valueRadians())
        , cos(m_cam_target_direction.valueRadians() + m_cam_rot_x.valueRadians()) * cos(m_cam_target_pitch.valueRadians() + m_cam_rot_y.valueRadians())
    );

    if (m_cam_limit_movement)
    {
        float h = App::GetSimTerrain()->GetHeightAt(desiredPosition.x, desiredPosition.z) + 1.0f;

        desiredPosition.y = std::max(h, desiredPosition.y);
    }

    if (m_cam_look_at_last == Vector3::ZERO)
    {
        m_cam_look_at_last = m_cam_look_at;
    }
    if (m_cam_look_at_smooth == Vector3::ZERO)
    {
        m_cam_look_at_smooth = m_cam_look_at;
    }
    if (m_cam_look_at_smooth_last == Vector3::ZERO)
    {
        m_cam_look_at_smooth_last = m_cam_look_at_smooth;
    }

    Vector3 camDisplacement = m_cam_look_at - m_cam_look_at_last;
    Vector3 precedingLookAt = m_cam_look_at_smooth_last + camDisplacement;
    Vector3 precedingPosition = gEnv->mainCamera->getPosition() + camDisplacement;

    Vector3 camPosition = (1.0f / (m_cam_ratio + 1.0f)) * desiredPosition + (m_cam_ratio / (m_cam_ratio + 1.0f)) * precedingPosition;

    if (gEnv->collisions && gEnv->collisions->forcecam)
    {
        gEnv->mainCamera->setPosition(gEnv->collisions->forcecampos);
        gEnv->collisions->forcecam = false;
    }
    else
    {
        if (m_cct_player_actor && m_cct_player_actor->ar_replay_mode && camDisplacement != Vector3::ZERO)
            gEnv->mainCamera->setPosition(desiredPosition);
        else
            gEnv->mainCamera->setPosition(camPosition);
    }

    m_cam_look_at_smooth = (1.0f / (m_cam_ratio + 1.0f)) * m_cam_look_at + (m_cam_ratio / (m_cam_ratio + 1.0f)) * precedingLookAt;

    m_cam_look_at_last = m_cam_look_at;
    m_cam_look_at_smooth_last = m_cam_look_at_smooth;
    gEnv->mainCamera->lookAt(m_cam_look_at_smooth);
}

bool CameraManager::CameraBehaviorOrbitMouseMoved(const OIS::MouseEvent& _arg)
{
    const OIS::MouseState ms = _arg.state;

    if (ms.buttonDown(OIS::MB_Right))
    {
        float scale = RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.002f : 0.02f;
        m_cam_rot_x += Degree(ms.X.rel * 0.13f);
        m_cam_rot_y += Degree(-ms.Y.rel * 0.13f);
        m_cam_dist += -ms.Z.rel * scale;
        return true;
    }

    return false;
}

void CameraManager::CameraBehaviorOrbitReset()
{
    m_cam_rot_x = 0.0f;
    m_cam_rot_y = 0.3f;
    m_cam_look_at_last = Vector3::ZERO;
    m_cam_look_at_smooth = Vector3::ZERO;
    m_cam_look_at_smooth_last = Vector3::ZERO;
    gEnv->mainCamera->setFOVy(m_cct_fov_exterior);
}

void CameraManager::UpdateCameraBehaviorFree()
{
    Degree mRotX(0.0f);
    Degree mRotY(0.0f);
    Degree cct_rot_scale(m_cct_rot_scale * 10.0f * m_cct_dt);
    Vector3 mTrans(Vector3::ZERO);
    Real cct_trans_scale(m_cct_trans_scale * 10.0f * m_cct_dt);

    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) || RoR::App::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT))
    {
        cct_rot_scale *= 3.0f;
        cct_trans_scale *= 3.0f;
    }
    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LCONTROL))
    {
        cct_rot_scale *= 6.0f;
        cct_trans_scale *= 20.0f;
    }
    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU))
    {
        cct_rot_scale *= 0.1f;
        cct_trans_scale *= 0.1f;
    }

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_LEFT))
    {
        mTrans.x -= cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_RIGHT))
    {
        mTrans.x += cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_FORWARD))
    {
        mTrans.z -= cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_BACKWARDS))
    {
        mTrans.z += cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_UP))
    {
        mTrans.y += cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_DOWN))
    {
        mTrans.y -= cct_trans_scale;
    }

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_RIGHT))
    {
        mRotX -= cct_rot_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_LEFT))
    {
        mRotX += cct_rot_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_ROT_UP))
    {
        mRotY += cct_rot_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_ROT_DOWN))
    {
        mRotY -= cct_rot_scale;
    }

    gEnv->mainCamera->yaw(mRotX);
    gEnv->mainCamera->pitch(mRotY);

    Vector3 camPosition = gEnv->mainCamera->getPosition() + gEnv->mainCamera->getOrientation() * mTrans.normalisedCopy() * cct_trans_scale;

    gEnv->mainCamera->setPosition(camPosition);
}

void CameraManager::UpdateCameraBehaviorVehicle()
{
	Vector3 dir = m_cct_player_actor->getDirection();

	m_cam_target_direction = -atan2(dir.dotProduct(Vector3::UNIT_X), dir.dotProduct(-Vector3::UNIT_Z));
	m_cam_target_pitch     = 0.0f;

	if ( RoR::App::gfx_extcam_mode.GetActive() == RoR::GfxExtCamMode::PITCHING)
	{
		m_cam_target_pitch = -asin(dir.dotProduct(Vector3::UNIT_Y));
	}

	m_cam_ratio = 1.0f / (m_cct_dt * 4.0f);

	m_cam_dist_min = std::min(m_cct_player_actor->getMinimalCameraRadius() * 2.0f, 33.0f);

	m_cam_look_at = m_cct_player_actor->getPosition();

	CameraManager::CameraBehaviorOrbitUpdate();
}

void CameraManager::CameraBehaviorVehicleReset()
{
	CameraManager::CameraBehaviorOrbitReset();
	m_cam_rot_y = 0.35f;
	m_cam_dist_min = std::min(m_cct_player_actor->getMinimalCameraRadius() * 2.0f, 33.0f);
	m_cam_dist = m_cam_dist_min * 1.5f + 2.0f;
}

bool CameraManager::CameraBehaviorVehicleMousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	const OIS::MouseState ms = _arg.state;

	if ( ms.buttonDown(OIS::MB_Middle) && RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) )
	{
		if ( m_cct_player_actor && m_cct_player_actor->ar_custom_camera_node >= 0 )
		{
			// Calculate new camera distance
			Vector3 lookAt = m_cct_player_actor->ar_nodes[m_cct_player_actor->ar_custom_camera_node].AbsPosition;
			m_cam_dist = 2.0f * gEnv->mainCamera->getPosition().distance(lookAt);

			// Calculate new camera pitch
			Vector3 camDir = (gEnv->mainCamera->getPosition() - lookAt).normalisedCopy();
			m_cam_rot_y = asin(camDir.y);

			// Calculate new camera yaw
			Vector3 dir = -m_cct_player_actor->getDirection();
			Quaternion rotX = dir.getRotationTo(camDir, Vector3::UNIT_Y);
			m_cam_rot_x = rotX.getYaw();

			// Corner case handling
			Radian angle = dir.angleBetween(camDir);
			if ( angle > Radian(Math::HALF_PI) )
			{
				if ( std::abs(Radian(m_cam_rot_x).valueRadians()) < Math::HALF_PI )
				{
					if ( m_cam_rot_x < Radian(0.0f) )
						m_cam_rot_x -= Radian(Math::HALF_PI);
					else
						m_cam_rot_x += Radian(Math::HALF_PI);
				}
			}
		}
	}

	return false;
}

void CameraManager::CameraBehaviorVehicleSplineUpdate()
{
    if (m_cct_player_actor->ar_num_camera_rails <= 0)
    {
        this->switchToNextBehavior();
        return;
    }

    Vector3 dir = m_cct_player_actor->getDirection();

    m_cam_target_pitch = 0.0f;

    if (App::gfx_extcam_mode.GetActive() == GfxExtCamMode::PITCHING)
    {
        m_cam_target_pitch = -asin(dir.dotProduct(Vector3::UNIT_Y));
    }

    if (m_cct_player_actor->GetAllLinkedActors().size() != m_splinecam_num_linked_beams)
    {
        this->CameraBehaviorVehicleSplineCreateSpline();
    }
    this->CameraBehaviorVehicleSplineUpdateSpline();
    this->CameraBehaviorVehicleSplineUpdateSplineDisplay();

    m_cam_look_at = m_splinecam_spline->interpolate(m_splinecam_spline_pos);

    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) && RoR::App::GetInputEngine()->isKeyDownValueBounce(OIS::KC_SPACE))
    {
        m_splinecam_auto_tracking = !m_splinecam_auto_tracking;
        if (m_splinecam_auto_tracking)
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Auto tracking enabled"), "camera_go.png", 3000);
            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Auto tracking enabled"));
        }
        else
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Auto tracking disabled"), "camera_go.png", 3000);
            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Auto tracking disabled"));
        }
    }

    if (m_splinecam_auto_tracking)
    {
        Vector3 centerDir = m_cct_player_actor->getPosition() - m_cam_look_at;
        if (centerDir.length() > 1.0f)
        {
            centerDir.normalise();
            Radian oldTargetDirection = m_cam_target_direction;
            m_cam_target_direction = -atan2(centerDir.dotProduct(Vector3::UNIT_X), centerDir.dotProduct(-Vector3::UNIT_Z));
            if (m_cam_target_direction.valueRadians() * oldTargetDirection.valueRadians() < 0.0f && centerDir.length() < m_cam_dist_min)
            {
                m_cam_rot_x = -m_cam_rot_x;
            }
        }
    }

    CameraManager::CameraBehaviorOrbitUpdate();
}

bool CameraManager::CameraBehaviorVehicleSplineMouseMoved(  const OIS::MouseEvent& _arg)
{
    const OIS::MouseState ms = _arg.state;

    m_cam_ratio = 1.0f / (m_cct_dt * 4.0f);

    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LCONTROL) && ms.buttonDown(OIS::MB_Right))
    {
        Real splinePosDiff = ms.X.rel * std::max(0.00005f, m_splinecam_spline_len * 0.0000001f);

        if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) || RoR::App::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT))
        {
            splinePosDiff *= 3.0f;
        }

        if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU))
        {
            splinePosDiff *= 0.1f;
        }

        m_splinecam_spline_pos += splinePosDiff;

        if (ms.X.rel > 0 && m_splinecam_spline_pos > 0.99f)
        {
            if (m_splinecam_spline_closed)
            {
                m_splinecam_spline_pos -= 1.0f;
            }
            else
            {
                // u - turn
            }
        }
        else if (ms.X.rel < 0 && m_splinecam_spline_pos < 0.01f)
        {
            if (m_splinecam_spline_closed)
            {
                m_splinecam_spline_pos += 1.0f;
            }
            else
            {
                // u - turn
            }
        }

        m_splinecam_spline_pos = std::max(0.0f, m_splinecam_spline_pos);
        m_splinecam_spline_pos = std::min(m_splinecam_spline_pos, 1.0f);

        return true;
    }
    else
    {
        return CameraManager::CameraBehaviorOrbitMouseMoved(_arg);
    }
}

void CameraManager::CameraBehaviorVehicleSplineReset()
{
    CameraManager::CameraBehaviorOrbitReset();

    m_cam_dist = std::min(m_cct_player_actor->getMinimalCameraRadius() * 2.0f, 33.0f);

    m_splinecam_spline_pos = 0.5f;
}

void CameraManager::CameraBehaviorVehicleSplineCreateSpline()
{
    m_splinecam_spline_closed = false;
    m_splinecam_spline_len = 1.0f;

    m_splinecam_spline->clear();
    m_splinecam_spline_nodes.clear();

    for (int i = 0; i < m_cct_player_actor->ar_num_camera_rails; i++)
    {
        m_splinecam_spline_nodes.push_back(&m_cct_player_actor->ar_nodes[m_cct_player_actor->ar_camera_rail[i]]);
    }

    std::list<Actor*> linkedBeams = m_cct_player_actor->GetAllLinkedActors();

    m_splinecam_num_linked_beams = static_cast<int>(linkedBeams.size());

    if (m_splinecam_num_linked_beams > 0)
    {
        for (std::list<Actor*>::iterator it = linkedBeams.begin(); it != linkedBeams.end(); ++it)
        {
            if ((*it)->ar_num_camera_rails <= 0)
                continue;

            Vector3 curSplineFront = m_splinecam_spline_nodes.front()->AbsPosition;
            Vector3 curSplineBack = m_splinecam_spline_nodes.back()->AbsPosition;

            Vector3 linkedSplineFront = (*it)->ar_nodes[(*it)->ar_camera_rail[0]].AbsPosition;
            Vector3 linkedSplineBack = (*it)->ar_nodes[(*it)->ar_camera_rail[(*it)->ar_num_camera_rails - 1]].AbsPosition;

            if (curSplineBack.distance(linkedSplineFront) < 5.0f)
            {
                for (int i = 1; i < (*it)->ar_num_camera_rails; i++)
                {
                    m_splinecam_spline_nodes.push_back(&(*it)->ar_nodes[(*it)->ar_camera_rail[i]]);
                }
            }
            else if (curSplineFront.distance(linkedSplineFront) < 5.0f)
            {
                for (int i = 1; i < (*it)->ar_num_camera_rails; i++)
                {
                    m_splinecam_spline_nodes.push_front(&(*it)->ar_nodes[(*it)->ar_camera_rail[i]]);
                }
            }
            else if (curSplineBack.distance(linkedSplineBack) < 5.0f)
            {
                for (int i = (*it)->ar_num_camera_rails - 2; i >= 0; i--)
                {
                    m_splinecam_spline_nodes.push_back(&(*it)->ar_nodes[(*it)->ar_camera_rail[i]]);
                }
            }
            else if (curSplineFront.distance(linkedSplineBack) < 5.0f)
            {
                for (int i = (*it)->ar_num_camera_rails - 2; i >= 0; i--)
                {
                    m_splinecam_spline_nodes.push_front(&(*it)->ar_nodes[(*it)->ar_camera_rail[i]]);
                }
            }
        }
    }

    for (unsigned int i = 0; i < m_splinecam_spline_nodes.size(); i++)
    {
        m_splinecam_spline->addPoint(m_splinecam_spline_nodes[i]->AbsPosition);
    }

    Vector3 firstPoint = m_splinecam_spline->getPoint(0);
    Vector3 lastPoint = m_splinecam_spline->getPoint(m_splinecam_spline->getNumPoints() - 1);

    if (firstPoint.distance(lastPoint) < 1.0f)
    {
        m_splinecam_spline_closed = true;
    }

    for (int i = 1; i < m_splinecam_spline->getNumPoints(); i++)
    {
        m_splinecam_spline_len += m_splinecam_spline->getPoint(i - 1).distance(m_splinecam_spline->getPoint(i));
    }

    m_splinecam_spline_len /= 2.0f;

    if (!m_splinecam_mo && RoR::App::diag_camera.GetActive())
    {
        m_splinecam_mo = gEnv->sceneManager->createManualObject();
        SceneNode* splineNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();

        m_splinecam_mo->begin("tracks/transred", Ogre::RenderOperation::OT_LINE_STRIP);
        for (int i = 0; i < SPLINECAM_DRAW_RESOLUTION; i++)
        {
            m_splinecam_mo->position(0, 0, 0);
        }
        m_splinecam_mo->end();

        splineNode->attachObject(m_splinecam_mo);
    }
}

void CameraManager::CameraBehaviorVehicleSplineUpdateSpline()
{
    for (int i = 0; i < m_splinecam_spline->getNumPoints(); i++)
    {
        m_splinecam_spline->updatePoint(i, m_splinecam_spline_nodes[i]->AbsPosition);
    }
}

void CameraManager::CameraBehaviorVehicleSplineUpdateSplineDisplay()
{
    if (!m_splinecam_mo)
        return;

    m_splinecam_mo->beginUpdate(0);
    for (int i = 0; i < SPLINECAM_DRAW_RESOLUTION; i++)
    {
        Real parametricDist = i / (float)SPLINECAM_DRAW_RESOLUTION;
        Vector3 position = m_splinecam_spline->interpolate(parametricDist);
        m_splinecam_mo->position(position);
    }
    m_splinecam_mo->end();
}
