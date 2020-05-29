/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

static const int LINE_BUF_MAX = 5000;

void GUI::ConsoleView::DrawConsoleMessages()
{
    // Update pre-filtered message list
    int num_incoming = this->UpdateMessages();

    // Gather visible (non-expired) messages
    const unsigned long curr_timestamp = App::GetConsole()->QueryMessageTimer();
    m_display_messages.clear();
    for (Console::Message& m: m_filtered_messages)
    {
        if (cvw_msg_duration_ms == 0 || curr_timestamp <= m.cm_timestamp + cvw_msg_duration_ms)
        {
            m_display_messages.push_back(&m);
        }
    }

    // Prepare drawing
    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    drawlist->ChannelsSplit(2); // 2 layers: 0=background, 1=text

    if (!cvw_enable_scrolling)
    {
        // Draw from bottom, messages may be multiline
        ImVec2 cursor = ImGui::GetWindowPos() + ImVec2(0, ImGui::GetWindowHeight());
        for (int i = m_display_messages.size() - 1; i >= 0; --i)
        {
            Console::Message const& m = *m_display_messages[i];
            float msg_h = ImGui::CalcTextSize(m.cm_text.c_str()).y + (2 * cvw_background_padding.y) + cvw_line_spacing;
            cursor -= ImVec2(0, msg_h);
            if (cursor.y < ImGui::GetWindowPos().y)
            {
                break; // Window is filled up
            }
            this->DrawMessage(cursor, m);
        }
    }
    else
    {
        // Add a dummy sized as messages to facilitate builtin scrolling
        float line_h = ImGui::CalcTextSize("").y + (2 * cvw_background_padding.y) + cvw_line_spacing;
        float dummy_h = line_h * (float)m_display_messages.size();
        ImGui::SetCursorPosY(dummy_h);

        // Autoscroll
        if (num_incoming != 0 && std::abs(ImGui::GetScrollMaxY() - ImGui::GetScrollY()) < line_h)
        {
            // it's evaluated on next frame, so we must exxagerate in advance.
            ImGui::SetScrollY(ImGui::GetScrollMaxY() + (num_incoming * line_h) + 100.f);
        }

        // Calculate message range and cursor pos
        int msg_start = 0, msg_count = 0;
        ImVec2 cursor = ImGui::GetWindowPos();
        if (ImGui::GetScrollMaxY() < 0) // No scrolling
        {
            msg_count = (int)m_display_messages.size();
            cursor += ImVec2(0, ImGui::GetWindowHeight() - (msg_count * line_h)); // Align to bottom
        }
        else // Scrolling
        {
            const float scroll_rel = ImGui::GetScrollY()/ImGui::GetScrollMaxY();
            const float scroll_offset = ((dummy_h - ImGui::GetWindowHeight()) *scroll_rel);
            msg_start = std::max(0, (int)(scroll_offset/line_h));

            msg_count = std::min((int)(ImGui::GetWindowHeight() / line_h)+2, // Bias (2) for partially visible messages (1 top, 1 bottom)
                                 (int)m_display_messages.size() - msg_start);

            const float line_offset = scroll_offset/line_h;
            cursor -= ImVec2(0, (line_offset - (float)(int)line_offset)*line_h);
        }

        // Horizontal scrolling
        if (ImGui::GetScrollMaxX() > 0)
        {
            cursor -= ImVec2(ImGui::GetScrollX(), 0);
        }

        // Draw the messages
        for (int i = msg_start; i < msg_start + msg_count; i++)
        {
            const Console::Message& m = *m_display_messages[i];

            ImVec2 text_size = this->DrawMessage(cursor, m);
            ImGui::SetCursorPosX(text_size.x); // Enable horizontal scrolling

            cursor += ImVec2(0.f, line_h);
        }
    }

    // Finalize drawing
    drawlist->ChannelsMerge();
}

ImVec2 GUI::ConsoleView::DrawMessage(ImVec2 cursor, Console::Message const& m)
{
    Str<LINE_BUF_MAX> line;
    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();

    const unsigned long curr_timestamp = App::GetConsole()->QueryMessageTimer();
    unsigned long overtime = curr_timestamp - (m.cm_timestamp + (cvw_msg_duration_ms - fadeout_interval));

    if (overtime > fadeout_interval)
    {
        alpha = 1.f;
    }
    else
    {
        alpha -= (float)overtime/(float)fadeout_interval;
    }

    // Draw icons based on filters
    Ogre::TexturePtr icon;
    if (cvw_enable_icons)
    {
        if (m.cm_area == Console::MessageArea::CONSOLE_MSGTYPE_SCRIPT)
        {
            icon = Ogre::TextureManager::getSingleton().load("script.png", "IconsRG");
        }
        else if (m.cm_type == Console::CONSOLE_SYSTEM_NOTICE)
        {
            icon = Ogre::TextureManager::getSingleton().load("information.png", "IconsRG");
        }
        else if (m.cm_type == Console::CONSOLE_SYSTEM_WARNING)
        {
            icon = Ogre::TextureManager::getSingleton().load("error.png", "IconsRG");
        }
        else if (m.cm_type == Console::CONSOLE_SYSTEM_NETCHAT)
        {
            icon = Ogre::TextureManager::getSingleton().load("comment.png", "IconsRG");
        }
    }

    // Add colored multiplayer username
    RoRnet::UserInfo user;
    if (m.cm_net_userid != 0 && RoR::Networking::GetAnyUserInfo((int)m.cm_net_userid, user))
    {
        Ogre::ColourValue col = RoR::Networking::GetPlayerColor(user.colournum);
        char prefix[400] = {};
        int r,g,b;
        color2i(ImVec4(col.r, col.g, col.b, col.a), r,g,b);
        snprintf(prefix, 400, "#%02x%02x%02x%s: #000000", r, g, b, user.username);

        line.Clear();
        line << prefix << m.cm_text;
    }
    else
    {
        line = m.cm_text;
    }

    // Colorize text by type
    ImVec4 base_color = ImGui::GetStyle().Colors[ImGuiCol_Text];
    switch (m.cm_type)
    {
    case Console::CONSOLE_TITLE:          base_color = theme.highlight_text_color;  break;
    case Console::CONSOLE_SYSTEM_ERROR:   base_color = theme.error_text_color;      break;
    case Console::CONSOLE_SYSTEM_WARNING: base_color = theme.warning_text_color;    break;
    case Console::CONSOLE_SYSTEM_REPLY:   base_color = theme.success_text_color;    break;
    case Console::CONSOLE_HELP:           base_color = theme.help_text_color;       break;
    default:;
    }

    return this->DrawColorMarkedText(cursor, icon, base_color, line.ToCStr());
}

void GUI::ConsoleView::DrawFilteringOptions()
{
    ImGui::TextDisabled(_LC("Console", "By area:"));
    m_reload_messages |= ImGui::MenuItem(_LC("Console", "Logfile echo"), "", &cvw_filter_area_echo);
    m_reload_messages |= ImGui::MenuItem(_LC("Console", "Scripting"),    "", &cvw_filter_area_script);
    m_reload_messages |= ImGui::MenuItem(_LC("Console", "Actors"),       "", &cvw_filter_area_actor);
    m_reload_messages |= ImGui::MenuItem(_LC("Console", "Terrain"),      "", &cvw_filter_area_terrn);

    ImGui::Separator();
    ImGui::TextDisabled(_LC("Console", "By level:"));
    m_reload_messages |= ImGui::MenuItem(_LC("Console", "Notices"),  "", &cvw_filter_type_notice);
    m_reload_messages |= ImGui::MenuItem(_LC("Console", "Warnings"), "", &cvw_filter_type_warning);
    m_reload_messages |= ImGui::MenuItem(_LC("Console", "Errors"),   "", &cvw_filter_type_error);
    m_reload_messages |= ImGui::MenuItem(_LC("Console", "Net chat"), "", &cvw_filter_type_chat);
    m_reload_messages |= ImGui::MenuItem(_LC("Console", "Commands"), "", &cvw_filter_type_cmd);
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
        (m.cm_type == Console::CONSOLE_HELP           && cvw_filter_type_cmd) ||
        (m.cm_type == Console::CONSOLE_TITLE          && cvw_filter_type_cmd) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_REPLY   && cvw_filter_type_cmd) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_ERROR   && cvw_filter_type_error) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_WARNING && cvw_filter_type_warning) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_NOTICE  && cvw_filter_type_notice) ||
        (m.cm_type == Console::CONSOLE_SYSTEM_NETCHAT && cvw_filter_type_chat);

    return type_ok && area_ok;
}

ImVec2 GUI::ConsoleView::DrawColorMarkedText(ImVec2 bg_cursor, Ogre::TexturePtr icon, ImVec4 default_color, std::string const& line)
{
    ImDrawList* drawlist = ImGui::GetWindowDrawList();

    // Print icon
    drawlist->ChannelsSetCurrent(1); // Text layer
    ImVec2 text_cursor = bg_cursor + cvw_background_padding;
    ImVec2 total_text_size(0,0);
    float text_h = ImGui::CalcTextSize("").y;
    if (cvw_enable_icons && icon)
    {
        ImVec2 icon_size(icon->getWidth(), icon->getHeight());
        ImVec2 tl = ImVec2(text_cursor.x, text_cursor.y + (text_h / 2) - (icon_size.y / 2));
        ImVec2 br = tl + icon_size;
        drawlist->AddImage(reinterpret_cast<ImTextureID>(icon->getHandle()), tl, br, ImVec2(0,0), ImVec2(1,1), ImColor(ImVec4(1,1,1,alpha)));
        const float ICON_GAP = 8;
        total_text_size += ImVec2(icon_size.x + ICON_GAP, text_h);
        text_cursor.x += icon_size.x + ICON_GAP;
    }

    // Print colored line segments

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
            Str<LINE_BUF_MAX> seg_text(seg_start, seg_end);
            ImVec2 text_size = ImGui::CalcTextSize(seg_text.ToCStr());
            drawlist->AddText(text_cursor, ImColor(r,g,b,(int)(alpha*255)), seg_text.ToCStr());
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
        drawlist->AddText(text_cursor, ImColor(r,g,b,(int)(alpha*255)), text.c_str());
        total_text_size.x += text_size.x;
        total_text_size.y = std::max(total_text_size.y, text_size.y);
    }

    // Draw background
    drawlist->ChannelsSetCurrent(0); // Background layer
    ImVec2 bg_rect_size = total_text_size + (cvw_background_padding * 2);
    drawlist->AddRectFilled(bg_cursor, bg_cursor + bg_rect_size,
        ImColor(ImVec4(0,0,0,(alpha / 2))), ImGui::GetStyle().FrameRounding);
    return bg_rect_size;
}

int GUI::ConsoleView::UpdateMessages()
{
    // Lock the console
    Console::MsgLockGuard lock(App::GetConsole());

    // Was console cleared?
    if (lock.messages.size() < m_total_messages)
    {
        m_reload_messages = true;
    }

    // Handle full reload
    if (m_reload_messages)
    {
        m_filtered_messages.clear();
        m_total_messages = 0;
        m_reload_messages = false;
    }

    // Apply filtering
    int orig_size = (int)m_filtered_messages.size();
    for (size_t i = m_total_messages; i < lock.messages.size() ; ++i)
    {
        Console::Message const& m = lock.messages[i];
        if (this->MessageFilter(m))
        {
            if (cvw_enable_scrolling && m.cm_text.find("\n"))
            {
                // Keep it simple: only use single-line messages with scrolling view
                Ogre::StringVector v = Ogre::StringUtil::split(m.cm_text, "\n"); // TODO: optimize
                for (Ogre::String& s: v)
                {
                    Ogre::StringUtil::trim(s);
                    if (s != "")
                    {
                        m_filtered_messages.emplace_back(m.cm_area, m.cm_type, s, m.cm_timestamp, m.cm_net_userid);
                    }
                }
            }
            else
            {
                m_filtered_messages.push_back(m); // Copy
            }
        }
    }
    m_total_messages = lock.messages.size();

    return (int)m_filtered_messages.size() - orig_size;
}
