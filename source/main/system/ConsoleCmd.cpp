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
#include "MainMenu.h"
#include "Network.h"
#include "OverlayWrapper.h"
#include "RoRFrameListener.h"
#include "RoRnet.h"
#include "RoRVersion.h"
#include "Scripting.h"
#include "TerrainManager.h"
#include "TerrainObjectManager.h"
#include "Utils.h"

using namespace RoR;

// -------------------------------------------------------------------------------------
// Builtin console commmands

class GravityCmd: public ConsoleCmd
{
public:
    GravityCmd(): ConsoleCmd("gravity", "[<number> or <constant>]", _L("Get or set terrain gravity. Constants: earth/mars/jupiter.")) {}

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
                 if (args[1] == "earth")    { App::GetSimTerrain()->setGravity(-9.81);              }
            else if (args[1] == "moon")     { App::GetSimTerrain()->setGravity(-1.6);               }
            else if (args[1] == "jupiter")  { App::GetSimTerrain()->setGravity(-50);                }
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
                water->WaterSetCamera(gEnv->mainCamera);
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

        assert(App::GetSimTerrain());

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
            assert(gEnv->player);
            pos = gEnv->player->getPosition();
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

        assert(gEnv->player);
        assert(App::GetSimTerrain());

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
                Ogre::Vector3 pos = gEnv->player->getPosition();
                Ogre::SceneNode* bake_node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
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
        bool now_logging = !App::diag_log_console_echo->GetActiveVal<bool>();
        const char* msg = (now_logging) ? " logging to console enabled" : " logging to console disabled";
        reply << _L(msg);
        App::diag_log_console_echo->SetActiveVal(now_logging);

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
        if (!b && gEnv->player)
        {
            Ogre::Vector3 pos = gEnv->player->getPosition();
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
            if (!b && gEnv->player)
            {
                gEnv->player->setPosition(pos);
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
        ScriptEngine::getSingleton().triggerEvent(SE_ANGELSCRIPT_MANIPULATIONS);

        Str<1000> code; // Re-compose the code snippet
        for (int i = 1; i < args.size(); ++i)
        {
            code << " " << args[i];
        }
        ScriptEngine::getSingleton().executeString(code.ToCStr());
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
        App::app_state->SetPendingVal((int)AppState::SHUTDOWN);
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
// Console integration

void Console::RegBuiltinCommands()
{
    ConsoleCmd* cmd = nullptr;

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
        reply << cvar->GetName() << ": " << cvar->GetActiveStr();
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
    if (App::app_state->GetActiveEnum<AppState>() == state)
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
