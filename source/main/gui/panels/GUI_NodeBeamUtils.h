/*
    This source file is part of Rigs of Rods
    Copyright 2016-2017 Petr Ohlidal & contributors

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

class NodeBeamUtils
{
public:
    NodeBeamUtils(): m_is_visible(false), m_is_searching(false) {}

    void Draw();

    bool IsVisible() const { return m_is_visible; }
    void SetVisible(bool v);

private:
    bool m_is_visible;
    bool m_is_searching;

    const ImVec4 GRAY_HINT_TEXT = ImVec4(0.62f, 0.62f, 0.61f, 1.f);
};

} // namespace GUI
} // namespace RoR
