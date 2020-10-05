/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal

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

#pragma once

#include "ForwardDeclarations.h"

// -----==== DEBUG ====----
#include "Application.h"
#include "CameraManager.h"

class CamProxy
{
public:
    // getters
    Ogre::Vector3 getDerivedPosition()
    {
        return RoR::App::GetCameraManager()->GetCameraNode()->_getDerivedPosition();
    }


    // setters
    void setPosition(Ogre::Vector3 pos)
    {
        RoR::App::GetCameraManager()->GetCameraNode()->setPosition(pos);
    }
    void setOrientation(const Ogre::Quaternion &q)
    {
        RoR::App::GetCameraManager()->GetCameraNode()->setOrientation(q);
    }
    void yaw(const Ogre::Radian& angle)
    {
        RoR::App::GetCameraManager()->GetCameraNode()->yaw(angle, Ogre::Node::TS_WORLD);
    }
    void pitch(const Ogre::Radian& angle)
    {
        RoR::App::GetCameraManager()->GetCameraNode()->pitch(angle);
    }
    void roll(const Ogre::Radian& angle)
    {
        RoR::App::GetCameraManager()->GetCameraNode()->roll(angle);
    }
    void lookAt(const Ogre::Vector3& targetPoint)
    {
        // ??
        RoR::App::GetCameraManager()->GetCameraNode()->lookAt(targetPoint, Ogre::Node::TS_WORLD);
    }
    void setDirection(const Ogre::Vector3& dir)
    {
        // ??
        RoR::App::GetCameraManager()->GetCameraNode()->setDirection(dir);
    }
};
// ----==== END DEBUG ====----

class GlobalEnvironment
{
public:

    GlobalEnvironment() :
         mainCamera(&m_camproxy)
        , sceneManager(0)
    {}

    CamProxy*       mainCamera;
    Ogre::SceneManager* sceneManager;

private:
    CamProxy m_camproxy;
};
