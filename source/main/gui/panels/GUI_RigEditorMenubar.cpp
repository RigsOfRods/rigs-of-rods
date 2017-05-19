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
    @file   GUI_RigEditorMenubar.cpp
    @author Petr Ohlidal
    @date   08/2014
*/

#include "GUI_RigEditorMenubar.h"

#include "RigEditor_GuiPopupWheelsList.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

RigEditorMenubar::RigEditorMenubar(RigEditor::IMain* rig_editor_interface)
{
    m_rig_editor_interface = rig_editor_interface;

    m_file_popup_item_open           ->eventMouseButtonClick += MyGUI::newDelegate(this, &RigEditorMenubar::OpenFileItemClicked);
    m_file_popup_item_close          ->eventMouseButtonClick += MyGUI::newDelegate(this, &RigEditorMenubar::CloseRigItemClicked);
    m_file_popup_item_quit           ->eventMouseButtonClick += MyGUI::newDelegate(this, &RigEditorMenubar::QuitEditorItemClicked);
    m_file_popup_item_properties     ->eventMouseButtonClick += MyGUI::newDelegate(this, &RigEditorMenubar::RigPropertiesItemClicked);
    m_file_popup_item_land_properties->eventMouseButtonClick += MyGUI::newDelegate(this, &RigEditorMenubar::LandVehiclePropertiesItemClicked);
    m_file_popup_item_save_as        ->eventMouseButtonClick += MyGUI::newDelegate(this, &RigEditorMenubar::SaveFileAsItemClicked);
    m_file_popup_item_create_empty   ->eventMouseButtonClick += MyGUI::newDelegate(this, &RigEditorMenubar::CreateEmptyRigItemClicked);
    m_menubar_item_help              ->eventMouseButtonClick += MyGUI::newDelegate(this, &RigEditorMenubar::MenubarItemHelpClicked);

    m_wheels_list = std::unique_ptr<RigEditor::GuiPopupWheelsList>(
        new RigEditor::GuiPopupWheelsList(
            rig_editor_interface, m_wheels_popup,
            m_wheels_popup_item_select_all, m_wheels_popup_item_deselect_all)
        );
}

void RigEditorMenubar::Show()
{
    m_rig_editor_menubar->setVisible(true);
}

void RigEditorMenubar::Hide()
{
    m_rig_editor_menubar->setVisible(false);
}

void RigEditorMenubar::StretchWidthToScreen()
{
    MyGUI::IntSize parentSize = m_rig_editor_menubar->getParentSize();
    m_rig_editor_menubar->setSize(parentSize.width, m_rig_editor_menubar->getHeight());
}

void RigEditorMenubar::UpdateLandVehicleWheelsList(std::vector<RigEditor::LandVehicleWheel*> & list)
{
    m_wheels_list->UpdateWheelsList(list);
}

void RigEditorMenubar::ClearLandVehicleWheelsList()
{
    m_wheels_list->ClearWheelsList();
}

// ============================================================================
// Event handlers
// ============================================================================

void RigEditorMenubar::OpenFileItemClicked(MyGUI::Widget* sender)
{
    m_rig_editor_interface->CommandShowDialogOpenRigFile();
}

void RigEditorMenubar::SaveFileAsItemClicked(MyGUI::Widget* sender)
{
    m_rig_editor_interface->CommandShowDialogSaveRigFileAs();
}

void RigEditorMenubar::CloseRigItemClicked(MyGUI::Widget* sender)
{
    m_rig_editor_interface->CommandCloseCurrentRig();
}

void RigEditorMenubar::QuitEditorItemClicked(MyGUI::Widget* sender)
{
    m_rig_editor_interface->CommandQuitRigEditor();
}

void RigEditorMenubar::RigPropertiesItemClicked(MyGUI::Widget* sender)
{
    m_rig_editor_interface->CommandShowRigPropertiesWindow();
}

void RigEditorMenubar::LandVehiclePropertiesItemClicked(MyGUI::Widget* sender)
{
    m_rig_editor_interface->CommandShowLandVehiclePropertiesWindow();
}

void RigEditorMenubar::MenubarItemHelpClicked(MyGUI::Widget* sender)
{
    m_rig_editor_interface->CommandShowHelpWindow();
}

void RigEditorMenubar::CreateEmptyRigItemClicked(MyGUI::Widget* sender)
{
    m_rig_editor_interface->CommandCreateNewEmptyRig();
}
