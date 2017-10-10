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

#include "GUI_GameMainMenu.h"

#include "Application.h"
#include "GUIManager.h"
#include "GUI_MainSelector.h"
#include "MainMenu.h"

RoR::GUI::GameMainMenu::GameMainMenu(): 
    m_is_visible(false), m_kb_focus_index(-1), m_kb_enter_index(-1)
{}

void RoR::GUI::GameMainMenu::Draw()
{
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
        const float window_height = (6*btn_height) + (5*ImGui::GetStyle().ItemSpacing.y) + (2*ImGui::GetStyle().WindowPadding.y); // 5 buttons + titlebar; 2x spacing around separator
        const float margin = display_size.y / 15.f;
        const float top = display_size.y - window_height - margin;
        ImGui::SetNextWindowPos(ImVec2(margin, top));
    }
    ImGui::SetNextWindowContentWidth(WINDOW_WIDTH);
    int flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin("Main menu", nullptr, static_cast<ImGuiWindowFlags_>(flags)))
    {
        ImVec2 btn_size(WINDOW_WIDTH - ImGui::GetStyle().WindowPadding.x, 0.f);

        const char* sp_title = (m_kb_focus_index == 0) ? "--> Single player <--" : "Single player"; // TODO: Localize all!
        if (ImGui::Button(sp_title, btn_size) || (m_kb_enter_index == 0))
        {
            this->SetVisible(false);
            if (App::diag_preset_terrain.IsActiveEmpty())
            {
                App::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
            }
            else
            {
                App::app_state.SetPending(RoR::AppState::SIMULATION);
            }
        }

        const char* mp_title = (m_kb_focus_index == 1) ? "--> Multi player <--" : "Multi player";
        if (ImGui::Button(mp_title , btn_size) || (m_kb_enter_index == 1))
        {
            App::GetGuiManager()->SetVisible_MultiplayerSelector(true);
            this->SetVisible(false);
        }

        ImGui::Separator();

        const char* settings_title = (m_kb_focus_index == 2) ? "--> Settings <--" : "Settings";
        if (ImGui::Button(settings_title, btn_size) || (m_kb_enter_index == 2))
        {
            // TODO: Re-implement the settings panel
        }

        const char* about_title = (m_kb_focus_index == 3) ? "--> About <--" : "About";
        if (ImGui::Button(about_title, btn_size)|| (m_kb_enter_index == 3))
        {
            App::GetGuiManager()->SetVisible_GameAbout(true);
            this->SetVisible(false);
        }

        const char* exit_title = (m_kb_focus_index == 4) ? "--> Exit game <--" : "Exit game";
        if (ImGui::Button(exit_title, btn_size) || (m_kb_enter_index == 4))
        {
            App::app_state.SetPending(RoR::AppState::SHUTDOWN);
            this->SetVisible(false);
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    m_kb_enter_index = -1;
}

