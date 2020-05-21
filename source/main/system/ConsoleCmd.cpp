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
#include "Beam.h"
#include "BeamFactory.h"
#include "Character.h"
#include "Console.h"
#include "GUIManager.h"
#include "IWater.h"
#include "Language.h"

#include "Network.h"
#include "OverlayWrapper.h"
#include "RoRFrameListener.h"
#include "RoRnet.h"
#include "RoRVersion.h"
#include "ScriptEngine.h"
#include "TerrainManager.h"
#include "TerrainObjectManager.h"
#include "Utils.h"

using namespace RoR;

// -------------------------------------------------------------------------------------
// Builtin console commmands

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
            reply << _L("Current gravity is: ") << App::GetSimTerrain()->getGravity();
        }
        else
        {
                 if (args[1] == "earth")    { App::GetSimTerrain()->setGravity(DEFAULT_GRAVITY);    }
            else if (args[1] == "moon")     { App::GetSimTerrain()->setGravity(-1.62f);             }
            else if (args[1] == "mars")     { App::GetSimTerrain()->setGravity(-3.711f);            }
            else if (args[1] == "jupiter")  { App::GetSimTerrain()->setGravity(-24.8f);             }
            else                            { App::GetSimTerrain()->setGravity(PARSEREAL(args[1])); }

            reply << _L("Gravity set to: ") << App::GetSimTerrain()->getGravity();
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
        if (!App::GetSimTerrain()->getWater())
        {
            reply_type = Console::CONSOLE_SYSTEM_ERROR;
            reply << _L("This terrain does not have water.");
        }
        else
        {
            reply_type = Console::CONSOLE_SYSTEM_REPLY;
            if (args.size() > 1)
            {
                IWater* water = App::GetSimTerrain()->getWater();
                float height = (args[1] == "default") ? App::GetSimTerrain()->getWaterHeight() : PARSEREAL(args[1]);
                water->SetStaticWaterHeight(height);
                water->UpdateWater();
            }
            reply << _L ("Water level set to: ") << App::GetSimTerrain()->getWater()->GetStaticWaterHeight();
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

        ROR_ASSERT(App::GetSimTerrain());

        Str<200> reply;
        reply << m_name << ": ";
        Console::MessageType reply_type = Console::CONSOLE_SYSTEM_REPLY;
        Ogre::Vector3 pos;
        Actor* const actor = App::GetSimController()->GetPlayerActor();
        if (actor)
        {
            pos = actor->getPosition();

        }
        else
        {
            ROR_ASSERT(App::GetSimController()->GetPlayerCharacter());
            pos = App::GetSimController()->GetPlayerCharacter()->getPosition();
        }
        reply << _L("Terrain height at position: ")
              << "x: " << pos.x << " z: " << pos.z << " = "
              <<  App::GetSimTerrain()->GetHeightAt(pos.x, pos.z);
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

        ROR_ASSERT(App::GetSimController()->GetPlayerCharacter());
        ROR_ASSERT(App::GetSimTerrain());

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
                Ogre::Vector3 pos = App::GetSimController()->GetPlayerCharacter()->getPosition();
                Ogre::SceneNode* bake_node = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
                App::GetSimTerrain()->getObjectManager()->LoadTerrainObject(args[1], pos, Ogre::Vector3::ZERO, bake_node, "Console", "");

                reply_type = Console::CONSOLE_SYSTEM_REPLY;
                reply << _L("Spawned object at position: ") << "x: " << pos.x << " z: " << pos.z;
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
        bool now_logging = !App::diag_log_console_echo->GetBool();
        const char* msg = (now_logging) ? " logging to console enabled" : " logging to console disabled";
        reply << _L(msg);
        App::diag_log_console_echo->SetVal(now_logging);

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

        Actor* b = App::GetSimController()->GetPlayerActor();
        if (!b && App::GetSimController()->GetPlayerCharacter())
        {
            Ogre::Vector3 pos = App::GetSimController()->GetPlayerCharacter()->getPosition();
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

            Actor* b = App::GetSimController()->GetPlayerActor();
            if (!b && App::GetSimController()->GetPlayerCharacter())
            {
                App::GetSimController()->GetPlayerCharacter()->setPosition(pos);
                reply << _L("Character position set to: ") << "x: " << pos.x << " y: " << pos.y << " z: " << pos.z;
            }
            else if (b)
            {
                b->ResetPosition(pos, false);
                TRIGGER_EVENT(SE_TRUCK_TELEPORT, b->ar_instance_id);
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

        Str<1000> code; // Re-compose the code snippet
        for (int i = 1; i < args.size(); ++i)
        {
            code << " " << args[i];
        }
        App::GetScriptEngine()->executeString(code.ToCStr());
        reply_type = Console::CONSOLE_SYSTEM_REPLY;
        reply << " >>> " << code.ToCStr();
#else
        reply_type = Console::CONSOLE_SYSTEM_ERROR;
        reply << _L("Scripting disabled in this build");
#endif
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, reply_type, reply.ToCStr());
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
        for (auto& cmd_pair: App::GetConsole()->GetCommands())
        {
            line.Clear() << cmd_pair.second->GetName() << " "
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

// -------------------------------------------------------------------------------------
// CVar (builtin) console commmands

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
            CVar* cvar = App::GetConsole()->CVarFind(args[1]);
            if (cvar)
            {
                if (args.size() > 2)
                {
                    App::GetConsole()->CVarAssign(cvar, args[2]);
                }
                reply_type = Console::CONSOLE_SYSTEM_REPLY;
                reply << cvar->GetName() << " = " << cvar->GetStr();
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
                CVar* cvar = App::GetConsole()->CVarGet(args[i], flags);
                if (args.size() > (i+1))
                {
                    App::GetConsole()->CVarAssign(cvar, args[i+1]);
                }
                reply_type = Console::CONSOLE_SYSTEM_REPLY;
                reply << cvar->GetName() << " = " << cvar->GetStr();
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
    ClearCmd(): ConsoleCmd("clear", "[]", _L("Clear console history")) {}

    void Run(Ogre::StringVector const& args) override
    {
        Console::MsgLockGuard lock(App::GetConsole());
        lock.messages.clear();
    }
};

// -------------------------------------------------------------------------------------
// Console integration

void Console::RegBuiltinCommands()
{
    ConsoleCmd* cmd = nullptr;

    // Classics
    cmd = new GravityCmd();               m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    cmd = new WaterlevelCmd();            m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    cmd = new TerrainheightCmd();         m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    cmd = new SpawnobjectCmd();           m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    cmd = new LogCmd();                   m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    cmd = new VerCmd();                   m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    cmd = new PosCmd();                   m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    cmd = new GotoCmd();                  m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    cmd = new AsCmd();                    m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    cmd = new QuitCmd();                  m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    cmd = new HelpCmd();                  m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    // Additions
    cmd = new ClearCmd();                 m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    // CVars
    cmd = new SetCmd();                   m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    cmd = new SetstringCmd();             m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    cmd = new SetboolCmd();               m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    cmd = new SetintCmd();                m_commands.insert(std::make_pair(cmd->GetName(), cmd));
    cmd = new SetfloatCmd();              m_commands.insert(std::make_pair(cmd->GetName(), cmd));
}

void Console::DoCommand(std::string msg)
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

    CVar* cvar = this->CVarFind(args[0]);
    if (cvar)
    {
        Str<200> reply;
        reply << cvar->GetName() << " = " << cvar->GetStr();
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
    if (App::app_state->GetEnum<AppState>() == state)
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
