/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2014 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "CameraBehaviorVehicleCineCam.h"

#include "Application.h"
#include "Beam.h"
#include "OverlayWrapper.h"
#include "PerVehicleCameraContext.h"

#include <OgreCamera.h>

using namespace Ogre;

CameraBehaviorVehicleCineCam::CameraBehaviorVehicleCineCam(CameraManager* camera_mgr) :
    CameraBehaviorVehicle(),
    m_camera_manager(camera_mgr)
{
}

void CameraBehaviorVehicleCineCam::update(const CameraManager::CameraContext &ctx)
{
    CameraBehaviorOrbit::update(ctx);

    Vector3 dir = (ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodepos[ctx.mCurrTruck->currentcamera]].AbsPosition
                 - ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodedir[ctx.mCurrTruck->currentcamera]].AbsPosition).normalisedCopy();

    Vector3 roll = (ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodepos[ctx.mCurrTruck->currentcamera]].AbsPosition
                  - ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranoderoll[ctx.mCurrTruck->currentcamera]].AbsPosition).normalisedCopy();

    if ( ctx.mCurrTruck->revroll[ctx.mCurrTruck->currentcamera] )
    {
        roll = -roll;
    }

    Vector3 up = dir.crossProduct(roll);

    roll = up.crossProduct(dir);

    Quaternion orientation = Quaternion(camRotX + camRotXSwivel, up) * Quaternion(Degree(180.0) + camRotY + camRotYSwivel, roll) * Quaternion(roll, up, dir);

    gEnv->mainCamera->setPosition(ctx.mCurrTruck->nodes[ctx.mCurrTruck->cinecameranodepos[ctx.mCurrTruck->currentcamera]].AbsPosition);
    gEnv->mainCamera->setOrientation(orientation);
}

void CameraBehaviorVehicleCineCam::activate(const CameraManager::CameraContext &ctx, bool reset /* = true */)
{
    Beam* current_vehicle = ctx.mCurrTruck;
    if (current_vehicle == nullptr)
    {
        m_camera_manager->switchToNextBehavior();
        return;
    }
    RoR::PerVehicleCameraContext* vehicle_cam_context = current_vehicle->GetCameraContext();
    if ( current_vehicle->freecinecamera <= 0 )
    {
        m_camera_manager->switchToNextBehavior();
        return;
    } 
    else if ( reset )
    {
        this->reset(ctx);
    }

    gEnv->mainCamera->setFOVy(ctx.fovInternal);

    current_vehicle->prepareInside(true);

    if ( RoR::App::GetOverlayWrapper() )
    {
        RoR::App::GetOverlayWrapper()->showDashboardOverlays(
            (current_vehicle->driveable == AIRPLANE), 
            current_vehicle
        );
    }

    current_vehicle->currentcamera = current_vehicle->GetCameraContext()->last_cinecam_index;
    current_vehicle->changedCamera();

    vehicle_cam_context->behavior = RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_CINECAM;
}

void CameraBehaviorVehicleCineCam::deactivate(const CameraManager::CameraContext &ctx)
{
    Beam* current_vehicle = ctx.mCurrTruck;
    if ( current_vehicle == nullptr 
        || current_vehicle->GetCameraContext()->behavior != RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_CINECAM )
    {
        return;
    }

    gEnv->mainCamera->setFOVy(ctx.fovExternal);
    
    current_vehicle->prepareInside(false);

    if ( RoR::App::GetOverlayWrapper() != nullptr )
    {
        RoR::App::GetOverlayWrapper()->showDashboardOverlays(true, current_vehicle);
    }

    current_vehicle->GetCameraContext()->last_cinecam_index = current_vehicle->currentcamera;

    current_vehicle->GetCameraContext()->last_cinecam_index = false;
    current_vehicle->currentcamera = -1;
    current_vehicle->changedCamera();
}

void CameraBehaviorVehicleCineCam::reset(const CameraManager::CameraContext &ctx)
{
    CameraBehaviorOrbit::reset(ctx);
    camRotY = Degree(DEFAULT_INTERNAL_CAM_PITCH);
    gEnv->mainCamera->setFOVy(ctx.fovInternal);
}

bool CameraBehaviorVehicleCineCam::switchBehavior(const CameraManager::CameraContext &ctx)
{
    Beam* vehicle = ctx.mCurrTruck;
    if ( (vehicle != nullptr) && (vehicle->currentcamera) < (vehicle->freecinecamera-1) )
    {
        vehicle->currentcamera++;
        vehicle->GetCameraContext()->last_cinecam_index = vehicle->currentcamera;
        vehicle->changedCamera();
        return false;
    }
    return true;
}
