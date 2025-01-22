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

#include "CVar.h"
#include "ConsoleCmd.h"

#include <Ogre.h>
#include <string>
#include <unordered_map>

namespace RoR {

/// @addtogroup Console
/// @{

/// Global game console backend. Frontend is RoR::GUI::ConsoleView.
/// @remarks Singleton; use `App::GetConsole()` to fetch instance.
class Console : public Ogre::LogListener
{
public:
    typedef std::unordered_map<std::string, CVar*> CVarPtrMap;
    typedef std::unordered_map<std::string, ConsoleCmd*> CommandPtrMap;

    enum MessageType
    {
        CONSOLE_HELP,
        CONSOLE_TITLE,

        CONSOLE_SYSTEM_NOTICE,
        CONSOLE_SYSTEM_ERROR,
        CONSOLE_SYSTEM_WARNING,
        CONSOLE_SYSTEM_REPLY,   //!< Success
        CONSOLE_SYSTEM_NETCHAT
    };

    enum MessageArea
    {
        CONSOLE_MSGTYPE_INFO,   //!< Generic message
        CONSOLE_MSGTYPE_LOG,    //!< Logfile echo
        CONSOLE_MSGTYPE_SCRIPT, //!< Messages sent from scripts
        CONSOLE_MSGTYPE_ACTOR,  //!< Parsing/spawn/simulation messages for actors
        CONSOLE_MSGTYPE_TERRN   //!< Parsing/spawn/simulation messages for terrain
    };

    struct Message
    {
        Message(MessageArea area, MessageType type, std::string const& text,
                size_t time, uint32_t net_user = 0, std::string icon = ""):
            cm_area(area), cm_type(type), cm_text(text), cm_timestamp(time), cm_net_userid(net_user), cm_icon(icon)
        {}

        MessageArea cm_area;
        MessageType cm_type;
        size_t      cm_timestamp;
        uint32_t    cm_net_userid;
        std::string cm_text;
        std::string cm_icon;
    };

    struct MsgLockGuard
    {
        MsgLockGuard(Console& console)
            : lock(console.m_messages_mutex), messages(console.m_messages)
        {}
        MsgLockGuard(Console* console)
            : MsgLockGuard(*console)
        {}

        std::vector<Message> & messages;
        std::lock_guard<std::mutex> lock;
    };

    void putMessage(MessageArea area, MessageType type, std::string const& msg, std::string icon = "");
    void putNetMessage(int user_id, MessageType type, const char* text);
    void forwardLogMessage(MessageArea area, std::string const& msg, Ogre::LogMessageLevel lml);
    unsigned long queryMessageTimer() { return m_msg_timer.getMilliseconds(); }
    void purgeNetChatMessagesByUser(int user_id);

    // ----------------------------
    // Commands (defined in ConsoleCmd.cpp):

    /// Register builtin commands
    void regBuiltinCommands();

    /// Identify and execute any console line
    void doCommand(std::string msg);

    // ----------------------------
    // CVars (defined in CVar.cpp):

    /// Add CVar and parse default value if specified
    CVar* cVarCreate(std::string const& name, std::string const& long_name,
        int flags, std::string const& val = std::string());

    /// Parse value by cvar type
    void cVarAssign(CVar* cvar, std::string const& value);

    /// Find cvar by short/long name
    CVar* cVarFind(std::string const& input_name);

    /// Set existing cvar by short/long name. Return the modified cvar (or NULL if not found)
    CVar* cVarSet(std::string const& input_name, std::string const& input_val);

    /// Get cvar by short/long name, or create new one using input as short name.
    CVar* cVarGet(std::string const& input_name, int flags);

    /// Create builtin vars and set defaults
    void cVarSetupBuiltins();

    CVarPtrMap& getCVars() { return m_cvars; }

    CommandPtrMap& getCommands() { return m_commands; }

    // ----------------------------
    // Command line (defined in AppCommandLine.cpp)

    void processCommandLine(int argc, char *argv[]);
    void showCommandLineUsage();
    void showCommandLineVersion();

    // ----------------------------
    // Config file (defined in AppConfig.cpp)

    void loadConfig();
    void saveConfig();

private:
    // Ogre::LogListener
    virtual void messageLogged(
        const Ogre::String& message, Ogre::LogMessageLevel lml,
        bool maskDebug, const Ogre::String& logName, bool& skipThisMessage) override;

    void handleMessage(MessageArea area, MessageType type, std::string const& msg, int net_id = 0, std::string icon = "");

    std::vector<Message>     m_messages;
    std::mutex               m_messages_mutex;
    Ogre::Timer              m_msg_timer;
    CVarPtrMap               m_cvars;
    CVarPtrMap               m_cvars_longname;
    CommandPtrMap            m_commands;
};

/// @} // addtogroup Console

} //namespace RoR
