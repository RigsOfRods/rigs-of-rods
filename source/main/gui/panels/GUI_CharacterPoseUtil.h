/*
    This source file is part of Rigs of Rods
    Copyright 2016-2020 Petr Ohlidal

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

namespace RoR {
namespace GUI {

class CharacterPoseUtil
{
public:
    void Draw();

    void SetVisible(bool visible);
    bool IsVisible() const { return m_is_visible; }
    bool IsHovered() const { return IsVisible() && m_is_hovered; }

private:
    bool m_is_visible = false;
    bool m_is_hovered = false;

};

} // namespace GUI
} // namespace RoR
