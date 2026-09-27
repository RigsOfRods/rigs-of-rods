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

void RaceSystem::StartRaceTimer(int raceID, int actorID)
{
    if (actorID > ACTORINSTANCEID_INVALID)
    {
        m_race_start_times[actorID] = App::GetGameContext()->GetActorManager()->GetTotalTime();
        m_races_in_progress[actorID] = true;
    }
    else
    {
        m_race_start_time = App::GetGameContext()->GetActorManager()->GetTotalTime();
    }
    m_race_time_diff = 0.0f;
    m_race_id = raceID;
}

void RaceSystem::StopRaceTimer(int actorID)
{
    float time;

    if (actorID > ACTORINSTANCEID_INVALID)
    {
        if (m_races_in_progress[actorID])
        {
            time = App::GetGameContext()->GetActorManager()->GetTotalTime() - m_race_start_times[actorID];
            m_race_lastlap_times[actorID] = time;
        }
    }
    else
    {
        m_race_start_time = 0.0f;
    }
    m_race_id = -1;
}

void RaceSystem::SetRaceTimer(int actorID, float time, bool raceIsInProgress)
{
    if (actorID > ACTORINSTANCEID_INVALID)
    {
        LOG("SetRaceTimer() for actorID: " + TOSTRING(actorID) + " Start time was: " + TOSTRING(m_race_start_times[actorID]) + " New start time: " + TOSTRING(time) + ".");
        m_race_start_times[actorID] = (double)time;
        m_races_in_progress[actorID] = raceIsInProgress;
    }
    else
    {
        m_race_start_time = time;
        m_race_in_progress = raceIsInProgress;
    }
}

float RaceSystem::GetRaceTime() const
{
    return App::GetGameContext()->GetActorManager()->GetTotalTime() - m_race_start_time;
}

void RaceSystem::ResetRaceUI()
{
    this->StopRaceTimer(ACTORINSTANCEID_INVALID);
    this->UpdateDirectionArrow(nullptr, Ogre::Vector3::ZERO); // hide arrow
}
