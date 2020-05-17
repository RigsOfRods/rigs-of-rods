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

#include "GameContext.h"

using namespace RoR;

GameContext::GameContext() {};

// --------------------------
// Message queue

void GameContext::PushMessage(Message m)
{
    std::lock_guard<std::mutex> lock(m_msg_mutex);
    m_msg_queue.push(m);
}

bool GameContext::HasMessages()
{
    std::lock_guard<std::mutex> lock(m_msg_mutex);
    return !m_msg_queue.empty();
}

Message GameContext::PopMessage()
{
    std::lock_guard<std::mutex> lock(m_msg_mutex);
    Message m = m_msg_queue.front();
    m_msg_queue.pop();
    return m;
}
