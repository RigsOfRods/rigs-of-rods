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


#include "GUI_ConsoleView.h"

#include "Application.h"
#include "Console.h"
#include "GUIManager.h"
#include "Language.h"
#include "Network.h"

#include <algorithm> // min
#include <stdio.h> // sscanf

using namespace RoR;
using namespace Ogre;

void GUI::ConsoleView::DrawConsoleMessages()
{
    m_display_list.clear();
    m_newest_msg_time = 0;

    { // Lock scope
        Console::MsgLockGuard lock(App::GetConsole()); // RAII: Scoped lock
        const size_t disp_max = std::min(cvw_max_lines, lock.messages.size());

        for (Console::Message const& m: lock.messages)
        {
            if (this->MessageFilter(m))
            {
                m_display_list.push_back(&m);
                if (m.cm_timestamp > m_newest_msg_time)
                {
                    m_newest_msg_time = (unsigned long)m.cm_timestamp;
                }
            }
        }
    } // End lock scope

    if (cvw_align_bottom)
    {
        for (size_t i = m_display_list.size(); i < cvw_max_lines; ++i)
        {
            ImGui::NewLine();
        }
    }

    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();
    for (const Console::Message* dm: m_display_list)
    {
        if (dm->cm_net_userid != 0)
        {
            RoRnet::UserInfo user;
            RoR::Networking::GetUserInfo(dm->cm_net_userid, user);
            Ogre::ColourValue col = RoR::Networking::GetPlayerColor(user.colournum);
            ImGui::TextColored(ImVec4(col.r, col.g, col.b, col.a), "%s: ", user.username);
            ImGui::SameLine();
        }

        switch (dm->cm_type)
        {
        case Console::Console::CONSOLE_TITLE:
            this->DrawColorMarkedText(theme.highlight_text_color, dm->cm_text);
            break;

        case Console::Console::CONSOLE_SYSTEM_ERROR:
            this->DrawColorMarkedText(theme.error_text_color, dm->cm_text);
            break;

        case Console::CONSOLE_SYSTEM_WARNING:
            this->DrawColorMarkedText(theme.warning_text_color, dm->cm_text);
            break;

        case Console::Console::CONSOLE_SYSTEM_REPLY:
            this->DrawColorMarkedText(theme.success_text_color, dm->cm_text);
            break;

        case Console::Console::CONSOLE_HELP:
            this->DrawColorMarkedText(theme.help_text_color, dm->cm_text);
            break;

        default:
            this->DrawColorMarkedText(ImGui::GetStyle().Colors[ImGuiCol_Text], dm->cm_text);
            break;
        }
    }
}

void GUI::ConsoleView::DrawFilteringOptions()
{
    ImGui::TextDisabled(_LC("Console", "By area:"));
    ImGui::MenuItem(_LC("Console", "Logfile echo"), "", &cvw_filter_area_echo);
    ImGui::MenuItem(_LC("Console", "Scripting"),    "", &cvw_filter_area_script);
    ImGui::MenuItem(_LC("Console", "Actors"),       "", &cvw_filter_area_actor);
    ImGui::MenuItem(_LC("Console", "Terrain"),      "", &cvw_filter_area_terrn);

    ImGui::Separator();
    ImGui::TextDisabled(_LC("Console", "By level:"));
    ImGui::MenuItem(_LC("Console", "Notices"),  "", &cvw_filter_type_notice);
    ImGui::MenuItem(_LC("Console", "Warnings"), "", &cvw_filter_type_warning);
    ImGui::MenuItem(_LC("Console", "Errors"),   "", &cvw_filter_type_error);
    ImGui::MenuItem(_LC("Console", "Net chat"), "", &cvw_filter_type_chat);
}

bool GUI::ConsoleView::MessageFilter(Console::Message const& m)
{
    const bool area_ok =
        (m.cm_area == Console::MessageArea::CONSOLE_MSGTYPE_INFO) ||
        (m.cm_area == Console::MessageArea::CONSOLE_MSGTYPE_LOG    && cvw_filter_area_echo) ||
        (m.cm_area == Console::MessageArea::CONSOLE_MSGTYPE_ACTOR  && cvw_filter_area_actor) ||
        (m.cm_area == Console::MessageArea::CONSOLE_MSGTYPE_TERRN  && cvw_filter_area_terrn) ||
        (m.cm_area == Console::MessageArea::CONSOLE_MSGTYPE_SCRIPT && cvw_filter_area_script);

    const bool type_ok =
        (m.cm_type == Console::CONSOLE_HELP) ||
        (m.cm_type == Console::CONSOLE_TITLE) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_REPLY) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_ERROR   && cvw_filter_type_error) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_WARNING && cvw_filter_type_warning) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_NOTICE  && cvw_filter_type_notice) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_NETCHAT && cvw_filter_type_chat);

    const bool time_ok =
        (cvw_filter_duration_ms == 0) ||
        m.cm_timestamp + cvw_filter_duration_ms >= App::GetConsole()->GetCurrentMsgTime();

    return type_ok && area_ok && time_ok;
}

inline void color2i(ImVec4 v, int&r, int&g, int&b) { r=(int)(v.x*255); g=(int)(v.y*255); b=(int)(v.z*255); }

void GUI::ConsoleView::DrawColorMarkedText(ImVec4 default_color, std::string const& line)
{
    // Print colored line segments
    int r,g,b;
    color2i(default_color, r,g,b);
    std::smatch color_match;
    std::string::const_iterator seg_start = line.begin();
    bool first = true;
    while (std::regex_search(seg_start, line.end(), color_match, m_text_color_regex)) // Find next marker
    {
        // Print segment before the color marker (if any)
        std::string::const_iterator seg_end = color_match[0].first;
        if (seg_start != seg_end)
        {
            if (!first)
            {
                ImGui::SameLine();
            }
            ImGui::TextColored(ImColor(r,g,b), "%s", std::string(seg_start, seg_end).c_str()); // TODO: optimize!
            first = false;
        }
        // Prepare for printing segment after color marker
        sscanf(color_match.str(0).c_str(), "#%2x%2x%2x", &r, &g, &b);
        if (r==0 && g==0 && b==0)
        {
            color2i(default_color, r,g,b);
        }
        seg_start = color_match[0].second;
    } // while ()

    // Print final segment (if any)
    if (seg_start != line.begin() + line.length())
    {
        if (!first)
        {
            ImGui::SameLine();
        }
        ImGui::TextColored(ImColor(r,g,b), "%s", std::string(seg_start, line.end()).c_str()); // TODO: optimize!
    }
}

