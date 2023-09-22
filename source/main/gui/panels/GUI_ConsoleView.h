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
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/// @file
/// @brief Generic console rendering

#pragma once

#include "Application.h"

#include "Application.h"
#include "Console.h"


#include <vector>


namespace RoR {
namespace GUI {

/// Configurable console renderer, implements filtering, colorized text, incons.
struct ConsoleView
{
    void DrawConsoleMessages();
    void DrawFilteringOptions();
    void RequestReloadMessages() {m_reload_messages=true;}

    // Filtering (true means allowed)
    bool  cvw_filter_type_notice = true;
    bool  cvw_filter_type_warning = true;
    bool  cvw_filter_type_error = true;
    bool  cvw_filter_type_chat = true;
    bool  cvw_filter_type_cmd = true;
    bool  cvw_filter_area_echo = false; //!< Not the same thing as 'log' command!
    bool  cvw_filter_area_script = true;
    bool  cvw_filter_area_actor = true;
    bool  cvw_filter_area_terrn = true;
    bool  cvw_smooth_scrolling = true;

    // Misc options
    size_t cvw_msg_duration_ms = 0u; //!< Message expiration; 0 means unlimited
    bool   cvw_enable_scrolling = false; // !< Vertical, multiline messages are broken apart when enabled
    bool   cvw_enable_icons = false;
    ImVec4 cvw_background_color = ImVec4(0,0,0,0); //!< Text-background color
    ImVec2 cvw_background_padding = ImVec2(0,0);
    float  cvw_line_spacing = 1.f;
    float  alpha = 1.f;
    size_t fadeout_interval = 700u;

private:
    bool   MessageFilter(Console::Message const& m); //!< Returns true if message should be displayed
    /// Returns final text size
    ImVec2 DrawColoredTextWithIcon(ImVec2 text_cursor, Ogre::TexturePtr icon, ImVec4 default_color, std::string const& line);
    int    UpdateMessages(); //!< Ret. num of new message(s)
    ImVec2 DrawMessage(ImVec2 cursor, Console::Message const& m);

    std::vector<Console::Message> m_filtered_messages;    //!< Updated as needed
    std::vector<const Console::Message*> m_display_messages; //!< Rebuilt every frame; kept as member to reuse allocated memory
    bool                          m_reload_messages = false;
    size_t                        m_total_messages = 0;
};

} // namespace GUI
} // namespace RoR
