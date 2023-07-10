/*
    This source file is part of Rigs of Rods
    Copyright 2023 Petr Ohlidal

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

/// @author Petr Ohlidal; Derived from VehicleButtonsUI by tritonas00
/// @date   07/2023

#pragma once

#include "ForwardDeclarations.h"

#include <vector>
#include <Ogre.h>

namespace RoR {
namespace GUI {

/// Common base for various mini-panels in the scene.
class SceneLabels
{
public:
    void Update();
    void DrawInstance(Ogre::Vector3 scene_pos, float cam_dist, const ActorPtr& actor);
    
    bool GetHornButtonState() { return m_horn; }
    std::vector<int> GetCommandEventID() { return m_id; }

private:
    ActorPtr FindHoveredActor();
    ActorPtr m_hovered_actor;
    
    bool m_horn = false;
    std::vector<int> m_id;
    Ogre::Timer m_timer;
    bool m_init = false;

    // Icon cache
    void CacheIcons();
    bool m_icons_cached = false;
public:
    Ogre::TexturePtr rc_headlight_icon;
    Ogre::TexturePtr rc_left_blinker_icon;
    Ogre::TexturePtr rc_right_blinker_icon;
    Ogre::TexturePtr rc_warning_light_icon;
    Ogre::TexturePtr rc_horn_icon;
    Ogre::TexturePtr rc_mirror_icon;
    Ogre::TexturePtr rc_repair_icon;
    Ogre::TexturePtr rc_parking_brake_icon;
    Ogre::TexturePtr rc_traction_control_icon;
    Ogre::TexturePtr rc_abs_icon;
    Ogre::TexturePtr rc_physics_icon;
    Ogre::TexturePtr rc_actor_physics_icon;
    Ogre::TexturePtr rc_a_icon;
    Ogre::TexturePtr rc_w_icon;
    Ogre::TexturePtr rc_m_icon;
    Ogre::TexturePtr rc_g_icon;
    Ogre::TexturePtr rc_particle_icon;
    Ogre::TexturePtr rc_shift_icon;
    Ogre::TexturePtr rc_engine_icon;
    Ogre::TexturePtr rc_beacons_icon;
    Ogre::TexturePtr rc_camera_icon;
    Ogre::TexturePtr rc_lock_icon;
    Ogre::TexturePtr rc_secure_icon;
    Ogre::TexturePtr rc_cruise_control_icon;
    // Mouse hints
    Ogre::TexturePtr rc_left_mouse_button;
    Ogre::TexturePtr rc_middle_mouse_button;
    Ogre::TexturePtr rc_middle_mouse_scroll_button;
    Ogre::TexturePtr rc_right_mouse_button;
};

} // namespace GUI
} // namespace RoR