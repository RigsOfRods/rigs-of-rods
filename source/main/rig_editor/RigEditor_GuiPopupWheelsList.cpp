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

#include "RigEditor_GuiPopupWheelsList.h"

#include "RigEditor_IMain.h"

using namespace RoR;
using namespace RigEditor;

GuiPopupWheelsList::GuiPopupWheelsList(
        RigEditor::IMain* rig_editor_interface,
        MyGUI::PopupMenu* parent_menu, 
        MyGUI::MenuItem* select_all_menuitem,  // = nullptr
        MyGUI::MenuItem* deselect_all_menuitem // = nullptr
        ):
    GuiPopupDynamicListBase(parent_menu, select_all_menuitem, deselect_all_menuitem),
    m_rig_editor_interface(rig_editor_interface)
{
    assert(rig_editor_interface != nullptr);
}

#define WHEEL_FORWARD_CALLBACK(MENUITEM_PTR, USERDATA_PTR, STATE_VALUE, FUNC_NAME) \
{ \
    assert(userdata != nullptr); \
    assert(userdata->bound_object_ptr != nullptr); \
    m_rig_editor_interface->FUNC_NAME( \
        static_cast<LandVehicleWheel*>((USERDATA_PTR)->bound_object_ptr), (USERDATA_PTR)->bound_object_index, (STATE_VALUE)); \
}

void GuiPopupWheelsList::OnItemMouseFocusGained       (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata)
{
    WHEEL_FORWARD_CALLBACK(menu_item, userdata, true, CommandSetWheelHovered);
}

void GuiPopupWheelsList::OnItemMouseFocusLost       (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata)
{
    WHEEL_FORWARD_CALLBACK(menu_item, userdata, false, CommandSetWheelHovered);
}

void GuiPopupWheelsList::OnItemMouseClicked       (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata)
{
    WHEEL_FORWARD_CALLBACK(menu_item, userdata, true, CommandScheduleSetWheelSelected);
}

void GuiPopupWheelsList::OnSelectAllMouseClicked  (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata)
{
    m_rig_editor_interface->CommandScheduleSetAllWheelsSelected(true);
}

void GuiPopupWheelsList::OnDeselectAllMouseClicked(MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata)
{
    m_rig_editor_interface->CommandScheduleSetAllWheelsSelected(false);
}

void GuiPopupWheelsList::UpdateWheelsList(std::vector<LandVehicleWheel*> wheels_list)
{
    this->ClearList();

    auto end = wheels_list.end();
    int index = 0;
    for (auto itor = wheels_list.begin(); itor != end; ++itor)
    {
        LandVehicleWheel* wheel = *itor;
        MyGUI::MenuItem* menuitem = this->AddItemToList(static_cast<void*>(wheel), index);
        menuitem->setVisible(true);
        char caption[25];
        sprintf(caption, "Wheel %d", index);
        menuitem->setCaption(caption);
        ++index;
    }
}

void GuiPopupWheelsList::OnSelectAllMouseFocusGained(MyGUI::MenuItem *,RoR::RigEditor::GuiPopupDynamicListBase::GuiWidgetUserdata *)
{
    m_rig_editor_interface->CommandSetAllWheelsHovered(true);
}

void GuiPopupWheelsList::OnSelectAllMouseFocusLost(MyGUI::MenuItem *,RoR::RigEditor::GuiPopupDynamicListBase::GuiWidgetUserdata *)
{
    m_rig_editor_interface->CommandSetAllWheelsHovered(false);
}

void GuiPopupWheelsList::OnDeselectAllMouseFocusGained(MyGUI::MenuItem *,RoR::RigEditor::GuiPopupDynamicListBase::GuiWidgetUserdata *)
{}

void GuiPopupWheelsList::OnDeselectAllMouseFocusLost(MyGUI::MenuItem *,RoR::RigEditor::GuiPopupDynamicListBase::GuiWidgetUserdata *)
{}