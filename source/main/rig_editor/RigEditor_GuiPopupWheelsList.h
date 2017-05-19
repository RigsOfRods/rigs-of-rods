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

#include "RigDef_Prerequisites.h"
#include "RigEditor_IMain.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_GuiPopupDynamicListBase.h"

#include <MyGUI_Prerequest.h>

namespace RoR
{

namespace RigEditor
{

class GuiPopupWheelsList: public GuiPopupDynamicListBase
{
public:

    /// @param parent_menu Required
    /// @param select_all_menuitem Optional
    /// @param deselect_all_menuitem Optional
    GuiPopupWheelsList(
        RigEditor::IMain* rig_editor_interface,
        MyGUI::PopupMenu* parent_menu, 
        MyGUI::MenuItem* select_all_menuitem = nullptr,
        MyGUI::MenuItem* deselect_all_menuitem = nullptr
        );

    void ClearWheelsList() { ClearList(); }

    void UpdateWheelsList(std::vector<LandVehicleWheel*> wheels_list);

protected:

    virtual void OnItemMouseFocusGained    (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata);
    virtual void OnItemMouseFocusLost      (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata);
                                           
    virtual void OnItemMouseClicked        (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata);
    virtual void OnSelectAllMouseClicked   (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata);
    virtual void OnDeselectAllMouseClicked (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata);

    void OnSelectAllMouseFocusGained  (MyGUI::MenuItem *,RoR::RigEditor::GuiPopupDynamicListBase::GuiWidgetUserdata *);
    void OnSelectAllMouseFocusLost    (MyGUI::MenuItem *,RoR::RigEditor::GuiPopupDynamicListBase::GuiWidgetUserdata *);
    void OnDeselectAllMouseFocusGained(MyGUI::MenuItem *,RoR::RigEditor::GuiPopupDynamicListBase::GuiWidgetUserdata *);
    void OnDeselectAllMouseFocusLost  (MyGUI::MenuItem *,RoR::RigEditor::GuiPopupDynamicListBase::GuiWidgetUserdata *);

    RigEditor::IMain* m_rig_editor_interface;
};

} // namespace RigEditor

} // namespace RoR
