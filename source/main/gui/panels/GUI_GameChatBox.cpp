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

#include "GUI_GameChatBox.h"

#include "ChatSystem.h"
#include "Language.h"

#include <cstring> // strtok, strncmp
#include <imgui.h>

void RoR::GUI::GameChatBox::pushMsg(std::string const& txt)
{
    std::lock_guard<std::mutex> lock(m_messages_mutex);
    m_messages.push_back(txt);
    m_message_added = true;
    m_disp_state = DispState::VISIBLE_STALE;
    if (m_messages.size() > MESSAGES_CAP)
    {
        m_messages.erase(m_messages.begin());
    }
}

void RoR::GUI::GameChatBox::Draw()
{
    // Calculate opacity (fade-out)
    const size_t time = static_cast<size_t>(Ogre::Root::getSingleton().getTimer()->getMilliseconds());
    if (m_message_added)
    {
        m_message_added = false;
        m_message_time = time;
    }

    float alpha = 1.f;
    if (m_disp_state == DispState::VISIBLE_STALE)
    {
        if (time > m_message_time + FADEOUT_DELAY_MS + FADEOUT_DURATION_MS)
        {
            m_disp_state = DispState::HIDDEN;
            return;
        }
        else if (time > m_message_time + FADEOUT_DELAY_MS)
        {
            const size_t fadeout_elapsed_ms = time - (m_message_time + FADEOUT_DELAY_MS);
            alpha -= (static_cast<float>(fadeout_elapsed_ms)/static_cast<float>(FADEOUT_DURATION_MS));
        }
    }

    // Begin drawing the window
    ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;
    const ImVec2 size(500.f, 300.f);
    const ImVec2 pos(0, ImGui::GetIO().DisplaySize.y - size.y);
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowPos(pos);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0)); // Fully transparent background!
    ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImVec4(0.4f, 0.4f, 0.4f, 0.4f)); // Semi-transparent reading pane
    if (!ImGui::Begin("Chat", nullptr, win_flags))
    {
        return;
    }

    // Draw messages
    const float footer_height_to_reserve = ImGui::GetTextLineHeightWithSpacing(); // 1 input text
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1 InputText

    { // Lock scope
        std::lock_guard<std::mutex> lock(m_messages_mutex);
        for (std::string const& line: m_messages)
        {
            ImGui::Text("%s", line.c_str());
        }
    } // Lock scope

    ImGui::EndChild();

    // Draw input box
    if (m_disp_state == DispState::VISIBLE_FRESH)
    {
        ImGui::SetKeyboardFocusHere();
    }

    const ImGuiInputTextFlags cmd_flags = ImGuiInputTextFlags_EnterReturnsTrue;
    if (ImGui::InputText(_L("Message"), m_msg_buffer.GetBuffer(), m_msg_buffer.GetCapacity(), cmd_flags))
    {
        if (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
        {
            this->SubmitMessage();
        }
        m_msg_buffer.Clear();
        m_message_time = time;
        m_disp_state = DispState::VISIBLE_STALE;
    }

    if (m_disp_state == DispState::VISIBLE_FRESH)
    {
        // We just called `ImGui::SetKeyboardFocusHere()`, `ImGui::IsItemActive()` won't return `true` until next frame! 
        m_disp_state = DispState::VISIBLE_FOCUSED;
    }
    else
    {
        m_disp_state = (ImGui::IsItemActive()) ? // Does the input box have keyboard focus?
            DispState::VISIBLE_FOCUSED : DispState::VISIBLE_STALE;
    }

    ImGui::End();
    ImGui::PopStyleVar(1); // Alpha
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
            RoR::ChatSystem::SendPrivateChat(username, message);
        }
        else
        {
            this->pushMsg(_L("usage: /whisper username message"));
        }
    }
    else
    {
        RoR::ChatSystem::SendChat(m_msg_buffer.GetBuffer());
    }
#endif // USE_SOCKETW
}

void RoR::GUI::GameChatBox::SetVisible(bool v)
{
    m_disp_state = (v) ? DispState::VISIBLE_FRESH : DispState::HIDDEN;
}

