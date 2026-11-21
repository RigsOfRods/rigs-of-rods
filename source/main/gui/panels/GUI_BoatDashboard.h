/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2024 Petr Ohlidal

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


/// @file  Boat dashboard drawn procedurally with DearIMGUI (no textures, no OGRE overlays).
///        Speed, one throttle lever per screwprop, heading + rudder (with 'center rudder' button),
///        echo sounder with history, heel/trim and light switches.
///        Selected like any boat dashboard in the Loader ('ImGui - Boat' = `imgui_boat.dashboard`,
///        a '.dashboard' file without layout); boats with their own dashboard keep it.

#pragma once

#include "ForwardDeclarations.h"

#include <vector>

namespace RoR {
namespace GUI {

class BoatDashboard
{
public:
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool v) { m_visible = v; }
    void Draw(GfxActor* gfx_actor);

    static constexpr const char* DASHBOARD_FILE = "imgui_boat.dashboard"; //!< Its entry in the Loader (resources/dashboards)
    static bool IsUsedBy(const ActorPtr& actor); //!< Boat without own dashboard, and this one is the default boat dashboard

private:
    bool               m_visible = false;
    bool               m_reset_pos = true;
    bool               m_transparent = false;
    float              m_scale_mul = 0.75f;
    float              m_speed_max = 20.f;     //!< Speed dial range in knots, grows with the boat
    float              m_depth = -1.f;         //!< Last echo sounder reading [m], negative = no reading
    float              m_depth_timer = 0.f;
    std::vector<float> m_depth_history;        //!< Echo sounder readings, oldest first
    int                m_actor_id = -1;        //!< Instance ID the history belongs to
};

} // namespace GUI
} // namespace RoR
