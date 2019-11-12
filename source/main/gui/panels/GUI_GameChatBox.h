/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

class GameChatBox
{
public:
    GameChatBox();

    void SetVisible(bool v) { m_is_visible = v; };
    bool IsVisible() const { return m_is_visible; }

    void Draw();

private:
    void SubmitMessage(); //!< Flush the user input box

    bool                      m_is_visible = false;
    Str<400>                  m_msg_buffer;
    ConsoleView               m_console_view;
};

} // namespace GUI
} // namespace RoR
