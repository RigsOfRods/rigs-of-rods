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

#include "GameContext.h"
#include "GUIManager.h"

#include "Language.h"

using namespace RoR;
using namespace GUI;
using namespace Ogre;

ConsoleWindow::ConsoleWindow()
{
    m_console_view.cvw_enable_scrolling = true;
}

void ConsoleWindow::Draw()
{
    ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar;
    ImGui::SetNextWindowPosCenter(ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2((ImGui::GetIO().DisplaySize.x / 1.6), (ImGui::GetIO().DisplaySize.y / 1.3)), ImGuiCond_FirstUseEver);
    bool keep_open = true;
    ImGui::Begin("Console", &keep_open, win_flags);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu(_LC("Console", "Filter options")))
        {
            m_console_view.DrawFilteringOptions();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(_LC("Console", "Commands")))
        {
            ImGui::Dummy(ImVec2(550.f, 1.f)); // Manually resize width (DearIMGUI bug workaround)
            ImGui::Columns(3);
            ImGui::SetColumnWidth(0, 100); // TODO: Calculate dynamically
            ImGui::SetColumnWidth(1, 170); // TODO: Calculate dynamically

            for (auto& cmd_pair: App::GetConsole()->GetCommands())
            {
                if (ImGui::Selectable(cmd_pair.second->GetName().c_str()))
                {
                    cmd_pair.second->Run(Ogre::StringVector{cmd_pair.second->GetName()});
                }
                ImGui::NextColumn();
                ImGui::Text("%s", cmd_pair.second->GetUsage().c_str());
                ImGui::NextColumn();
                ImGui::Text("%s", cmd_pair.second->GetDoc().c_str());
                ImGui::NextColumn();
            }

            ImGui::Columns(1); // reset
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(_LC("Console", "Addons")))
        {
            this->DrawAddonSelector();
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    const float footer_height_to_reserve = ImGui::GetFrameHeightWithSpacing(); // 1 input text
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1 InputText

    m_console_view.DrawConsoleMessages();

    ImGui::EndChild();

    const ImGuiInputTextFlags cmd_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory;
    if (ImGui::InputText(_LC("Console", "Command"), m_cmd_buffer.GetBuffer(), m_cmd_buffer.GetCapacity(), cmd_flags, &ConsoleWindow::TextEditCallback, this))
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

void ConsoleWindow::DoCommand(std::string msg) // All commands are processed here
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

int ConsoleWindow::TextEditCallback(ImGuiTextEditCallbackData *data)
{
    ConsoleWindow* c = static_cast<ConsoleWindow*>(data->UserData);
    c->TextEditCallbackProc(data);
    return 0;
}

void ConsoleWindow::TextEditCallbackProc(ImGuiTextEditCallbackData *data)
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

void ConsoleWindow::DrawAddonSelector()
{
    // Initial refresh
    if (!m_addons_refreshed)
    {
        m_addons_query.cqy_filter_type = LT_Addon;
        App::GetCacheSystem()->Query(m_addons_query);
        m_addons_refreshed = true;
    }

    // Scroll area
    ImGui::BeginChild("scrolling", ImVec2(400.f, 150.f), false);

    // Addons
    for (int i = 0; i < (int)m_addons_query.cqy_results.size(); i++)
    {
        ImGui::PushID(i);
        CacheEntry* entry = m_addons_query.cqy_results[i].cqr_entry;

        if (ImGui::CollapsingHeader(entry->dname.c_str()))
        {
            ImGui::TextDisabled("%s", entry->resource_bundle_path.c_str());

            // 2nd column - actions
            if (ImGui::Button(_LC("AddonSetup", "Load")))
            {
                //TODO App::GetGameContext()->PushMessage(Message(MSG_APP_LOAD_ADDON_REQUESTED, (void*)entry));
            }
            ImGui::SameLine();
            bool autoload = false; // TODO
            if (ImGui::Checkbox(_LC("AddonSetup", "AutoLoad"), &autoload))
            {
                // MSG_APP_SETUP_ADDON_REQUESTED
            }
        }

        ImGui::PopID();
    }
    ImGui::Columns(1);
    ImGui::EndChild(); // End of scroll area
}
