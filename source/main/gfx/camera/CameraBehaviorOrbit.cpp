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

#include "CameraBehaviorOrbit.h"

#include <Ogre.h>

#include "Application.h"
#include "Beam.h"
#include "Collisions.h"
#include "GUI_GameConsole.h"
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
    , camLookAtLast(Vector3::ZERO)
    , camLookAtSmooth(Vector3::ZERO)
    , camLookAtSmoothLast(Vector3::ZERO)
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

void CameraBehaviorOrbit::update(const CameraManager::CameraContext& ctx)
{
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_LOOKBACK))
    {
        if (camRotX > Degree(0))
        {
            camRotX = Degree(0);
        }
        else
        {
            camRotX = Degree(180);
        }
    }

    camRotX += (RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_RIGHT) - RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_LEFT)) * ctx.mRotScale;
    camRotY += (RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_UP) - RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_DOWN)) * ctx.mRotScale;

    camRotY = std::max((Radian)Degree(-80), camRotY);
    camRotY = std::min(camRotY, (Radian)Degree(88));

    camRotXSwivel = (RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_RIGHT) - RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_LEFT)) * Degree(90);
    camRotYSwivel = (RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_UP) - RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_DOWN)) * Degree(60);

    camRotYSwivel = std::max((Radian)Degree(-80) - camRotY, camRotYSwivel);
    camRotYSwivel = std::min(camRotYSwivel, (Radian)Degree(88) - camRotY);

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_IN) && camDist > 1)
    {
        camDist -= ctx.mTransScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_IN_FAST) && camDist > 1)
    {
        camDist -= ctx.mTransScale * 10;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_OUT))
    {
        camDist += ctx.mTransScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_OUT_FAST))
    {
        camDist += ctx.mTransScale * 10;
    }

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_RESET))
    {
        reset(ctx);
    }

    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT) && RoR::App::GetInputEngine()->isKeyDownValueBounce(OIS::KC_SPACE))
    {
        limitCamMovement = !limitCamMovement;
        if (limitCamMovement)
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Limited camera movement enabled"), "camera_go.png", 3000);
            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Limited camera movement enabled"));
        }
        else
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Limited camera movement disabled"), "camera_go.png", 3000);
            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Limited camera movement disabled"));
        }
    }

    if (limitCamMovement && camDistMin > 0.0f)
    {
        camDist = std::max(camDistMin, camDist);
    }
    if (limitCamMovement && camDistMax > 0.0f)
    {
        camDist = std::min(camDist, camDistMax);
    }

    camDist = std::max(0.0f, camDist);

    Vector3 desiredPosition = camLookAt + camDist * 0.5f * Vector3(
        sin(targetDirection.valueRadians() + (camRotX - camRotXSwivel).valueRadians()) * cos(targetPitch.valueRadians() + (camRotY - camRotYSwivel).valueRadians())
        , sin(targetPitch.valueRadians() + (camRotY - camRotYSwivel).valueRadians())
        , cos(targetDirection.valueRadians() + (camRotX - camRotXSwivel).valueRadians()) * cos(targetPitch.valueRadians() + (camRotY - camRotYSwivel).valueRadians())
    );

    if (limitCamMovement && gEnv->terrainManager && gEnv->terrainManager->getHeightFinder())
    {
        float h = gEnv->terrainManager->getHeightFinder()->getHeightAt(desiredPosition.x, desiredPosition.z) + 1.0f;

        desiredPosition.y = std::max(h, desiredPosition.y);
    }

    if (camLookAtLast == Vector3::ZERO)
    {
        camLookAtLast = camLookAt;
    }
    if (camLookAtSmooth == Vector3::ZERO)
    {
        camLookAtSmooth = camLookAt;
    }
    if (camLookAtSmoothLast == Vector3::ZERO)
    {
        camLookAtSmoothLast = camLookAtSmooth;
    }

    Vector3 camDisplacement = camLookAt - camLookAtLast;
    Vector3 precedingLookAt = camLookAtSmoothLast + camDisplacement;
    Vector3 precedingPosition = gEnv->mainCamera->getPosition() + camDisplacement;

    Vector3 camPosition = (1.0f / (camRatio + 1.0f)) * desiredPosition + (camRatio / (camRatio + 1.0f)) * precedingPosition;

    if (gEnv->collisions && gEnv->collisions->forcecam)
    {
        gEnv->mainCamera->setPosition(gEnv->collisions->forcecampos);
        gEnv->collisions->forcecam = false;
    }
    else
    {
        if (ctx.mCurrTruck && ctx.mCurrTruck->replaymode && camDisplacement != Vector3::ZERO)
            gEnv->mainCamera->setPosition(desiredPosition);
        else
            gEnv->mainCamera->setPosition(camPosition);
    }

    camLookAtSmooth = (1.0f / (camRatio + 1.0f)) * camLookAt + (camRatio / (camRatio + 1.0f)) * precedingLookAt;

    camLookAtLast = camLookAt;
    camLookAtSmoothLast = camLookAtSmooth;
    gEnv->mainCamera->lookAt(camLookAtSmooth);
}

bool CameraBehaviorOrbit::mouseMoved(const CameraManager::CameraContext& ctx, const OIS::MouseEvent& _arg)
{
    const OIS::MouseState ms = _arg.state;

    if (ms.buttonDown(OIS::MB_Right))
    {
        float scale = RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.002f : 0.02f;
        camRotX += Degree(ms.X.rel * 0.13f);
        camRotY += Degree(-ms.Y.rel * 0.13f);
        camDist += -ms.Z.rel * scale;
        return true;
    }

    return false;
}

void CameraBehaviorOrbit::reset(const CameraManager::CameraContext& ctx)
{
    camRotX = 0.0f;
    camRotXSwivel = 0.0f;
    camRotY = 0.3f;
    camRotYSwivel = 0.0f;
    camLookAtLast = Vector3::ZERO;
    camLookAtSmooth = Vector3::ZERO;
    camLookAtSmoothLast = Vector3::ZERO;
    gEnv->mainCamera->setFOVy(ctx.fovExternal);
}

void CameraBehaviorOrbit::notifyContextChange(const CameraManager::CameraContext& ctx)
{
    camLookAtLast = Vector3::ZERO;
}
