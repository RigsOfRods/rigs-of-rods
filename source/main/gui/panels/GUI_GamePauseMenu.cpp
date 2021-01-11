/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
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

/// @file
/// @author Petr Ohlidal
/// @date   06/2017

#include "GUI_GamePauseMenu.h"

#include "Application.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "Language.h"

using namespace RoR;
using namespace GUI;

#define formatbutton(args, button_index) (m_kb_focus_index == button_index) ? std::string("--> ").append(args).append(" <--").c_str() : args

GamePauseMenu::GamePauseMenu(): 
    m_kb_focus_index(-1), m_kb_enter_index(-1)
{}

void GamePauseMenu::Draw() // TODO: Copypaste of 'GameMainMenu' -- cleanup and unify the logic! ~ only_a_ptr, 06/2017
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
    // The Pause menu
    if (ImGui::Begin(_L("Pause"), nullptr, static_cast<ImGuiWindowFlags_>(flags)))
    {
        ImVec2 btn_size(WINDOW_WIDTH, 0.f);

        if (ImGui::Button(formatbutton(_L("Resume game"), 0), btn_size) || (m_kb_enter_index == 0))
        {
            App::GetGameContext()->PushMessage(Message(MSG_SIM_UNPAUSE_REQUESTED));
        }

        if (ImGui::Button(formatbutton(_L("Return to menu"), 1), btn_size) || (m_kb_enter_index == 1))
        {
            App::GetGameContext()->PushMessage(Message(MSG_SIM_UNLOAD_TERRN_REQUESTED));
            if (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED)
            {
                App::GetGameContext()->PushMessage(Message(MSG_NET_DISCONNECT_REQUESTED));
            }
        }

        if (ImGui::Button(formatbutton(_L("Exit game"), 2), btn_size) || (m_kb_enter_index == 2))
        {
            App::GetGameContext()->PushMessage(Message(MSG_APP_SHUTDOWN_REQUESTED));
        }
    }

    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    m_kb_enter_index = -1;
}

