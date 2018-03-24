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

#include "CameraBehaviorFree.h"

#include "Application.h"
#include "GUIManager.h"
#include "InputEngine.h"

#include <Ogre.h>
#include <MyGUI_PointerManager.h>

using namespace Ogre;
using namespace RoR;

void CameraBehaviorFree::update(const CameraManager::CameraContext& ctx)
{
    Degree mRotX(0.0f);
    Degree mRotY(0.0f);
    Degree cct_rot_scale(ctx.cct_rot_scale * 10.0f * ctx.cct_dt);
    Vector3 mTrans(Vector3::ZERO);
    Real cct_trans_scale(ctx.cct_trans_scale * 10.0f * ctx.cct_dt);

    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) || RoR::App::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT))
    {
        cct_rot_scale *= 3.0f;
        cct_trans_scale *= 3.0f;
    }
    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LCONTROL))
    {
        cct_rot_scale *= 6.0f;
        cct_trans_scale *= 20.0f;
    }
    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU))
    {
        cct_rot_scale *= 0.1f;
        cct_trans_scale *= 0.1f;
    }

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_LEFT))
    {
        mTrans.x -= cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_RIGHT))
    {
        mTrans.x += cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_FORWARD))
    {
        mTrans.z -= cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_BACKWARDS))
    {
        mTrans.z += cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_UP))
    {
        mTrans.y += cct_trans_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_DOWN))
    {
        mTrans.y -= cct_trans_scale;
    }

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_RIGHT))
    {
        mRotX -= cct_rot_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_LEFT))
    {
        mRotX += cct_rot_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_ROT_UP))
    {
        mRotY += cct_rot_scale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_ROT_DOWN))
    {
        mRotY -= cct_rot_scale;
    }

    gEnv->mainCamera->yaw(mRotX);
    gEnv->mainCamera->pitch(mRotY);

    Vector3 camPosition = gEnv->mainCamera->getPosition() + gEnv->mainCamera->getOrientation() * mTrans.normalisedCopy() * cct_trans_scale;

    gEnv->mainCamera->setPosition(camPosition);
}

bool CameraBehaviorFree::mouseMoved(const CameraManager::CameraContext& ctx, const OIS::MouseEvent& _arg)
{
    const OIS::MouseState ms = _arg.state;

    gEnv->mainCamera->yaw(Degree(-ms.X.rel * 0.13f));
    gEnv->mainCamera->pitch(Degree(-ms.Y.rel * 0.13f));

    App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::HIDDEN);

    return true;
}

