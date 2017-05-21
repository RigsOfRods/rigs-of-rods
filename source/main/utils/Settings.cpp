/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal

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

/**
    @file
    @date   4th of January 2009
    @author Thomas Fischer
*/

#include "Settings.h"
#include "Utils.h"

#include <Ogre.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <shlobj.h> // for CoCreateGuid and SHGetFolderPathA
#include <direct.h> // for _chdir
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include <mach-o/dyld.h>
#endif

#include "Application.h"
#include "ErrorUtils.h"
#include "Language.h"
#include "PlatformUtils.h"
#include "RoRVersion.h"
#include "SHA1.h"
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
    OPT_TRUCK,
    OPT_SETUP,
    OPT_WDIR,
    OPT_VER,
    OPT_CHECKCACHE,
    OPT_TRUCKCONFIG,
    OPT_ENTERTRUCK,
    OPT_USERPATH,
    OPT_STATE,
    OPT_INCLUDEPATH,
    OPT_ADVLOG,
    OPT_NOCACHE,
    OPT_JOINMPSERVER
};

// option array
CSimpleOpt::SOption cmdline_options[] = {
    { OPT_MAP,            ("-map"),         SO_REQ_SEP },
    { OPT_MAP,            ("-terrain"),     SO_REQ_SEP },
    { OPT_TRUCK,          ("-truck"),       SO_REQ_SEP },
    { OPT_ENTERTRUCK,     ("-enter"),       SO_NONE    },
    { OPT_WDIR,           ("-wd"),          SO_REQ_SEP },
    { OPT_SETUP,          ("-setup"),       SO_NONE    },
    { OPT_TRUCKCONFIG,    ("-truckconfig"), SO_REQ_SEP },
    { OPT_HELP,           ("--help"),       SO_NONE    },
    { OPT_HELP,           ("-help"),        SO_NONE    },
    { OPT_CHECKCACHE,     ("-checkcache"),  SO_NONE    },
    { OPT_VER,            ("-version"),     SO_NONE    },
    { OPT_USERPATH,       ("-userpath"),    SO_REQ_SEP },
    { OPT_ADVLOG,         ("-advlog"),      SO_NONE    },
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
            "-truck <truck> (loads truck on startup)"   "\n"
            "-setup shows the ogre configurator"        "\n"
            "-version shows the version information"    "\n"
            "-enter enters the selected truck"          "\n"
            "-userpath <path> sets the user directory"  "\n"
            "For example: RoR.exe -map oahu -truck semi"));
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

bool FileExists(const char *path)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    DWORD attributes = GetFileAttributesA(path);
    return (attributes != INVALID_FILE_ATTRIBUTES && ! (attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0);
#endif
}

bool FolderExists(const char *path)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    DWORD attributes = GetFileAttributesA(path);
    return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0);
#endif
}

bool FileExists(Ogre::String const & path)
{
    return FileExists(path.c_str());
}

bool FolderExists(Ogre::String const & path)
{
    return FolderExists(path.c_str());
}

namespace RoR{
namespace System {

inline bool IsWhitespace(char c) { return (c == ' ' || c == '\n' || c == '\t'); }
inline bool IsSeparator (char c) { return (c == '\\' || c == '/'); }

void GetParentDirectory(char* dst_buf, const char* src_buff)
{
    const char* start = src_buff;
    size_t count = strlen(src_buff);
    // Trim trailing separator(s)
    for (;;)
    {
        if (count == 0) { dst_buf[0] = '\0'; return; }
        if (!IsSeparator(start[count - 1])) { break; }
        --count;
    }
    // Remove last path entry
    for (;;)
    {
        if (count == 0) { dst_buf[0] = '\0'; return; }
        if (IsSeparator(start[count - 1])) {break; }
        --count;
    }
    // Trim rear separator(s)
    for (;;)
    {
        if (count == 0) { dst_buf[0] = '\0'; return; }
        if (!IsSeparator(start[count - 1])) { break; }
        --count;
    }
    strncpy(dst_buf, start, count);
}

int DetectBasePaths()
{
    char buf[500] = "";

    // Process dir (system)    
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 // NOTE: We use non-UNICODE interfaces for simplicity
    // Process dir
    if (!GetModuleFileNameA(nullptr, buf, 1000))
    {
        return -1;
    }
    
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX // http://stackoverflow.com/a/625523
    // Process dir
    if (readlink("/proc/self/exe", buf, 1000) == -1)
    {
        return -1;
    }
    
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    // Process dir
    uint32_t length = 1000;
    if (_NSGetExecutablePath(procpath, &length) == -1) // Returns absolute path to binary
    {
        return -1;
    } 
#endif
    GStr<500> process_dir;
    GetParentDirectory(process_dir.GetBuffer(), buf);
    App::sys_process_dir.SetActive(process_dir);

    // User directory (local override - portable installation)
    GStr<500> local_userdir;
    local_userdir << App::sys_process_dir.GetActive() << PATH_SLASH << "config";
    if (FolderExists(local_userdir))
    {
        App::sys_user_dir.SetActive(local_userdir);
        return 0; // Done!
    }

    // User directory (system)
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 // NOTE: We use non-UNICODE interfaces for simplicity	
    if (SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, buf) != S_OK)
    {
        return -2;
    }
    sprintf(buf, "%s\\Rigs of Rods %s", buf, ROR_VERSION_STRING_SHORT);
     
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    snprintf(buf, 1000, "%s/.rigsofrods", getenv("HOME"));

#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    snprintf(buf, 1000, "%s/RigsOfRods", getenv("HOME"));

#endif

    App::sys_user_dir.SetActive(buf);
    return 0;
}

} // namespace System
} // namespace RoR

//RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED

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
        else if (args.OptionId() == OPT_ADVLOG) 
        {
            SETTINGS.setSetting("Advanced Logging", "Yes");
        }
        else if (args.OptionId() == OPT_INCLUDEPATH) 
        {
            SETTINGS.setSetting("resourceIncludePath", args.OptionArg());
        } 
        else if (args.OptionId() == OPT_CHECKCACHE) 
        {
            // just regen cache and exit
            SETTINGS.setSetting("regen-cache-only", "Yes");
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
            const int colon = server_args.rfind(":");
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

UTFString Settings::getUTFSetting(UTFString key, UTFString defaultValue)
{
    return getSetting(key, defaultValue);
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

void Settings::setUTFSetting(UTFString key, UTFString value)
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

void App__SetIoInputGrabMode(std::string const & s)
{
    if (s == CONF_INPUT_GRAB_DYNAMIC) { App::io_input_grab_mode.SetActive(RoR::IoInputGrabMode::DYNAMIC); return; }
    if (s == CONF_INPUT_GRAB_NONE   ) { App::io_input_grab_mode.SetActive(RoR::IoInputGrabMode::NONE);    return; }
    else                              { App::io_input_grab_mode.SetActive(RoR::IoInputGrabMode::ALL);     return; }
}

void App__SetShadowTech(std::string const & s)
{
    if (s == CONF_GFX_SHADOW_TEX)     { App::gfx_shadow_type.SetActive(GfxShadowType::TEXTURE); return; }
    if (s == CONF_GFX_SHADOW_PSSM)    { App::gfx_shadow_type.SetActive(GfxShadowType::PSSM   ); return; }
    else                              { App::gfx_shadow_type.SetActive(GfxShadowType::NONE   ); return; }
}

void App__SetExtcamMode(std::string const & s)
{
    if (s == CONF_EXTCAM_PITCHING)    { App::gfx_extcam_mode.SetActive(GfxExtCamMode::PITCHING); return; }
    if (s == CONF_EXTCAM_STATIC)      { App::gfx_extcam_mode.SetActive(GfxExtCamMode::STATIC  ); return; }
    else                              { App::gfx_extcam_mode.SetActive(GfxExtCamMode::NONE    ); return; }
}

void App__SetTexFiltering(std::string const & s)
{
    if (s == CONF_TEXFILTER_NONE)     { App::gfx_texture_filter.SetActive(GfxTexFilter::NONE);        return; }
    if (s == CONF_TEXFILTER_BILI)     { App::gfx_texture_filter.SetActive(GfxTexFilter::BILINEAR);    return; }
    if (s == CONF_TEXFILTER_TRILI)    { App::gfx_texture_filter.SetActive(GfxTexFilter::TRILINEAR);   return; }
    if (s == CONF_TEXFILTER_ANISO)    { App::gfx_texture_filter.SetActive(GfxTexFilter::ANISOTROPIC); return; }
}

void App__SetVegetationMode(std::string const & s)
{
    if (s == CONF_VEGET_NONE  )       { App::gfx_vegetation_mode.SetActive(GfxVegetation::NONE);    return; }
    if (s == CONF_VEGET_20PERC)       { App::gfx_vegetation_mode.SetActive(GfxVegetation::x20PERC); return; }
    if (s == CONF_VEGET_50PERC)       { App::gfx_vegetation_mode.SetActive(GfxVegetation::x50PERC); return; }
    if (s == CONF_VEGET_FULL  )       { App::gfx_vegetation_mode.SetActive(GfxVegetation::FULL);    return; }
}

void App__SetSimGearboxMode(std::string const & s)
{
    if (s == CONF_GEARBOX_AUTO      ) { App::sim_gearbox_mode.SetActive(SimGearboxMode::AUTO         ); return; }
    if (s == CONF_GEARBOX_SEMIAUTO  ) { App::sim_gearbox_mode.SetActive(SimGearboxMode::SEMI_AUTO    ); return; }
    if (s == CONF_GEARBOX_MANUAL    ) { App::sim_gearbox_mode.SetActive(SimGearboxMode::MANUAL       ); return; }
    if (s == CONF_GEARBOX_MAN_STICK ) { App::sim_gearbox_mode.SetActive(SimGearboxMode::MANUAL_STICK ); return; }
    if (s == CONF_GEARBOX_MAN_RANGES) { App::sim_gearbox_mode.SetActive(SimGearboxMode::MANUAL_RANGES); return; }
}

void App__SetGfxFlaresMode(std::string const & s)
{
    if (s == CONF_FLARES_NONE      )  { App::gfx_flares_mode.SetActive(GfxFlaresMode::NONE);                    return; }
    if (s == CONF_FLARES_NO_LIGHT  )  { App::gfx_flares_mode.SetActive(GfxFlaresMode::NO_LIGHTSOURCES);         return; }
    if (s == CONF_FLARES_CURR_HEAD )  { App::gfx_flares_mode.SetActive(GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY);  return; }
    if (s == CONF_FLARES_ALL_HEADS )  { App::gfx_flares_mode.SetActive(GfxFlaresMode::ALL_VEHICLES_HEAD_ONLY);  return; }
    if (s == CONF_FLARES_ALL_LIGHTS)  { App::gfx_flares_mode.SetActive(GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS); return; }
}

void App__SetGfxWaterMode(std::string const & s)
{
    if (s == CONF_WATER_NONE     )    { App::gfx_water_mode.SetActive(GfxWaterMode::NONE     ); return; }
    if (s == CONF_WATER_BASIC    )    { App::gfx_water_mode.SetActive(GfxWaterMode::BASIC    ); return; }
    if (s == CONF_WATER_REFLECT  )    { App::gfx_water_mode.SetActive(GfxWaterMode::REFLECT  ); return; }
    if (s == CONF_WATER_FULL_FAST)    { App::gfx_water_mode.SetActive(GfxWaterMode::FULL_FAST); return; }
    if (s == CONF_WATER_FULL_HQ  )    { App::gfx_water_mode.SetActive(GfxWaterMode::FULL_HQ  ); return; }
    if (s == CONF_WATER_HYDRAX   )    { App::gfx_water_mode.SetActive(GfxWaterMode::HYDRAX   ); return; }
}

void App__SetGfxSkyMode(std::string const & s)
{
    if (s == CONF_SKY_SANDSTORM)      { App::gfx_sky_mode.SetActive(GfxSkyMode::SANDSTORM); return; }
    if (s == CONF_SKY_CAELUM   )      { App::gfx_sky_mode.SetActive(GfxSkyMode::CAELUM);    return; }
    if (s == CONF_SKY_SKYX     )      { App::gfx_sky_mode.SetActive(GfxSkyMode::SKYX);      return; }
}

void App__SetScreenshotFormat(std::string const & s)
{
    if (s.size() >= 3) { App::app_screenshot_format.SetActive(s.substr(0, 3).c_str()); }
}

void App__SetGfxEnvmapRate(std::string const & s)
{
    int rate = Ogre::StringConverter::parseInt(s);
    if (rate < 0) { rate = 0; }
    if (rate > 6) { rate = 6; }
    App::gfx_envmap_rate.SetActive(rate);
}

void Settings::SetMpNetworkEnable(bool enable)
{
    m_network_enable = enable;
    if (m_network_enable)
    {
        App::mp_state.SetPending(RoR::MpState::CONNECTED);
    }
}

void Settings::SetGfxFovExternal(float fov)
{
    m_fov_external = fov;
    App::gfx_fov_external.SetActive(m_fov_external);
}

void Settings::SetGfxFovInternal(float fov)
{
    m_fov_internal = fov;
    App::gfx_fov_internal.SetActive(m_fov_internal);
}

static const char* CONF_MP_NET_ENABLE   = "Network enable";
static const char* CONF_MP_NICKNAME     = "Nickname";
static const char* CONF_MP_HOSTNAME     = "Server name";
static const char* CONF_MP_PORT         = "Server port";
static const char* CONF_MP_PASSWORD     = "Server password";
static const char* CONF_MP_PORTAL_URL   = "Multiplayer portal URL";
// Sim
static const char* CONF_SIM_GEARBOX     = "GearboxMode";
static const char* CONF_SIM_MULTITHREAD = "Multi-threading";
// Input-Output
static const char* CONF_FF_ENABLED      = "Force Feedback";
static const char* CONF_FF_CAMERA       = "Force Feedback Camera";
static const char* CONF_FF_CENTERING    = "Force Feedback Centering";
static const char* CONF_FF_GAIN         = "Force Feedback Gain";
static const char* CONF_FF_STRESS       = "Force Feedback Stress";
static const char* CONF_INPUT_GRAB      = "Input Grab";
static const char* CONF_ARCADE_CONTROL  = "ArcadeControls";
static const char* CONF_OUTGAUGE_MODE   = "OutGauge Mode";
static const char* CONF_OUTGAUGE_IP     = "OutGauge IP";
static const char* CONF_OUTGAUGE_PORT   = "OutGauge Port";
static const char* CONF_OUTGAUGE_DELAY  = "OutGauge Delay";
static const char* CONF_OUTGAUGE_ID     = "OutGauge ID";
// Gfx
static const char* CONF_GFX_SHADOWS     = "Shadow technique";
static const char* CONF_GFX_EXTCAM      = "External CameraMode";
static const char* CONF_GFX_TEX_FILTER  = "Texture Filtering";
static const char* CONF_GFX_VEGETATION  = "Vegetation";
static const char* CONF_GFX_SUNBURN     = "Sunburn";
static const char* CONF_GFX_WAVES       = "Waves";
static const char* CONF_MINIMAP_OFF     = "disableOverViewMap";
static const char* CONF_GFX_PARTICLES   = "Particles";
static const char* CONF_GFX_GLOW        = "Glow";
static const char* CONF_GFX_HDR         = "HDR";
static const char* CONF_GFX_HEATHAZE    = "HeatHaze";
static const char* CONF_GFX_VIDEOCAMS   = "gfx_enable_videocams";
static const char* CONF_GFX_SKIDMARKS   = "Skidmarks";
static const char* CONF_ENVMAP_RATE     = "EnvmapUpdateRate";
static const char* CONF_ENVMAP_ENABLED  = "Envmap";
static const char* CONF_GFX_LIGHTS      = "Lights";
static const char* CONF_GFX_WATER_MODE  = "Water effects";
static const char* CONF_GFX_SIGHT_RANGE = "SightRange";
static const char* CONF_GFX_FOV_EXTERN  = "FOV External";
static const char* CONF_GFX_FOV_INTERN  = "FOV Internal";
static const char* CONF_GFX_FPS_LIMIT   = "FPS-Limiter";
static const char* CONF_GFX_SKY_EFFECTS = "Sky effects";
// Audio
static const char* CONF_SOUND_VOLUME    = "Sound Volume";
static const char* CONF_SOUND_CREAK     = "Creak Sound";
static const char* CONF_MUSIC_MAIN_MENU = "MainMenuMusic";
static const char* CONF_AUDIO_DEVICE    = "AudioDevice";
// Diag
static const char* CONF_LOG_NODE_IMPORT = "RigImporter_Debug_TraverseAndLogAllNodes";
static const char* CONF_LOG_NODE_STATS  = "RigImporter_PrintNodeStatsToLog";
static const char* CONF_LOG_RIG_IMPORT  = "RigImporter_PrintMessagesToLog";
static const char* CONF_COLLISION_DBG   = "Debug Collisions";
static const char* CONF_TRUCKMASS_DBG   = "Debug TruckMass";
static const char* CONF_ENVMAP_DEBUG    = "EnvMapDebug";
static const char* CONF_VIDEOCAM_DEBUG  = "VideoCameraDebug";
static const char* CONF_DIAG_MASS_CALC  = "Debug Truck Mass";
static const char* CONF_CONSOLE_ECHO    = "Enable Ingame Console";
static const char* CONF_DIAG_BREAK_LOG  = "Beam Break Debug";
static const char* CONF_DIAG_DEFORM_LOG = "Beam Deform Debug";
static const char* CONF_DIAG_TRIG_LOG   = "Trigger Debug";
static const char* CONF_DIAG_DOF_EFFECT = "DOFDebug";
static const char* CONF_PRESET_TERRAIN  = "Preselected Map";
static const char* CONF_PRESET_TRUCK    = "Preselected Truck";
static const char* CONF_PRESET_TRUCKCFG = "Preselected TruckConfig";
static const char* CONF_PRESET_ENTER_RIG= "Enter PreselectedTruck";
// App
static const char* CONF_SCREENSHOT_FMT  = "Screenshot Format";
static const char* CONF_REPLAY_MODE     = "Replay mode";
static const char* CONF_REPLAY_LENGTH   = "Replay length";
static const char* CONF_REPLAY_STEPPING = "Replay Steps per second";
static const char* CONF_POS_STORAGE     = "Position Storage";
static const char* CONF_LANGUAGE_SHORT  = "Language Short";

#define I(_VAL_)  Ogre::StringConverter::parseInt (_VAL_)
#define F(_VAL_)  Ogre::StringConverter::parseReal(_VAL_)
#define B(_VAL_)  Ogre::StringConverter::parseBool(_VAL_)
#define M(_VAL_)  ((int)B(_VAL_))
#define S(_VAL_)  (_VAL_.c_str())

bool Settings::ParseGlobalVarSetting(std::string const & k, std::string const & v)
{
    // Process and erase settings which propagate to global vars.

    // Multiplayer
    if (k == CONF_MP_NET_ENABLE   ) { this->SetMpNetworkEnable           (B(v)); return true; }
    if (k == CONF_MP_NICKNAME     ) { App::mp_player_name      .SetActive(S(v)); return true; }
    if (k == CONF_MP_HOSTNAME     ) { App::mp_server_host      .SetActive(S(v)); return true; }
    if (k == CONF_MP_PORT         ) { App::mp_server_port      .SetActive(I(v)); return true; }
    if (k == CONF_MP_PASSWORD     ) { App::mp_server_password  .SetActive(S(v)); return true; }
    if (k == CONF_MP_PORTAL_URL   ) { App::mp_portal_url       .SetActive(S(v)); return true; }
    // Sim
    if (k == CONF_SIM_GEARBOX     ) { App__SetSimGearboxMode             (S(v)); return true; }
    if (k == CONF_SIM_MULTITHREAD ) { App::app_multithread     .SetActive(B(v)); return true; }
    // Input&Output
    if (k == CONF_FF_ENABLED      ) { App::io_ffb_enabled      .SetActive(B(v)); return true; }
    if (k == CONF_FF_CAMERA       ) { App::io_ffb_camera_gain  .SetActive(F(v)); return true; }
    if (k == CONF_FF_CENTERING    ) { App::io_ffb_center_gain  .SetActive(F(v)); return true; }
    if (k == CONF_FF_GAIN         ) { App::io_ffb_master_gain  .SetActive(F(v)); return true; }
    if (k == CONF_FF_STRESS       ) { App::io_ffb_stress_gain  .SetActive(F(v)); return true; }
    if (k == CONF_INPUT_GRAB      ) { App__SetIoInputGrabMode            (S(v)); return true; }
    if (k == CONF_ARCADE_CONTROL  ) { App::io_arcade_controls  .SetActive(B(v)); return true; }
    if (k == CONF_OUTGAUGE_MODE   ) { App::io_outgauge_mode    .SetActive(I(v)); return true; }
    if (k == CONF_OUTGAUGE_IP     ) { App::io_outgauge_ip      .SetActive(S(v)); return true; }
    if (k == CONF_OUTGAUGE_PORT   ) { App::io_outgauge_port    .SetActive(I(v)); return true; }
    if (k == CONF_OUTGAUGE_DELAY  ) { App::io_outgauge_delay   .SetActive(F(v)); return true; }
    if (k == CONF_OUTGAUGE_ID     ) { App::io_outgauge_id      .SetActive(I(v)); return true; }
    // Gfx
    if (k == CONF_GFX_SHADOWS     ) { App__SetShadowTech                 (S(v)); return true; }
    if (k == CONF_GFX_EXTCAM      ) { App__SetExtcamMode                 (S(v)); return true; }
    if (k == CONF_GFX_TEX_FILTER  ) { App__SetTexFiltering               (S(v)); return true; }
    if (k == CONF_GFX_VEGETATION  ) { App__SetVegetationMode             (S(v)); return true; }
    if (k == CONF_GFX_SUNBURN     ) { App::gfx_enable_sunburn  .SetActive(B(v)); return true; }
    if (k == CONF_GFX_WAVES       ) { App::gfx_water_waves     .SetActive(B(v)); return true; }
    if (k == CONF_MINIMAP_OFF     ) { App::gfx_minimap_disabled.SetActive(B(v)); return true; }
    if (k == CONF_GFX_PARTICLES   ) { App::gfx_particles_mode  .SetActive(M(v)); return true; }
    if (k == CONF_GFX_GLOW        ) { App::gfx_enable_glow     .SetActive(B(v)); return true; }
    if (k == CONF_GFX_HDR         ) { App::gfx_enable_hdr      .SetActive(B(v)); return true; }
    if (k == CONF_GFX_HEATHAZE    ) { App::gfx_enable_heathaze .SetActive(B(v)); return true; }
    if (k == CONF_GFX_VIDEOCAMS   ) { App::gfx_enable_videocams.SetActive(B(v)); return true; }
    if (k == CONF_GFX_SKIDMARKS   ) { App::gfx_skidmarks_mode  .SetActive(M(v)); return true; }
    if (k == CONF_ENVMAP_RATE     ) { App__SetGfxEnvmapRate              (S(v)); return true; }
    if (k == CONF_ENVMAP_ENABLED  ) { App::gfx_envmap_enabled  .SetActive(B(v)); return true; }
    if (k == CONF_GFX_LIGHTS      ) { App__SetGfxFlaresMode              (S(v)); return true; }
    if (k == CONF_GFX_WATER_MODE  ) { App__SetGfxWaterMode               (S(v)); return true; }
    if (k == CONF_GFX_SIGHT_RANGE ) { App::gfx_sight_range     .SetActive(F(v)); return true; }
    if (k == CONF_GFX_FOV_EXTERN  ) { this->SetGfxFovExternal            (F(v)); return true; }
    if (k == CONF_GFX_FOV_INTERN  ) { this->SetGfxFovInternal            (F(v)); return true; }
    if (k == CONF_GFX_FPS_LIMIT   ) { App::gfx_fps_limit       .SetActive(I(v)); return true; }
    if (k == CONF_GFX_SKY_EFFECTS ) { App__SetGfxSkyMode                 (S(v)); return true; }
    // Audio
    if (k == CONF_SOUND_VOLUME    ) { App::audio_master_volume .SetActive(F(v)); return true; }
    if (k == CONF_SOUND_CREAK     ) { App::audio_enable_creak  .SetActive(B(v)); return true; }
    if (k == CONF_MUSIC_MAIN_MENU ) { App::audio_menu_music    .SetActive(B(v)); return true; }
    if (k == CONF_AUDIO_DEVICE    ) { App::audio_device_name   .SetActive(S(v)); return true; }
    // Diag
    if (k == CONF_LOG_NODE_IMPORT ) { App::diag_rig_log_node_import .SetActive(B(v)); return true; }
    if (k == CONF_LOG_NODE_STATS  ) { App::diag_rig_log_node_stats  .SetActive(B(v)); return true; }
    if (k == CONF_LOG_RIG_IMPORT  ) { App::diag_rig_log_messages    .SetActive(B(v)); return true; }
    if (k == CONF_COLLISION_DBG   ) { App::diag_collisions          .SetActive(B(v)); return true; }
    if (k == CONF_DIAG_MASS_CALC  ) { App::diag_truck_mass          .SetActive(B(v)); return true; }
    if (k == CONF_ENVMAP_DEBUG    ) { App::diag_envmap              .SetActive(B(v)); return true; }
    if (k == CONF_VIDEOCAM_DEBUG  ) { App::diag_videocameras        .SetActive(B(v)); return true; }
    if (k == CONF_CONSOLE_ECHO    ) { App::diag_log_console_echo    .SetActive(B(v)); return true; }
    if (k == CONF_DIAG_BREAK_LOG  ) { App::diag_log_beam_break      .SetActive(B(v)); return true; }
    if (k == CONF_DIAG_DEFORM_LOG ) { App::diag_log_beam_deform     .SetActive(B(v)); return true; }
    if (k == CONF_DIAG_TRIG_LOG   ) { App::diag_log_beam_trigger    .SetActive(B(v)); return true; }
    if (k == CONF_DIAG_DOF_EFFECT ) { App::diag_dof_effect          .SetActive(B(v)); return true; }
    if (k == CONF_PRESET_TERRAIN  ) { App::diag_preset_terrain      .SetActive(S(v)); return true; }
    if (k == CONF_PRESET_TRUCK    ) { App::diag_preset_vehicle      .SetActive(S(v)); return true; }
    if (k == CONF_PRESET_TRUCKCFG ) { App::diag_preset_veh_config   .SetActive(S(v)); return true; }
    if (k == CONF_PRESET_ENTER_RIG) { App::diag_preset_veh_enter    .SetActive(B(v)); return true; }
    // App
    if (k == CONF_SCREENSHOT_FMT  ) { App__SetScreenshotFormat     (S(v)); return true; }
    if (k == CONF_REPLAY_MODE     ) { App::sim_replay_enabled       .SetActive(B(v)); return true; }
    if (k == CONF_REPLAY_LENGTH   ) { App::sim_replay_length        .SetActive(I(v)); return true; }
    if (k == CONF_REPLAY_STEPPING ) { App::sim_replay_stepping      .SetActive(I(v)); return true; }
    if (k == CONF_POS_STORAGE     ) { App::sim_position_storage     .SetActive(B(v)); return true; }
    if (k == CONF_LANGUAGE_SHORT  ) { App::app_locale               .SetActive(S(v)); return true; }

    return false;
}

#undef I
#undef F
#undef B
#undef M
#undef S

void Settings::LoadRoRCfg()
{
    Ogre::ConfigFile cfg;
    try
    {
        GStr<300> path;
        path << App::sys_config_dir.GetActive() << PATH_SLASH << "RoR.cfg";
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

    // generate hash of the token
    String usertoken = SSETTING("User Token", "");
    char usertokensha1result[250];
    memset(usertokensha1result, 0, 250);
    if (usertoken.size()>0)
    {
        RoR::CSHA1 sha1;
        sha1.UpdateHash((uint8_t *)usertoken.c_str(), (uint32_t)usertoken.size());
        sha1.Final();
        sha1.ReportHash(usertokensha1result, RoR::CSHA1::REPORT_HEX_SHORT);
    }

    setSetting("User Token Hash", String(usertokensha1result));
}

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

inline const char* App__IoInputGrabToStr   () { return IoInputGrabToStr   (App::io_input_grab_mode.GetActive  ()); }
inline const char* App__GfxShadowTechToStr () { return GfxShadowTechToStr (App::gfx_shadow_type.GetActive     ()); }
inline const char* App__GfxExtcamModeToStr () { return GfxExtcamModeToStr (App::gfx_extcam_mode.GetActive     ()); }
inline const char* App__GfxTexFilterToStr  () { return GfxTexFilterToStr  (App::gfx_texture_filter.GetActive  ()); }
inline const char* App__GfxVegetationToStr () { return GfxVegetationToStr (App::gfx_vegetation_mode.GetActive ()); }
inline const char* App__SimGearboxToStr    () { return SimGearboxToStr    (App::sim_gearbox_mode.GetActive    ()); }
inline const char* App__GfxFlaresToStr     () { return GfxFlaresToStr     (App::gfx_flares_mode.GetActive     ()); }
inline const char* App__GfxWaterToStr      () { return GfxWaterToStr      (App::gfx_water_mode.GetActive      ()); }
inline const char* App__GfxSkyToStr        () { return GfxSkyToStr        (App::gfx_sky_mode.GetActive        ()); }
inline int         App__GetEnvmapRate      () { return std::min(6, std::max(0, App::gfx_envmap_rate.GetActive())); }

#define _(_V_) (_V_)
#define B(_V_) (_V_ ? "Yes" : "No")
#define Y(_V_) ((_V_ != 0) ? "Yes" : "No")

void Settings::SaveSettings()
{
    using namespace std;

    GStr<300> rorcfg_path;
    rorcfg_path << App::sys_config_dir.GetActive() << PATH_SLASH << "RoR.cfg";
    std::ofstream f(rorcfg_path);
    if (!f.is_open())
    {
        LOG("[RoR] Failed to save RoR.cfg: Could not open file");
        return;
    }

    f << "; Rigs of Rods configuration file"                                     << endl;
    f << "; -------------------------------"                                     << endl;
    f                                                                            << endl;
    f << "; Multiplayer"                                                         << endl;
    f << CONF_MP_NET_ENABLE   << "=" << B(m_network_enable                     ) << endl;
    f << CONF_MP_NICKNAME     << "=" << _(App::mp_player_name.GetActive      ()) << endl;
    f << CONF_MP_HOSTNAME     << "=" << _(App::mp_server_host.GetActive      ()) << endl;
    f << CONF_MP_PORT         << "=" << _(App::mp_server_port.GetActive      ()) << endl;
    f << CONF_MP_PASSWORD     << "=" << _(App::mp_server_password.GetActive  ()) << endl;
    f << CONF_MP_PORTAL_URL   << "=" << _(App::mp_portal_url.GetActive       ()) << endl;
    f                                                                            << endl;
    f << "; Simulation"                                                          << endl;
    f << CONF_SIM_GEARBOX     << "=" << _(App__SimGearboxToStr               ()) << endl;
    f << CONF_SIM_MULTITHREAD << "=" << B(App::app_multithread.GetActive     ()) << endl;
    f                                                                            << endl;
    f << "; Input/Output"                                                        << endl;
    f << CONF_FF_ENABLED      << "=" << B(App::io_ffb_enabled.GetActive      ()) << endl;
    f << CONF_FF_CAMERA       << "=" << _(App::io_ffb_camera_gain.GetActive  ()) << endl;
    f << CONF_FF_CENTERING    << "=" << _(App::io_ffb_center_gain.GetActive  ()) << endl;
    f << CONF_FF_GAIN         << "=" << _(App::io_ffb_master_gain.GetActive  ()) << endl;
    f << CONF_FF_STRESS       << "=" << _(App::io_ffb_stress_gain.GetActive  ()) << endl;
    f << CONF_INPUT_GRAB      << "=" << _(App__IoInputGrabToStr              ()) << endl;
    f << CONF_ARCADE_CONTROL  << "=" << B(App::io_arcade_controls.GetActive  ()) << endl;
    f << CONF_OUTGAUGE_MODE   << "=" << _(App::io_outgauge_mode.GetActive    ()) << endl;
    f << CONF_OUTGAUGE_IP     << "=" << _(App::io_outgauge_ip.GetActive      ()) << endl;
    f << CONF_OUTGAUGE_PORT   << "=" << _(App::io_outgauge_port.GetActive    ()) << endl;
    f << CONF_OUTGAUGE_DELAY  << "=" << _(App::io_outgauge_delay.GetActive   ()) << endl;
    f << CONF_OUTGAUGE_ID     << "=" << _(App::io_outgauge_id.GetActive      ()) << endl;
    f                                                                            << endl;
    f << "; Graphics"                                                            << endl;
    f << CONF_GFX_SHADOWS     << "=" << _(App__GfxShadowTechToStr            ()) << endl;
    f << CONF_GFX_EXTCAM      << "=" << _(App__GfxExtcamModeToStr            ()) << endl;
    f << CONF_GFX_TEX_FILTER  << "=" << _(App__GfxTexFilterToStr             ()) << endl;
    f << CONF_GFX_VEGETATION  << "=" << _(App__GfxVegetationToStr            ()) << endl;
    f << CONF_GFX_SUNBURN     << "=" << B(App::gfx_enable_sunburn  .GetActive()) << endl;
    f << CONF_GFX_WAVES       << "=" << B(App::gfx_water_waves     .GetActive()) << endl;
    f << CONF_MINIMAP_OFF     << "=" << B(App::gfx_minimap_disabled.GetActive()) << endl;
    f << CONF_GFX_PARTICLES   << "=" << Y(App::gfx_particles_mode  .GetActive()) << endl;
    f << CONF_GFX_GLOW        << "=" << B(App::gfx_enable_glow     .GetActive()) << endl;
    f << CONF_GFX_HDR         << "=" << B(App::gfx_enable_hdr      .GetActive()) << endl;
    f << CONF_GFX_HEATHAZE    << "=" << B(App::gfx_enable_heathaze .GetActive()) << endl;
    f << CONF_GFX_VIDEOCAMS   << "=" << B(App::diag_videocameras   .GetActive()) << endl;
    f << CONF_GFX_SKIDMARKS   << "=" << Y(App::gfx_skidmarks_mode  .GetActive()) << endl;
    f << CONF_ENVMAP_ENABLED  << "=" << B(App::gfx_envmap_enabled  .GetActive()) << endl;
    f << CONF_ENVMAP_RATE     << "=" << _(App::gfx_envmap_rate     .GetActive()) << endl;
    f << CONF_GFX_LIGHTS      << "=" << _(App__GfxFlaresToStr                ()) << endl;
    f << CONF_GFX_WATER_MODE  << "=" << _(App__GfxWaterToStr                 ()) << endl;
    f << CONF_GFX_SIGHT_RANGE << "=" << _(App::gfx_sight_range     .GetActive()) << endl;
    f << CONF_GFX_FOV_EXTERN  << "=" << _(m_fov_external)                        << endl;
    f << CONF_GFX_FOV_INTERN  << "=" << _(m_fov_internal)                        << endl;
    f << CONF_GFX_FPS_LIMIT   << "=" << _(App::gfx_fps_limit.GetActive       ()) << endl;
    f << CONF_GFX_SKY_EFFECTS << "=" << _(App__GfxSkyToStr                   ()) << endl;
    f                                                                            << endl;
    f << "; Audio"                                                               << endl;
    f << CONF_SOUND_VOLUME    << "=" << _(App::audio_master_volume      .GetActive()) << endl;
    f << CONF_SOUND_CREAK     << "=" << B(App::audio_enable_creak       .GetActive()) << endl;
    f << CONF_MUSIC_MAIN_MENU << "=" << B(App::audio_menu_music         .GetActive()) << endl;
    f << CONF_AUDIO_DEVICE    << "=" << _(App::audio_device_name        .GetActive()) << endl;
    f                                                                                 << endl;
    f << "; Diagnostics"                                                              << endl;
    f << CONF_LOG_NODE_IMPORT << "=" << B(App::diag_rig_log_node_import .GetActive()) << endl;
    f << CONF_LOG_NODE_STATS  << "=" << B(App::diag_rig_log_node_stats  .GetActive()) << endl;
    f << CONF_LOG_RIG_IMPORT  << "=" << B(App::diag_rig_log_messages    .GetActive()) << endl;
    f << CONF_COLLISION_DBG   << "=" << B(App::diag_collisions          .GetActive()) << endl;
    f << CONF_DIAG_MASS_CALC  << "=" << B(App::diag_truck_mass          .GetActive()) << endl;
    f << CONF_ENVMAP_DEBUG    << "=" << B(App::diag_envmap              .GetActive()) << endl;
    f << CONF_CONSOLE_ECHO    << "=" << B(App::diag_log_console_echo    .GetActive()) << endl;
    f << CONF_DIAG_BREAK_LOG  << "=" << B(App::diag_log_beam_break      .GetActive()) << endl;
    f << CONF_DIAG_DEFORM_LOG << "=" << B(App::diag_log_beam_deform     .GetActive()) << endl;
    f << CONF_DIAG_TRIG_LOG   << "=" << B(App::diag_log_beam_trigger    .GetActive()) << endl;
    f << CONF_DIAG_DOF_EFFECT << "=" << B(App::diag_dof_effect          .GetActive()) << endl;
    f << CONF_PRESET_TERRAIN  << "=" << _(App::diag_preset_terrain      .GetActive()) << endl;
    f << CONF_PRESET_TRUCK    << "=" << _(App::diag_preset_vehicle      .GetActive()) << endl;
    f << CONF_PRESET_TRUCKCFG << "=" << _(App::diag_preset_veh_config   .GetActive()) << endl;
    f << CONF_PRESET_ENTER_RIG<< "=" << B(App::diag_preset_veh_enter    .GetActive()) << endl;
    f                                                                                 << endl;
    f << "; Application"                                                              << endl;
    f << CONF_SCREENSHOT_FMT  << "=" << _(App::app_screenshot_format    .GetActive()) << endl;
    f << CONF_REPLAY_MODE     << "=" << B(App::sim_replay_enabled       .GetActive()) << endl;
    f << CONF_REPLAY_LENGTH   << "=" << _(App::sim_replay_length        .GetActive()) << endl;
    f << CONF_REPLAY_STEPPING << "=" << _(App::sim_replay_stepping      .GetActive()) << endl;
    f << CONF_POS_STORAGE     << "=" << B(App::sim_position_storage     .GetActive()) << endl;
    f << CONF_LANGUAGE_SHORT  << "=" << _(App::app_locale               .GetActive()) << endl;

    // Append misc legacy entries
    f << endl << "; Misc" << endl;
    for (auto itor = settings.begin(); itor != settings.end(); ++itor)
    {
        f << itor->first << "=" << itor->second << endl;
    }

    f.close();
}

#undef _
#undef B
#undef Y

bool Settings::SetupAllPaths()
{
    using namespace RoR;
    GStr<300> buf;

    // User directories
    buf.Clear() << App::sys_user_dir.GetActive() << PATH_SLASH << "config";             App::sys_config_dir    .SetActive(buf);
    buf.Clear() << App::sys_user_dir.GetActive() << PATH_SLASH << "cache";              App::sys_cache_dir     .SetActive(buf);
    buf.Clear() << App::sys_user_dir.GetActive() << PATH_SLASH << "screenshots";        App::sys_screenshot_dir.SetActive(buf);
    buf.Clear() << App::sys_user_dir.GetActive() << PATH_SLASH << "projects/rigs";      App::sys_rig_projects_dir.SetActive(buf);

    // Resources dir
    buf.Clear() << App::sys_process_dir.GetActive() << PATH_SLASH << "resources";
    if (FolderExists(buf))
    {
        App::sys_resources_dir.SetActive(buf);
        return true;
    }

    System::GetParentDirectory(buf.GetBuffer(), App::sys_process_dir.GetActive());
    if (FolderExists(buf))
    {
        App::sys_resources_dir.SetActive(buf);
        return true;
    }

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    buf = "/usr/share/rigsofrods/resources/";
    if (FolderExists(buf))
    {
        App::sys_resources_dir.SetActive(buf);
        return true;
    }
#endif
    return false;
}

