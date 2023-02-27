/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal

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

/// @author Petr Ohlidal
/// @date   10/2019
///   Based on SimUtils - TruckInfo by Moncef Ben Slimane, 12/2014

#pragma once

#include "ForwardDeclarations.h"

namespace RoR {
namespace GUI {

class SimActorStats
{
public:
    void SetVisible(bool vis) { m_is_visible = vis; }
    bool IsVisible() const { return m_is_visible; }

    void UpdateStats(float dt, ActorPtr actor); //!< Caution: touches live data, must be synced with sim. thread
    void Draw(RoR::GfxActor* actorx);

private:
    bool  m_is_visible          = false;
    float m_stat_health         = 0.f;
    int   m_stat_broken_beams   = 0;
    int   m_stat_deformed_beams = 0;
    float m_stat_beam_stress    = 0.f;
    float m_stat_mass_Kg        = 0.f;
    float m_stat_avg_deform     = 0.f;
    float m_stat_gcur_x         = 0.f;
    float m_stat_gcur_y         = 0.f;
    float m_stat_gcur_z         = 0.f;
    float m_stat_gmax_x         = 0.f;
    float m_stat_gmax_y         = 0.f;
    float m_stat_gmax_z         = 0.f;
};

} // namespace GUI
} // namespace RoR
