/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2014-2017 Petr Ohlidal & contributors

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
/// @author Petr Ohlidal
/// @date   06/2017

#include "GUI_GamePauseMenu.h"

#include "Application.h"
#include "GUIManager.h"
#include "RoRPrerequisites.h"

RoR::GUI::GamePauseMenu::GamePauseMenu(): 
    m_kb_focus_index(-1), m_kb_enter_index(-1)
{}

void RoR::GUI::GamePauseMenu::Draw() // TODO: Copypaste of 'GameMainMenu' -- cleanup and unify the logic! ~ only_a_ptr, 06/2017
{
    // Keyboard updates - move up/down and wrap on top/bottom. Initial index is '-1' which means "no focus"
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
    {
        m_kb_focus_index = (m_kb_focus_index <= 0) ? (NUM_BUTTONS - 1) : (m_kb_focus_index - 1);
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
    {
        m_kb_focus_index = (m_kb_focus_index < (NUM_BUTTONS - 1)) ? (m_kb_focus_index + 1) : 0;
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
    {
        m_kb_enter_index = m_kb_focus_index;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, BUTTON_PADDING);
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive]);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, WINDOW_BG_COLOR);
    ImGui::PushStyleColor(ImGuiCol_Button, BUTTON_BG_COLOR);

    // Display in bottom left corner for single-screen setups and centered for multi-screen (typically 3-screen) setups.
    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    if ((display_size.x > 2200.f) && (display_size.y < 1100.f)) // Silly approximate values
    {
        ImGui::SetNextWindowPosCenter();
    }
    else
    {
        const float btn_height = ImGui::GetTextLineHeight() + (BUTTON_PADDING.y * 2);
        const float window_height = ((NUM_BUTTONS+1)*btn_height) + ((NUM_BUTTONS+1)*ImGui::GetStyle().ItemSpacing.y) 
                                    + (2*ImGui::GetStyle().WindowPadding.y); // 5 buttons + titlebar; 2x spacing around separator
        const float margin = display_size.y / 15.f;
        const float top = display_size.y - window_height - margin;
        ImGui::SetNextWindowPos(ImVec2(margin, top));
    }
    ImGui::SetNextWindowContentWidth(WINDOW_WIDTH);
    int flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin("Pause", nullptr, static_cast<ImGuiWindowFlags_>(flags)))
    {
        ImVec2 btn_size(WINDOW_WIDTH, 0.f);

        const char* resume_title = (m_kb_focus_index == 0) ? "--> Resume game <--" : "Resume game"; // TODO: Localize all!
        if (ImGui::Button(resume_title, btn_size) || (m_kb_enter_index == 0))
        {
            App::sim_state->SetPendingVal((int)SimState::RUNNING);
        }

        const char* settings_title = (m_kb_focus_index == 1) ? "--> Return to menu <--" : "Return to menu";
        if (ImGui::Button(settings_title, btn_size) || (m_kb_enter_index == 1))
        {
            App::app_state->SetPendingVal((int)RoR::AppState::MAIN_MENU);
        }

        const char* exit_title = (m_kb_focus_index == 2) ? "--> Exit game <--" : "Exit game";
        if (ImGui::Button(exit_title, btn_size) || (m_kb_enter_index == 2))
        {
            App::app_state->SetPendingVal((int)RoR::AppState::SHUTDOWN);
        }
    }

    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    m_kb_enter_index = -1;
}

