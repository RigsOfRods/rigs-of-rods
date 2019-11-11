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

#include <Ogre.h>

using namespace RoR;
using namespace Ogre;

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
    if (m_messages.size() + 1 > MESSAGES_CAP)
    {
        m_messages.erase(m_messages.begin());
    }
    m_messages.emplace_back(MessageArea(area), MessageType(type), msg,
        Ogre::Root::getSingleton().getTimer()->getMilliseconds());
}

void Console::putNetMessage(uint32_t user_id, MessageType type, const char* text)
{
    std::lock_guard<std::mutex> lock(m_messages_mutex);
    if (m_messages.size() + 1 > MESSAGES_CAP)
    {
        m_messages.erase(m_messages.begin());
    }
    m_messages.emplace_back(CONSOLE_MSGTYPE_INFO, type, text,
        Ogre::Root::getSingleton().getTimer()->getMilliseconds(), user_id);
}

void Console::DoCommand(std::string msg) // All commands are processed here
{
    const bool is_appstate_sim = (App::app_state.GetActive() == AppState::SIMULATION);
    const bool is_sim_select = (App::sim_state.GetActive() == SimState::SELECTING);

    if (msg[0] == '/' || msg[0] == '\\')
    {
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, _L ("Using slashes before commands are deprecated, you can now type command without any slashes"), "help.png");
        msg.erase(msg.begin());
    }

    Ogre::StringVector args = Ogre::StringUtil::split(msg, " ");
    if (args[0] == "help")
    {
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_TITLE, _L("Available commands:"), "help.png");
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, _L("help - information on commands (this)"), "help.png");
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, _L("ver - shows the Rigs of Rods version"), "information.png");
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, _L("pos - outputs the current position"), "world.png");
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, _L("goto <x> <y> <z> - jumps to the mentioned position"), "world.png");


        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, _L("terrainheight - get height of terrain at current position"), "world.png");

        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, _L("log - toggles log output on the console"), "table_save.png");

        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, _L("quit - exit Rigs of Rods"), "table_save.png");

#ifdef USE_ANGELSCRIPT
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, _L("as <code here> - interpret angel code using console"), "script_go.png");
#endif // USE_ANGELSCRIPT

        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, _L("gravity <real> or <text string> - changes gravity constant. Outputs current value if no argument is given"), "script_go.png");
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, _L("Possible values: \n earth \n moon \n jupiter \n A random number (use negative)"), "script_go.png");

        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, _L("setwaterlevel <real> or default - changes water's level"), "script_go.png");

        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, _L("spawnobject <odef name> - spawn a object at the player position"), "script_go.png");

        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_TITLE, _L("Tips:"), "help.png");
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, _L("- use Arrow Up/Down Keys in the InputBox to reuse old messages"), "information.png");
        return;
    }
    else if (args[0] == "gravity")
    {
        float gValue;

        if (args.size() > 1)
        {
            if (args[1] == "earth")
                gValue = -9.81;
            else if (args[1] == "moon")
                gValue = -1.6;
            else if (args[1] == "jupiter")
                gValue = -50;
            else
                gValue = PARSEREAL(args[1]);
        }
        else
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, _L("Current gravity is: ") + StringConverter::toString(App::GetSimTerrain()->getGravity()), "information.png");
            return;
        }

        App::GetSimTerrain()->setGravity(gValue);
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, _L("Gravity set to: ") + StringConverter::toString(gValue), "information.png");
        return;
    }
    else if (args[0] == "setwaterlevel" && is_appstate_sim)
    {
        if (args.size() > 1)
        {
            IWater* water = App::GetSimTerrain()->getWater();
            if (water != nullptr)
            {
                float height = (args[1] == "default") ? App::GetSimTerrain()->getWaterHeight() : PARSEREAL(args[1]);
                water->WaterSetCamera(gEnv->mainCamera);
                water->SetStaticWaterHeight(height);
                water->UpdateWater();
                RoR::App::GetConsole()->putMessage (Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, _L ("Water level set to: ") + StringConverter::toString (water->GetStaticWaterHeight ()), "information.png");
            }
            else
            {
                RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("This terrain does not have water."), "information.png");
            }
        }
        else
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("Please enter a correct value. "), "information.png");
        }
        return;
    }
    else if (args[0] == "pos" && (is_appstate_sim && !is_sim_select))
    {
        Actor* b = App::GetSimController()->GetPlayerActor();
        if (!b && gEnv->player)
        {
            Vector3 pos = gEnv->player->getPosition();
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, _L("Character position: ") + String("x: ") + TOSTRING(pos.x) + String(" y: ") + TOSTRING(pos.y) + String(" z: ") + TOSTRING(pos.z), "world.png");
        }
        else if (b)
        {
            Vector3 pos = b->getPosition();
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, _L("Vehicle position: ") + String("x: ") + TOSTRING(pos.x) + String(" y: ") + TOSTRING(pos.y) + String(" z: ") + TOSTRING(pos.z), "world.png");
        }

        return;
    }
    else if (args[0] == "goto" && (is_appstate_sim && !is_sim_select))
    {
        if (args.size() != 4)
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, RoR::Color::CommandColour + _L("usage: goto x y z"), "information.png");
            return;
        }

        Vector3 pos = Vector3(PARSEREAL(args[1]), PARSEREAL(args[2]), PARSEREAL(args[3]));

        Actor* b = App::GetSimController()->GetPlayerActor();
        if (!b && gEnv->player)
        {
            gEnv->player->setPosition(pos);
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, _L("Character position set to: ") + String("x: ") + TOSTRING(pos.x) + String(" y: ") + TOSTRING(pos.y) + String(" z: ") + TOSTRING(pos.z), "world.png");
        }
        else if (b)
        {
            b->ResetPosition(pos, false);
            TRIGGER_EVENT(SE_TRUCK_TELEPORT, b->ar_instance_id);
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, _L("Vehicle position set to: ") + String("x: ") + TOSTRING(pos.x) + String(" y: ") + TOSTRING(pos.y) + String(" z: ") + TOSTRING(pos.z), "world.png");
        }

        return;
    }
    else if (args[0] == "terrainheight" && (is_appstate_sim && !is_sim_select))
    {
        if (!App::GetSimTerrain())
            return;
        Vector3 pos = Vector3::ZERO;

        Actor* b = App::GetSimController()->GetPlayerActor();
        if (!b && gEnv->player)
        {
            pos = gEnv->player->getPosition();
        }
        else if (b)
        {
            pos = b->getPosition();
        }

        Real h = App::GetSimTerrain()->GetHeightAt(pos.x, pos.z);
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, _L("Terrain height at position: ") + String("x: ") + TOSTRING(pos.x) + String("z: ") + TOSTRING(pos.z) + _L(" = ") + TOSTRING(h), "world.png");

        return;
    }
    else if (args[0] == "ver")
    {
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_TITLE, "Rigs of Rods:", "information.png");
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, " Version: " + String(ROR_VERSION_STRING), "information.png");
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, " Protocol version: " + String(RORNET_VERSION), "information.png");
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, " build time: " + String(ROR_BUILD_DATE) + ", " + String(ROR_BUILD_TIME), "information.png");
        return;
    }
    else if (args[0] == "quit")
    {
        RoR::App::app_state.SetPending (RoR::AppState::SHUTDOWN);
        return;
    }
#ifdef USE_ANGELSCRIPT
    else if (args[0] == "as" && (is_appstate_sim && !is_sim_select))
    {
        // we want to notify any running scripts that we might change something (prevent cheating)
        ScriptEngine::getSingleton().triggerEvent(SE_ANGELSCRIPT_MANIPULATIONS);

        String command = msg.substr(args[0].length());

        StringUtil::trim(command);
        if (command.empty())
            return;

        String nmsg = RoR::Color::ScriptCommandColour + ">>> " + RoR::Color::NormalColour + command;
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_NOTICE, nmsg, "script_go.png");
        ScriptEngine::getSingleton().executeString(command);
        return;
    }
#endif //ANGELSCRIPT
    else if (args[0] == "log")
    {
        // switch to console logging
        bool now_logging = !App::diag_log_console_echo.GetActive();
        const char* msg = (now_logging) ? " logging to console enabled" : " logging to console disabled";
        RoR::App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L(msg), "information.png");
        App::diag_log_console_echo.SetActive(now_logging);
        return;
    }
    else if (args[0] == "spawnobject" && (is_appstate_sim && !is_sim_select))
    {
        Vector3 pos = Vector3::ZERO;

        if (gEnv->player)
        {
            pos = gEnv->player->getPosition();
        }

        try
        {
            SceneNode* bakeNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
            App::GetSimTerrain()->getObjectManager()->LoadTerrainObject(args[1], pos, Vector3::ZERO, bakeNode, "Console", "");

            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, _L("Spawned object at position: ") + String("x: ") + TOSTRING(pos.x) + String("z: ") + TOSTRING(pos.z), "world.png");
        }
        catch (std::exception e)
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, e.what(), "error.png");
        }

        return;
    }
    else
    {
#ifdef USE_ANGELSCRIPT
        // Just send the complete message to the ScriptEngine
        ScriptEngine::getSingleton().executeString(msg);
#else
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("unknown command: ") + msg, "error.png");
#endif //ANGELSCRIPT
    }
}

