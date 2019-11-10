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

/// @file

#pragma once

#include <Ogre.h>

namespace RoR {

/// Global game console backend
class Console : public Ogre::LogListener
{
public:
    static const size_t MESSAGES_CAP = 1000u;

    enum MessageType
    {
        CONSOLE_HELP,
        CONSOLE_TITLE,

        CONSOLE_SYSTEM_NOTICE,
        CONSOLE_SYSTEM_ERROR,
        CONSOLE_SYSTEM_WARNING,
        CONSOLE_SYSTEM_REPLY,   //!< Success
    };

    enum MessageArea
    {
        CONSOLE_MSGTYPE_INFO,   //!< Generic message
        CONSOLE_MSGTYPE_LOG,    //!< Logfile echo
        CONSOLE_MSGTYPE_SCRIPT, //!< Any scripting subsystem
        CONSOLE_MSGTYPE_ACTOR,  //!< Parsing/spawn/simulation messages for actors
        CONSOLE_MSGTYPE_TERRN   //!< Parsing/spawn/simulation messages for terrain
    };

    struct Message
    {
        Message(MessageArea area, MessageType type, std::string const& text):
            cm_area(area), cm_type(type), cm_text(text)
        {}

        MessageArea cm_area;
        MessageType cm_type;
        std::string cm_text;
    };

    struct MsgLockGuard
    {
        MsgLockGuard(Console& console)
            : lock(console.m_messages_mutex), messages(console.m_messages)
        {}
        MsgLockGuard(Console* console)
            : MsgLockGuard(*console)
        {}

        std::vector<Message> const& messages;
        std::lock_guard<std::mutex> lock;
    };

    // Legacy function, params `icon, ttl, forcevisible` unused
    void putMessage(MessageArea area, MessageType type, std::string const& msg,
        std::string icon = "", size_t ttl = 0, bool forcevisible = false);
    void ForwardLogMessage(MessageArea area, std::string const& msg, Ogre::LogMessageLevel lml);

private:
    // Ogre::LogListener
    virtual void messageLogged(
        const Ogre::String& message, Ogre::LogMessageLevel lml,
        bool maskDebug, const Ogre::String& logName, bool& skipThisMessage) override;

    std::vector<Message>     m_messages;
    std::mutex               m_messages_mutex;
};

} //namespace RoR
