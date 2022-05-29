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

/// @author tritonas00
/// @date   04/2022

#pragma once

#include "ForwardDeclarations.h"

#include <vector>

namespace RoR {
namespace GUI {

class VehicleButtons
{
public:
    void Draw(RoR::GfxActor* actorx);
    bool GetHornButtonState() { return m_horn; }
    std::vector<int> GetCommandEventID() { return m_id; }

private:
    void DrawHeadLightButton(RoR::GfxActor* actorx);
    void DrawLeftBlinkerButton(RoR::GfxActor* actorx);
    void DrawRightBlinkerButton(RoR::GfxActor* actorx);
    void DrawWarnBlinkerButton(RoR::GfxActor* actorx);
    void DrawHornButton(RoR::GfxActor* actorx);
    void DrawMirrorButton(RoR::GfxActor* actorx);
    void DrawRepairButton(RoR::GfxActor* actorx);
    void DrawParkingBrakeButton(RoR::GfxActor* actorx);
    void DrawTractionControlButton(RoR::GfxActor* actorx);
    void DrawAbsButton(RoR::GfxActor* actorx);
    void DrawPhysicsButton();
    void DrawActorPhysicsButton(RoR::GfxActor* actorx);
    void DrawAxleDiffButton(RoR::GfxActor* actorx);
    void DrawWheelDiffButton(RoR::GfxActor* actorx);
    void DrawTransferCaseModeButton(RoR::GfxActor* actorx);
    void DrawTransferCaseGearRatioButton(RoR::GfxActor* actorx);
    void DrawParticlesButton(RoR::GfxActor* actorx);
    void DrawBeaconButton(RoR::GfxActor* actorx);
    void DrawShiftModeButton(RoR::GfxActor* actorx);
    void DrawEngineButton(RoR::GfxActor* actorx);
    void DrawCustomLightButton(RoR::GfxActor* actorx);
    void DrawCommandButton(RoR::GfxActor* actorx);
    void DrawLockButton(RoR::GfxActor* actorx);
    void DrawSecureButton(RoR::GfxActor* actorx);
    void DrawCruiseControlButton(RoR::GfxActor* actorx);
    void DrawCameraButton();
    void CacheIcons();
    bool m_horn = false;
    std::vector<int> m_id;
    Ogre::Timer m_timer;
    bool m_init = false;

    // Icon cache
    bool m_icons_cached = false;
    Ogre::TexturePtr m_headlight_icon;
    Ogre::TexturePtr m_left_blinker_icon;
    Ogre::TexturePtr m_right_blinker_icon;
    Ogre::TexturePtr m_warning_light_icon;
    Ogre::TexturePtr m_horn_icon;
    Ogre::TexturePtr m_mirror_icon;
    Ogre::TexturePtr m_repair_icon;
    Ogre::TexturePtr m_parking_brake_icon;
    Ogre::TexturePtr m_traction_control_icon;
    Ogre::TexturePtr m_abs_icon;
    Ogre::TexturePtr m_physics_icon;
    Ogre::TexturePtr m_actor_physics_icon;
    Ogre::TexturePtr m_a_icon;
    Ogre::TexturePtr m_w_icon;
    Ogre::TexturePtr m_m_icon;
    Ogre::TexturePtr m_g_icon;
    Ogre::TexturePtr m_particle_icon;
    Ogre::TexturePtr m_shift_icon;
    Ogre::TexturePtr m_engine_icon;
    Ogre::TexturePtr m_beacons_icon;
    Ogre::TexturePtr m_camera_icon;
    Ogre::TexturePtr m_lock_icon;
    Ogre::TexturePtr m_secure_icon;
    Ogre::TexturePtr m_cruise_control_icon;
};

} // namespace GUI
} // namespace RoR