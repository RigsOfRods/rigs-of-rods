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

#include "ChatSystem.h"

#include "Console.h"
#include "Language.h"

#include <cstring> // strtok, strncmp
#include <imgui.h>

RoR::GUI::GameChatBox::GameChatBox()
{
    m_console_view.cvw_align_bottom = true;
    m_console_view.cvw_max_lines = 20u;
    m_console_view.cvw_filter_duration_ms = 4000; // 4sec
}

void RoR::GUI::GameChatBox::Draw()
{
    // Begin drawing the window
    ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
    const ImVec2 size(
        ImGui::GetIO().DisplaySize.x - (2 * ImGui::GetStyle().WindowPadding.x),
        ((m_console_view.cvw_max_lines + 1) * ImGui::GetTextLineHeightWithSpacing()) + (2*ImGui::GetStyle().WindowPadding.y));
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - size.y));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0)); // Fully transparent background!
    ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImVec4(0,0,0,0)); // Fully transparent background!
    if (!ImGui::Begin("Chat", nullptr, win_flags))
    {
        return;
    }

    m_console_view.DrawConsoleMessages();

    // Draw filter button and input box in one line
    if (ImGui::Button(_LC("Console", "Filter options")))
    {
        ImGui::OpenPopup("chatbox-filtering");
    }
    m_console_view.DrawFilteringPopup("chatbox-filtering");
    ImGui::SameLine();
    const ImGuiInputTextFlags cmd_flags = ImGuiInputTextFlags_EnterReturnsTrue;
    if (ImGui::InputText(_L("Message"), m_msg_buffer.GetBuffer(), m_msg_buffer.GetCapacity(), cmd_flags))
    {
        if (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
        {
            this->SubmitMessage();
        }
        m_msg_buffer.Clear();
    }

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

