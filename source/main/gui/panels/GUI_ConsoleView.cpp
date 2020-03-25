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

inline void color2i(ImVec4 v, int&r, int&g, int&b) { r=(int)(v.x*255); g=(int)(v.y*255); b=(int)(v.z*255); }

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
        ImVec2 text_size = ImGui::CalcTextSize("A");
        for (size_t i = m_display_list.size(); i < cvw_max_lines; ++i)
        {
            this->NewLine(text_size);
        }
    }

    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();
    for (const Console::Message* dm: m_display_list)
    {
        std::string line = dm->cm_text;
        RoRnet::UserInfo user;
        if (dm->cm_net_userid != 0 && RoR::Networking::GetAnyUserInfo((int)dm->cm_net_userid, user))
        {
            Ogre::ColourValue col = RoR::Networking::GetPlayerColor(user.colournum);
            char prefix[400] = {};
            int r,g,b;
            color2i(ImVec4(col.r, col.g, col.b, col.a), r,g,b);
            snprintf(prefix, 400, "#%02x%02x%02x%s: #000000", r, g, b, user.username);
            line = std::string(prefix) + line;
        }        

        switch (dm->cm_type)
        {
        case Console::Console::CONSOLE_TITLE:
            this->DrawColorMarkedText(theme.highlight_text_color, line);
            break;

        case Console::Console::CONSOLE_SYSTEM_ERROR:
            this->DrawColorMarkedText(theme.error_text_color, line);
            break;

        case Console::CONSOLE_SYSTEM_WARNING:
            this->DrawIcon(FetchIcon("error.png"), ImVec2(0.f, ImGui::GetTextLineHeight()));
            this->DrawColorMarkedText(theme.warning_text_color, line);
            break;

        case Console::Console::CONSOLE_SYSTEM_REPLY:
            this->DrawColorMarkedText(theme.success_text_color, line);
            break;

        case Console::Console::CONSOLE_HELP:
            this->DrawColorMarkedText(theme.help_text_color, line);
            break;

        case Console::Console::CONSOLE_SYSTEM_NOTICE:
            if (dm->cm_area == Console::MessageArea::CONSOLE_MSGTYPE_SCRIPT)
            {
                this->DrawIcon(FetchIcon("script.png"), ImVec2(0.f, ImGui::GetTextLineHeight()));
            }
            else
            {
                this->DrawIcon(FetchIcon("information.png"), ImVec2(0.f, ImGui::GetTextLineHeight()));
            }
            this->DrawColorMarkedText(ImGui::GetStyle().Colors[ImGuiCol_Text], line);
            break;

        case Console::Console::CONSOLE_SYSTEM_NETCHAT:
            this->DrawIcon(FetchIcon("comments.png"), ImVec2(0.f, ImGui::GetTextLineHeight()));
            this->DrawColorMarkedText(ImGui::GetStyle().Colors[ImGuiCol_Text], line);
            break;

        default:
            this->DrawColorMarkedText(ImGui::GetStyle().Colors[ImGuiCol_Text], line);
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
    ImGui::MenuItem(_LC("Console", "Commands"), "", &cvw_filter_type_cmd);
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
        (m.cm_type == Console::CONSOLE_HELP) && cvw_filter_type_cmd ||
        (m.cm_type == Console::CONSOLE_TITLE) && cvw_filter_type_cmd ||
        (m.cm_type == Console::CONSOLE_SYSTEM_REPLY) && cvw_filter_type_cmd ||
        (m.cm_type == Console::CONSOLE_SYSTEM_ERROR   && cvw_filter_type_error) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_WARNING && cvw_filter_type_warning) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_NOTICE  && cvw_filter_type_notice) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_NETCHAT && cvw_filter_type_chat);

    const bool time_ok =
        (cvw_filter_duration_ms == 0) ||
        m.cm_timestamp + cvw_filter_duration_ms >= App::GetConsole()->GetCurrentMsgTime();

    return type_ok && area_ok && time_ok;
}

void GUI::ConsoleView::DrawColorMarkedText(ImVec4 default_color, std::string const& line)
{
    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    drawlist->ChannelsSplit(2); // 2 layers
    ImVec2 bg_cursor = ImGui::GetCursorScreenPos();
    ImVec2 text_cursor = bg_cursor + cvw_background_padding;
    ImVec2 total_text_size;

    // Print colored line segments
    drawlist->ChannelsSetCurrent(1); // Text layer
    int r,g,b;
    color2i(default_color, r,g,b);
    std::smatch color_match;
    std::string::const_iterator seg_start = line.begin();
    while (std::regex_search(seg_start, line.end(), color_match, m_text_color_regex)) // Find next marker
    {
        // Print segment before the color marker (if any)
        std::string::const_iterator seg_end = color_match[0].first;
        if (seg_start != seg_end)
        {
            std::string text(seg_start, seg_end); // TODO: optimize!
            ImVec2 text_size = ImGui::CalcTextSize(text.c_str());           
            drawlist->AddText(text_cursor, ImColor(r,g,b), text.c_str());
            total_text_size.x += text_size.x;
            total_text_size.y = std::max(total_text_size.y, text_size.y);
            text_cursor.x += text_size.x;
        }
        // Prepare for printing segment after color marker
        sscanf(color_match.str(0).c_str(), "#%2x%2x%2x", &r, &g, &b);
        if (r==0 && g==0 && b==0)
        {
            color2i(default_color, r,g,b);
        }
        seg_start = color_match[0].second;
    }

    // Print final segment (if any)
    if (seg_start != line.begin() + line.length())
    {
        std::string text(seg_start, line.end()); // TODO: optimize!
        ImVec2 text_size = ImGui::CalcTextSize(text.c_str());            
        drawlist->AddText(text_cursor, ImColor(r,g,b), text.c_str());
        total_text_size.x += text_size.x;
        total_text_size.y = std::max(total_text_size.y, text_size.y);
    }

    // Draw background
    drawlist->ChannelsSetCurrent(0); // Background layer
    ImVec2 bg_rect_size = total_text_size + (cvw_background_padding * 2);
    drawlist->AddRectFilled(bg_cursor, bg_cursor + bg_rect_size,
        ImColor(cvw_background_color), ImGui::GetStyle().FrameRounding);

    // Finalize
    drawlist->ChannelsMerge();
    this->NewLine(total_text_size);
}

void GUI::ConsoleView::NewLine(ImVec2 text_size)
{
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (text_size + (cvw_background_padding * 2)).y + cvw_line_spacing);
}

Ogre::TexturePtr GUI::ConsoleView::FetchIcon(const char* name)
{
    try
    {
        return Ogre::static_pointer_cast<Ogre::Texture>(
            Ogre::TextureManager::getSingleton().createOrRetrieve(name, "IconsRG").first);
    }
    catch (...) {}

    return Ogre::TexturePtr(); // null
}

bool GUI::ConsoleView::DrawIcon(Ogre::TexturePtr tex, ImVec2 reference_box)
{
    if (!App::GetGuiManager()->IsVisible_Console())
    {
        ImGui::SetCursorPosX(10.f); // Give some room for icon
        if (tex)
        {
            ImGui::Image(reinterpret_cast<ImTextureID>(tex->getHandle()), ImVec2(16, 16));
        }
        ImGui::SameLine(); // Keep icon and text in the same line
    }
    return NULL;
}
