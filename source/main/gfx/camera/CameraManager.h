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

#pragma once

#include "Application.h"

#include <OIS.h>
#include <Ogre.h>

namespace RoR {

/// @addtogroup Gfx
/// @{

/// @addtogroup Camera
/// @{

class CameraManager
{
public:

    CameraManager();
    ~CameraManager();

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

    void UpdateInputEvents(float dt);

    bool hasActiveBehavior();

    CameraBehaviors   GetCurrentBehavior() const  { return m_current_behavior; }
    Ogre::SceneNode*  GetCameraNode()             { return m_camera_node; }
    Ogre::Camera*     GetCamera()                 { return m_camera; }

    void NotifyContextChange();
    void NotifyVehicleChanged(ActorPtr new_vehicle);

    void CameraBehaviorOrbitReset();
    bool CameraBehaviorOrbitMouseMoved(const OIS::MouseEvent& _arg);
    void CameraBehaviorOrbitUpdate();

    bool mouseMoved(const OIS::MouseEvent& _arg);
    bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);

    void ResetAllBehaviors();
    void ReCreateCameraNode(); //!< Needed since we call `Ogre::SceneManager::ClearScene()` after end of sim. session

    void switchToNextBehavior();
    bool EvaluateSwitchBehavior();

protected:

    void switchBehavior(CameraBehaviors new_behavior);
    void SwitchBehaviorOnVehicleChange(CameraBehaviors new_behavior, ActorPtr new_vehicle);
    void ToggleCameraBehavior(CameraBehaviors new_behavior); //!< Only accepts FREE and FREEFIX modes
    void ActivateNewBehavior(CameraBehaviors new_behavior, bool reset);
    void UpdateCurrentBehavior();
    void ResetCurrentBehavior();
    void DeactivateCurrentBehavior();
    void UpdateCameraBehaviorStatic();
    bool CameraBehaviorStaticMouseMoved(const OIS::MouseEvent& _arg);
    void UpdateCameraBehaviorFree();
    void UpdateCameraBehaviorFixed();
    void UpdateCameraBehaviorVehicle();
    void CameraBehaviorVehicleReset();
    bool CameraBehaviorVehicleMousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    void CameraBehaviorVehicleSplineUpdate();
    bool CameraBehaviorVehicleSplineMouseMoved(const OIS::MouseEvent& _arg);
    void CameraBehaviorVehicleSplineReset();
    void CameraBehaviorVehicleSplineCreateSpline();
    void CameraBehaviorVehicleSplineUpdateSpline();
    void CameraBehaviorVehicleSplineUpdateSplineDisplay();
    void CreateCameraNode();

    Ogre::Camera*        m_camera;
    Ogre::SceneNode*     m_camera_node;

    CameraBehaviors      m_current_behavior;
    CameraBehaviors      m_cam_before_toggled;  //!< Toggled modes (FREE, FREEFIX) remember original state.
    CameraBehaviors      m_prev_toggled_cam;    //!< Switching toggled modes (FREE, FREEFIX) keeps 1-slot history.
    // Old `CameraContext`
    ActorPtr             m_cct_player_actor; // TODO: duplicates `GameContext::m_player_actor`
    Ogre::Degree         m_cct_rot_scale;
    Ogre::Real           m_cct_dt;
    Ogre::Real           m_cct_trans_scale;
    float                m_cct_sim_speed; // TODO: duplicates `ActorManager::m_simulation_speed`
    // Old `CameraBehaviorOrbit` attributes
    Ogre::Radian         m_cam_rot_x;
    Ogre::Radian         m_cam_rot_y;
    Ogre::Radian         m_cam_target_direction;
    Ogre::Radian         m_cam_target_pitch;
    float                m_cam_dist;
    float                m_cam_dist_min;
    float                m_cam_dist_max;
    float                m_cam_ratio;
    Ogre::Vector3        m_cam_look_at;
    bool                 m_cam_limit_movement;
    Ogre::Vector3        m_cam_look_at_last;
    Ogre::Vector3        m_cam_look_at_smooth;
    Ogre::Vector3        m_cam_look_at_smooth_last;
    // Static cam attributes
    bool                 m_staticcam_force_update;
    float                m_staticcam_fov_exponent;
    Ogre::Radian         m_staticcam_previous_fov;
    Ogre::Vector3        m_staticcam_look_at;
    Ogre::Vector3        m_staticcam_position;
    Ogre::Timer          m_staticcam_update_timer;
    // Character cam attributes
    bool                 m_charactercam_is_3rdperson;
    // Spline cam attributes
    Ogre::ManualObject*  m_splinecam_mo;
    Ogre::SimpleSpline*  m_splinecam_spline;
    Ogre::Real           m_splinecam_spline_len;
    Ogre::Real           m_splinecam_spline_pos;
    bool                 m_splinecam_spline_closed;
    bool                 m_splinecam_auto_tracking;
    std::deque<NodeNum_t>  m_splinecam_spline_nodes;
    unsigned int         m_splinecam_num_linked_beams;
};

/// @} // addtogroup Camera
/// @} // addtogroup Gfx

} // namespace RoR
