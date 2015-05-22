/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
// created by thomas fischer, 4th of January 2009
#include "Settings.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <shlobj.h> // for CoCreateGuid and SHGetFolderPathA
#include <direct.h> // for _chdir
#endif

#define _L

#include "ErrorUtils.h"
#include "Ogre.h"
#include "PlatformUtils.h"
#include "RoRVersion.h"
#include "SHA1.h"
#include "Utils.h"

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

Settings::Settings():
	m_flares_mode(-1),
	m_gearbox_mode(-1)
{
}

Settings::~Settings()
{
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
	if (key == "User Token" || key == "User Token Hash" || key == "Config Root" || key == "Cache Path" || key == "Log Path" || key == "Resources Path" || key == "Program Path")
		return "permission denied";

	return settings[key];
}

void Settings::setSettingScriptSafe(const String &key, const String &value)
{
	// hide certain settings for scripts
	if (key == "User Token" || key == "User Token Hash" || key == "Config Root" || key == "Cache Path" || key == "Log Path" || key == "Resources Path" || key == "Program Path")
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
	saveSettings(getSetting("Config Root", "")+"RoR.cfg");
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

void Settings::loadSettings(String configFile, bool overwrite)
{
	//printf("trying to load configfile: %s...\n", configFile.c_str());
	ConfigFile cfg;
	cfg.load(configFile, "=:\t", false);

	// load all settings into a map!
	ConfigFile::SettingsIterator i = cfg.getSettingsIterator();
	String svalue, sname;
	while (i.hasMoreElements())
	{
		sname = i.peekNextKey();
		svalue = i.getNext();
		if (!overwrite && settings[sname] != "") continue;
		settings[sname] = svalue;
		//logfile->AddLine(conv("### ") + conv(sname) + conv(" : ") + conv(svalue));logfile->Write();
	}
	// add a GUID if not there
	checkGUID();
	generateBinaryHash();

#ifndef NOOGRE
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
#endif // NOOGRE
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

bool Settings::get_system_paths(char *program_path, char *user_path)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// note: we enforce usage of the non-UNICODE interfaces (since its easier to integrate here)
	if (!GetModuleFileNameA(NULL, program_path, 512))
	{
		ErrorUtils::ShowError(_L("Startup error"), _L("Error while retrieving program space path"));
		return false;
	}
	GetShortPathNameA(program_path, program_path, 512); //this is legal
	path_descend(program_path);

	if (getSetting("userpath", "").empty())
	{
		if (SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, user_path)!=S_OK)
		{
			ErrorUtils::ShowError(_L("Startup error"), _L("Error while retrieving user space path"));
			return false;
		}
		GetShortPathNameA(user_path, user_path, 512); //this is legal
		sprintf(user_path, "%s\\Rigs of Rods %s\\", user_path, ROR_VERSION_STRING_SHORT); // do not use the full string, as same minor versions should share this directory
	} else
	{
		strcpy(user_path, getSetting("userpath", "").c_str());
	}

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
		path_descend(program_path);
	} else return false;
	//user path is easy
	char home_path[256];
	strncpy(home_path, getenv ("HOME"), 240);
	//sprintf(user_path, "%s/RigsOfRods/", home_path); // old version
	sprintf(user_path, "%s/.rigsofrods/", home_path);
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	//found this code, will look later
	String path = "./";
	ProcessSerialNumber PSN;
	ProcessInfoRec pinfo;
	FSSpec pspec;
	FSRef fsr;
	OSStatus err;
	/* set up process serial number */
	PSN.highLongOfPSN = 0;
	PSN.lowLongOfPSN = kCurrentProcess;
	/* set up info block */
	pinfo.processInfoLength = sizeof(pinfo);
	pinfo.processName = NULL;
	pinfo.processAppSpec = &pspec;
	/* grab the vrefnum and directory */
	
	//path = "~/RigsOfRods/";
	//strcpy(user_path, path.c_str());
	
	err = GetProcessInformation(&PSN, &pinfo);
	if (! err ) {
		char c_path[2048];
		FSSpec fss2;
		int tocopy;
		err = FSMakeFSSpec(pspec.vRefNum, pspec.parID, 0, &fss2);
		if ( ! err ) {
			err = FSpMakeFSRef(&fss2, &fsr);
			if ( ! err ) {
				err = (OSErr)FSRefMakePath(&fsr, (UInt8*)c_path, 2048);
				if (! err ) {
					path = c_path;
					path += "/";
					strcpy(program_path, path.c_str());
				}
				
				err = FSFindFolder(kOnAppropriateDisk, kDocumentsFolderType, kDontCreateFolder, &fsr);
				if (! err ) {
					FSRefMakePath(&fsr, (UInt8*)c_path, 2048);
					if (! err ) {
						path = c_path;
						path += "/Rigs\ of\ Rods/";
						strcpy(user_path, path.c_str());
					}
				}
			}
		}
	}
#endif
	return true;
}

bool Settings::setupPaths()
{
	char program_path[1024] = {};
	char resources_path[1024] = {};
	//char streams_path[1024] = {};
	char user_path[1024] = {};
	char config_root[1024] = {};
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	const char *dsStr = "\\";
#else
	const char *dsStr="/";
#endif

	if (!get_system_paths(program_path, user_path))
		return false;

	String local_config = String(program_path) + String(dsStr) + String("config");
	if (FolderExists(local_config.c_str()))
	{
		sprintf(user_path, "%s%sconfig%s",program_path, dsStr, dsStr);
	}

	// check for resource folder: first the normal version (in the executables directory)
	strcpy(resources_path, program_path);
	path_add(resources_path, "resources");
	if (! FolderExists(resources_path))
	{
		// if not existing: check one dir up (dev version)
		strcpy(resources_path, program_path);
		path_descend(resources_path);
		path_add(resources_path, "resources");
		if (! FolderExists(resources_path))
		{
			// 3rd fallback: check the installation path
#ifndef _WIN32
			// linux fallback
			// TODO: use installation patch values from CMake
			strcpy(resources_path, "/usr/share/rigsofrods/resources/");
#endif // !_WIN32

			if (! FolderExists(resources_path))
			{
				ErrorUtils::ShowError(_L("Startup error"), _L("Resources folder not found. Check if correctly installed."));
				exit(1);
			}
		}
	}

	// change working directory to executable path
#ifdef _WIN32
	_chdir(program_path);
#endif // _WIN32

	//setup config files names
	char plugins_fname[1024] = {};

#ifdef _WIN32
	// under windows, the plugins.cfg is in the installation directory
	strcpy(plugins_fname, program_path);
#else
	// under linux, the plugins.cfg is somewhere in /usr/share/rigsofrods/resources
	// we will test both locations: program and resource path
	char tmppp[1024] = "";
	strcpy(tmppp, resources_path);	
	strcat(tmppp, "plugins.cfg");
	if(FileExists(tmppp))
	{
		strcpy(plugins_fname, resources_path);
	} else
	{
		strcpy(tmppp, program_path);	
		strcat(tmppp, "plugins.cfg");
		if(FileExists(tmppp))
			strcpy(plugins_fname, program_path);
	}
	
#endif // _WIN32


#ifdef _DEBUG
	strcat(plugins_fname, "plugins_d.cfg");
#else
	strcat(plugins_fname, "plugins.cfg");
#endif

	char ogreconf_fname[1024] = {};
	strcpy(ogreconf_fname, user_path);
	path_add(ogreconf_fname, "config");
	strcpy(config_root, ogreconf_fname); //setting the config root here
	strcat(ogreconf_fname, "ogre.cfg");

	char ogrelog_fname[1024] = {};
	strcpy(ogrelog_fname, user_path);
	path_add(ogrelog_fname, "logs");

    settings["Log dir"] = ogrelog_fname;

    char profiler_out_dir[1000];
    strcpy(profiler_out_dir, user_path);
    path_add(profiler_out_dir, "profiler");
    settings["Profiler output dir"] = profiler_out_dir;

	char ogrelog_path[1024] = {};
	strcpy(ogrelog_path, ogrelog_fname);
	strcat(ogrelog_fname, "RoR.log");

	// now update our settings with the results:

	settings["dirsep"] = String(dsStr);
	settings["Config Root"] = String(config_root);
	settings["CachePath"] = String(user_path) + "cache" + String(dsStr);

	// only set log path if it was not set before
	settings["Log Path"] = String(ogrelog_path);
	settings["Resources Path"] = String(resources_path);
	settings["User Path"] = String(user_path);
	settings["Program Path"] = String(program_path);
	settings["plugins.cfg"] = String(plugins_fname);
	settings["ogre.cfg"] = String(ogreconf_fname);
	settings["ogre.log"] = String(ogrelog_fname);

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// XXX maybe use StringUtil::standardisePath here?
	// windows is case insensitive, so norm here
	StringUtil::toLowerCase(settings["Config Root"]);
	StringUtil::toLowerCase(settings["Cache Path"]);
	StringUtil::toLowerCase(settings["Log Path"]);
	StringUtil::toLowerCase(settings["Resources Path"]);
	StringUtil::toLowerCase(settings["Program Path"]);
#endif
	// now enable the user to override that:
	if (FileExists("config.cfg"))
	{
		loadSettings("config.cfg", true);

		// fix up time things...
		settings["Config Root"] = settings["User Path"]+String(dsStr)+"config"+String(dsStr);
		settings["Cache Path"]  = settings["User Path"]+String(dsStr)+"cache"+String(dsStr);
		settings["Log Path"]    = settings["User Path"]+String(dsStr)+"logs"+String(dsStr);
		settings["ogre.cfg"]    = settings["User Path"]+String(dsStr)+"config"+String(dsStr)+"ogre.cfg";
		settings["ogre.log"]    = settings["User Path"]+String(dsStr)+"logs"+String(dsStr)+"RoR.log";
	}

	if (!settings["Enforce Log Path"].empty())
	{
		settings["Log Path"] = settings["Enforce Log Path"];
		settings["ogre.log"] = settings["Log Path"]+String(dsStr)+"RoR.log";
	}

	printf(" * log path:         %s\n", settings["Log Path"].c_str());
	printf(" * config path:      %s\n", settings["Config Root"].c_str());
	printf(" * user path:        %s\n", settings["User Path"].c_str());
	printf(" * program path:     %s\n", settings["Program Path"].c_str());
	printf(" * used plugins.cfg: %s\n", settings["plugins.cfg"].c_str());
	printf(" * used ogre.cfg:    %s\n", settings["ogre.cfg"].c_str());
	printf(" * used ogre.log:    %s\n", settings["ogre.log"].c_str());

	return true;
}


void Settings::path_descend(char* path)
{
	char dirsep='/';
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep='\\';
#endif
	char* pt=path+strlen(path)-1;
	if (pt>=path && *pt==dirsep) pt--;
	while (pt>=path && *pt!=dirsep) pt--;
	if (pt>=path) *(pt+1)=0;
}

void Settings::path_add(char* path, const char* dirname)
{
	char tmp[1024];
	char dirsep='/';
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep='\\';
#endif
	sprintf(tmp, "%s%s%c", path, dirname, dirsep);
	strcpy(path, tmp);
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
