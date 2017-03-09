/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

/// @file
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   13th of August 2009


#pragma once

#include "RoRPrerequisites.h"
#include "Singleton.h"

#include <MyGUI.h>

#include <atomic>

namespace RoR {
namespace GUI {

class TopMenubar : public ZeroedMemoryAllocator
{
public:

    TopMenubar();

    ~TopMenubar();

    bool IsVisible();

    void SetVisible(bool value);

    int getMenuHeight()
    {
        return m_menu_height;
    };

    void updatePositionUponMousePosition(int x, int y);

    void triggerUpdateVehicleList();

    void ReflectMultiplayerState();

protected:

    void onMenuBtn(MyGUI::MenuCtrlPtr _sender, MyGUI::MenuItemPtr _item);

    void addUserToMenu(RoRnet::UserInfo &user);

    void vehiclesListUpdate();

    Ogre::UTFString getUserString(RoRnet::UserInfo &user, int num_vehicles);

    std::vector<MyGUI::PopupMenuPtr> m_popup_menus;
    MyGUI::PopupMenuPtr              m_vehicles_menu_widget;
    MyGUI::MenuBarPtr                m_menubar_widget;
    MyGUI::MenuItem*                 m_item_activate_all;
    MyGUI::MenuItem*                 m_item_never_sleep;
    MyGUI::MenuItem*                 m_item_sleep_all;
    MyGUI::MenuItem*                 m_item_spawner_log;
    int                              m_menu_width;
    int                              m_menu_height;
    std::atomic<bool>                m_vehicle_list_needs_update;
};

} // namespace GUI
} // namespace RoR
