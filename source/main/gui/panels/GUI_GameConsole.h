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
#include "OgreImGui.h"

#include <vector>
#include <string>

namespace RoR {
// Special - not in namespace GUI

class Console : public Ogre::LogListener
{
public:
    static const size_t MESSAGES_CAP = 100u;
    static const size_t HISTORY_CAP = 100u;

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

    Console();

    void SetVisible(bool visible) { m_is_visible = visible; }
    bool IsVisible() const { return m_is_visible; }

    void putMessage(int area, int type, Ogre::UTFString msg, Ogre::String icon = "bullet_black.png", unsigned long ttl = 30000, bool forcevisible = false);
    void ForwardLogMessage(MessageArea area, std::string const& msg, Ogre::LogMessageLevel lml);

    void Draw();
    void DoCommand(std::string msg);

private:
    // Ogre::LogListener
    void messageLogged(const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String& logName, bool& skipThisMessage);

    static int TextEditCallback(ImGuiTextEditCallbackData *data);
    void TextEditCallbackProc(ImGuiTextEditCallbackData *data);

    bool MessageFilter(Message const& m); //!< True if message should be displayed

    std::vector<Message>     m_messages;
    std::mutex               m_messages_mutex;
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

} // namespace RoR
