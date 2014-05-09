/*/
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
#include "CameraBehaviorVehicleCineCam.h"

#include "Beam.h"
#include "OverlayWrapper.h"

using namespace Ogre;

CameraBehaviorVehicleCineCam::CameraBehaviorVehicleCineCam() :
	  CameraBehaviorVehicle()
	, currTruck(0)
	, lastCineCam(0)
{
}

void CameraBehaviorVehicleCineCam::update(const CameraManager::CameraContext &ctx)
{
	CameraBehaviorOrbit::update(ctx);

	Vector3 dir = (ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodepos[ctx.mCurrTruck->currentcamera]].smoothpos
				 - ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodedir[ctx.mCurrTruck->currentcamera]].smoothpos).normalisedCopy();

	Vector3 roll = (ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodepos[ctx.mCurrTruck->currentcamera]].smoothpos
				  - ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranoderoll[ctx.mCurrTruck->currentcamera]].smoothpos).normalisedCopy();

	if ( ctx.mCurrTruck->revroll[ctx.mCurrTruck->currentcamera] )
	{
		roll = -roll;
	}

	Vector3 up = dir.crossProduct(roll);

	roll = up.crossProduct(dir);

	Quaternion orientation = Quaternion(camRotX + camRotXSwivel, up) * Quaternion(Degree(180.0) + camRotY + camRotYSwivel, roll) * Quaternion(roll, up, dir);

	gEnv->mainCamera->setPosition(ctx.mCurrTruck->nodes[ctx.mCurrTruck->cinecameranodepos[ctx.mCurrTruck->currentcamera]].smoothpos);
	gEnv->mainCamera->setOrientation(orientation);
}

void CameraBehaviorVehicleCineCam::activate(const CameraManager::CameraContext &ctx, bool reset /* = true */)
{
	if ( !ctx.mCurrTruck || ctx.mCurrTruck->freecinecamera <= 0 )
	{
		gEnv->cameraManager->switchToNextBehavior();
		return;
	} else if ( reset )
	{
		lastCineCam = 0;
		this->reset(ctx);
	}

	currTruck = ctx.mCurrTruck;

	gEnv->mainCamera->setFOVy(ctx.fovInternal);

	ctx.mCurrTruck->prepareInside(true);

	if ( ctx.mOverlayWrapper )
	{
		if ( ctx.mCurrTruck->driveable == AIRPLANE )
			ctx.mOverlayWrapper->showDashboardOverlays(true, ctx.mCurrTruck);
		else
			ctx.mOverlayWrapper->showDashboardOverlays(false, ctx.mCurrTruck);
	}

	ctx.mCurrTruck->currentcamera = lastCineCam;
	ctx.mCurrTruck->changedCamera();
}

void CameraBehaviorVehicleCineCam::deactivate(const CameraManager::CameraContext &ctx)
{
	// Do not use ctx.mCurrTruck in here (could be null)
	if ( !currTruck )
	{
		return;
	}

	gEnv->mainCamera->setFOVy(ctx.fovExternal);
		
	currTruck->prepareInside(false);

	if ( ctx.mOverlayWrapper )
	{
		ctx.mOverlayWrapper->showDashboardOverlays(true, currTruck);
	}

	lastCineCam = currTruck->currentcamera;

	currTruck->currentcamera = -1;
	currTruck->changedCamera();
	currTruck = 0;
}

void CameraBehaviorVehicleCineCam::reset(const CameraManager::CameraContext &ctx)
{
	CameraBehaviorOrbit::reset(ctx);
	camRotY = Degree(DEFAULT_INTERNAL_CAM_PITCH);
	gEnv->mainCamera->setFOVy(ctx.fovInternal);
}

bool CameraBehaviorVehicleCineCam::switchBehavior(const CameraManager::CameraContext &ctx)
{
	if ( ctx.mCurrTruck && ctx.mCurrTruck->currentcamera < ctx.mCurrTruck->freecinecamera-1 )
	{
		ctx.mCurrTruck->currentcamera++;
		ctx.mCurrTruck->changedCamera();
		return false;
	}
	return true;
}
