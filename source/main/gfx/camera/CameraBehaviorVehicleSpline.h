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

#include "CameraBehaviorVehicle.h"

class CameraBehaviorVehicleSpline : public CameraBehaviorVehicle
{
public:

    CameraBehaviorVehicleSpline();
    ~CameraBehaviorVehicleSpline();

    void update(const CameraManager::CameraContext& ctx);

    bool mouseMoved(const CameraManager::CameraContext& ctx, const OIS::MouseEvent& _arg);

    void activate(const CameraManager::CameraContext& ctx, bool reset = true);
    void reset(const CameraManager::CameraContext& ctx);

    void createSpline(const CameraManager::CameraContext& ctx);
    void updateSpline();
    void updateSplineDisplay();

protected:

    Ogre::ManualObject* splineObject;
    Ogre::SimpleSpline* spline;
    Ogre::Real splineLength;
    Ogre::Real splinePos;
    bool splineClosed;
    bool autoTracking;

    std::deque<node_t*> splineNodes;
    unsigned int numLinkedBeams;

    static const int splineDrawResolution = 200;
};
