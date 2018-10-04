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

#include "ForwardDeclarations.h"
#include "GUI_TeleportWindowLayout.h"
#include "GuiPanelBase.h"

namespace RoR {
namespace GUI {

class TeleportWindow : public TeleportWindowLayout, public GuiPanelBase
{
public:
    TeleportWindow();

    void SetupMap(Terrn2Def* def, Ogre::Vector3 map_size);
    void Reset();
    void SetVisible(bool v);
    bool IsVisible();
    void TeleportWindowFrameStep(float x, float z, bool alt_active);

private:
    void WindowButtonClicked   (MyGUI::Widget* sender, const std::string& name);
    void TelepointIconGotFocus (MyGUI::Widget* cur_widget, MyGUI::Widget* previous);
    void TelepointIconLostFocus(MyGUI::Widget* cur_widget, MyGUI::Widget* next);
    void TelepointIconClicked  (MyGUI::Widget* sender);
    void MinimapPanelResized   (MyGUI::Widget* sender);
    void MinimapGotFocus       (MyGUI::Widget* widget, MyGUI::Widget* previous);
    void MinimapLostFocus      (MyGUI::Widget* widget, MyGUI::Widget* next);
    void MinimapMouseMoved     (MyGUI::Widget* minimap, int left, int top);
    void MinimapMouseClick     (MyGUI::Widget* minimap);
    void EnableAltMode         (bool enable);
    void ShowAltmodeCursor     (bool show);
    void SetAltmodeCursorPos   (int screen_left, int screen_top);

    std::vector<MyGUI::ImageBox*> m_telepoint_icons;
    Ogre::Vector3                 m_map_size;
    MyGUI::ImageBox*              m_person_icon;
    MyGUI::ImageBox*              m_mouse_icon;
    bool                          m_is_altmode_active;
    bool                          m_minimap_has_focus;
    MyGUI::IntPoint               m_minimap_last_mouse; ///< Screen coordinates.
};

} // namespace GUI
} // namespace RoR

