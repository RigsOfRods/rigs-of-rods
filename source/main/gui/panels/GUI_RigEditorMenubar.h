/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors.

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

#pragma once

/** 
    @file   GUI_RigEditorMenubar.h
    @author Petr Ohlidal
    @date   08/2014
*/

#include "ForwardDeclarations.h"
#include "RigEditor_GuiPopupWheelsList.h"
#include "GUI_RigEditorMenubarLayout.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_GuiPopupWheelsList.h"

#include "RigEditor_IMain.h"

#include <vector>

namespace RoR
{

namespace GUI
{

class RigEditorMenubar: public RigEditorMenubarLayout
{

public:

    RigEditorMenubar(RigEditor::IMain* rig_editor_interface);

    void Show();
    
    void Hide();

    void StretchWidthToScreen();

    void UpdateLandVehicleWheelsList(std::vector<RigEditor::LandVehicleWheel*> & list);

    void ClearLandVehicleWheelsList();

private:

    /* Event handlers */

    void OpenFileItemClicked(MyGUI::Widget* sender);

    void SaveFileAsItemClicked(MyGUI::Widget* sender);

    void CloseRigItemClicked(MyGUI::Widget* sender);

    void RigPropertiesItemClicked(MyGUI::Widget* sender);

    void QuitEditorItemClicked(MyGUI::Widget* sender);

    void LandVehiclePropertiesItemClicked(MyGUI::Widget* sender);

    void CreateEmptyRigItemClicked(MyGUI::Widget* sender);

    void MenubarItemHelpClicked(MyGUI::Widget* sender);

    inline RigEditor::GuiPopupWheelsList* GetWheelsList() { return m_wheels_list.get(); }

private:

    RigEditor::IMain*      m_rig_editor_interface;

    std::unique_ptr<RigEditor::GuiPopupWheelsList> m_wheels_list;

};

} // namespace GUI

} // namespace RoR
