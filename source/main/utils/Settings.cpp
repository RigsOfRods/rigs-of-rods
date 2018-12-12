/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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
#include "Utils.h"

#include <Ogre.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <direct.h> // for _chdir
#include <Windows.h>
#endif

#include "Application.h"
#include "ErrorUtils.h"
#include "Language.h"
#include "PlatformUtils.h"

#include "Utils.h"

// simpleopt by http://code.jellycan.com/simpleopt/
// license: MIT
#ifdef _UNICODE
#    undef _UNICODE // We want narrow-string args.
#endif
#include "SimpleOpt.h"

// option identifiers
enum {
    OPT_HELP,
    OPT_MAP,
    OPT_POS,
    OPT_ROT,
    OPT_TRUCK,
    OPT_SETUP,
    OPT_WDIR,
    OPT_VER,
    OPT_TRUCKCONFIG,
    OPT_ENTERTRUCK,
    OPT_USERPATH,
    OPT_STATE,
    OPT_INCLUDEPATH,
    OPT_NOCACHE,
    OPT_JOINMPSERVER
};

// option array
CSimpleOpt::SOption cmdline_options[] = {
    { OPT_MAP,            ("-map"),         SO_REQ_SEP },
    { OPT_MAP,            ("-terrain"),     SO_REQ_SEP },
    { OPT_POS,            ("-pos"),         SO_REQ_SEP },
    { OPT_ROT,            ("-rot"),         SO_REQ_SEP },
    { OPT_TRUCK,          ("-truck"),       SO_REQ_SEP },
    { OPT_ENTERTRUCK,     ("-enter"),       SO_NONE    },
    { OPT_WDIR,           ("-wd"),          SO_REQ_SEP },
    { OPT_SETUP,          ("-setup"),       SO_NONE    },
    { OPT_TRUCKCONFIG,    ("-actorconfig"), SO_REQ_SEP },
    { OPT_HELP,           ("--help"),       SO_NONE    },
    { OPT_HELP,           ("-help"),        SO_NONE    },
    { OPT_VER,            ("-version"),     SO_NONE    },
    { OPT_USERPATH,       ("-userpath"),    SO_REQ_SEP },
    { OPT_STATE,          ("-state"),       SO_REQ_SEP },
    { OPT_INCLUDEPATH,    ("-includepath"), SO_REQ_SEP },
    { OPT_NOCACHE,        ("-nocache"),     SO_NONE    },
    { OPT_JOINMPSERVER,   ("-joinserver"),  SO_REQ_CMB },
    SO_END_OF_OPTIONS
};

namespace RoR {

void ShowCommandLineUsage()
{
    ErrorUtils::ShowInfo(
        _L("Command Line Arguments"),
        _L("--help (this)"                              "\n"
            "-map <map> (loads map on startup)"         "\n"
            "-pos <Vect> (overrides spawn position)" "\n"
            "-rot <float> (overrides spawn rotation)"   "\n"
            "-truck <truck> (loads truck on startup)"   "\n"
            "-setup shows the ogre configurator"        "\n"
            "-version shows the version information"    "\n"
            "-enter enters the selected truck"          "\n"
            "-userpath <path> sets the user directory"  "\n"
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
            RoR::App::app_state.SetPending(RoR::AppState::PRINT_HELP_EXIT);
            return;
        }
        else if (args.OptionId() == OPT_HELP)
        {
            RoR::App::app_state.SetPending(RoR::AppState::PRINT_HELP_EXIT);
            return;
        }
        else if (args.OptionId() == OPT_VER)
        {
            RoR::App::app_state.SetPending(RoR::AppState::PRINT_VERSION_EXIT);
            return;
        }
        else if (args.OptionId() == OPT_TRUCK)
        {
            App::diag_preset_vehicle.SetActive(args.OptionArg());
        }
        else if (args.OptionId() == OPT_TRUCKCONFIG)
        {
            App::diag_preset_veh_config.SetActive(args.OptionArg());
        }
        else if (args.OptionId() == OPT_MAP)
        {
            App::diag_preset_terrain.SetActive(args.OptionArg());
        }
        else if (args.OptionId() == OPT_POS)
        {
            App::diag_preset_spawn_pos.SetActive(args.OptionArg());
        }
        else if (args.OptionId() == OPT_ROT)
        {
            App::diag_preset_spawn_rot.SetActive(args.OptionArg());
        }
        else if (args.OptionId() == OPT_USERPATH)
        {
            SETTINGS.setSetting("userpath", String(args.OptionArg()));
        }
        else if (args.OptionId() == OPT_WDIR)
        {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            SetCurrentDirectory(args.OptionArg());
#endif
        }
        else if (args.OptionId() == OPT_STATE)
        {
            SETTINGS.setSetting("StartState", args.OptionArg());
        }
        else if (args.OptionId() == OPT_NOCACHE)
        {
            SETTINGS.setSetting("NOCACHE", "Yes");
        }
        else if (args.OptionId() == OPT_INCLUDEPATH)
        {
            App::diag_extra_resource_dir.SetActive(args.OptionArg());
        }
        else if (args.OptionId() == OPT_ENTERTRUCK)
        {
            App::diag_preset_veh_enter.SetActive(true);
        }
        else if (args.OptionId() == OPT_SETUP)
        {
            SETTINGS.setSetting("USE_OGRE_CONFIG", "Yes");
        }
        else if (args.OptionId() == OPT_JOINMPSERVER)
        {
            std::string server_args = args.OptionArg();
            const int colon = static_cast<int>(server_args.rfind(":"));
            if (colon != std::string::npos)
            {
                App::mp_state.SetPending(RoR::MpState::CONNECTED);

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
                App::mp_server_host.SetActive(host_str.c_str());
                App::mp_server_port.SetActive(Ogre::StringConverter::parseInt(port_str));
            }
        }
    }
}

String Settings::getSetting(String key, String defaultValue)
{
    settings_map_t::iterator it = settings.find(key);
    if (it == settings.end())
    {
        setSetting(key, defaultValue);
        return defaultValue;
    }
    return it->second;
}

int Settings::getIntegerSetting(String key, int defaultValue )
{
    settings_map_t::iterator it = settings.find(key);
    if (it == settings.end())
    {
        setSetting(key, TOSTRING(defaultValue));
        return defaultValue;
    }
    return PARSEINT(it->second);
}

float Settings::getFloatSetting(String key, float defaultValue )
{
    settings_map_t::iterator it = settings.find(key);
    if (it == settings.end())
    {
        setSetting(key, TOSTRING(defaultValue));
        return defaultValue;
    }
    return PARSEREAL(it->second);
}

bool Settings::getBooleanSetting(String key, bool defaultValue)
{
    settings_map_t::iterator it = settings.find(key);
    if (it == settings.end())
    {
        setSetting(key, defaultValue ?"Yes":"No");
        return defaultValue;
    }
    String strValue = it->second;
    StringUtil::toLowerCase(strValue);
    return (strValue == "yes");
}

String Settings::getSettingScriptSafe(const String &key)
{
    // hide certain settings for scripts
    if (key == "User Token" || key == "User Token Hash")
        return "permission denied";

    return settings[key];
}

void Settings::setSettingScriptSafe(const String &key, const String &value)
{
    // hide certain settings for scripts
    if (key == "User Token" || key == "User Token Hash")
        return;

    settings[key] = value;
}

void Settings::setSetting(String key, String value)
{
    settings[key] = value;
}

const char* CONF_GFX_SHADOW_TEX     = "Texture shadows";
const char* CONF_GFX_SHADOW_PSSM    = "Parallel-split Shadow Maps";
const char* CONF_GFX_SHADOW_NONE    = "No shadows (fastest)";

const char* CONF_EXTCAM_PITCHING    = "Pitching";
const char* CONF_EXTCAM_STATIC      = "Static";
const char* CONF_EXTCAM_NONE        = "None";

const char* CONF_TEXFILTER_NONE     = "None (fastest)";
const char* CONF_TEXFILTER_BILI     = "Bilinear";
const char* CONF_TEXFILTER_TRILI    = "Trilinear";
const char* CONF_TEXFILTER_ANISO    = "Anisotropic (best looking)";

const char* CONF_VEGET_NONE         = "None (fastest)";
const char* CONF_VEGET_20PERC       = "20%";
const char* CONF_VEGET_50PERC       = "50%";
const char* CONF_VEGET_FULL         = "Full (best looking, slower)";

const char* CONF_GEARBOX_AUTO       = "Automatic shift";
const char* CONF_GEARBOX_SEMIAUTO   = "Manual shift - Auto clutch";
const char* CONF_GEARBOX_MANUAL     = "Fully Manual: sequential shift";
const char* CONF_GEARBOX_MAN_STICK  = "Fully manual: stick shift";
const char* CONF_GEARBOX_MAN_RANGES = "Fully Manual: stick shift with ranges";

const char* CONF_FLARES_NONE        = "None (fastest)";
const char* CONF_FLARES_NO_LIGHT    = "No light sources";
const char* CONF_FLARES_CURR_HEAD   = "Only current vehicle, main lights";
const char* CONF_FLARES_ALL_HEADS   = "All vehicles, main lights";
const char* CONF_FLARES_ALL_LIGHTS  = "All vehicles, all lights";

const char* CONF_WATER_NONE         = "None";
const char* CONF_WATER_BASIC        = "Basic (fastest)";
const char* CONF_WATER_REFLECT      = "Reflection";
const char* CONF_WATER_FULL_FAST    = "Reflection + refraction (speed optimized)";
const char* CONF_WATER_FULL_HQ      = "Reflection + refraction (quality optimized)";
const char* CONF_WATER_HYDRAX       = "Hydrax";

const char* CONF_SKY_CAELUM         = "Caelum (best looking, slower)";
const char* CONF_SKY_SKYX           = "SkyX (best looking, slower)";
const char* CONF_SKY_SANDSTORM      = "Sandstorm (fastest)";

const char* CONF_INPUT_GRAB_DYNAMIC = "Dynamically";
const char* CONF_INPUT_GRAB_NONE    = "None";
const char* CONF_INPUT_GRAB_ALL     = "All";

// ---------------------------- READING CONFIG FILE ------------------------------- //

inline bool CheckIoInputGrabMode(std::string const & key, std::string const & s)
{
    if (key != App::io_input_grab_mode.conf_name)
        return false;

    if (s == CONF_INPUT_GRAB_DYNAMIC) { App::io_input_grab_mode.SetActive(RoR::IoInputGrabMode::DYNAMIC); return true; }
    if (s == CONF_INPUT_GRAB_NONE   ) { App::io_input_grab_mode.SetActive(RoR::IoInputGrabMode::NONE);    return true; }
    else                              { App::io_input_grab_mode.SetActive(RoR::IoInputGrabMode::ALL);     return true; }
}

inline bool CheckShadowTech(std::string const & key, std::string const & s)
{
    if (key != App::gfx_shadow_type.conf_name)
        return false;

    if (s == CONF_GFX_SHADOW_TEX)     { App::gfx_shadow_type.SetActive(GfxShadowType::TEXTURE); return true; }
    if (s == CONF_GFX_SHADOW_PSSM)    { App::gfx_shadow_type.SetActive(GfxShadowType::PSSM   ); return true; }
    else                              { App::gfx_shadow_type.SetActive(GfxShadowType::NONE   ); return true; }
}

inline bool CheckExtcamMode(std::string const & key, std::string const & s)
{
    if (key != App::gfx_extcam_mode.conf_name)
        return false;

    if (s == CONF_EXTCAM_PITCHING)    { App::gfx_extcam_mode.SetActive(GfxExtCamMode::PITCHING); return true; }
    if (s == CONF_EXTCAM_STATIC)      { App::gfx_extcam_mode.SetActive(GfxExtCamMode::STATIC  ); return true; }
    else                              { App::gfx_extcam_mode.SetActive(GfxExtCamMode::NONE    ); return true; }
}

inline bool CheckTexFiltering(std::string const & key, std::string const & s)
{
    if (key != App::gfx_texture_filter.conf_name)
        return false;

    if (s == CONF_TEXFILTER_NONE)     { App::gfx_texture_filter.SetActive(GfxTexFilter::NONE);        return true; }
    if (s == CONF_TEXFILTER_BILI)     { App::gfx_texture_filter.SetActive(GfxTexFilter::BILINEAR);    return true; }
    if (s == CONF_TEXFILTER_TRILI)    { App::gfx_texture_filter.SetActive(GfxTexFilter::TRILINEAR);   return true; }
    if (s == CONF_TEXFILTER_ANISO)    { App::gfx_texture_filter.SetActive(GfxTexFilter::ANISOTROPIC); return true; }
    return true;
}

inline bool CheckVegetationMode(std::string const & key, std::string const & s)
{
    if (key != App::gfx_vegetation_mode.conf_name)
        return false;

    if (s == CONF_VEGET_NONE  )       { App::gfx_vegetation_mode.SetActive(GfxVegetation::NONE);    return true; }
    if (s == CONF_VEGET_20PERC)       { App::gfx_vegetation_mode.SetActive(GfxVegetation::x20PERC); return true; }
    if (s == CONF_VEGET_50PERC)       { App::gfx_vegetation_mode.SetActive(GfxVegetation::x50PERC); return true; }
    if (s == CONF_VEGET_FULL  )       { App::gfx_vegetation_mode.SetActive(GfxVegetation::FULL);    return true; }
    return true;
}

inline bool CheckSimGearboxMode(std::string const & key, std::string const & s)
{
    if (key != App::sim_gearbox_mode.conf_name)
        return false;

    if (s == CONF_GEARBOX_AUTO      ) { App::sim_gearbox_mode.SetActive(SimGearboxMode::AUTO         ); return true; }
    if (s == CONF_GEARBOX_SEMIAUTO  ) { App::sim_gearbox_mode.SetActive(SimGearboxMode::SEMI_AUTO    ); return true; }
    if (s == CONF_GEARBOX_MANUAL    ) { App::sim_gearbox_mode.SetActive(SimGearboxMode::MANUAL       ); return true; }
    if (s == CONF_GEARBOX_MAN_STICK ) { App::sim_gearbox_mode.SetActive(SimGearboxMode::MANUAL_STICK ); return true; }
    if (s == CONF_GEARBOX_MAN_RANGES) { App::sim_gearbox_mode.SetActive(SimGearboxMode::MANUAL_RANGES); return true; }
    return true;
}

inline bool CheckGfxFlaresMode(std::string const & key, std::string const & s)
{
    if (key != App::gfx_flares_mode.conf_name)
        return false;

    if (s == CONF_FLARES_NONE      )  { App::gfx_flares_mode.SetActive(GfxFlaresMode::NONE);                    return true; }
    if (s == CONF_FLARES_NO_LIGHT  )  { App::gfx_flares_mode.SetActive(GfxFlaresMode::NO_LIGHTSOURCES);         return true; }
    if (s == CONF_FLARES_CURR_HEAD )  { App::gfx_flares_mode.SetActive(GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY);  return true; }
    if (s == CONF_FLARES_ALL_HEADS )  { App::gfx_flares_mode.SetActive(GfxFlaresMode::ALL_VEHICLES_HEAD_ONLY);  return true; }
    if (s == CONF_FLARES_ALL_LIGHTS)  { App::gfx_flares_mode.SetActive(GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS); return true; }
    return true;
}

inline bool CheckGfxWaterMode(std::string const & key, std::string const & s)
{
    if (key != App::gfx_water_mode.conf_name)
        return false;

    if (s == CONF_WATER_NONE     )    { App::gfx_water_mode.SetActive(GfxWaterMode::NONE     ); return true; }
    if (s == CONF_WATER_BASIC    )    { App::gfx_water_mode.SetActive(GfxWaterMode::BASIC    ); return true; }
    if (s == CONF_WATER_REFLECT  )    { App::gfx_water_mode.SetActive(GfxWaterMode::REFLECT  ); return true; }
    if (s == CONF_WATER_FULL_FAST)    { App::gfx_water_mode.SetActive(GfxWaterMode::FULL_FAST); return true; }
    if (s == CONF_WATER_FULL_HQ  )    { App::gfx_water_mode.SetActive(GfxWaterMode::FULL_HQ  ); return true; }
    if (s == CONF_WATER_HYDRAX   )    { App::gfx_water_mode.SetActive(GfxWaterMode::HYDRAX   ); return true; }
    return true;
}

inline bool CheckGfxSkyMode(std::string const & key, std::string const & s)
{
    if (key != App::gfx_sky_mode.conf_name)
        return false;

    if (s == CONF_SKY_SANDSTORM)      { App::gfx_sky_mode.SetActive(GfxSkyMode::SANDSTORM); return true; }
    if (s == CONF_SKY_CAELUM   )      { App::gfx_sky_mode.SetActive(GfxSkyMode::CAELUM);    return true; }
    if (s == CONF_SKY_SKYX     )      { App::gfx_sky_mode.SetActive(GfxSkyMode::SKYX);      return true; }
    return true;
}

inline bool CheckScreenshotFormat(std::string const & key, std::string const & s)
{
    if (key != App::app_screenshot_format.conf_name)
        return false;

    if (s.size() >= 3) { App::app_screenshot_format.SetActive(s.substr(0, 3).c_str()); }
    return true;
}

inline bool CheckEnvmapRate(std::string const & key, std::string const & s)
{
    if (key != App::gfx_envmap_rate.conf_name)
        return false;

    int rate = Ogre::StringConverter::parseInt(s);
    if (rate < 0) { rate = 0; }
    if (rate > 6) { rate = 6; }
    App::gfx_envmap_rate.SetActive(rate);
    return true;
}

inline bool CheckShadowQuality(std::string const & key, std::string const & s)
{
    if (key != App::gfx_envmap_rate.conf_name)
        return false;

    int quality = Ogre::StringConverter::parseInt(s);
    if (quality < 0) { quality = 0; }
    if (quality > 3) { quality = 3; }
    App::gfx_shadow_quality.SetActive(quality);
    return true;
}

template <typename GVarStr_T>
inline bool CheckStr(GVarStr_T& gvar, std::string const & key, std::string const & value)
{
    if (key == gvar.conf_name)
    {
        gvar.SetActive(value.c_str());
        return true;
    }
    return false;
}

template <typename GVarStr_T>
inline bool CheckStrAS(GVarStr_T& gvar, std::string const & key, std::string const & value)
{
    if (key == gvar.conf_name)
    {
        gvar.SetActive(value.c_str());
        gvar.SetStored(value.c_str());
        return true;
    }
    return false;
}

inline bool CheckInt(GVarPod_A<int>& gvar, std::string const & key, std::string const & value)
{
    if (key == gvar.conf_name)
    {
        gvar.SetActive(Ogre::StringConverter::parseInt(value));
        return true;
    }
    return false;
}

inline bool CheckInt(GVarPod_APS<int>& gvar, std::string const & key, std::string const & value)
{
    if (key == gvar.conf_name)
    {
        int val = Ogre::StringConverter::parseInt(value);
        gvar.SetActive(val);
        gvar.SetStored(val);
        return true;
    }
    return false;
}

inline bool CheckBool(GVarPod_A<bool>& gvar, std::string const & key, std::string const & value)
{
    if (key == gvar.conf_name)
    {
        gvar.SetActive(Ogre::StringConverter::parseBool(value));
        return true;
    }
    return false;
}

inline bool CheckBool(GVarPod_APS<bool>& gvar, std::string const & key, std::string const & value)
{
    if (key == gvar.conf_name)
    {
        bool val = Ogre::StringConverter::parseBool(value);
        gvar.SetActive(val);
        gvar.SetStored(val);
        return true;
    }
    return false;
}

inline bool CheckB2I(GVarPod_A<int>& gvar, std::string const & key, std::string const & value) // bool to int
{
    if (key == gvar.conf_name)
    {
        gvar.SetActive(Ogre::StringConverter::parseBool(value) ? 1 : 0);
        return true;
    }
    return false;
}

inline bool CheckFloat(GVarPod_A<float>& gvar, std::string const & key, std::string const & value)
{
    if (key == gvar.conf_name)
    {
        gvar.SetActive(static_cast<float>(Ogre::StringConverter::parseReal(value)));
        return true;
    }
    return false;
}

inline bool CheckFloat(GVarPod_APS<float>& gvar, std::string const & key, std::string const & value)
{
    if (key == gvar.conf_name)
    {
        float val = static_cast<float>(Ogre::StringConverter::parseReal(value));
        gvar.SetActive(val);
        gvar.SetStored(val);
        return true;
    }
    return false;
}

inline bool CheckSpeedoImperial(std::string const & key, std::string const & value)
{
    if (key == "SpeedUnit") // String ["Metric"/"Imperial"], decommissioned 10/2017
    {
        App::gfx_speedo_imperial.SetActive(value == "Imperial");
        return true;
    }
    return CheckBool(App::gfx_speedo_imperial, key, value);
}
inline bool CheckFOV(GVarPod_APS<int>& gvar, std::string const & key, std::string const & value)
{
    if (key == gvar.conf_name)
    {
        int val = Ogre::StringConverter::parseInt(value);

        // FOV shouldn't be below 10
        if (val < 10) return false;

        gvar.SetActive (val);
        gvar.SetStored (val);
        return true;
    }
    return false;
}

bool Settings::ParseGlobalVarSetting(std::string const & k, std::string const & v)
{
    // Process and erase settings which propagate to global vars.

    // Multiplayer
    if (CheckBool (App::mp_join_on_startup,        k, v)) { return true; }
    if (CheckBool (App::mp_chat_auto_hide,         k, v)) { return true; }
    if (CheckBool (App::mp_hide_net_labels,        k, v)) { return true; }
    if (CheckBool (App::mp_hide_own_net_label,     k, v)) { return true; }
    if (CheckStr  (App::mp_player_name,            k, v)) { return true; }
    if (CheckStr  (App::mp_server_host,            k, v)) { return true; }
    if (CheckInt  (App::mp_server_port,            k, v)) { return true; }
    if (CheckStr  (App::mp_server_password,        k, v)) { return true; }
    if (CheckStr  (App::mp_portal_url,             k, v)) { return true; }
    if (CheckStr  (App::mp_player_token_hash,      k, v)) { return true; }
    // App
    if (CheckScreenshotFormat                     (k, v)) { return true; }
    if (CheckStr  (App::app_locale,                k, v)) { return true; }
    if (CheckStr  (App::app_desired_render_sys,    k, v)) { return true; }
    if (CheckBool (App::app_skip_main_menu,        k, v)) { return true; }
    if (CheckBool (App::app_async_physics,         k, v)) { return true; }
    if (CheckInt  (App::app_num_workers,           k, v)) { return true; }
    // Input&Output
    if (CheckBool (App::io_ffb_enabled,            k, v)) { return true; }
    if (CheckFloat(App::io_ffb_camera_gain,        k, v)) { return true; }
    if (CheckFloat(App::io_ffb_center_gain,        k, v)) { return true; }
    if (CheckFloat(App::io_ffb_master_gain,        k, v)) { return true; }
    if (CheckFloat(App::io_ffb_stress_gain,        k, v)) { return true; }
    if (CheckBool (App::io_arcade_controls,        k, v)) { return true; }
    if (CheckInt  (App::io_outgauge_mode,          k, v)) { return true; }
    if (CheckStr  (App::io_outgauge_ip,            k, v)) { return true; }
    if (CheckInt  (App::io_outgauge_port,          k, v)) { return true; }
    if (CheckFloat(App::io_outgauge_delay,         k, v)) { return true; }
    if (CheckInt  (App::io_outgauge_id,            k, v)) { return true; }
    if (CheckIoInputGrabMode                      (k, v)) { return true; }
    // Gfx
    if (CheckEnvmapRate                           (k, v)) { return true; }
    if (CheckShadowQuality                        (k, v)) { return true; }
    if (CheckShadowTech                           (k, v)) { return true; }
    if (CheckExtcamMode                           (k, v)) { return true; }
    if (CheckTexFiltering                         (k, v)) { return true; }
    if (CheckVegetationMode                       (k, v)) { return true; }
    if (CheckGfxFlaresMode                        (k, v)) { return true; }
    if (CheckGfxWaterMode                         (k, v)) { return true; }
    if (CheckGfxSkyMode                           (k, v)) { return true; }
    if (CheckSpeedoImperial                       (k, v)) { return true; }
    if (CheckInt  (App::gfx_anisotropy,            k, v)) { return true; }
    if (CheckBool (App::gfx_water_waves,           k, v)) { return true; }
    if (CheckBool (App::gfx_minimap_enabled,       k, v)) { return true; }
    if (CheckB2I  (App::gfx_particles_mode,        k, v)) { return true; }
    if (CheckBool (App::gfx_enable_videocams,      k, v)) { return true; }
    if (CheckB2I  (App::gfx_skidmarks_mode,        k, v)) { return true; }
    if (CheckBool (App::gfx_envmap_enabled,        k, v)) { return true; }
    if (CheckInt  (App::gfx_sight_range,           k, v)) { return true; }
    if (CheckFOV  (App::gfx_fov_external,          k, v)) { return true; }
    if (CheckFOV  (App::gfx_fov_internal,          k, v)) { return true; }
    if (CheckInt  (App::gfx_fps_limit,             k, v)) { return true; }
    if (CheckBool (App::gfx_speedo_digital,        k, v)) { return true; }
    if (CheckBool (App::gfx_flexbody_lods,         k, v)) { return true; }
    if (CheckBool (App::gfx_flexbody_cache,        k, v)) { return true; }
    if (CheckBool (App::gfx_reduce_shadows,        k, v)) { return true; }
    if (CheckBool (App::gfx_enable_rtshaders,      k, v)) { return true; }
    // Audio
    if (CheckFloat(App::audio_master_volume,       k, v)) { return true; }
    if (CheckBool (App::audio_enable_creak,        k, v)) { return true; }
    if (CheckBool (App::audio_menu_music,          k, v)) { return true; }
    if (CheckStr  (App::audio_device_name,         k, v)) { return true; }
    // Diag
    if (CheckBool (App::diag_auto_spawner_report,  k, v)) { return true; }
    if (CheckBool (App::diag_camera,               k, v)) { return true; }
    if (CheckBool (App::diag_rig_log_node_import,  k, v)) { return true; }
    if (CheckBool (App::diag_rig_log_node_stats,   k, v)) { return true; }
    if (CheckBool (App::diag_rig_log_messages,     k, v)) { return true; }
    if (CheckBool (App::diag_collisions,           k, v)) { return true; }
    if (CheckBool (App::diag_truck_mass,           k, v)) { return true; }
    if (CheckBool (App::diag_envmap,               k, v)) { return true; }
    if (CheckBool (App::diag_videocameras,         k, v)) { return true; }
    if (CheckBool (App::diag_log_console_echo,     k, v)) { return true; }
    if (CheckBool (App::diag_log_beam_break,       k, v)) { return true; }
    if (CheckBool (App::diag_log_beam_deform,      k, v)) { return true; }
    if (CheckBool (App::diag_log_beam_trigger,     k, v)) { return true; }
    if (CheckStrAS(App::diag_preset_terrain,       k, v)) { return true; }
    if (CheckStrAS(App::diag_preset_vehicle,       k, v)) { return true; }
    if (CheckStr  (App::diag_preset_veh_config,    k, v)) { return true; }
    if (CheckBool (App::diag_preset_veh_enter,     k, v)) { return true; }
    if (CheckStr  (App::diag_extra_resource_dir,   k, v)) { return true; }
    if (CheckBool (App::diag_simple_materials,     k, v)) { return true; }
    // Sim
    if (CheckSimGearboxMode                       (k, v)) { return true; }
    if (CheckBool (App::sim_replay_enabled,        k, v)) { return true; }
    if (CheckInt  (App::sim_replay_length,         k, v)) { return true; }
    if (CheckInt  (App::sim_replay_stepping,       k, v)) { return true; }
    if (CheckBool (App::sim_position_storage,      k, v)) { return true; }
    if (CheckBool (App::sim_no_collisions,         k, v)) { return true; }
    if (CheckBool (App::sim_no_self_collisions,    k, v)) { return true; }

    return false;
}

void Settings::LoadRoRCfg()
{
    Ogre::ConfigFile cfg;
    try
    {
        std::string path = PathCombine(App::sys_config_dir.GetActive(), "RoR.cfg");
        cfg.load(Ogre::String(path), "=:\t", false);

        // load all settings into a map!
        Ogre::ConfigFile::SettingsIterator i = cfg.getSettingsIterator();
        String s_value, s_name;
        while (i.hasMoreElements())
        {
            s_name  = RoR::Utils::SanitizeUtf8String(i.peekNextKey());
            s_value = RoR::Utils::SanitizeUtf8String(i.getNext());

            // Purge unwanted entries
            if (s_name == "Program Path") { continue; }

            // Process and clear GVar values
            if (this->ParseGlobalVarSetting(s_name, s_value))
            {
                continue;
            }

            settings[s_name] = s_value;
        }
    }
    catch (Ogre::FileNotFoundException&) {} // Just continue with defaults...
}

// --------------------------- WRITING CONFIG FILE ------------------------------- //

inline const char* IoInputGrabToStr(IoInputGrabMode v)
{
    switch (v)
    {
    case IoInputGrabMode::DYNAMIC: return CONF_INPUT_GRAB_DYNAMIC;
    case IoInputGrabMode::NONE   : return CONF_INPUT_GRAB_NONE;
    case IoInputGrabMode::ALL    : return CONF_INPUT_GRAB_ALL;
    default                      : return "";
    }
}

inline const char* GfxShadowTechToStr(GfxShadowType v)
{
    switch (v)
    {
    case GfxShadowType::TEXTURE: return CONF_GFX_SHADOW_TEX;
    case GfxShadowType::PSSM   : return CONF_GFX_SHADOW_PSSM;
    case GfxShadowType::NONE   : return CONF_GFX_SHADOW_NONE;
    default                    : return "";
    }
}

inline const char* GfxExtcamModeToStr(GfxExtCamMode v)
{
    switch (v)
    {
    case GfxExtCamMode::PITCHING: return CONF_EXTCAM_PITCHING;
    case GfxExtCamMode::STATIC  : return CONF_EXTCAM_STATIC;
    case GfxExtCamMode::NONE    : return CONF_EXTCAM_NONE;
    default                     : return "";
    }
}

inline const char* GfxTexFilterToStr(GfxTexFilter v)
{
    switch (v)
    {
    case GfxTexFilter::NONE       : return CONF_TEXFILTER_NONE;
    case GfxTexFilter::BILINEAR   : return CONF_TEXFILTER_BILI;
    case GfxTexFilter::TRILINEAR  : return CONF_TEXFILTER_TRILI;
    case GfxTexFilter::ANISOTROPIC: return CONF_TEXFILTER_ANISO;
    default                       : return "";
    }
}

inline const char* GfxVegetationToStr(GfxVegetation v)
{
    switch (v)
    {
    case GfxVegetation::NONE   : return CONF_VEGET_NONE;
    case GfxVegetation::x20PERC: return CONF_VEGET_20PERC;
    case GfxVegetation::x50PERC: return CONF_VEGET_50PERC;
    case GfxVegetation::FULL   : return CONF_VEGET_FULL;
    default                    : return "";
    }
}

inline const char* SimGearboxToStr(SimGearboxMode v)
{
    switch (v)
    {
    case SimGearboxMode::AUTO         : return CONF_GEARBOX_AUTO;
    case SimGearboxMode::SEMI_AUTO    : return CONF_GEARBOX_SEMIAUTO;
    case SimGearboxMode::MANUAL       : return CONF_GEARBOX_MANUAL;
    case SimGearboxMode::MANUAL_STICK : return CONF_GEARBOX_MAN_STICK;
    case SimGearboxMode::MANUAL_RANGES: return CONF_GEARBOX_MAN_RANGES;
    default                           : return "";
    }
}

inline const char* GfxFlaresToStr(GfxFlaresMode v)
{
    switch(v)
    {
    case GfxFlaresMode::NONE                   : return CONF_FLARES_NONE;
    case GfxFlaresMode::NO_LIGHTSOURCES        : return CONF_FLARES_NO_LIGHT;
    case GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY : return CONF_FLARES_CURR_HEAD;
    case GfxFlaresMode::ALL_VEHICLES_HEAD_ONLY : return CONF_FLARES_ALL_HEADS;
    case GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS: return CONF_FLARES_ALL_LIGHTS;
    default                                    : return "";
    }
}

inline const char* GfxWaterToStr(GfxWaterMode v)
{
    switch(v)
    {
    case GfxWaterMode::BASIC    : return CONF_WATER_BASIC;
    case GfxWaterMode::REFLECT  : return CONF_WATER_REFLECT;
    case GfxWaterMode::FULL_FAST: return CONF_WATER_FULL_FAST;
    case GfxWaterMode::FULL_HQ  : return CONF_WATER_FULL_HQ;
    case GfxWaterMode::HYDRAX   : return CONF_WATER_HYDRAX;
    default                     : return "";
    }
}

inline const char* GfxSkyToStr(GfxSkyMode v)
{
    switch(v)
    {
    case GfxSkyMode::CAELUM   : return CONF_SKY_CAELUM;
    case GfxSkyMode::SKYX     : return CONF_SKY_SKYX;
    case GfxSkyMode::SANDSTORM: return CONF_SKY_SANDSTORM;
    default                   : return "";
    }
}

template <typename GVarStr_T>
inline void WriteStr(std::ofstream& f, GVarStr_T& gvar)
{
    f << gvar.conf_name << "=" << gvar.GetStored() << std::endl;
}

template <typename T>
inline void WritePod(std::ofstream& f, GVarPod_A<T>& gvar)
{
    f << gvar.conf_name << "=" << gvar.GetActive() << std::endl;
}

template <typename T>
inline void WritePod(std::ofstream& f, GVarPod_APS<T>& gvar)
{
    f << gvar.conf_name << "=" << gvar.GetStored() << std::endl;
}

template <typename GVarPod_T>
inline void WriteYN(std::ofstream& f, GVarPod_T& gvar) ///< Writes "Yes/No" - handles `bool` and `int(1/0)`
{
    f << gvar.conf_name << "=" << (static_cast<int>(gvar.GetActive()) == 0 ? "No" : "Yes") << std::endl;
}

template <typename T>
inline void WriteYN(std::ofstream& f, GVarPod_APS<T>& gvar) ///< Writes "Yes/No" - handles `bool` and `int(1/0)`
{
    f << gvar.conf_name << "=" << (static_cast<int>(gvar.GetStored()) == 0 ? "No" : "Yes") << std::endl;
}

inline void WriteAny(std::ofstream& f, const char* name, const char* value)
{
    f << name << "=" << value << std::endl;
}

void Settings::SaveSettings()
{
    std::string rorcfg_path = PathCombine(App::sys_config_dir.GetActive(), "RoR.cfg");
    std::ofstream f(rorcfg_path);
    if (!f.is_open())
    {
        LOG("[RoR] Failed to save RoR.cfg: Could not open file");
        return;
    }

    f << "; Rigs of Rods configuration file" << std::endl;
    f << "; -------------------------------" << std::endl;

    f << std::endl << "; Multiplayer" << std::endl;
    WriteYN  (f, App::mp_join_on_startup);
    WriteYN  (f, App::mp_chat_auto_hide);
    WriteYN  (f, App::mp_hide_net_labels);
    WriteYN  (f, App::mp_hide_own_net_label);
    WriteStr (f, App::mp_player_name);
    WriteStr (f, App::mp_server_host);
    WritePod (f, App::mp_server_port);
    WriteStr (f, App::mp_server_password);
    WriteStr (f, App::mp_portal_url);

    f << std::endl << "; Simulation" << std::endl;
    WriteAny (f, App::sim_gearbox_mode.conf_name, SimGearboxToStr(App::sim_gearbox_mode.GetActive()));
    WriteYN  (f, App::sim_replay_enabled    );
    WritePod (f, App::sim_replay_length     );
    WritePod (f, App::sim_replay_stepping   );
    WriteYN  (f, App::sim_position_storage  );
    WriteYN  (f, App::sim_no_collisions     );
    WriteYN  (f, App::sim_no_self_collisions);

    f << std::endl << "; Input/Output" << std::endl;
    WriteAny (f, App::io_input_grab_mode.conf_name, IoInputGrabToStr(App::io_input_grab_mode.GetActive()));
    WriteYN  (f, App::io_ffb_enabled);
    WritePod (f, App::io_ffb_camera_gain);
    WritePod (f, App::io_ffb_center_gain);
    WritePod (f, App::io_ffb_master_gain);
    WritePod (f, App::io_ffb_stress_gain);
    WriteYN  (f, App::io_arcade_controls);
    WritePod (f, App::io_outgauge_mode);
    WriteStr (f, App::io_outgauge_ip);
    WritePod (f, App::io_outgauge_port);
    WritePod (f, App::io_outgauge_delay);
    WritePod (f, App::io_outgauge_id);

    f << std::endl << "; Graphics" << std::endl;
    WriteAny (f, App::gfx_shadow_type.conf_name    , GfxShadowTechToStr(App::gfx_shadow_type.GetActive     ()));
    WriteAny (f, App::gfx_extcam_mode.conf_name    , GfxExtcamModeToStr(App::gfx_extcam_mode.GetActive     ()));
    WriteAny (f, App::gfx_texture_filter.conf_name , GfxTexFilterToStr (App::gfx_texture_filter.GetActive  ()));
    WriteAny (f, App::gfx_vegetation_mode.conf_name, GfxVegetationToStr(App::gfx_vegetation_mode.GetActive ()));
    WriteAny (f, App::gfx_flares_mode.conf_name    , GfxFlaresToStr    (App::gfx_flares_mode.GetActive     ()));
    WriteAny (f, App::gfx_water_mode.conf_name     , GfxWaterToStr     (App::gfx_water_mode.GetActive      ()));
    WriteAny (f, App::gfx_sky_mode.conf_name       , GfxSkyToStr       (App::gfx_sky_mode.GetActive        ()));
    WritePod (f, App::gfx_anisotropy      );
    WriteYN  (f, App::gfx_enable_videocams);
    WriteYN  (f, App::gfx_water_waves     );
    WriteYN  (f, App::gfx_minimap_enabled );
    WriteYN  (f, App::gfx_particles_mode  );
    WriteYN  (f, App::gfx_skidmarks_mode  );
    WriteYN  (f, App::gfx_speedo_digital  );
    WriteYN  (f, App::gfx_speedo_imperial );
    WriteYN  (f, App::gfx_envmap_enabled  );
    WritePod (f, App::gfx_envmap_rate     );
    WritePod (f, App::gfx_shadow_quality  );
    WritePod (f, App::gfx_sight_range     );
    WritePod (f, App::gfx_fps_limit       );
    WritePod (f, App::gfx_fov_external    );
    WritePod (f, App::gfx_fov_internal    );
    WriteYN  (f, App::gfx_flexbody_lods   );
    WriteYN  (f, App::gfx_flexbody_cache  );
    WriteYN  (f, App::gfx_reduce_shadows  );
    WriteYN  (f, App::gfx_enable_rtshaders);

    f << std::endl << "; Audio" << std::endl;
    WritePod (f, App::audio_master_volume);
    WriteYN  (f, App::audio_enable_creak );
    WriteYN  (f, App::audio_menu_music   );
    WriteStr (f, App::audio_device_name  );

    f << std::endl << "; Diagnostics" << std::endl;
    WriteYN  (f, App::diag_auto_spawner_report);
    WriteYN  (f, App::diag_camera             );
    WriteYN  (f, App::diag_rig_log_node_import);
    WriteYN  (f, App::diag_rig_log_node_stats );
    WriteYN  (f, App::diag_rig_log_messages   );
    WriteYN  (f, App::diag_collisions         );
    WriteYN  (f, App::diag_truck_mass         );
    WriteYN  (f, App::diag_envmap             );
    WriteYN  (f, App::diag_log_console_echo   );
    WriteYN  (f, App::diag_log_beam_break     );
    WriteYN  (f, App::diag_log_beam_deform    );
    WriteYN  (f, App::diag_log_beam_trigger   );
    WriteYN  (f, App::diag_preset_veh_enter   );
    WriteStr (f, App::diag_preset_terrain     );
    WriteStr (f, App::diag_preset_vehicle     );
    WriteStr (f, App::diag_preset_veh_config  );
    WriteYN  (f, App::diag_videocameras       );
    WriteYN  (f, App::diag_simple_materials   );

    f << std::endl << "; Application"<< std::endl;
    WriteStr (f, App::app_screenshot_format );
    WriteStr (f, App::app_locale            );
    WriteStr (f, App::app_desired_render_sys);
    WriteYN  (f, App::app_skip_main_menu    );
    WriteYN  (f, App::app_async_physics     );
    WritePod (f, App::app_num_workers       );

    // Append misc legacy entries
    f << std::endl << "; Misc" << std::endl;
    for (auto itor = settings.begin(); itor != settings.end(); ++itor)
    {
        f << itor->first << "=" << itor->second << std::endl;
    }

    f.close();
}

bool Settings::SetupAllPaths()
{
    using namespace RoR;

    // User directories
    App::sys_config_dir    .SetActive(PathCombine(App::sys_user_dir.GetActive(), "config").c_str());
    App::sys_cache_dir     .SetActive(PathCombine(App::sys_user_dir.GetActive(), "cache").c_str());
    App::sys_screenshot_dir.SetActive(PathCombine(App::sys_user_dir.GetActive(), "screenshots").c_str());

    // Resources dir
    std::string process_dir = PathCombine(App::sys_process_dir.GetActive(), "resources");
    if (FolderExists(process_dir))
    {
        App::sys_resources_dir.SetActive(process_dir.c_str());
        return true;
    }

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    process_dir = "/usr/share/rigsofrods/resources/";
    if (FolderExists(process_dir))
    {
        App::sys_resources_dir.SetActive(process_dir.c_str());
        return true;
    }
#endif

    return false;
}
