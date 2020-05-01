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
    m_console_view.cvw_filter_area_actor = false; // Disable vehicle spawn warnings/errors
    m_console_view.cvw_filter_type_error = false; // Disable errors
    m_console_view.cvw_filter_type_cmd = false; // Disable commands
    m_console_view.cvw_enable_icons = true;
    m_console_view.cvw_background_padding = ImVec2(2,1);
}

void RoR::GUI::GameChatBox::Draw()
{
    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0)); // Fully transparent background!
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));

    // Begin drawing the messages pane (no input)
    ImGuiWindowFlags msg_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
    const float width = ImGui::GetIO().DisplaySize.x - (2 * theme.screen_edge_padding.x);
    ImVec2 msg_size(width, (ImGui::GetIO().DisplaySize.y / 3.f) + (2*ImGui::GetStyle().WindowPadding.y));
    ImVec2 chat_size(width, ImGui::GetTextLineHeightWithSpacing() + 20);
    ImGui::SetNextWindowSize(msg_size);
    ImVec2 msg_pos(theme.screen_edge_padding.x, ImGui::GetIO().DisplaySize.y - (msg_size.y + theme.screen_edge_padding.y));
    if (m_is_visible)
    {
        msg_pos.y -= chat_size.y;
    }
    ImGui::SetNextWindowPos(msg_pos);

    if (m_is_visible)
    {
        ImGui::Begin("ChatMessages", nullptr, msg_flags);
    }
    else
    {
        ImGui::Begin("ChatMessages", nullptr, msg_flags | ImGuiWindowFlags_NoInputs);
    }

    if (initialized == true)
    {
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, "Welcome to Rigs of Rods!", "", 10000, false);
        initialized = false;
    }

    if (m_is_visible)
    {
        m_console_view.cvw_enable_scrolling = true;
        m_console_view.cvw_msg_duration_ms = 3600000; // 1hour
        m_console_view.cvw_filter_type_notice = false;
        m_console_view.cvw_filter_type_warning = false; 
        m_console_view.cvw_filter_area_script = false; 
        if (init_scroll == false) // Initialize auto scrolling
        {
            m_console_view.RequestReloadMessages();
            ImGui::SetScrollFromPosY(9999); // Force to bottom once
            init_scroll = true;
        }
    }
    else
    {
        m_console_view.cvw_enable_scrolling = false;
        m_console_view.cvw_filter_type_notice = true;
        m_console_view.cvw_filter_type_warning = true; 
        m_console_view.cvw_filter_area_script = true; 
        m_console_view.cvw_msg_duration_ms = 10000; // 10sec
        if (init_scroll == true)
        {
            m_console_view.RequestReloadMessages();
            init_scroll = false;
        }
    }

    m_console_view.DrawConsoleMessages();

    ImGui::End();

    // Draw chat box
    ImGuiWindowFlags chat_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;

    ImGui::SetNextWindowSize(chat_size);
    ImGui::SetNextWindowPos(ImVec2(theme.screen_edge_padding.x, ImGui::GetIO().DisplaySize.y - (chat_size.y + theme.screen_edge_padding.y)));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0.0,0,0)); // Fully transparent background!
    ImGui::Begin("ChatBottomBar", nullptr, chat_flags);

    if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
    {
        this->SetVisible(false);
    }

    if (m_is_visible) // Full display?
    {
        ImGui::Text(_L("Chat history (use mouse wheel to scroll)"));
        ImGui::Text(_L("Message"));
        ImGui::SameLine();
        if (!m_kb_focused)
        {
            ImGui::SetKeyboardFocusHere();
            m_kb_focused = true;
        }
        const ImGuiInputTextFlags cmd_flags = ImGuiInputTextFlags_EnterReturnsTrue;
        if (ImGui::InputText("", m_msg_buffer.GetBuffer(), m_msg_buffer.GetCapacity(), cmd_flags))
        {
            if (RoR::App::mp_state->GetActiveEnum<MpState>() == RoR::MpState::CONNECTED)
            {
                this->SubmitMessage();
            }
            m_msg_buffer.Clear();
            this->SetVisible(false);
        }
    }

    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();

    ImGui::PopStyleColor(2); // 2*WindowBg
    ImGui::PopStyleVar(); // WindowPadding
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

