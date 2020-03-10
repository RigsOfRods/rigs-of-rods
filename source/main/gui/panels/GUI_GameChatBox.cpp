/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal

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

#include "GUI_GameChatBox.h"

#include "Application.h"
#include "ChatSystem.h"
#include "Console.h"
#include "GUIManager.h"
#include "Language.h"

#include <cstring> // strtok, strncmp
#include <imgui.h>

RoR::GUI::GameChatBox::GameChatBox()
{
    m_console_view.cvw_align_bottom = true;
    m_console_view.cvw_max_lines = 20u;
    m_console_view.cvw_filter_duration_ms = 10000; // 10sec
    m_console_view.cvw_filter_area_actor = false; // Disable vehicle spawn warnings/errors
    m_console_view.cvw_filter_type_error = false; // Disable errors
    m_console_view.cvw_filter_type_cmd = false; // Disable commands
}

void RoR::GUI::GameChatBox::Draw()
{
    // Begin drawing the messages pane (no input)
    ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
    ImVec2 size(
        ImGui::GetIO().DisplaySize.x - (2 * ImGui::GetStyle().WindowPadding.x),
        (m_console_view.cvw_max_lines * ImGui::GetTextLineHeightWithSpacing()) + (2*ImGui::GetStyle().WindowPadding.y));
    if (m_is_visible) // Full display?
    {
        size.y += ImGui::GetTextLineHeightWithSpacing() + (2 * ImGui::GetStyle().WindowPadding.x); // reserve space for input window
    }
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - size.y));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0)); // Fully transparent background!
    ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImVec4(0,0,0,0)); // Fully transparent background!
    ImGui::Begin("ChatMessages", nullptr, win_flags);

    m_console_view.DrawConsoleMessages();
    ImGui::SetScrollFromPosY(9999); // Force to bottom

    ImGui::End();

    // Draw bottom bar (text input, filter settings)
    ImGuiWindowFlags bbar_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
    ImVec2 bbar_size(
        ImGui::GetIO().DisplaySize.x - (2 * ImGui::GetStyle().WindowPadding.x),
        ImGui::GetTextLineHeightWithSpacing() + (2 * ImGui::GetStyle().WindowPadding.x));
    ImGui::SetNextWindowSize(bbar_size);
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - bbar_size.y));
    ImGui::Begin("ChatBottomBar", nullptr, bbar_flags);

    if (m_is_visible) // Full display?
    {
        const char* name = "chatbox-filtering";
        // Draw filter button and input box in one line
        if (ImGui::Button(_LC("Console", "Filter options")))
        {
            ImGui::OpenPopup(name);
        }
        if (ImGui::BeginPopup(name))
        {
            m_console_view.DrawFilteringOptions();
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        if (!m_kb_focused)
        {
            ImGui::SetKeyboardFocusHere();
            m_kb_focused = true;
        }
        const ImGuiInputTextFlags cmd_flags = ImGuiInputTextFlags_EnterReturnsTrue;
        if (ImGui::InputText(_L("Message"), m_msg_buffer.GetBuffer(), m_msg_buffer.GetCapacity(), cmd_flags))
        {
            if (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
            {
                this->SubmitMessage();
            }
            m_msg_buffer.Clear();
            this->SetVisible(false);
        }
    }

    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();
    ImGui::PopStyleColor(2); // WindowBg, ChildWindowBg
}

void RoR::GUI::GameChatBox::SubmitMessage()
{
#ifdef USE_SOCKETW
    if (m_msg_buffer.IsEmpty())
    {
        return;
    }
    if (std::strncmp(m_msg_buffer.GetBuffer(), "/whisper", 8) == 0)
    {
        std::strtok(m_msg_buffer.GetBuffer(), " ");
        const char* username = std::strtok(nullptr, " ");
        const char* message = std::strtok(nullptr, " ");
        if (username != nullptr && message != nullptr)
        {
            RoRnet::UserInfo user;
            if (RoR::Networking::FindUserInfo(username, user))
            {
                RoR::Networking::WhisperChatMsg(user, message);
            }
            else
            {
                Str<200> text;
                text << _L("Whisper message not sent, unknown username") << ": " << username;
                App::GetConsole()->putMessage(
                    Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING, text.ToCStr());
            }
        }
        else
        {
            App::GetConsole()->putMessage(
                Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                _L("usage: /whisper username message"));
        }
    }
    else
    {
        RoR::Networking::BroadcastChatMsg(m_msg_buffer.GetBuffer());
    }
#endif // USE_SOCKETW
}

