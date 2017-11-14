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
    createGlobalBehaviors();

    ctx.mCurrTruck = 0;
    ctx.mDof = 0;
    ctx.mDebug = BSETTING("Camera Debug", false);

    if (BSETTING("DOF", false))
    {
        ctx.mDof = new DOFManager();
        ctx.mDof->setFocusMode(DOFManager::Auto);
    }

    m_config_enter_vehicle_keep_fixedfreecam = BSETTING("Camera_EnterVehicle_KeepFixedFreeCam", true);
    m_config_exit_vehicle_keep_fixedfreecam  = BSETTING("Camera_ExitVehicle_KeepFixedFreeCam",  false);
}

CameraManager::~CameraManager()
{
    for (std::map <int , IBehavior<CameraContext> *>::iterator it = globalBehaviors.begin(); it != globalBehaviors.end(); ++it)
    {
        delete it->second;
    }

    globalBehaviors.clear();

    delete ctx.mDof;
}

void CameraManager::SetSimController(RoRFrameListener* sim)
{
    if (ctx.mDof != nullptr)
    {
        ctx.mDof->SetSimController(sim);
    }
}

bool CameraManager::Update(float dt, Beam* cur_truck, float sim_speed) // Called every frame
{
    if (RoR::App::sim_state.GetActive() == RoR::SimState::PAUSED) { return true; } // Do nothing when paused

    if ( dt == 0 ) return false;

    mTransScale = mTransSpeed  * dt;
    mRotScale   = mRotateSpeed * dt;

    ctx.mCurrTruck  = cur_truck;
    ctx.mSimSpeed   = sim_speed;
    ctx.mDt         = dt;
    ctx.mRotScale   = Degree(mRotScale);
    ctx.mTransScale = mTransScale;
    ctx.fovInternal = Degree(App::gfx_fov_internal.GetActive()); // TODO: don't copy Gvar!
    ctx.fovExternal = Degree(App::gfx_fov_external.GetActive());

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

    if ( globalBehaviors.find(newBehaviorID) == globalBehaviors.end() )
    {
        return;
    }

    // deactivate old
    if ( currentBehavior )
    {
        currentBehavior->deactivate(ctx);
    }

    // set new
    currentBehavior = globalBehaviors[newBehaviorID];
    currentBehaviorID = newBehaviorID;

    // activate new
    if (ctx.mCurrTruck != nullptr)
    {
        ctx.mCurrTruck->GetCameraContext()->behavior = RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_EXTERNAL;
    }
    currentBehavior->activate(ctx, reset);
}

void CameraManager::SwitchBehaviorOnVehicleChange(int newBehaviorID, bool reset, Beam* old_vehicle, Beam* new_vehicle)
{
    if (newBehaviorID == currentBehaviorID)
    {
        if (old_vehicle != new_vehicle)
        {
            currentBehavior->notifyContextChange(ctx);
        }
        return;
    }

    if ( globalBehaviors.find(newBehaviorID) == globalBehaviors.end() )
    {
        return;
    }

    // deactivate old
    ctx.mCurrTruck = old_vehicle;
    if ( currentBehavior )
    {
        currentBehavior->deactivate(ctx);
    }

    // set new
    currentBehavior = globalBehaviors[newBehaviorID];
    currentBehaviorID = newBehaviorID;

    // activate new
    ctx.mCurrTruck = new_vehicle;
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

void CameraManager::createGlobalBehaviors()
{
    globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_CHARACTER,       new CameraBehaviorCharacter()));
    globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_STATIC,          new CameraBehaviorStatic()));
    globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_VEHICLE,         new CameraBehaviorVehicle()));
    globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_VEHICLE_SPLINE,  new CameraBehaviorVehicleSpline()));
    globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_VEHICLE_CINECAM, new CameraBehaviorVehicleCineCam(this)));
    globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_FREE,            new CameraBehaviorFree()));
    globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_FIXED,           new CameraBehaviorFixed()));
    globalBehaviors.insert(std::pair<int, IBehavior<CameraContext>*>(CAMERA_BEHAVIOR_ISOMETRIC,       new CameraBehaviorIsometric()));
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
    ctx.mCurrTruck = nullptr;
    currentBehavior = nullptr;
    currentBehaviorID = -1;
}

void CameraManager::NotifyContextChange()
{
    if ( !currentBehavior ) return;
    currentBehavior->notifyContextChange(ctx);
}

void CameraManager::NotifyVehicleChanged(Beam* old_vehicle, Beam* new_vehicle)
{
    // Getting out of vehicle
    if (new_vehicle == nullptr)
    {
        ctx.mCurrTruck = nullptr;
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
