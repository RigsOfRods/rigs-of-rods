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
#include "InputEngine.h"
#include "Language.h"
#include "TerrainManager.h"

using namespace Ogre;
using namespace RoR;



void CameraBehaviorOrbit::update( CameraManager::CameraContext& ctx)
{
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_CAMERA_LOOKBACK))
    {
        if (ctx.camRotX > Degree(0))
        {
            ctx.camRotX = Degree(0);
        }
        else
        {
            ctx.camRotX = Degree(180);
        }
    }

    ctx.camRotX += (RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_RIGHT) - RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_LEFT)) * ctx.cct_rot_scale;
    ctx.camRotY += (RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_UP) - RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_ROTATE_DOWN)) * ctx.cct_rot_scale;

    ctx.camRotY = std::max((Radian)Degree(-80), ctx.camRotY);
    ctx.camRotY = std::min(ctx.camRotY, (Radian)Degree(88));

    ctx.camRotXSwivel = (RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_RIGHT) - RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_LEFT)) * Degree(90);
    ctx.camRotYSwivel = (RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_UP) - RoR::App::GetInputEngine()->getEventValue(EV_CAMERA_SWIVEL_DOWN)) * Degree(60);

    ctx.camRotYSwivel = std::max((Radian)Degree(-80) - ctx.camRotY, ctx.camRotYSwivel);
    ctx.camRotYSwivel = std::min(ctx.camRotYSwivel, (Radian)Degree(88) - ctx.camRotY);

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_IN) && ctx.camDist > 1)
    {
        ctx.camDist -= ctx.cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_IN_FAST) && ctx.camDist > 1)
    {
        ctx.camDist -= ctx.cct_trans_scale * 10;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_OUT))
    {
        ctx.camDist += ctx.cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_ZOOM_OUT_FAST))
    {
        ctx.camDist += ctx.cct_trans_scale * 10;
    }

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_RESET))
    {
        reset(ctx);
    }

    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT) && RoR::App::GetInputEngine()->isKeyDownValueBounce(OIS::KC_SPACE))
    {
        ctx.limitCamMovement = !ctx.limitCamMovement;
        if (ctx.limitCamMovement)
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

    if (ctx.limitCamMovement && ctx.camDistMin > 0.0f)
    {
        ctx.camDist = std::max(ctx.camDistMin, ctx.camDist);
    }
    if (ctx.limitCamMovement && ctx.camDistMax > 0.0f)
    {
        ctx.camDist = std::min(ctx.camDist, ctx.camDistMax);
    }

    ctx.camDist = std::max(0.0f, ctx.camDist);

    Vector3 desiredPosition = ctx.camLookAt + ctx.camDist * 0.5f * Vector3(
        sin(ctx.targetDirection.valueRadians() + (ctx.camRotX - ctx.camRotXSwivel).valueRadians()) * cos(ctx.targetPitch.valueRadians() + (ctx.camRotY - ctx.camRotYSwivel).valueRadians())
        , sin(ctx.targetPitch.valueRadians() + (ctx.camRotY - ctx.camRotYSwivel).valueRadians())
        , cos(ctx.targetDirection.valueRadians() + (ctx.camRotX - ctx.camRotXSwivel).valueRadians()) * cos(ctx.targetPitch.valueRadians() + (ctx.camRotY - ctx.camRotYSwivel).valueRadians())
    );

    if (ctx.limitCamMovement && App::GetSimTerrain())
    {
        float h = App::GetSimTerrain()->GetHeightAt(desiredPosition.x, desiredPosition.z) + 1.0f;

        desiredPosition.y = std::max(h, desiredPosition.y);
    }

    if (ctx.camLookAtLast == Vector3::ZERO)
    {
        ctx.camLookAtLast = ctx.camLookAt;
    }
    if (ctx.camLookAtSmooth == Vector3::ZERO)
    {
        ctx.camLookAtSmooth = ctx.camLookAt;
    }
    if (ctx.camLookAtSmoothLast == Vector3::ZERO)
    {
        ctx.camLookAtSmoothLast = ctx.camLookAtSmooth;
    }

    Vector3 camDisplacement = ctx.camLookAt - ctx.camLookAtLast;
    Vector3 precedingLookAt = ctx.camLookAtSmoothLast + camDisplacement;
    Vector3 precedingPosition = gEnv->mainCamera->getPosition() + camDisplacement;

    Vector3 camPosition = (1.0f / (ctx.camRatio + 1.0f)) * desiredPosition + (ctx.camRatio / (ctx.camRatio + 1.0f)) * precedingPosition;

    if (gEnv->collisions && gEnv->collisions->forcecam)
    {
        gEnv->mainCamera->setPosition(gEnv->collisions->forcecampos);
        gEnv->collisions->forcecam = false;
    }
    else
    {
        if (ctx.cct_player_actor && ctx.cct_player_actor->ar_replay_mode && camDisplacement != Vector3::ZERO)
            gEnv->mainCamera->setPosition(desiredPosition);
        else
            gEnv->mainCamera->setPosition(camPosition);
    }

    ctx.camLookAtSmooth = (1.0f / (ctx.camRatio + 1.0f)) * ctx.camLookAt + (ctx.camRatio / (ctx.camRatio + 1.0f)) * precedingLookAt;

    ctx.camLookAtLast = ctx.camLookAt;
    ctx.camLookAtSmoothLast = ctx.camLookAtSmooth;
    gEnv->mainCamera->lookAt(ctx.camLookAtSmooth);
}

bool CameraBehaviorOrbit::mouseMoved( CameraManager::CameraContext& ctx, const OIS::MouseEvent& _arg)
{
    const OIS::MouseState ms = _arg.state;

    if (ms.buttonDown(OIS::MB_Right))
    {
        float scale = RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.002f : 0.02f;
        ctx.camRotX += Degree(ms.X.rel * 0.13f);
        ctx.camRotY += Degree(-ms.Y.rel * 0.13f);
        ctx.camDist += -ms.Z.rel * scale;
        return true;
    }

    return false;
}

void CameraBehaviorOrbit::reset( CameraManager::CameraContext& ctx)
{
    ctx.camRotX = 0.0f;
    ctx.camRotXSwivel = 0.0f;
    ctx.camRotY = 0.3f;
    ctx.camRotYSwivel = 0.0f;
    ctx.camLookAtLast = Vector3::ZERO;
    ctx.camLookAtSmooth = Vector3::ZERO;
    ctx.camLookAtSmoothLast = Vector3::ZERO;
    gEnv->mainCamera->setFOVy(ctx.cct_fov_exterior);
}

void CameraBehaviorOrbit::notifyContextChange( CameraManager::CameraContext& ctx)
{
    ctx.camLookAtLast = Vector3::ZERO;
}
