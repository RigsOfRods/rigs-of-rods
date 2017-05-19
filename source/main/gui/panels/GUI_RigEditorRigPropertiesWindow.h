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
    @file
    @author Petr Ohlidal
    @date   09/2014
*/

#include "ForwardDeclarations.h"
#include "GUI_RigEditorRigPropertiesWindowLayout.h"
#include "GuiPanelBase.h"
#include "RigDef_File.h"
#include "RigDef_Prerequisites.h"
#include "RigEditor_ForwardDeclarations.h"

namespace RoR
{

namespace GUI
{

class RigEditorRigPropertiesWindow: public RigEditorRigPropertiesWindowLayout, public GuiPanelBase
{

public:

    RigEditorRigPropertiesWindow(RigEditor::IMain* rig_editor_interface);

    void Import(RigEditor::RigProperties* rig_properties);

    void Export(RigEditor::RigProperties* rig_properties);

private:

    void SetExtCameraMode(RigDef::ExtCamera::Mode mode, RigDef::Node::Ref node_ref);

    void SetMinimapMode(RigDef::GuiSettings::MapMode mode);

    /* Event handlers */

    void WindowButtonClicked(MyGUI::Widget* sender, const std::string& name);

    void CancelButtonClicked(MyGUI::Widget* sender);

    void CheckboxClicked(MyGUI::Widget* sender);

    void ExtcameraRadiobuttonClicked(MyGUI::Widget* sender);

    void MinimapRadiobuttonClicked(MyGUI::Widget* sender);

    void SaveButtonClicked(MyGUI::Widget* sender);

private:

    RigEditor::IMain* m_rig_editor_interface;

    RigDef::ExtCamera::Mode m_extcamera_mode;

};

} // namespace GUI

} // namespace RoR
