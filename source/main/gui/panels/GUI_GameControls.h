/*
    This source file is part of Rigs of Rods
    Copyright 2020 tritonas00

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

namespace RoR {
namespace GUI {

class GameControls
{
public:
    void SetVisible(bool vis) { m_is_visible = vis; }
    bool IsVisible() const { return m_is_visible; }
    void Draw();

private:
    enum ControlsTab {AIRPLANE, BOAT, CAMERA, SKY, CHARACTER, COMMANDS, COMMON, GRASS, MAP, MENU, TRUCK};
    ControlsTab m_tab;
    bool m_is_visible = false;
    Str<1000> buf;
};

} // namespace GUI
} // namespace RoR
