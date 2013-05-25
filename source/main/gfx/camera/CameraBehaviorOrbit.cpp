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
#include "CameraBehaviorOrbit.h"

#include "Beam.h"
#include "Collisions.h"
#include "Console.h"
#include "IHeightFinder.h"
#include "InputEngine.h"
#include "Language.h"
#include "Ogre.h"
#include "TerrainManager.h"

using namespace Ogre;

CameraBehaviorOrbit::CameraBehaviorOrbit() :
	  camDist(5.0f)
	, camDistMax(0.0f)
	, camDistMin(0.0f)
	, camLookAt(Vector3::ZERO)
	, camRatio(11.0f)
	, camRotX(0.0f)
	, camRotY(0.3f)
	, limitMinCamDist(true)
	, targetDirection(0.0f)
	, targetPitch(0.0f)
{
}

void CameraBehaviorOrbit::update(const CameraManager::CameraContext &ctx)
{
	Degree mRotX(0.0f);
	Degree mRotY(0.0f);

	if ( INPUTENGINE.getEventBoolValueBounce(EV_CAMERA_LOOKBACK) )
	{
		if ( camRotX > Degree(0) )
		{
			camRotX = Degree(0);
		} else
		{
			camRotX = Degree(180);
		}
	}
	
	mRotX -= ctx.mRotScale * INPUTENGINE.getEventValue(EV_CAMERA_ROTATE_LEFT);
	mRotX += ctx.mRotScale * INPUTENGINE.getEventValue(EV_CAMERA_ROTATE_RIGHT);
	
	mRotY += ctx.mRotScale * INPUTENGINE.getEventValue(EV_CAMERA_ROTATE_UP);
	mRotY -= ctx.mRotScale * INPUTENGINE.getEventValue(EV_CAMERA_ROTATE_DOWN);

	camRotX += mRotX;
	camRotY += mRotY;

	Radian rotXWithSwivel = camRotX;
	Radian rotYWithSwivel = camRotY;

	rotXWithSwivel -= INPUTENGINE.getEventValue(EV_CAMERA_SWIVEL_LEFT)  * Degree(90);
	rotXWithSwivel += INPUTENGINE.getEventValue(EV_CAMERA_SWIVEL_RIGHT) * Degree(90);

	rotYWithSwivel += INPUTENGINE.getEventValue(EV_CAMERA_SWIVEL_UP)   * Degree(60);
	rotYWithSwivel -= INPUTENGINE.getEventValue(EV_CAMERA_SWIVEL_DOWN) * Degree(60);

	rotYWithSwivel = std::max((Radian)Degree(-80), rotYWithSwivel);
	rotYWithSwivel = std::min(rotYWithSwivel, (Radian)Degree(88));

	if ( INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_IN) && camDist > 1 )
	{
		camDist -= ctx.mTransScale;
	}
	if ( INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_IN_FAST) && camDist > 1 )
	{
		camDist -= ctx.mTransScale * 10;
	}
	if ( INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_OUT) )
	{
		camDist += ctx.mTransScale;
	}
	if ( INPUTENGINE.getEventBoolValue(EV_CAMERA_ZOOM_OUT_FAST) )
	{
		camDist += ctx.mTransScale * 10;
	}

	if ( INPUTENGINE.getEventBoolValue(EV_CAMERA_RESET) )
	{
		reset(ctx);
	}

	if ( INPUTENGINE.isKeyDown(OIS::KC_RSHIFT) && INPUTENGINE.isKeyDownValueBounce(OIS::KC_SPACE) )
	{
		limitMinCamDist = !limitMinCamDist;
#ifdef USE_MYGUI
		if ( limitMinCamDist )
		{
			Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("limited camera zoom enabled"), "camera_go.png", 3000);
		} else
		{
			Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("limited camera zoom disabled"), "camera_go.png", 3000);
		}
#endif // USE_MYGUI
	}

	if ( limitMinCamDist && camDistMin > 0.0f )
	{
		camDist = std::max(camDistMin, camDist);
	}
	if ( limitMinCamDist && camDistMax > 0.0f )
	{
		camDist = std::min(camDist, camDistMax);
	}

	camDist = std::max(0.0f, camDist);

	Vector3 desiredPosition = camLookAt + camDist * 0.5f * Vector3(
			  sin(targetDirection.valueRadians() + rotXWithSwivel.valueRadians()) * cos(targetPitch.valueRadians() + rotYWithSwivel.valueRadians())
			, sin(targetPitch.valueRadians()     + rotYWithSwivel.valueRadians())
			, cos(targetDirection.valueRadians() + rotXWithSwivel.valueRadians()) * cos(targetPitch.valueRadians() + rotYWithSwivel.valueRadians())
			);

	if ( gEnv->terrainManager && gEnv->terrainManager->getHeightFinder() )
	{
		float h = gEnv->terrainManager->getHeightFinder()->getHeightAt(desiredPosition.x, desiredPosition.z) + 1.0f;

		desiredPosition.y = std::max(h, desiredPosition.y);
	}

	Vector3 precedingPosition = gEnv->mainCamera->getPosition(); 
	
	if ( ctx.mCurrTruck )
	{
		precedingPosition += ctx.mCurrTruck->nodes[0].Velocity * ctx.mCurrTruck->ttdt;
	}

	Vector3 camPosition = (1.0f / (camRatio + 1.0f)) * desiredPosition + (camRatio / (camRatio + 1.0f)) * precedingPosition;

	if ( gEnv->collisions && gEnv->collisions->forcecam )
	{
		gEnv->mainCamera->setPosition(gEnv->collisions->forcecampos);
		gEnv->collisions->forcecam = false;
	} else
	{
		gEnv->mainCamera->setPosition(camPosition);
	}

	gEnv->mainCamera->lookAt(camLookAt);
}

bool CameraBehaviorOrbit::mouseMoved(const CameraManager::CameraContext &ctx, const OIS::MouseEvent& _arg)
{
	const OIS::MouseState ms = _arg.state;

	if ( ms.buttonDown(OIS::MB_Right) )
	{
		camRotX += Degree( ms.X.rel * 0.13f);
		camRotY += Degree(-ms.Y.rel * 0.13f);
		camDist +=        -ms.Z.rel * 0.02f;
		return true;
	}

	return false;
}

void CameraBehaviorOrbit::reset(const CameraManager::CameraContext &ctx)
{
	camRotX = 0.0f;
	camRotY = 0.3f;
	gEnv->mainCamera->setFOVy(ctx.fovExternal);
}
