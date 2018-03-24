/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2018 Petr Ohlidal

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
    
    m_camera_manager(camera_mgr)
{
}

void CameraBehaviorVehicleCineCam::update( CameraManager::CameraContext &ctx)
{
    CameraManager::CameraBehaviorOrbitUpdate(ctx);

    Vector3 dir = (ctx.cct_player_actor->ar_nodes[ctx.cct_player_actor->ar_camera_node_pos[ctx.cct_player_actor->ar_current_cinecam]].AbsPosition
                 - ctx.cct_player_actor->ar_nodes[ctx.cct_player_actor->ar_camera_node_dir[ctx.cct_player_actor->ar_current_cinecam]].AbsPosition).normalisedCopy();

    Vector3 roll = (ctx.cct_player_actor->ar_nodes[ctx.cct_player_actor->ar_camera_node_pos[ctx.cct_player_actor->ar_current_cinecam]].AbsPosition
                  - ctx.cct_player_actor->ar_nodes[ctx.cct_player_actor->ar_camera_node_roll[ctx.cct_player_actor->ar_current_cinecam]].AbsPosition).normalisedCopy();

    if ( ctx.cct_player_actor->ar_camera_node_roll_inv[ctx.cct_player_actor->ar_current_cinecam] )
    {
        roll = -roll;
    }

    Vector3 up = dir.crossProduct(roll);

    roll = up.crossProduct(dir);

    Quaternion orientation = Quaternion(ctx.camRotX + ctx.camRotXSwivel, up) * Quaternion(Degree(180.0) + ctx.camRotY + ctx.camRotYSwivel, roll) * Quaternion(roll, up, dir);

    gEnv->mainCamera->setPosition(ctx.cct_player_actor->ar_nodes[ctx.cct_player_actor->ar_cinecam_node[ctx.cct_player_actor->ar_current_cinecam]].AbsPosition);
    gEnv->mainCamera->setOrientation(orientation);
}

void CameraBehaviorVehicleCineCam::deactivate( CameraManager::CameraContext &ctx)
{
    Actor* current_vehicle = ctx.cct_player_actor;
    if ( current_vehicle == nullptr 
        || current_vehicle->GetCameraContext()->behavior != RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_CINECAM )
    {
        return;
    }

    gEnv->mainCamera->setFOVy(ctx.cct_fov_exterior);
    
    current_vehicle->prepareInside(false);

    if ( RoR::App::GetOverlayWrapper() != nullptr )
    {
        RoR::App::GetOverlayWrapper()->showDashboardOverlays(true, current_vehicle);
    }

    current_vehicle->GetCameraContext()->last_cinecam_index = current_vehicle->ar_current_cinecam;

    current_vehicle->GetCameraContext()->last_cinecam_index = false;
    current_vehicle->ar_current_cinecam = -1;
    current_vehicle->NotifyActorCameraChanged();
}

void CameraBehaviorVehicleCineCam::reset( CameraManager::CameraContext &ctx)
{
    CameraManager::CameraBehaviorOrbitReset(ctx);
    ctx.camRotY = Degree(DEFAULT_INTERNAL_CAM_PITCH);
    gEnv->mainCamera->setFOVy(ctx.cct_fov_interior);
}

bool CameraBehaviorVehicleCineCam::switchBehavior( CameraManager::CameraContext &ctx)
{
    Actor* vehicle = ctx.cct_player_actor;
    if ( (vehicle != nullptr) && (vehicle->ar_current_cinecam) < (vehicle->ar_num_cinecams-1) )
    {
        vehicle->ar_current_cinecam++;
        vehicle->GetCameraContext()->last_cinecam_index = vehicle->ar_current_cinecam;
        vehicle->NotifyActorCameraChanged();
        return false;
    }
    return true;
}
