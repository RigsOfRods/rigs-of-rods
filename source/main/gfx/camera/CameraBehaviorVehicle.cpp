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

CameraBehaviorVehicle::CameraBehaviorVehicle() :
	  CameraBehaviorOrbit()
{
}

void CameraBehaviorVehicle::update(const CameraManager::CameraContext &ctx)
{
	Vector3 dir = ctx.mCurrTruck->getDirection();

	targetDirection = -atan2(dir.dotProduct(Vector3::UNIT_X), dir.dotProduct(-Vector3::UNIT_Z));
	targetPitch     = 0.0f;

	if ( RoR::App::gfx_extcam_mode.GetActive() == RoR::GfxExtCamMode::PITCHING)
	{
		targetPitch = -asin(dir.dotProduct(Vector3::UNIT_Y));
	}

	camRatio = 1.0f / (ctx.mDt * 4.0f);

	camDistMin = std::min(ctx.mCurrTruck->getMinimalCameraRadius() * 2.0f, 33.0f);

	camLookAt = ctx.mCurrTruck->getPosition();

	CameraBehaviorOrbit::update(ctx);
}

void CameraBehaviorVehicle::activate(const CameraManager::CameraContext &ctx, bool reset /* = true */)
{
	if ( !ctx.mCurrTruck )
	{
		gEnv->cameraManager->switchToNextBehavior();
		return;
	} 
	else if ( reset )
	{
		this->reset(ctx);
	}
	ctx.mCurrTruck->GetCameraContext()->behavior = RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_3rdPERSON;
}

void CameraBehaviorVehicle::reset(const CameraManager::CameraContext &ctx)
{
	CameraBehaviorOrbit::reset(ctx);
	camRotY = 0.35f;
	camDistMin = std::min(ctx.mCurrTruck->getMinimalCameraRadius() * 2.0f, 33.0f);
	camDist = camDistMin * 1.5f + 2.0f;
}

bool CameraBehaviorVehicle::mousePressed(const CameraManager::CameraContext &ctx, const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	const OIS::MouseState ms = _arg.state;

	if ( ms.buttonDown(OIS::MB_Middle) && RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) )
	{
		if ( ctx.mCurrTruck && ctx.mCurrTruck->m_custom_camera_node >= 0 )
		{
			// Calculate new camera distance
			Vector3 lookAt = ctx.mCurrTruck->nodes[ctx.mCurrTruck->m_custom_camera_node].AbsPosition;
			camDist = 2.0f * gEnv->mainCamera->getPosition().distance(lookAt);

			// Calculate new camera pitch
			Vector3 camDir = (gEnv->mainCamera->getPosition() - lookAt).normalisedCopy();
			camRotY = asin(camDir.y);

			// Calculate new camera yaw
			Vector3 dir = -ctx.mCurrTruck->getDirection();
			Quaternion rotX = dir.getRotationTo(camDir, Vector3::UNIT_Y);
			camRotX = rotX.getYaw();

			// Corner case handling
			Radian angle = dir.angleBetween(camDir);
			if ( angle > Radian(Math::HALF_PI) )
			{
				if ( std::abs(Radian(camRotX).valueRadians()) < Math::HALF_PI )
				{
					if ( camRotX < Radian(0.0f) )
						camRotX -= Radian(Math::HALF_PI);
					else
						camRotX += Radian(Math::HALF_PI);
				}
			}
		}
	}

	return false;
}
