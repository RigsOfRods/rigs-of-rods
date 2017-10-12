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

#include "conf_file.h"

#include <Ogre.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <shlobj.h> // for CoCreateGuid and SHGetFolderPathA
#include <direct.h> // for _chdir
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include <mach-o/dyld.h>
#endif

#define _L

#include "ErrorUtils.h"
#include "PlatformUtils.h"
#include "RoRVersion.h"

#include "utf8/checked.h"
#include "utf8/unchecked.h"

using namespace Ogre;

std::string SanitizeUtf8String(std::string const& str_in)
{
    // Cloned from UTFCPP tutorial: http://utfcpp.sourceforge.net/#fixinvalid
    std::string str_out;
    utf8::replace_invalid(str_in.begin(), str_in.end(), std::back_inserter(str_out));
    return str_out;
}

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

void Settings::setSetting(String key, String value)
{
    settings[key] = value;
}

void Settings::setUTFSetting(UTFString key, UTFString value)
{
    settings[key] = value;
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
        s_name  = SanitizeUtf8String(i.peekNextKey());
        s_value = SanitizeUtf8String(i.getNext());
        if (!overwrite && !settings[s_name].empty())
        {
            continue;
        }
        settings[s_name] = s_value;
    }
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
    int cx;

    // program path
    const int len = 256;
    uint32_t lenb = (uint32_t) len;
    char procpath[len];
    
    if (_NSGetExecutablePath(procpath, &lenb) == -1)
        return false;

    /*  
     *  _NSGetExecutablePath returns the absolute path to the binary so procpath
     *  has "./RoR" at its end. We only want the path to the directory so we
     *  cut off the last 5 characters of the string.
     */
    assert(strlen(procpath) > 5);
    procpath[strlen(procpath) - 4] = '\0';

    cx = snprintf(program_path, len, "%s", procpath);
    if ( cx < 0 || cx >= len)
        return false;
    
    // user path
    cx = snprintf(user_path, 256, "%s/RigsOfRods/", getenv("HOME"));
    if ( cx < 0 || cx >= 256)
        return false;
#endif
    return true;
}

bool Settings::setupPaths()
{
    char program_path[1024] = {};
    //char streams_path[1024] = {};
    char user_path[1024] = {};
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

    // change working directory to executable path
#ifdef _WIN32
    _chdir(program_path);
#endif // _WIN32


    // now update our settings with the results:

    // only set log path if it was not set before
    m_user_path = user_path;
    m_program_path = program_path;

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

