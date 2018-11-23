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


/// @file
/// @date   4th of January 2009
/// @author Thomas Fischer

#include "Settings.h"

#include "Application.h"
#include "Console.h"
#include "ContentManager.h" // RGN_CONFIG
#include "ErrorUtils.h"
#include "Language.h"
#include "PlatformUtils.h"
#include "Utils.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <direct.h> // for _chdir
#include <Windows.h>
#endif

#include <Ogre.h>
#include <sstream>

// simpleopt by http://code.jellycan.com/simpleopt/
// license: MIT
#ifdef _UNICODE
#    undef _UNICODE // We want narrow-string args.
#endif
#include "SimpleOpt.h"

#define CONFIG_FILE_NAME "RoR.cfg"

// option identifiers
enum {
    OPT_HELP,
    OPT_MAP,
    OPT_POS,
    OPT_ROT,
    OPT_TRUCK,
    OPT_WDIR,
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
    { OPT_WDIR,           ("-wd"),          SO_REQ_SEP },
    { OPT_HELP,           ("--help"),       SO_NONE    },
    { OPT_HELP,           ("-help"),        SO_NONE    },
    { OPT_RESUME,         ("-resume"),      SO_NONE    },
    { OPT_CHECKCACHE,     ("-checkcache"),  SO_NONE    },
    { OPT_VER,            ("-version"),     SO_NONE    },
    { OPT_JOINMPSERVER,   ("-joinserver"),  SO_REQ_CMB },
    SO_END_OF_OPTIONS
};

namespace RoR {

void ShowCommandLineUsage()
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

void ShowVersion()
{
    ErrorUtils::ShowInfo(_L("Version Information"), getVersionString());
#ifdef __GNUC__
    printf(" * built with gcc %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif //__GNUC__
}

}

using namespace Ogre;
using namespace RoR;

void Settings::ProcessCommandLine(int argc, char *argv[])
{
    CSimpleOpt args(argc, argv, cmdline_options);

    while (args.Next())
    {
        if (args.LastError() != SO_SUCCESS)
        {
            RoR::App::app_state->SetPendingVal((int)RoR::AppState::PRINT_HELP_EXIT);
            return;
        }
        else if (args.OptionId() == OPT_HELP)
        {
            RoR::App::app_state->SetPendingVal((int)RoR::AppState::PRINT_HELP_EXIT);
            return;
        }
        else if (args.OptionId() == OPT_VER)
        {
            RoR::App::app_state->SetPendingVal((int)RoR::AppState::PRINT_VERSION_EXIT);
            return;
        }
        else if (args.OptionId() == OPT_TRUCK)
        {
            App::diag_preset_vehicle->SetActiveStr(args.OptionArg());
        }
        else if (args.OptionId() == OPT_TRUCKCONFIG)
        {
            App::diag_preset_veh_config->SetActiveStr(args.OptionArg());
        }
        else if (args.OptionId() == OPT_MAP)
        {
            App::diag_preset_terrain->SetActiveStr(args.OptionArg());
        }
        else if (args.OptionId() == OPT_POS)
        {
            App::diag_preset_spawn_pos->SetActiveStr(args.OptionArg());
        }
        else if (args.OptionId() == OPT_ROT)
        {
            App::diag_preset_spawn_rot->SetActiveStr(args.OptionArg());
        }
        else if (args.OptionId() == OPT_WDIR)
        {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            SetCurrentDirectory(args.OptionArg());
#endif
        }
        else if (args.OptionId() == OPT_RESUME)
        {
            if (FileExists(PathCombine(App::sys_savegames_dir->GetActiveStr(), "autosave.sav")))
            {
                App::sim_savegame->SetPendingStr("autosave.sav");
            }
        }
        else if (args.OptionId() == OPT_CHECKCACHE)
        {
            App::app_force_cache_udpate->SetActiveVal(true);
        }
        else if (args.OptionId() == OPT_ENTERTRUCK)
        {
            App::diag_preset_veh_enter->SetActiveVal(true);
        }
        else if (args.OptionId() == OPT_JOINMPSERVER)
        {
            std::string server_args = args.OptionArg();
            const int colon = static_cast<int>(server_args.rfind(":"));
            if (colon != std::string::npos)
            {
                App::mp_join_on_startup->SetActiveVal(true);

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
                App::mp_server_host->SetActiveStr(host_str.c_str());
                App::mp_server_port->SetActiveVal(Ogre::StringConverter::parseInt(port_str));
            }
        }
    }
}

void AssignHelper(CVar* cvar, int val)
{
    Str<25> s; s << val;
    App::GetConsole()->CVarAssign(cvar, s.ToCStr(), CVAR_FORCE_APPLY | CVAR_FORCE_STORE);
}

void Settings::ParseGlobalVarSetting(CVar* cvar, std::string const & val)
{
    if (cvar->GetName() == App::gfx_envmap_rate->GetName())
    {
        int rate = Ogre::StringConverter::parseInt(val);
        if (rate < 0) { rate = 0; }
        if (rate > 6) { rate = 6; }
        AssignHelper(App::gfx_envmap_rate, rate);
    }
    else if (cvar->GetName() == App::gfx_shadow_quality->GetName())
    {
        int quality = Ogre::StringConverter::parseInt(val);
        if (quality < 0) { quality = 0; }
        if (quality > 3) { quality = 3; }
        AssignHelper(App::gfx_shadow_quality, quality);
    }
    else if (cvar->GetName() == App::gfx_shadow_type->GetName())
    {
        AssignHelper(App::gfx_shadow_type, (int)ParseGfxShadowType(val));
    }
    else if (cvar->GetName() == App::gfx_extcam_mode->GetName())
    {
        AssignHelper(App::gfx_extcam_mode, (int)ParseGfxExtCamMode(val));
    }
    else if (cvar->GetName() == App::gfx_texture_filter->GetName())
    {
        AssignHelper(App::gfx_texture_filter, (int)ParseGfxTexFilter(val));
    }
    else if (cvar->GetName() == App::gfx_vegetation_mode->GetName())
    {
        AssignHelper(App::gfx_vegetation_mode, (int)ParseGfxVegetation(val));
    }
    else if (cvar->GetName() == App::gfx_flares_mode->GetName())
    {
        AssignHelper(App::gfx_flares_mode, (int)ParseGfxFlaresMode(val));
    }
    else if (cvar->GetName() == App::gfx_water_mode->GetName())
    {
        AssignHelper(App::gfx_water_mode, (int)ParseGfxWaterMode(val));
    }
    else if (cvar->GetName() == App::gfx_sky_mode->GetName())
    {
        AssignHelper(App::gfx_sky_mode, (int)ParseGfxSkyMode(val));
    }
    else if (cvar->GetName() == App::sim_gearbox_mode->GetName())
    {
        AssignHelper(App::sim_gearbox_mode, (int)ParseSimGearboxMode(val));
    }
    else if (cvar->GetName() == App::gfx_fov_external->GetName() ||
                cvar->GetName() == App::gfx_fov_internal->GetName())
    {
        int fov = Ogre::StringConverter::parseInt(val);
        if (fov >= 10) // FOV shouldn't be below 10
        {
            AssignHelper(cvar, fov);
        }
    }
    else
    {
        App::GetConsole()->CVarAssign(cvar, val, CVAR_FORCE_APPLY | CVAR_FORCE_STORE);
    }
}

void Settings::LoadRoRCfg()
{
    Ogre::ConfigFile cfg;
    try
    {
        std::string path = PathCombine(App::sys_config_dir->GetActiveStr(), CONFIG_FILE_NAME);
        cfg.load(path, "=:\t", /*trimWhitespace=*/true);

        Ogre::ConfigFile::SettingsIterator i = cfg.getSettingsIterator();
        while (i.hasMoreElements())
        {
            std::string cvar_name = RoR::Utils::SanitizeUtf8String(i.peekNextKey());
            CVar* cvar = App::GetConsole()->CVarFind(cvar_name);
            if (cvar && !cvar->HasFlags(CVAR_ALLOW_STORE))
            {
                RoR::LogFormat("[RoR|Settings] CVar '%s' cannot be set from %s (defined without 'allow store' flag)", cvar->GetName(), CONFIG_FILE_NAME);
                i.moveNext();
                continue;
            }

            if (!cvar)
            {
                cvar = App::GetConsole()->CVarGet(cvar_name, CVAR_ALLOW_STORE | CVAR_AUTO_APPLY);
            }

            this->ParseGlobalVarSetting(cvar, RoR::Utils::SanitizeUtf8String(i.peekNextValue()));

            i.moveNext();
        }
    }
    catch (Ogre::FileNotFoundException&) {} // Just continue with defaults...
}

void WriteVarsHelper(std::stringstream& f, const char* label, const char* prefix)
{
    f << std::endl << "; " << label << std::endl;

    for (auto& pair: App::GetConsole()->GetCVars())
    {
        if (pair.second->HasFlags(CVAR_ALLOW_STORE) && pair.first.find(prefix) == 0)
        {
            if (App::app_config_long_names->GetActiveVal<bool>())
            {
                f << pair.second->GetLongName() << "=";
            }
            else
            {
                f << pair.second->GetName() << "=";
            }

                 if (pair.second->GetName() == App::gfx_shadow_type->GetName()     ){ f << GfxShadowTypeToStr(App::gfx_shadow_type     ->GetStoredEnum<GfxShadowType>()); }
            else if (pair.second->GetName() == App::gfx_extcam_mode->GetName()     ){ f << GfxExtCamModeToStr(App::gfx_extcam_mode     ->GetStoredEnum<GfxExtCamMode>()); }
            else if (pair.second->GetName() == App::gfx_texture_filter->GetName()  ){ f << GfxTexFilterToStr (App::gfx_texture_filter  ->GetStoredEnum<GfxTexFilter>());  }
            else if (pair.second->GetName() == App::gfx_vegetation_mode->GetName() ){ f << GfxVegetationToStr(App::gfx_vegetation_mode ->GetStoredEnum<GfxVegetation>()); }
            else if (pair.second->GetName() == App::gfx_flares_mode->GetName()     ){ f << GfxFlaresModeToStr(App::gfx_flares_mode     ->GetStoredEnum<GfxFlaresMode>()); }
            else if (pair.second->GetName() == App::gfx_water_mode->GetName()      ){ f << GfxWaterModeToStr (App::gfx_water_mode      ->GetStoredEnum<GfxWaterMode>());  }
            else if (pair.second->GetName() == App::gfx_sky_mode->GetName()        ){ f << GfxSkyModeToStr   (App::gfx_sky_mode        ->GetStoredEnum<GfxSkyMode>());    }
            else if (pair.second->GetName() == App::sim_gearbox_mode->GetName()    ){ f << SimGearboxModeToStr(App::sim_gearbox_mode->GetStoredEnum<SimGearboxMode>());    }
            else                                                                    { f << pair.second->GetStoredStr(); }

            f << std::endl;
        }
    }    
}

void Settings::SaveSettings()
{
    std::stringstream f;

    f << "; Rigs of Rods configuration file" << std::endl;
    f << "; -------------------------------" << std::endl;
    
    WriteVarsHelper(f, "Application",   "app_");
    WriteVarsHelper(f, "Multiplayer",   "mp_");
    WriteVarsHelper(f, "Simulation",    "sim_");
    WriteVarsHelper(f, "Input/Output",  "io_");
    WriteVarsHelper(f, "Graphics",      "gfx_");
    WriteVarsHelper(f, "Audio",         "audio_");
    WriteVarsHelper(f, "Diagnostics",   "diag_");

    try
    {
        Ogre::DataStreamPtr stream
            = Ogre::ResourceGroupManager::getSingleton().createResource(
                CONFIG_FILE_NAME, RGN_CONFIG, /*overwrite=*/true);
        size_t written = stream->write(f.str().c_str(), f.str().length());
        if (written < f.str().length())
        {
            RoR::LogFormat("[RoR] Error writing file '%s' (resource group '%s'), ",
                           "only written %u out of %u bytes!",
                           CONFIG_FILE_NAME, RGN_CONFIG, written, f.str().length());
        }
    }
    catch (std::exception& e)
    {
        RoR::LogFormat("[RoR|Settings] Error writing file '%s' (resource group '%s'), message: '%s'",
                        CONFIG_FILE_NAME, RGN_CONFIG, e.what());
    }
}

bool Settings::SetupAllPaths()
{
    using namespace RoR;

    // User directories
    App::sys_config_dir    ->SetActiveStr(PathCombine(App::sys_user_dir->GetActiveStr(), "config").c_str());
    App::sys_cache_dir     ->SetActiveStr(PathCombine(App::sys_user_dir->GetActiveStr(), "cache").c_str());
    App::sys_savegames_dir ->SetActiveStr(PathCombine(App::sys_user_dir->GetActiveStr(), "savegames").c_str());
    App::sys_screenshot_dir->SetActiveStr(PathCombine(App::sys_user_dir->GetActiveStr(), "screenshots").c_str());
    App::sys_projects_dir  ->SetActiveStr(PathCombine(App::sys_user_dir->GetActiveStr(), "projects").c_str());

    // Resources dir
    std::string process_dir = PathCombine(App::sys_process_dir->GetActiveStr(), "resources");
    if (FolderExists(process_dir))
    {
        App::sys_resources_dir->SetActiveStr(process_dir.c_str());
        return true;
    }

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    process_dir = "/usr/share/rigsofrods/resources/";
    if (FolderExists(process_dir))
    {
        App::sys_resources_dir->SetActiveStr(process_dir);
        return true;
    }
#endif

    return false;
}
