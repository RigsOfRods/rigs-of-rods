/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal & contributors

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

/// @file


#pragma once

#include "RoRPrerequisites.h"

#include "Application.h"
#include "Console.h"
#include "OgreImGui.h"

#include <vector>
#include <regex>


namespace RoR {
namespace GUI {

struct ConsoleView
{
    void DrawConsoleMessages();
    void DrawFilteringOptions();
    unsigned long GetNewestMsgTime() { return m_newest_msg_time; }

    // Filtering (true means allowed)
    bool  cvw_filter_type_notice = true;
    bool  cvw_filter_type_warning = true;
    bool  cvw_filter_type_error = true;
    bool  cvw_filter_type_chat = true;
    bool  cvw_filter_area_echo = false; //!< Not the same thing as 'log' command!
    bool  cvw_filter_area_script = true;
    bool  cvw_filter_area_actor = true;
    bool  cvw_filter_area_terrn = true;
    // Misc options
    size_t cvw_filter_duration_ms = 0u; //!< Message expiration; 0 means unlimited
    size_t cvw_max_lines = 100u;
    bool   cvw_align_bottom = false;

private:
    typedef std::vector<const Console::Message*> DisplayMsgVec;

    bool MessageFilter(Console::Message const& m); //!< Returns true if message should be displayed
    void DrawColorMarkedText(ImVec4 default_color, std::string const& line);

    DisplayMsgVec    m_display_list;
    unsigned long    m_newest_msg_time = 0;      // Updated by `DrawConsoleMessages()`
    std::regex       m_text_color_regex = std::regex(R"(#[a-fA-F\d]{6})");
};

} // namespace GUI
} // namespace RoR
