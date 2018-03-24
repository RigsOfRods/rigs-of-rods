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

#include "CameraManager.h"

#include "Application.h"
#include "BeamFactory.h"
#include "Character.h"
#include "Collisions.h"
#include "DepthOfFieldEffect.h"
#include "GUI_GameConsole.h"
#include "InputEngine.h"
#include "Language.h"
#include "OverlayWrapper.h"
#include "Settings.h"
#include "TerrainManager.h"
#include "GUIManager.h"
#include "PerVehicleCameraContext.h"

#include "CameraBehaviorFree.h"
#include "CameraBehaviorVehicle.h"
#include "CameraBehaviorVehicleCineCam.h"
#include "CameraBehaviorVehicleSpline.h"

#include <stack>

using namespace Ogre;
using namespace RoR;

static const Ogre::Vector3 CHARACTERCAM_OFFSET_1ST_PERSON(0.0f, 1.82f, 0.0f);
static const Ogre::Vector3 CHARACTERCAM_OFFSET_3RD_PERSON(0.0f, 1.1f, 0.0f);

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
    , m_cam_before_toggled(CAMERA_BEHAVIOR_INVALID)
    , m_prev_toggled_cam(CAMERA_BEHAVIOR_INVALID)
    , mTransScale(1.0f)
    , mTransSpeed(50.0f)
    , mRotScale(0.1f)
    , m_config_enter_vehicle_keep_fixedfreecam(false)
    , m_config_exit_vehicle_keep_fixedfreecam(false)
    , m_charactercam_is_3rdperson(true)
    , mRotateSpeed(100.0f)
{
    // Create global behaviors
    m_cam_behav_vehicle_spline   = new CameraBehaviorVehicleSpline();
    m_cam_behav_vehicle_cinecam  = new CameraBehaviorVehicleCineCam(this);

    ctx.cct_player_actor = nullptr;
    ctx.cct_dof_manager = nullptr;
    ctx.cct_debug = BSETTING("Camera Debug", false);

    m_config_enter_vehicle_keep_fixedfreecam = BSETTING("Camera_EnterVehicle_KeepFixedFreeCam", true);
    m_config_exit_vehicle_keep_fixedfreecam  = BSETTING("Camera_ExitVehicle_KeepFixedFreeCam",  false);

    m_staticcam_update_timer.reset();
}

CameraManager::~CameraManager()
{
    delete m_cam_behav_vehicle_spline;
    delete m_cam_behav_vehicle_cinecam;

    delete ctx.cct_dof_manager;
}

void CameraManager::ActivateDepthOfFieldEffect()
{
    if (ctx.cct_dof_manager == nullptr)
    {
        ctx.cct_dof_manager = new DOFManager();
        ctx.cct_dof_manager->setFocusMode(DOFManager::Auto);
    }
    ctx.cct_dof_manager->setEnabled(true);
}

void CameraManager::DisableDepthOfFieldEffect()
{
    if (ctx.cct_dof_manager != nullptr)
    {
        ctx.cct_dof_manager->setEnabled(false);
    }
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
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: return m_cam_behav_vehicle_cinecam->switchBehavior(ctx);
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
        ctx.targetDirection = -gEnv->player->getRotation() - Radian(Math::HALF_PI);
        Ogre::Vector3 offset = (!m_charactercam_is_3rdperson) ? CHARACTERCAM_OFFSET_1ST_PERSON : CHARACTERCAM_OFFSET_3RD_PERSON;
        ctx.camLookAt = gEnv->player->getPosition() + offset;

        CameraManager::CameraBehaviorOrbitUpdate(ctx);
        return;
    }

    case CAMERA_BEHAVIOR_STATIC:
        this->UpdateCameraBehaviorStatic(ctx);
        return;

    case CAMERA_BEHAVIOR_VEHICLE:         this->UpdateCameraBehaviorVehicle();  return;
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  m_cam_behav_vehicle_spline ->update(ctx);  return;
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: m_cam_behav_vehicle_cinecam->update(ctx);  return;
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
    mTransScale = mTransSpeed  * dt;
    mRotScale   = mRotateSpeed * dt;

    ctx.cct_player_actor  = player_vehicle;
    ctx.cct_sim_speed   = sim_speed;
    ctx.cct_dt         = dt;
    ctx.cct_rot_scale   = Degree(mRotScale);
    ctx.cct_trans_scale = mTransScale;
    ctx.cct_fov_interior = Degree(App::gfx_fov_internal.GetActive()); // TODO: don't copy Gvar!
    ctx.cct_fov_exterior = Degree(App::gfx_fov_external.GetActive());

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
    this->switchBehavior(i);
}

void CameraManager::ResetCurrentBehavior()
{
    switch(m_current_behavior)
    {
    case CAMERA_BEHAVIOR_CHARACTER: {
        CameraManager::CameraBehaviorOrbitReset(ctx);

        // Vars from CameraBehaviorOrbit
        if (!m_charactercam_is_3rdperson)
        {
            ctx.camRotY = 0.1f;
            ctx.camDist = 0.1f;
            ctx.camRatio = 0.0f;
        }
        else
        {
            ctx.camRotY = 0.3f;
            ctx.camDist = 5.0f;
            ctx.camRatio = 11.0f;
        }
        ctx.camDistMin = 0;
        return;
    }

    case CAMERA_BEHAVIOR_STATIC:
        return;

    case CAMERA_BEHAVIOR_VEHICLE:
        this->CameraBehaviorVehicleReset();
        return;

    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  m_cam_behav_vehicle_spline ->reset(ctx);  return;
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: m_cam_behav_vehicle_cinecam->reset(ctx);  return;
    case CAMERA_BEHAVIOR_FREE:            return;
    case CAMERA_BEHAVIOR_FIXED:           return;
    case CAMERA_BEHAVIOR_ISOMETRIC:       return;
    case CAMERA_BEHAVIOR_INVALID:         return;
    default:                              return;
    }
}

void CameraManager::ActivateNewBehavior(CameraBehaviors behav_id, bool reset)
{
    // Assign new behavior
    m_current_behavior = behav_id;

    // Resolve per-behavior constraints and actions
    switch (behav_id)
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
        if ( (ctx.cct_player_actor == nullptr) || ctx.cct_player_actor->ar_num_camera_rails <= 0)
        {
            this->switchToNextBehavior();
            return;
        }
        else if (reset)
        {
            m_cam_behav_vehicle_spline->reset(ctx);
            m_cam_behav_vehicle_spline->createSpline(ctx);
        }
        ctx.cct_player_actor->GetCameraContext()->behavior = RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_SPLINE;
        break;

    case CAMERA_BEHAVIOR_VEHICLE_CINECAM:
        if ((ctx.cct_player_actor == nullptr) || (ctx.cct_player_actor->ar_num_cinecams <= 0))
        {
            this->switchToNextBehavior();
            return;
        }
        else if ( reset )
        {
            this->ResetCurrentBehavior();
        }

        gEnv->mainCamera->setFOVy(ctx.cct_fov_interior);

        ctx.cct_player_actor->prepareInside(true);

        if ( RoR::App::GetOverlayWrapper() != nullptr )
        {
            RoR::App::GetOverlayWrapper()->showDashboardOverlays((ctx.cct_player_actor->ar_driveable == AIRPLANE), ctx.cct_player_actor);
        }

        ctx.cct_player_actor->ar_current_cinecam = ctx.cct_player_actor->GetCameraContext()->last_cinecam_index;
        ctx.cct_player_actor->NotifyActorCameraChanged();

        ctx.cct_player_actor->GetCameraContext()->behavior = RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_CINECAM;
        break;

    case CAMERA_BEHAVIOR_VEHICLE:
        if ( ctx.cct_player_actor == nullptr )
        {
            this->switchToNextBehavior();
            return;
        }
        else if ( reset )
        {
            this->ResetCurrentBehavior();
        }
        ctx.cct_player_actor->GetCameraContext()->behavior = RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_3rdPERSON;
        break;

    case CAMERA_BEHAVIOR_CHARACTER:
        if (ctx.cct_player_actor != nullptr)
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
    switch(m_current_behavior)
    {
    case CAMERA_BEHAVIOR_CHARACTER:       return;

    case CAMERA_BEHAVIOR_STATIC:
        gEnv->mainCamera->setFOVy(m_staticcam_previous_fov);
        return;

    case CAMERA_BEHAVIOR_VEHICLE:         return;
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  return;
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: m_cam_behav_vehicle_cinecam->deactivate(ctx);  return;
    case CAMERA_BEHAVIOR_FREE:            return;
    case CAMERA_BEHAVIOR_FIXED:           return;
    case CAMERA_BEHAVIOR_ISOMETRIC:       return;
    case CAMERA_BEHAVIOR_INVALID:         return;
    default:                              return;
    }    
}

void CameraManager::switchBehavior(int newBehaviorID, bool reset)
{
    CameraBehaviors new_behav = static_cast<CameraBehaviors>(newBehaviorID);
    if (new_behav == m_current_behavior)
    {
        return; // Nothing to do
    }

    //OLD IBehavior<CameraContext>* new_behav = this->FindBehavior(newBehaviorID);
    //OLD if ( new_behav == nullptr )
    //OLD {
    //OLD     return; // Legacy logic - behavior is looked up before fully determined. TODO: Research and refactor.
    //OLD }

    // deactivate old
    this->DeactivateCurrentBehavior();

    if (ctx.cct_player_actor != nullptr)
    {
        ctx.cct_player_actor->GetCameraContext()->behavior = RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_EXTERNAL;
    }

    this->ActivateNewBehavior(new_behav, reset);
}

void CameraManager::SwitchBehaviorOnVehicleChange(int newBehaviorID, bool reset, Actor* old_vehicle, Actor* new_vehicle)
{
    CameraBehaviors new_behav = static_cast<CameraBehaviors>(newBehaviorID);
    if (new_behav == m_current_behavior)
    {
        if (old_vehicle != new_vehicle)
        {
            this->NotifyContextChange();
        }
        return;
    }

    // deactivate old
    ctx.cct_player_actor = old_vehicle;
    this->DeactivateCurrentBehavior();

    // activate new
    ctx.cct_player_actor = new_vehicle;
    this->ActivateNewBehavior(static_cast<CameraBehaviors>(newBehaviorID), reset);
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

            ctx.camRotY += Degree(ms.Y.rel * 0.13f);
            angle += Degree(ms.X.rel * 0.13f);

            ctx.camRotY = Radian(std::min(+Math::HALF_PI * 0.65f, ctx.camRotY.valueRadians()));
            ctx.camRotY = Radian(std::max(ctx.camRotY.valueRadians(), -Math::HALF_PI * 0.9f));

            gEnv->player->setRotation(angle);

            RoR::App::GetGuiManager()->SetMouseCursorVisibility(RoR::GUIManager::MouseCursorVisibility::HIDDEN);

            return true;
        }

        return CameraManager::CameraBehaviorOrbitMouseMoved(ctx, _arg);
    }
    case CAMERA_BEHAVIOR_STATIC:          return false;
    case CAMERA_BEHAVIOR_VEHICLE:         return CameraBehaviorOrbitMouseMoved(ctx, _arg);
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  return m_cam_behav_vehicle_spline ->mouseMoved(ctx, _arg);
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: return CameraBehaviorOrbitMouseMoved(ctx, _arg);
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

bool CameraManager::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    switch(m_current_behavior)
    {
    case CAMERA_BEHAVIOR_CHARACTER:       return false;
    case CAMERA_BEHAVIOR_STATIC:          return false;
    case CAMERA_BEHAVIOR_VEHICLE:         return false;
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  return false;
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: return false;
    case CAMERA_BEHAVIOR_FREE:            return false;
    case CAMERA_BEHAVIOR_FIXED:           return false;
    case CAMERA_BEHAVIOR_ISOMETRIC:       return false;
    case CAMERA_BEHAVIOR_INVALID:         return false;
    default:                              return false;
    }
}

bool CameraManager::gameControlsLocked()
{
    // game controls are only disabled in free camera mode for now
    return (m_current_behavior == CAMERA_BEHAVIOR_FREE);
}

void CameraManager::OnReturnToMainMenu()
{
    ctx.cct_player_actor = nullptr;
    m_current_behavior = CAMERA_BEHAVIOR_INVALID;
}

void CameraManager::NotifyContextChange()
{
    switch(m_current_behavior)
    {
    case CAMERA_BEHAVIOR_CHARACTER:       CameraBehaviorOrbitNotifyContextChange(ctx);  return;
    case CAMERA_BEHAVIOR_STATIC:          return;
    case CAMERA_BEHAVIOR_VEHICLE:         CameraBehaviorOrbitNotifyContextChange(ctx);  return;
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  CameraBehaviorOrbitNotifyContextChange(ctx);  return;
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: CameraBehaviorOrbitNotifyContextChange(ctx);  return;
    case CAMERA_BEHAVIOR_FREE:            return;
    case CAMERA_BEHAVIOR_FIXED:           return;
    case CAMERA_BEHAVIOR_ISOMETRIC:       return;
    case CAMERA_BEHAVIOR_INVALID:         return;
    default:                              return;
    }
}

void CameraManager::NotifyVehicleChanged(Actor* old_vehicle, Actor* new_vehicle)
{
    // Getting out of vehicle
    if (new_vehicle == nullptr)
    {
        ctx.cct_player_actor = nullptr;
        if ( !(this->m_current_behavior == CAMERA_BEHAVIOR_STATIC && m_config_exit_vehicle_keep_fixedfreecam) &&
             !(this->m_current_behavior == CAMERA_BEHAVIOR_FIXED  && m_config_exit_vehicle_keep_fixedfreecam) &&
             !(this->m_current_behavior == CAMERA_BEHAVIOR_FREE   && m_config_exit_vehicle_keep_fixedfreecam) )
        {
            this->switchBehavior(CAMERA_BEHAVIOR_CHARACTER);
        }
        return;
    }

    // Getting in vehicle
    if ( !(this->m_current_behavior == CAMERA_BEHAVIOR_STATIC && m_config_enter_vehicle_keep_fixedfreecam) &&
         !(this->m_current_behavior == CAMERA_BEHAVIOR_FIXED  && m_config_enter_vehicle_keep_fixedfreecam) &&
         !(this->m_current_behavior == CAMERA_BEHAVIOR_FREE   && m_config_enter_vehicle_keep_fixedfreecam) )
    {
        // Change camera
        switch (new_vehicle->GetCameraContext()->behavior)
        {
        case RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_3rdPERSON:
            this->SwitchBehaviorOnVehicleChange(CAMERA_BEHAVIOR_VEHICLE, true, old_vehicle, new_vehicle);
            break;

        case RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_SPLINE:
            this->SwitchBehaviorOnVehicleChange(CAMERA_BEHAVIOR_VEHICLE_SPLINE, true, old_vehicle, new_vehicle);
            break;

        case RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_CINECAM:
            this->SwitchBehaviorOnVehicleChange(CAMERA_BEHAVIOR_VEHICLE_CINECAM, true, old_vehicle, new_vehicle);
            break;

        default:
            this->SwitchBehaviorOnVehicleChange(CAMERA_BEHAVIOR_VEHICLE, true, old_vehicle, new_vehicle);
        }
    }
}

void CameraManager::ToggleCameraBehavior(CameraBehaviors behav) // Only accepts FREE and FREEFIX modes
{
    assert(behav == CAMERA_BEHAVIOR_FIXED || behav == CAMERA_BEHAVIOR_FREE);

    if (m_current_behavior == behav) // Leaving toggled mode?
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
        this->switchBehavior(behav);
    }
}

void CameraManager::UpdateCameraBehaviorStatic(const CameraManager::CameraContext& ctx)
{
    Vector3 lookAt = Vector3::ZERO;
    Vector3 velocity = Vector3::ZERO;
    Radian angle = Degree(90);
    float rotation = 0.0f;
    float speed = 0.0f;

    if (ctx.cct_player_actor)
    {
        lookAt = ctx.cct_player_actor->getPosition();
        rotation = ctx.cct_player_actor->getRotation();
        velocity = ctx.cct_player_actor->ar_nodes[0].Velocity * ctx.cct_sim_speed;
        angle = (lookAt - m_staticcam_position).angleBetween(velocity);
        speed = velocity.length();
        if (ctx.cct_player_actor->ar_replay_mode)
        {
            speed *= ctx.cct_player_actor->ar_replay_precision;
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
            float r = (float)std::rand() / RAND_MAX;
            if (App::GetSimTerrain())
            {
                for (int i = 0; i < 100; i++)
                {
                    r = (float)std::rand() / RAND_MAX;
                    Vector3 pos = m_staticcam_position + offset * (0.5f - r) * 2.0f;
                    float h = App::GetSimTerrain()->GetHeightAt(pos.x, pos.z);
                    pos.y = std::max(h, pos.y);
                    if (!intersectsTerrain(pos, lookAt + Vector3::UNIT_Y))
                    {
                        m_staticcam_position = pos;
                        break;
                    }
                }
            }
            m_staticcam_position += offset * (0.5f - r) * 2.0f;

            if (App::GetSimTerrain())
            {
                float h = App::GetSimTerrain()->GetHeightAt(m_staticcam_position.x, m_staticcam_position.z);

                m_staticcam_position.y = std::max(h, m_staticcam_position.y);
            }

            m_staticcam_position.y += 5.0f;

            m_staticcam_update_timer.reset();
        }
    }

    float camDist = m_staticcam_position.distance(lookAt);
    float fov = atan2(20.0f, camDist);

    gEnv->mainCamera->setPosition(m_staticcam_position);
    gEnv->mainCamera->lookAt(lookAt);
    gEnv->mainCamera->setFOVy(Radian(fov));
}

void CameraManager::CameraBehaviorOrbitUpdate( CameraManager::CameraContext& ctx)
{
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_LOOKBACK))
    {
        if (ctx.camRotX > Degree(0))
        {
            ctx.camRotX = Degree(0);
        }
        else
        {
            ctx.camRotX = Degree(180);
        }
    }

    ctx.camRotX += (RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_RIGHT) - RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_LEFT)) * ctx.cct_rot_scale;
    ctx.camRotY += (RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_UP) - RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_DOWN)) * ctx.cct_rot_scale;

    ctx.camRotY = std::max((Radian)Degree(-80), ctx.camRotY);
    ctx.camRotY = std::min(ctx.camRotY, (Radian)Degree(88));

    ctx.camRotXSwivel = (RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_RIGHT) - RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_LEFT)) * Degree(90);
    ctx.camRotYSwivel = (RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_UP) - RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_DOWN)) * Degree(60);

    ctx.camRotYSwivel = std::max((Radian)Degree(-80) - ctx.camRotY, ctx.camRotYSwivel);
    ctx.camRotYSwivel = std::min(ctx.camRotYSwivel, (Radian)Degree(88) - ctx.camRotY);

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_IN) && ctx.camDist > 1)
    {
        ctx.camDist -= ctx.cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_IN_FAST) && ctx.camDist > 1)
    {
        ctx.camDist -= ctx.cct_trans_scale * 10;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_OUT))
    {
        ctx.camDist += ctx.cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_OUT_FAST))
    {
        ctx.camDist += ctx.cct_trans_scale * 10;
    }

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_RESET))
    {
        CameraBehaviorOrbitReset(ctx);
    }

    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT) && RoR::App::GetInputEngine()->isKeyDownValueBounce(OIS::KC_SPACE))
    {
        ctx.limitCamMovement = !ctx.limitCamMovement;
        if (ctx.limitCamMovement)
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

    if (ctx.limitCamMovement && ctx.camDistMin > 0.0f)
    {
        ctx.camDist = std::max(ctx.camDistMin, ctx.camDist);
    }
    if (ctx.limitCamMovement && ctx.camDistMax > 0.0f)
    {
        ctx.camDist = std::min(ctx.camDist, ctx.camDistMax);
    }

    ctx.camDist = std::max(0.0f, ctx.camDist);

    Vector3 desiredPosition = ctx.camLookAt + ctx.camDist * 0.5f * Vector3(
        sin(ctx.targetDirection.valueRadians() + (ctx.camRotX - ctx.camRotXSwivel).valueRadians()) * cos(ctx.targetPitch.valueRadians() + (ctx.camRotY - ctx.camRotYSwivel).valueRadians())
        , sin(ctx.targetPitch.valueRadians() + (ctx.camRotY - ctx.camRotYSwivel).valueRadians())
        , cos(ctx.targetDirection.valueRadians() + (ctx.camRotX - ctx.camRotXSwivel).valueRadians()) * cos(ctx.targetPitch.valueRadians() + (ctx.camRotY - ctx.camRotYSwivel).valueRadians())
    );

    if (ctx.limitCamMovement && App::GetSimTerrain())
    {
        float h = App::GetSimTerrain()->GetHeightAt(desiredPosition.x, desiredPosition.z) + 1.0f;

        desiredPosition.y = std::max(h, desiredPosition.y);
    }

    if (ctx.camLookAtLast == Vector3::ZERO)
    {
        ctx.camLookAtLast = ctx.camLookAt;
    }
    if (ctx.camLookAtSmooth == Vector3::ZERO)
    {
        ctx.camLookAtSmooth = ctx.camLookAt;
    }
    if (ctx.camLookAtSmoothLast == Vector3::ZERO)
    {
        ctx.camLookAtSmoothLast = ctx.camLookAtSmooth;
    }

    Vector3 camDisplacement = ctx.camLookAt - ctx.camLookAtLast;
    Vector3 precedingLookAt = ctx.camLookAtSmoothLast + camDisplacement;
    Vector3 precedingPosition = gEnv->mainCamera->getPosition() + camDisplacement;

    Vector3 camPosition = (1.0f / (ctx.camRatio + 1.0f)) * desiredPosition + (ctx.camRatio / (ctx.camRatio + 1.0f)) * precedingPosition;

    if (gEnv->collisions && gEnv->collisions->forcecam)
    {
        gEnv->mainCamera->setPosition(gEnv->collisions->forcecampos);
        gEnv->collisions->forcecam = false;
    }
    else
    {
        if (ctx.cct_player_actor && ctx.cct_player_actor->ar_replay_mode && camDisplacement != Vector3::ZERO)
            gEnv->mainCamera->setPosition(desiredPosition);
        else
            gEnv->mainCamera->setPosition(camPosition);
    }

    ctx.camLookAtSmooth = (1.0f / (ctx.camRatio + 1.0f)) * ctx.camLookAt + (ctx.camRatio / (ctx.camRatio + 1.0f)) * precedingLookAt;

    ctx.camLookAtLast = ctx.camLookAt;
    ctx.camLookAtSmoothLast = ctx.camLookAtSmooth;
    gEnv->mainCamera->lookAt(ctx.camLookAtSmooth);
}

bool CameraManager::CameraBehaviorOrbitMouseMoved( CameraManager::CameraContext& ctx, const OIS::MouseEvent& _arg)
{
    const OIS::MouseState ms = _arg.state;

    if (ms.buttonDown(OIS::MB_Right))
    {
        float scale = RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.002f : 0.02f;
        ctx.camRotX += Degree(ms.X.rel * 0.13f);
        ctx.camRotY += Degree(-ms.Y.rel * 0.13f);
        ctx.camDist += -ms.Z.rel * scale;
        return true;
    }

    return false;
}

void CameraManager::CameraBehaviorOrbitReset( CameraManager::CameraContext& ctx)
{
    ctx.camRotX = 0.0f;
    ctx.camRotXSwivel = 0.0f;
    ctx.camRotY = 0.3f;
    ctx.camRotYSwivel = 0.0f;
    ctx.camLookAtLast = Vector3::ZERO;
    ctx.camLookAtSmooth = Vector3::ZERO;
    ctx.camLookAtSmoothLast = Vector3::ZERO;
    gEnv->mainCamera->setFOVy(ctx.cct_fov_exterior);
}

void CameraManager::CameraBehaviorOrbitNotifyContextChange( CameraManager::CameraContext& ctx)
{
    ctx.camLookAtLast = Vector3::ZERO;
}

void CameraManager::UpdateCameraBehaviorFree()
{
    Degree mRotX(0.0f);
    Degree mRotY(0.0f);
    Degree cct_rot_scale(ctx.cct_rot_scale * 10.0f * ctx.cct_dt);
    Vector3 mTrans(Vector3::ZERO);
    Real cct_trans_scale(ctx.cct_trans_scale * 10.0f * ctx.cct_dt);

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
	Vector3 dir = ctx.cct_player_actor->getDirection();

	ctx.targetDirection = -atan2(dir.dotProduct(Vector3::UNIT_X), dir.dotProduct(-Vector3::UNIT_Z));
	ctx.targetPitch     = 0.0f;

	if ( RoR::App::gfx_extcam_mode.GetActive() == RoR::GfxExtCamMode::PITCHING)
	{
		ctx.targetPitch = -asin(dir.dotProduct(Vector3::UNIT_Y));
	}

	ctx.camRatio = 1.0f / (ctx.cct_dt * 4.0f);

	ctx.camDistMin = std::min(ctx.cct_player_actor->getMinimalCameraRadius() * 2.0f, 33.0f);

	ctx.camLookAt = ctx.cct_player_actor->getPosition();

	CameraManager::CameraBehaviorOrbitUpdate(ctx);
}

void CameraManager::CameraBehaviorVehicleReset()
{
	CameraManager::CameraBehaviorOrbitReset(ctx);
	ctx.camRotY = 0.35f;
	ctx.camDistMin = std::min(ctx.cct_player_actor->getMinimalCameraRadius() * 2.0f, 33.0f);
	ctx.camDist = ctx.camDistMin * 1.5f + 2.0f;
}

bool CameraManager::CameraBehaviorVehicleMousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	const OIS::MouseState ms = _arg.state;

	if ( ms.buttonDown(OIS::MB_Middle) && RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) )
	{
		if ( ctx.cct_player_actor && ctx.cct_player_actor->ar_custom_camera_node >= 0 )
		{
			// Calculate new camera distance
			Vector3 lookAt = ctx.cct_player_actor->ar_nodes[ctx.cct_player_actor->ar_custom_camera_node].AbsPosition;
			ctx.camDist = 2.0f * gEnv->mainCamera->getPosition().distance(lookAt);

			// Calculate new camera pitch
			Vector3 camDir = (gEnv->mainCamera->getPosition() - lookAt).normalisedCopy();
			ctx.camRotY = asin(camDir.y);

			// Calculate new camera yaw
			Vector3 dir = -ctx.cct_player_actor->getDirection();
			Quaternion rotX = dir.getRotationTo(camDir, Vector3::UNIT_Y);
			ctx.camRotX = rotX.getYaw();

			// Corner case handling
			Radian angle = dir.angleBetween(camDir);
			if ( angle > Radian(Math::HALF_PI) )
			{
				if ( std::abs(Radian(ctx.camRotX).valueRadians()) < Math::HALF_PI )
				{
					if ( ctx.camRotX < Radian(0.0f) )
						ctx.camRotX -= Radian(Math::HALF_PI);
					else
						ctx.camRotX += Radian(Math::HALF_PI);
				}
			}
		}
	}

	return false;
}
