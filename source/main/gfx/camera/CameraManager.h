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
#include "IBehaviorManager.h"

#include <OIS.h>

class CameraManager : public IBehaviorManager
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
        CAMERA_BEHAVIOR_ISOMETRIC
    };

    /**
    * TIGHT-LOOP; Called once per frame.
    */
    bool update(float dt);

    void switchBehavior(int newBehavior, bool reset = true);
    void switchToNextBehavior(bool force = true);
    void toggleBehavior(int behavior);

    bool gameControlsLocked();
    bool hasActiveBehavior();
    bool hasActiveCharacterBehavior();
    bool hasActiveVehicleBehavior();

    int getCurrentBehavior();

    void OnReturnToMainMenu();
    void NotifyContextChange();
    void NotifyVehicleChanged(Beam* old_vehicle, Beam* new_vehicle);

protected:

    void createGlobalBehaviors();
    void SwitchBehaviorOnVehicleChange(int newBehaviorID, bool reset, Beam* old_vehicle, Beam* new_vehicle);

    CameraContext ctx;

    float mTransScale, mTransSpeed;
    float mRotScale, mRotateSpeed;

    int currentBehaviorID;
    IBehavior<CameraContext>* currentBehavior;

    std::map<int, IBehavior<CameraContext> *> globalBehaviors;

    bool m_config_enter_vehicle_keep_fixedfreecam;
    bool m_config_exit_vehicle_keep_fixedfreecam;

    bool mouseMoved(const OIS::MouseEvent& _arg);
    bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
};
