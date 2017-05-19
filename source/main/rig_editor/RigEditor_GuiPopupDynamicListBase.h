/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.com/

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

/** 
    @file   
    @date   03/2015
    @author Petr Ohlidal
*/

#pragma once

#include <MyGUI.h>

namespace RoR
{

namespace RigEditor
{

class GuiPopupDynamicListBase
{
public:
    struct GuiWidgetUserdata
    {
        void* bound_object_ptr;
        int   bound_object_index;
    };

    /// @param parent_menu Required
    /// @param select_all_menuitem Optional
    /// @param deselect_all_menuitem Optional
    GuiPopupDynamicListBase(
        MyGUI::PopupMenu* parent_menu, 
        MyGUI::MenuItem* select_all_menuitem = nullptr,
        MyGUI::MenuItem* deselect_all_menuitem = nullptr
        );

protected:

    void ClearList();

    MyGUI::MenuItem* AddItemToList(void* bound_ptr, int bound_index);

    virtual void OnItemMouseFocusGained       (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata) = 0;
    virtual void OnItemMouseFocusLost         (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata) = 0;
    virtual void OnItemMouseClicked           (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata) = 0;

    virtual void OnSelectAllMouseClicked      (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata) = 0;
    virtual void OnSelectAllMouseFocusGained  (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata) = 0;
    virtual void OnSelectAllMouseFocusLost    (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata) = 0;
    
    virtual void OnDeselectAllMouseClicked    (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata) = 0;
    virtual void OnDeselectAllMouseFocusGained(MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata) = 0;
    virtual void OnDeselectAllMouseFocusLost  (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata) = 0;

    void GuiCallback_MouseFocusGained(MyGUI::Widget* menu_item_widget, MyGUI::Widget* menu_widget);
    void GuiCallback_MouseFocusLost  (MyGUI::Widget* menu_item_widget, MyGUI::Widget* menu_widget);
    void GuiCallback_MouseClicked    (MyGUI::Widget* sender);

    std::vector<MyGUI::MenuItem*> m_list_item_widgets;
    MyGUI::PopupMenu*             m_parent_popup_menu;
    MyGUI::MenuItem*              m_select_all_menuitem;
    MyGUI::MenuItem*              m_deselect_all_menuitem;
};

} // namespace RigEditor

} // namespace RoR
