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

#pragma once

#include "Application.h"

#include "Application.h"
#include "CacheSystem.h"
#include "GUI_ConsoleView.h"
#include "OgreImGui.h"

#include <vector>
#include <string>

namespace RoR {
namespace GUI {

class ConsoleWindow
{
public:
    static const size_t HISTORY_CAP = 100u;

    ConsoleWindow();
    void SetVisible(bool visible) { m_is_visible = visible; }
    bool IsVisible() const { return m_is_visible; }

    void DoCommand(std::string msg);

    void Draw();
    void DrawAddonSelector();

private:

    static int TextEditCallback(ImGuiTextEditCallbackData *data);
    void TextEditCallbackProc(ImGuiTextEditCallbackData *data);

    ConsoleView              m_console_view;
    Str<500>                 m_cmd_buffer;
    std::vector<std::string> m_cmd_history;
    CacheQuery               m_addons_query; //!< Buffered display list of addons.
    bool                     m_addons_refreshed = false; //!< Initial refresh.
    int                      m_cmd_history_cursor = -1;
    bool                     m_is_visible = false;
};

} // namespace GUI
} // namespace RoR
