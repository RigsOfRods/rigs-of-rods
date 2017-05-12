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
#include "GUI_GameConsole.h"
#include "InputEngine.h"
#include "Language.h"
#include "OverlayWrapper.h"
#include "Settings.h"
#include "GUIManager.h"
#include "PerVehicleCameraContext.h"

#include "CameraBehaviorOrbit.h"
#include "CameraBehaviorCharacter.h"
#include "CameraBehaviorStatic.h"
#include "CameraBehaviorVehicle.h"
#include "CameraBehaviorVehicleCineCam.h"
#include "CameraBehaviorVehicleSpline.h"
#include "CameraBehaviorIsometric.h"

#include <stack>

using namespace Ogre;
using namespace RoR;

CameraManager::CameraManager() : 
      m_active_behavior(CAMERA_BEHAVIOR_INVALID)
    , m_cam_before_free(CAMERA_BEHAVIOR_INVALID)
    , m_cam_before_fixed(CAMERA_BEHAVIOR_INVALID)
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
    m_cam_behav_isometric        = new CameraBehaviorIsometric();

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
    delete m_cam_behav_character;
    delete m_cam_behav_static;
    delete m_cam_behav_vehicle;
    delete m_cam_behav_vehicle_spline;
    delete m_cam_behav_vehicle_cinecam;
    delete m_cam_behav_isometric;

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
    mTransScale = mTransSpeed  * dt;
    mRotScale   = mRotateSpeed * dt;

    ctx.mCurrTruck  = cur_truck;
    ctx.mSimSpeed   = sim_speed;
    ctx.mDt         = dt;
    ctx.mRotScale   = Degree(mRotScale);
    ctx.mTransScale = mTransScale;
    ctx.fovInternal = Degree(App::GetGfxFovInternal());
    ctx.fovExternal = Degree(App::GetGfxFovExternal());

    if ( m_active_behavior < CAMERA_BEHAVIOR_END && RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_CHANGE) )
    {
        if ( (m_active_behavior == CAMERA_BEHAVIOR_INVALID) || this->FindBehavior(m_active_behavior)->switchBehavior(ctx) )
        {
            this->switchToNextBehavior();
        }
    }

    if ( RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_FREE_MODE_FIX) )
    {
        toggleBehavior(CAMERA_BEHAVIOR_FIXED);
    }

    if ( RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_FREE_MODE) )
    {
        toggleBehavior(CAMERA_BEHAVIOR_FREE);
    }

    if (m_active_behavior == CAMERA_BEHAVIOR_FREE)
    {
        this->UpdateFreeCamera();
    }
    else if (m_active_behavior != CAMERA_BEHAVIOR_INVALID)
    {
        this->FindBehavior(m_active_behavior)->update(ctx);
    }
    else
    {
        switchBehavior(CAMERA_BEHAVIOR_CHARACTER);
    }

    return true;
}

void CameraManager::switchToNextBehavior()
{
    int i = (m_active_behavior + 1) % CAMERA_BEHAVIOR_END;
    this->switchBehavior(i);
}

void CameraManager::ActivateNewBehavior(CameraBehaviors behav_id, bool reset)
{
    // Assign new behavior
    m_active_behavior = behav_id;

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
        m_cam_behav_static->SetPreviousFov(gEnv->mainCamera->getFOVy());
        break;

    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:
        if ( (ctx.mCurrTruck == nullptr) || ctx.mCurrTruck->free_camerarail <= 0)
        {
            this->switchToNextBehavior();
            return;
        }
        else if (reset)
        {
            m_cam_behav_vehicle_spline->ResetOrbitStyleCam();
            m_cam_behav_vehicle_spline->createSpline(ctx);
        }
        ctx.mCurrTruck->GetCameraContext()->behavior = RoR::PerVehicleCameraContext::CAMERA_BEHAVIOR_VEHICLE_SPLINE;
        break;

    case CAMERA_BEHAVIOR_VEHICLE_CINECAM:
        if ((ctx.mCurrTruck == nullptr) || (ctx.mCurrTruck->freecinecamera <= 0))
        {
            this->switchToNextBehavior();
            return;
        }
        else if ( reset )
        {
            m_cam_behav_vehicle_cinecam->ResetOrbitStyleCam();
        }

        gEnv->mainCamera->setFOVy(ctx.fovInternal);

        ctx.mCurrTruck->prepareInside(true);

        if ( RoR::App::GetOverlayWrapper() != nullptr )
        {
            RoR::App::GetOverlayWrapper()->showDashboardOverlays((ctx.mCurrTruck->driveable == AIRPLANE), ctx.mCurrTruck);
        }

        ctx.mCurrTruck->currentcamera = ctx.mCurrTruck->GetCameraContext()->last_cinecam_index;
        ctx.mCurrTruck->changedCamera();

        ctx.mCurrTruck->GetCameraContext()->behavior = RoR::PerVehicleCameraContext::CAMERA_BEHAVIOR_VEHICLE_CINECAM;
        break;

    case CAMERA_BEHAVIOR_VEHICLE:
        if ( ctx.mCurrTruck == nullptr )
        {
            this->switchToNextBehavior();
            return;
        }
        else if ( reset )
        {
            m_cam_behav_vehicle->ResetOrbitStyleCam();
        }
        ctx.mCurrTruck->GetCameraContext()->behavior = RoR::PerVehicleCameraContext::CAMERA_BEHAVIOR_VEHICLE_3rdPERSON;
        break;

    case CAMERA_BEHAVIOR_CHARACTER:
        if (ctx.mCurrTruck != nullptr)
        {
            this->switchToNextBehavior();
            return;
        }
        else if (reset)
        {
            m_cam_behav_character->ResetOrbitStyleCam();
        }
        break;

    default:;
    }
}

void CameraManager::switchBehavior(int newBehaviorID, bool reset)
{
    if (newBehaviorID == m_active_behavior)
    {
        return; // Nothing to do
    }

    IBehavior<CameraContext>* new_behav = this->FindBehavior(newBehaviorID);
    if ( new_behav == nullptr )
    {
        return; // Legacy logic - behavior is looked up before fully determined. TODO: Research and refactor.
    }

    this->DeActivateCurrentBehavior();

    if (ctx.mCurrTruck != nullptr)
    {
        ctx.mCurrTruck->GetCameraContext()->behavior = RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_EXTERNAL;
    }

    this->ActivateNewBehavior(static_cast<CameraBehaviors>(newBehaviorID), reset);
}

void CameraManager::SwitchBehaviorOnVehicleChange(int newBehaviorID, bool reset, Beam* old_vehicle, Beam* new_vehicle)
{
    if (newBehaviorID == m_active_behavior)
    {
        if (old_vehicle != new_vehicle)
        {
            this->NotifyContextChange();
        }
        return;
    }

    auto* new_behav = this->FindBehavior(newBehaviorID);
    if ( new_behav == nullptr )
    {
        return;
    }

    // deactivate old
    ctx.mCurrTruck = old_vehicle;
    this->DeActivateCurrentBehavior();

    // activate new
    ctx.mCurrTruck = new_vehicle;
    this->ActivateNewBehavior(static_cast<CameraBehaviors>(newBehaviorID), reset);
}

void CameraManager::toggleBehavior(int behavior)
{
    switch (behavior)
    {
    case CAMERA_BEHAVIOR_FIXED:
        if (m_active_behavior == CAMERA_BEHAVIOR_FIXED)
        {
            this->switchBehavior(m_cam_before_fixed);
            m_cam_before_fixed = CAMERA_BEHAVIOR_INVALID;
        }
        else
        {
            m_cam_before_fixed = m_active_behavior;
            this->switchBehavior(CAMERA_BEHAVIOR_FIXED);
        }
        return;

    case CAMERA_BEHAVIOR_FREE:
        if (m_active_behavior == CAMERA_BEHAVIOR_FREE)
        {
            this->switchBehavior(m_cam_before_free);
            m_cam_before_free = CAMERA_BEHAVIOR_INVALID;
        }
        else
        {
            m_cam_before_free = m_active_behavior;
            this->switchBehavior(CAMERA_BEHAVIOR_FREE);
        }
        return;

    default:
        this->switchToNextBehavior();
        return;
    }
}

bool CameraManager::hasActiveBehavior()
{
    return (m_active_behavior != CAMERA_BEHAVIOR_INVALID);
}

int CameraManager::getCurrentBehavior()
{
    return m_active_behavior;
}

bool CameraManager::mouseMoved(const OIS::MouseEvent& _arg)
{
    switch (m_active_behavior)
    {
    case CAMERA_BEHAVIOR_FREE:
        gEnv->mainCamera->yaw(Degree(-_arg.state.X.rel * 0.13f));
        gEnv->mainCamera->pitch(Degree(-_arg.state.Y.rel * 0.13f));
        MyGUI::PointerManager::getInstance().setVisible(false);
        return true;

    default:;
    }

    auto* obj = this->FindBehavior(m_active_behavior);
    if (obj = nullptr)
        return false;
    else
        return obj->mouseMoved(ctx, _arg);
}

bool CameraManager::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    auto* obj = this->FindBehavior(m_active_behavior);
    if (obj = nullptr)
        return false;
    else
        return obj->mousePressed(ctx, _arg, _id);
}

bool CameraManager::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    auto* obj = this->FindBehavior(m_active_behavior);
    if (obj = nullptr)
        return false;
    else
        return obj->mouseReleased(ctx, _arg, _id);
}

bool CameraManager::gameControlsLocked()
{
    // game controls are only disabled in free camera mode for now
    return (m_active_behavior == CAMERA_BEHAVIOR_FREE);
}

void CameraManager::OnReturnToMainMenu()
{
    ctx.mCurrTruck = nullptr;
    m_active_behavior = CAMERA_BEHAVIOR_INVALID;
}

void CameraManager::NotifyContextChange()
{
    switch (m_active_behavior)
    {
    case CAMERA_BEHAVIOR_CHARACTER:
        m_cam_behav_character->ResetCamLastLookatPosition();
        break;

    case CAMERA_BEHAVIOR_VEHICLE:
        m_cam_behav_vehicle->ResetCamLastLookatPosition();
        break;

    case CAMERA_BEHAVIOR_VEHICLE_CINECAM:
        m_cam_behav_vehicle_cinecam->ResetCamLastLookatPosition();
        break;

    case CAMERA_BEHAVIOR_VEHICLE_SPLINE:
        m_cam_behav_vehicle_spline->ResetCamLastLookatPosition();
        break;

    default:;
    }
}

void CameraManager::NotifyVehicleChanged(Beam* old_vehicle, Beam* new_vehicle)
{
    // Getting out of vehicle
    if (new_vehicle == nullptr)
    {
        ctx.mCurrTruck = nullptr;
        if ( !(this->m_active_behavior == CAMERA_BEHAVIOR_STATIC && m_config_exit_vehicle_keep_fixedfreecam) &&
             !(this->m_active_behavior == CAMERA_BEHAVIOR_FIXED  && m_config_exit_vehicle_keep_fixedfreecam) &&
             !(this->m_active_behavior == CAMERA_BEHAVIOR_FREE   && m_config_exit_vehicle_keep_fixedfreecam) )
        {
            this->switchBehavior(CAMERA_BEHAVIOR_CHARACTER);
        }
        return;
    }

    // Getting in vehicle
    if ( !(this->m_active_behavior == CAMERA_BEHAVIOR_STATIC && m_config_enter_vehicle_keep_fixedfreecam) &&
         !(this->m_active_behavior == CAMERA_BEHAVIOR_FIXED  && m_config_enter_vehicle_keep_fixedfreecam) &&
         !(this->m_active_behavior == CAMERA_BEHAVIOR_FREE   && m_config_enter_vehicle_keep_fixedfreecam) )
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
    case CAMERA_BEHAVIOR_ISOMETRIC:       return m_cam_behav_isometric;
    default:                              return nullptr;
    };
}

void CameraManager::DeActivateCurrentBehavior()
{
    switch (m_active_behavior)
    {
    case CAMERA_BEHAVIOR_STATIC:
        m_cam_behav_static->RestorePreviousFov();
        break;

    case CAMERA_BEHAVIOR_VEHICLE_CINECAM:
        if ( ctx.mCurrTruck == nullptr 
            || ctx.mCurrTruck->GetCameraContext()->behavior != RoR::PerVehicleCameraContext::CAMERA_BEHAVIOR_VEHICLE_CINECAM )
        {
            return;
        }

        gEnv->mainCamera->setFOVy(ctx.fovExternal);

        ctx.mCurrTruck->prepareInside(false);

        if ( RoR::App::GetOverlayWrapper() != nullptr )
        {
            RoR::App::GetOverlayWrapper()->showDashboardOverlays(true, ctx.mCurrTruck);
        }

        ctx.mCurrTruck->GetCameraContext()->last_cinecam_index = ctx.mCurrTruck->currentcamera;

        ctx.mCurrTruck->GetCameraContext()->last_cinecam_index = false; // WTF?! ~ Verbatim legacy logic, TODO: research and fix ~ only_a_ptr, 05/2017
        ctx.mCurrTruck->currentcamera = -1;
        ctx.mCurrTruck->changedCamera();
        break;

    default:;
    }
}

void CameraManager::UpdateFreeCamera()
{
    Ogre::Degree mRotX(0.0f);
    Ogre::Degree mRotY(0.0f);
    Ogre::Degree mRotScale(ctx.mRotScale * 10.0f * ctx.mDt);
    Ogre::Vector3 mTrans(Ogre::Vector3::ZERO);
    Ogre::Real mTransScale(ctx.mTransScale * 10.0f * ctx.mDt);

    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) || RoR::App::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT))
    {
        mRotScale *= 3.0f;
        mTransScale *= 3.0f;
    }
    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LCONTROL))
    {
        mRotScale *= 6.0f;
        mTransScale *= 20.0f;
    }
    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU))
    {
        mRotScale *= 0.1f;
        mTransScale *= 0.1f;
    }

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_LEFT))
    {
        mTrans.x -= mTransScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_RIGHT))
    {
        mTrans.x += mTransScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_FORWARD))
    {
        mTrans.z -= mTransScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_BACKWARDS))
    {
        mTrans.z += mTransScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_UP))
    {
        mTrans.y += mTransScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_DOWN))
    {
        mTrans.y -= mTransScale;
    }

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_RIGHT))
    {
        mRotX -= mRotScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_LEFT))
    {
        mRotX += mRotScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_ROT_UP))
    {
        mRotY += mRotScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_ROT_DOWN))
    {
        mRotY -= mRotScale;
    }

    gEnv->mainCamera->yaw(mRotX);
    gEnv->mainCamera->pitch(mRotY);

    Ogre::Vector3 camPosition = gEnv->mainCamera->getPosition() + gEnv->mainCamera->getOrientation() * mTrans.normalisedCopy() * mTransScale;

    gEnv->mainCamera->setPosition(camPosition);
}
