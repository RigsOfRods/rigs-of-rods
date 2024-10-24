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

/// @author Petr Ohlidal
/// @date   10/2019
///   Based on SimUtils - TruckInfo by Moncef Ben Slimane, 12/2014
///   Iconified buttons by tritonas00, 2021

#pragma once

#include "ForwardDeclarations.h"

namespace RoR {
namespace GUI {

class VehicleInfoTPanel
{
public:
    enum TPanelMode { TPANELMODE_HIDDEN, TPANELMODE_OPAQUE, TPANELMODE_TRANSLUCENT };
    enum TPanelFocus { TPANELFOCUS_NONE, TPANELFOCUS_BASICS, TPANELFOCUS_COMMANDS, TPANELFOCUS_STATS, TPANELFOCUS_DIAG };

    void SetVisible(TPanelMode mode, TPanelFocus focus = TPANELFOCUS_NONE);
    bool IsVisible(TPanelFocus focus = TPANELFOCUS_NONE) const { return m_visibility_mode == TPANELMODE_OPAQUE && (focus == TPANELFOCUS_NONE || focus == m_current_focus); }
    CommandkeyID_t GetActiveCommandKey() const { return m_active_commandkey; }
    bool IsHornButtonActive() const { return m_horn_btn_active; }

    void UpdateStats(float dt, ActorPtr actor); //!< Caution: touches live data, must be synced with sim. thread
    void Draw(RoR::GfxActor* actorx);

private:
    /// @name The T panel itself
    /// @{
    TPanelMode m_visibility_mode = TPANELMODE_HIDDEN;
    TPanelFocus m_requested_focus = TPANELFOCUS_NONE; //!< Requested by `SetVisible()`
    TPanelFocus m_current_focus = TPANELFOCUS_NONE; //!< Updated based on currently open tab
    Ogre::Timer m_startupdemo_timer;
    bool m_startupdemo_init = false;
    ImVec4 m_panel_translucent_color = ImVec4(0.1f, 0.1f, 0.1f, 0.5f);
    ImVec4 m_transluc_textdis_color = ImVec4(0.64f, 0.64f, 0.63f, 1.f);
    /// @}

    /// @name 'Vehicle commands' tab
    /// @{
    void DrawVehicleCommandsUI(RoR::GfxActor* actorx);
    void DrawVehicleCommandHighlights(RoR::GfxActor* actorx);
    void DrawVehicleHelpTextureFullsize(RoR::GfxActor* actorx);
    
    CommandkeyID_t m_active_commandkey = COMMANDKEYID_INVALID;
    CommandkeyID_t m_hovered_commandkey = COMMANDKEYID_INVALID;
    
    ImVec4 m_cmdbeam_highlight_color = ImVec4(0.733f, 1.f, 0.157f, 0.745f);
    float m_cmdbeam_ui_rect_thickness = 3.f;
    float m_cmdbeam_highlight_thickness = 15.f;
    ImVec4 m_command_hovered_text_color = ImVec4(0.1f, 0.1f, 0.1f, 1.f);
    bool m_helptext_fullsize = false;
    ImVec2 m_helptext_fullsize_screenpos; //!< The image is drawn into separate window
    /// @}
   
    /// @name 'Vehicle stats' tab
    /// @{
    void DrawVehicleStatsUI(RoR::GfxActor* actorx);
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
    /// @}

    /// @name 'Vehicle diagnostics (skeletonview)' tab
    /// @{
    void DrawVehicleDiagUI(RoR::GfxActor* actorx);
    /// @}

    /// @name 'Vehicle basics (buttons)' tab
    /// @{
    void DrawVehicleBasicsUI(RoR::GfxActor* actorx);
    void DrawHeadLightButton(RoR::GfxActor* actorx);
    void DrawLeftBlinkerButton(RoR::GfxActor* actorx);
    void DrawRightBlinkerButton(RoR::GfxActor* actorx);
    void DrawWarnBlinkerButton(RoR::GfxActor* actorx);
    void DrawHornButton(RoR::GfxActor* actorx);
    void DrawMirrorButton(RoR::GfxActor* actorx);
    void DrawRepairButton(RoR::GfxActor* actorx);
    void DrawParkingBrakeButton(RoR::GfxActor* actorx);
    void DrawTractionControlButton(RoR::GfxActor* actorx);
    void DrawAntiLockBrakeButton(RoR::GfxActor* actorx);
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
    void DrawLockButton(RoR::GfxActor* actorx);
    void DrawSecureButton(RoR::GfxActor* actorx);
    void DrawCruiseControlButton(RoR::GfxActor* actorx);
    void DrawCameraButton();
    void DrawCustomLightButtons(RoR::GfxActor* actorx);
    void CacheIcons();
    bool m_horn_btn_active = false;
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
    /// @}
};



} // namespace GUI
} // namespace RoR
