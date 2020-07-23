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

/// @file Wheel 'pressure adjustment' logic (only for 'wheels2')
///       Originally placed in RoRFrameListener.h|cpp and Beam.h|cpp

#include "Application.h"

#include <vector>

namespace RoR {

/// Land vehicle feat
/// Simulates tyre pressurization by adjusting springness of in-wheel beams.
class TyrePressure
{
public:
    TyrePressure(Actor* a): m_actor(a) {}

    void                AddBeam(int beam_id) { m_pressure_beams.push_back(beam_id); }
    bool                IsEnabled() const { return m_pressure_beams.size() != 0; }
    void                UpdateInputEvents(float dt);
    bool                ModifyTyrePressure(float v);
    float               GetCurPressure() const { return m_ref_tyre_pressure; }
    bool                IsPressurizing() const { return m_pressure_pressed; }

private:
    Actor*              m_actor;
    std::vector<int>    m_pressure_beams;
    bool                m_pressure_pressed = false;
    float               m_pressure_pressed_timer = 0.f;
    float               m_ref_tyre_pressure = 50.f;
};

} // namespace RoR
