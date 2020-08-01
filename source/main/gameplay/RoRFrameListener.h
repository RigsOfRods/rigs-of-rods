/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2015-2020 Petr Ohlidal

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

#include "Application.h"

#include "SimData.h" // RoR::ActorSpawnRequest
#include "ActorManager.h"
#include "CameraManager.h" // CameraManager::CameraBehaviors
#include "CharacterFactory.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "OutGauge.h"
#include "GUI_SceneMouse.h"

namespace RoR {

/// UNDER REMOVAL
class SimController
{
public:
    void   RemoveActorByCollisionBox(std::string const & ev_src_instance_name, std::string const & box_name); //!< Scripting utility. TODO: Does anybody use it? ~ only_a_ptr, 08/2017

    void   UpdateSimulation(float dt);

private:

    void   UpdateInputEvents       (float dt);

};

} // namespace RoR
