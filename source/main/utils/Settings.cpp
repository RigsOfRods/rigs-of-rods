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
    OPT_LOGPATH,
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
    { OPT_LOGPATH,        ("-logpath"),     SO_REQ_SEP },
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

Settings::Settings():
	m_flares_mode(-1),
	m_gearbox_mode(-1)
{
}

Settings::~Settings()
{
}

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
            SETTINGS.setSetting("Preselected Truck", String(args.OptionArg()));
        } 
        else if (args.OptionId() == OPT_TRUCKCONFIG) 
        {
            SETTINGS.setSetting("Preselected TruckConfig", String(args.OptionArg()));
        } 
        else if (args.OptionId() == OPT_MAP) 
        {
            RoR::App::SetPendingTerrain(args.OptionArg());
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
        else if (args.OptionId() == OPT_LOGPATH) 
        {
            SETTINGS.setSetting("Enforce Log Path", args.OptionArg());
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
            SETTINGS.setSetting("Enter Preselected Truck", "Yes");
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

void Settings::checkGUID()
{
	if (getSetting("GUID", "").empty())
		createGUID();
}

void Settings::createGUID()
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	GUID *g = new GUID();
	CoCreateGuid(g);

	char buf[120];
	sprintf(buf,"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", g->Data1,g->Data2,g->Data3,UINT(g->Data4[0]),UINT(g->Data4[1]),UINT(g->Data4[2]),UINT(g->Data4[3]),UINT(g->Data4[4]),UINT(g->Data4[5]),UINT(g->Data4[6]),UINT(g->Data4[7]));
	delete g;

	String guid = String(buf);

	// save in settings
	setSetting("GUID", guid);
	saveSettings();

#endif //OGRE_PLATFORM
}

void Settings::saveSettings()
{
	saveSettings(RoR::App::GetSysConfigDir() + PATH_SLASH + "RoR.cfg");
}

void Settings::saveSettings(String configFile)
{
	std::ofstream f(configFile.c_str());
	if (!f.is_open()) return;

	// now save the settings to RoR.cfg
	settings_map_t::iterator it;
	for (it = settings.begin(); it != settings.end(); it++)
	{
		if (it->first == "BinaryHash") continue;
		f << it->first << "=" << it->second << std::endl;
	}
	f.close();
	/*
	FILE *fd;
	Ogre::String rorcfg = configFile;
	std::map<std::string, std::string>::iterator it;

	LOG("Saving to Config file: " + rorcfg);

	const char * rorcfg_char = rorcfg.c_str();
	fd = fopen(rorcfg_char, "w");
	if (!fd)
	{
		LOG("Could not write config file");
		return;
	}
	// now save the Settings to RoR.cfg
	for (it = settings.begin(); it != settings.end(); it++)
	{
		fprintf(fd, "%s=%s\n", it->first.c_str(), it->second.c_str());
	}
	fclose(fd);*/
}

inline void App__SetShadowTech(std::string const & s)
{
    if      (s == "Texture shadows")            { App::SetGfxShadowType(App::GFX_SHADOW_TYPE_TEXTURE); }
    else if (s == "Parallel-split Shadow Maps") { App::SetGfxShadowType(App::GFX_SHADOW_TYPE_PSSM   ); }
    else                                        { App::SetGfxShadowType(App::GFX_SHADOW_TYPE_NONE   ); }
}

inline void App__SetExtcamMode(std::string const & s)
{
    if (s == "Pitching") { App::SetGfxExternCamMode(App::GFX_EXTCAM_MODE_PITCHING); return; }
    if (s == "Static")   { App::SetGfxExternCamMode(App::GFX_EXTCAM_MODE_STATIC);   return; }
    else                 { App::SetGfxExternCamMode(App::GFX_EXTCAM_MODE_NONE    ); return; }
}

inline void App__SetTexFiltering(std::string const & s)
{
    if (s == "Bilinear")                   { App::SetGfxTexFiltering(App::GFX_TEXFILTER_BILINEAR);    return; }
    if (s == "Trilinear")                  { App::SetGfxTexFiltering(App::GFX_TEXFILTER_TRILINEAR);   return; }
    if (s == "Anisotropic (best looking)") { App::SetGfxTexFiltering(App::GFX_TEXFILTER_ANISOTROPIC); return; }
}

#define STR2BOOL_(_VAL_)  Ogre::StringConverter::parseBool(_VAL_)
#define STR2FLOAT(_VAL_)  Ogre::StringConverter::parseReal(_VAL_)
#define STR2INT32(_VAL_)  Ogre::StringConverter::parseInt (_VAL_)

bool Settings::ParseGlobalVarSetting(std::string const & name, std::string const & value)
{
    // Process and erase settings which propagate to global vars.
    if (name == "Network enable" && (Ogre::StringConverter::parseBool(value) == true))
    {
        RoR::App::SetPendingMpState(RoR::App::MP_STATE_CONNECTED);
        return true;
    }
    else if (name == "Preselected Map"         ) { App::SetPendingTerrain            (value);  return true; }
    else if (name == "Nickname"                ) { App::SetMpPlayerName              (value);  return true; }
    else if (name == "Server name"             ) { App::SetMpServerHost              (value);  return true; }
    else if (name == "Server port"             ) { App::SetMpServerPort    (STR2INT32(value)); return true; }
    else if (name == "Server password"         ) { App::SetMpServerPassword          (value);  return true; }
    // Input
    else if (name == "Force Feedback"          ) { App::SetInputFFEnabled  (STR2BOOL_(value)); return true; }
    else if (name == "Force Feedback Camera"   ) { App::SetInputFFCamera   (STR2FLOAT(value)); return true; }
    else if (name == "Force Feedback Centering") { App::SetInputFFCentering(STR2FLOAT(value)); return true; }
    else if (name == "Force Feedback Gain"     ) { App::SetInputFFGain     (STR2FLOAT(value)); return true; }
    else if (name == "Force Feedback Stress"   ) { App::SetInputFFStress   (STR2FLOAT(value)); return true; }
    // Gfx
    else if (name == "Shadow technique"        ) { App__SetShadowTech                (value);  return true; }
    else if (name == "External Camera Mode"    ) { App__SetExtcamMode                (value);  return true; }
    else if (name == "Texture Filtering"       ) { App__SetTexFiltering              (value);  return true; }

    return false;
}

void Settings::loadSettings(String configFile, bool overwrite)
{
	ConfigFile cfg;
	cfg.load(configFile, "=:\t", false);

	// load all settings into a map!
	ConfigFile::SettingsIterator i = cfg.getSettingsIterator();
	String s_value, s_name;
	while (i.hasMoreElements())
	{
		s_name  = RoR::Utils::SanitizeUtf8String(i.peekNextKey());
		s_value = RoR::Utils::SanitizeUtf8String(i.getNext());

        // Purge unwanted entries
        if (s_name == "Program Path") { continue; }

        if (this->ParseGlobalVarSetting(s_name, s_value))
        {
            continue;
        }

		if (!overwrite && !settings[s_name].empty())
		{
			continue;
		}
		settings[s_name] = s_value;
	}
	// add a GUID if not there
	checkGUID();
	generateBinaryHash();

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

int Settings::generateBinaryHash()
{
#ifndef NOOGRE
	char program_path[1024]="";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// note: we enforce usage of the non-UNICODE interfaces (since its easier to integrate here)
	if (!GetModuleFileNameA(NULL, program_path, 512))
	{
		ErrorUtils::ShowError(_L("Startup error"), _L("Error while retrieving program space path"));
		return 1;
	}
	GetShortPathNameA(program_path, program_path, 512); //this is legal

#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	//true program path is impossible to get from POSIX functions
	//lets hack!
	pid_t pid = getpid();
	char procpath[256];
	sprintf(procpath, "/proc/%d/exe", pid);
	int ch = readlink(procpath,program_path,240);
	if (ch != -1)
	{
		program_path[ch] = 0;
	} else return 1;
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	// TO BE DONE
#endif //OGRE_PLATFORM
	// now hash ourself
	{
		char hash_result[250];
		memset(hash_result, 0, 249);
		RoR::CSHA1 sha1;
		sha1.HashFile(program_path);
		sha1.Final();
		sha1.ReportHash(hash_result, RoR::CSHA1::REPORT_HEX_SHORT);
		setSetting("BinaryHash", String(hash_result));
	}
#endif //NOOGRE
	return 0;
}
 
bool Settings::SetupAllPaths()
{
    using namespace RoR;
    std::string user_dir = App::GetSysUserDir();
        
    App::SetSysConfigDir(user_dir + PATH_SLASH + "config");
    App::SetSysCacheDir (user_dir + PATH_SLASH + "cache" );
    
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

int Settings::GetFlaresMode(int default_value /*=2*/)
{
	if (m_flares_mode == -1) // -1: unknown, -2: default, 0+: mode ID
	{
		auto itor = settings.find("Lights");
		if (itor == settings.end())
		{
			m_flares_mode = -2;
		}
		else
		{
			if      (itor->second == "None (fastest)")                    { m_flares_mode = 0; }
			else if (itor->second == "No light sources")                  { m_flares_mode = 1; }
			else if (itor->second == "Only current vehicle, main lights") { m_flares_mode = 2; }
			else if (itor->second == "All vehicles, main lights")         { m_flares_mode = 3; }
			else if (itor->second == "All vehicles, all lights")          { m_flares_mode = 4; }

			else                                                          { m_flares_mode = -2; }
		}
	}
	if (m_flares_mode == -2)
	{
		return default_value;
	}
	return m_flares_mode;
}

int Settings::GetGearBoxMode(int default_value /*=0*/)
{
	if (m_gearbox_mode == -1) // -1: unknown, -2: default, 0+: mode ID
	{
		auto itor = settings.find("GearboxMode");
		if (itor == settings.end())
		{
			m_gearbox_mode = -2;
		}
		else
		{
			if (itor->second == "Automatic shift")	{ m_gearbox_mode = 0; }
			else if (itor->second == "Manual shift - Auto clutch")	{ m_gearbox_mode = 1; }
			else if (itor->second == "Fully Manual: sequential shift")	{ m_gearbox_mode = 2; }
			else if (itor->second == "Fully manual: stick shift")	{ m_gearbox_mode = 3; }
			else if (itor->second == "Fully Manual: stick shift with ranges")	{ m_gearbox_mode = 4; }

			else { m_gearbox_mode = -2; }
		}
	}
	if (m_gearbox_mode == -2)
	{
		return default_value;
	}
	return m_gearbox_mode;
}
