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

#define _L

#include "Application.h"
#include "ErrorUtils.h"
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
    OPT_NOCRASHCRPT,
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

using namespace RoR;
using namespace Ogre;

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

std::string GetParentDirectory(const char* src_buff)
{   
    const char* start = src_buff;
    int count = strlen(src_buff);
    // Trim trailing separator(s)
    for (;;)
    {
        if (count == 0) { return ""; }
        if (!IsSeparator(start[count - 1])) { break; }
        --count;
    }
    // Remove last path entry
    for (;;)
    {
        if (count == 0) { return ""; }
        if (IsSeparator(start[count - 1])) {break; }
        --count;
    }
    // Trim rear separator(s)
    for (;;)
    {
        if (count == 0) { return ""; }
        if (!IsSeparator(start[count - 1])) { break; }
        --count;
    }
    return std::string(start, count);
}

int DetectBasePaths()
{
    char buf[1000] = "";

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
    if (_NSGetExecutablePath(procpath, &lenb) == -1) // Returns absolute path to binary
    {
        return -1;        
    } 
#endif
    App::SetSysProcessDir(RoR::System::GetParentDirectory(buf));      

    // User directory (local override)
    std::string local_userdir = App::GetSysProcessDir() + PATH_SLASH + "config";
    if (FolderExists(local_userdir.c_str()))
    {
        App::SetSysUserDir(local_userdir);
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

    App::SetSysUserDir(buf);    
    return 0;
}

} // namespace System
} // namespace RoR

//RoR::App::GetActiveMpState() == RoR::App::MP_STATE_CONNECTED

void Settings::ProcessCommandLine(int argc, char *argv[])
{
    CSimpleOpt args(argc, argv, cmdline_options);

    while (args.Next())
    {
        if (args.LastError() != SO_SUCCESS)
        {
            RoR::App::SetPendingAppState(RoR::App::APP_STATE_PRINT_HELP_EXIT);
            return;
        }
        else if (args.OptionId() == OPT_HELP)
        {
            RoR::App::SetPendingAppState(RoR::App::APP_STATE_PRINT_HELP_EXIT);
            return;
        }
        else if (args.OptionId() == OPT_VER)
        {
            RoR::App::SetPendingAppState(RoR::App::APP_STATE_PRINT_VERSION_EXIT);
            return;
        }
        else if (args.OptionId() == OPT_TRUCK) 
        {
            App::SetDiagPreselectedVehicle(args.OptionArg());
        } 
        else if (args.OptionId() == OPT_TRUCKCONFIG) 
        {
            App::SetDiagPreselectedVehConfig(args.OptionArg());
        } 
        else if (args.OptionId() == OPT_MAP) 
        {
            RoR::App::SetDiagPreselectedTerrain(args.OptionArg());
        } 
        else if (args.OptionId() == OPT_NOCRASHCRPT) 
        {
            SETTINGS.setSetting("NoCrashRpt", "Yes");
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
            App::SetDiagPreselectedVehEnter(true);
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
                RoR::App::SetPendingMpState(RoR::App::MP_STATE_CONNECTED);

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
                RoR::App::SetMpServerHost(host_str);
                RoR::App::SetMpServerPort(Ogre::StringConverter::parseInt(port_str));
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
    if (s == CONF_INPUT_GRAB_DYNAMIC) { App::SetIoInputGrabMode(App::INPUT_GRAB_DYNAMIC); return; }
    if (s == CONF_INPUT_GRAB_NONE   ) { App::SetIoInputGrabMode(App::INPUT_GRAB_NONE);    return; }
    else                              { App::SetIoInputGrabMode(App::INPUT_GRAB_ALL);     return; }
}

void App__SetShadowTech(std::string const & s)
{
    if (s == CONF_GFX_SHADOW_TEX)     { App::SetGfxShadowType(App::GFX_SHADOW_TYPE_TEXTURE); return; }
    if (s == CONF_GFX_SHADOW_PSSM)    { App::SetGfxShadowType(App::GFX_SHADOW_TYPE_PSSM   ); return; }
    else                              { App::SetGfxShadowType(App::GFX_SHADOW_TYPE_NONE   ); return; }
}

void App__SetExtcamMode(std::string const & s)
{
    if (s == CONF_EXTCAM_PITCHING)    { App::SetGfxExternCamMode(App::GFX_EXTCAM_MODE_PITCHING); return; }
    if (s == CONF_EXTCAM_STATIC)      { App::SetGfxExternCamMode(App::GFX_EXTCAM_MODE_STATIC  ); return; }
    else                              { App::SetGfxExternCamMode(App::GFX_EXTCAM_MODE_NONE    ); return; }
}

void App__SetTexFiltering(std::string const & s)
{
    if (s == CONF_TEXFILTER_NONE)     { App::SetGfxTexFiltering(App::GFX_TEXFILTER_NONE);        return; }
    if (s == CONF_TEXFILTER_BILI)     { App::SetGfxTexFiltering(App::GFX_TEXFILTER_BILINEAR);    return; }
    if (s == CONF_TEXFILTER_TRILI)    { App::SetGfxTexFiltering(App::GFX_TEXFILTER_TRILINEAR);   return; }
    if (s == CONF_TEXFILTER_ANISO)    { App::SetGfxTexFiltering(App::GFX_TEXFILTER_ANISOTROPIC); return; }
}

void App__SetVegetationMode(std::string const & s)
{
    if (s == CONF_VEGET_NONE  )       { App::SetGfxVegetationMode(App::GFX_VEGETATION_NONE);   return; }
    if (s == CONF_VEGET_20PERC)       { App::SetGfxVegetationMode(App::GFX_VEGETATION_20PERC); return; }
    if (s == CONF_VEGET_50PERC)       { App::SetGfxVegetationMode(App::GFX_VEGETATION_50PERC); return; }
    if (s == CONF_VEGET_FULL  )       { App::SetGfxVegetationMode(App::GFX_VEGETATION_FULL);   return; }
}

void App__SetSimGearboxMode(std::string const & s)
{
    if (s == CONF_GEARBOX_AUTO      ) { App::SetSimGearboxMode(App::SIM_GEARBOX_AUTO         ); return; }
    if (s == CONF_GEARBOX_SEMIAUTO  ) { App::SetSimGearboxMode(App::SIM_GEARBOX_SEMI_AUTO    ); return; }
    if (s == CONF_GEARBOX_MANUAL    ) { App::SetSimGearboxMode(App::SIM_GEARBOX_MANUAL       ); return; }
    if (s == CONF_GEARBOX_MAN_STICK ) { App::SetSimGearboxMode(App::SIM_GEARBOX_MANUAL_STICK ); return; }
    if (s == CONF_GEARBOX_MAN_RANGES) { App::SetSimGearboxMode(App::SIM_GEARBOX_MANUAL_RANGES); return; }
}

void App__SetGfxFlaresMode(std::string const & s)
{
    if (s == CONF_FLARES_NONE      )  { App::SetGfxFlaresMode(App::GFX_FLARES_NONE);                    return; }
    if (s == CONF_FLARES_NO_LIGHT  )  { App::SetGfxFlaresMode(App::GFX_FLARES_NO_LIGHTSOURCES);         return; }
    if (s == CONF_FLARES_CURR_HEAD )  { App::SetGfxFlaresMode(App::GFX_FLARES_CURR_VEHICLE_HEAD_ONLY);  return; }
    if (s == CONF_FLARES_ALL_HEADS )  { App::SetGfxFlaresMode(App::GFX_FLARES_ALL_VEHICLES_HEAD_ONLY);  return; }
    if (s == CONF_FLARES_ALL_LIGHTS)  { App::SetGfxFlaresMode(App::GFX_FLARES_ALL_VEHICLES_ALL_LIGHTS); return; }
}

void App__SetGfxWaterMode(std::string const & s)
{
    if (s == CONF_WATER_NONE     )    { App::SetGfxWaterMode(App::GFX_WATER_NONE     ); return; }
    if (s == CONF_WATER_BASIC    )    { App::SetGfxWaterMode(App::GFX_WATER_BASIC    ); return; }
    if (s == CONF_WATER_REFLECT  )    { App::SetGfxWaterMode(App::GFX_WATER_REFLECT  ); return; }
    if (s == CONF_WATER_FULL_FAST)    { App::SetGfxWaterMode(App::GFX_WATER_FULL_FAST); return; }
    if (s == CONF_WATER_FULL_HQ  )    { App::SetGfxWaterMode(App::GFX_WATER_FULL_HQ  ); return; }
    if (s == CONF_WATER_HYDRAX   )    { App::SetGfxWaterMode(App::GFX_WATER_HYDRAX   ); return; }
}

void App__SetGfxSkyMode(std::string const & s)
{
    if (s == CONF_SKY_SANDSTORM)      { App::SetGfxSkyMode(App::GFX_SKY_SANDSTORM); return; }
    if (s == CONF_SKY_CAELUM   )      { App::SetGfxSkyMode(App::GFX_SKY_CAELUM);    return; }
    if (s == CONF_SKY_SKYX     )      { App::SetGfxSkyMode(App::GFX_SKY_SKYX);      return; }
}

void App__SetScreenshotFormat(std::string const & s)
{
    if (s.size() >= 3) { App::SetAppScreenshotFormat(s.substr(0, 3)); }
}

void App__SetGfxEnvmapRate(std::string const & s)
{
    int rate = Ogre::StringConverter::parseInt(s);
    if (rate < 0) { rate = 0; }
    if (rate > 6) { rate = 6; }
    App::SetGfxEnvmapRate(rate);
}

void Settings::SetMpNetworkEnable(bool enable)
{
    m_network_enable = enable;
    if (m_network_enable)
    {
        App::SetPendingMpState(RoR::App::MP_STATE_CONNECTED);
    }
}

void Settings::SetGfxFovExternal(float fov)
{
    m_fov_external = fov;
    App::SetGfxFovExternal(m_fov_external);
}

void Settings::SetGfxFovInternal(float fov)
{
    m_fov_internal = fov;
    App::SetGfxFovInternal(m_fov_internal);
}

static const char* CONF_MP_NET_ENABLE   = "Network enable";
static const char* CONF_MP_NICKNAME     = "Nickname";
static const char* CONF_MP_HOSTNAME     = "Server name";
static const char* CONF_MP_PORT         = "Server port";
static const char* CONF_MP_PASSWORD     = "Server password";
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
static const char* CONF_PRESELECTED_TERRAIN     = "Preselected Map";
static const char* CONF_PRESELECTED_TRUCK       = "Preselected Truck";
static const char* CONF_PRESELECTED_TRUCK_CFG   = "Preselected TruckConfig";
static const char* CONF_PRESELECTED_TRUCK_ENTER = "Enter PreselectedTruck";
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
#define S(_VAL_)  (_VAL_)

bool Settings::ParseGlobalVarSetting(std::string const & k, std::string const & v)
{
    // Process and erase settings which propagate to global vars.

    // Multiplayer
    if (k == CONF_MP_NET_ENABLE   ) { this->SetMpNetworkEnable     (B(v)); return true; }
    if (k == CONF_MP_NICKNAME     ) { App::SetMpPlayerName         (S(v)); return true; }
    if (k == CONF_MP_HOSTNAME     ) { App::SetMpServerHost         (S(v)); return true; }
    if (k == CONF_MP_PORT         ) { App::SetMpServerPort         (I(v)); return true; }
    if (k == CONF_MP_PASSWORD     ) { App::SetMpServerPassword     (S(v)); return true; }
    // Sim
    if (k == CONF_SIM_GEARBOX     ) { App__SetSimGearboxMode       (S(v)); return true; }
    if (k == CONF_SIM_MULTITHREAD ) { App::SetAppMultithread       (B(v)); return true; }
    // Input&Output
    if (k == CONF_FF_ENABLED      ) { App::SetIoFFbackEnabled      (B(v)); return true; }
    if (k == CONF_FF_CAMERA       ) { App::SetIoFFbackCameraGain   (F(v)); return true; }
    if (k == CONF_FF_CENTERING    ) { App::SetIoFFbackCenterGain   (F(v)); return true; }
    if (k == CONF_FF_GAIN         ) { App::SetIoFFbackMasterGain   (F(v)); return true; }
    if (k == CONF_FF_STRESS       ) { App::SetIoFFbackStressGain   (F(v)); return true; }
    if (k == CONF_INPUT_GRAB      ) { App__SetIoInputGrabMode      (S(v)); return true; }
    if (k == CONF_ARCADE_CONTROL  ) { App::SetIoArcadeControls     (B(v)); return true; }
    if (k == CONF_OUTGAUGE_MODE   ) { App::SetIoOutGaugeMode       (I(v)); return true; }
    if (k == CONF_OUTGAUGE_IP     ) { App::SetIoOutGaugeIp         (S(v)); return true; }
    if (k == CONF_OUTGAUGE_PORT   ) { App::SetIoOutGaugePort       (I(v)); return true; }
    if (k == CONF_OUTGAUGE_DELAY  ) { App::SetIoOutGaugeDelay      (F(v)); return true; }
    if (k == CONF_OUTGAUGE_ID     ) { App::SetIoOutGaugeId         (I(v)); return true; }
    // Gfx
    if (k == CONF_GFX_SHADOWS     ) { App__SetShadowTech           (S(v)); return true; }
    if (k == CONF_GFX_EXTCAM      ) { App__SetExtcamMode           (S(v)); return true; }
    if (k == CONF_GFX_TEX_FILTER  ) { App__SetTexFiltering         (S(v)); return true; }
    if (k == CONF_GFX_VEGETATION  ) { App__SetVegetationMode       (S(v)); return true; }
    if (k == CONF_GFX_SUNBURN     ) { App::SetGfxEnableSunburn     (B(v)); return true; }
    if (k == CONF_GFX_WAVES       ) { App::SetGfxWaterUseWaves     (B(v)); return true; }
    if (k == CONF_MINIMAP_OFF     ) { App::SetGfxMinimapDisabled   (B(v)); return true; }
    if (k == CONF_GFX_PARTICLES   ) { App::SetGfxParticlesMode     (M(v)); return true; }
    if (k == CONF_GFX_GLOW        ) { App::SetGfxEnableGlow        (B(v)); return true; }
    if (k == CONF_GFX_HDR         ) { App::SetGfxEnableHdr         (B(v)); return true; }
    if (k == CONF_GFX_HEATHAZE    ) { App::SetGfxUseHeathaze       (B(v)); return true; }
    if (k == CONF_GFX_SKIDMARKS   ) { App::SetGfxSkidmarksMode     (M(v)); return true; }
    if (k == CONF_ENVMAP_RATE     ) { App__SetGfxEnvmapRate        (S(v)); return true; }
    if (k == CONF_ENVMAP_ENABLED  ) { App::SetGfxEnvmapEnabled     (B(v)); return true; }
    if (k == CONF_GFX_LIGHTS      ) { App__SetGfxFlaresMode        (S(v)); return true; }
    if (k == CONF_GFX_WATER_MODE  ) { App__SetGfxWaterMode         (S(v)); return true; }
    if (k == CONF_GFX_SIGHT_RANGE ) { App::SetGfxSightRange        (F(v)); return true; }
    if (k == CONF_GFX_FOV_EXTERN  ) { this->SetGfxFovExternal      (F(v)); return true; }
    if (k == CONF_GFX_FOV_INTERN  ) { this->SetGfxFovInternal      (F(v)); return true; }
    if (k == CONF_GFX_FPS_LIMIT   ) { App::SetGfxFpsLimit          (I(v)); return true; }
    if (k == CONF_GFX_SKY_EFFECTS ) { App__SetGfxSkyMode           (S(v)); return true; }
    // Audio
    if (k == CONF_SOUND_VOLUME    ) { App::SetAudioMasterVolume    (F(v)); return true; }
    if (k == CONF_SOUND_CREAK     ) { App::SetAudioEnableCreak     (B(v)); return true; }
    if (k == CONF_MUSIC_MAIN_MENU ) { App::SetAudioMenuMusic       (B(v)); return true; }
    if (k == CONF_AUDIO_DEVICE    ) { App::SetAudioDeviceName      (S(v)); return true; }
    // Diag
    if (k == CONF_LOG_NODE_IMPORT ) { App::SetDiagRigLogNodeImport (B(v)); return true; }
    if (k == CONF_LOG_NODE_STATS  ) { App::SetDiagRigLogNodeStats  (B(v)); return true; }
    if (k == CONF_LOG_RIG_IMPORT  ) { App::SetDiagRigLogMessages   (B(v)); return true; }
    if (k == CONF_COLLISION_DBG   ) { App::SetDiagCollisions       (B(v)); return true; }
    if (k == CONF_TRUCKMASS_DBG   ) { App::SetDiagTruckMass        (B(v)); return true; }
    if (k == CONF_ENVMAP_DEBUG    ) { App::SetDiagEnvmap           (B(v)); return true; }
    if (k == CONF_PRESELECTED_TERRAIN     ) { App::SetDiagPreselectedTerrain   (S(v)); return true; }
    if (k == CONF_PRESELECTED_TRUCK       ) { App::SetDiagPreselectedVehicle   (S(v)); return true; }
    if (k == CONF_PRESELECTED_TRUCK_CFG   ) { App::SetDiagPreselectedVehConfig (S(v)); return true; }
    if (k == CONF_PRESELECTED_TRUCK_ENTER ) { App::SetDiagPreselectedVehEnter  (B(v)); return true; }
    // App
    if (k == CONF_SCREENSHOT_FMT  ) { App__SetScreenshotFormat     (S(v)); return true; }
    if (k == CONF_REPLAY_MODE     ) { App::SetSimReplayEnabled     (B(v)); return true; }
    if (k == CONF_REPLAY_LENGTH   ) { App::SetSimReplayLength      (I(v)); return true; }
    if (k == CONF_REPLAY_STEPPING ) { App::SetSimReplayStepping    (I(v)); return true; }
    if (k == CONF_POS_STORAGE     ) { App::SetSimPositionStorage   (B(v)); return true; }
    if (k == CONF_LANGUAGE_SHORT  ) { App::SetAppLocale            (S(v)); return true; }

    return false;
}

#undef I
#undef F
#undef B
#undef M
#undef S

void Settings::LoadSettings(std::string filepath)
{
    ConfigFile cfg;
    try
    {
        cfg.load(filepath, "=:\t", false);

        // load all settings into a map!
        ConfigFile::SettingsIterator i = cfg.getSettingsIterator();
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
    catch (Ogre::FileNotFoundException e) {} // Just continue with defaults...

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

inline const char* IoInputGrabToStr(App::IoInputGrabMode v)
{
    switch (v)
    {
    case App::INPUT_GRAB_DYNAMIC: return CONF_INPUT_GRAB_DYNAMIC;
    case App::INPUT_GRAB_NONE   : return CONF_INPUT_GRAB_NONE   ;
    case App::INPUT_GRAB_ALL    : return CONF_INPUT_GRAB_ALL    ;
    default                     : return ""                     ;
    }
}

inline const char* GfxShadowTechToStr(App::GfxShadowType v)
{
    switch (v)
    {
    case App::GFX_SHADOW_TYPE_TEXTURE: return CONF_GFX_SHADOW_TEX  ;
    case App::GFX_SHADOW_TYPE_PSSM   : return CONF_GFX_SHADOW_PSSM ;
    case App::GFX_SHADOW_TYPE_NONE   : return CONF_GFX_SHADOW_NONE ;
    default                          : return ""                   ;
    }
}

inline const char* GfxExtcamModeToStr(App::GfxExtCamMode v)
{
    switch (v)
    {
    case App::GFX_EXTCAM_MODE_PITCHING: return CONF_EXTCAM_PITCHING;
    case App::GFX_EXTCAM_MODE_STATIC  : return CONF_EXTCAM_STATIC  ;
    case App::GFX_EXTCAM_MODE_NONE    : return CONF_EXTCAM_NONE    ;
    default                           : return ""                  ;
    }
}

inline const char* GfxTexFilterToStr(App::GfxTexFilter v)
{
    switch (v)
    {
    case App::GFX_TEXFILTER_NONE       : return CONF_TEXFILTER_NONE ;
    case App::GFX_TEXFILTER_BILINEAR   : return CONF_TEXFILTER_BILI ;
    case App::GFX_TEXFILTER_TRILINEAR  : return CONF_TEXFILTER_TRILI;
    case App::GFX_TEXFILTER_ANISOTROPIC: return CONF_TEXFILTER_ANISO;
    default                            : return ""                  ;
    }
}

inline const char* GfxVegetationToStr(App::GfxVegetation v)
{
    switch (v)
    {
    case App::GFX_VEGETATION_NONE  : return CONF_VEGET_NONE  ;
    case App::GFX_VEGETATION_20PERC: return CONF_VEGET_20PERC;
    case App::GFX_VEGETATION_50PERC: return CONF_VEGET_50PERC;
    case App::GFX_VEGETATION_FULL  : return CONF_VEGET_FULL  ;
    default                        : return ""               ;
    }
}

inline const char* SimGearboxToStr(App::SimGearboxMode v)
{
    switch (v)
    {
    case App::SIM_GEARBOX_AUTO         : return CONF_GEARBOX_AUTO      ;
    case App::SIM_GEARBOX_SEMI_AUTO    : return CONF_GEARBOX_SEMIAUTO  ;
    case App::SIM_GEARBOX_MANUAL       : return CONF_GEARBOX_MANUAL    ;
    case App::SIM_GEARBOX_MANUAL_STICK : return CONF_GEARBOX_MAN_STICK ;
    case App::SIM_GEARBOX_MANUAL_RANGES: return CONF_GEARBOX_MAN_RANGES;
    default                            : return ""                     ;
    }
}

inline const char* GfxFlaresToStr(App::GfxFlaresMode v)
{
    switch(v)
    {
    case App::GFX_FLARES_NONE                   : return CONF_FLARES_NONE      ;
    case App::GFX_FLARES_NO_LIGHTSOURCES        : return CONF_FLARES_NO_LIGHT  ;
    case App::GFX_FLARES_CURR_VEHICLE_HEAD_ONLY : return CONF_FLARES_CURR_HEAD ;
    case App::GFX_FLARES_ALL_VEHICLES_HEAD_ONLY : return CONF_FLARES_ALL_HEADS ;
    case App::GFX_FLARES_ALL_VEHICLES_ALL_LIGHTS: return CONF_FLARES_ALL_LIGHTS;
    default                                     : return ""                    ;
    }
}

inline const char* GfxWaterToStr(App::GfxWaterMode v)
{
    switch(v)
    {
    case App::GFX_WATER_BASIC    : return CONF_WATER_BASIC    ;
    case App::GFX_WATER_REFLECT  : return CONF_WATER_REFLECT  ;
    case App::GFX_WATER_FULL_FAST: return CONF_WATER_FULL_FAST;
    case App::GFX_WATER_FULL_HQ  : return CONF_WATER_FULL_HQ  ;
    case App::GFX_WATER_HYDRAX   : return CONF_WATER_HYDRAX   ;
    default                      : return ""                  ;
    }
}

inline const char* GfxSkyToStr(App::GfxSkyMode v)
{
    switch(v)
    {
    case App::GFX_SKY_CAELUM   : return CONF_SKY_CAELUM   ;
    case App::GFX_SKY_SKYX     : return CONF_SKY_SKYX     ;
    case App::GFX_SKY_SANDSTORM: return CONF_SKY_SANDSTORM;
    default                    : return ""                ;
    }
}

inline const char* App__IoInputGrabToStr   () { return IoInputGrabToStr   (App::GetIoInputGrabMode   ()); }
inline const char* App__GfxShadowTechToStr () { return GfxShadowTechToStr (App::GetGfxShadowType     ()); }
inline const char* App__GfxExtcamModeToStr () { return GfxExtcamModeToStr (App::GetGfxExternCamMode  ()); }
inline const char* App__GfxTexFilterToStr  () { return GfxTexFilterToStr  (App::GetGfxTexFiltering   ()); }
inline const char* App__GfxVegetationToStr () { return GfxVegetationToStr (App::GetGfxVegetationMode ()); }
inline const char* App__SimGearboxToStr    () { return SimGearboxToStr    (App::GetSimGearboxMode    ()); }
inline const char* App__GfxFlaresToStr     () { return GfxFlaresToStr     (App::GetGfxFlaresMode     ()); }
inline const char* App__GfxWaterToStr      () { return GfxWaterToStr      (App::GetGfxWaterMode      ()); }
inline const char* App__GfxSkyToStr        () { return GfxSkyToStr        (App::GetGfxSkyMode        ()); }
inline int         App__GetEnvmapRate      () { return std::min(6, std::max(0, App::GetGfxEnvmapRate())); }

#define _(_V_) (_V_)
#define B(_V_) (_V_ ? "Yes" : "No")
#define Y(_V_) ((_V_ != 0) ? "Yes" : "No")

void Settings::SaveSettings()
{
    using namespace std;

    std::ofstream f(RoR::App::GetSysConfigDir() + PATH_SLASH + "RoR.cfg");
    if (!f.is_open())
    {
        LOG("[RoR] Failed to save RoR.cfg: Could not open file");
        return;
    }

    f << "; Rigs of Rods configuration file"                             << endl;
    f << "; -------------------------------"                             << endl;
    f                                                                    << endl;
    f << "; Multiplayer"                                                 << endl;
    f << CONF_MP_NET_ENABLE   << "=" << B(m_network_enable              ) << endl;
    f << CONF_MP_NICKNAME     << "=" << _(App::GetMpPlayerName        ()) << endl;
    f << CONF_MP_HOSTNAME     << "=" << _(App::GetMpServerHost        ()) << endl;
    f << CONF_MP_PORT         << "=" << _(App::GetMpServerPort        ()) << endl;
    f << CONF_MP_PASSWORD     << "=" << _(App::GetMpServerPassword    ()) << endl;
    f                                                                     << endl;
    f << "; Simulation"                                                   << endl;
    f << CONF_SIM_GEARBOX     << "=" << _(App__SimGearboxToStr        ()) << endl;
    f << CONF_SIM_MULTITHREAD << "=" << B(App::GetAppMultithread      ()) << endl;
    f                                                                     << endl;
    f << "; Input/Output"                                                 << endl;
    f << CONF_FF_ENABLED      << "=" << B(App::GetIoFFbackEnabled     ()) << endl;
    f << CONF_FF_CAMERA       << "=" << _(App::GetIoFFbackCameraGain  ()) << endl;
    f << CONF_FF_CENTERING    << "=" << _(App::GetIoFFbackCenterGain  ()) << endl;
    f << CONF_FF_GAIN         << "=" << _(App::GetIoFFbackMasterGain  ()) << endl;
    f << CONF_FF_STRESS       << "=" << _(App::GetIoFFbackStressGain  ()) << endl;
    f << CONF_INPUT_GRAB      << "=" << _(App__IoInputGrabToStr       ()) << endl;
    f << CONF_ARCADE_CONTROL  << "=" << B(App::GetIoArcadeControls    ()) << endl;
    f << CONF_OUTGAUGE_MODE   << "=" << _(App::GetIoOutGaugeMode      ()) << endl;
    f << CONF_OUTGAUGE_IP     << "=" << _(App::GetIoOutGaugeIp        ()) << endl;
    f << CONF_OUTGAUGE_PORT   << "=" << _(App::GetIoOutGaugePort      ()) << endl;
    f << CONF_OUTGAUGE_DELAY  << "=" << _(App::GetIoOutGaugeDelay     ()) << endl;
    f << CONF_OUTGAUGE_ID     << "=" << _(App::GetIoOutGaugeId        ()) << endl;
    f                                                                     << endl;
    f << "; Graphics"                                                     << endl;
    f << CONF_GFX_SHADOWS     << "=" << _(App__GfxShadowTechToStr     ()) << endl;
    f << CONF_GFX_EXTCAM      << "=" << _(App__GfxExtcamModeToStr     ()) << endl;
    f << CONF_GFX_TEX_FILTER  << "=" << _(App__GfxTexFilterToStr      ()) << endl;
    f << CONF_GFX_VEGETATION  << "=" << _(App__GfxVegetationToStr     ()) << endl;
    f << CONF_GFX_SUNBURN     << "=" << B(App::GetGfxEnableSunburn    ()) << endl;
    f << CONF_GFX_WAVES       << "=" << B(App::GetGfxWaterUseWaves    ()) << endl;
    f << CONF_MINIMAP_OFF     << "=" << B(App::GetGfxMinimapDisabled  ()) << endl;
    f << CONF_GFX_PARTICLES   << "=" << Y(App::GetGfxParticlesMode    ()) << endl;
    f << CONF_GFX_GLOW        << "=" << B(App::GetGfxEnableGlow       ()) << endl;
    f << CONF_GFX_HDR         << "=" << B(App::GetGfxEnableHdr        ()) << endl;
    f << CONF_GFX_HEATHAZE    << "=" << B(App::GetGfxUseHeathaze      ()) << endl;
    f << CONF_GFX_SKIDMARKS   << "=" << Y(App::GetGfxSkidmarksMode    ()) << endl;
    f << CONF_ENVMAP_ENABLED  << "=" << B(App::GetGfxEnvmapEnabled    ()) << endl;
    f << CONF_ENVMAP_RATE     << "=" << _(App::GetGfxEnvmapRate       ()) << endl;
    f << CONF_GFX_LIGHTS      << "=" << _(App__GfxFlaresToStr         ()) << endl;
    f << CONF_GFX_WATER_MODE  << "=" << _(App__GfxWaterToStr          ()) << endl;
    f << CONF_GFX_SIGHT_RANGE << "=" << _(App::GetGfxSightRange       ()) << endl;
    f << CONF_GFX_FOV_EXTERN  << "=" << _(m_fov_external                ) << endl;
    f << CONF_GFX_FOV_INTERN  << "=" << _(m_fov_internal                ) << endl;
    f << CONF_GFX_FPS_LIMIT   << "=" << _(App::GetGfxFpsLimit         ()) << endl;
    f << CONF_GFX_SKY_EFFECTS << "=" << _(App__GfxSkyToStr            ()) << endl;
    f                                                                     << endl;
    f << "; Audio"                                                        << endl;
    f << CONF_SOUND_VOLUME    << "=" << _(App::GetAudioMasterVolume   ()) << endl;
    f << CONF_SOUND_CREAK     << "=" << B(App::GetAudioEnableCreak    ()) << endl;
    f << CONF_MUSIC_MAIN_MENU << "=" << B(App::GetAudioMenuMusic      ()) << endl;
    f << CONF_AUDIO_DEVICE    << "=" << _(App::GetAudioDeviceName     ()) << endl;
    f                                                                     << endl;
    f << "; Diagnostics"                                                  << endl;
    f << CONF_LOG_NODE_IMPORT << "=" << B(App::GetDiagRigLogNodeImport()) << endl;
    f << CONF_LOG_NODE_STATS  << "=" << B(App::GetDiagRigLogNodeStats ()) << endl;
    f << CONF_LOG_RIG_IMPORT  << "=" << B(App::GetDiagRigLogMessages  ()) << endl;
    f << CONF_COLLISION_DBG   << "=" << B(App::GetDiagCollisions      ()) << endl;
    f << CONF_TRUCKMASS_DBG   << "=" << B(App::GetDiagTruckMass       ()) << endl;
    f << CONF_ENVMAP_DEBUG    << "=" << B(App::GetDiagEnvmap          ()) << endl;
    f << CONF_PRESELECTED_TERRAIN     << "=" << _(App::GetDiagPreselectedTerrain  ()) << endl;
    f << CONF_PRESELECTED_TRUCK       << "=" << _(App::GetDiagPreselectedVehicle  ()) << endl;
    f << CONF_PRESELECTED_TRUCK_CFG   << "=" << _(App::GetDiagPreselectedVehConfig()) << endl;
    f << CONF_PRESELECTED_TRUCK_ENTER << "=" << B(App::GetDiagPreselectedVehEnter ()) << endl;
    f                                                                     << endl;
    f << "; Application"                                                  << endl;
    f << CONF_SCREENSHOT_FMT  << "=" << _(App::GetAppScreenshotFormat ()) << endl;
    f << CONF_REPLAY_MODE     << "=" << B(App::GetSimReplayEnabled    ()) << endl;
    f << CONF_REPLAY_LENGTH   << "=" << _(App::GetSimReplayLength     ()) << endl;
    f << CONF_REPLAY_STEPPING << "=" << _(App::GetSimReplayStepping   ()) << endl;
    f << CONF_POS_STORAGE     << "=" << B(App::GetSimPositionStorage  ()) << endl;
    f << CONF_LANGUAGE_SHORT  << "=" << _(App::GetAppLocale           ()) << endl;

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

    // User directories
    std::string user_dir = App::GetSysUserDir();
    App::SetSysConfigDir     (user_dir + PATH_SLASH + "config");
    App::SetSysCacheDir      (user_dir + PATH_SLASH + "cache" );
    App::SetSysScreenshotDir (user_dir + PATH_SLASH + "screenshots" );

    // Resources dir
    std::string process_dir = App::GetSysProcessDir();
    std::string resources_dir = process_dir + PATH_SLASH + "resources";
    if (FolderExists(resources_dir))
    {
        App::SetSysResourcesDir(resources_dir);
        return true;
    }
    resources_dir = System::GetParentDirectory(process_dir.c_str());
    if (FolderExists(resources_dir))
    {
        App::SetSysResourcesDir(resources_dir);
        return true;
    }
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    resources_dir = "/usr/share/rigsofrods/resources/";
    if (FolderExists(resources_dir))
    {
        App::SetSysResourcesDir(resources_dir);
        return true;
    }
#endif
    return false;
}

