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

#include <Ogre.h>

#include "Application.h"
#include "GUIManager.h"
#include "GUI_GameConsole.h"
#include "InputEngine.h"
#include "Language.h"

using namespace Ogre;
using namespace RoR;

void CameraBehaviorFree::update(const CameraManager::CameraContext& ctx)
{
    Degree mRotX(0.0f);
    Degree mRotY(0.0f);
    Degree mRotScale(ctx.mRotScale * 10.0f * ctx.mDt);
    Vector3 mTrans(Vector3::ZERO);
    Real mTransScale(ctx.mTransScale * 10.0f * ctx.mDt);

    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) || RoR::App::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT))
    {
        mRotScale *= 3.0f;
        mTransScale *= 3.0f;
    }
    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LCONTROL))
    {
        mRotScale *= 6.0f;
        mTransScale *= 20.0f;
    }
    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU))
    {
        mRotScale *= 0.1f;
        mTransScale *= 0.1f;
    }

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_LEFT))
    {
        mTrans.x -= mTransScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_RIGHT))
    {
        mTrans.x += mTransScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_FORWARD))
    {
        mTrans.z -= mTransScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_BACKWARDS))
    {
        mTrans.z += mTransScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_UP))
    {
        mTrans.y += mTransScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CAMERA_DOWN))
    {
        mTrans.y -= mTransScale;
    }

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_RIGHT))
    {
        mRotX -= mRotScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_LEFT))
    {
        mRotX += mRotScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_ROT_UP))
    {
        mRotY += mRotScale;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_ROT_DOWN))
    {
        mRotY -= mRotScale;
    }

    gEnv->mainCamera->yaw(mRotX);
    gEnv->mainCamera->pitch(mRotY);

    Vector3 camPosition = gEnv->mainCamera->getPosition() + gEnv->mainCamera->getOrientation() * mTrans.normalisedCopy() * mTransScale;

    gEnv->mainCamera->setPosition(camPosition);
}

bool CameraBehaviorFree::mouseMoved(const CameraManager::CameraContext& ctx, const OIS::MouseEvent& _arg)
{
    const OIS::MouseState ms = _arg.state;

    gEnv->mainCamera->yaw(Degree(-ms.X.rel * 0.13f));
    gEnv->mainCamera->pitch(Degree(-ms.Y.rel * 0.13f));

    MyGUI::PointerManager::getInstance().setVisible(false);

    return true;
}

void CameraBehaviorFree::activate(const CameraManager::CameraContext& ctx, bool reset /* = true */)
{
    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Free camera"), "camera_go.png", 3000);
    RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Free camera"));
}
