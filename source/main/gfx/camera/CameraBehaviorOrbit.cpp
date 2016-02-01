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

#include <Ogre.h>

#include "Application.h"
#include "Beam.h"
#include "BeamFactory.h"
#include "Collisions.h"
#include "Console.h"
#include "GUIManager.h"
#include "IHeightFinder.h"
#include "InputEngine.h"
#include "Language.h"
#include "TerrainManager.h"

using namespace Ogre;
using namespace RoR;

CameraBehaviorOrbit::CameraBehaviorOrbit() :
	  camDist(5.0f)
	, camDistMax(0.0f)
	, camDistMin(0.0f)
	, camLookAt(Vector3::ZERO)
	, camRatio(11.0f)
	, camRotX(0.0f)
	, camRotXSwivel(0.0f)
	, camRotY(0.3f)
	, camRotYSwivel(0.0f)
	, limitCamMovement(true)
	, targetDirection(0.0f)
	, targetPitch(0.0f)
{
}

void CameraBehaviorOrbit::update(const CameraManager::CameraContext &ctx)
{
	if ( RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_LOOKBACK) )
	{
		if ( camRotX > Degree(0) )
		{
			camRotX = Degree(0);
		} else
		{
			camRotX = Degree(180);
		}
	}

	camRotX += (RoR::Application::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_RIGHT) - RoR::Application::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_LEFT)) * ctx.mRotScale;
	camRotY += (RoR::Application::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_UP)   - RoR::Application::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_DOWN))  * ctx.mRotScale;

	camRotY = std::max((Radian)Degree(-80), camRotY);
	camRotY = std::min(camRotY, (Radian)Degree(88));

	camRotXSwivel = (RoR::Application::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_RIGHT) - RoR::Application::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_LEFT)) * Degree(90);
	camRotYSwivel = (RoR::Application::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_UP)   - RoR::Application::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_DOWN))  * Degree(60);

	camRotYSwivel = std::max((Radian)Degree(-80) - camRotY, camRotYSwivel);
	camRotYSwivel = std::min(camRotYSwivel, (Radian)Degree(88) - camRotY);

	if ( RoR::Application::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_IN) && camDist > 1 )
	{
		camDist -= ctx.mTransScale;
	}
	if ( RoR::Application::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_IN_FAST) && camDist > 1 )
	{
		camDist -= ctx.mTransScale * 10;
	}
	if ( RoR::Application::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_OUT) )
	{
		camDist += ctx.mTransScale;
	}
	if ( RoR::Application::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_OUT_FAST) )
	{
		camDist += ctx.mTransScale * 10;
	}

	if ( RoR::Application::GetInputEngine()->getEventBoolValue(EV_CAMERA_RESET) )
	{
		reset(ctx);
	}

	if ( RoR::Application::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT) && RoR::Application::GetInputEngine()->isKeyDownValueBounce(OIS::KC_SPACE) )
	{
		limitCamMovement = !limitCamMovement;
#ifdef USE_MYGUI
		if ( limitCamMovement )
		{
			RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Limited camera movement enabled"), "camera_go.png", 3000);
			RoR::Application::GetGuiManager()->PushNotification("Notice:", _L("Limited camera movement enabled"));
		} else
		{
			RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Limited camera movement disabled"), "camera_go.png", 3000);
			RoR::Application::GetGuiManager()->PushNotification("Notice:", _L("Limited camera movement disabled"));
		}
#endif // USE_MYGUI
	}

	if ( limitCamMovement && camDistMin > 0.0f )
	{
		camDist = std::max(camDistMin, camDist);
	}
	if ( limitCamMovement && camDistMax > 0.0f )
	{
		camDist = std::min(camDist, camDistMax);
	}

	camDist = std::max(0.0f, camDist);

	Vector3 desiredPosition = camLookAt + camDist * 0.5f * Vector3(
			  sin(targetDirection.valueRadians() + (camRotX - camRotXSwivel).valueRadians()) * cos(targetPitch.valueRadians() + (camRotY - camRotYSwivel).valueRadians())
			, sin(targetPitch.valueRadians()     + (camRotY - camRotYSwivel).valueRadians())
			, cos(targetDirection.valueRadians() + (camRotX - camRotXSwivel).valueRadians()) * cos(targetPitch.valueRadians() + (camRotY - camRotYSwivel).valueRadians())
			);

	if ( limitCamMovement && gEnv->terrainManager && gEnv->terrainManager->getHeightFinder() )
	{
		float h = gEnv->terrainManager->getHeightFinder()->getHeightAt(desiredPosition.x, desiredPosition.z) + 1.0f;

		desiredPosition.y = std::max(h, desiredPosition.y);
	}

	Vector3 precedingPosition = gEnv->mainCamera->getPosition(); 
	
	if ( ctx.mCurrTruck )
	{
		if (BeamFactory::getSingleton().getThreadingMode() == THREAD_MULTI)
			precedingPosition += ctx.mCurrTruck->nodes[0].Velocity * ctx.mCurrTruck->oldframe_global_dt;
		else
			precedingPosition += ctx.mCurrTruck->nodes[0].Velocity * ctx.mCurrTruck->global_dt;
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
		float scale = RoR::Application::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.002f : 0.02f;
		camRotX += Degree( ms.X.rel * 0.13f);
		camRotY += Degree(-ms.Y.rel * 0.13f);
		camDist +=        -ms.Z.rel * scale;
		return true;
	}

	return false;
}

void CameraBehaviorOrbit::reset(const CameraManager::CameraContext &ctx)
{
	camRotX = 0.0f;
	camRotXSwivel = 0.0f;
	camRotY = 0.3f;
	camRotYSwivel = 0.0f;
	gEnv->mainCamera->setFOVy(ctx.fovExternal);
}
