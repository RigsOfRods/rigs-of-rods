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

#pragma once

#include "RoRPrerequisites.h"



#include <OIS.h>
#include <OgreVector3.h>
#include <OgreMath.h> // Degree, Radian
#include <OgreTimer.h>

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

              camLookAt(Ogre::Vector3::ZERO)
            , camLookAtLast(Ogre::Vector3::ZERO)
            , camLookAtSmooth(Ogre::Vector3::ZERO)
            , camLookAtSmoothLast(Ogre::Vector3::ZERO)

            , limitCamMovement(true)

        {}

        // CameraBehaviorOrbit context -- TODO: cleanup

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

    void CameraBehaviorOrbitNotifyContextChange( CameraManager::CameraContext& ctx);
    void CameraBehaviorOrbitReset( CameraManager::CameraContext& ctx);
    bool CameraBehaviorOrbitMouseMoved( CameraManager::CameraContext& ctx, const OIS::MouseEvent& _arg);
    void CameraBehaviorOrbitUpdate( CameraManager::CameraContext& ctx);

protected:

    void SwitchBehaviorOnVehicleChange(int newBehaviorID, bool reset, Actor* old_vehicle, Actor* new_vehicle);
    void ToggleCameraBehavior(CameraBehaviors behav); //!< Only accepts FREE and FREEFIX modes
    void ActivateNewBehavior(CameraBehaviors behav_id, bool reset);
    bool EvaluateSwitchBehavior();
    void UpdateCurrentBehavior();
    void ResetCurrentBehavior();
    void DeactivateCurrentBehavior();
    void UpdateCameraBehaviorStatic(const CameraManager::CameraContext& ctx);
    void UpdateCameraBehaviorFree();
    void UpdateCameraBehaviorVehicle();
    void CameraBehaviorVehicleReset();
    bool CameraBehaviorVehicleMousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    void CameraBehaviorVehicleSplineUpdate();
    bool CameraBehaviorVehicleSplineMouseMoved(const OIS::MouseEvent& _arg);
    void CameraBehaviorVehicleSplineReset();
    void CameraBehaviorVehicleSplineCreateSpline();
    void CameraBehaviorVehicleSplineUpdateSpline();
    void CameraBehaviorVehicleSplineUpdateSplineDisplay();

    CameraContext ctx;

    CameraBehaviors m_current_behavior;
    CameraBehaviors m_cam_before_toggled; ///< Toggled modes (FREE, FREEFIX) remember original state.
    CameraBehaviors m_prev_toggled_cam; ///< Switching toggled modes (FREE, FREEFIX) keeps 1-slot history.
    // Old `CameraContext`
    Actor*               m_cct_player_actor; // TODO: duplicates `SimController::m_player_actor`
    DOFManager*          m_cct_dof_manager;
    Ogre::Degree         m_cct_rot_scale;
    Ogre::Real           m_cct_dt;
    Ogre::Real           m_cct_trans_scale;
    Ogre::Radian         m_cct_fov_interior; // TODO: duplicates GVar
    Ogre::Radian         m_cct_fov_exterior; // TODO: Duplicates GVar
    bool                 m_cct_debug;
    float                m_cct_sim_speed; // TODO: duplicates `ActorManager::m_simulation_speed`
    // Old `CameraBehaviorOrbit` attributes
    Ogre::Radian         m_cam_rot_x;
    Ogre::Radian         m_cam_rot_y;
    Ogre::Radian         m_cam_rot_swivel_x;
    Ogre::Radian         m_cam_rot_swivel_y;
    Ogre::Radian         m_cam_target_direction;
    Ogre::Radian         m_cam_target_pitch;
    float                m_cam_dist;
    float                m_cam_dist_min;
    float                m_cam_dist_max;
    float                m_cam_ratio;
    // Static cam attributes
    Ogre::Radian m_staticcam_previous_fov;
    Ogre::Vector3 m_staticcam_position;
    Ogre::Timer m_staticcam_update_timer;
    // Character cam attributes
    bool m_charactercam_is_3rdperson;
    // Spline cam attributes
    Ogre::ManualObject*  m_splinecam_mo;
    Ogre::SimpleSpline*  m_splinecam_spline;
    Ogre::Real           m_splinecam_spline_len;
    Ogre::Real           m_splinecam_spline_pos;
    bool                 m_splinecam_spline_closed;
    bool                 m_splinecam_auto_tracking;
    std::deque<node_t*>  m_splinecam_spline_nodes;
    unsigned int         m_splinecam_num_linked_beams;

    bool m_config_enter_vehicle_keep_fixedfreecam;
    bool m_config_exit_vehicle_keep_fixedfreecam;

    bool mouseMoved(const OIS::MouseEvent& _arg);
    bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
};
