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

/// Counterpart to Neorej16's race system script

#include "RaceSystem.h"

#include "AppContext.h"
#include "GameContext.h"

using namespace RoR;

void RaceSystem::UpdateDirectionArrow(char* text, Ogre::Vector3 position)
{
    if (text == nullptr)
    {
        m_dir_arrow_visible = false;
        m_dir_arrow_target = Ogre::Vector3::ZERO;
    }
    else
    {
        m_dir_arrow_visible = true;
        m_dir_arrow_text = text;
        m_dir_arrow_target = position;
    }
}

void RaceSystem::StartRaceTimer(int id)
{
    m_race_start_time = App::GetGameContext()->GetActorManager()->GetTotalTime();
    m_race_time_diff = 0.0f;
    m_race_id = id;
}

void RaceSystem::StopRaceTimer()
{
    m_race_start_time = 0.0f;
    m_race_id = -1;
}

float RaceSystem::GetRaceTime() const
{
    return App::GetGameContext()->GetActorManager()->GetTotalTime() - m_race_start_time;
}

void RaceSystem::ResetRaceUI()
{
    this->StopRaceTimer();
    this->UpdateDirectionArrow(nullptr, Ogre::Vector3::ZERO); // hide arrow
}
