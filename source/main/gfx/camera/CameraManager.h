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



#include <OIS.h>
#include <OgreVector3.h>
#include <OgreMath.h> // Degree, Radian
#include <OgreTimer.h>

// Forward decl.
class CameraBehaviorVehicleSpline;
class CameraBehaviorCharacter;
class CameraBehaviorVehicle;
class CameraBehaviorVehicleCineCam;
class CameraBehaviorFree;

class CameraManager
{
    friend class RoR::SceneMouse;

public:

    CameraManager();
    ~CameraManager();

    class CameraContext
    {
    public:

        CameraContext() :
            camDist(5.0f)
            , camDistMax(0.0f)
            , camDistMin(0.0f)
            , camLookAt(Ogre::Vector3::ZERO)
            , camLookAtLast(Ogre::Vector3::ZERO)
            , camLookAtSmooth(Ogre::Vector3::ZERO)
            , camLookAtSmoothLast(Ogre::Vector3::ZERO)
            , camRatio(11.0f)
            , camRotX(0.0f)
            , camRotXSwivel(0.0f)
            , camRotY(0.3f)
            , camRotYSwivel(0.0f)
            , limitCamMovement(true)
            , targetDirection(0.0f)
            , targetPitch(0.0f)
        {}

        Actor*       cct_player_actor;
        DOFManager*  cct_dof_manager;
        Ogre::Degree cct_rot_scale;
        Ogre::Real   cct_dt;
        Ogre::Real   cct_trans_scale;
        Ogre::Radian cct_fov_interior;
        Ogre::Radian cct_fov_exterior;
        bool         cct_debug;
        float        cct_sim_speed;

        // CameraBehaviorOrbit context -- TODO: cleanup
        Ogre::Radian camRotX, camRotY;
        Ogre::Radian camRotXSwivel, camRotYSwivel;
        Ogre::Radian targetDirection, targetPitch;
        Ogre::Real camDist, camDistMin, camDistMax, camRatio;
        Ogre::Vector3 camLookAt;
        bool limitCamMovement;
        Ogre::Vector3 camLookAtLast;
        Ogre::Vector3 camLookAtSmooth;
        Ogre::Vector3 camLookAtSmoothLast;
    };

    enum CameraBehaviors
    {
        CAMERA_BEHAVIOR_CHARACTER=0,
        CAMERA_BEHAVIOR_STATIC,
        CAMERA_BEHAVIOR_VEHICLE,
        CAMERA_BEHAVIOR_VEHICLE_SPLINE,
        CAMERA_BEHAVIOR_VEHICLE_CINECAM,
        CAMERA_BEHAVIOR_END,
        CAMERA_BEHAVIOR_FREE,
        CAMERA_BEHAVIOR_FIXED,
        CAMERA_BEHAVIOR_ISOMETRIC,
        CAMERA_BEHAVIOR_INVALID = -1,
    };

    bool Update(float dt, Actor* player_vehicle, float sim_speed);

    void switchBehavior(int newBehavior, bool reset = true);
    void switchToNextBehavior();


    bool gameControlsLocked();
    bool hasActiveBehavior();

    int getCurrentBehavior();

    void OnReturnToMainMenu();
    void NotifyContextChange();
    void NotifyVehicleChanged(Actor* old_vehicle, Actor* new_vehicle);
    void ActivateDepthOfFieldEffect();
    void DisableDepthOfFieldEffect();

    static void CameraBehaviorOrbitNotifyContextChange( CameraManager::CameraContext& ctx);
    static void CameraBehaviorOrbitReset( CameraManager::CameraContext& ctx);
    static bool CameraBehaviorOrbitMouseMoved( CameraManager::CameraContext& ctx, const OIS::MouseEvent& _arg);
    static void CameraBehaviorOrbitUpdate( CameraManager::CameraContext& ctx);

protected:

    void SwitchBehaviorOnVehicleChange(int newBehaviorID, bool reset, Actor* old_vehicle, Actor* new_vehicle);
    void ToggleCameraBehavior(CameraBehaviors behav); //!< Only accepts FREE and FREEFIX modes
    void ActivateNewBehavior(CameraBehaviors behav_id, bool reset);
    bool EvaluateSwitchBehavior();
    void UpdateCurrentBehavior();
    void ResetCurrentBehavior();
    void DeactivateCurrentBehavior();
    void UpdateCameraBehaviorStatic(const CameraManager::CameraContext& ctx);

    CameraContext ctx;

    float mTransScale, mTransSpeed;
    float mRotScale, mRotateSpeed;

    CameraBehaviors m_current_behavior;
    CameraBehaviors m_cam_before_toggled; ///< Toggled modes (FREE, FREEFIX) remember original state.
    CameraBehaviors m_prev_toggled_cam; ///< Switching toggled modes (FREE, FREEFIX) keeps 1-slot history.
    // Static cam attributes
    Ogre::Radian m_staticcam_previous_fov;
    Ogre::Vector3 m_staticcam_position;
    Ogre::Timer m_staticcam_update_timer;
    // Global behaviors
    CameraBehaviorCharacter*  m_cam_behav_character;
    CameraBehaviorVehicle*    m_cam_behav_vehicle;
    CameraBehaviorVehicleSpline* m_cam_behav_vehicle_spline;
    CameraBehaviorVehicleCineCam* m_cam_behav_vehicle_cinecam;
    CameraBehaviorFree*       m_cam_behav_free;

    bool m_config_enter_vehicle_keep_fixedfreecam;
    bool m_config_exit_vehicle_keep_fixedfreecam;

    bool mouseMoved(const OIS::MouseEvent& _arg);
    bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
};
