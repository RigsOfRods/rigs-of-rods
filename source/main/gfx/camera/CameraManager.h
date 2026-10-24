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

    void ResetLookatPos();
    void NotifyVehicleChanged(ActorPtr new_vehicle);

    bool handleMouseMoved();
    bool handleMousePressed();

    void ResetAllBehaviors();
    void ReCreateCameraNode(); //!< Needed since we call `Ogre::SceneManager::ClearScene()` after end of sim. session

    void switchToNextBehavior();
    bool EvaluateSwitchBehavior();

protected:

    // Camera behavior management
    void switchBehavior(CameraBehaviors new_behavior);
    void SwitchBehaviorOnVehicleChange(CameraBehaviors new_behavior, ActorPtr new_vehicle);
    void ToggleCameraBehavior(CameraBehaviors new_behavior); //!< Only accepts FREE and FREEFIX modes
    void ActivateNewBehavior(CameraBehaviors new_behavior, bool reset);
    void UpdateCurrentBehavior(float dt);
    void ResetCurrentBehavior();
    void DeactivateCurrentBehavior();

    // Orbit cam (helper)
    void CameraBehaviorOrbitReset();
    bool CameraBehaviorOrbitMouseMoved();
    void CameraBehaviorOrbitUpdate(float dt);

    // Character cam
    bool CameraBehaviorCharacterMouseMoved(float dt);

    // Static cam
    void UpdateCameraBehaviorStatic(float dt);
    bool CameraBehaviorStaticMouseMoved();

    // Free cam
    void UpdateCameraBehaviorFree(float dt);
    bool CameraBehaviorFreeMouseMoved(float dt);

    // Free-fix cam
    void UpdateCameraBehaviorFixed(float dt);

    // Vehicle cam
    void UpdateCameraBehaviorVehicle(float dt);
    void CameraBehaviorVehicleReset();
    bool CameraBehaviorVehicleMousePressed();

    // Vehicle-spline cam
    void CameraBehaviorVehicleSplineUpdate(float dt);
    bool CameraBehaviorVehicleSplineMouseMoved();
    void CameraBehaviorVehicleSplineReset();
    void CameraBehaviorVehicleSplineCreateSpline();
    void CameraBehaviorVehicleSplineUpdateSpline();
    void CameraBehaviorVehicleSplineUpdateSplineDisplay();

    // Internal helpers
    void CreateCameraNode();
    void UpdateMouseLook(float dt);

    Ogre::Camera*        m_camera {nullptr};
    Ogre::SceneNode*     m_camera_node {nullptr};

    CameraBehaviors      m_current_behavior {CAMERA_BEHAVIOR_INVALID};
    ActorPtr             m_current_actor; //!< Kept around for the camera-switching process (deactivate old/activate new).

    CameraBehaviors      m_cam_before_toggled {CAMERA_BEHAVIOR_INVALID};  //!< Toggled modes (FREE, FREEFIX) remember original state.
    CameraBehaviors      m_prev_toggled_cam {CAMERA_BEHAVIOR_INVALID};    //!< Switching toggled modes (FREE, FREEFIX) keeps 1-slot history.

    // We defer processing mouse input (received via listener) until we have 'dt' (received via `UpdateInputEvents()`).
    // Note the actual mouse data are buffered by `InputEngine`.
    bool                 m_mouse_moved = false;
    bool                 m_mouse_pressed = false;
    // Common mouse smoothing for freecam + 1st person charactercam.
    Ogre::Vector2        m_mouselook_smooth_vec {Ogre::Vector2::ZERO};

    // `CameraBehaviorOrbit` attributes
    Ogre::Radian         m_cam_rot_x {0.f};
    Ogre::Radian         m_cam_rot_y {0.3f};
    Ogre::Radian         m_cam_target_direction {0.f};
    Ogre::Radian         m_cam_target_pitch {0.f};
    float                m_cam_dist {5.f};
    float                m_cam_dist_min {0.f};
    float                m_cam_dist_max {0.f};
    float                m_cam_ratio {11.f};
    Ogre::Vector3        m_cam_look_at {Ogre::Vector3::ZERO};
    bool                 m_cam_limit_movement {true};
    Ogre::Vector3        m_cam_look_at_last {Ogre::Vector3::ZERO};
    Ogre::Vector3        m_cam_look_at_smooth {Ogre::Vector3::ZERO};
    Ogre::Vector3        m_cam_look_at_smooth_last {Ogre::Vector3::ZERO};

    // Static cam attributes
    bool                 m_staticcam_force_update {false};
    float                m_staticcam_fov_exponent {1.f};
    Ogre::Radian         m_staticcam_previous_fov {Ogre::Radian(0)};
    Ogre::Vector3        m_staticcam_look_at {Ogre::Vector3::ZERO};
    Ogre::Vector3        m_staticcam_position {Ogre::Vector3::ZERO};
    Ogre::Timer          m_staticcam_update_timer;

    // Character cam attributes
    bool                 m_charactercam_is_3rdperson {true};

    // Spline cam attributes
    Ogre::ManualObject*  m_splinecam_mo {nullptr};
    Ogre::SimpleSpline*  m_splinecam_spline {new Ogre::SimpleSpline()};
    Ogre::Real           m_splinecam_spline_len {1.f};
    Ogre::Real           m_splinecam_spline_pos {0.5f};
    bool                 m_splinecam_spline_closed {false};
    bool                 m_splinecam_auto_tracking {false};
    std::deque<node_t*>  m_splinecam_spline_nodes;
    unsigned int         m_splinecam_num_linked_beams {0};

};

/// @} // addtogroup Camera
/// @} // addtogroup Gfx

} // namespace RoR
