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
#include "DepthOfFieldEffect.h"
#include "GUI_GameConsole.h"
#include "InputEngine.h"
#include "Language.h"
#include "OverlayWrapper.h"
#include "Settings.h"
#include "TerrainManager.h"
#include "GUIManager.h"
#include "PerVehicleCameraContext.h"

#include "CameraBehaviorOrbit.h"
#include "CameraBehaviorCharacter.h"
#include "CameraBehaviorFixed.h"
#include "CameraBehaviorFree.h"
#include "CameraBehaviorVehicle.h"
#include "CameraBehaviorVehicleCineCam.h"
#include "CameraBehaviorVehicleSpline.h"
#include "CameraBehaviorIsometric.h"

#include <stack>

using namespace Ogre;
using namespace RoR;

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
    , mRotateSpeed(100.0f)
{
    // Create global behaviors
    m_cam_behav_character        = new CameraBehaviorCharacter();
    m_cam_behav_vehicle          = new CameraBehaviorVehicle();
    m_cam_behav_vehicle_spline   = new CameraBehaviorVehicleSpline();
    m_cam_behav_vehicle_cinecam  = new CameraBehaviorVehicleCineCam(this);
    m_cam_behav_free             = new CameraBehaviorFree();
    m_cam_behav_fixed            = new CameraBehaviorFixed();
    m_cam_behav_isometric        = new CameraBehaviorIsometric();

    ctx.cct_player_actor = nullptr;
    ctx.cct_dof_manager = nullptr;
    ctx.cct_debug = BSETTING("Camera Debug", false);

    m_config_enter_vehicle_keep_fixedfreecam = BSETTING("Camera_EnterVehicle_KeepFixedFreeCam", true);
    m_config_exit_vehicle_keep_fixedfreecam  = BSETTING("Camera_ExitVehicle_KeepFixedFreeCam",  false);

    m_staticcam_update_timer.reset();
}

CameraManager::~CameraManager()
{
    delete m_cam_behav_character;
    delete m_cam_behav_vehicle;
    delete m_cam_behav_vehicle_spline;
    delete m_cam_behav_vehicle_cinecam;
    delete m_cam_behav_free;
    delete m_cam_behav_fixed;
    delete m_cam_behav_isometric;

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
    case CAMERA_BEHAVIOR_CHARACTER:       return m_cam_behav_character      ->switchBehavior(ctx);
    case CAMERA_BEHAVIOR_STATIC:          return true;
    case CAMERA_BEHAVIOR_VEHICLE:         return m_cam_behav_vehicle        ->switchBehavior(ctx);
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  return m_cam_behav_vehicle_spline ->switchBehavior(ctx);
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: return m_cam_behav_vehicle_cinecam->switchBehavior(ctx);
    case CAMERA_BEHAVIOR_FREE:            return m_cam_behav_free           ->switchBehavior(ctx);
    case CAMERA_BEHAVIOR_FIXED:           return m_cam_behav_fixed          ->switchBehavior(ctx);
    case CAMERA_BEHAVIOR_ISOMETRIC:       return m_cam_behav_isometric      ->switchBehavior(ctx);
    case CAMERA_BEHAVIOR_INVALID:         return true;
    default:                              return false;
    }
}

void CameraManager::UpdateCurrentBehavior()
{
    switch(m_current_behavior)
    {
    case CAMERA_BEHAVIOR_CHARACTER:       m_cam_behav_character      ->update(ctx);  return;
    case CAMERA_BEHAVIOR_STATIC:
        this->UpdateCameraBehaviorStatic(ctx);
        return;
    case CAMERA_BEHAVIOR_VEHICLE:         m_cam_behav_vehicle        ->update(ctx);  return;
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  m_cam_behav_vehicle_spline ->update(ctx);  return;
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: m_cam_behav_vehicle_cinecam->update(ctx);  return;
    case CAMERA_BEHAVIOR_FREE:            m_cam_behav_free           ->update(ctx);  return;
    case CAMERA_BEHAVIOR_FIXED:           m_cam_behav_fixed          ->update(ctx);  return;
    case CAMERA_BEHAVIOR_ISOMETRIC:       m_cam_behav_isometric      ->update(ctx);  return;
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
    case CAMERA_BEHAVIOR_CHARACTER:
        m_cam_behav_character->reset(ctx);
        ctx.camDistMin = 0;
        return;

    case CAMERA_BEHAVIOR_STATIC:
        return;

    case CAMERA_BEHAVIOR_VEHICLE:         m_cam_behav_vehicle        ->reset(ctx);  return;
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  m_cam_behav_vehicle_spline ->reset(ctx);  return;
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: m_cam_behav_vehicle_cinecam->reset(ctx);  return;
    case CAMERA_BEHAVIOR_FREE:            m_cam_behav_free           ->reset(ctx);  return;
    case CAMERA_BEHAVIOR_FIXED:           m_cam_behav_fixed          ->reset(ctx);  return;
    case CAMERA_BEHAVIOR_ISOMETRIC:       m_cam_behav_isometric      ->reset(ctx);  return;
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
    case CAMERA_BEHAVIOR_CHARACTER:       m_cam_behav_character      ->deactivate(ctx);  return;
    case CAMERA_BEHAVIOR_STATIC:
        gEnv->mainCamera->setFOVy(m_staticcam_previous_fov);
        return;
    case CAMERA_BEHAVIOR_VEHICLE:         m_cam_behav_vehicle        ->deactivate(ctx);  return;
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  m_cam_behav_vehicle_spline ->deactivate(ctx);  return;
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: m_cam_behav_vehicle_cinecam->deactivate(ctx);  return;
    case CAMERA_BEHAVIOR_FREE:            m_cam_behav_free           ->deactivate(ctx);  return;
    case CAMERA_BEHAVIOR_FIXED:           m_cam_behav_fixed          ->deactivate(ctx);  return;
    case CAMERA_BEHAVIOR_ISOMETRIC:       m_cam_behav_isometric      ->deactivate(ctx);  return;
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
    case CAMERA_BEHAVIOR_CHARACTER:       return m_cam_behav_character      ->mouseMoved(ctx, _arg);
    case CAMERA_BEHAVIOR_STATIC:          return false;
    case CAMERA_BEHAVIOR_VEHICLE:         return m_cam_behav_vehicle        ->mouseMoved(ctx, _arg);
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  return m_cam_behav_vehicle_spline ->mouseMoved(ctx, _arg);
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: return m_cam_behav_vehicle_cinecam->mouseMoved(ctx, _arg);
    case CAMERA_BEHAVIOR_FREE:            return m_cam_behav_free           ->mouseMoved(ctx, _arg);
    case CAMERA_BEHAVIOR_FIXED:           return m_cam_behav_fixed          ->mouseMoved(ctx, _arg);
    case CAMERA_BEHAVIOR_ISOMETRIC:       return m_cam_behav_isometric      ->mouseMoved(ctx, _arg);
    case CAMERA_BEHAVIOR_INVALID:         return false;
    default:                              return false;
    }
}

bool CameraManager::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    switch(m_current_behavior)
    {
    case CAMERA_BEHAVIOR_CHARACTER:       return m_cam_behav_character      ->mousePressed(ctx, _arg, _id);
    case CAMERA_BEHAVIOR_STATIC:          return false;
    case CAMERA_BEHAVIOR_VEHICLE:         return m_cam_behav_vehicle        ->mousePressed(ctx, _arg, _id);
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  return m_cam_behav_vehicle_spline ->mousePressed(ctx, _arg, _id);
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: return m_cam_behav_vehicle_cinecam->mousePressed(ctx, _arg, _id);
    case CAMERA_BEHAVIOR_FREE:            return m_cam_behav_free           ->mousePressed(ctx, _arg, _id);
    case CAMERA_BEHAVIOR_FIXED:           return m_cam_behav_fixed          ->mousePressed(ctx, _arg, _id);
    case CAMERA_BEHAVIOR_ISOMETRIC:       return m_cam_behav_isometric      ->mousePressed(ctx, _arg, _id);
    case CAMERA_BEHAVIOR_INVALID:         return false;
    default:                              return false;
    }
}

bool CameraManager::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    switch(m_current_behavior)
    {
    case CAMERA_BEHAVIOR_CHARACTER:       return m_cam_behav_character      ->mouseReleased(ctx, _arg, _id);
    case CAMERA_BEHAVIOR_STATIC:          return false;
    case CAMERA_BEHAVIOR_VEHICLE:         return m_cam_behav_vehicle        ->mouseReleased(ctx, _arg, _id);
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  return m_cam_behav_vehicle_spline ->mouseReleased(ctx, _arg, _id);
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: return m_cam_behav_vehicle_cinecam->mouseReleased(ctx, _arg, _id);
    case CAMERA_BEHAVIOR_FREE:            return m_cam_behav_free           ->mouseReleased(ctx, _arg, _id);
    case CAMERA_BEHAVIOR_FIXED:           return m_cam_behav_fixed          ->mouseReleased(ctx, _arg, _id);
    case CAMERA_BEHAVIOR_ISOMETRIC:       return m_cam_behav_isometric      ->mouseReleased(ctx, _arg, _id);
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
    case CAMERA_BEHAVIOR_CHARACTER:       m_cam_behav_character      ->notifyContextChange(ctx);  return;
    case CAMERA_BEHAVIOR_STATIC:          return;
    case CAMERA_BEHAVIOR_VEHICLE:         m_cam_behav_vehicle        ->notifyContextChange(ctx);  return;
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  m_cam_behav_vehicle_spline ->notifyContextChange(ctx);  return;
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: m_cam_behav_vehicle_cinecam->notifyContextChange(ctx);  return;
    case CAMERA_BEHAVIOR_FREE:            m_cam_behav_free           ->notifyContextChange(ctx);  return;
    case CAMERA_BEHAVIOR_FIXED:           m_cam_behav_fixed          ->notifyContextChange(ctx);  return;
    case CAMERA_BEHAVIOR_ISOMETRIC:       m_cam_behav_isometric      ->notifyContextChange(ctx);  return;
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
