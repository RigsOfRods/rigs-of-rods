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
#include "DepthOfFieldEffect.h"
#include "InputEngine.h"
#include "Settings.h"
#include "GUIManager.h"
#include "PerVehicleCameraContext.h"

#include "CameraBehaviorOrbit.h"
#include "CameraBehaviorCharacter.h"
#include "CameraBehaviorFixed.h"
#include "CameraBehaviorFree.h"
#include "CameraBehaviorStatic.h"
#include "CameraBehaviorVehicle.h"
#include "CameraBehaviorVehicleCineCam.h"
#include "CameraBehaviorVehicleSpline.h"
#include "CameraBehaviorIsometric.h"

#include <stack>

using namespace Ogre;
using namespace RoR;

CameraManager::CameraManager() : 
      currentBehavior(nullptr)
    , currentBehaviorID(-1)
    , mTransScale(1.0f)
    , mTransSpeed(50.0f)
    , mRotScale(0.1f)
    , m_config_enter_vehicle_keep_fixedfreecam(false)
    , m_config_exit_vehicle_keep_fixedfreecam(false)
    , mRotateSpeed(100.0f)
{
    // Create global behaviors
    m_cam_behav_character        = new CameraBehaviorCharacter();
    m_cam_behav_static           = new CameraBehaviorStatic();
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
}

CameraManager::~CameraManager()
{
    delete m_cam_behav_character;
    delete m_cam_behav_static;
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

    if ( currentBehaviorID < CAMERA_BEHAVIOR_END && RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_CHANGE) )
    {
        switchToNextBehavior(false);
    }

    if ( RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_FREE_MODE_FIX) )
    {
        toggleBehavior(CAMERA_BEHAVIOR_FIXED);
    }

    if ( RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_FREE_MODE) )
    {
        toggleBehavior(CAMERA_BEHAVIOR_FREE);
    }

    if ( currentBehavior )
    {
        currentBehavior->update(ctx);
    } else
    {
        switchBehavior(CAMERA_BEHAVIOR_CHARACTER);
    }

    return true;
}

void CameraManager::switchToNextBehavior(bool force /* = true */)
{
    if ( !currentBehavior || force || currentBehavior->switchBehavior(ctx) )
    {
        int i = (currentBehaviorID + 1) % CAMERA_BEHAVIOR_END;
        switchBehavior(i);
    }
}

void CameraManager::switchBehavior(int newBehaviorID, bool reset)
{
    if (newBehaviorID == currentBehaviorID)
    {
        return;
    }

    auto* new_behav = this->FindBehavior(newBehaviorID);
    if ( new_behav == nullptr )
    {
        return;
    }

    // deactivate old
    if ( currentBehavior )
    {
        currentBehavior->deactivate(ctx);
    }

    // set new
    currentBehavior = new_behav;
    currentBehaviorID = newBehaviorID;

    // activate new
    if (ctx.cct_player_actor != nullptr)
    {
        ctx.cct_player_actor->GetCameraContext()->behavior = RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_EXTERNAL;
    }
    currentBehavior->activate(ctx, reset);
}

void CameraManager::SwitchBehaviorOnVehicleChange(int newBehaviorID, bool reset, Actor* old_vehicle, Actor* new_vehicle)
{
    if (newBehaviorID == currentBehaviorID)
    {
        if (old_vehicle != new_vehicle)
        {
            currentBehavior->notifyContextChange(ctx);
        }
        return;
    }

    auto* new_behav = this->FindBehavior(newBehaviorID);
    if ( new_behav == nullptr )
    {
        return;
    }

    // deactivate old
    ctx.cct_player_actor = old_vehicle;
    if ( currentBehavior )
    {
        currentBehavior->deactivate(ctx);
    }

    // set new
    currentBehavior = new_behav;
    currentBehaviorID = newBehaviorID;

    // activate new
    ctx.cct_player_actor = new_vehicle;
    currentBehavior->activate(ctx, reset);
}

void CameraManager::toggleBehavior(int behavior)
{
    static std::stack<int> precedingBehaviors;

    if ( behavior != currentBehaviorID && (precedingBehaviors.empty() || precedingBehaviors.top() != behavior))
    {
        if ( currentBehaviorID >= 0 )
        {
            precedingBehaviors.push(currentBehaviorID);
        }
        switchBehavior(behavior);
    } else if ( !precedingBehaviors.empty() )
    {
        switchBehavior(precedingBehaviors.top(), false);
        precedingBehaviors.pop();
    } else
    {
        switchToNextBehavior();
    }
}

bool CameraManager::hasActiveBehavior()
{
    return currentBehavior != 0;
}

bool CameraManager::hasActiveCharacterBehavior()
{
    return dynamic_cast<CameraBehaviorCharacter*>(currentBehavior) != 0;
}

bool CameraManager::hasActiveVehicleBehavior()
{
    return dynamic_cast<CameraBehaviorVehicle*>(currentBehavior) != 0;
}

int CameraManager::getCurrentBehavior()
{
    return currentBehaviorID;
}

bool CameraManager::mouseMoved(const OIS::MouseEvent& _arg)
{
    if ( !currentBehavior ) return false;
    return currentBehavior->mouseMoved(ctx, _arg);
}

bool CameraManager::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    if ( !currentBehavior ) return false;
    return currentBehavior->mousePressed(ctx, _arg, _id);
}

bool CameraManager::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    if ( !currentBehavior ) return false;
    return currentBehavior->mouseReleased(ctx, _arg, _id);
}

bool CameraManager::gameControlsLocked()
{
    // game controls are only disabled in free camera mode for now
    return (currentBehaviorID == CAMERA_BEHAVIOR_FREE);
}

void CameraManager::OnReturnToMainMenu()
{
    ctx.cct_player_actor = nullptr;
    currentBehavior = nullptr;
    currentBehaviorID = -1;
}

void CameraManager::NotifyContextChange()
{
    if ( !currentBehavior ) return;
    currentBehavior->notifyContextChange(ctx);
}

void CameraManager::NotifyVehicleChanged(Actor* old_vehicle, Actor* new_vehicle)
{
    // Getting out of vehicle
    if (new_vehicle == nullptr)
    {
        ctx.cct_player_actor = nullptr;
        if ( !(this->currentBehaviorID == CAMERA_BEHAVIOR_STATIC && m_config_exit_vehicle_keep_fixedfreecam) &&
             !(this->currentBehaviorID == CAMERA_BEHAVIOR_FIXED  && m_config_exit_vehicle_keep_fixedfreecam) &&
             !(this->currentBehaviorID == CAMERA_BEHAVIOR_FREE   && m_config_exit_vehicle_keep_fixedfreecam) )
        {
            this->switchBehavior(CAMERA_BEHAVIOR_CHARACTER);
        }
        return;
    }

    // Getting in vehicle
    if ( !(this->currentBehaviorID == CAMERA_BEHAVIOR_STATIC && m_config_enter_vehicle_keep_fixedfreecam) &&
         !(this->currentBehaviorID == CAMERA_BEHAVIOR_FIXED  && m_config_enter_vehicle_keep_fixedfreecam) &&
         !(this->currentBehaviorID == CAMERA_BEHAVIOR_FREE   && m_config_enter_vehicle_keep_fixedfreecam) )
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

IBehavior<CameraManager::CameraContext>* CameraManager::FindBehavior(int behaviorID) // TODO: eliminate the `int ID`
{
    switch (behaviorID)
    {
    case CAMERA_BEHAVIOR_CHARACTER:       return m_cam_behav_character;
    case CAMERA_BEHAVIOR_STATIC:          return m_cam_behav_static;
    case CAMERA_BEHAVIOR_VEHICLE:         return m_cam_behav_vehicle;
    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:  return m_cam_behav_vehicle_spline;
    case CAMERA_BEHAVIOR_VEHICLE_CINECAM: return m_cam_behav_vehicle_cinecam;
    case CAMERA_BEHAVIOR_FREE:            return m_cam_behav_free;
    case CAMERA_BEHAVIOR_FIXED:           return m_cam_behav_fixed;
    case CAMERA_BEHAVIOR_ISOMETRIC:       return m_cam_behav_isometric;
    default:                              return nullptr;
    };
}
