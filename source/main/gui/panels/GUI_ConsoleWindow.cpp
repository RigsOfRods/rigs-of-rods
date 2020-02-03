/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2014 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/


#include "GUI_ConsoleWindow.h"
#include "GUIManager.h"

#include "Language.h"

using namespace RoR;
using namespace Ogre;

void GUI::ConsoleWindow::Draw()
{
    ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar;
    ImGui::SetNextWindowPosCenter();
    ImGui::SetNextWindowSize(ImVec2(500.f, 550.f), ImGuiSetCond_FirstUseEver);
    bool keep_open = true;
    ImGui::Begin("Console", &keep_open, win_flags);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu(_LC("Console", "Filter options")))
        {
            this->DrawFilteringOptions();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(_LC("Console", "Commands")))
        {
            if (ImGui::MenuItem("help"))    { this->DoCommand("help"); }
            if (ImGui::MenuItem("ver"))     { this->DoCommand("ver"); }
            if (ImGui::MenuItem("pos"))     { this->DoCommand("pos"); }
            if (ImGui::MenuItem("gravity")) { this->DoCommand("gravity"); }
            if (ImGui::MenuItem("quit"))    { this->DoCommand("quit"); }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetTextLineHeightWithSpacing(); // 1 separator, 1 input text
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1 InputText

    this->DrawConsoleMessages();

    ImGui::EndChild();
    ImGui::Separator();

    const ImGuiInputTextFlags cmd_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory;
    if (ImGui::InputText(_LC("Console", "Command"), m_cmd_buffer.GetBuffer(), m_cmd_buffer.GetCapacity(), cmd_flags, &GUI::ConsoleWindow::TextEditCallback, this))
    {
        this->DoCommand(m_cmd_buffer.ToCStr());
        m_cmd_buffer.Clear();
    }

    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();

    if (!keep_open)
    {
        this->SetVisible(false);
    }
}

void GUI::ConsoleWindow::DoCommand(std::string msg) // All commands are processed here
{
    Ogre::StringUtil::trim(msg);
    if (msg.empty())
    {
        // discard the empty message
        return;
    }

    m_cmd_history.push_back(msg);
    if (m_cmd_history.size() > HISTORY_CAP)
    {
        m_cmd_history.erase(m_cmd_history.begin());
    }
    m_cmd_history_cursor = -1;

    App::GetConsole()->DoCommand(msg);
}

int GUI::ConsoleWindow::TextEditCallback(ImGuiTextEditCallbackData *data)
{
    ConsoleWindow* c = static_cast<ConsoleWindow*>(data->UserData);
    c->TextEditCallbackProc(data);
    return 0;
}

void GUI::ConsoleWindow::TextEditCallbackProc(ImGuiTextEditCallbackData *data)
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
    {
        const int prev_cursor = m_cmd_history_cursor;
        if (data->EventKey == ImGuiKey_UpArrow)
        {
            if (m_cmd_history_cursor == -1)
            {
                m_cmd_history_cursor = static_cast<int>(m_cmd_history.size()) - 1;
            }
            else if (m_cmd_history_cursor > 0)
            {
                m_cmd_history_cursor--;
            }
        }
        else if (data->EventKey == ImGuiKey_DownArrow)
        {
            if (m_cmd_history_cursor != -1)
            {
                ++m_cmd_history_cursor;

                if (m_cmd_history_cursor >= static_cast<int>(m_cmd_history.size()))
                {
                    m_cmd_history_cursor = -1;
                }
            }
        }

        if (m_cmd_history_cursor != prev_cursor)
        {
            const char* text = (m_cmd_history_cursor >= 0) ? m_cmd_history.at(m_cmd_history_cursor).c_str() : "";
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, text);
        }
    }
}

