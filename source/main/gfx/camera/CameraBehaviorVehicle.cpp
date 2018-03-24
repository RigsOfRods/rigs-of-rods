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

#include "CameraBehaviorVehicle.h"

#include <Ogre.h>

#include "Application.h"
#include "Beam.h"
#include "InputEngine.h"
#include "Settings.h"

using namespace Ogre;

CameraBehaviorVehicle::CameraBehaviorVehicle() 
{
}

void CameraBehaviorVehicle::update( CameraManager::CameraContext &ctx)
{
	Vector3 dir = ctx.cct_player_actor->getDirection();

	ctx.targetDirection = -atan2(dir.dotProduct(Vector3::UNIT_X), dir.dotProduct(-Vector3::UNIT_Z));
	ctx.targetPitch     = 0.0f;

	if ( RoR::App::gfx_extcam_mode.GetActive() == RoR::GfxExtCamMode::PITCHING)
	{
		ctx.targetPitch = -asin(dir.dotProduct(Vector3::UNIT_Y));
	}

	ctx.camRatio = 1.0f / (ctx.cct_dt * 4.0f);

	ctx.camDistMin = std::min(ctx.cct_player_actor->getMinimalCameraRadius() * 2.0f, 33.0f);

	ctx.camLookAt = ctx.cct_player_actor->getPosition();

	CameraManager::CameraBehaviorOrbitUpdate(ctx);
}

void CameraBehaviorVehicle::reset( CameraManager::CameraContext &ctx)
{
	CameraManager::CameraBehaviorOrbitReset(ctx);
	ctx.camRotY = 0.35f;
	ctx.camDistMin = std::min(ctx.cct_player_actor->getMinimalCameraRadius() * 2.0f, 33.0f);
	ctx.camDist = ctx.camDistMin * 1.5f + 2.0f;
}

bool CameraBehaviorVehicle::mousePressed( CameraManager::CameraContext &ctx, const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	const OIS::MouseState ms = _arg.state;

	if ( ms.buttonDown(OIS::MB_Middle) && RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) )
	{
		if ( ctx.cct_player_actor && ctx.cct_player_actor->ar_custom_camera_node >= 0 )
		{
			// Calculate new camera distance
			Vector3 lookAt = ctx.cct_player_actor->ar_nodes[ctx.cct_player_actor->ar_custom_camera_node].AbsPosition;
			ctx.camDist = 2.0f * gEnv->mainCamera->getPosition().distance(lookAt);

			// Calculate new camera pitch
			Vector3 camDir = (gEnv->mainCamera->getPosition() - lookAt).normalisedCopy();
			ctx.camRotY = asin(camDir.y);

			// Calculate new camera yaw
			Vector3 dir = -ctx.cct_player_actor->getDirection();
			Quaternion rotX = dir.getRotationTo(camDir, Vector3::UNIT_Y);
			ctx.camRotX = rotX.getYaw();

			// Corner case handling
			Radian angle = dir.angleBetween(camDir);
			if ( angle > Radian(Math::HALF_PI) )
			{
				if ( std::abs(Radian(ctx.camRotX).valueRadians()) < Math::HALF_PI )
				{
					if ( ctx.camRotX < Radian(0.0f) )
						ctx.camRotX -= Radian(Math::HALF_PI);
					else
						ctx.camRotX += Radian(Math::HALF_PI);
				}
			}
		}
	}

	return false;
}
