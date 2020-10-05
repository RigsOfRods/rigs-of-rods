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

/// @file Actor feat - interactive recovery and repair mode
///       Originally placed in RoRFrameListener.h|cpp

#include "Application.h"

#include <vector>

namespace RoR {

/// Actor feat - interactive recovery and repair mode, operates on player vehicle
///              Aka 'advanced repair' or 'interactive reset'
class RecoveryMode
{
public:
    void                UpdateInputEvents(float dt);

private:
    bool                m_advanced_vehicle_repair = false;
    float               m_advanced_vehicle_repair_timer = 0.f;
};

} // namespace RoR
