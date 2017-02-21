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

#pragma once

#include "RoRPrerequisites.h"

#include "Singleton.h"

// some shortcuts to improve code readability
#define SETTINGS          Settings::getSingleton()
#define SSETTING(x, y)    Settings::getSingleton().getSetting(x, y)            //<! get string setting
#define UTFSSETTING(x, y) Settings::getSingleton().getUTFSetting(x, y)         //<! get UTF string setting
#define BSETTING(x, y)    Settings::getSingleton().getBooleanSetting(x, y)     //<! get boolean setting
#define ISETTING(x, y)    Settings::getSingleton().getIntegerSetting(x, y)     //<! get int setting
#define FSETTING(x, y)    Settings::getSingleton().getFloatSetting(x, y)       //<! get float setting

std::string SanitizeUtf8String(std::string const& str_in);

class Settings : public RoRSingleton<Settings>, public ZeroedMemoryAllocator
{
    friend class RoRSingleton<Settings>;

public:

    Ogre::String getSetting(Ogre::String key, Ogre::String defaultValue);
    Ogre::UTFString getUTFSetting(Ogre::UTFString key, Ogre::UTFString defaultValue);
    bool getBooleanSetting(Ogre::String key, bool defaultValue);
    float getFloatSetting(Ogre::String key, float defaultValue);
    int getIntegerSetting(Ogre::String key, int defaultValue);

    void setSetting(Ogre::String key, Ogre::String value);
    void setUTFSetting(Ogre::UTFString key, Ogre::UTFString value);

    bool setupPaths();
    void loadSettings(Ogre::String configFile, bool overwrite=false);

    std::map<Ogre::String, Ogre::String> GetSettingMap()
    {
        return settings;
    }
    std::string GetProgramPath() { return m_program_path; }
    std::string GetUserPath()    { return m_user_path; }

protected:

    static Settings* myInstance;

    // members
    // TODO: use wide char / UTFString ...
    typedef std::map<Ogre::String, Ogre::String> settings_map_t;
    settings_map_t settings;
    std::string m_program_path;
    std::string m_user_path;

    // methods
    void path_descend(char* path);
    void path_add(char* path, const char* dirname);

    bool get_system_paths(char *program_path, char *user_path);

};
