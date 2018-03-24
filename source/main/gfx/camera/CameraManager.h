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

#include "IBehavior.h"

#include <OIS.h>

class CameraManager
{
    friend class RoR::SceneMouse;

public:

    CameraManager();
    ~CameraManager();

    class CameraContext
    {
    public:

        Actor*       cct_player_actor;
        DOFManager*  cct_dof_manager;
        Ogre::Degree cct_rot_scale;
        Ogre::Real   cct_dt;
        Ogre::Real   cct_trans_scale;
        Ogre::Radian cct_fov_interior;
        Ogre::Radian cct_fov_exterior;
        bool         cct_debug;
        float        cct_sim_speed;
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
        CAMERA_BEHAVIOR_INVALID = 0xFFFFFFFF,
    };

    bool Update(float dt, Actor* player_vehicle, float sim_speed);

    void switchBehavior(int newBehavior, bool reset = true);
    void switchToNextBehavior(bool force = true);


    bool gameControlsLocked();
    bool hasActiveBehavior();

    int getCurrentBehavior();

    void OnReturnToMainMenu();
    void NotifyContextChange();
    void NotifyVehicleChanged(Actor* old_vehicle, Actor* new_vehicle);
    void ActivateDepthOfFieldEffect();
    void DisableDepthOfFieldEffect();

protected:

    void SwitchBehaviorOnVehicleChange(int newBehaviorID, bool reset, Actor* old_vehicle, Actor* new_vehicle);
    void ToggleCameraBehavior(CameraBehaviors behav); //!< Only accepts FREE and FREEFIX modes

    IBehavior<CameraContext>* FindBehavior(int behaviorID); // TODO: eliminate the `int ID`

    CameraContext ctx;

    float mTransScale, mTransSpeed;
    float mRotScale, mRotateSpeed;

    int currentBehaviorID;
    IBehavior<CameraContext>* currentBehavior;
    CameraBehaviors m_cam_before_toggled; ///< Toggled modes (FREE, FREEFIX) remember original state.
    CameraBehaviors m_prev_toggled_cam; ///< Switching toggled modes (FREE, FREEFIX) keeps 1-slot history.
    // Global behaviors
    IBehavior<CameraContext>* m_cam_behav_character;
    IBehavior<CameraContext>* m_cam_behav_static;
    IBehavior<CameraContext>* m_cam_behav_vehicle;
    IBehavior<CameraContext>* m_cam_behav_vehicle_spline;
    IBehavior<CameraContext>* m_cam_behav_vehicle_cinecam;
    IBehavior<CameraContext>* m_cam_behav_free;
    IBehavior<CameraContext>* m_cam_behav_fixed;
    IBehavior<CameraContext>* m_cam_behav_isometric;

    bool m_config_enter_vehicle_keep_fixedfreecam;
    bool m_config_exit_vehicle_keep_fixedfreecam;

    bool mouseMoved(const OIS::MouseEvent& _arg);
    bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
};
