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
/// @author Original MyGUI version: Moncef Ben Slimane, 2/2015
/// @author Remake with DearIMGUI: Petr Ohlidal, 11/2019

#pragma once

#include "Application.h"

#include "GUI_ConsoleView.h"

#include <mutex>
#include <string>
#include <vector>

namespace RoR {
namespace GUI {

/// Overlay showing chat/console messages on screen, with optional entry field.
class GameChatBox
{
public:
    GameChatBox();

    void SetVisible(bool v) { m_is_visible = v; m_kb_focused = false; };
    bool IsVisible() const { return m_is_visible; }

    void Draw();
    ConsoleView& GetConsoleView() { return m_console_view; }
    void AssignBuffer(const std::string& buffer) { m_msg_buffer = buffer; m_scheduled_move_textcursor_to_end = true; }

private:
    void SubmitMessage(); //!< Flush the user input box

    bool                      m_is_visible = false; //!< Special: false means 'display only messages'
    bool                      m_kb_focused = false;
    Str<400>                  m_msg_buffer;
    ConsoleView               m_console_view;
    bool                      initialized = true;
    bool                      init_scroll = false;
    bool                      m_scheduled_move_textcursor_to_end = false;
};

} // namespace GUI
} // namespace RoR
