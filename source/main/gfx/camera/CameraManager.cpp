/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2017-2020 Petr Ohlidal

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

#include "AppContext.h"
#include "ApproxMath.h"
#include "Actor.h"
#include "ActorManager.h"
#include "Character.h"
#include "Collisions.h"
#include "Console.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "InputEngine.h"
#include "Language.h"
#include "OverlayWrapper.h"
#include "Replay.h"
#include "TerrainManager.h"
#include "GUIManager.h"
#include "PerVehicleCameraContext.h"
#include "Water.h"

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
    b.y = std::max(b.y, App::GetSimTerrain()->GetHeightAt(b.x, b.z) + 1.0f);

    int steps = std::max(3.0f, a.distance(b) * 2.0f);
    for (int i = 1; i < steps; i++)
    {
        Vector3 pos = a + (b - a) * (float)i / steps;
        float h = App::GetSimTerrain()->GetHeightAt(pos.x, pos.z);
        if (h > pos.y)
        {
            return true;
        }
    }
    return App::GetSimTerrain()->GetCollisions()->intersectsTris(Ray(a, b - a)).first;
}

bool intersectsTerrain(Vector3 a, Vector3 start, Vector3 end, float interval) // internal helper
{
    int steps = std::max(3.0f, start.distance(end) * (6.0f / interval));
    for (int i = 0; i <= steps; i++)
    {
        Vector3 b = start + (end - start) * (float)i / steps;
        if (intersectsTerrain(a, b))
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
    , m_staticcam_force_update(false)
    , m_staticcam_fov_exponent(1.0f)
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
    , m_camera_node(nullptr)
{
    m_cct_player_actor = nullptr;
    m_staticcam_update_timer.reset();

    m_camera = App::GetGfxScene()->GetSceneManager()->createCamera("PlayerCam");
    m_camera->setNearClipDistance(0.5);
    m_camera->setAutoAspectRatio(true);
    this->CreateCameraNode();

    App::GetAppContext()->GetViewport()->setCamera(m_camera);
}

CameraManager::~CameraManager()
{
    if (m_splinecam_spline)
        delete m_splinecam_spline;
    if (m_splinecam_mo)
        delete m_splinecam_mo;
}

void CameraManager::CreateCameraNode()
{
    ROR_ASSERT(!m_camera_node);
    m_camera_node = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
    m_camera_node->setFixedYawAxis(true);
    m_camera_node->attachObject(m_camera);
}

void CameraManager::ReCreateCameraNode()
{
    m_camera_node = nullptr; // after call to `Ogre::SceneManager::ClearScene()`, the node pointer is invalid.
    this->CreateCameraNode();
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
        if (!App::GetGameContext()->GetPlayerCharacter())
            return;
        m_cam_target_direction = -App::GetGameContext()->GetPlayerCharacter()->getRotation() - Radian(Math::HALF_PI);
        Ogre::Vector3 offset = (!m_charactercam_is_3rdperson) ? CHARACTERCAM_OFFSET_1ST_PERSON : CHARACTERCAM_OFFSET_3RD_PERSON;
        m_cam_look_at = App::GetGameContext()->GetPlayerCharacter()->getPosition() + offset;

        CameraManager::CameraBehaviorOrbitUpdate();
        return;
    }

    case CAMERA_BEHAVIOR_STATIC:
        m_staticcam_fov_exponent = App::gfx_static_cam_fov_exp->GetFloat();
        this->UpdateCameraBehaviorStatic();
        return;

    case CAMERA_BEHAVIOR_VEHICLE:         this->UpdateCameraBehaviorVehicle();  return;
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  this->CameraBehaviorVehicleSplineUpdate();  return;
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: {
        CameraManager::CameraBehaviorOrbitUpdate();

        const NodeIdx_t pos_node  = m_cct_player_actor->ar_camera_node_pos [m_cct_player_actor->ar_current_cinecam];
        const NodeIdx_t dir_node  = m_cct_player_actor->ar_camera_node_dir [m_cct_player_actor->ar_current_cinecam];
        const NodeIdx_t roll_node = m_cct_player_actor->ar_camera_node_roll[m_cct_player_actor->ar_current_cinecam];

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

        this->GetCameraNode()->setPosition(m_cct_player_actor->ar_nodes[m_cct_player_actor->ar_cinecam_node[m_cct_player_actor->ar_current_cinecam]].AbsPosition);
        this->GetCameraNode()->setOrientation(orientation);
        return;
    }
    case CAMERA_BEHAVIOR_FREE:            this->UpdateCameraBehaviorFree(); return;
    case CAMERA_BEHAVIOR_FIXED:           this->UpdateCameraBehaviorFixed(); return;
    case CAMERA_BEHAVIOR_ISOMETRIC:       return;
    case CAMERA_BEHAVIOR_INVALID:         return;
    default:                              return;
    }
}

void CameraManager::UpdateInputEvents(float dt) // Called every frame
{
    if (App::sim_state->GetEnum<SimState>() != SimState::RUNNING &&
        App::sim_state->GetEnum<SimState>() != SimState::EDITOR_MODE)
    {
        return;
    }

    m_cct_player_actor = App::GetGameContext()->GetPlayerActor();
    m_cct_sim_speed    = App::GetGameContext()->GetActorManager()->GetSimulationSpeed();
    m_cct_dt           = dt;
    m_cct_rot_scale    = Degree(TRANS_SPEED * dt);
    m_cct_trans_scale  = ROTATE_SPEED * dt;

    if ( m_current_behavior < CAMERA_BEHAVIOR_END && App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_CHANGE) )
    {
        if ( (m_current_behavior == CAMERA_BEHAVIOR_INVALID) || this->EvaluateSwitchBehavior() )
        {
            this->switchToNextBehavior();
        }
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_FREE_MODE_FIX))
    {
        this->ToggleCameraBehavior(CAMERA_BEHAVIOR_FIXED);
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_FREE_MODE))
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

    // camera FOV settings
    if (this->GetCurrentBehavior() != CameraManager::CAMERA_BEHAVIOR_STATIC) // the static camera has its own fov logic
    {
        CVar* cvar_fov = ((this->GetCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM))
            ? App::gfx_fov_internal : App::gfx_fov_external;

        int modifier = 0;
        modifier = (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FOV_LESS, 0.1f)) ? -1 : 0;
        modifier += (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FOV_MORE, 0.1f)) ?  1 : 0;
        int fov = -1;
        if (modifier != 0)
        {
            fov = cvar_fov->GetInt() + modifier;
            if (fov >= 10 && fov <= 160)
            {
                cvar_fov->SetVal(fov);
            }
            else
            {
                App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("FOV: Limit reached"));
            }
        }
        if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FOV_RESET))
        {
            CVar* cvar_fov_default = ((this->GetCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM))
                ? App::gfx_fov_internal_default : App::gfx_fov_external_default;
            cvar_fov->SetVal(cvar_fov_default->GetInt());
        }

        if (fov != -1)
        {
            Str<100> msg; msg << _L("FOV: ") << fov;
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, msg.ToCStr());
        }
    }
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
        m_staticcam_fov_exponent = 1.0f;
        App::gfx_static_cam_fov_exp->SetVal(1.0f);
        return;

    case CAMERA_BEHAVIOR_VEHICLE:
        this->CameraBehaviorVehicleReset();
        return;

    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  this->CameraBehaviorVehicleSplineReset();  return;
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM:
        CameraManager::CameraBehaviorOrbitReset();
        m_cam_rot_y = Degree(DEFAULT_INTERNAL_CAM_PITCH);
        App::GetCameraManager()->GetCamera()->setFOVy(Degree(App::gfx_fov_internal->GetInt()));
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
        break;

    case CAMERA_BEHAVIOR_ISOMETRIC:
    case CAMERA_BEHAVIOR_FIXED:
        RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_INFO, RoR::Console::CONSOLE_SYSTEM_NOTICE, _L("Fixed camera"), "camera_link.png", 3000);
        break;

    case CAMERA_BEHAVIOR_STATIC:
        m_staticcam_previous_fov = App::GetCameraManager()->GetCamera()->getFOVy();
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

        App::GetCameraManager()->GetCamera()->setFOVy(Degree(App::gfx_fov_internal->GetInt()));

        m_cct_player_actor->prepareInside(true);

        if ( RoR::App::GetOverlayWrapper() != nullptr )
        {
            bool visible = m_cct_player_actor->ar_driveable == AIRPLANE && !App::GetGuiManager()->IsGuiHidden();
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

    case CAMERA_BEHAVIOR_INVALID:
        this->CameraBehaviorOrbitReset();
        break;
    }
}

void CameraManager::DeactivateCurrentBehavior()
{
    if (m_current_behavior == CAMERA_BEHAVIOR_STATIC)
    {
        App::GetCameraManager()->GetCamera()->setFOVy(m_staticcam_previous_fov);
    }
    else if (m_current_behavior == CAMERA_BEHAVIOR_VEHICLE_CINECAM)
    {
        if ( m_cct_player_actor != nullptr )
        {
            App::GetCameraManager()->GetCamera()->setFOVy(Degree(App::gfx_fov_external->GetInt()));
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
        if (!App::GetGuiManager()->IsGuiHidden())
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

void CameraManager::ResetAllBehaviors()
{
    this->SwitchBehaviorOnVehicleChange(CAMERA_BEHAVIOR_INVALID, nullptr);
}

bool CameraManager::mouseMoved(const OIS::MouseEvent& _arg)
{

    if (App::sim_state->GetEnum<SimState>() == SimState::PAUSED)
    {
        return true; // Do nothing when paused
    }

    switch(m_current_behavior)
    {
    case CAMERA_BEHAVIOR_CHARACTER: {
        if (!App::GetGameContext()->GetPlayerCharacter())
            return false;
        if (!m_charactercam_is_3rdperson)
        {
            const OIS::MouseState ms = _arg.state;
            Radian angle = App::GetGameContext()->GetPlayerCharacter()->getRotation();

            m_cam_rot_y += Degree(ms.Y.rel * 0.13f);
            angle += Degree(ms.X.rel * 0.13f);

            m_cam_rot_y = Radian(std::min(+Math::HALF_PI * 0.65f, m_cam_rot_y.valueRadians()));
            m_cam_rot_y = Radian(std::max(m_cam_rot_y.valueRadians(), -Math::HALF_PI * 0.9f));

            App::GetGameContext()->GetPlayerCharacter()->setRotation(angle);

            RoR::App::GetGuiManager()->SetMouseCursorVisibility(RoR::GUIManager::MouseCursorVisibility::HIDDEN);

            return true;
        }

        return CameraManager::CameraBehaviorOrbitMouseMoved(_arg);
    }
    case CAMERA_BEHAVIOR_STATIC:          return CameraBehaviorStaticMouseMoved(_arg);
    case CAMERA_BEHAVIOR_VEHICLE:         return CameraBehaviorOrbitMouseMoved(_arg);
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  return this->CameraBehaviorVehicleSplineMouseMoved(_arg);
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: return CameraBehaviorOrbitMouseMoved(_arg);
    case CAMERA_BEHAVIOR_FREE: {
        const OIS::MouseState ms = _arg.state;

        App::GetCameraManager()->GetCameraNode()->yaw(Degree(-ms.X.rel * 0.13f), Ogre::Node::TS_WORLD);
        App::GetCameraManager()->GetCameraNode()->pitch(Degree(-ms.Y.rel * 0.13f));

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
    const OIS::MouseState ms = _arg.state;

    if (ms.buttonDown(OIS::MB_Right) && _id == OIS::MB_Middle)
    {
        ResetCurrentBehavior();
    }

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

void CameraManager::NotifyVehicleChanged(Actor* new_vehicle)
{
    // Getting out of vehicle
    if (new_vehicle == nullptr)
    {
        m_cct_player_actor = nullptr;
        if (this->m_current_behavior != CAMERA_BEHAVIOR_FIXED && this->m_current_behavior != CAMERA_BEHAVIOR_STATIC &&
                this->m_current_behavior != CAMERA_BEHAVIOR_FREE)
        {
            this->switchBehavior(CAMERA_BEHAVIOR_CHARACTER);
        }
        return;
    }

    // Getting in vehicle
    if (this->m_current_behavior != CAMERA_BEHAVIOR_FIXED && this->m_current_behavior != CAMERA_BEHAVIOR_STATIC &&
            this->m_current_behavior != CAMERA_BEHAVIOR_FREE)
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
    ROR_ASSERT(new_behavior == CAMERA_BEHAVIOR_FIXED || new_behavior == CAMERA_BEHAVIOR_FREE);

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
    Vector3 velocity = Vector3::ZERO;
    Radian angle = Degree(90);
    float radius = 3.0f;
    float speed = 0.0f;

    if (m_cct_player_actor)
    {
        if (m_cct_player_actor->isBeingReset())
        {
            m_staticcam_force_update |= m_cct_player_actor->getPosition().distance(m_staticcam_look_at) > 100.0f;
        }
        m_staticcam_look_at = m_cct_player_actor->getPosition();
        velocity = m_cct_player_actor->ar_nodes[0].Velocity * m_cct_sim_speed;
        if (App::GetGameContext()->GetPlayerActor()->ar_driveable != AIRPLANE)
        {
            radius = m_cct_player_actor->getMinCameraRadius();
        }
        angle = (m_staticcam_look_at - m_staticcam_position).angleBetween(velocity);
        speed = velocity.normalise();

        if (m_cct_player_actor->ar_sim_state == Actor::SimState::LOCAL_REPLAY)
        {
            speed *= m_cct_player_actor->GetReplay()->getPrecision();
        }
    }
    else
    {
        m_staticcam_look_at = App::GetGameContext()->GetPlayerCharacter()->getPosition();
    }

    m_staticcam_force_update |= RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_RESET, 1.0f);

    if (m_staticcam_force_update || m_staticcam_update_timer.getMilliseconds() > 1000)
    {
        Vector3 lookAt = m_staticcam_look_at;
        Vector3 lookAtPrediction = lookAt + velocity * speed;
        float distance = m_staticcam_position.distance(lookAt);
        float interval = std::max(radius, speed);
        float cmradius = std::max(radius, App::gfx_camera_height->GetFloat() / 7.0f);

        if (m_staticcam_force_update ||
               (distance > cmradius * 8.0f && angle < Degree(30)) ||
               (distance < cmradius * 2.0f && angle > Degree(150)) ||
                distance > cmradius * std::max(25.0f, speed * 1.15f) ||
                intersectsTerrain(m_staticcam_position, lookAt, lookAtPrediction, interval))
        {
            const auto water = App::GetSimTerrain()->getWater();
            float water_height = (water && !water->IsUnderWater(lookAt)) ? water->GetStaticWaterHeight() : 0.0f;
            float desired_offset = std::max(std::sqrt(radius) * 2.89f, App::gfx_camera_height->GetFloat());

            std::vector<std::pair<float, Vector3>> viable_positions;
            for (int i = 0; i < 10; i++)
            {
                Vector3 pos = lookAt;
                if (speed < 2.5f)
                {
                    float angle = Math::TWO_PI * frand();
                    pos += Vector3(cos(angle), 0, sin(angle)) * cmradius * 2.5f;
                }
                else
                {
                    float dist = std::max(cmradius * 2.5f, std::sqrt(cmradius) * speed);
                    float mrnd = Math::Clamp(0.6f * cmradius / dist, 0.0f, 0.3f);
                    float arnd = mrnd + frand() * (1.0f - mrnd);
                    float  rnd = frand() > 0.5f ? arnd : -arnd;
                    pos += (velocity + velocity.crossProduct(Vector3::UNIT_Y) * rnd) * dist;
                }
                pos.y = std::max(pos.y, water_height);
                pos.y = std::max(pos.y, App::GetSimTerrain()->GetHeightAt(pos.x, pos.z));
                pos.y += desired_offset * (i < 7 ? 1.0f : frand());
                if (!intersectsTerrain(pos, lookAt, lookAtPrediction, interval))
                {
                    float hdiff = std::abs(pos.y - lookAt.y - desired_offset);
                    viable_positions.push_back({hdiff, pos});
                    if (hdiff < 1.0f || viable_positions.size() > 2)
                        break;
                }
            }
            if (!viable_positions.empty())
            {
                std::sort(viable_positions.begin(), viable_positions.end());
                m_staticcam_update_timer.reset();
                m_staticcam_position = viable_positions.front().second;
                m_staticcam_force_update = false;
            }
        }
    }

    static float fovExp = m_staticcam_fov_exponent;
    fovExp = (1.0f / (m_cam_ratio + 1.0f)) * m_staticcam_fov_exponent + (m_cam_ratio / (m_cam_ratio + 1.0f)) * fovExp;

    float camDist = m_staticcam_position.distance(m_staticcam_look_at);
    float fov = atan2(20.0f, std::pow(camDist, fovExp));

    this->GetCameraNode()->setPosition(m_staticcam_position);
    App::GetCameraManager()->GetCameraNode()->lookAt(m_staticcam_look_at, Ogre::Node::TS_WORLD);
    App::GetCameraManager()->GetCamera()->setFOVy(Radian(fov));
}

bool CameraManager::CameraBehaviorStaticMouseMoved(const OIS::MouseEvent& _arg)
{
    const OIS::MouseState ms = _arg.state;

    if (ms.buttonDown(OIS::MB_Right))
    {
        float scale = RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.00002f : 0.0002f;
        m_staticcam_fov_exponent += ms.Z.rel * scale;
        m_staticcam_fov_exponent = Math::Clamp(m_staticcam_fov_exponent, 0.8f, 1.50f);
        App::gfx_static_cam_fov_exp->SetVal(m_staticcam_fov_exponent);
        return true;
    }

    return false;
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
        }
        else
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Limited camera movement disabled"), "camera_go.png", 3000);
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
    Vector3 precedingPosition = this->GetCameraNode()->getPosition() + camDisplacement;

    Vector3 camPosition = (1.0f / (m_cam_ratio + 1.0f)) * desiredPosition + (m_cam_ratio / (m_cam_ratio + 1.0f)) * precedingPosition;

    if (App::GetSimTerrain()->GetCollisions() && App::GetSimTerrain()->GetCollisions()->forcecam)
    {
        this->GetCameraNode()->setPosition(App::GetSimTerrain()->GetCollisions()->forcecampos);
        App::GetSimTerrain()->GetCollisions()->forcecam = false;
    }
    else
    {
        if (m_cct_player_actor && m_cct_player_actor->ar_sim_state == Actor::SimState::LOCAL_REPLAY && camDisplacement != Vector3::ZERO)
            this->GetCameraNode()->setPosition(desiredPosition);
        else
            this->GetCameraNode()->setPosition(camPosition);
    }

    m_cam_look_at_smooth = (1.0f / (m_cam_ratio + 1.0f)) * m_cam_look_at + (m_cam_ratio / (m_cam_ratio + 1.0f)) * precedingLookAt;

    m_cam_look_at_last = m_cam_look_at;
    m_cam_look_at_smooth_last = m_cam_look_at_smooth;
    App::GetCameraManager()->GetCameraNode()->lookAt(m_cam_look_at_smooth, Ogre::Node::TS_WORLD);
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
    App::GetCameraManager()->GetCamera()->setFOVy(Degree(App::gfx_fov_external->GetInt()));
}

void CameraManager::UpdateCameraBehaviorFree()
{
    Degree mRotX(0.0f);
    Degree mRotY(0.0f);
    Degree cct_rot_scale(m_cct_rot_scale * 5.0f * m_cct_dt);
    Vector3 mTrans(Vector3::ZERO);
    Real cct_trans_scale(m_cct_trans_scale * 5.0f * m_cct_dt);

    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) || RoR::App::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT))
    {
        cct_rot_scale *= 3.0f;
        cct_trans_scale *= 5.0f;
    }
    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LCONTROL))
    {
        cct_rot_scale *= 6.0f;
        cct_trans_scale *= 10.0f;
    }
    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU))
    {
        cct_rot_scale *= 0.2f;
        cct_trans_scale *= 0.2f;
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

    App::GetCameraManager()->GetCameraNode()->yaw(mRotX, Ogre::Node::TS_WORLD);
    App::GetCameraManager()->GetCameraNode()->pitch(mRotY);

    Vector3 camPosition = this->GetCameraNode()->getPosition() + this->GetCameraNode()->getOrientation() * mTrans.normalisedCopy() * cct_trans_scale;

    this->GetCameraNode()->setPosition(camPosition);
}

void CameraManager::UpdateCameraBehaviorFixed()
{
	if (App::gfx_fixed_cam_tracking->GetBool())
    {
        Vector3 look_at = m_cct_player_actor ? m_cct_player_actor->getPosition() : App::GetGameContext()->GetPlayerCharacter()->getPosition();
        App::GetCameraManager()->GetCameraNode()->lookAt(look_at, Ogre::Node::TS_WORLD);
    }
}

void CameraManager::UpdateCameraBehaviorVehicle()
{
	Vector3 dir = m_cct_player_actor->getDirection();

	m_cam_target_direction = -atan2(dir.dotProduct(Vector3::UNIT_X), dir.dotProduct(-Vector3::UNIT_Z));
	m_cam_target_pitch     = 0.0f;

	if ( RoR::App::gfx_extcam_mode->GetEnum<GfxExtCamMode>() == RoR::GfxExtCamMode::PITCHING)
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
			m_cam_dist = 2.0f * this->GetCameraNode()->getPosition().distance(lookAt);

			// Calculate new camera pitch
			Vector3 camDir = (this->GetCameraNode()->getPosition() - lookAt).normalisedCopy();
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

    if (App::gfx_extcam_mode->GetEnum<GfxExtCamMode>() == GfxExtCamMode::PITCHING)
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
        }
        else
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Auto tracking disabled"), "camera_go.png", 3000);
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

    auto linkedBeams = m_cct_player_actor->GetAllLinkedActors();

    m_splinecam_num_linked_beams = static_cast<int>(linkedBeams.size());

    if (m_splinecam_num_linked_beams > 0)
    {
        for (auto actor : linkedBeams)
        {
            if (actor->ar_num_camera_rails <= 0)
                continue;

            Vector3 curSplineFront = m_splinecam_spline_nodes.front()->AbsPosition;
            Vector3 curSplineBack = m_splinecam_spline_nodes.back()->AbsPosition;

            Vector3 linkedSplineFront = actor->ar_nodes[actor->ar_camera_rail[0]].AbsPosition;
            Vector3 linkedSplineBack = actor->ar_nodes[actor->ar_camera_rail[actor->ar_num_camera_rails - 1]].AbsPosition;

            if (curSplineBack.distance(linkedSplineFront) < 5.0f)
            {
                for (int i = 1; i < actor->ar_num_camera_rails; i++)
                {
                    m_splinecam_spline_nodes.push_back(&actor->ar_nodes[actor->ar_camera_rail[i]]);
                }
            }
            else if (curSplineFront.distance(linkedSplineFront) < 5.0f)
            {
                for (int i = 1; i < actor->ar_num_camera_rails; i++)
                {
                    m_splinecam_spline_nodes.push_front(&actor->ar_nodes[actor->ar_camera_rail[i]]);
                }
            }
            else if (curSplineBack.distance(linkedSplineBack) < 5.0f)
            {
                for (int i = actor->ar_num_camera_rails - 2; i >= 0; i--)
                {
                    m_splinecam_spline_nodes.push_back(&actor->ar_nodes[actor->ar_camera_rail[i]]);
                }
            }
            else if (curSplineFront.distance(linkedSplineBack) < 5.0f)
            {
                for (int i = actor->ar_num_camera_rails - 2; i >= 0; i--)
                {
                    m_splinecam_spline_nodes.push_front(&actor->ar_nodes[actor->ar_camera_rail[i]]);
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

    if (!m_splinecam_mo && RoR::App::diag_camera->GetBool())
    {
        m_splinecam_mo = App::GetGfxScene()->GetSceneManager()->createManualObject();
        SceneNode* splineNode = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();

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
