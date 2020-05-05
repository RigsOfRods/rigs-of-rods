/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

/// @file
/// @author Moncef Ben Slimane
/// @date   12/2014

#include "GUI_MessageBox.h"

#include "Application.h"
#include "GUIManager.h"
#include "ScriptEvents.h"
#include "Scripting.h"

#include <imgui.h>

RoR::GUI::MessageBoxDialog::MessageBoxDialog():
    m_close_handle(nullptr),
    m_is_visible(false)
{}

RoR::GUI::MessageBoxDialog::~MessageBoxDialog()
{}

void RoR::GUI::MessageBoxDialog::Draw()
{
    const bool was_visible = m_is_visible;

    // Draw window
    ImGui::SetNextWindowContentWidth(300.f); // Initial size only
    ImGui::SetNextWindowPosCenter(ImGuiCond_Appearing); // Initial pos. only
    ImGui::Begin(m_title.c_str(), m_close_handle);
    ImGui::TextWrapped("%s", m_text.c_str());

    // Draw buttons
    if (!m_button1_text.empty())
    {
        if (ImGui::Button(m_button1_text.c_str()))
        {
            TRIGGER_EVENT(SE_GENERIC_MESSAGEBOX_CLICK, 1); // Scripting
            m_is_visible = false;
        }
    }
    if (!m_button2_text.empty())
    {
        if (ImGui::Button(m_button2_text.c_str()))
        {
            TRIGGER_EVENT(SE_GENERIC_MESSAGEBOX_CLICK, 2); // Scripting
            m_is_visible = false;
        }
    }

    // Finalize
    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();
    if (m_is_visible != was_visible)
    {
        TRIGGER_EVENT(SE_GENERIC_MESSAGEBOX_CLICK, 0); // Scripting
    }
}

void RoR::GUI::MessageBoxDialog::Show(const char* title, const char* text, bool allow_close, const char* button1_text, const char* button2_text)
{
    m_is_visible = true;
    m_title = title;
    m_text = text;
    m_button1_text = ((button1_text != nullptr) ? button1_text : "");
    m_button2_text = ((button2_text != nullptr) ? button2_text : "");

    if (allow_close)
    {
        m_close_handle = &m_is_visible;
    }
    else
    {
        m_close_handle = nullptr;
    }
}

