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

namespace GUI {

class CharacterPoseUtil
{
public:
    enum class Tab { SKELETAL, GAME };

    void Draw();

    void SetVisible(bool visible);
    bool IsVisible() const { return m_is_visible; }
    bool IsHovered() const { return IsVisible() && m_is_hovered; }

private:
    void DrawAnimControls(Ogre::AnimationState* anim_state);
    void DrawActionDbgItemFull(CharacterActionID_t id);
    void DrawActionDbgItemInline(CharacterActionID_t id, Ogre::Entity* ent);
    void DrawActionDbgPanel(Ogre::Entity* ent);
    void DrawSkeletalPanel(Ogre::Entity* ent);

    GfxCharacter* FetchCharacter();

    bool m_is_visible = false;
    bool m_is_hovered = false;
    Tab  m_selected_tab = Tab::SKELETAL;
};

} // namespace GUI
} // namespace RoR
