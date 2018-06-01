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

#pragma once

#include "RoRPrerequisites.h"

#include "CameraBehaviorOrbit.h"

class CameraBehaviorCharacter : public CameraBehaviorOrbit
{
public:

    CameraBehaviorCharacter();

    void update(const CameraManager::CameraContext& ctx);

    bool mouseMoved(const CameraManager::CameraContext& ctx, const OIS::MouseEvent& _arg);

    void reset(const CameraManager::CameraContext& ctx);

    bool switchBehavior(const CameraManager::CameraContext& ctx);

protected:

    bool m_is_3rd_person;
};
