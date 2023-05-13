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

#include "Application.h"
#include "Actor.h"
#include "ActorManager.h"
#include "CameraManager.h"
#include "Character.h"
#include "Console.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "IWater.h"
#include "Language.h"
#include "Network.h"
#include "OverlayWrapper.h"
#include "RoRnet.h"
#include "RoRVersion.h"
#include "ScriptEngine.h"
#include "Terrain.h"
#include "TerrainObjectManager.h"
#include "Utils.h"

#include <algorithm>
#include <Ogre.h>
#include <fmt/core.h>

/// @file

using namespace RoR;

/// @addtogroup Console
/// @{

// -------------------------------------------------------------------------------------
// Builtin console commands.

/// @addtogroup ConsoleCmd
/// @{

class GravityCmd: public ConsoleCmd
{
public:
    GravityCmd(): ConsoleCmd("gravity", "[<number> or <constant>]", _L("Get or set terrain gravity. Constants: earth/moon/mars/jupiter.")) {}

    void Run(Ogre::StringVector const& args) override
    {
        if (!this->CheckAppState(AppState::SIMULATION))
            return;

        Str<200> reply;
        if (args.size() == 1)
        {
            reply << _L("Current gravity is: ") << App::GetGameContext()->GetTerrain()->getGravity();
        }
        else
        {
                 if (args[1] == "earth")    { App::GetGameContext()->GetTerrain()->setGravity(DEFAULT_GRAVITY);    }
            else if (args[1] == "moon")     { App::GetGameContext()->GetTerrain()->setGravity(-1.62f);             }
            else if (args[1] == "mars")     { App::GetGameContext()->GetTerrain()->setGravity(-3.711f);            }
            else if (args[1] == "jupiter")  { App::GetGameContext()->GetTerrain()->setGravity(-24.8f);             }
            else                            { App::GetGameContext()->GetTerrain()->setGravity(PARSEREAL(args[1])); }

            reply << _L("Gravity set to: ") << App::GetGameContext()->GetTerrain()->getGravity();
        }

        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, reply.ToCStr());
    }
};

class WaterlevelCmd: public ConsoleCmd
{
public:
    WaterlevelCmd(): ConsoleCmd("waterlevel", "[<number>]", _L("Get or set water level.")) {}

    void Run(Ogre::StringVector const& args) override
    {
        if (!this->CheckAppState(AppState::SIMULATION))
            return;

        Str<200> reply;
        Console::MessageType reply_type;
        reply << m_name << ": ";
        if (!App::GetGameContext()->GetTerrain()->getWater())
        {
            reply_type = Console::CONSOLE_SYSTEM_ERROR;
            reply << _L("This terrain does not have water.");
        }
        else
        {
            reply_type = Console::CONSOLE_SYSTEM_REPLY;
            if (args.size() > 1)
            {
                IWater* water = App::GetGameContext()->GetTerrain()->getWater();
                float height = (args[1] == "default") ? App::GetGameContext()->GetTerrain()->getWaterHeight() : PARSEREAL(args[1]);
                water->SetStaticWaterHeight(height);
                water->UpdateWater();
            }
            reply << _L ("Water level set to: ") << App::GetGameContext()->GetTerrain()->getWater()->GetStaticWaterHeight();
        }
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, reply_type, reply.ToCStr());
    }
};

class TerrainheightCmd: public ConsoleCmd
{
public:
    TerrainheightCmd(): ConsoleCmd("terrainheight", "[]", _L("Get elevation of terrain at current position")) {}

    void Run(Ogre::StringVector const& args) override
    {
        if (!this->CheckAppState(AppState::SIMULATION))
            return;

        ROR_ASSERT(App::GetGameContext()->GetTerrain());

        Str<200> reply;
        reply << m_name << ": ";
        Console::MessageType reply_type = Console::CONSOLE_SYSTEM_REPLY;
        Ogre::Vector3 pos;
        ActorPtr const actor = App::GetGameContext()->GetPlayerActor();
        if (actor)
        {
            pos = actor->getPosition();

        }
        else
        {
            ROR_ASSERT(App::GetGameContext()->GetPlayerCharacter());
            pos = App::GetGameContext()->GetPlayerCharacter()->getPosition();
        }
        reply << _L("Terrain height at position: ")
              << "x: " << pos.x << " z: " << pos.z << " = "
              <<  App::GetGameContext()->GetTerrain()->GetHeightAt(pos.x, pos.z);
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, reply_type, reply.ToCStr());
    }
};

class SpawnobjectCmd: public ConsoleCmd
{
public:
    SpawnobjectCmd(): ConsoleCmd("spawnobject", "<odef name>", _L("spawnobject  - spawn a object at the player position")) {}

    void Run(Ogre::StringVector const& args) override
    {
        if (!this->CheckAppState(AppState::SIMULATION))
            return;

        ROR_ASSERT(App::GetGameContext()->GetPlayerCharacter());
        ROR_ASSERT(App::GetGameContext()->GetTerrain());

        Str<200> reply;
        reply << m_name << ": ";
        Console::MessageType reply_type;

        try
        {
            if (args.size() == 1)
            {
                reply_type = Console::CONSOLE_SYSTEM_ERROR;
                reply << _L("Missing parameter: ") << m_usage;
            }
            else
            {
                Ogre::Vector3 pos = App::GetGameContext()->GetPlayerCharacter()->getPosition();
                if (App::GetGameContext()->GetTerrain()->getObjectManager()->LoadTerrainObject(args[1], pos, Ogre::Vector3::ZERO, "Console", ""))
                {
                    reply_type = Console::CONSOLE_SYSTEM_REPLY;
                    reply << _L("Spawned object at position: ") << "x: " << pos.x << " z: " << pos.z;
                }
                else
                {
                    reply_type = Console::CONSOLE_SYSTEM_ERROR;
                    reply << _L("Could not spawn object");
                }
            }
        }
        catch (std::exception& e)
        {
            reply_type = Console::CONSOLE_SYSTEM_ERROR;
            reply << e.what();
        }
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, reply_type, reply.ToCStr());
    }
};

class LogCmd: public ConsoleCmd
{
public:
    LogCmd(): ConsoleCmd("log", "[]", _L("log - toggles log output on the console")) {}

    void Run(Ogre::StringVector const& args) override
    {
        Str<200> reply;
        reply << m_name << ": ";
        Console::MessageType reply_type = Console::CONSOLE_SYSTEM_REPLY;

        // switch to console logging
        bool now_logging = !App::diag_log_console_echo->getBool();
        const char* msg = (now_logging) ? " logging to console enabled" : " logging to console disabled";
        reply << _L(msg);
        App::diag_log_console_echo->setVal(now_logging);

        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, reply_type, reply.ToCStr());
    }
};

class VerCmd: public ConsoleCmd
{
public:
    VerCmd(): ConsoleCmd("ver", "[]", _L("ver - shows the Rigs of Rods version")) {}

    void Run(Ogre::StringVector const& args) override
    {
        Str<200> reply;
        reply << m_name << ": ";
        Console::MessageType reply_type = Console::CONSOLE_SYSTEM_REPLY;

        reply << "Rigs of Rods " << ROR_VERSION_STRING 
              << " (" << RORNET_VERSION << ") [" 
              << ROR_BUILD_DATE << ", " << ROR_BUILD_TIME << "]";

        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, reply_type, reply.ToCStr());
    }
};

class PosCmd: public ConsoleCmd
{
public:
    PosCmd(): ConsoleCmd("pos", "[]", _L("pos - outputs the current position")) {}

    void Run(Ogre::StringVector const& args) override
    {
        if (!this->CheckAppState(AppState::SIMULATION))
            return;

        Str<200> reply;
        reply << m_name << ": ";
        Console::MessageType reply_type = Console::CONSOLE_SYSTEM_REPLY;

        ActorPtr b = App::GetGameContext()->GetPlayerActor();
        if (!b && App::GetGameContext()->GetPlayerCharacter())
        {
            Ogre::Vector3 pos = App::GetGameContext()->GetPlayerCharacter()->getPosition();
            reply << _L("Character position: ") << "x: " << pos.x << " y: " << pos.y << " z: " << pos.z;
        }
        else if (b)
        {
            Ogre::Vector3 pos = b->getPosition();
            reply << _L("Vehicle position: ") << "x: " << pos.x << " y: " << pos.y << " z: " << pos.z;
        }

        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, reply_type, reply.ToCStr());
    }
};

class GotoCmd: public ConsoleCmd
{
public:
    GotoCmd(): ConsoleCmd("goto", "<x> <y> <z>", _L("goto <x> <y> <z> - jumps to the mentioned position")) {}

    void Run(Ogre::StringVector const& args) override
    {
        if (!this->CheckAppState(AppState::SIMULATION))
            return;

        Str<200> reply;
        reply << m_name << ": ";
        Console::MessageType reply_type;

        if (args.size() != 4)
        {
            reply_type = Console::CONSOLE_HELP;
            reply <<_L("usage: goto x y z");
        }
        else
        {
            reply_type = Console::CONSOLE_SYSTEM_REPLY;
            Ogre::Vector3 pos(PARSEREAL(args[1]), PARSEREAL(args[2]), PARSEREAL(args[3]));

            ActorPtr b = App::GetGameContext()->GetPlayerActor();
            if (!b && App::GetGameContext()->GetPlayerCharacter())
            {
                App::GetGameContext()->GetPlayerCharacter()->setPosition(pos);
                reply << _L("Character position set to: ") << "x: " << pos.x << " y: " << pos.y << " z: " << pos.z;
            }
            else if (b)
            {
                b->resetPosition(pos, false);
                TRIGGER_EVENT_ASYNC(SE_TRUCK_TELEPORT, b->ar_instance_id);
                reply << _L("Vehicle position set to: ") << "x: " << pos.x << " y: " << pos.y << " z: " << pos.z;
            }
        }

        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, reply_type, reply.ToCStr());
    }
};


class AsCmd: public ConsoleCmd
{
public:
    AsCmd(): ConsoleCmd("as", "<code>", _L("Run AngelScript code snippet")) {}

    void Run(Ogre::StringVector const& args) override
    {
        if (!this->CheckAppState(AppState::SIMULATION))
            return;

        Str<200> reply;
        reply << m_name << ": ";
        Console::MessageType reply_type;

#ifdef USE_ANGELSCRIPT
        // we want to notify any running scripts that we might change something (prevent cheating)
        App::GetScriptEngine()->triggerEvent(SE_ANGELSCRIPT_MANIPULATIONS);

        // Re-compose the code snippet
        Str<1000> code;
        for (int i = 1; i < args.size(); ++i)
        {
            code << " " << args[i];
        }

        // Echo the code back to console user.
        reply_type = Console::CONSOLE_SYSTEM_REPLY;
        reply << " >>> " << code.ToCStr();
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, reply_type, reply.ToCStr());

        // Run the code - will output script messages/AngelScript errors.
        App::GetScriptEngine()->executeString(code.ToCStr());
#else
        reply_type = Console::CONSOLE_SYSTEM_ERROR;
        reply << _L("Scripting disabled in this build");
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, reply_type, reply.ToCStr());
#endif
    }
};

class QuitCmd: public ConsoleCmd
{
public:
    QuitCmd(): ConsoleCmd("quit", "[]", _L("quit - exit Rigs of Rods")) {}

    void Run(Ogre::StringVector const& args) override
    {
        App::GetGameContext()->PushMessage(Message(MSG_APP_SHUTDOWN_REQUESTED));
    }
};

class HelpCmd: public ConsoleCmd
{
public:
    HelpCmd(): ConsoleCmd("help", "[]", _L("help - information on commands (this)")) {}

    void Run(Ogre::StringVector const& args) override
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO,
            Console::CONSOLE_TITLE, _L("Available commands:"));

        Str<500> line;
        for (auto& cmd_pair: App::GetConsole()->getCommands())
        {
            line.Clear() << cmd_pair.second->getName() << " "
                         << cmd_pair.second->GetUsage() << " - " << cmd_pair.second->GetDoc();

            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO,
                Console::CONSOLE_HELP, line.ToCStr());
        }

        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO,
            Console::CONSOLE_TITLE, _L("Tips:"));
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO,
            Console::CONSOLE_HELP, _L("- use Arrow Up/Down Keys in the InputBox to reuse old messages"));
    }
};

class LoadScriptCmd : public ConsoleCmd
{
public:
    LoadScriptCmd() : ConsoleCmd("loadscript", "[filename]", _L("Runs an AngelScript file")) {}

    void Run(Ogre::StringVector const& args) override
    {
        Str<200> reply;
        reply << m_name << ": ";
        Console::MessageType reply_type;

#ifdef USE_ANGELSCRIPT
        if (args.size() == 1)
        {
            reply_type = Console::CONSOLE_SYSTEM_ERROR;
            reply << _L("Missing parameter: ") << m_usage;
        }
        else
        {
            ScriptUnitId_t id = App::GetScriptEngine()->loadScript(args[1], ScriptCategory::CUSTOM);
            if (id == SCRIPTUNITID_INVALID)
            {
                reply_type = Console::CONSOLE_SYSTEM_ERROR;
                reply << _L("Failed to load script. See 'Angelscript.log' or use console command `log` and retry.");
            }
            else
            {
                reply_type = Console::CONSOLE_SYSTEM_REPLY;
                reply << fmt::format(_L("Script '{}' running with ID '{}'"), args[1], id);
            }
        }
#else
        reply_type = Console::CONSOLE_SYSTEM_ERROR;
        reply << _L("Scripting disabled in this build");
#endif
        
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, reply_type, reply.ToCStr());
    }
};

// -------------------------------------------------------------------------------------
// CVar (builtin) console commmands

class VarsCmd: public ConsoleCmd
{
public:
    VarsCmd(): ConsoleCmd("vars", "[<expr> ...]", _L("Print cvars with one of <expr> in name")) {}

    void Run(Ogre::StringVector const& args) override
    {
        for (auto& pair: App::GetConsole()->getCVars())
        {
            bool match = args.size() == 1;
            for (size_t i = 1; i < args.size(); ++i)
            {
                if (pair.first.find(args[i]) != std::string::npos)
                {
                    match = true;
                    break;
                }
            }

            if (match)
            {
                Str<200> reply;
                reply << "vars: " << pair.first << "=" << pair.second->getStr() << " (";

                if      (pair.second->hasFlag(CVAR_TYPE_BOOL))  { reply << "bool"; }
                else if (pair.second->hasFlag(CVAR_TYPE_INT))   { reply << "int"; }
                else if (pair.second->hasFlag(CVAR_TYPE_FLOAT)) { reply << "float"; }
                else                                            { reply << "string"; }

                if (pair.second->hasFlag(CVAR_ARCHIVE)) { reply << ", archive"; }
                if (pair.second->hasFlag(CVAR_NO_LOG))  { reply << ", no log"; }

                reply << ")";
                App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, reply.ToCStr());
            }
        }
    }
};

class SetCmd: public ConsoleCmd
{
public:
    SetCmd(): ConsoleCmd("set", "<cvar> [<flags>] [<value>]", _L("Get or set value of existing CVar")) {}

    void Run(Ogre::StringVector const& args) override
    {
        Str<200> reply;
        reply << m_name << ": ";
        Console::MessageType reply_type = Console::CONSOLE_SYSTEM_ERROR;

        if (args.size() == 1)
        {
            reply_type = Console::CONSOLE_HELP;
            reply << this->GetUsage() << " - " << this->GetDoc();
        }
        else
        {
            CVar* cvar = App::GetConsole()->cVarFind(args[1]);
            if (cvar)
            {
                if (args.size() > 2)
                {
                    App::GetConsole()->cVarAssign(cvar, args[2]);
                }
                reply_type = Console::CONSOLE_SYSTEM_REPLY;
                reply << cvar->getName() << " = " << cvar->getStr();
            }
            else
            {
                reply_type = Console::CONSOLE_SYSTEM_ERROR;
                reply << _L("No such CVar: ") << args[1];
            }
        }

        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, reply_type, reply.ToCStr());
    }
};

class SetCVarCmd: public ConsoleCmd // Generic
{
public:
    SetCVarCmd(std::string const& name, std::string const& usage, std::string const& doc, int flag):
        ConsoleCmd(name, usage, doc), m_cvar_flag(flag)
    {}

    void Run(Ogre::StringVector const& args) override
    {
        Str<200> reply;
        reply << m_name << ": ";
        Console::MessageType reply_type;

        if (args.size() == 1)
        {
            reply_type = Console::CONSOLE_HELP;
            reply << this->GetUsage() << " - " << this->GetDoc() << "Switches: --autoapply/--allowstore/--autostore";
        }
        else
        {
            int flags = m_cvar_flag;
            size_t i;
            for (i = 1; i < args.size(); ++i)
            {
                     if (args[i] == "--archive")  { flags |= CVAR_ARCHIVE; }
                else break; // Exit loop on first non-switch arg!
            }

            if (i == args.size()) // Only switches but no cvar?
            {
                reply_type = Console::CONSOLE_HELP;
                reply << this->GetUsage() << " - " << this->GetDoc() << "Switches: --archive";
            }
            else
            {
                CVar* cvar = App::GetConsole()->cVarGet(args[i], flags);
                if (args.size() > (i+1))
                {
                    App::GetConsole()->cVarAssign(cvar, args[i+1]);
                }
                reply_type = Console::CONSOLE_SYSTEM_REPLY;
                reply << cvar->getName() << " = " << cvar->getStr();
            }
        }

        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, reply_type, reply.ToCStr());
    }
private:
    int m_cvar_flag;
};

class SetstringCmd: public SetCVarCmd
{
public:
    SetstringCmd(): SetCVarCmd("setstring", "<cvar> [<value>]", _L("Set or create string CVar"), /*flag=*/0) {}
};

class SetboolCmd: public SetCVarCmd
{
public:
    SetboolCmd(): SetCVarCmd("setbool", "<cvar> [<value>]", _L("Set or create boolean CVar"), CVAR_TYPE_BOOL) {}
};

class SetintCmd: public SetCVarCmd
{
public:
    SetintCmd(): SetCVarCmd("setint", "<cvar> [<value>]", _L("Set or create integer CVar"), CVAR_TYPE_INT) {}
};

class SetfloatCmd: public SetCVarCmd
{
public:
    SetfloatCmd(): SetCVarCmd("setfloat", "<cvar> [<value>]", _L("Set or create real-number CVar"), CVAR_TYPE_FLOAT) {}
};

class ClearCmd: public ConsoleCmd
{
public:
    ClearCmd(): ConsoleCmd("clear", "[<type>]", _L("Clear console history. Types: all/info/net/chat/terrn/actor/script")) {}

    void Run(Ogre::StringVector const& args) override
    {
        if (args.size() < 2 || args[1] == "all")
        {
            Console::MsgLockGuard lock(App::GetConsole());
            lock.messages.clear();
        }
        else
        {
            // Create a predicate function
            std::function<bool(Console::Message const& m)> filter_fn;
            if (args[1] == "chat")
            {
                filter_fn = [](Console::Message const& m){ return m.cm_type == Console::CONSOLE_SYSTEM_NETCHAT; };
            }
            else if (args[1] == "net") // Chat and user notifications
            {
                filter_fn = [](Console::Message const& m){ return m.cm_net_userid != 0; };
            }
            else
            {
                Console::MessageArea area;
                bool valid = false;
                     if (args[1] == "info")   { area = Console::CONSOLE_MSGTYPE_INFO;   valid = true; }
                else if (args[1] == "terrn")  { area = Console::CONSOLE_MSGTYPE_TERRN;  valid = true; }
                else if (args[1] == "actor")  { area = Console::CONSOLE_MSGTYPE_ACTOR;  valid = true; }
                else if (args[1] == "script") { area = Console::CONSOLE_MSGTYPE_SCRIPT; valid = true; }

                if (valid)
                {
                    filter_fn = [area](Console::Message const& m) { return m.cm_area == area; };
                }
                else
                {
                    App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
                        fmt::format(_L("No such message type: {}"), args[1]));
                }
            }

            Console::MsgLockGuard lock(App::GetConsole());
            // Shove unwanted entries to the end
            auto erase_begin = std::remove_if(lock.messages.begin(), lock.messages.end(), filter_fn);
            // Erase unwanted
            lock.messages.erase(erase_begin, lock.messages.end());
        }
    }
};

/// @} // addtogroup ConsoleCmd

// -------------------------------------------------------------------------------------
// Console integration

void Console::regBuiltinCommands()
{
    ConsoleCmd* cmd = nullptr;

    // Classics
    cmd = new GravityCmd();               m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new WaterlevelCmd();            m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new TerrainheightCmd();         m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new SpawnobjectCmd();           m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new LogCmd();                   m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new VerCmd();                   m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new PosCmd();                   m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new GotoCmd();                  m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new AsCmd();                    m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new QuitCmd();                  m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new HelpCmd();                  m_commands.insert(std::make_pair(cmd->getName(), cmd));
    // Additions
    cmd = new ClearCmd();                 m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new LoadScriptCmd();            m_commands.insert(std::make_pair(cmd->getName(), cmd));
    // CVars
    cmd = new SetCmd();                   m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new SetstringCmd();             m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new SetboolCmd();               m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new SetintCmd();                m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new SetfloatCmd();              m_commands.insert(std::make_pair(cmd->getName(), cmd));
    cmd = new VarsCmd();                  m_commands.insert(std::make_pair(cmd->getName(), cmd));
}

void Console::doCommand(std::string msg)
{
    if (msg[0] == '/' || msg[0] == '\\')
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, 
            _L("Using slashes before commands are deprecated, you can now type command without any slashes"));
        msg.erase(msg.begin());
    }

    Ogre::StringVector args = Ogre::StringUtil::split(msg, " ");

    auto found = m_commands.find(args[0]);
    if (found != m_commands.end())
    {
        found->second->Run(args);
        return;
    }

    CVar* cvar = this->cVarFind(args[0]);
    if (cvar)
    {
        Str<200> reply;
        reply << cvar->getName() << " = " << cvar->getStr();
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, reply.ToCStr());
        return;
    }

    Str<200> reply;
    reply << _L("unknown command: ") << msg;
    App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, reply.ToCStr());
}

// -------------------------------------------------------------------------------------
// Helpers

bool ConsoleCmd::CheckAppState(AppState state)
{
    if (App::app_state->getEnum<AppState>() == state)
        return true;

    Str<200> reply;
    reply << m_name << ": ";
    if (state == AppState::SIMULATION)
    {
        reply << _L("Only allowed when simulation is running");
    }
    else
    {
        reply << _L("Not allowed in current app state");
    }
    App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, reply.ToCStr());
    return false;
}

     // Currently unused: _L("Please enter a correct value. ")

/// @} // addtogroup Console
