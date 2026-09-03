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
/// @brief Counterpart to Neorej16's race system script

#include <Ogre.h>

namespace RoR {

class RaceSystem
{
public:
    void                StartRaceTimer(int raceID, int actorID);
    void                StopRaceTimer(int actorID);
    void                SetRaceTimer(int actorID, float time, bool raceIsInProgress);
    bool                IsRaceInProgress() const { return m_race_id != -1; }
    int                 GetRaceId() const { return m_race_id; }
    float               GetRaceTime() const;
    void                ResetRaceUI();

    void                SetRaceTimeDiff(float diff) { m_race_time_diff = diff; };
    float               GetRaceTimeDiff() const { return m_race_time_diff; };

    void                SetRaceBestTime(float time) { m_race_best_time = time; };
    float               GetRaceBestTime() const { return m_race_best_time; };    

    void                UpdateDirectionArrow(char* text, Ogre::Vector3 position);
    Ogre::Vector3       GetDirArrowTarget() { return m_dir_arrow_target; }
    std::string const&  GetDirArrowText() const { return m_dir_arrow_text; }
    bool                IsDirArrowVisible() const { return m_dir_arrow_visible; }

private:
    bool                m_dir_arrow_visible = false;
    std::string         m_dir_arrow_text;
    Ogre::Vector3       m_dir_arrow_target = Ogre::Vector3::ZERO;

    int                 m_race_id = -1;
    float               m_race_time_diff = 0.f;
    float               m_race_best_time = 0.f;
    float               m_race_start_time = 0.f;
    bool                m_race_in_progress = false;
    std::map<int, float> m_race_time_diffs;
    std::map<int, float> m_race_best_times;
    std::map<int, float> m_race_start_times;
    std::map<int, float> m_race_lastlap_times;
    std::map<int, bool>  m_races_in_progress;
};

} // namespace RoR