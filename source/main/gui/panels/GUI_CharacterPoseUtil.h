/*
    This source file is part of Rigs of Rods
    Copyright 2022 Petr Ohlidal

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

#include "Application.h"
#include "OgreImGui.h"

#include <unordered_map>

namespace RoR {

struct CharacterActionDbg
{
    bool active = false;

    // State diagnostic.
    BitMask_t missing_situations = 0;  //!< `RoRnet::SituationFlags`; The flags from 'for_situations' mask which are not satisfied.
    BitMask_t missing_controls = 0;    //!< `RoRnet::ControlFlags`; The flags from 'for_controls' mask which are not satisfied.
    BitMask_t blocking_situations = 0; //!< `RoRnet::SituationFlags`; The flags from 'except_situations' mask which block this anim.
    BitMask_t blocking_controls = 0;   //!< `RoRnet::ControlFlags`; The flags from 'except_controls' mask which block this anim.

    // The raw source data of anim position.
    float source_dt = 0.f;
    float source_hspeed = 0.f;
    float source_steering = 0.f;

    // The transformed inputs to anim position.
    float input_dt = 0.f;
    float input_hspeed = 0.f;
    float input_steering = 0.f;
};

namespace GUI {

class CharacterPoseUtil
{
public:
    enum class Tab { SKELETAL, GAME };

    void Draw();

    void SetVisible(bool visible);
    bool IsVisible() const { return m_is_visible; }
    bool IsHovered() const { return IsVisible() && m_is_hovered; }

    bool IsManualPoseActive() const { return m_manual_pose_active; }

    std::vector<CharacterActionDbg> action_dbg_states;

private:
    void DrawAnimControls(Ogre::AnimationState* anim_state);
    void DrawActionDbgItemFull(CharacterActionID_t id);
    void DrawActionDbgItemInline(CharacterActionID_t id, Ogre::Entity* ent);
    void DrawActionDbgPanel(Ogre::Entity* ent);
    void DrawSkeletalPanel(Ogre::Entity* ent);

    bool m_is_visible = false;
    bool m_is_hovered = false;
    Tab  m_selected_tab = Tab::SKELETAL;

    bool m_manual_pose_active = false;
};

} // namespace GUI
} // namespace RoR
