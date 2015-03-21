/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2015 Petr Ohlidal

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

#include "RigEditor_GuiPopupDynamicListBase.h"

using namespace RoR;
using namespace RigEditor;

GuiPopupDynamicListBase::GuiPopupDynamicListBase(
		MyGUI::PopupMenu* parent_menu, 
		MyGUI::MenuItem* select_all_menuitem,  // = nullptr
		MyGUI::MenuItem* deselect_all_menuitem // = nullptr
		):
	m_parent_popup_menu(parent_menu),
	m_select_all_menuitem(select_all_menuitem),
	m_deselect_all_menuitem(deselect_all_menuitem)
{
	assert(parent_menu != nullptr);	
}

void GuiPopupDynamicListBase::ClearList()
{
	while (m_list_item_widgets.size() != 0)
	{
		MyGUI::MenuItem* menuitem = m_list_item_widgets.back();
		m_parent_popup_menu->removeItem(menuitem); // Deallocates the widget
		m_list_item_widgets.pop_back();
	}
}

MyGUI::MenuItem* GuiPopupDynamicListBase::AddItemToList(void* bound_ptr, int bound_index)
{
	GuiWidgetUserdata userdata;
	userdata.bound_object_ptr = bound_ptr;
	userdata.bound_object_index = bound_index;
	MyGUI::MenuItem* menuitem = m_parent_popup_menu->addItem("", MyGUI::MenuItemType::Normal, "", userdata);
	m_list_item_widgets.push_back(menuitem);
	return menuitem;
}

void GuiPopupDynamicListBase::GuiCallback_MouseFocusGained(MyGUI::Widget* old_widget, MyGUI::Widget* new_widget)
{
	MyGUI::MenuItem* menu_item = static_cast<MyGUI::MenuItem*>(new_widget);
	GuiWidgetUserdata* userdata_ptr = new_widget->getUserData<GuiWidgetUserdata>();
	if (menu_item == m_select_all_menuitem)
	{
		this->OnSelectAllMouseFocusGained(menu_item, userdata_ptr);
	}
	else if (menu_item == m_deselect_all_menuitem)
	{
		this->OnDeselectAllMouseFocusGained(menu_item, userdata_ptr);
	}
	else
	{
		this->OnItemMouseFocusGained(menu_item, userdata_ptr);
	}
}

void GuiPopupDynamicListBase::GuiCallback_MouseFocusLost(MyGUI::Widget* old_widget, MyGUI::Widget* new_widget)
{
	MyGUI::MenuItem* menu_item = static_cast<MyGUI::MenuItem*>(new_widget);
	GuiWidgetUserdata* userdata_ptr = new_widget->getUserData<GuiWidgetUserdata>();
	this->OnItemMouseFocusLost(menu_item, userdata_ptr);
	if (menu_item == m_select_all_menuitem)
	{
		this->OnSelectAllMouseFocusLost(menu_item, userdata_ptr);
	}
	else if (menu_item == m_deselect_all_menuitem)
	{
		this->OnDeselectAllMouseFocusLost(menu_item, userdata_ptr);
	}
	else
	{
		this->OnItemMouseFocusLost(menu_item, userdata_ptr);
	}
}

void GuiPopupDynamicListBase::GuiCallback_MouseClicked(MyGUI::Widget* sender)
{
	MyGUI::MenuItem* menu_item = static_cast<MyGUI::MenuItem*>(sender);
	GuiWidgetUserdata* userdata_ptr = sender->getUserData<GuiWidgetUserdata>();
	if (menu_item == m_select_all_menuitem)
	{
		this->OnSelectAllMouseClicked(menu_item, userdata_ptr);
	}
	else if (menu_item == m_deselect_all_menuitem)
	{
		this->OnDeselectAllMouseClicked(menu_item, userdata_ptr);
	}
	else
	{
		this->OnItemMouseClicked(menu_item, userdata_ptr);
	}
}
