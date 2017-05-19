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
    @file   GUI_RigEditorDeleteMenu.cpp
    @author Petr Ohlidal
    @date   09/2014
*/

#include "GUI_RigEditorDeleteMenu.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

RigEditorDeleteMenu::RigEditorDeleteMenu(RigEditor::IMain* rig_editor_interface):
    GuiPanelBase(m_rig_editor_delete_menu)
{
    m_rig_editor_interface = rig_editor_interface;

    m_delete_nodes_button->eventMouseButtonClick += MyGUI::newDelegate(this, &RigEditorDeleteMenu::DeleteNodesButtonClicked);
    m_delete_beams_button->eventMouseButtonClick += MyGUI::newDelegate(this, &RigEditorDeleteMenu::DeleteBeamsButtonClicked);

    Hide();
}

void RigEditorDeleteMenu::DeleteNodesButtonClicked(MyGUI::Widget* sender)
{
    m_rig_editor_interface->CommandCurrentRigDeleteSelectedNodes();
}

void RigEditorDeleteMenu::DeleteBeamsButtonClicked(MyGUI::Widget* sender)
{
    m_rig_editor_interface->CommandCurrentRigDeleteSelectedBeams();
}

bool RigEditorDeleteMenu::TestCursorInRange(int x, int y, int range)
{
    bool ok = true;
    MyGUI::IntRect box = m_rig_editor_delete_menu->getAbsoluteRect();
    ok = x < box.left - range   ? false : ok;
    ok = x > box.right + range  ? false : ok;
    ok = y < box.top - range    ? false : ok;
    ok = y > box.bottom + range ? false : ok;
    return ok;
}
