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

#include "TyrePressure.h"

#include "Actor.h"
#include "InputEngine.h"
#include "SoundScriptManager.h"

#include <Ogre.h>

using namespace RoR;

void TyrePressure::UpdateInputEvents(float dt)
{
    if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_PRESSURE_LESS))
    {
        float change = m_ref_tyre_pressure * (1.0f - pow(2.0f, dt / 2.0f));
        change = Ogre::Math::Clamp(change, -dt * 10.0f, -dt * 1.0f);
        if (m_pressure_pressed = this->ModifyTyrePressure(change))
        {
            SOUND_START(m_actor, SS_TRIG_AIR);
        }
    }
    else if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_PRESSURE_MORE))
    {
        float change = m_ref_tyre_pressure * (pow(2.0f, dt / 2.0f) - 1.0f);
        change = Ogre::Math::Clamp(change, +dt * 1.0f, +dt * 10.0f);
        if (m_pressure_pressed = this->ModifyTyrePressure(change))
        {
            SOUND_START(m_actor, SS_TRIG_AIR);
        }
    }
    else if (m_pressure_pressed)
    {
        SOUND_STOP(m_actor, SS_TRIG_AIR);
        m_pressure_pressed = false;
        m_pressure_pressed_timer = 1.5f;
    }
    else if (m_pressure_pressed_timer > 0.0f)
    {
        m_pressure_pressed_timer -= dt;
    }
}

bool TyrePressure::ModifyTyrePressure(float v)
{
    float newpressure = Ogre::Math::Clamp(m_ref_tyre_pressure + v, 0.0f, 100.0f);
    if (newpressure == m_ref_tyre_pressure)
        return false;

    for (int beam_id: m_pressure_beams)
    {
        m_actor->ar_beams[beam_id].k = 10000 + newpressure * 10000;
    }
    m_ref_tyre_pressure = newpressure;
    return true;
}
