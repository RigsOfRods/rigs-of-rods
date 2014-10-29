/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "CameraBehaviorVehicle.h"

#include "Beam.h"
#include "BeamFactory.h"
#include "Settings.h"

using namespace Ogre;

CameraBehaviorVehicle::CameraBehaviorVehicle() :
	  CameraBehaviorOrbit()
	, camPitching(true)
{
	if ( SSETTING("External Camera Mode", "Pitching") == "Static" )
	{
		camPitching = false;
	}
}

void CameraBehaviorVehicle::update(const CameraManager::CameraContext &ctx)
{
	Vector3 dir = (ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodepos[0]].smoothpos
				 - ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodedir[0]].smoothpos).normalisedCopy();

	targetDirection = -atan2(dir.dotProduct(Vector3::UNIT_X), dir.dotProduct(-Vector3::UNIT_Z));
	targetPitch     = 0.0f;

	if ( camPitching )
	{
		targetPitch = -asin(dir.dotProduct(Vector3::UNIT_Y));
	}

	if (BeamFactory::getSingleton().getThreadingMode() == THREAD_MULTI)
		camRatio = 1.0f / (ctx.mCurrTruck->ttdt * 4.0f);
	else
		camRatio = 1.0f / (ctx.mCurrTruck->tdt * 4.0f);

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
	} else if ( reset )
	{
		this->reset(ctx);
	}
}

void CameraBehaviorVehicle::reset(const CameraManager::CameraContext &ctx)
{
	CameraBehaviorOrbit::reset(ctx);
	camRotY = 0.35f;
	camDistMin = std::min(ctx.mCurrTruck->getMinimalCameraRadius() * 2.0f, 33.0f);
	camDist = camDistMin * 1.5f + 2.0f;
}
