/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal

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

#include "Console.h"

#include "Application.h"
#include "Utils.h"

#include <Ogre.h>

using namespace RoR;

void Console::messageLogged(const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String& logName, bool& skipThisMessage)
{
    if (App::diag_log_console_echo.GetActive())
    {
        this->ForwardLogMessage(CONSOLE_MSGTYPE_LOG, message, lml);
    }
}

void Console::ForwardLogMessage(MessageArea area, std::string const& message, Ogre::LogMessageLevel lml)
{
    switch (lml)
    {
    case Ogre::LML_WARNING:
        this->putMessage(area, CONSOLE_SYSTEM_WARNING, RoR::Utils::SanitizeUtf8String(message));
        break;

    case Ogre::LML_CRITICAL:
        this->putMessage(area, CONSOLE_SYSTEM_ERROR, RoR::Utils::SanitizeUtf8String(message));
        break;

    default: // LML_NORMAL, LML_TRIVIAL
        this->putMessage(area, CONSOLE_SYSTEM_NOTICE, RoR::Utils::SanitizeUtf8String(message));
        break;
    }
}

void Console::putMessage(MessageArea area, MessageType type, std::string const& msg,
    std::string icon, size_t ttl, bool forcevisible)
{
    std::lock_guard<std::mutex> lock(m_messages_mutex);
    m_messages.emplace_back(MessageArea(area), MessageType(type), msg);
    if (m_messages.size() > MESSAGES_CAP)
    {
        m_messages.erase(m_messages.begin());
    }
}

