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

#include <mutex>
#include <string>
#include <vector>

namespace RoR {
namespace GUI {

class GameChatBox
{
public:
    static const size_t MESSAGES_CAP = 100u;
    static const size_t FADEOUT_DELAY_MS = 4000u;
    static const size_t FADEOUT_DURATION_MS = 1000u;

    enum class DispState
    {
        HIDDEN,
        VISIBLE_FRESH,   //<! Invoked by user; will set keyboard focus and switch to FOCUSED
        VISIBLE_FOCUSED, //!< User is typing
        VISIBLE_STALE    //!< User sent message or external message was displayed, fadeout countdown is running
    };

    void SetVisible(bool v);
    bool IsVisible() const { return m_disp_state != DispState::HIDDEN; }

    void Draw();
    void pushMsg(std::string const& txt);

private:
    void SubmitMessage(); //!< Flush the user input box

    DispState                 m_disp_state = DispState::HIDDEN;
    Str<400>                  m_msg_buffer;
    std::vector<std::string>  m_messages;
    std::mutex                m_messages_mutex;
    bool                      m_message_added = false;
    size_t                    m_message_time = 0;
};

} // namespace GUI
} // namespace RoR
