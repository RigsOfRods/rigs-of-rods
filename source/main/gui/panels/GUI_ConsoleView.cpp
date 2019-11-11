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

using namespace RoR;
using namespace Ogre;

void GUI::ConsoleView::DrawConsoleMessages()
{
    Console::MsgLockGuard lock(App::GetConsole());
    const size_t disp_count = std::min(MSG_DISP_LIMIT, lock.messages.size());
    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();
    auto disp_endi = lock.messages.end();
    auto disp_itor = disp_endi - disp_count;
    for (; disp_itor != disp_endi; ++disp_itor)
    {
        Console::Message const& m = *disp_itor;
        if (this->MessageFilter(m))
        {
            if (m.cm_net_userid != 0)
            {
                RoRnet::UserInfo user;
                RoR::Networking::GetUserInfo(m.cm_net_userid, user);
                Ogre::ColourValue col = RoR::Networking::GetPlayerColor(user.colournum);
                ImGui::TextColored(ImVec4(col.r, col.g, col.b, col.a), "%s: ", user.username);
                ImGui::SameLine();
            }

            switch (m.cm_type)
            {
            case Console::Console::CONSOLE_TITLE:
                ImGui::TextColored(theme.highlight_text_color, "%s", m.cm_text.c_str());
                break;

            case Console::Console::CONSOLE_SYSTEM_ERROR:
                ImGui::TextColored(theme.error_text_color, "%s", m.cm_text.c_str());
                break;

            case Console::CONSOLE_SYSTEM_WARNING:
                ImGui::TextColored(theme.warning_text_color, "%s", m.cm_text.c_str());
                break;

            case Console::Console::CONSOLE_SYSTEM_REPLY:
                ImGui::TextColored(theme.success_text_color, "%s", m.cm_text.c_str());
                break;

            case Console::Console::CONSOLE_HELP:
                ImGui::TextColored(theme.help_text_color, "%s", m.cm_text.c_str());
                break;

            default:
                ImGui::Text("%s", m.cm_text.c_str());
                break;
            }
        }
    }
}

void GUI::ConsoleView::DrawFilteringPopup(const char* name)
{
    if (ImGui::BeginPopup(name))
    {
        ImGui::TextDisabled(_LC("Console", "By area:"));
        ImGui::MenuItem(_LC("Console", "Logfile echo"), "", &m_filter_area_echo);
        ImGui::MenuItem(_LC("Console", "Scripting"),    "", &m_filter_area_script);
        ImGui::MenuItem(_LC("Console", "Actors"),       "", &m_filter_area_actor);
        ImGui::MenuItem(_LC("Console", "Terrain"),      "", &m_filter_area_terrn);

        ImGui::Separator();
        ImGui::TextDisabled(_LC("Console", "By level:"));
        ImGui::MenuItem(_LC("Console", "Notices"),  "", &m_filter_type_notice);
        ImGui::MenuItem(_LC("Console", "Warnings"), "", &m_filter_type_warning);
        ImGui::MenuItem(_LC("Console", "Errors"),   "", &m_filter_type_error);

        ImGui::EndPopup();
    }
}

bool GUI::ConsoleView::MessageFilter(Console::Message const& m)
{
    const bool area_ok = 
        (m.cm_area == Console::MessageArea::CONSOLE_MSGTYPE_INFO) ||
        (m.cm_area == Console::MessageArea::CONSOLE_MSGTYPE_LOG    && m_filter_area_echo) ||
        (m.cm_area == Console::MessageArea::CONSOLE_MSGTYPE_ACTOR  && m_filter_area_actor) ||
        (m.cm_area == Console::MessageArea::CONSOLE_MSGTYPE_TERRN  && m_filter_area_terrn) ||
        (m.cm_area == Console::MessageArea::CONSOLE_MSGTYPE_SCRIPT && m_filter_area_script);

    const bool type_ok =
        (m.cm_type == Console::CONSOLE_HELP) ||
        (m.cm_type == Console::CONSOLE_TITLE) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_REPLY) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_ERROR   && m_filter_type_error) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_WARNING && m_filter_type_warning) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_NOTICE  && m_filter_type_notice);

    const bool time_ok =
        m_filter_expired ||
        m.cm_timestamp <= Ogre::Root::getSingleton().getTimer()->getMilliseconds();

    return type_ok && area_ok && time_ok;
}

