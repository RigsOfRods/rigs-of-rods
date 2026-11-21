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

/// @file  Airplane dashboard drawn procedurally with DearIMGUI (no textures, no OGRE overlays).
///        Movable by dragging the background; gear button: choose instruments, scale, reset position.
///        Layout is in "design units", scaled by the user-adjustable factor.

#pragma once

#include "ForwardDeclarations.h"

namespace RoR {
namespace GUI {

class AirDashboard
{
public:
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool v) { m_visible = v; }
    void Draw(GfxActor* gfx_actor);

private:
    bool  m_visible = false;
    bool  m_reset_pos = true;
    float m_scale_mul = 1.f;
    float m_hold_timer = 0.f;
    // 0..5: airspeed, altitude, vertical speed, attitude, AOA, heading; 6..8: engine RPM, pitch, torque
    bool  m_show[9] = { true, true, true, true, true, true, true, true, true };
    bool  m_show_temp = true;
    bool  m_transparent = false;
    int   m_edit_field = -1;     //!< autopilot value being typed in: 0 HDG, 1 ALT, 2 VS, 3 IAS; -1 none
    bool  m_edit_focus = false;
    bool  m_edit_was_active = false; //!< the text field really got the keyboard focus
    char  m_edit_buf[12] = {};
};

} // namespace GUI
} // namespace RoR
