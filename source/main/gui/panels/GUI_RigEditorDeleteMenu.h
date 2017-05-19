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
    @file   GUI_RigEditorDeleteMenu.h
    @author Petr Ohlidal
    @date   09/2014
*/

#include "ForwardDeclarations.h"
#include "GUI_RigEditorDeleteMenuLayout.h"
#include "RigEditor_IMain.h"
#include "GuiPanelBase.h"

namespace RoR
{

namespace GUI
{

class RigEditorDeleteMenu: public RigEditorDeleteMenuLayout, public GuiPanelBase
{

public:

    RigEditorDeleteMenu(RigEditor::IMain* rig_editor_interface);

    bool TestCursorInRange(int x, int y, int range);

private:

    void DeleteNodesButtonClicked(MyGUI::Widget* sender);

    void DeleteBeamsButtonClicked(MyGUI::Widget* sender);

    RigEditor::IMain* m_rig_editor_interface;

};

} // namespace GUI

} // namespace RoR
