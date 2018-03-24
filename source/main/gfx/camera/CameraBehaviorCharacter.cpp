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

#include "CameraBehaviorCharacter.h"

#include "Application.h"
#include "Character.h"
#include "GUIManager.h"
#include "mygui/BaseLayout.h"

using namespace Ogre;

static const Ogre::Vector3 OFFSET_1ST_PERSON(0.0f, 1.82f, 0.0f);
static const Ogre::Vector3 OFFSET_3RD_PERSON(0.0f, 1.1f, 0.0f);

CameraBehaviorCharacter::CameraBehaviorCharacter() :
    CameraBehaviorOrbit()
    , m_is_3rd_person(true)
{
}

void CameraBehaviorCharacter::update(const CameraManager::CameraContext& ctx)
{
    if (!gEnv->player)
        return;
    targetDirection = -gEnv->player->getRotation() - Radian(Math::HALF_PI);
    Ogre::Vector3 offset = (!m_is_3rd_person) ? OFFSET_1ST_PERSON : OFFSET_3RD_PERSON;
    camLookAt = gEnv->player->getPosition() + offset;

    CameraBehaviorOrbit::update(ctx);
}

bool CameraBehaviorCharacter::mouseMoved(const CameraManager::CameraContext& ctx, const OIS::MouseEvent& _arg)
{
    if (!gEnv->player)
        return false;
    if (!m_is_3rd_person)
    {
        const OIS::MouseState ms = _arg.state;
        Radian angle = gEnv->player->getRotation();

        camRotY += Degree(ms.Y.rel * 0.13f);
        angle += Degree(ms.X.rel * 0.13f);

        camRotY = Radian(std::min(+Math::HALF_PI * 0.65f, camRotY.valueRadians()));
        camRotY = Radian(std::max(camRotY.valueRadians(), -Math::HALF_PI * 0.9f));

        gEnv->player->setRotation(angle);

        RoR::App::GetGuiManager()->SetMouseCursorVisibility(RoR::GUIManager::MouseCursorVisibility::HIDDEN);

        return true;
    }

    return CameraBehaviorOrbit::mouseMoved(ctx, _arg);
}

void CameraBehaviorCharacter::reset(const CameraManager::CameraContext& ctx)
{
    CameraBehaviorOrbit::reset(ctx);

    // Vars from CameraBehaviorOrbit
    if (!m_is_3rd_person)
    {
        camRotY = 0.1f;
        camDist = 0.1f;
        camRatio = 0.0f;
    }
    else
    {
        camRotY = 0.3f;
        camDist = 5.0f;
        camRatio = 11.0f;
    }
}

bool CameraBehaviorCharacter::switchBehavior(const CameraManager::CameraContext& ctx)
{
    if (m_is_3rd_person)
    {
        m_is_3rd_person = false;
        this->reset(ctx);
        return false;
    }
    else // first person
    {
        m_is_3rd_person = true;
        return true;
    }
}
