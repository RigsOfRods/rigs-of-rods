/*
    This source file is part of Rigs of Rods
    Copyright 2013-2020 Petr Ohlidal

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

#pragma once

#include "GUI_VirtualKeyboard.h"

/* ------- CrystalKeys (https://github.com/cryham/ckeys) --------
Deps (and their modifications): 
    Bundled: NFD (Native File Dialog)
     > All files are fed externally using OGRE resource system
    Bundled: imgui-SFML (https://github.com/eliasdaler/imgui-sfml/blob/master/imgui-SFML.h)
     > Using Ogre-IMGUI to draw, all direct calls to SFML were replaced by ImGUI's DrawList API
    Bundled: TinyXML2
     > XML config support removed
    Bundled: Jsmn (https://github.com/zserge/jsmn)
     > JSON support removed - the format is poorly designed
*/

void RoR::GUI::VirtualKeyboard::Init()
{
    m_ckeys = std::make_unique<ckeys::App>();
    m_ckeys->Init();
}

void RoR::GUI::VirtualKeyboard::Draw()
{
    // Handle input
    m_ckeys->Mouse((int)ImGui::GetIO().MousePos.x, (int)ImGui::GetIO().MousePos.y); // Update internal mouse pos info

    if (ImGui::IsKeyDown(ImGuiKey_Enter))  { m_ckeys->KeyDown(ImGuiKey_Enter); }
    if (ImGui::IsKeyDown(ImGuiKey_Escape)) { m_ckeys->KeyDown(ImGuiKey_Escape); }

    // Set GUI theme
    ImGuiStyle orig_style = ImGui::GetStyle(); // Back up current settings (copy out ImGui's internal state)
    m_ckeys->SetupGuiClr(); // Set up ckeys theme

    // Draw GUI
    m_ckeys->Gui(); // Draw supporting GUI panels (options, graphics)
    m_ckeys->Graph(); // Draw the virtual keyboard

    // Restore GUI theme
    ImGui::GetStyle() = orig_style; // Rude overwrite of ImGui's internal state
}


