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
#include <string>

namespace RoR {
namespace GUI {

class ConsoleView
{
public:
    static const size_t MSG_DISP_LIMIT = 100u; // Quick and dirty performance trick
    static const size_t HISTORY_CAP = 100u;

    void SetVisible(bool visible) { m_is_visible = visible; }
    bool IsVisible() const { return m_is_visible; }

    void Draw();
    void DoCommand(std::string msg);

private:

    static int TextEditCallback(ImGuiTextEditCallbackData *data);
    void TextEditCallbackProc(ImGuiTextEditCallbackData *data);

    bool MessageFilter(Console::Message const& m); //!< True if message should be displayed

    Str<500>                 m_cmd_buffer;
    std::vector<std::string> m_cmd_history;
    int                      m_cmd_history_cursor = -1;
    bool                     m_is_visible = false;
    // Filtering (true means allowed)
    bool                     m_filter_type_notice = true;
    bool                     m_filter_type_warning = true;
    bool                     m_filter_type_error = true;
    bool                     m_filter_area_echo = false; //!< Not the same thing as 'log' command!
    bool                     m_filter_area_script = true;
    bool                     m_filter_area_actor = true;
    bool                     m_filter_area_terrn = true;
};

} // namespace GUI
} // namespace RoR
