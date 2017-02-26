/*
    This source file is part of Rigs of Rods
    Copyright 2017 Petr Ohlidal & contributors

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

#include "GUI_TeleportWindowLayout.h"
#include "GuiPanelBase.h"

namespace RoR {

struct Terrn2Def; // Forward

namespace GUI {

class TeleportWindow : public TeleportWindowLayout, public GuiPanelBase
{
public:
    TeleportWindow();

    void SetupMap(RoRFrameListener* sim_controller, Terrn2Def* def, Ogre::Vector3 map_size, std::string minimap_tex_name);
    void Reset();
    void SetVisible(bool v);
    bool IsVisible();

private:
    void WindowButtonClicked(MyGUI::Widget* sender, const std::string& name);
    void TelepointIconGotFocus(MyGUI::Widget* cur_widget, MyGUI::Widget* prev_widget);
    void TelepointIconLostFocus(MyGUI::Widget* cur_widget, MyGUI::Widget* prev_widget);
    void TelepointIconClicked(MyGUI::Widget* sender);

    std::vector<MyGUI::ImageBox*> m_telepoint_icons;
    RoRFrameListener* m_sim_controller;
};

} // namespace GUI
} // namespace RoR

