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

#include "Actor.h"
#include "Application.h"
#include "ChatSystem.h"
#include "Console.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "Language.h"
#include "InputEngine.h"

#include <cstring> // strtok, strncmp
#include <fmt/format.h>
#include <imgui.h>

using namespace RoR;
using namespace GUI;

GameChatBox::GameChatBox()
{
    m_console_view.cvw_filter_area_actor = false; // Disable vehicle spawn warnings/errors
    m_console_view.cvw_filter_type_error = true; // Enable errors
    m_console_view.cvw_filter_type_cmd = false; // Disable commands
    m_console_view.cvw_enable_icons = true;
    m_console_view.cvw_background_padding = ImVec2(2,1);
}

void GameChatBox::Draw()
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
    ImVec2 msg_pos(theme.screen_edge_padding.x, ImGui::GetIO().DisplaySize.y - (msg_size.y + theme.screen_edge_padding.y));
    if (m_is_visible)
    {
        msg_pos.y -= chat_size.y;
        msg_size.y -= 6.f; // prevents partially bottom chat messages
    }
    else
    {
        msg_flags |= ImGuiWindowFlags_NoInputs;
    }
    ImGui::SetNextWindowPos(msg_pos);
    ImGui::SetNextWindowSize(msg_size);

    ImGui::Begin("ChatMessages", nullptr, msg_flags);

    if (initialized)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                                      fmt::format(_LC("ChatBox", "Press {} to spawn a vehicle"),
                   App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_GET_NEW_VEHICLE)), "lightbulb.png");
        initialized = false;
    }

    if (m_is_visible)
    {
        m_console_view.cvw_enable_scrolling = true;
        m_console_view.cvw_smooth_scrolling = false;
        m_console_view.cvw_msg_duration_ms = 3600000; // 1hour
        m_console_view.cvw_filter_type_notice = false;
        m_console_view.cvw_filter_type_warning = false; 
        m_console_view.cvw_filter_type_error = false;
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
        m_console_view.cvw_smooth_scrolling = true;
        m_console_view.cvw_filter_type_notice = true;
        m_console_view.cvw_filter_type_warning = true; 
        m_console_view.cvw_filter_type_error = true;
        m_console_view.cvw_filter_area_script = true; 
        m_console_view.cvw_msg_duration_ms = 10000; // 10sec
        if (init_scroll == true)
        {
            m_console_view.RequestReloadMessages();
            init_scroll = false;
        }
    }

    if (!App::mp_chat_auto_hide->getBool())
    {
        m_console_view.cvw_msg_duration_ms = 2629800000; // 1month, should be enough
    }

    m_console_view.DrawConsoleMessages();

    ImGui::End();

    // Draw chat box
    ImGuiWindowFlags chat_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (!m_is_visible)
    {
        chat_flags |= ImGuiWindowFlags_NoInputs;
    }

    ImGui::SetNextWindowSize(chat_size);
    ImGui::SetNextWindowPos(ImVec2(theme.screen_edge_padding.x, ImGui::GetIO().DisplaySize.y - (chat_size.y + theme.screen_edge_padding.y)));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0.0,0,0)); // Fully transparent background!
    ImGui::Begin("ChatBottomBar", nullptr, chat_flags);

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
    {
        this->SetVisible(false);
    }

    if (m_is_visible) // Full display?
    {
        ImGui::Text("%s", _LC("ChatBox", "Chat history (use mouse wheel to scroll)"));
        ImGui::Text("%s", _LC("ChatBox", "Message"));
        ImGui::SameLine();
        if (!m_kb_focused)
        {
            ImGui::SetKeyboardFocusHere();
            m_kb_focused = true;
        }
        if (m_scheduled_move_textcursor_to_end && ImMoveTextInputCursorToEnd("##chatbox"))
        {
            m_scheduled_move_textcursor_to_end = false;
        }
        const ImGuiInputTextFlags cmd_flags = ImGuiInputTextFlags_EnterReturnsTrue;
        if (ImGui::InputText("##chatbox", m_msg_buffer.GetBuffer(), m_msg_buffer.GetCapacity(), cmd_flags))
        {
            if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
            {
                this->SubmitMessage();
            }
            m_msg_buffer.Clear();
            this->SetVisible(false);
        }
    }

    ImGui::End();

    ImGui::PopStyleColor(2); // 2*WindowBg
    ImGui::PopStyleVar(); // WindowPadding
}

void GameChatBox::SubmitMessage()
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
            if (App::GetNetwork()->FindUserInfo(username, user))
            {
                App::GetNetwork()->WhisperChatMsg(user, message);
            }
            else
            {
                App::GetConsole()->putMessage(
                    Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format(_LC("ChatBox", "Whisper message not sent, unknown username: {}"), username));
            }
        }
        else
        {
            App::GetConsole()->putMessage(
                Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                _LC("ChatBox", "usage: /whisper username message"));
        }
    }
    else
    {
        App::GetNetwork()->BroadcastChatMsg(m_msg_buffer.GetBuffer());
    }
#endif // USE_SOCKETW
}

