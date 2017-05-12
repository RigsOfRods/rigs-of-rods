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

// Forward decl.
class CameraBehaviorStatic;
class CameraBehaviorVehicleSpline;
class CameraBehaviorCharacter;
class CameraBehaviorVehicle;
class CameraBehaviorVehicleCineCam;
class CameraBehaviorOrbit;

class CameraManager
{
    friend class RoR::SceneMouse;

public:

    CameraManager();
    ~CameraManager();

    class CameraContext
    {
    public:

        Beam* mCurrTruck;
        DOFManager* mDof;
        Ogre::Degree mRotScale;
        Ogre::Real mDt;
        Ogre::Real mTransScale;
        Ogre::Radian fovInternal;
        Ogre::Radian fovExternal;
        bool mDebug;
        float mSimSpeed;
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

    bool Update(float dt, Beam* cur_truck, float sim_speed);

    void switchBehavior(int newBehavior, bool reset = true);
    void switchToNextBehavior();
    void toggleBehavior(int behavior);

    bool gameControlsLocked();
    bool hasActiveBehavior();

    int getCurrentBehavior();

    void OnReturnToMainMenu();
    void NotifyContextChange();
    void NotifyVehicleChanged(Beam* old_vehicle, Beam* new_vehicle);
    void SetSimController(RoRFrameListener* sim);

protected:

    void SwitchBehaviorOnVehicleChange(int newBehaviorID, bool reset, Beam* old_vehicle, Beam* new_vehicle);
    void ActivateNewBehavior(CameraBehaviors behav_id, bool reset);
    void DeActivateCurrentBehavior();
    void UpdateFreeCamera();

    IBehavior<CameraContext>* FindBehavior(int behaviorID); // TODO: eliminate the `int ID`

    CameraContext ctx;

    float mTransScale, mTransSpeed;
    float mRotScale, mRotateSpeed;

    CameraBehaviors m_active_behavior;
    CameraBehaviors m_cam_before_free;  ///< Activated by toggling; remembers previous mode.
    CameraBehaviors m_cam_before_fixed; ///< Activated by toggling; remembers previous mode.
    // Global behaviors
    CameraBehaviorCharacter*      m_cam_behav_character;
    CameraBehaviorStatic*         m_cam_behav_static;
    CameraBehaviorVehicle*        m_cam_behav_vehicle;
    CameraBehaviorVehicleSpline*  m_cam_behav_vehicle_spline;
    CameraBehaviorVehicleCineCam* m_cam_behav_vehicle_cinecam;
    IBehavior<CameraContext>*     m_cam_behav_isometric;

    bool m_config_enter_vehicle_keep_fixedfreecam;
    bool m_config_exit_vehicle_keep_fixedfreecam;

    bool mouseMoved(const OIS::MouseEvent& _arg);
    bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
};
