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

/// @file
/// @brief Actor feat - interactive recovery and repair mode

#include "Application.h"

#include <vector>

namespace RoR {

/// @addtogroup Gameplay
/// @{

/// Interactive recovery and repair mode, operates on player vehicle
/// Formerly 'advanced repair' or 'interactive reset'
class RepairMode
{
public:
    void                UpdateInputEvents(float dt);
    bool                IsLiveRepairActive() const { return m_live_repair_active; }
    bool                IsQuickRepairActive() const { return m_quick_repair_active; }
    float               GetLiveRepairTimer() const { return m_live_repair_timer; }

private:
    bool                m_quick_repair_active = false; // Player is holding EV_COMMON_REPAIR_TRUCK
    bool                m_live_repair_active = false; // Player pressed EV_COMMON_LIVE_REPAIR_MODE or held QuickRepair for 'sim_live_repair_interval' seconds.
    float               m_live_repair_timer = 0.f;
};

/// @} // addtogroup Gameplay

} // namespace RoR
