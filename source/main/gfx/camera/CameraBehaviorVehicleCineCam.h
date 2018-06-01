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
#include "CameraManager.h"

class CameraBehaviorVehicleCineCam : public CameraBehaviorVehicle
{
public:

    CameraBehaviorVehicleCineCam(CameraManager* camera_mgr);

    void update( CameraManager::CameraContext& ctx);

    void deactivate( CameraManager::CameraContext& ctx);
    void reset( CameraManager::CameraContext& ctx);

    bool switchBehavior( CameraManager::CameraContext& ctx);

protected:

    CameraManager* m_camera_manager;

    static const int DEFAULT_INTERNAL_CAM_PITCH = -15;
};

