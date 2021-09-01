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

#include "Console.h"
#include "GameContext.h"
#include "ErrorUtils.h"
#include "PlatformUtils.h"
#include "Utils.h"

using namespace Ogre;
using namespace RoR;

#ifdef _UNICODE
#    undef _UNICODE // We want narrow-string args.
#endif
#include "SimpleOpt.h" // https://github.com/brofield/simpleopt

// option identifiers
enum {
    OPT_HELP,
    OPT_MAP,
    OPT_POS,
    OPT_ROT,
    OPT_TRUCK,
    OPT_VER,
    OPT_RESUME,
    OPT_CHECKCACHE,
    OPT_TRUCKCONFIG,
    OPT_ENTERTRUCK,
    OPT_JOINMPSERVER
};

// option array
CSimpleOpt::SOption cmdline_options[] = {
    { OPT_MAP,            ("-map"),         SO_REQ_SEP },
    { OPT_MAP,            ("-terrain"),     SO_REQ_SEP },
    { OPT_POS,            ("-pos"),         SO_REQ_SEP },
    { OPT_ROT,            ("-rot"),         SO_REQ_SEP },
    { OPT_TRUCK,          ("-truck"),       SO_REQ_SEP },
    { OPT_TRUCKCONFIG,    ("-truckconfig"), SO_REQ_SEP },
    { OPT_ENTERTRUCK,     ("-enter"),       SO_NONE    },
    { OPT_HELP,           ("--help"),       SO_NONE    },
    { OPT_HELP,           ("-help"),        SO_NONE    },
    { OPT_RESUME,         ("-resume"),      SO_NONE    },
    { OPT_CHECKCACHE,     ("-checkcache"),  SO_NONE    },
    { OPT_VER,            ("-version"),     SO_NONE    },
    { OPT_JOINMPSERVER,   ("-joinserver"),  SO_REQ_CMB },
    SO_END_OF_OPTIONS
};

void Console::processCommandLine(int argc, char *argv[])
{
    CSimpleOpt args(argc, argv, cmdline_options);

    while (args.Next())
    {
        if (args.LastError() != SO_SUCCESS)
        {
            App::app_state->SetVal((int)AppState::PRINT_HELP_EXIT);
            return;
        }
        else if (args.OptionId() == OPT_HELP)
        {
            App::app_state->SetVal((int)AppState::PRINT_HELP_EXIT);
            return;
        }
        else if (args.OptionId() == OPT_VER)
        {
            App::app_state->SetVal((int)AppState::PRINT_VERSION_EXIT);
            return;
        }
        else if (args.OptionId() == OPT_TRUCK)
        {
            App::cli_preset_vehicle->SetStr(args.OptionArg());
        }
        else if (args.OptionId() == OPT_TRUCKCONFIG)
        {
            App::cli_preset_veh_config->SetStr(args.OptionArg());
        }
        else if (args.OptionId() == OPT_MAP)
        {
            App::cli_preset_terrain->SetStr(args.OptionArg());
        }
        else if (args.OptionId() == OPT_POS)
        {
            App::cli_preset_spawn_pos->SetStr(args.OptionArg());
        }
        else if (args.OptionId() == OPT_ROT)
        {
            App::cli_preset_spawn_rot->SetStr(args.OptionArg());
        }
        else if (args.OptionId() == OPT_RESUME)
        {
            App::cli_resume_autosave->SetVal(true);
        }
        else if (args.OptionId() == OPT_CHECKCACHE)
        {
            App::cli_force_cache_update->SetVal(true);
        }
        else if (args.OptionId() == OPT_ENTERTRUCK)
        {
            App::cli_preset_veh_enter->SetVal(true);
        }
        else if (args.OptionId() == OPT_JOINMPSERVER)
        {
            std::string server_args = args.OptionArg();
            const int colon = static_cast<int>(server_args.rfind(":"));
            if (colon != std::string::npos)
            {
                std::string host_str;
                std::string port_str;
                if (server_args.find("rorserver://") != String::npos) // Windows URI Scheme retuns rorserver://server:port/
                {
                    host_str = server_args.substr(12, colon - 12);
                    port_str = server_args.substr(colon + 1, server_args.length() - colon - 2);
                }
                else
                {
                    host_str = server_args.substr(0, colon);
                    port_str = server_args.substr(colon + 1, server_args.length());
                }
                App::cli_server_host->SetStr(host_str.c_str());
                App::cli_server_port->SetVal(Ogre::StringConverter::parseInt(port_str));
            }
        }
    }
}

void Console::showCommandLineUsage()
{
    ErrorUtils::ShowInfo(
        _L("Command Line Arguments"),
        _L("--help (this)"                                          "\n"
            "-map <map> (loads map on startup)"                     "\n"
            "-pos <Vect> (overrides spawn position)"                "\n"
            "-rot <float> (overrides spawn rotation)"               "\n"
            "-truck <truck> (loads truck on startup)"               "\n"
            "-truckconfig <section>"                                "\n"
            "-enter (player enters the selected truck)"             "\n"
            "-resume loads previous autosave"                       "\n"
            "-checkcache forces cache update"                       "\n"
            "-version shows the version information"                "\n"
            "-joinserver=<server>:<port> (join multiplayer server)" "\n"
            "For example: RoR.exe -map simple2 -pos '518 0 518' -rot 45 -truck semi.truck -enter"));
}

void Console::showCommandLineVersion()
{
    ErrorUtils::ShowInfo(_L("Version Information"), getVersionString());
#ifdef __GNUC__
    printf(" * built with gcc %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif //__GNUC__
}

